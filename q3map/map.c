/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// map.c

#include "qbsp.h"


int			entitySourceBrushes;		// to track editor brush numbers

int			numMapPatches;

// undefine to make plane finding use linear sort
#define	USE_HASHING
#define	PLANE_HASHES	1024
plane_t		*planehash[PLANE_HASHES];

plane_t		mapplanes[MAX_MAP_PLANES];
int			nummapplanes;

// as brushes and patches are read in, the shaders are stored out in order
// here, so -onlytextures can just copy them out over the existing shaders
// in the drawSurfaces
char		mapIndexedShaders[MAX_MAP_BRUSHSIDES][MAX_QPATH];
int			numMapIndexedShaders;

vec3_t		map_mins, map_maxs;

entity_t	*mapent;



int		c_boxbevels;
int		c_edgebevels;

int		c_areaportals;
int		c_detail;
int		c_structural;

// brushes are parsed into a temporary array of sides,
// which will have the bevels added and duplicates
// removed before the final brush is allocated
bspbrush_t	*buildBrush;


void TestExpandBrushes (void);
void SetTerrainTextures( void );
void ParseTerrain( void );


/*
=============================================================================

PLANE FINDING

=============================================================================
*/


/*
================
PlaneEqual
================
*/
#define	NORMAL_EPSILON	0.00001
#define	DIST_EPSILON	0.01
qboolean	PlaneEqual (plane_t *p, vec3_t normal, vec_t dist)
{
#if 1
	if (
	   fabs(p->normal[0] - normal[0]) < NORMAL_EPSILON
	&& fabs(p->normal[1] - normal[1]) < NORMAL_EPSILON
	&& fabs(p->normal[2] - normal[2]) < NORMAL_EPSILON
	&& fabs(p->dist - dist) < DIST_EPSILON )
		return qtrue;
#else
	if (p->normal[0] == normal[0]
		&& p->normal[1] == normal[1]
		&& p->normal[2] == normal[2]
		&& p->dist == dist)
		return qtrue;
#endif
	return qfalse;
}

/*
================
AddPlaneToHash
================
*/
void	AddPlaneToHash (plane_t *p)
{
	int		hash;

	hash = (int)fabs(p->dist) / 8;
	hash &= (PLANE_HASHES-1);

	p->hash_chain = planehash[hash];
	planehash[hash] = p;
}

/*
================
CreateNewFloatPlane
================
*/
int CreateNewFloatPlane (vec3_t normal, vec_t dist)
{
	plane_t	*p, temp;

	if (VectorLength(normal) < 0.5)
	{
		_printf( "FloatPlane: bad normal\n");
		return -1;
	}

	// create a new plane
	if (nummapplanes+2 > MAX_MAP_PLANES)
		Error ("MAX_MAP_PLANES");

	p = &mapplanes[nummapplanes];
	VectorCopy (normal, p->normal);
	p->dist = dist;
	p->type = (p+1)->type = PlaneTypeForNormal (p->normal);

	VectorSubtract (vec3_origin, normal, (p+1)->normal);
	(p+1)->dist = -dist;

	nummapplanes += 2;

	// allways put axial planes facing positive first
	if (p->type < 3)
	{
		if (p->normal[0] < 0 || p->normal[1] < 0 || p->normal[2] < 0)
		{
			// flip order
			temp = *p;
			*p = *(p+1);
			*(p+1) = temp;

			AddPlaneToHash (p);
			AddPlaneToHash (p+1);
			return nummapplanes - 1;
		}
	}

	AddPlaneToHash (p);
	AddPlaneToHash (p+1);
	return nummapplanes - 2;
}

/*
==============
SnapVector
==============
*/
void	SnapVector (vec3_t normal)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(normal[i] - 1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = 1;
			break;
		}
		if ( fabs(normal[i] - -1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = -1;
			break;
		}
	}
}

/*
==============
SnapPlane
==============
*/
void	SnapPlane (vec3_t normal, vec_t *dist)
{
	SnapVector (normal);

	if (fabs(*dist-Q_rint(*dist)) < DIST_EPSILON)
		*dist = Q_rint(*dist);
}

