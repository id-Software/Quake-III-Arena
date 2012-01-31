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

#include "qbsp.h"
#include "l_bsp_hl.h"
#include "l_bsp_q1.h"
#include "l_bsp_q2.h"
#include "l_bsp_q3.h"
#include "l_bsp_sin.h"
#include "l_mem.h"
#include "../botlib/aasfile.h"		//aas_bbox_t
#include "aas_store.h"		//AAS_MAX_BBOXES
#include "aas_cfg.h"

#define Sign(x)		(x < 0 ? 1 : 0)

int					nummapbrushes;
mapbrush_t			mapbrushes[MAX_MAPFILE_BRUSHES];

int					nummapbrushsides;
side_t				brushsides[MAX_MAPFILE_BRUSHSIDES];
brush_texture_t		side_brushtextures[MAX_MAPFILE_BRUSHSIDES];

int					nummapplanes;
plane_t				mapplanes[MAX_MAPFILE_PLANES];
int					mapplaneusers[MAX_MAPFILE_PLANES];

#define				PLANE_HASHES	1024
plane_t				*planehash[PLANE_HASHES];
vec3_t				map_mins, map_maxs;

#ifdef SIN
textureref_t		side_newrefs[MAX_MAPFILE_BRUSHSIDES];
#endif

map_texinfo_t		map_texinfo[MAX_MAPFILE_TEXINFO];
int					map_numtexinfo;
int					loadedmaptype;		//loaded map type

// undefine to make plane finding use linear sort
#define	USE_HASHING

int c_boxbevels;
int c_edgebevels;
int c_areaportals;
int c_clipbrushes;
int c_squattbrushes;
int c_writtenbrushes;

/*
=============================================================================

PLANE FINDING

=============================================================================
*/


//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int PlaneSignBits(vec3_t normal)
{
	int i, signbits;

	signbits = 0;
	for (i = 2; i >= 0; i--)
	{
		signbits = (signbits << 1) + Sign(normal[i]);
	} //end for
	return signbits;
} //end of the function PlaneSignBits
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int PlaneTypeForNormal(vec3_t normal)
{
	vec_t	ax, ay, az;
	
// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] == 1.0 || normal[0] == -1.0)
		return PLANE_X;
	if (normal[1] == 1.0 || normal[1] == -1.0)
		return PLANE_Y;
	if (normal[2] == 1.0 || normal[2] == -1.0)
		return PLANE_Z;
		
	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);
	
	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
} //end of the function PlaneTypeForNormal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
//ME NOTE: changed from 0.00001
#define	NORMAL_EPSILON	0.0001
//ME NOTE: changed from 0.01
#define	DIST_EPSILON	0.02
qboolean	PlaneEqual(plane_t *p, vec3_t normal, vec_t dist)
{
#if 1
	if (
	   fabs(p->normal[0] - normal[0]) < NORMAL_EPSILON
	&& fabs(p->normal[1] - normal[1]) < NORMAL_EPSILON
	&& fabs(p->normal[2] - normal[2]) < NORMAL_EPSILON
	&& fabs(p->dist - dist) < DIST_EPSILON )
		return true;
#else
	if (p->normal[0] == normal[0]
		&& p->normal[1] == normal[1]
		&& p->normal[2] == normal[2]
		&& p->dist == dist)
		return true;
#endif
	return false;
} //end of the function PlaneEqual
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddPlaneToHash(plane_t *p)
{
	int		hash;

	hash = (int)fabs(p->dist) / 8;
	hash &= (PLANE_HASHES-1);

	p->hash_chain = planehash[hash];
	planehash[hash] = p;
} //end of the function AddPlaneToHash
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int CreateNewFloatPlane (vec3_t normal, vec_t dist)
{
	plane_t	*p, temp;

	if (VectorLength(normal) < 0.5)
		Error ("FloatPlane: bad normal");
	// create a new plane
	if (nummapplanes+2 > MAX_MAPFILE_PLANES)
		Error ("MAX_MAPFILE_PLANES");

	p = &mapplanes[nummapplanes];
	VectorCopy (normal, p->normal);
	p->dist = dist;
	p->type = (p+1)->type = PlaneTypeForNormal (p->normal);
	p->signbits = PlaneSignBits(p->normal);

	VectorSubtract (vec3_origin, normal, (p+1)->normal);
	(p+1)->dist = -dist;
	(p+1)->signbits = PlaneSignBits((p+1)->normal);

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
} //end of the function CreateNewFloatPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void SnapVector(vec3_t normal)
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
} //end of the function SnapVector
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void SnapPlane(vec3_t normal, vec_t *dist)
{
	SnapVector(normal);

	if (fabs(*dist-Q_rint(*dist)) < DIST_EPSILON)
		*dist = Q_rint(*dist);
} //end of the function SnapPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifndef USE_HASHING
int FindFloatPlane(vec3_t normal, vec_t dist)
{
	int i;
	plane_t *p;

	SnapPlane(normal, &dist);
	for (i = 0, p = mapplanes; i < nummapplanes; i++, p++)
	{
		if (PlaneEqual (p, normal, dist))
		{
			mapplaneusers[i]++;
			return i;
		} //end if
	} //end for
	i = CreateNewFloatPlane (normal, dist);
	mapplaneusers[i]++;
	return i;
} //end of the function FindFloatPlane
#else
int FindFloatPlane (vec3_t normal, vec_t dist)
{
	int i;
	plane_t *p;
	int hash, h;

	SnapPlane (normal, &dist);
	hash = (int)fabs(dist) / 8;
	hash &= (PLANE_HASHES-1);

	// search the border bins as well
	for (i = -1; i <= 1; i++)
	{
		h = (hash+i)&(PLANE_HASHES-1);
		for (p = planehash[h]; p; p = p->hash_chain)
		{
			if (PlaneEqual(p, normal, dist))
			{
				mapplaneusers[p-mapplanes]++;
				return p - mapplanes;
			} //end if
		} //end for
	} //end for
	i = CreateNewFloatPlane (normal, dist);
	mapplaneusers[i]++;
	return i;
} //end of the function FindFloatPlane
#endif
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int PlaneFromPoints (int *p0, int *p1, int *p2)
{
	vec3_t	t1, t2, normal;
	vec_t	dist;

	VectorSubtract (p0, p1, t1);
	VectorSubtract (p2, p1, t2);
	CrossProduct (t1, t2, normal);
	VectorNormalize (normal);

	dist = DotProduct (p0, normal);

	return FindFloatPlane (normal, dist);
} //end of the function PlaneFromPoints
//===========================================================================
// Adds any additional planes necessary to allow the brush to be expanded
// against axial bounding boxes
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddBrushBevels (mapbrush_t *b)
{
	int		axis, dir;
	int		i, j, k, l, order;
	side_t	sidetemp;
	brush_texture_t	tdtemp;
#ifdef SIN
   textureref_t trtemp;
#endif
	side_t	*s, *s2;
	vec3_t	normal;
	float	dist;
	winding_t	*w, *w2;
	vec3_t	vec, vec2;
	float	d;

	//
	// add the axial planes
	//
	order = 0;
	for (axis=0 ; axis <3 ; axis++)
	{
		for (dir=-1 ; dir <= 1 ; dir+=2, order++)
		{
			// see if the plane is allready present
			for (i=0, s=b->original_sides ; i<b->numsides ; i++,s++)
			{
				if (mapplanes[s->planenum].normal[axis] == dir)
					break;
			}

			if (i == b->numsides)
			{	// add a new side
				if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
					Error ("MAX_MAP_BRUSHSIDES");
				nummapbrushsides++;
				b->numsides++;
				VectorClear (normal);
				normal[axis] = dir;
				if (dir == 1)
					dist = b->maxs[axis];
				else
					dist = -b->mins[axis];
				s->planenum = FindFloatPlane (normal, dist);
				s->texinfo = b->original_sides[0].texinfo;
#ifdef SIN
				s->lightinfo = b->original_sides[0].lightinfo;
#endif
				s->contents = b->original_sides[0].contents;
				s->flags |= SFL_BEVEL;
				c_boxbevels++;
			}

			// if the plane is not in it canonical order, swap it
			if (i != order)
			{
				sidetemp = b->original_sides[order];
				b->original_sides[order] = b->original_sides[i];
				b->original_sides[i] = sidetemp;

				j = b->original_sides - brushsides;
				tdtemp = side_brushtextures[j+order];
				side_brushtextures[j+order] = side_brushtextures[j+i];
				side_brushtextures[j+i] = tdtemp;

#ifdef SIN
				trtemp = side_newrefs[j+order];
				side_newrefs[j+order] = side_newrefs[j+i];
				side_newrefs[j+i] = trtemp;
#endif
			}
		}
	}

	//
	// add the edge bevels
	//
	if (b->numsides == 6)
		return;		// pure axial

	// test the non-axial plane edges
	for (i=6 ; i<b->numsides ; i++)
	{
		s = b->original_sides + i;
		w = s->winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
		{
			k = (j+1)%w->numpoints;
			VectorSubtract (w->p[j], w->p[k], vec);
			if (VectorNormalize (vec) < 0.5)
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
					if (VectorNormalize (normal) < 0.5)
						continue;
					dist = DotProduct (w->p[j], normal);

					// if all the points on all the sides are
					// behind this plane, it is a proper edge bevel
					for (k=0 ; k<b->numsides ; k++)
					{
						// if this plane has allready been used, skip it
						if (PlaneEqual (&mapplanes[b->original_sides[k].planenum]
							, normal, dist) )
							break;

						w2 = b->original_sides[k].winding;
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

					if (k != b->numsides)
						continue;	// wasn't part of the outer hull
					// add this plane
					if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
						Error ("MAX_MAP_BRUSHSIDES");
					nummapbrushsides++;
					s2 = &b->original_sides[b->numsides];
					s2->planenum = FindFloatPlane (normal, dist);
					s2->texinfo = b->original_sides[0].texinfo;
#ifdef SIN
					s2->lightinfo = b->original_sides[0].lightinfo;
#endif
					s2->contents = b->original_sides[0].contents;
					s2->flags |= SFL_BEVEL;
					c_edgebevels++;
					b->numsides++;
				}
			}
		}
	}
} //end of the function AddBrushBevels
//===========================================================================
// creates windigs for sides and mins / maxs for the brush
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean MakeBrushWindings(mapbrush_t *ob)
{
	int			i, j;
	winding_t	*w;
	side_t		*side;
	plane_t		*plane;

	ClearBounds (ob->mins, ob->maxs);

	for (i = 0; i < ob->numsides; i++)
	{
		plane = &mapplanes[ob->original_sides[i].planenum];
		w = BaseWindingForPlane(plane->normal, plane->dist);
		for (j = 0; j <ob->numsides && w; j++)
		{
			if (i == j) continue;
			if (ob->original_sides[j].flags & SFL_BEVEL) continue;
			plane = &mapplanes[ob->original_sides[j].planenum^1];
			ChopWindingInPlace(&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		}

		side = &ob->original_sides[i];
		side->winding = w;
		if (w)
		{
			side->flags |= SFL_VISIBLE;
			for (j = 0; j < w->numpoints; j++)
				AddPointToBounds (w->p[j], ob->mins, ob->maxs);
		}
	}

	for (i = 0; i < 3; i++)
	{
		//IDBUG: all the indexes into the mins and maxs were zero (not using i)
		if (ob->mins[i] < -MAX_MAP_BOUNDS || ob->maxs[i] > MAX_MAP_BOUNDS)
		{
			Log_Print("entity %i, brush %i: bounds out of range\n", ob->entitynum, ob->brushnum);
			ob->numsides = 0; //remove the brush
			break;
		} //end if
		if (ob->mins[i] > MAX_MAP_BOUNDS || ob->maxs[i] < -MAX_MAP_BOUNDS)
		{
			Log_Print("entity %i, brush %i: no visible sides on brush\n", ob->entitynum, ob->brushnum);
			ob->numsides = 0; //remove the brush
			break;
		} //end if
	} //end for
	return true;
} //end of the function MakeBrushWindings
//===========================================================================
// FIXME: currently doesn't mark all bevels
// NOTE: when one brush bevel is found the remaining sides of the brush
//       are bevels as well (when the brush isn't expanded for AAS :))
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void MarkBrushBevels(mapbrush_t *brush)
{
	int i;
	int we;
	side_t *s;

	//check all the sides of the brush
	for (i = 0; i < brush->numsides; i++)
	{
		s = brush->original_sides + i;
		//if the side has no winding
		if (!s->winding)
		{
			Log_Write("MarkBrushBevels: brush %d no winding", brush->brushnum);
			s->flags |= SFL_BEVEL;
		} //end if
		//if the winding is tiny
		else if (WindingIsTiny(s->winding))
		{
			s->flags |= SFL_BEVEL;
			Log_Write("MarkBrushBevels: brush %d tiny winding", brush->brushnum);
		} //end else if
		//if the winding has errors
		else
		{
			we = WindingError(s->winding);
			if (we == WE_NOTENOUGHPOINTS
					|| we == WE_SMALLAREA
					|| we == WE_POINTBOGUSRANGE
//					|| we == WE_NONCONVEX
					)
			{
				Log_Write("MarkBrushBevels: brush %d %s", brush->brushnum, WindingErrorString());
				s->flags |= SFL_BEVEL;
			} //end else if
		} //end else
		if (s->flags & SFL_BEVEL)
		{
			s->flags &= ~SFL_VISIBLE;
			//if the side has a valid plane
			if (s->planenum > 0 && s->planenum < nummapplanes)
			{
				//if it is an axial plane
				if (mapplanes[s->planenum].type < 3) c_boxbevels++;
				else c_edgebevels++;
			} //end if
		} //end if
	} //end for
} //end of the function MarkBrushBevels
//===========================================================================
// returns true if the map brush already exists
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int BrushExists(mapbrush_t *brush)
{
	int i, s1, s2;
	side_t *side1, *side2;
	mapbrush_t *brush1, *brush2;

	for (i = 0; i < nummapbrushes; i++)
	{
		brush1 = brush;
		brush2 = &mapbrushes[i];
		//compare the brushes
		if (brush1->entitynum != brush2->entitynum) continue;
		//if (brush1->contents != brush2->contents) continue;
		if (brush1->numsides != brush2->numsides) continue;
		for (s1 = 0; s1 < brush1->numsides; s1++)
		{
			side1 = brush1->original_sides + s1;
			//
			for (s2 = 0; s2 < brush2->numsides; s2++)
			{
				side2 = brush2->original_sides + s2;
				//
				if ((side1->planenum & ~1) == (side2->planenum & ~1)
//						&& side1->texinfo == side2->texinfo
//						&& side1->contents == side2->contents
//						&& side1->surf == side2->surf
					) break;
			} //end if
			if (s2 >= brush2->numsides) break;
		} //end for
		if (s1 >= brush1->numsides) return true;
	} //end for
	return false;
} //end of the function BrushExists
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WriteMapBrush(FILE *fp, mapbrush_t *brush, vec3_t origin)
{
	int sn, rotate, shift[2], sv, tv, planenum, p1, i, j;
	float scale[2], originshift[2], ang1, ang2, newdist;
	vec3_t vecs[2], axis[2];
	map_texinfo_t *ti;
	winding_t *w;
	side_t *s;
	plane_t *plane;

	if (noliquids)
	{
		if (brush->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA))
		{
			return true;
		} //end if
	} //end if
	//if the brush has no contents
	if (!brush->contents) return true;
	//print the leading {
	if (fprintf(fp, " { //brush %d\n", brush->brushnum) < 0) return false;
	//write brush sides
	for (sn = 0; sn < brush->numsides; sn++)
	{
		s = brush->original_sides + sn;
		//don't write out bevels
		if (!(s->flags & SFL_BEVEL))
		{
			//if the entity has an origin set
			if (origin[0] || origin[1] || origin[2])
			{
				newdist = mapplanes[s->planenum].dist +
					DotProduct(mapplanes[s->planenum].normal, origin);
				planenum = FindFloatPlane(mapplanes[s->planenum].normal, newdist);
			} //end if
			else
			{
				planenum = s->planenum;
			} //end else
			//always take the first plane, then flip the points if necesary
			plane = &mapplanes[planenum & ~1];
			w = BaseWindingForPlane(plane->normal, plane->dist);
			//
			for (i = 0; i < 3; i++)
			{
				for (j = 0; j < 3; j++)
				{
					if (fabs(w->p[i][j]) < 0.2) w->p[i][j] = 0;
					else if (fabs((int)w->p[i][j] - w->p[i][j]) < 0.3) w->p[i][j] = (int) w->p[i][j];
					//w->p[i][j] = (int) (w->p[i][j] + 0.2);
				} //end for
			} //end for
			//three non-colinear points to define the plane
			if (planenum & 1) p1 = 1;
			else p1 = 0;
			if (fprintf(fp,"  ( %5i %5i %5i ) ", (int)w->p[p1][0], (int)w->p[p1][1], (int)w->p[p1][2]) < 0) return false;
			if (fprintf(fp,"( %5i %5i %5i ) ", (int)w->p[!p1][0], (int)w->p[!p1][1], (int)w->p[!p1][2]) < 0) return false;
			if (fprintf(fp,"( %5i %5i %5i ) ", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]) < 0) return false;
			//free the winding
			FreeWinding(w);
			//
			if (s->texinfo == TEXINFO_NODE)
			{
				if (brush->contents & CONTENTS_PLAYERCLIP)
				{
					//player clip
					if (loadedmaptype == MAPTYPE_SIN)
					{
						if (fprintf(fp, "generic/misc/clip 0 0 0 1 1") < 0) return false;
					} //end if
					else if (loadedmaptype == MAPTYPE_QUAKE2)
					{	//FIXME: don't always use e1u1
						if (fprintf(fp, "e1u1/clip 0 0 0 1 1") < 0) return false;
					} //end else
					else if (loadedmaptype == MAPTYPE_QUAKE3)
					{
						if (fprintf(fp, "e1u1/clip 0 0 0 1 1") < 0) return false;
					} //end else if
					else
					{
						if (fprintf(fp, "clip 0 0 0 1 1") < 0) return false;
					} //end else
				} //end if
				else if (brush->contents == CONTENTS_MONSTERCLIP)
				{
					//monster clip
					if (loadedmaptype == MAPTYPE_SIN)
					{
						if (fprintf(fp, "generic/misc/monster 0 0 0 1 1") < 0) return false;
					} //end if
					else if (loadedmaptype == MAPTYPE_QUAKE2)
					{
						if (fprintf(fp, "e1u1/clip_mon 0 0 0 1 1") < 0) return false;
					} //end else
					else
					{
						if (fprintf(fp, "clip 0 0 0 1 1") < 0) return false;
					} //end else
				} //end else
				else
				{
					if (fprintf(fp, "clip 0 0 0 1 1") < 0) return false;
					Log_Write("brush->contents = %d\n", brush->contents);
				} //end else
			} //end if
			else if (loadedmaptype == MAPTYPE_SIN && s->texinfo == 0)
			{
				if (brush->contents & CONTENTS_DUMMYFENCE)
				{
					if (fprintf(fp, "generic/misc/fence 0 0 0 1 1") < 0) return false;
				} //end if
				else if (brush->contents & CONTENTS_MIST)
				{
					if (fprintf(fp, "generic/misc/volumetric_base 0 0 0 1 1") < 0) return false;
				} //end if
				else //unknown so far
				{
					if (fprintf(fp, "generic/misc/red 0 0 0 1 1") < 0) return false;
				} //end else
			} //end if
			else if (loadedmaptype == MAPTYPE_QUAKE3)
			{
				//always use the same texture
				if (fprintf(fp, "e2u3/floor1_2 0 0 0 1 1 1 0 0") < 0) return false;
			} //end else if
			else
			{
				//*
				ti = &map_texinfo[s->texinfo];
				//the scaling of the texture
				scale[0] = 1 / VectorNormalize2(ti->vecs[0], vecs[0]);
				scale[1] = 1 / VectorNormalize2(ti->vecs[1], vecs[1]);
				//
				TextureAxisFromPlane(plane, axis[0], axis[1]);
				//calculate texture shift done by entity origin
				originshift[0] = DotProduct(origin, axis[0]);
				originshift[1] = DotProduct(origin, axis[1]);
				//the texture shift without origin shift
				shift[0] = ti->vecs[0][3] - originshift[0];
				shift[1] = ti->vecs[1][3] - originshift[1];
				//
				if (axis[0][0]) sv = 0;
				else if (axis[0][1]) sv = 1;
				else sv = 2;
				if (axis[1][0]) tv = 0;
				else if (axis[1][1]) tv = 1;
				else tv = 2;
				//calculate rotation of texture
				if (vecs[0][tv] == 0) ang1 = vecs[0][sv] > 0 ? 90.0 : -90.0;
				else ang1 = atan2(vecs[0][sv], vecs[0][tv]) * 180 / Q_PI;
				if (ang1 < 0) ang1 += 360;
				if (ang1 >= 360) ang1 -= 360;
				if (axis[0][tv] == 0) ang2 = axis[0][sv] > 0 ? 90.0 : -90.0;
				else ang2 = atan2(axis[0][sv], axis[0][tv]) * 180 / Q_PI;
				if (ang2 < 0) ang2 += 360;
				if (ang2 >= 360) ang2 -= 360;
				rotate = ang2 - ang1;
				if (rotate < 0) rotate += 360;
				if (rotate >= 360) rotate -= 360;
				//write the texture info
				if (fprintf(fp, "%s %d %d %d", ti->texture, shift[0], shift[1], rotate) < 0) return false;
				if (fabs(scale[0] - ((int) scale[0])) < 0.001)
				{
					if (fprintf(fp, " %d", (int) scale[0]) < 0) return false;
				} //end if
				else
				{
					if (fprintf(fp, " %4f", scale[0]) < 0) return false;
				} //end if
				if (fabs(scale[1] - ((int) scale[1])) < 0.001)
				{
					if (fprintf(fp, " %d", (int) scale[1]) < 0) return false;
				} //end if
				else
				{
					if (fprintf(fp, " %4f", scale[1]) < 0) return false;
				} //end else
				//write the extra brush side info
				if (loadedmaptype == MAPTYPE_QUAKE2)
				{
					if (fprintf(fp, " %ld %ld %ld", s->contents, ti->flags, ti->value) < 0) return false;
				} //end if
				//*/
			} //end else
			if (fprintf(fp, "\n") < 0) return false;
		} //end if
	} //end if
	if (fprintf(fp, " }\n") < 0) return false;
	c_writtenbrushes++;
	return true;
} //end of the function WriteMapBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WriteOriginBrush(FILE *fp, vec3_t origin)
{
	vec3_t normal;
	float dist;
	int i, s;
	winding_t *w;

	if (fprintf(fp, " {\n") < 0) return false;
	//
	for (i = 0; i < 3; i++)
	{
		for (s = -1; s <= 1; s += 2)
		{
			//
			VectorClear(normal);
			normal[i] = s;
			dist = origin[i] * s + 16;
			//
			w = BaseWindingForPlane(normal, dist);
			//three non-colinear points to define the plane
			if (fprintf(fp,"  ( %5i %5i %5i ) ", (int)w->p[0][0], (int)w->p[0][1], (int)w->p[0][2]) < 0) return false;
			if (fprintf(fp,"( %5i %5i %5i ) ", (int)w->p[1][0], (int)w->p[1][1], (int)w->p[1][2]) < 0) return false;
			if (fprintf(fp,"( %5i %5i %5i ) ", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]) < 0) return false;
			//free the winding
			FreeWinding(w);
			//write origin texture:
			// CONTENTS_ORIGIN = 16777216
			// SURF_NODRAW = 128
			if (loadedmaptype == MAPTYPE_SIN)
			{
				if (fprintf(fp, "generic/misc/origin 0 0 0 1 1") < 0) return false;
			} //end if
			else if (loadedmaptype == MAPTYPE_HALFLIFE)
			{
				if (fprintf(fp, "origin 0 0 0 1 1") < 0) return false;
			} //end if
			else
			{
				if (fprintf(fp, "e1u1/origin 0 0 0 1 1") < 0) return false;
			} //end else
			//Quake2 extra brush side info
			if (loadedmaptype == MAPTYPE_QUAKE2)
			{
				//if (fprintf(fp, " 16777216 128 0") < 0) return false;
			} //end if
			if (fprintf(fp, "\n") < 0) return false;
		} //end for
	} //end for
	if (fprintf(fp, " }\n") < 0) return false;
	c_writtenbrushes++;
	return true;
} //end of the function WriteOriginBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
mapbrush_t *GetAreaPortalBrush(entity_t *mapent)
{
	int portalnum, bn;
	mapbrush_t *brush;

	//the area portal number
	portalnum = mapent->areaportalnum;
	//find the area portal brush in the world brushes
	for (bn = 0; bn < nummapbrushes && portalnum; bn++)
	{
		brush = &mapbrushes[bn];
		//must be in world entity
		if (brush->entitynum == 0)
		{
			if (brush->contents & CONTENTS_AREAPORTAL)
			{
				portalnum--;
			} //end if
		} //end if
	} //end for
	if (bn < nummapbrushes)
	{
		return brush;
	} //end if
	else
	{
		Log_Print("area portal %d brush not found\n", mapent->areaportalnum);
		return NULL;
	} //end else
} //end of the function GetAreaPortalBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WriteMapFileSafe(FILE *fp)
{
	char key[1024], value[1024];
	int i, bn, entitybrushes;
	epair_t *ep;
	mapbrush_t *brush;
	entity_t *mapent;
	//vec3_t vec_origin = {0, 0, 0};

	//
	if (fprintf(fp,"//=====================================================\n"
			"//\n"
			"// map file created with BSPC "BSPC_VERSION"\n"
			"//\n"
			"// BSPC is designed to decompile material in which you own the copyright\n"
			"// or have obtained permission to decompile from the copyright owner. Unless\n"
			"// you own the copyright or have permission to decompile from the copyright\n"
			"// owner, you may be violating copyright law and be subject to payment of\n"
			"// damages and other remedies. If you are uncertain about your rights, contact\n"
			"// your legal advisor.\n"
			"//\n") < 0) return false;
	if (loadedmaptype == MAPTYPE_SIN)
	{
		if (fprintf(fp,
						"// generic/misc/red is used for unknown textures\n") < 0) return false;
	} //end if
	if (fprintf(fp,"//\n"
						"//=====================================================\n") < 0) return false;
	//write out all the entities
	for (i = 0; i < num_entities; i++)
	{
		mapent = &entities[i];
		if (!mapent->epairs)
		{
			continue;
		} //end if
		if (fprintf(fp, "{\n") < 0) return false;
		//
		if (loadedmaptype == MAPTYPE_QUAKE3)
		{
			if (!stricmp(ValueForKey(mapent, "classname"), "light"))
			{
				SetKeyValue(mapent, "light", "10000");
			} //end if
		} //end if
		//write epairs
		for (ep = mapent->epairs; ep; ep = ep->next)
		{
			strcpy(key, ep->key);
			StripTrailing (key);
			strcpy(value, ep->value);
			StripTrailing(value);
			//
			if (loadedmaptype == MAPTYPE_QUAKE2 ||
					loadedmaptype == MAPTYPE_SIN)
			{
				//don't write an origin for BSP models
				if (mapent->modelnum >= 0 && !strcmp(key, "origin")) continue;
			} //end if
			//don't write BSP model numbers
			if (mapent->modelnum >= 0 && !strcmp(key, "model") && value[0] == '*') continue;
			//
			if (fprintf(fp, " \"%s\" \"%s\"\n", key, value) < 0) return false;
		} //end for
		//
		if (ValueForKey(mapent, "origin")) GetVectorForKey(mapent, "origin", mapent->origin);
		else mapent->origin[0] = mapent->origin[1] = mapent->origin[2] = 0;
		//if this is an area portal entity
		if (!strcmp("func_areaportal", ValueForKey(mapent, "classname")))
		{
			brush = GetAreaPortalBrush(mapent);
			if (!brush) return false;
			if (!WriteMapBrush(fp, brush, mapent->origin)) return false;
		} //end if
		else
		{
			entitybrushes = false;
			//write brushes
			for (bn = 0; bn < nummapbrushes; bn++)
			{
				brush = &mapbrushes[bn];
				//if the brush is part of this entity
				if (brush->entitynum == i)
				{
					//don't write out area portal brushes in the world
					if (!((brush->contents & CONTENTS_AREAPORTAL) && brush->entitynum == 0))
					{
						/*
						if (!strcmp("func_door_rotating", ValueForKey(mapent, "classname")))
						{
							AAS_PositionFuncRotatingBrush(mapent, brush);
							if (!WriteMapBrush(fp, brush, vec_origin)) return false;
						} //end if
						else //*/
						{
							if (!WriteMapBrush(fp, brush, mapent->origin)) return false;
						} //end else
						entitybrushes = true;
					} //end if
				} //end if
			} //end for
			//if the entity had brushes
			if (entitybrushes)
			{
				//if the entity has an origin set
				if (mapent->origin[0] || mapent->origin[1] || mapent->origin[2])
				{
					if (!WriteOriginBrush(fp, mapent->origin)) return false;
				} //end if
			} //end if
		} //end else
		if (fprintf(fp, "}\n") < 0) return false;
	} //end for
	if (fprintf(fp, "//total of %d brushes\n", c_writtenbrushes) < 0) return false;
	return true;
} //end of the function WriteMapFileSafe
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void WriteMapFile(char *filename)
{
	FILE *fp;
	double start_time;

	c_writtenbrushes = 0;
	//the time started
	start_time = I_FloatTime();
	//
	Log_Print("writing %s\n", filename);
	fp = fopen(filename, "wb");
	if (!fp)
	{
		Log_Print("can't open %s\n", filename);
		return;
	} //end if
	if (!WriteMapFileSafe(fp))
	{
		fclose(fp);
		Log_Print("error writing map file %s\n", filename);
		return;
	} //end if
	fclose(fp);
	//display creation time
	Log_Print("written %d brushes\n", c_writtenbrushes);
	Log_Print("map file written in %5.0f seconds\n", I_FloatTime() - start_time);
} //end of the function WriteMapFile
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void PrintMapInfo(void)
{
	Log_Print("\n");
	Log_Print("%6i brushes\n", nummapbrushes);
	Log_Print("%6i brush sides\n", nummapbrushsides);
//	Log_Print("%6i clipbrushes\n", c_clipbrushes);
//	Log_Print("%6i total sides\n", nummapbrushsides);
//	Log_Print("%6i boxbevels\n", c_boxbevels);
//	Log_Print("%6i edgebevels\n", c_edgebevels);
//	Log_Print("%6i entities\n", num_entities);
//	Log_Print("%6i planes\n", nummapplanes);
//	Log_Print("%6i areaportals\n", c_areaportals);
//	Log_Print("%6i squatt brushes\n", c_squattbrushes);
//	Log_Print("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", map_mins[0],map_mins[1],map_mins[2],
//		map_maxs[0],map_maxs[1],map_maxs[2]);
} //end of the function PrintMapInfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ResetMapLoading(void)
{
	int i;
	epair_t *ep, *nextep;

	Q2_ResetMapLoading();
	Sin_ResetMapLoading();

	//free all map brush side windings
	for (i = 0; i < nummapbrushsides; i++)
	{
		if (brushsides[i].winding)
		{
			FreeWinding(brushsides[i].winding);
		} //end for
	} //end for

	//reset regular stuff
	nummapbrushes = 0;
	memset(mapbrushes, 0, MAX_MAPFILE_BRUSHES * sizeof(mapbrush_t));
	//
	nummapbrushsides = 0;
	memset(brushsides, 0, MAX_MAPFILE_BRUSHSIDES * sizeof(side_t));
	memset(side_brushtextures, 0, MAX_MAPFILE_BRUSHSIDES * sizeof(brush_texture_t));
	//
	nummapplanes = 0;
	memset(mapplanes, 0, MAX_MAPFILE_PLANES * sizeof(plane_t));
	//
	memset(planehash, 0, PLANE_HASHES * sizeof(plane_t *));
	//
	memset(map_texinfo, 0, MAX_MAPFILE_TEXINFO * sizeof(map_texinfo_t));
	map_numtexinfo = 0;
	//
	VectorClear(map_mins);
	VectorClear(map_maxs);
	//
	c_boxbevels = 0;
	c_edgebevels = 0;
	c_areaportals = 0;
	c_clipbrushes = 0;
	c_writtenbrushes = 0;
	//clear the entities
	for (i = 0; i < num_entities; i++)
	{
		for (ep = entities[i].epairs; ep; ep = nextep)
		{
			nextep = ep->next;
			FreeMemory(ep->key);
			FreeMemory(ep->value);
			FreeMemory(ep);
		} //end for
	} //end for
	num_entities = 0;
	memset(entities, 0, MAX_MAP_ENTITIES * sizeof(entity_t));
} //end of the function ResetMapLoading
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifndef Q1_BSPVERSION
#define Q1_BSPVERSION	29
#endif
#ifndef HL_BSPVERSION
#define HL_BSPVERSION	30
#endif