/*
=============
FindFloatPlane

=============
*/
#ifndef USE_HASHING
int		FindFloatPlane (vec3_t normal, vec_t dist)
{
	int		i;
	plane_t	*p;

	SnapPlane (normal, &dist);
	for (i=0, p=mapplanes ; i<nummapplanes ; i++, p++)
	{
		if (PlaneEqual (p, normal, dist))
			return i;
	}

	return CreateNewFloatPlane (normal, dist);
}
#else
int		FindFloatPlane (vec3_t normal, vec_t dist)
{
	int		i;
	plane_t	*p;
	int		hash, h;

	SnapPlane (normal, &dist);
	hash = (int)fabs(dist) / 8;
	hash &= (PLANE_HASHES-1);

	// search the border bins as well
	for (i=-1 ; i<=1 ; i++)
	{
		h = (hash+i)&(PLANE_HASHES-1);
		for (p = planehash[h] ; p ; p=p->hash_chain)
		{
			if (PlaneEqual (p, normal, dist))
				return p-mapplanes;
		}
	}

	return CreateNewFloatPlane (normal, dist);
}
#endif

/*
================
MapPlaneFromPoints
================
*/
int MapPlaneFromPoints (vec3_t p0, vec3_t p1, vec3_t p2) {
	vec3_t	t1, t2, normal;
	vec_t	dist;

	VectorSubtract (p0, p1, t1);
	VectorSubtract (p2, p1, t2);
	CrossProduct (t1, t2, normal);
	VectorNormalize (normal, normal);

	dist = DotProduct (p0, normal);

	return FindFloatPlane (normal, dist);
}


//====================================================================

/*
===========
SetBrushContents

The contents on all sides of a brush should be the same
Sets contentsShader, contents, opaque, and detail
===========
*/
void SetBrushContents( bspbrush_t *b ) {
	int			contents, c2;
	side_t		*s;
	int			i;
	qboolean	mixed;
	int			allFlags;

	s = &b->sides[0];
	contents = s->contents;
	b->contentShader = s->shaderInfo;
	mixed = qfalse;

	allFlags = 0;

	for ( i=1 ; i<b->numsides ; i++, s++ ) {
		s = &b->sides[i];

		if ( !s->shaderInfo ) {
			continue;
		}

		c2 = s->contents;
		if (c2 != contents) {
			mixed = qtrue;
		}

		allFlags |= s->surfaceFlags;
	}

	if ( mixed ) {
		qprintf ("Entity %i, Brush %i: mixed face contents\n"
			, b->entitynum, b->brushnum);
	}

	if ( ( contents & CONTENTS_DETAIL ) && ( contents & CONTENTS_STRUCTURAL ) ) {
		_printf ("Entity %i, Brush %i: mixed CONTENTS_DETAIL and CONTENTS_STRUCTURAL\n"
			, num_entities-1, entitySourceBrushes );
		contents &= ~CONTENTS_DETAIL;
	}

	// the fulldetail flag will cause detail brushes to be
	// treated like normal brushes
	if ( fulldetail ) {
		contents &= ~CONTENTS_DETAIL;
	}

	// all translucent brushes that aren't specirically made structural will
	// be detail
	if ( ( contents & CONTENTS_TRANSLUCENT ) && !( contents & CONTENTS_STRUCTURAL ) ) {
		contents |= CONTENTS_DETAIL;
	}

	if ( contents & CONTENTS_DETAIL ) {
		c_detail++;
		b->detail = qtrue;
	} else {
		c_structural++;
		b->detail = qfalse;
	}

	if ( contents & CONTENTS_TRANSLUCENT ) {
		b->opaque = qfalse;
	} else {
		b->opaque = qtrue;
	}

	if ( contents & CONTENTS_AREAPORTAL ) {
		c_areaportals++;
	}

	b->contents = contents;
}


//============================================================================