#define Q2_BSPHEADER				(('P'<<24)+('S'<<16)+('B'<<8)+'I')	//IBSP
#define Q2_BSPVERSION	38

#define SINGAME_BSPHEADER		(('P'<<24)+('S'<<16)+('B'<<8)+'R')	//RBSP
#define SINGAME_BSPVERSION		1

#define SIN_BSPHEADER			(('P'<<24)+('S'<<16)+('B'<<8)+'I')	//IBSP
#define SIN_BSPVERSION	41

typedef struct
{
	int ident;
	int version;
} idheader_t;

int LoadMapFromBSP(struct quakefile_s *qf)
{
	idheader_t idheader;

	if (ReadQuakeFile(qf, &idheader, 0, sizeof(idheader_t)) != sizeof(idheader_t))
	{
		return false;
	} //end if

	idheader.ident = LittleLong(idheader.ident);
	idheader.version = LittleLong(idheader.version);
	//Quake3 BSP file
	if (idheader.ident == Q3_BSP_IDENT && idheader.version == Q3_BSP_VERSION)
	{
		ResetMapLoading();
		Q3_LoadMapFromBSP(qf);
		Q3_FreeMaxBSP();
	} //end if
	//Quake2 BSP file
	else if (idheader.ident == Q2_BSPHEADER && idheader.version == Q2_BSPVERSION)
	{
		ResetMapLoading();
		Q2_AllocMaxBSP();
		Q2_LoadMapFromBSP(qf->filename, qf->offset, qf->length);
		Q2_FreeMaxBSP();
	} //endif
	//Sin BSP file
	else if ((idheader.ident == SIN_BSPHEADER && idheader.version == SIN_BSPVERSION) ||
				//the dorks gave the same format another ident and verions
				(idheader.ident == SINGAME_BSPHEADER && idheader.version == SINGAME_BSPVERSION))
	{
		ResetMapLoading();
		Sin_AllocMaxBSP();
		Sin_LoadMapFromBSP(qf->filename, qf->offset, qf->length);
		Sin_FreeMaxBSP();
	} //end if
	//the Quake1 bsp files don't have a ident only a version
	else if (idheader.ident == Q1_BSPVERSION)
	{
		ResetMapLoading();
		Q1_AllocMaxBSP();
		Q1_LoadMapFromBSP(qf->filename, qf->offset, qf->length);
		Q1_FreeMaxBSP();
	} //end if
	//Half-Life also only uses a version number
	else if (idheader.ident == HL_BSPVERSION)
	{
		ResetMapLoading();
		HL_AllocMaxBSP();
		HL_LoadMapFromBSP(qf->filename, qf->offset, qf->length);
		HL_FreeMaxBSP();
	} //end if
	else
	{
		Error("unknown BSP format %c%c%c%c, version %d\n",
										(idheader.ident & 0xFF),
										((idheader.ident >> 8) & 0xFF),
										((idheader.ident >> 16) & 0xFF),
										((idheader.ident >> 24) & 0xFF), idheader.version);
		return false;
	} //end if
	//
	return true;
} //end of the function LoadMapFromBSP