/*
=================
AddBrushBevels

Adds any additional planes necessary to allow the brush being
built to be expanded against axial bounding boxes
=================
*/
void AddBrushBevels( void ) {
	int		axis, dir;
	int		i, order;
	side_t	sidetemp;
	side_t	*s;
	vec3_t	normal;
	float	dist;

	//
	// add the axial planes
	//
	order = 0;
	for (axis=0 ; axis <3 ; axis++)
	{
		for (dir=-1 ; dir <= 1 ; dir+=2, order++)
		{
			// see if the plane is allready present
			for ( i=0, s=buildBrush->sides ; i < buildBrush->numsides ; i++,s++ ) {
				if (mapplanes[s->planenum].normal[axis] == dir)
					break;
			}

			if (i == buildBrush->numsides )
			{	// add a new side
				if ( buildBrush->numsides == MAX_BUILD_SIDES ) {
					Error( "MAX_BUILD_SIDES" );
				}
				memset( s, 0, sizeof( *s ) );
				buildBrush->numsides++;
				VectorClear (normal);
				normal[axis] = dir;
				if (dir == 1)
					dist = buildBrush->maxs[axis];
				else
					dist = -buildBrush->mins[axis];
				s->planenum = FindFloatPlane (normal, dist);
				s->contents = buildBrush->sides[0].contents;
				s->bevel = qtrue;
				c_boxbevels++;
			}

			// if the plane is not in it canonical order, swap it
			if (i != order)
			{
				sidetemp = buildBrush->sides[order];
				buildBrush->sides[order] = buildBrush->sides[i];
				buildBrush->sides[i] = sidetemp;
			}
		}
	}

	//
	// add the edge bevels
	//
	if ( buildBrush->numsides == 6 ) {
		return;		// pure axial
  } else {
	  int			j, k, l;
	  float		d;
	  winding_t	*w, *w2;
	  side_t		*s2;
	  vec3_t		vec, vec2;

	  // test the non-axial plane edges
	  // this code tends to cause some problems...
	  for (i=6 ; i<buildBrush->numsides ; i++)
	  {
		  s = buildBrush->sides + i;
		  w = s->winding;
		  if (!w)
			  continue;
		  for (j=0 ; j<w->numpoints ; j++)
		  {
			  k = (j+1)%w->numpoints;
			  VectorSubtract (w->p[j], w->p[k], vec);
			  if (VectorNormalize (vec, vec) < 0.5)
				  continue;
			  SnapVector (vec);
			  for (k=0 ; k<3 ; k++)
				  if ( vec[k] == -1 || vec[k] == 1)
					  break;	// axial
			  if (k != 3)
				  continue;	// only test non-axial edges

			  // try the six possible slanted axials from this edge
			  for (axis=0 ; axis <3 ; axis++)
			  {
				  for (dir=-1 ; dir <= 1 ; dir+=2)
				  {
					  // construct a plane
					  VectorClear (vec2);
					  vec2[axis] = dir;
					  CrossProduct (vec, vec2, normal);
					  if (VectorNormalize (normal, normal) < 0.5)
						  continue;
					  dist = DotProduct (w->p[j], normal);

					  // if all the points on all the sides are
					  // behind this plane, it is a proper edge bevel
					  for (k=0 ; k < buildBrush->numsides ; k++)
					  {
						  // if this plane has allready been used, skip it
						  if (PlaneEqual (&mapplanes[buildBrush->sides[k].planenum]
							  , normal, dist) )
							  break;

						  w2 = buildBrush->sides[k].winding;
						  if (!w2)
							  continue;
						  for (l=0 ; l<w2->numpoints ; l++)
						  {
							  d = DotProduct (w2->p[l], normal) - dist;
							  if (d > 0.1)
								  break;	// point in front
						  }
						  if (l != w2->numpoints)
							  break;
					  }

					  if (k != buildBrush->numsides)
						  continue;	// wasn't part of the outer hull
					  // add this plane
					  if ( buildBrush->numsides == MAX_BUILD_SIDES ) {
						  Error( "MAX_BUILD_SIDES" );
					  }

					  s2 = &buildBrush->sides[buildBrush->numsides];
					  buildBrush->numsides++;
					  memset( s2, 0, sizeof( *s2 ) );

					  s2->planenum = FindFloatPlane (normal, dist);
					  s2->contents = buildBrush->sides[0].contents;
					  s2->bevel = qtrue;
					  c_edgebevels++;
				  }
			  }
		  }
	  }
  }
}

/*
===============
AddBackSides

fog volumes need to have inside faces created
===============
*/
void AddBackSides( void ) {
/*
	bspbrush_t	*b;
	int			i, originalSides;
	side_t		*s;
	side_t		*newSide;

	b = buildBrush;
	originalSides = b->numsides;
	for ( i = 0 ; i < originalSides ; i++ ) {
		s = &b->sides[i];
		if ( !s->shaderInfo ) {
			continue;
		}
		if ( !(s->shaderInfo->contents & CONTENTS_FOG) ) {
			continue;
		}

		// duplicate the up-facing side
		if ( mapplanes[ s->planenum ].normal[2] == 1 ) {
			newSide = &b->sides[ b->numsides ];
			b->numsides++;

			*newSide = *s;
			newSide->backSide = qtrue;
			newSide->planenum = s->planenum ^ 1;	// opposite side
		}
	}
*/
}

/*
===============
FinishBrush

Produces a final brush based on the buildBrush->sides array
and links it to the current entity
===============
*/
bspbrush_t *FinishBrush( void ) {
	bspbrush_t	*b;

	// liquids may need to have extra sides created for back sides
	AddBackSides();

	// create windings for sides and bounds for brush
	if ( !CreateBrushWindings( buildBrush ) ) {
		// don't keep this brush
		return NULL;
	}

	// brushes that will not be visible at all are forced to be detail
	if ( buildBrush->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
	{
		buildBrush->detail = qtrue;
		c_detail++;
	}

	//
	// origin brushes are removed, but they set
	// the rotation origin for the rest of the brushes
	// in the entity.  After the entire entity is parsed,
	// the planenums and texinfos will be adjusted for
	// the origin brush
	//
	if ( buildBrush->contents & CONTENTS_ORIGIN )
	{
		char	string[32];
		vec3_t	origin;

		if (num_entities == 1) {
			_printf ("Entity %i, Brush %i: origin brushes not allowed in world\n"
				,  num_entities - 1, entitySourceBrushes);
			return NULL;
		}

		VectorAdd (buildBrush->mins, buildBrush->maxs, origin);
		VectorScale (origin, 0.5, origin);

		sprintf (string, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue (&entities[num_entities - 1], "origin", string);

		VectorCopy (origin, entities[num_entities - 1].origin);

		// don't keep this brush
		return NULL;
	}

	if ( buildBrush->contents & CONTENTS_AREAPORTAL ) {
		if (num_entities != 1) {
			_printf ("Entity %i, Brush %i: areaportals only allowed in world\n"
				,  num_entities - 1, entitySourceBrushes);
			return NULL;
		}
	}

	AddBrushBevels ();

	// keep it
	b = CopyBrush( buildBrush );

	b->entitynum = num_entities-1;
	b->brushnum = entitySourceBrushes;

	b->original = b;

	b->next = mapent->brushes;
	mapent->brushes = b;

	return b;
}

//======================================================================


/*
==================
textureAxisFromPlane
==================
*/
vec3_t	baseaxis[18] =
{
{0,0,1}, {1,0,0}, {0,-1,0},			// floor
{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
{1,0,0}, {0,1,0}, {0,0,-1},			// west wall
{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
{0,1,0}, {1,0,0}, {0,0,-1},			// south wall
{0,-1,0}, {1,0,0}, {0,0,-1}			// north wall
};

void TextureAxisFromPlane(plane_t *pln, vec3_t xv, vec3_t yv)
{
	int		bestaxis;
	vec_t	dot,best;
	int		i;
	
	best = 0;
	bestaxis = 0;
	
	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct (pln->normal, baseaxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	VectorCopy (baseaxis[bestaxis*3+1], xv);
	VectorCopy (baseaxis[bestaxis*3+2], yv);
}



/*
=================
QuakeTextureVecs

Creates world-to-texture mapping vecs for crappy quake plane arrangements
=================
*/
void QuakeTextureVecs( 	plane_t *plane, vec_t shift[2], vec_t rotate, vec_t scale[2],
					  vec_t mappingVecs[2][4] ) {
 
	vec3_t	vecs[2];
	int		sv, tv;
	vec_t	ang, sinv, cosv;
	vec_t	ns, nt;
	int		i, j;

	TextureAxisFromPlane(plane, vecs[0], vecs[1]);

	if (!scale[0])
		scale[0] = 1;
	if (!scale[1])
		scale[1] = 1;

	// rotate axis
	if (rotate == 0)
		{ sinv = 0 ; cosv = 1; }
	else if (rotate == 90)
		{ sinv = 1 ; cosv = 0; }
	else if (rotate == 180)
		{ sinv = 0 ; cosv = -1; }
	else if (rotate == 270)
		{ sinv = -1 ; cosv = 0; }
	else
	{	
		ang = rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (vecs[0][0])
		sv = 0;
	else if (vecs[0][1])
		sv = 1;
	else
		sv = 2;
				
	if (vecs[1][0])
		tv = 0;
	else if (vecs[1][1])
		tv = 1;
	else
		tv = 2;
					
	for (i=0 ; i<2 ; i++) {
		ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
		nt = sinv * vecs[i][sv] +  cosv * vecs[i][tv];
		vecs[i][sv] = ns;
		vecs[i][tv] = nt;
	}

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			mappingVecs[i][j] = vecs[i][j] / scale[i];

	mappingVecs[0][3] = shift[0];
	mappingVecs[1][3] = shift[1];
}

//======================================================================

/*
=================
ParseRawBrush

Just parses the sides into buildBrush->sides[], nothing else.
no validation, back plane removal, etc.

Timo - 08/26/99
added brush epairs parsing ( ignoring actually )
Timo - 08/04/99
added exclusive brush primitive parsing
Timo - 08/08/99
support for old brush format back in
NOTE : it would be "cleaner" to have seperate functions to parse between old and new brushes
=================
*/
void	ParseRawBrush( ) {
	side_t		*side;
	vec3_t		planepts[3];
	int			planenum;
	shaderInfo_t	*si;
	// old brushes
	vec_t		shift[2];
	vec_t		rotate;
	vec_t		scale[2];
	char		name[MAX_QPATH];
	char		shader[MAX_QPATH];
	int			flags;

	buildBrush->numsides = 0;
	buildBrush->detail = qfalse;

	if (g_bBrushPrimit==BPRIMIT_NEWBRUSHES)
		MatchToken( "{" );

	do
	{
		if (!GetToken (qtrue))
			break;
		if (!strcmp (token, "}") )
			break;
		//Timo : brush primitive : here we may have to jump over brush epairs ( only used in editor )
		if (g_bBrushPrimit==BPRIMIT_NEWBRUSHES)
		{
			do
			{
				if (strcmp (token, "(") )
					GetToken( qfalse );
				else
					break;
				GetToken( qtrue );
			} while (1);
		}
		UnGetToken();

		if ( buildBrush->numsides == MAX_BUILD_SIDES ) {
			Error( "MAX_BUILD_SIDES" );
		}

		side = &buildBrush->sides[ buildBrush->numsides ];
		memset( side, 0, sizeof( *side ) );
		buildBrush->numsides++;

		// read the three point plane definition
		Parse1DMatrix( 3, planepts[0] );
		Parse1DMatrix( 3, planepts[1] );
		Parse1DMatrix( 3, planepts[2] );

		if (g_bBrushPrimit==BPRIMIT_NEWBRUSHES)
			// read the texture matrix
			Parse2DMatrix( 2, 3, (float *)side->texMat );

		// read the texturedef
		GetToken (qfalse);
		strcpy (name, token);

		// save the shader name for retexturing
		if ( numMapIndexedShaders == MAX_MAP_BRUSHSIDES ) {
			Error( "MAX_MAP_BRUSHSIDES" );
		}
		strcpy( mapIndexedShaders[numMapIndexedShaders], name );
		numMapIndexedShaders++;

		if (g_bBrushPrimit==BPRIMIT_OLDBRUSHES)
		{
			GetToken (qfalse);
			shift[0] = atoi(token);
			GetToken (qfalse);
			shift[1] = atoi(token);
			GetToken (qfalse);
			rotate = atoi(token);	
			GetToken (qfalse);
			scale[0] = atof(token);
			GetToken (qfalse);
			scale[1] = atof(token);
		}

		// find default flags and values
		sprintf( shader, "textures/%s", name );
		si = ShaderInfoForShader( shader );
		side->shaderInfo = si;
		side->surfaceFlags = si->surfaceFlags;
		side->value = si->value;
		side->contents = si->contents;

		// allow override of default flags and values
		// in Q3, the only thing you can override is DETAIL
		if (TokenAvailable())
		{
			GetToken (qfalse);
//			side->contents = atoi(token);
			flags = atoi(token);
			if ( flags & CONTENTS_DETAIL ) {
				side->contents |= CONTENTS_DETAIL;
			}

			GetToken (qfalse);
//			td.flags = atoi(token);

			GetToken (qfalse);
//			td.value = atoi(token);
		}


		// find the plane number
		planenum = MapPlaneFromPoints (planepts[0], planepts[1], planepts[2]);
		side->planenum = planenum;

		if (g_bBrushPrimit==BPRIMIT_OLDBRUSHES)
			// get the texture mapping for this texturedef / plane combination
			QuakeTextureVecs( &mapplanes[planenum], shift, rotate, scale, side->vecs );

	} while (1);

	if (g_bBrushPrimit==BPRIMIT_NEWBRUSHES)
	{
		UnGetToken();
		MatchToken( "}" );
		MatchToken( "}" );
	}
}

/*
=================
RemoveDuplicateBrushPlanes

Returns false if the brush has a mirrored set of planes,
meaning it encloses no volume.
Also removes planes without any normal
=================
*/
qboolean RemoveDuplicateBrushPlanes( bspbrush_t * b ) {
	int			i, j, k;
	side_t		*sides;

	sides = b->sides;

	for ( i = 1 ; i < b->numsides ; i++ ) {

		// check for a degenerate plane
		if ( sides[i].planenum == -1) {
			_printf ("Entity %i, Brush %i: degenerate plane\n"
				, b->entitynum, b->brushnum);
			// remove it
			for ( k = i + 1 ; k < b->numsides ; k++ ) {
				sides[k-1] = sides[k];
			}
			b->numsides--;
			i--;
			continue;
		}

		// check for duplication and mirroring
		for ( j = 0 ; j < i ; j++ ) {
			if ( sides[i].planenum == sides[j].planenum ) {
				_printf ("Entity %i, Brush %i: duplicate plane\n"
					, b->entitynum, b->brushnum);
				// remove the second duplicate
				for ( k = i + 1 ; k < b->numsides ; k++ ) {
					sides[k-1] = sides[k];
				}
				b->numsides--;
				i--;
				break;
			}

			if ( sides[i].planenum == (sides[j].planenum ^ 1) ) {
				// mirror plane, brush is invalid
				_printf ("Entity %i, Brush %i: mirrored plane\n"
					, b->entitynum, b->brushnum);
				return qfalse;
			}
		}
	}
	return qtrue;
}


/*
=================
ParseBrush

  qboolean parameter to true -> parse new brush primitive format ( else use old format )
=================
*/
void ParseBrush (void) {
	bspbrush_t	*b;

	ParseRawBrush();

	buildBrush->portalareas[0] = -1;
	buildBrush->portalareas[1] = -1;
	buildBrush->entitynum = num_entities-1;
	buildBrush->brushnum = entitySourceBrushes;

	// if there are mirrored planes, the entire brush is invalid
	if ( !RemoveDuplicateBrushPlanes( buildBrush ) ) {
		return;
	}

	// get the content for the entire brush
	SetBrushContents( buildBrush );

	// allow detail brushes to be removed 
	if (nodetail && (buildBrush->contents & CONTENTS_DETAIL) ) {
		FreeBrush( buildBrush );
		return;
	}

	// allow water brushes to be removed
	if (nowater && (buildBrush->contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) ) {
		FreeBrush( buildBrush );
		return;
	}

	b = FinishBrush( );
	if ( !b ) {
		return;
	}
}


/*
================
MoveBrushesToWorld

Takes all of the brushes from the current entity and
adds them to the world's brush list.

Used by func_group
================
*/
void MoveBrushesToWorld (entity_t *mapent) {
	bspbrush_t	*b, *next;
	parseMesh_t	*pm;

	// move brushes
	for ( b = mapent->brushes ; b ; b = next ) {
		next = b->next;

		b->next = entities[0].brushes;
		entities[0].brushes = b;
	}
	mapent->brushes = NULL;

	// move patches
	if ( mapent->patches ) {

		for ( pm = mapent->patches ; pm->next ; pm = pm->next ) {
		}

		pm->next = entities[0].patches;
		entities[0].patches = mapent->patches;

		mapent->patches = NULL;
	}
}


/*
================
AdjustBrushesForOrigin
================
*/
void AdjustBrushesForOrigin( entity_t *ent ) {
	bspbrush_t	*b;
	int			i;
	side_t		*s;
	vec_t		newdist;
	parseMesh_t	*p;

	for ( b = ent->brushes ; b ; b = b->next ) {
		for (i=0 ; i<b->numsides ; i++) {
			s = &b->sides[i];
			newdist = mapplanes[s->planenum].dist -
				DotProduct (mapplanes[s->planenum].normal, ent->origin);
			s->planenum = FindFloatPlane (mapplanes[s->planenum].normal, newdist);
		}
		CreateBrushWindings(b);
	}

	for ( p = ent->patches ; p ; p = p->next ) {
		for ( i = 0 ; i < p->mesh.width*p->mesh.height ; i++ ) {
			VectorSubtract( p->mesh.verts[i].xyz, ent->origin, p->mesh.verts[i].xyz );
		}
	}

}

/*
================
ParseMapEntity
================
*/
qboolean	ParseMapEntity (void) {
	epair_t		*e;

	if (!GetToken (qtrue))
		return qfalse;

	if (strcmp (token, "{") )
	{
		Error ("ParseEntity: { not found, found %s on line %d - last entity was at: <%4.2f, %4.2f, %4.2f>...", token, scriptline, entities[num_entities].origin[0], entities[num_entities].origin[1], entities[num_entities].origin[2]);
	}
	
	if (num_entities == MAX_MAP_ENTITIES)
		Error ("num_entities == MAX_MAP_ENTITIES");

	entitySourceBrushes = 0;

	mapent = &entities[num_entities];
	num_entities++;
	memset (mapent, 0, sizeof(*mapent));

	do
	{
		if (!GetToken (qtrue))
			Error ("ParseEntity: EOF without closing brace");
		if (!strcmp (token, "}") )
			break;

		if (!strcmp (token, "{") ) {
			// parse a brush or patch
			if (!GetToken (qtrue))
				break;
			if ( !strcmp( token, "patchDef2" ) ) {
				numMapPatches++;
				ParsePatch();
			} else if ( !strcmp( token, "terrainDef" ) ) {
				ParseTerrain();
			} else if ( !strcmp( token, "brushDef" ) ) {
				if (g_bBrushPrimit==BPRIMIT_OLDBRUSHES)
					Error("old brush format not allowed in new brush format map");
				g_bBrushPrimit=BPRIMIT_NEWBRUSHES;
				// parse brush primitive
				ParseBrush();
			}
			else
			{
				if (g_bBrushPrimit==BPRIMIT_NEWBRUSHES)
					Error("new brush format not allowed in old brush format map");
				g_bBrushPrimit=BPRIMIT_OLDBRUSHES;
				// parse old brush format
				UnGetToken();
				ParseBrush();
			}
			entitySourceBrushes++;
		}
		else
		{
			// parse a key / value pair
			e = ParseEpair ();
			e->next = mapent->epairs;
			mapent->epairs = e;
		}
	} while (1);

	GetVectorForKey (mapent, "origin", mapent->origin);

	//
	// if there was an origin brush, offset all of the planes and texinfo
	// for all the brushes in the entity
	if (mapent->origin[0] || mapent->origin[1] || mapent->origin[2]) {
		AdjustBrushesForOrigin( mapent );
	}

  // group_info entities are just for editor grouping
  // ignored
  // FIXME: leak!
  if (!strcmp("group_info", ValueForKey (mapent, "classname")))
  {
    num_entities--;
    return qtrue;
  }

	// group entities are just for editor convenience
	// toss all brushes into the world entity
	if (!strcmp ("func_group", ValueForKey (mapent, "classname")))
	{
		if ( !strcmp ("1", ValueForKey (mapent, "terrain"))) {
			SetTerrainTextures();
		}
		MoveBrushesToWorld (mapent);
		num_entities--;
		return qtrue;
	}

	return qtrue;
}

//===================================================================


/*
================
LoadMapFile
================
*/
void LoadMapFile (char *filename) {		
	bspbrush_t	*b;

	qprintf ("--- LoadMapFile ---\n");
	_printf ("Loading map file %s\n", filename);

	LoadScriptFile (filename);

	num_entities = 0;
	numMapDrawSurfs = 0;
	c_detail = 0;

	g_bBrushPrimit = BPRIMIT_UNDEFINED;

	// allocate a very large temporary brush for building
	// the brushes as they are loaded
	buildBrush = AllocBrush( MAX_BUILD_SIDES );

	while (ParseMapEntity ())
	{
	}

	ClearBounds (map_mins, map_maxs);
	for ( b = entities[0].brushes ; b ; b=b->next ) {
		AddPointToBounds( b->mins, map_mins, map_maxs );
		AddPointToBounds( b->maxs, map_mins, map_maxs );
	}

	qprintf ("%5i total world brushes\n", CountBrushList( entities[0].brushes ) );
	qprintf ("%5i detail brushes\n", c_detail );
	qprintf ("%5i patches\n", numMapPatches);
	qprintf ("%5i boxbevels\n", c_boxbevels);
	qprintf ("%5i edgebevels\n", c_edgebevels);
	qprintf ("%5i entities\n", num_entities);
	qprintf ("%5i planes\n", nummapplanes);
	qprintf ("%5i areaportals\n", c_areaportals);
	qprintf ("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", map_mins[0],map_mins[1],map_mins[2],
		map_maxs[0],map_maxs[1],map_maxs[2]);

	if ( fakemap ) {
		WriteBspBrushMap ("fakemap.map", entities[0].brushes );
	}

	if ( testExpand ) {
		TestExpandBrushes ();
	}
}


//====================================================================


/*
================
TestExpandBrushes

Expands all the brush planes and saves a new map out to
allow visual inspection of the clipping bevels
================
*/
void TestExpandBrushes( void ) {
	side_t	*s;
	int		i, j;
	bspbrush_t	*brush, *list, *copy;
	vec_t	dist;
	plane_t		*plane;

	list = NULL;

	for ( brush = entities[0].brushes ; brush ; brush = brush->next ) {
		copy = CopyBrush( brush );
		copy->next = list;
		list = copy;

		// expand all the planes
		for ( i=0 ; i<brush->numsides ; i++ ) {
			s = brush->sides + i;
			plane = &mapplanes[s->planenum];
			dist = plane->dist;
			for (j=0 ; j<3 ; j++) {
				dist += fabs( 16 * plane->normal[j] );
			}
			s->planenum = FindFloatPlane( plane->normal, dist );
		}

	}

	WriteBspBrushMap ( "expanded.map", entities[0].brushes );

	Error ("can't proceed after expanding brushes");
}
