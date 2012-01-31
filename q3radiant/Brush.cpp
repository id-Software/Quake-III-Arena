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


#include "stdafx.h"
#include <assert.h>
#include "qe3.h"
#include "winding.h"


// globals

int g_nBrushId = 0;

const char* Brush_Name(brush_t *b)
{
  static char cBuff[1024];
	b->numberId = g_nBrushId++;
	if (g_qeglobals.m_bBrushPrimitMode)
  {
    sprintf(cBuff, "Brush %i", b->numberId);
    Brush_SetEpair(b, "Name", cBuff);
  }
  return cBuff;
}

brush_t *Brush_Alloc()
{
  brush_t *b = (brush_t*)qmalloc(sizeof(brush_t));
  return b;
}




void PrintWinding (winding_t *w)
{
	int		i;
	
	printf ("-------------\n");
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.2f, %5.2f, %5.2f)\n", w->points[i][0]
		, w->points[i][1], w->points[i][2]);
}

void PrintPlane (plane_t *p)
{
  printf ("(%5.2f, %5.2f, %5.2f) : %5.2f\n",  p->normal[0],  p->normal[1], 
  p->normal[2],  p->dist);
}

void PrintVector (vec3_t v)
{
  printf ("(%5.2f, %5.2f, %5.2f)\n",  v[0],  v[1], v[2]);
}


/*
=============================================================================

			TEXTURE COORDINATES

=============================================================================
*/


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
	float	dot,best;
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



float	lightaxis[3] = {0.6, 0.8, 1.0};
/*
================
SetShadeForPlane

Light different planes differently to
improve recognition
================
*/
float SetShadeForPlane (plane_t *p)
{
	int		i;
	float	f;

	// axial plane
	for (i=0 ; i<3 ; i++)
		if (fabs(p->normal[i]) > 0.9)
		{
			f = lightaxis[i];
			return f;
		}

	// between two axial planes
	for (i=0 ; i<3 ; i++)
		if (fabs(p->normal[i]) < 0.1)
		{
			f = (lightaxis[(i+1)%3] + lightaxis[(i+2)%3])/2;
			return f;
		}

	// other
	f= (lightaxis[0] + lightaxis[1] + lightaxis[2]) / 3;
	return f;
}

vec3_t  vecs[2];
float	shift[2];

/*
================
Face_Alloc
================
*/
face_t *Face_Alloc( void )
{
	face_t *f = (face_t*)qmalloc( sizeof( *f ) );
	
	if (g_qeglobals.bSurfacePropertiesPlugin)
		f->pData = static_cast<void *>( g_SurfaceTable.m_pfnTexdefAlloc( f ) );

	return f;
}

/*
================
Face_Free
================
*/
void Face_Free( face_t *f )
{
	assert( f != 0 );

	if ( f->face_winding )
	{
		free( f->face_winding );
		f->face_winding = 0;
	}

	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
#ifdef _DEBUG
		if ( !f->pData )
		{
			Sys_Printf("WARNING: unexpected IPluginTexdef is NULL in Face_Free\n");
		}
		else
#endif
		GETPLUGINTEXDEF(f)->DecRef();
	}

	f->texdef.~texdef_t();;

	free( f );
}

/*
================
Face_Clone
================
*/
face_t	*Face_Clone (face_t *f)
{
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;

	memcpy (n->planepts, f->planepts, sizeof(n->planepts));

	// all other fields are derived, and will be set by Brush_Build
	return n;
}

/*
================
Face_FullClone

makes an exact copy of the face
================
*/
face_t	*Face_FullClone (face_t *f)
{
	face_t	*n;

	n = Face_Alloc();
	n->texdef = f->texdef;
	memcpy(n->planepts, f->planepts, sizeof(n->planepts));
	memcpy(&n->plane, &f->plane, sizeof(plane_t));
	if (f->face_winding)
		n->face_winding = Winding_Clone(f->face_winding);
	else
		n->face_winding = NULL;
	n->d_texture = Texture_ForName( n->texdef.name );
	return n;
}

/*
================
Clamp
================
*/
void Clamp(float& f, int nClamp)
{
  float fFrac = f - static_cast<int>(f);
  f = static_cast<int>(f) % nClamp;
  f += fFrac;
}

/*
================
Face_MoveTexture
================
*/
void Face_MoveTexture(face_t *f, vec3_t delta)
{
	vec3_t vX, vY;
/*
#ifdef _DEBUG
	if (g_PrefsDlg.m_bBrushPrimitMode)
		Sys_Printf("Warning : Face_MoveTexture not done in brush primitive mode\n");
#endif
*/
	if (g_qeglobals.m_bBrushPrimitMode)
		Face_MoveTexture_BrushPrimit( f, delta );
	else
	{
		TextureAxisFromPlane(&f->plane, vX, vY);

		vec3_t vDP, vShift;
		vDP[0] = DotProduct(delta, vX);
		vDP[1] = DotProduct(delta, vY);

		double fAngle = f->texdef.rotate  / 180 * Q_PI;
		double c = cos(fAngle);
		double s = sin(fAngle);

		vShift[0] = vDP[0] * c - vDP[1] * s;
		vShift[1] = vDP[0] * s + vDP[1] * c;

		if (!f->texdef.scale[0])
			f->texdef.scale[0] = 1;
		if (!f->texdef.scale[1])
			f->texdef.scale[1] = 1;

		f->texdef.shift[0] -= vShift[0] / f->texdef.scale[0];
		f->texdef.shift[1] -= vShift[1] / f->texdef.scale[1];
  
		// clamp the shifts
		Clamp(f->texdef.shift[0], f->d_texture->width);
		Clamp(f->texdef.shift[1], f->d_texture->height);
	}
}

/*
================
Face_SetColor
================
*/
void Face_SetColor (brush_t *b, face_t *f, float fCurveColor) 
{
	float	shade;
	qtexture_t *q;

	q = f->d_texture;

	// set shading for face
	shade = SetShadeForPlane (&f->plane);
	if (g_pParentWnd->GetCamera()->Camera().draw_mode == cd_texture && !b->owner->eclass->fixedsize)
	{
		//if (b->curveBrush)
		//  shade = fCurveColor;
		f->d_color[0] = 
		f->d_color[1] = 
		f->d_color[2] = shade;
	}
	else
	{
		f->d_color[0] = shade*q->color[0];
		f->d_color[1] = shade*q->color[1];
		f->d_color[2] = shade*q->color[2];
	}
}

/*
================
Face_TextureVectors
TTimo: NOTE: this is never to get called while in brush primitives mode
================
*/
void Face_TextureVectors (face_t *f, float STfromXYZ[2][4])
{
	vec3_t		pvecs[2];
	int			sv, tv;
	float		ang, sinv, cosv;
	float		ns, nt;
	int			i,j;
	qtexture_t *q;
	texdef_t	*td;

#ifdef _DEBUG
	//++timo when playing with patches, this sometimes get called and the Warning is displayed
	// find some way out ..
	if (g_qeglobals.m_bBrushPrimitMode && !g_qeglobals.bNeedConvert)
		Sys_Printf("Warning : illegal call of Face_TextureVectors in brush primitive mode\n");
#endif

	td = &f->texdef;
	q = f->d_texture;

	memset (STfromXYZ, 0, 8*sizeof(float));

	if (!td->scale[0])
		td->scale[0] = (g_PrefsDlg.m_bHiColorTextures) ? 0.5 : 1;
	if (!td->scale[1])
		td->scale[1] = (g_PrefsDlg.m_bHiColorTextures) ? 0.5 : 1;

	// get natural texture axis
	TextureAxisFromPlane(&f->plane, pvecs[0], pvecs[1]);

	// rotate axis
	if (td->rotate == 0)
		{ sinv = 0 ; cosv = 1; }
	else if (td->rotate == 90)
		{ sinv = 1 ; cosv = 0; }
	else if (td->rotate == 180)
		{ sinv = 0 ; cosv = -1; }
	else if (td->rotate == 270)
		{ sinv = -1 ; cosv = 0; }
	else
	{	
		ang = td->rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (pvecs[0][0])
		sv = 0;
	else if (pvecs[0][1])
		sv = 1;
	else
		sv = 2;
				
	if (pvecs[1][0])
		tv = 0;
	else if (pvecs[1][1])
		tv = 1;
	else
		tv = 2;
					
	for (i=0 ; i<2 ; i++) {
		ns = cosv * pvecs[i][sv] - sinv * pvecs[i][tv];
		nt = sinv * pvecs[i][sv] +  cosv * pvecs[i][tv];
		STfromXYZ[i][sv] = ns;
		STfromXYZ[i][tv] = nt;
	}

	// scale
	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			STfromXYZ[i][j] = STfromXYZ[i][j] / td->scale[i];

	// shift
	STfromXYZ[0][3] = td->shift[0];
	STfromXYZ[1][3] = td->shift[1];

	for (j=0 ; j<4 ; j++) {
		STfromXYZ[0][j] /= q->width;
		STfromXYZ[1][j] /= q->height;
	}
}

/*
================
Face_MakePlane
================
*/
void Face_MakePlane (face_t *f)
{
	int		j;
	vec3_t	t1, t2, t3;

	// convert to a vector / dist plane
	for (j=0 ; j<3 ; j++)
	{
		t1[j] = f->planepts[0][j] - f->planepts[1][j];
		t2[j] = f->planepts[2][j] - f->planepts[1][j];
		t3[j] = f->planepts[1][j];
	}
	
	CrossProduct(t1,t2, f->plane.normal);
	if (VectorCompare (f->plane.normal, vec3_origin))
		printf ("WARNING: brush plane with no normal\n");
	VectorNormalize (f->plane.normal);
	f->plane.dist = DotProduct (t3, f->plane.normal);
}

/*
================
EmitTextureCoordinates
================
*/
void EmitTextureCoordinates ( float *xyzst, qtexture_t *q, face_t *f)
{
	float	STfromXYZ[2][4];

	Face_TextureVectors (f,  STfromXYZ);
	xyzst[3] = DotProduct (xyzst, STfromXYZ[0]) + STfromXYZ[0][3];
	xyzst[4] = DotProduct (xyzst, STfromXYZ[1]) + STfromXYZ[1][3];
}

//==========================================================================

/*
================
Brush_MakeFacePlanes
================
*/
void Brush_MakeFacePlanes (brush_t *b)
{
	face_t	*f;

	for (f=b->brush_faces ; f ; f=f->next)
	{
		Face_MakePlane (f);
	}
}

/*
================
DrawBrushEntityName
================
*/
void DrawBrushEntityName (brush_t *b)
{
	char	*name;
	//float	a, s, c;
	//vec3_t	mid;
	//int		i;

	if (!b->owner)
		return;		// during contruction

	if (b->owner == world_entity)
		return;

	if (b != b->owner->brushes.onext)
		return;	// not key brush

// MERGEME
#if 0
	if (!(g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES))
	{
		// draw the angle pointer
		a = FloatForKey (b->owner, "angle");
		if (a)
		{
			s = sin (a/180*Q_PI);
			c = cos (a/180*Q_PI);
			for (i=0 ; i<3 ; i++)
				mid[i] = (b->mins[i] + b->maxs[i])*0.5; 

			qglBegin (GL_LINE_STRIP);
			qglVertex3fv (mid);
			mid[0] += c*8;
			mid[1] += s*8;
			mid[2] += s*8;
			qglVertex3fv (mid);
			mid[0] -= c*4;
			mid[1] -= s*4;
			mid[2] -= s*4;
			mid[0] -= s*4;
			mid[1] += c*4;
			mid[2] += c*4;
			qglVertex3fv (mid);
			mid[0] += c*4;
			mid[1] += s*4;
			mid[2] += s*4;
			mid[0] += s*4;
			mid[1] -= c*4;
			mid[2] -= c*4;
			qglVertex3fv (mid);
			mid[0] -= c*4;
			mid[1] -= s*4;
			mid[2] -= s*4;
			mid[0] += s*4;
			mid[1] -= c*4;
			mid[2] -= c*4;
			qglVertex3fv (mid);
			qglEnd ();
		}
	}
#endif

	if (g_qeglobals.d_savedinfo.show_names)
	{
		name = ValueForKey (b->owner, "classname");
		qglRasterPos3f (b->mins[0]+4, b->mins[1]+4, b->mins[2]+4);
		qglCallLists (strlen(name), GL_UNSIGNED_BYTE, name);
	}
}

/*
=================
Brush_MakeFaceWinding

returns the visible polygon on a face
=================
*/
winding_t *Brush_MakeFaceWinding (brush_t *b, face_t *face)
{
	winding_t	*w;
	face_t		*clip;
	plane_t			plane;
	qboolean		past;

	// get a poly that covers an effectively infinite area
	w = Winding_BaseForPlane (&face->plane);

	// chop the poly by all of the other faces
	past = false;
	for (clip = b->brush_faces ; clip && w ; clip=clip->next)
	{
		if (clip == face)
		{
			past = true;
			continue;
		}
		if (DotProduct (face->plane.normal, clip->plane.normal) > 0.999
			&& fabs(face->plane.dist - clip->plane.dist) < 0.01 )
		{	// identical plane, use the later one
			if (past)
			{
				free (w);
				return NULL;
			}
			continue;
		}

		// flip the plane, because we want to keep the back side
		VectorSubtract (vec3_origin,clip->plane.normal, plane.normal);
		plane.dist = -clip->plane.dist;
		
		w = Winding_Clip (w, &plane, false);
		if (!w)
			return w;
	}
	
	if (w->numpoints < 3)
	{
		free(w);
		w = NULL;
	}

	if (!w)
		printf ("unused plane\n");

	return w;
}

/*
=================
Brush_SnapPlanepts
=================
*/
void Brush_SnapPlanepts (brush_t *b)
{
	int		i, j;
	face_t	*f;

  if (g_PrefsDlg.m_bNoClamp)
    return;

	for (f=b->brush_faces ; f; f=f->next)
		for (i=0 ; i<3 ; i++)
			for (j=0 ; j<3 ; j++)
				f->planepts[i][j] = floor (f->planepts[i][j] + 0.5);
}
	
/*
** Brush_Build
**
** Builds a brush rendering data and also sets the min/max bounds
*/
// TTimo
// added a bConvert flag to convert between old and new brush texture formats
// TTimo
// brush grouping: update the group treeview if necessary
void Brush_Build( brush_t *b, bool bSnap, bool bMarkMap, bool bConvert )
{
	bool		bLocalConvert;

#ifdef _DEBUG
	if (!g_qeglobals.m_bBrushPrimitMode && bConvert)
		Sys_Printf("Warning : conversion from brush primitive to old brush format not implemented\n");
#endif

	// if bConvert is set and g_qeglobals.bNeedConvert is not, that just means we need convert for this brush only
	if (bConvert && !g_qeglobals.bNeedConvert)
	{
		bLocalConvert = true;
		g_qeglobals.bNeedConvert = true;
	}

	/*
	** build the windings and generate the bounding box
	*/
	Brush_BuildWindings(b, bSnap);

	Patch_BuildPoints (b);

	/*
	** move the points and edges if in select mode
	*/
	if (g_qeglobals.d_select_mode == sel_vertex || g_qeglobals.d_select_mode == sel_edge)
		SetupVertexSelection ();

    if (b->itemOwner == NULL)
      Group_AddToProperGroup(b);

	if (bMarkMap)
	{
		Sys_MarkMapModified();
	}

	if (bLocalConvert)
		g_qeglobals.bNeedConvert = false;
}

/*
==============
Brush_SplitBrushByFace

The incoming brush is NOT freed.
The incoming face is NOT left referenced.
==============
*/
void Brush_SplitBrushByFace (brush_t *in, face_t *f, brush_t **front, brush_t **back)
{
	brush_t	*b;
	face_t	*nf;
	vec3_t	temp;

	b = Brush_Clone (in);
	nf = Face_Clone (f);

	nf->texdef = b->brush_faces->texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	Brush_Build( b );
	Brush_RemoveEmptyFaces ( b );
	if ( !b->brush_faces )
	{	// completely clipped away
		Brush_Free (b);
		*back = NULL;
	}
	else
	{
		Entity_LinkBrush (in->owner, b);
		*back = b;
	}

	b = Brush_Clone (in);
	nf = Face_Clone (f);
	// swap the plane winding
	VectorCopy (nf->planepts[0], temp);
	VectorCopy (nf->planepts[1], nf->planepts[0]);
	VectorCopy (temp, nf->planepts[1]);

	nf->texdef = b->brush_faces->texdef;
	nf->next = b->brush_faces;
	b->brush_faces = nf;

	Brush_Build( b );
	Brush_RemoveEmptyFaces ( b );
	if ( !b->brush_faces )
	{	// completely clipped away
		Brush_Free (b);
		*front = NULL;
	}
	else
	{
		Entity_LinkBrush (in->owner, b);
		*front = b;
	}
}

/*
=================
Brush_BestSplitFace

returns the best face to split the brush with.
return NULL if the brush is convex
=================
*/
face_t *Brush_BestSplitFace(brush_t *b)
{
	face_t *face, *f, *bestface;
	winding_t *front, *back;
	int splits, tinywindings, value, bestvalue;

	bestvalue = 999999;
	bestface = NULL;
	for (face = b->brush_faces; face; face = face->next)
	{
		splits = 0;
		tinywindings = 0;
		for (f = b->brush_faces; f; f = f->next)
		{
			if (f == face) continue;
			//
			Winding_SplitEpsilon(f->face_winding, face->plane.normal, face->plane.dist, 0.1, &front, &back);

			if (!front)
			{
				Winding_Free(back);
			}
			else if (!back)
			{
				Winding_Free(front);
			}
			else
			{
				splits++;
				if (Winding_IsTiny(front)) tinywindings++;
				if (Winding_IsTiny(back)) tinywindings++;
			}
		}
		if (splits)
		{
			value = splits + 50 * tinywindings;
			if (value < bestvalue)
			{
				bestvalue = value;
				bestface = face;
			}
		}
	}
	return bestface;
}

/*
=================
Brush_MakeConvexBrushes

MrE FIXME: this doesn't work because the old
		   Brush_SplitBrushByFace is used
Turns the brush into a minimal number of convex brushes.
If the input brush is convex then it will be returned.
Otherwise the input brush will be freed.
NOTE: the input brush should have windings for the faces.
=================
*/
brush_t *Brush_MakeConvexBrushes(brush_t *b)
{
	brush_t *front, *back, *end;
	face_t *face;

	b->next = NULL;
	face = Brush_BestSplitFace(b);
	if (!face) return b;
	Brush_SplitBrushByFace(b, face, &front, &back);
	//this should never happen
	if (!front && !back) return b;
	Brush_Free(b);
	if (!front)
		return Brush_MakeConvexBrushes(back);
	b = Brush_MakeConvexBrushes(front);
	if (back)
	{
		for (end = b; end->next; end = end->next);
		end->next = Brush_MakeConvexBrushes(back);
	}
	return b;
}

/*
=================
Brush_Convex
=================
*/
int Brush_Convex(brush_t *b)
{
	face_t *face1, *face2;

	for (face1 = b->brush_faces; face1; face1 = face1->next)
	{
		if (!face1->face_winding) continue;
		for (face2 = b->brush_faces; face2; face2 = face2->next)
		{
			if (face1 == face2) continue;
			if (!face2->face_winding) continue;
			if (Winding_PlanesConcave(face1->face_winding, face2->face_winding,
										face1->plane.normal, face2->plane.normal,
										face1->plane.dist, face2->plane.dist))
			{
				return false;
			}
		}
	}
	return true;
}

/*
=================
Brush_MoveVertexes_old1

- The input brush must have face windings.
- The input brush must be a brush with faces that do not intersect.
- The input brush does not have to be convex.
- The vertex will not be moved if the movement either causes the
  brush to have faces that intersect or causes the brush to be
  flipped inside out.
  (For instance a tetrahedron can easily be flipped inside out
  without having faces that intersect.)
- The created brush does not have to be convex.
- Returns true if the vertex movement is performed.
=================
*/

#define MAX_MOVE_FACES		64
#define INTERSECT_EPSILON	0.1
#define POINT_EPSILON		0.3

int Brush_MoveVertex_old1(brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end, bool bSnap)
{
	face_t *f, *face, *newface, *lastface, *nextface;
	face_t *movefaces[MAX_MOVE_FACES];
	int movefacepoints[MAX_MOVE_FACES];
	winding_t *w, tmpw;
	int i, j, k, nummovefaces, result;
	float dot;

	result = false;
	//
	tmpw.numpoints = 3;
	tmpw.maxpoints = 3;
	VectorAdd(vertex, delta, end);
	//snap or not?
	if (bSnap)
		for (i = 0; i < 3; i++)
			end[i] = floor(end[i] / g_qeglobals.d_gridsize + 0.5) * g_qeglobals.d_gridsize;
	//chop off triangles from all brush faces that use the to be moved vertex
	//store pointers to these chopped off triangles in movefaces[]
	nummovefaces = 0;
	for (face = b->brush_faces; face; face = face->next)
	{
		w = face->face_winding;
		if (!w) continue;
		for (i = 0; i < w->numpoints; i++)
		{
			if (Point_Equal(w->points[i], vertex, POINT_EPSILON))
			{
				if (face->face_winding->numpoints <= 3)
				{
					movefacepoints[nummovefaces] = i;
					movefaces[nummovefaces++] = face;
					break;
				}
				dot = DotProduct(end, face->plane.normal) - face->plane.dist;
				//if the end point is in front of the face plane
				if (dot > 0.1)
				{
					//fanout triangle subdivision
					for (k = i; k < i + w->numpoints-3; k++)
					{
						VectorCopy(w->points[i], tmpw.points[0]);
						VectorCopy(w->points[(k+1) % w->numpoints], tmpw.points[1]);
						VectorCopy(w->points[(k+2) % w->numpoints], tmpw.points[2]);
						//
						newface = Face_Clone(face);
						//get the original
						for (f = face; f->original; f = f->original) ;
						newface->original = f;
						//store the new winding
						if (newface->face_winding) Winding_Free(newface->face_winding);
						newface->face_winding = Winding_Clone(&tmpw);
						//get the texture
						newface->d_texture = Texture_ForName( newface->texdef.name );
						//add the face to the brush
						newface->next = b->brush_faces;
						b->brush_faces = newface;
						//add this new triangle to the move faces
						movefacepoints[nummovefaces] = 0;
						movefaces[nummovefaces++] = newface;
					}
					//give the original face a new winding
					VectorCopy(w->points[(i-2+w->numpoints) % w->numpoints], tmpw.points[0]);
					VectorCopy(w->points[(i-1+w->numpoints) % w->numpoints], tmpw.points[1]);
					VectorCopy(w->points[i], tmpw.points[2]);
					Winding_Free(face->face_winding);
					face->face_winding = Winding_Clone(&tmpw);
					//add the original face to the move faces
					movefacepoints[nummovefaces] = 2;
					movefaces[nummovefaces++] = face;
				}
				else
				{
					//chop a triangle off the face
					VectorCopy(w->points[(i-1+w->numpoints) % w->numpoints], tmpw.points[0]);
					VectorCopy(w->points[i], tmpw.points[1]);
					VectorCopy(w->points[(i+1) % w->numpoints], tmpw.points[2]);
					//remove the point from the face winding
					Winding_RemovePoint(w, i);
					//get texture crap right
					Face_SetColor(b, face, 1.0);
					for (j = 0; j < w->numpoints; j++)
						EmitTextureCoordinates(w->points[j], face->d_texture, face);
					//make a triangle face
					newface = Face_Clone(face);
					//get the original
					for (f = face; f->original; f = f->original) ;
					newface->original = f;
					//store the new winding
					if (newface->face_winding) Winding_Free(newface->face_winding);
					newface->face_winding = Winding_Clone(&tmpw);
					//get the texture
					newface->d_texture = Texture_ForName( newface->texdef.name );
					//add the face to the brush
					newface->next = b->brush_faces;
					b->brush_faces = newface;
					//
					movefacepoints[nummovefaces] = 1;
					movefaces[nummovefaces++] = newface;
				}
				break;
			}
		}
	}
	//now movefaces contains pointers to triangle faces that
	//contain the to be moved vertex

	//check if the move is valid
	int l;
	vec3_t p1, p2;
	winding_t *w2;
	plane_t plane;

	face = NULL;
	VectorCopy(vertex, tmpw.points[1]);
	VectorCopy(end, tmpw.points[2]);
	for (face = b->brush_faces; face; face = face->next)
	{
		for (i = 0; i < nummovefaces; i++)
		{
			if (face == movefaces[i])
				break;
		}
		if (i < nummovefaces)
			continue;
		//the delta vector may not intersect with any of the not move faces
		if (Winding_VectorIntersect(face->face_winding, &face->plane, vertex, end, INTERSECT_EPSILON))
			break;
		//if the end point of the to be moved vertex is near this not move face
		if (abs(DotProduct(face->plane.normal, end) - face->plane.dist) < 0.5)
		{
			//the end point may not be inside or very close to the not move face winding
			if (Winding_PointInside(face->face_winding, &face->plane, end, 0.5))
				break;
		}
		for (i = 0; i < nummovefaces; i++)
		{
			w = movefaces[i]->face_winding;
			j = movefacepoints[i];
			for (k = -1; k <= 1; k += 2)
			{
				//check if the new edge will not intersect with the not move face
				VectorCopy(w->points[(j + k + w->numpoints) % w->numpoints], tmpw.points[0]);
				if (Winding_VectorIntersect(face->face_winding, &face->plane, tmpw.points[0], end, INTERSECT_EPSILON))
				{
					//ok the new edge instersects with the not move face
					//we can't perform the vertex movement
					//break;
				}
				//check if the not move face intersects the "movement winding"
				Winding_Plane(&tmpw, plane.normal, &plane.dist);
				w2 = face->face_winding;
				for (l = 0; l < w2->numpoints; l++)
				{
					VectorCopy(w2->points[l], p1);
					if (Point_Equal(p1, tmpw.points[0], POINT_EPSILON)) continue;
					VectorCopy(w2->points[(l+1) % w2->numpoints], p2);
					if (Point_Equal(p2, tmpw.points[0], POINT_EPSILON)) continue;
					if (Winding_VectorIntersect(&tmpw, &plane, p1, p2, INTERSECT_EPSILON))
						break;
				}
				if (l < w2->numpoints)
				{
					//ok this not move face intersects the "movement winding"
					//we can't perform the vertex movement
					break;
				}
			}
			if (k <= 1) break;
		}
		if (i < nummovefaces)
			break;
	}
	if (!face)
	{
		//ok the move was valid
		//now move all the vertexes of the movefaces
		for (i = 0; i < nummovefaces; i++)
		{
			VectorCopy(end, movefaces[i]->face_winding->points[movefacepoints[i]]);
			//create new face plane
			for (j = 0; j < 3; j++)
			{
				VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);
			}
			Face_MakePlane(movefaces[i]);
		}
		result = true;
	}
	//get texture crap right
	for (i = 0; i < nummovefaces; i++)
	{
		Face_SetColor(b, movefaces[i], 1.0);
		for (j = 0; j < movefaces[i]->face_winding->numpoints; j++)
			EmitTextureCoordinates(movefaces[i]->face_winding->points[j], movefaces[i]->d_texture, movefaces[i]);
	}

	//now try to merge faces with their original faces
	lastface = NULL;
	for (face = b->brush_faces; face; face = nextface)
	{
		nextface = face->next;
		if (!face->original)
		{
			lastface = face;
			continue;
		}
		if (!Plane_Equal(&face->plane, &face->original->plane, false))
		{
			lastface = face;
			continue;
		}
		w = Winding_TryMerge(face->face_winding, face->original->face_winding, face->plane.normal, true);
		if (!w)
		{
			lastface = face;
			continue;
		}
		Winding_Free(face->original->face_winding);
		face->original->face_winding = w;
		//get texture crap right
		Face_SetColor(b, face->original, 1.0);
		for (j = 0; j < face->original->face_winding->numpoints; j++)
			EmitTextureCoordinates(face->original->face_winding->points[j], face->original->d_texture, face->original);
		//remove the face that was merged with the original
		if (lastface) lastface->next = face->next;
		else b->brush_faces = face->next;
		Face_Free(face);
	}
	return result;
}

/*
=================
Brush_MoveVertexes_old2

- The input brush must be convex
- The input brush must have face windings.
- The output brush will be convex.
- Returns true if the vertex movement is performed.
=================
*/

#define MAX_MOVE_FACES		64
#define INTERSECT_EPSILON	0.1
#define POINT_EPSILON		0.3

int Brush_MoveVertex_old2(brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end, bool bSnap)
{
	face_t *f, *face, *newface, *lastface, *nextface;
	face_t *movefaces[MAX_MOVE_FACES];
	int movefacepoints[MAX_MOVE_FACES];
	winding_t *w, tmpw;
	int i, j, k, nummovefaces, result;
	float dot;

	result = true;
	//
	tmpw.numpoints = 3;
	tmpw.maxpoints = 3;
	VectorAdd(vertex, delta, end);
	//snap or not?
	if (bSnap)
		for (i = 0; i < 3; i++)
			end[i] = floor(end[i] / g_qeglobals.d_gridsize + 0.5) * g_qeglobals.d_gridsize;
	//chop off triangles from all brush faces that use the to be moved vertex
	//store pointers to these chopped off triangles in movefaces[]
	nummovefaces = 0;
	for (face = b->brush_faces; face; face = face->next)
	{
		w = face->face_winding;
		if (!w) continue;
		for (i = 0; i < w->numpoints; i++)
		{
			if (Point_Equal(w->points[i], vertex, POINT_EPSILON))
			{
				if (face->face_winding->numpoints <= 3)
				{
					movefacepoints[nummovefaces] = i;
					movefaces[nummovefaces++] = face;
					break;
				}
				dot = DotProduct(end, face->plane.normal) - face->plane.dist;
				//if the end point is in front of the face plane
				if (dot > 0.1)
				{
					//fanout triangle subdivision
					for (k = i; k < i + w->numpoints-3; k++)
					{
						VectorCopy(w->points[i], tmpw.points[0]);
						VectorCopy(w->points[(k+1) % w->numpoints], tmpw.points[1]);
						VectorCopy(w->points[(k+2) % w->numpoints], tmpw.points[2]);
						//
						newface = Face_Clone(face);
						//get the original
						for (f = face; f->original; f = f->original) ;
						newface->original = f;
						//store the new winding
						if (newface->face_winding) Winding_Free(newface->face_winding);
						newface->face_winding = Winding_Clone(&tmpw);
						//get the texture
						newface->d_texture = Texture_ForName( newface->texdef.name );
						//add the face to the brush
						newface->next = b->brush_faces;
						b->brush_faces = newface;
						//add this new triangle to the move faces
						movefacepoints[nummovefaces] = 0;
						movefaces[nummovefaces++] = newface;
					}
					//give the original face a new winding
					VectorCopy(w->points[(i-2+w->numpoints) % w->numpoints], tmpw.points[0]);
					VectorCopy(w->points[(i-1+w->numpoints) % w->numpoints], tmpw.points[1]);
					VectorCopy(w->points[i], tmpw.points[2]);
					Winding_Free(face->face_winding);
					face->face_winding = Winding_Clone(&tmpw);
					//add the original face to the move faces
					movefacepoints[nummovefaces] = 2;
					movefaces[nummovefaces++] = face;
				}
				else
				{
					//chop a triangle off the face
					VectorCopy(w->points[(i-1+w->numpoints) % w->numpoints], tmpw.points[0]);
					VectorCopy(w->points[i], tmpw.points[1]);
					VectorCopy(w->points[(i+1) % w->numpoints], tmpw.points[2]);
					//remove the point from the face winding
					Winding_RemovePoint(w, i);
					//get texture crap right
					Face_SetColor(b, face, 1.0);
					for (j = 0; j < w->numpoints; j++)
						EmitTextureCoordinates(w->points[j], face->d_texture, face);
					//make a triangle face
					newface = Face_Clone(face);
					//get the original
					for (f = face; f->original; f = f->original) ;
					newface->original = f;
					//store the new winding
					if (newface->face_winding) Winding_Free(newface->face_winding);
					newface->face_winding = Winding_Clone(&tmpw);
					//get the texture
					newface->d_texture = Texture_ForName( newface->texdef.name );
					//add the face to the brush
					newface->next = b->brush_faces;
					b->brush_faces = newface;
					//
					movefacepoints[nummovefaces] = 1;
					movefaces[nummovefaces++] = newface;
				}
				break;
			}
		}
	}
	//now movefaces contains pointers to triangle faces that
	//contain the to be moved vertex

	//move the vertex
	for (i = 0; i < nummovefaces; i++)
	{
		//move vertex to end position
		VectorCopy(end, movefaces[i]->face_winding->points[movefacepoints[i]]);
		//create new face plane
		for (j = 0; j < 3; j++)
		{
			VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);
		}
		Face_MakePlane(movefaces[i]);
	}
	//if the brush is no longer convex
	if (!Brush_Convex(b))
	{
		for (i = 0; i < nummovefaces; i++)
		{
			//move the vertex back to the initial position
			VectorCopy(vertex, movefaces[i]->face_winding->points[movefacepoints[i]]);
			//create new face plane
			for (j = 0; j < 3; j++)
			{
				VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);
			}
			Face_MakePlane(movefaces[i]);
		}
		result = false;
	}
	//get texture crap right
	for (i = 0; i < nummovefaces; i++)
	{
		Face_SetColor(b, movefaces[i], 1.0);
		for (j = 0; j < movefaces[i]->face_winding->numpoints; j++)
			EmitTextureCoordinates(movefaces[i]->face_winding->points[j], movefaces[i]->d_texture, movefaces[i]);
	}

	//now try to merge faces with their original faces
	lastface = NULL;
	for (face = b->brush_faces; face; face = nextface)
	{
		nextface = face->next;
		if (!face->original)
		{
			lastface = face;
			continue;
		}
		if (!Plane_Equal(&face->plane, &face->original->plane, false))
		{
			lastface = face;
			continue;
		}
		w = Winding_TryMerge(face->face_winding, face->original->face_winding, face->plane.normal, true);
		if (!w)
		{
			lastface = face;
			continue;
		}
		Winding_Free(face->original->face_winding);
		face->original->face_winding = w;
		//get texture crap right
		Face_SetColor(b, face->original, 1.0);
		for (j = 0; j < face->original->face_winding->numpoints; j++)
			EmitTextureCoordinates(face->original->face_winding->points[j], face->original->d_texture, face->original);
		//remove the face that was merged with the original
		if (lastface) lastface->next = face->next;
		else b->brush_faces = face->next;
		Face_Free(face);
	}
	return result;
}

/*
=================
Brush_MoveVertexes

- The input brush must be convex
- The input brush must have face windings.
- The output brush will be convex.
- Returns true if the WHOLE vertex movement is performed.
=================
*/

#define MAX_MOVE_FACES		64

int Brush_MoveVertex(brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end, bool bSnap)
{
	face_t *f, *face, *newface, *lastface, *nextface;
	face_t *movefaces[MAX_MOVE_FACES];
	int movefacepoints[MAX_MOVE_FACES];
	winding_t *w, tmpw;
	vec3_t start, mid;
	plane_t plane;
	int i, j, k, nummovefaces, result, done;
	float dot, front, back, frac, smallestfrac;

	result = true;
	//
	tmpw.numpoints = 3;
	tmpw.maxpoints = 3;
	VectorCopy(vertex, start);
	VectorAdd(vertex, delta, end);
	//snap or not?
	if (bSnap)
		for (i = 0; i < 3; i++)
			end[i] = floor(end[i] / g_qeglobals.d_gridsize + 0.5) * g_qeglobals.d_gridsize;
	//
	VectorCopy(end, mid);
	//if the start and end are the same
	if (Point_Equal(start, end, 0.3)) return false;
	//the end point may not be the same as another vertex
	for (face = b->brush_faces; face; face = face->next)
	{
		w = face->face_winding;
		if (!w) continue;
		for (i = 0; i < w->numpoints; i++)
		{
			if (Point_Equal(w->points[i], end, 0.3))
			{
				VectorCopy(vertex, end);
				return false;
			}
		}
	}
	//
	done = false;
	while(!done)
	{
		//chop off triangles from all brush faces that use the to be moved vertex
		//store pointers to these chopped off triangles in movefaces[]
		nummovefaces = 0;
		for (face = b->brush_faces; face; face = face->next)
		{
			w = face->face_winding;
			if (!w) continue;
			for (i = 0; i < w->numpoints; i++)
			{
				if (Point_Equal(w->points[i], start, 0.2))
				{
					if (face->face_winding->numpoints <= 3)
					{
						movefacepoints[nummovefaces] = i;
						movefaces[nummovefaces++] = face;
						break;
					}
					dot = DotProduct(end, face->plane.normal) - face->plane.dist;
					//if the end point is in front of the face plane
					if (dot > 0.1)
					{
						//fanout triangle subdivision
						for (k = i; k < i + w->numpoints-3; k++)
						{
							VectorCopy(w->points[i], tmpw.points[0]);
							VectorCopy(w->points[(k+1) % w->numpoints], tmpw.points[1]);
							VectorCopy(w->points[(k+2) % w->numpoints], tmpw.points[2]);
							//
							newface = Face_Clone(face);
							//get the original
							for (f = face; f->original; f = f->original) ;
							newface->original = f;
							//store the new winding
							if (newface->face_winding) Winding_Free(newface->face_winding);
							newface->face_winding = Winding_Clone(&tmpw);
							//get the texture
							newface->d_texture = Texture_ForName( newface->texdef.name );
							//add the face to the brush
							newface->next = b->brush_faces;
							b->brush_faces = newface;
							//add this new triangle to the move faces
							movefacepoints[nummovefaces] = 0;
							movefaces[nummovefaces++] = newface;
						}
						//give the original face a new winding
						VectorCopy(w->points[(i-2+w->numpoints) % w->numpoints], tmpw.points[0]);
						VectorCopy(w->points[(i-1+w->numpoints) % w->numpoints], tmpw.points[1]);
						VectorCopy(w->points[i], tmpw.points[2]);
						Winding_Free(face->face_winding);
						face->face_winding = Winding_Clone(&tmpw);
						//add the original face to the move faces
						movefacepoints[nummovefaces] = 2;
						movefaces[nummovefaces++] = face;
					}
					else
					{
						//chop a triangle off the face
						VectorCopy(w->points[(i-1+w->numpoints) % w->numpoints], tmpw.points[0]);
						VectorCopy(w->points[i], tmpw.points[1]);
						VectorCopy(w->points[(i+1) % w->numpoints], tmpw.points[2]);
						//remove the point from the face winding
						Winding_RemovePoint(w, i);
						//get texture crap right
						Face_SetColor(b, face, 1.0);
						for (j = 0; j < w->numpoints; j++)
							EmitTextureCoordinates(w->points[j], face->d_texture, face);
						//make a triangle face
						newface = Face_Clone(face);
						//get the original
						for (f = face; f->original; f = f->original) ;
						newface->original = f;
						//store the new winding
						if (newface->face_winding) Winding_Free(newface->face_winding);
						newface->face_winding = Winding_Clone(&tmpw);
						//get the texture
						newface->d_texture = Texture_ForName( newface->texdef.name );
						//add the face to the brush
						newface->next = b->brush_faces;
						b->brush_faces = newface;
						//
						movefacepoints[nummovefaces] = 1;
						movefaces[nummovefaces++] = newface;
					}
					break;
				}
			}
		}
		//now movefaces contains pointers to triangle faces that
		//contain the to be moved vertex
		//
		done = true;
		VectorCopy(end, mid);
		smallestfrac = 1;
		for (face = b->brush_faces; face; face = face->next)
		{
			//check if there is a move face that has this face as the original
			for (i = 0; i < nummovefaces; i++)
			{
				if (movefaces[i]->original == face) break;
			}
			if (i >= nummovefaces) continue;
			//check if the original is not a move face itself
			for (j = 0; j < nummovefaces; j++)
			{
				if (face == movefaces[j]) break;
			}
			//if the original is not a move face itself
			if (j >= nummovefaces)
			{
				memcpy(&plane, &movefaces[i]->original->plane, sizeof(plane_t));
			}
			else
			{
				k = movefacepoints[j];
				w = movefaces[j]->face_winding;
				VectorCopy(w->points[(k+1)%w->numpoints], tmpw.points[0]);
				VectorCopy(w->points[(k+2)%w->numpoints], tmpw.points[1]);
				//
				k = movefacepoints[i];
				w = movefaces[i]->face_winding;
				VectorCopy(w->points[(k+1)%w->numpoints], tmpw.points[2]);
				if (!Plane_FromPoints(tmpw.points[0], tmpw.points[1], tmpw.points[2], &plane))
				{
					VectorCopy(w->points[(k+2)%w->numpoints], tmpw.points[2]);
					if (!Plane_FromPoints(tmpw.points[0], tmpw.points[1], tmpw.points[2], &plane))
						//this should never happen otherwise the face merge did a crappy job a previous pass
						continue;
				}
			}
			//now we've got the plane to check agains
			front = DotProduct(start, plane.normal) - plane.dist;
			back = DotProduct(end, plane.normal) - plane.dist;
			//if the whole move is at one side of the plane
			if (front < 0.01 && back < 0.01) continue;
			if (front > -0.01 && back > -0.01) continue;
			//if there's no movement orthogonal to this plane at all
			if (fabs(front-back) < 0.001) continue;
			//ok first only move till the plane is hit
			frac = front/(front-back);
			if (frac < smallestfrac)
			{
				mid[0] = start[0] + (end[0] - start[0]) * frac;
				mid[1] = start[1] + (end[1] - start[1]) * frac;
				mid[2] = start[2] + (end[2] - start[2]) * frac;
				smallestfrac = frac;
			}
			//
			done = false;
		}

		//move the vertex
		for (i = 0; i < nummovefaces; i++)
		{
			//move vertex to end position
			VectorCopy(mid, movefaces[i]->face_winding->points[movefacepoints[i]]);
			//create new face plane
			for (j = 0; j < 3; j++)
			{
				VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);
			}
			Face_MakePlane(movefaces[i]);
			if (VectorLength(movefaces[i]->plane.normal) < 0.1)
				result = false;
		}
		//if the brush is no longer convex
		if (!result || !Brush_Convex(b))
		{
			for (i = 0; i < nummovefaces; i++)
			{
				//move the vertex back to the initial position
				VectorCopy(start, movefaces[i]->face_winding->points[movefacepoints[i]]);
				//create new face plane
				for (j = 0; j < 3; j++)
				{
					VectorCopy(movefaces[i]->face_winding->points[j], movefaces[i]->planepts[j]);
				}
				Face_MakePlane(movefaces[i]);
			}
			result = false;
			VectorCopy(start, end);
			done = true;
		}
		else
		{
			VectorCopy(mid, start);
		}
		//get texture crap right
		for (i = 0; i < nummovefaces; i++)
		{
			Face_SetColor(b, movefaces[i], 1.0);
			for (j = 0; j < movefaces[i]->face_winding->numpoints; j++)
				EmitTextureCoordinates(movefaces[i]->face_winding->points[j], movefaces[i]->d_texture, movefaces[i]);
		}

		//now try to merge faces with their original faces
		lastface = NULL;
		for (face = b->brush_faces; face; face = nextface)
		{
			nextface = face->next;
			if (!face->original)
			{
				lastface = face;
				continue;
			}
			if (!Plane_Equal(&face->plane, &face->original->plane, false))
			{
				lastface = face;
				continue;
			}
			w = Winding_TryMerge(face->face_winding, face->original->face_winding, face->plane.normal, true);
			if (!w)
			{
				lastface = face;
				continue;
			}
			Winding_Free(face->original->face_winding);
			face->original->face_winding = w;
			//get texture crap right
			Face_SetColor(b, face->original, 1.0);
			for (j = 0; j < face->original->face_winding->numpoints; j++)
				EmitTextureCoordinates(face->original->face_winding->points[j], face->original->d_texture, face->original);
			//remove the face that was merged with the original
			if (lastface) lastface->next = face->next;
			else b->brush_faces = face->next;
			Face_Free(face);
		}
	}
	return result;
}

/*
=================
Brush_InsertVertexBetween
=================
*/
int Brush_InsertVertexBetween(brush_t *b, vec3_t p1, vec3_t p2)
{
	face_t *face;
	winding_t *w, *neww;
	vec3_t point;
	int i, insert;

	if (Point_Equal(p1, p2, 0.4))
		return false;
	VectorAdd(p1, p2, point);
	VectorScale(point, 0.5, point);
	insert = false;
	//the end point may not be the same as another vertex
	for (face = b->brush_faces; face; face = face->next)
	{
		w = face->face_winding;
		if (!w) continue;
		neww = NULL;
		for (i = 0; i < w->numpoints; i++)
		{
			if (!Point_Equal(w->points[i], p1, 0.1))
				continue;
			if (Point_Equal(w->points[(i+1) % w->numpoints], p2, 0.1))
			{
				neww = Winding_InsertPoint(w, point, (i+1) % w->numpoints);
				break;
			}
			else if (Point_Equal(w->points[(i-1+w->numpoints) % w->numpoints], p2, 0.3))
			{
				neww = Winding_InsertPoint(w, point, i);
				break;
			}
		}
		if (neww)
		{
			Winding_Free(face->face_winding);
			face->face_winding = neww;
			insert = true;
		}
	}
	return insert;
}


/*
=================
Brush_ResetFaceOriginals
=================
*/
void Brush_ResetFaceOriginals(brush_t *b)
{
	face_t *face;

	for (face = b->brush_faces; face; face = face->next)
	{
		face->original = NULL;
	}
}

/*
=================
Brush_Parse

The brush is NOT linked to any list
=================
*/
//++timo FIXME: when using old brush primitives, the test loop for "Brush" and "patchDef2" "patchDef3" is ran
// before each face parsing. It works, but it's a performance hit
brush_t *Brush_Parse (void)
{
	brush_t		*b;
	face_t		*f;
	int			i,j;
	
	g_qeglobals.d_parsed_brushes++;
	b = Brush_Alloc();

	do
	{
		if (!GetToken (true))
			break;
		if (!strcmp (token, "}") )
			break;
		
		// handle "Brush" primitive
		if (strcmpi(token, "brushDef") == 0)
		{
			// Timo parsing new brush format
			g_qeglobals.bPrimitBrushes=true;
			// check the map is not mixing the two kinds of brushes
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				if (g_qeglobals.bOldBrushes)
					Sys_Printf("Warning : old brushes and brush primitive in the same file are not allowed ( Brush_Parse )\n");
			}
			//++Timo write new brush primitive -> old conversion code for Q3->Q2 conversions ?
			else
				Sys_Printf("Warning : conversion code from brush primitive not done ( Brush_Parse )\n");
			
			BrushPrimit_Parse(b);
			if (b == NULL)
			{
				Warning ("parsing brush primitive");
				return NULL;
			}
			else
			{
        		continue;
			}
		}
		if ( strcmpi( token, "terrainDef" ) == 0 )
		{
			free (b);

			b = Terrain_Parse();
			if (b == NULL)
			{
				Warning ("parsing terrain/brush");
				return NULL;
			}
			else
			{
				continue;
			}
		}
		if (strcmpi(token, "patchDef2") == 0 || strcmpi(token, "patchDef3") == 0)
		{
			free (b);
			
			// double string compare but will go away soon
			b = Patch_Parse(strcmpi(token, "patchDef2") == 0);
			if (b == NULL)
			{
				Warning ("parsing patch/brush");
				return NULL;
			}
			else
			{
				continue;
			}
			// handle inline patch
		}
		else
		{
			// Timo parsing old brush format
			g_qeglobals.bOldBrushes=true;
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				// check the map is not mixing the two kinds of brushes
				if (g_qeglobals.bPrimitBrushes)
					Sys_Printf("Warning : old brushes and brush primitive in the same file are not allowed ( Brush_Parse )\n");
				// set the "need" conversion flag
				g_qeglobals.bNeedConvert=true;
			}
			
			f = Face_Alloc();
			
			// add the brush to the end of the chain, so
			// loading and saving a map doesn't reverse the order
			
			f->next = NULL;
			if (!b->brush_faces)
			{
				b->brush_faces = f;
			}
			else
			{
				face_t *scan;
				for (scan=b->brush_faces ; scan->next ; scan=scan->next)
					;
				scan->next = f;
			}
			
			// read the three point plane definition
			for (i=0 ; i<3 ; i++)
			{
				if (i != 0)
					GetToken (true);
				if (strcmp (token, "(") )
				{
					Warning ("parsing brush");
					return NULL;
				}
				
				for (j=0 ; j<3 ; j++)
				{
					GetToken (false);
					f->planepts[i][j] = atof(token);
				}
				
				GetToken (false);
				if (strcmp (token, ")") )
				{
					Warning ("parsing brush");
					return NULL;
				}
			}
		}

		// Timo
		// if we have a surface plugin, we'll call the plugin parsing
		if (g_qeglobals.bSurfacePropertiesPlugin)
		{
			GETPLUGINTEXDEF(f)->ParseTexdef();
		}
		else
		{
			
			// read the texturedef
			GetToken (false);
			f->texdef.SetName(token);
			if (token[0] == '(')
			{
				int i = 32;
			}
			GetToken (false);
			f->texdef.shift[0] = atoi(token);
			GetToken (false);
			f->texdef.shift[1] = atoi(token);
			GetToken (false);
			f->texdef.rotate = atoi(token);	
			GetToken (false);
			f->texdef.scale[0] = atof(token);
			GetToken (false);
			f->texdef.scale[1] = atof(token);
						
			// the flags and value field aren't necessarily present
			f->d_texture = Texture_ForName( f->texdef.name );
			f->texdef.flags = f->d_texture->flags;
			f->texdef.value = f->d_texture->value;
			f->texdef.contents = f->d_texture->contents;
			
			if (TokenAvailable ())
			{
				GetToken (false);
				f->texdef.contents = atoi(token);
				GetToken (false);
				f->texdef.flags = atoi(token);
				GetToken (false);
				f->texdef.value = atoi(token);
			}
			
		}
	} while (1);
	
	return b;
}

/*
=================
QERApp_MapPrintf_FILE
callback for surface properties plugin
must fit a PFN_QERAPP_MAPPRINTF ( see isurfaceplugin.h )
=================
*/
// carefully initialize !
FILE * g_File;
void WINAPI QERApp_MapPrintf_FILE( char *text, ... )
{
	va_list argptr;
	char	buf[32768];

	va_start (argptr,text);
	vsprintf (buf, text,argptr);
	va_end (argptr);

	fprintf( g_File, buf );
}


/*
==============
Brush_SetEpair
sets an epair for the given brush
==============
*/
void Brush_SetEpair(brush_t *b, const char *pKey, const char *pValue)
{
	if (g_qeglobals.m_bBrushPrimitMode)
	{
    if (b->patchBrush)
    {
      Patch_SetEpair(b->pPatch, pKey, pValue);
    }
    else if (b->terrainBrush)
    {
      Terrain_SetEpair(b->pTerrain, pKey, pValue);
    }
    else
    {
		  SetKeyValue(b->epairs, pKey, pValue);
    }
	}
	else
	{
		Sys_Printf("Can only set key/values in Brush primitive mode\n");
	}
}

/* 
=================
Brush_GetKeyValue
=================
*/
const char* Brush_GetKeyValue(brush_t *b, const char *pKey)
{
	if (g_qeglobals.m_bBrushPrimitMode)
	{
    if (b->patchBrush)
    {
      return Patch_GetKeyValue(b->pPatch, pKey);
    }
    else if (b->terrainBrush)
    {
      return Terrain_GetKeyValue(b->pTerrain, pKey);
    }
    else
    {
		  return ValueForKey(b->epairs, pKey);
    }
	}
	else
	{
		Sys_Printf("Can only set brush/patch key/values in Brush primitive mode\n");
	}
  return "";
}

/*
=================
Brush_Write
save all brushes as Brush primitive format
=================
*/
void Brush_Write (brush_t *b, FILE *f)
{
	epair_t	*ep;
	face_t	*fa;
	char	*pname;
	int		i;
	
	if (b->patchBrush)
	{
		Patch_Write(b->pPatch, f);
		return;
	}
	if ( b->pTerrain )
	{
		Terrain_Write(b->pTerrain, f);
		return;
	}
	if (g_qeglobals.m_bBrushPrimitMode)
	{
		// save brush primitive format
		fprintf (f, "{\nbrushDef\n{\n");
		// brush epairs
		if (b->epairs)
			for (ep = b->epairs ; ep ; ep=ep->next)
				fprintf (f, "\"%s\" \"%s\"\n", ep->key, ep->value);
		for (fa=b->brush_faces ; fa ; fa=fa->next)
		{
			// save planepts
			for (i=0 ; i<3 ; i++)
			{
				fprintf(f, "( ");
				for (int j = 0; j < 3; j++)
					if (fa->planepts[i][j] == static_cast<int>(fa->planepts[i][j]))
						fprintf(f, "%i ", static_cast<int>(fa->planepts[i][j]));
					else
						fprintf(f, "%f ", fa->planepts[i][j]);
				fprintf(f, ") ");
			}
			// save texture coordinates
			fprintf(f,"( ( ");
			for (i=0 ; i<3 ; i++)
				if (fa->brushprimit_texdef.coords[0][i] == static_cast<int>(fa->brushprimit_texdef.coords[0][i]))
					fprintf(f,"%i ",static_cast<int>(fa->brushprimit_texdef.coords[0][i]));
				else
					fprintf(f,"%f ",fa->brushprimit_texdef.coords[0][i]);
			fprintf(f,") ( ");
			for (i=0 ; i<3 ; i++)
				if (fa->brushprimit_texdef.coords[1][i] == static_cast<int>(fa->brushprimit_texdef.coords[1][i]))
					fprintf(f,"%i ",static_cast<int>(fa->brushprimit_texdef.coords[1][i]));
				else
					fprintf(f,"%f ",fa->brushprimit_texdef.coords[1][i]);
			fprintf(f,") ) ");
			// save texture attribs
			//++timo surface properties plugin not implemented for brush primitives
			if (g_qeglobals.bSurfacePropertiesPlugin)
				Sys_Printf("WARNING: surface properties plugin not supported with brush primitives (yet)\n");

      char *pName = strlen(fa->texdef.name) > 0 ? fa->texdef.name : "unnamed";
			fprintf(f, "%s ", pName );
			fprintf(f, "%i %i %i\n", fa->texdef.contents, fa->texdef.flags, fa->texdef.value);
		}
		fprintf (f, "}\n}\n");
	}
	else
	{
		fprintf (f, "{\n");
		for (fa=b->brush_faces ; fa ; fa=fa->next)
		{
			for (i=0 ; i<3 ; i++)
			{
				fprintf(f, "( ");
				for (int j = 0; j < 3; j++)
				{
					if (fa->planepts[i][j] == static_cast<int>(fa->planepts[i][j]))
						fprintf(f, "%i ", static_cast<int>(fa->planepts[i][j]));
					else
						fprintf(f, "%f ", fa->planepts[i][j]);
				}
				fprintf(f, ") ");
			}
			
			if (g_qeglobals.bSurfacePropertiesPlugin)
			{
				g_File = f;
#ifdef _DEBUG
				if (!fa->pData)
					Sys_Printf("ERROR: unexpected IPluginTexdef* is NULL in Brush_Write\n");
				else
#endif
				GETPLUGINTEXDEF(fa)->WriteTexdef( QERApp_MapPrintf_FILE );
			}
			else
			{
				pname = fa->texdef.name;
				if (pname[0] == 0)
					pname = "unnamed";
				
				fprintf (f, "%s %i %i %i ", pname,
					(int)fa->texdef.shift[0], (int)fa->texdef.shift[1],
					(int)fa->texdef.rotate);
				
				if (fa->texdef.scale[0] == (int)fa->texdef.scale[0])
					fprintf (f, "%i ", (int)fa->texdef.scale[0]);
				else
					fprintf (f, "%f ", (float)fa->texdef.scale[0]);
				if (fa->texdef.scale[1] == (int)fa->texdef.scale[1])
					fprintf (f, "%i", (int)fa->texdef.scale[1]);
				else
					fprintf (f, "%f", (float)fa->texdef.scale[1]);
				
				fprintf (f, " %i %i %i", fa->texdef.contents, fa->texdef.flags, fa->texdef.value);
			}
			fprintf (f, "\n");
		}
		fprintf (f, "}\n");
	}
}

/*
=================
QERApp_MapPrintf_MEMFILE
callback for surface properties plugin
must fit a PFN_QERAPP_MAPPRINTF ( see isurfaceplugin.h )
=================
*/
// carefully initialize !
CMemFile * g_pMemFile;
void WINAPI QERApp_MapPrintf_MEMFILE( char *text, ... )
{
	va_list argptr;
	char	buf[32768];

	va_start (argptr,text);
	vsprintf (buf, text,argptr);
	va_end (argptr);

	MemFile_fprintf( g_pMemFile, buf );
}

/*
=================
Brush_Write to a CMemFile*
save all brushes as Brush primitive format
=================
*/
void Brush_Write (brush_t *b, CMemFile *pMemFile)
{
	epair_t *ep;
	face_t	*fa;
	char *pname;
	int		i;
	
	if (b->patchBrush)
	{
		Patch_Write(b->pPatch, pMemFile);
		return;
	}
	if (b->terrainBrush)
	{
		Terrain_Write(b->pTerrain, pMemFile);
		return;
	}
	//++timo NOTE: it's not very difficult to add since the surface properties plugin
	// writes throught a printf-style function prototype
	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
		Sys_Printf("WARNING: Brush_Write to a CMemFile and Surface Properties plugin not done\n");
	}
	if (g_qeglobals.m_bBrushPrimitMode)
	{
		// brush primitive format
		MemFile_fprintf (pMemFile, "{\nBrushDef\n{\n");
		// brush epairs
		if (b->epairs)
			for( ep = b->epairs ; ep ; ep=ep->next )
				MemFile_fprintf (pMemFile, "\"%s\" \"%s\"\n", ep->key, ep->value );
		for (fa=b->brush_faces ; fa ; fa=fa->next)
		{
			// save planepts
			for (i=0 ; i<3 ; i++)
			{
				MemFile_fprintf(pMemFile, "( ");
				for (int j = 0; j < 3; j++)
					if (fa->planepts[i][j] == static_cast<int>(fa->planepts[i][j]))
						MemFile_fprintf(pMemFile, "%i ", static_cast<int>(fa->planepts[i][j]));
					else
						MemFile_fprintf(pMemFile, "%f ", fa->planepts[i][j]);
				MemFile_fprintf(pMemFile, ") ");
			}
			// save texture coordinates
			MemFile_fprintf(pMemFile,"( ( ");
			for (i=0 ; i<3 ; i++)
				if (fa->brushprimit_texdef.coords[0][i] == static_cast<int>(fa->brushprimit_texdef.coords[0][i]))
					MemFile_fprintf(pMemFile,"%i ",static_cast<int>(fa->brushprimit_texdef.coords[0][i]));
				else
					MemFile_fprintf(pMemFile,"%f ",fa->brushprimit_texdef.coords[0][i]);
			MemFile_fprintf(pMemFile,") ( ");
			for (i=0 ; i<3 ; i++)
				if (fa->brushprimit_texdef.coords[1][i] == static_cast<int>(fa->brushprimit_texdef.coords[1][i]))
					MemFile_fprintf(pMemFile,"%i ",static_cast<int>(fa->brushprimit_texdef.coords[1][i]));
				else
					MemFile_fprintf(pMemFile,"%f ",fa->brushprimit_texdef.coords[1][i]);
			MemFile_fprintf(pMemFile,") ) ");
			// save texture attribs
      char *pName = strlen(fa->texdef.name) > 0 ? fa->texdef.name : "unnamed";
			MemFile_fprintf(pMemFile, "%s ", pName);
			MemFile_fprintf(pMemFile, "%i %i %i\n", fa->texdef.contents, fa->texdef.flags, fa->texdef.value);
		}
		MemFile_fprintf (pMemFile, "}\n}\n");
	}
	else
	{
		// old brushes format
		// also handle surface properties plugin
		MemFile_fprintf (pMemFile, "{\n");
		for (fa=b->brush_faces ; fa ; fa=fa->next)
		{
			for (i=0 ; i<3 ; i++)
			{
				MemFile_fprintf(pMemFile, "( ");
				for (int j = 0; j < 3; j++)
				{
					if (fa->planepts[i][j] == static_cast<int>(fa->planepts[i][j]))
						MemFile_fprintf(pMemFile, "%i ", static_cast<int>(fa->planepts[i][j]));
					else
						MemFile_fprintf(pMemFile, "%f ", fa->planepts[i][j]);
				}
				MemFile_fprintf(pMemFile, ") ");
			}
			
			if (g_qeglobals.bSurfacePropertiesPlugin)
			{
				g_pMemFile = pMemFile;
#ifdef _DEBUG
				if (!fa->pData)
					Sys_Printf("ERROR: unexpected IPluginTexdef* is NULL in Brush_Write\n");
				else
#endif
				GETPLUGINTEXDEF(fa)->WriteTexdef( QERApp_MapPrintf_MEMFILE );
			}
			else
			{
				pname = fa->texdef.name;
				if (pname[0] == 0)
					pname = "unnamed";
				
				MemFile_fprintf (pMemFile, "%s %i %i %i ", pname,
					(int)fa->texdef.shift[0], (int)fa->texdef.shift[1],
					(int)fa->texdef.rotate);
				
				if (fa->texdef.scale[0] == (int)fa->texdef.scale[0])
					MemFile_fprintf (pMemFile, "%i ", (int)fa->texdef.scale[0]);
				else
					MemFile_fprintf (pMemFile, "%f ", (float)fa->texdef.scale[0]);
				if (fa->texdef.scale[1] == (int)fa->texdef.scale[1])
					MemFile_fprintf (pMemFile, "%i", (int)fa->texdef.scale[1]);
				else
					MemFile_fprintf (pMemFile, "%f", (float)fa->texdef.scale[1]);
				
				MemFile_fprintf (pMemFile, " %i %i %i", fa->texdef.contents, fa->texdef.flags, fa->texdef.value);
			}
			MemFile_fprintf (pMemFile, "\n");
		}
		MemFile_fprintf (pMemFile, "}\n");
	}
	
	
}


/*
=============
Brush_Create

Create non-textured blocks for entities
The brush is NOT linked to any list
=============
*/
brush_t	*Brush_Create (vec3_t mins, vec3_t maxs, texdef_t *texdef)
{
	int		i, j;
	vec3_t	pts[4][2];
	face_t	*f;
	brush_t	*b;

	// brush primitive mode : convert texdef to brushprimit_texdef ?
	// most of the time texdef is empty
	if (g_qeglobals.m_bBrushPrimitMode)
	{
		// check texdef is empty .. if there are cases it's not we need to write some conversion code
		if (texdef->shift[0]!=0 || texdef->shift[1]!=0 || texdef->scale[0]!=0 || texdef->scale[1]!=0 || texdef->rotate!=0)
			Sys_Printf("Warning : non-zero texdef detected in Brush_Create .. need brush primitive conversion\n");
	}

	for (i=0 ; i<3 ; i++)
	{
		if (maxs[i] < mins[i])
			Error ("Brush_InitSolid: backwards");
	}

	b = Brush_Alloc();
	
	pts[0][0][0] = mins[0];
	pts[0][0][1] = mins[1];
	
	pts[1][0][0] = mins[0];
	pts[1][0][1] = maxs[1];
	
	pts[2][0][0] = maxs[0];
	pts[2][0][1] = maxs[1];
	
	pts[3][0][0] = maxs[0];
	pts[3][0][1] = mins[1];
	
	for (i=0 ; i<4 ; i++)
	{
		pts[i][0][2] = mins[2];
		pts[i][1][0] = pts[i][0][0];
		pts[i][1][1] = pts[i][0][1];
		pts[i][1][2] = maxs[2];
	}

	for (i=0 ; i<4 ; i++)
	{
		f = Face_Alloc();
		f->texdef = *texdef;
		f->texdef.flags &= ~SURF_KEEP;
		f->texdef.contents &= ~CONTENTS_KEEP;
		f->next = b->brush_faces;
		b->brush_faces = f;
		j = (i+1)%4;

		VectorCopy (pts[j][1], f->planepts[0]);
		VectorCopy (pts[i][1], f->planepts[1]);
		VectorCopy (pts[i][0], f->planepts[2]);
	}
	
	f = Face_Alloc();
	f->texdef = *texdef;
	f->texdef.flags &= ~SURF_KEEP;
	f->texdef.contents &= ~CONTENTS_KEEP;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy (pts[0][1], f->planepts[0]);
	VectorCopy (pts[1][1], f->planepts[1]);
	VectorCopy (pts[2][1], f->planepts[2]);

	f = Face_Alloc();
	f->texdef = *texdef;
	f->texdef.flags &= ~SURF_KEEP;
	f->texdef.contents &= ~CONTENTS_KEEP;
	f->next = b->brush_faces;
	b->brush_faces = f;

	VectorCopy (pts[2][0], f->planepts[0]);
	VectorCopy (pts[1][0], f->planepts[1]);
	VectorCopy (pts[0][0], f->planepts[2]);

	return b;
}

/*
=============
Brush_CreatePyramid

Create non-textured pyramid for light entities
The brush is NOT linked to any list
=============
*/
brush_t	*Brush_CreatePyramid (vec3_t mins, vec3_t maxs, texdef_t *texdef)
{
	//++timo handle new brush primitive ? return here ??
	return Brush_Create(mins, maxs, texdef);

	for (int i=0 ; i<3 ; i++)
		if (maxs[i] < mins[i])
			Error ("Brush_InitSolid: backwards");

	brush_t* b = Brush_Alloc();

	vec3_t corners[4];

	float fMid = Q_rint(mins[2] + (Q_rint((maxs[2] - mins[2]) / 2)));

	corners[0][0] = mins[0];
	corners[0][1] = mins[1];
	corners[0][2] = fMid;

	corners[1][0] = mins[0];
	corners[1][1] = maxs[1];
	corners[1][2] = fMid;

	corners[2][0] = maxs[0];
	corners[2][1] = maxs[1];
	corners[2][2] = fMid;

	corners[3][0] = maxs[0];
	corners[3][1] = mins[1];
	corners[3][2] = fMid;

	vec3_t top, bottom;

	top[0] = Q_rint(mins[0] + ((maxs[0] - mins[0]) / 2));
	top[1] = Q_rint(mins[1] + ((maxs[1] - mins[1]) / 2));
	top[2] = Q_rint(maxs[2]);

	VectorCopy(top, bottom);
	bottom[2] = mins[2];

	// sides
	for (i = 0; i < 4; i++)
	{
		face_t* f = Face_Alloc();
		f->texdef = *texdef;
		f->texdef.flags &= ~SURF_KEEP;
		f->texdef.contents &= ~CONTENTS_KEEP;
		f->next = b->brush_faces;
		b->brush_faces = f;
		int j = (i+1)%4;

		VectorCopy (top, f->planepts[0]);
		VectorCopy (corners[i], f->planepts[1]);
		VectorCopy(corners[j], f->planepts[2]);

		f = Face_Alloc();
		f->texdef = *texdef;
		f->texdef.flags &= ~SURF_KEEP;
		f->texdef.contents &= ~CONTENTS_KEEP;
		f->next = b->brush_faces;
		b->brush_faces = f;

		VectorCopy (bottom, f->planepts[2]);
		VectorCopy (corners[i], f->planepts[1]);
		VectorCopy(corners[j], f->planepts[0]);
	}

	return b;
}




/*
=============
Brush_MakeSided

Makes the current brush have the given number of 2d sides
=============
*/
void Brush_MakeSided (int sides)
{
	int		i, axis;
	vec3_t	mins, maxs;
	brush_t	*b;
	texdef_t	*texdef;
	face_t	*f;
	vec3_t	mid;
	float	width;
	float	sv, cv;

	if (sides < 3)
	{
		Sys_Status ("Bad sides number", 0);
		return;
	}

	if (sides >= MAX_POINTS_ON_WINDING-4)
	{
		Sys_Printf("too many sides.\n");
		return;
	}

	if (!QE_SingleBrush ())
	{
		Sys_Status ("Must have a single brush selected", 0 );
		return;
	}

	b = selected_brushes.next;
	VectorCopy (b->mins, mins);
	VectorCopy (b->maxs, maxs);
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free (b);

	if (g_pParentWnd->ActiveXY())
	{
		switch(g_pParentWnd->ActiveXY()->GetViewType())
		{
			case XY: axis = 2; break;
			case XZ: axis = 1; break;
			case YZ: axis = 0; break;
		}
	}
	else
	{
		axis = 2;
	}

	// find center of brush
	width = 8;
	for (i = 0; i < 3; i++)
	{
		mid[i] = (maxs[i] + mins[i]) * 0.5;
		if (i == axis) continue;
		if ((maxs[i] - mins[i]) * 0.5 > width)
			width = (maxs[i] - mins[i]) * 0.5;
	}

	b = Brush_Alloc();
		
	// create top face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[2][(axis+1)%3] = mins[(axis+1)%3]; f->planepts[2][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[2][axis] = maxs[axis];
	f->planepts[1][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[1][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[1][axis] = maxs[axis];
	f->planepts[0][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[0][(axis+2)%3] = maxs[(axis+2)%3]; f->planepts[0][axis] = maxs[axis];

	// create bottom face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[0][(axis+1)%3] = mins[(axis+1)%3]; f->planepts[0][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[0][axis] = mins[axis];
	f->planepts[1][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[1][(axis+2)%3] = mins[(axis+2)%3]; f->planepts[1][axis] = mins[axis];
	f->planepts[2][(axis+1)%3] = maxs[(axis+1)%3]; f->planepts[2][(axis+2)%3] = maxs[(axis+2)%3]; f->planepts[2][axis] = mins[axis];

	for (i=0 ; i<sides ; i++)
	{
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		sv = sin (i*3.14159265*2/sides);
		cv = cos (i*3.14159265*2/sides);

		f->planepts[0][(axis+1)%3] = floor(mid[(axis+1)%3]+width*cv+0.5);
		f->planepts[0][(axis+2)%3] = floor(mid[(axis+2)%3]+width*sv+0.5);
		f->planepts[0][axis] = mins[axis];

		f->planepts[1][(axis+1)%3] = f->planepts[0][(axis+1)%3];
		f->planepts[1][(axis+2)%3] = f->planepts[0][(axis+2)%3];
		f->planepts[1][axis] = maxs[axis];

		f->planepts[2][(axis+1)%3] = floor(f->planepts[0][(axis+1)%3] - width*sv + 0.5);
		f->planepts[2][(axis+2)%3] = floor(f->planepts[0][(axis+2)%3] + width*cv + 0.5);
		f->planepts[2][axis] = maxs[axis];
	}

	Brush_AddToList (b, &selected_brushes);

	Entity_LinkBrush (world_entity, b);

	Brush_Build( b );

	Sys_UpdateWindows (W_ALL);
}



/*
=============
Brush_Free

Frees the brush with all of its faces and display list.
Unlinks the brush from whichever chain it is in.
Decrements the owner entity's brushcount.
Removes owner entity if this was the last brush
unless owner is the world.
Removes from groups
=============
*/
void Brush_Free (brush_t *b, bool bRemoveNode)
{
	face_t	*f, *next;
	epair_t	*ep, *enext;

	// remove from group
	if (bRemoveNode)
		Group_RemoveBrush(b);

	// free the patch if it's there
	if (b->patchBrush)
	{
		Patch_Delete(b->pPatch);
	}

	if( b->terrainBrush )
	{
		Terrain_Delete( b->pTerrain );
	}

	// free faces
	for (f=b->brush_faces ; f ; f=next)
	{
		next = f->next;
		Face_Free( f );
	}

	//Timo : free brush epairs
	for (ep = b->epairs ; ep ; ep=enext )
	{
		enext = ep->next;
		free (ep->key);
		free (ep->value);
		free (ep);
	}

	// unlink from active/selected list
	if (b->next)
		Brush_RemoveFromList (b);

	// unlink from entity list
	if (b->onext)
		Entity_UnlinkBrush (b);

	free (b);
}

/*
=============
Face_MemorySize
=============
*/
int Face_MemorySize(face_t *f )
{
	int size = 0;

	if (f->face_winding)
	{
		size += _msize(f->face_winding);
	}
	//f->texdef.~texdef_t();;
	size += _msize(f);
	return size;
}

/*
=============
Brush_MemorySize
=============
*/
int Brush_MemorySize(brush_t *b)
{
	face_t	*f;
	epair_t	*ep;
	int size = 0;

	//
	if (b->patchBrush)
	{
		size += Patch_MemorySize(b->pPatch);
	}
	if (b->terrainBrush)
	{
		size += Terrain_MemorySize(b->pTerrain);
	}
	//
	for (f = b->brush_faces; f; f = f->next)
	{
		size += Face_MemorySize(f);
	}
	//
	for (ep = b->epairs; ep; ep = ep->next )
	{
		size += _msize(ep->key);
		size += _msize(ep->value);
		size += _msize(ep);
	}
	size += _msize(b);
	return size;
}


/*
============
Brush_Clone

Does NOT add the new brush to any lists
============
*/
brush_t *Brush_Clone (brush_t *b)
{
	brush_t	*n = NULL;
	face_t	*f, *nf;

	if (b->patchBrush)
	{
		patchMesh_t *p = Patch_Duplicate(b->pPatch);
		Brush_RemoveFromList(p->pSymbiot);
		Entity_UnlinkBrush(p->pSymbiot);
		n = p->pSymbiot;
	}
	else if (b->terrainBrush)
	{
		terrainMesh_t *p = Terrain_Duplicate(b->pTerrain);
		Brush_RemoveFromList(p->pSymbiot);
		Entity_UnlinkBrush(p->pSymbiot);
		n = p->pSymbiot;
	}
	else
	{
  	n = Brush_Alloc();
	  n->numberId = g_nBrushId++;
		n->owner = b->owner;
		for (f=b->brush_faces ; f ; f=f->next)
		{
			nf = Face_Clone( f );
			nf->next = n->brush_faces;
			n->brush_faces = nf;
		}
	}

	return n;
}



/*
============
Brush_Clone

Does NOT add the new brush to any lists
============
*/
brush_t *Brush_FullClone(brush_t *b)
{
	brush_t	*n = NULL;
	face_t *f, *nf, *f2, *nf2;
	int j;

	if (b->patchBrush)
	{
		patchMesh_t *p = Patch_Duplicate(b->pPatch);
		Brush_RemoveFromList(p->pSymbiot);
		Entity_UnlinkBrush(p->pSymbiot);
		n = p->pSymbiot;
		n->owner = b->owner;
		Brush_Build(n);
	}
	else if (b->terrainBrush)
	{
		terrainMesh_t *p = Terrain_Duplicate(b->pTerrain);
		Brush_RemoveFromList(p->pSymbiot);
		Entity_UnlinkBrush(p->pSymbiot);
		n = p->pSymbiot;
		n->owner = b->owner;
		Brush_Build(n);
	}
	else
	{
  	n = Brush_Alloc();
   	n->numberId = g_nBrushId++;
		n->owner = b->owner;
		VectorCopy(b->mins, n->mins);
		VectorCopy(b->maxs, n->maxs);
		//
		for (f = b->brush_faces; f; f = f->next)
		{
			if (f->original) continue;
			nf = Face_FullClone(f);
			nf->next = n->brush_faces;
			n->brush_faces = nf;
			//copy all faces that have the original set to this face
			for (f2 = b->brush_faces; f2; f2 = f2->next)
			{
				if (f2->original == f)
				{
					nf2 = Face_FullClone(f2);
					nf2->next = n->brush_faces;
					n->brush_faces = nf2;
					//set original
					nf2->original = nf;
				}
			}
		}
		for (nf = n->brush_faces; nf; nf = nf->next)
		{
			Face_SetColor(n, nf, 1.0);
			if (nf->face_winding)
      {
        if (g_qeglobals.m_bBrushPrimitMode)
    			EmitBrushPrimitTextureCoordinates(nf,nf->face_winding);
        else
        {
				  for (j = 0; j < nf->face_winding->numpoints; j++)
  					EmitTextureCoordinates(nf->face_winding->points[j], nf->d_texture, nf);
        }
      }
		}
  }
	return n;
}

/*
==============
Brush_Ray

Itersects a ray with a brush
Returns the face hit and the distance along the ray the intersection occured at
Returns NULL and 0 if not hit at all
==============
*/
face_t *Brush_Ray (vec3_t origin, vec3_t dir, brush_t *b, float *dist)
{
	face_t	*f, *firstface;
	vec3_t	p1, p2;
	float	frac, d1, d2;
	int		i;

	VectorCopy (origin, p1);
	for (i=0 ; i<3 ; i++)
		p2[i] = p1[i] + dir[i]*16384;

	for (f=b->brush_faces ; f ; f=f->next)
	{
		d1 = DotProduct (p1, f->plane.normal) - f->plane.dist;
		d2 = DotProduct (p2, f->plane.normal) - f->plane.dist;
		if (d1 >= 0 && d2 >= 0)
		{
			*dist = 0;
			return NULL;	// ray is on front side of face
		}
		if (d1 <=0 && d2 <= 0)
			continue;
	// clip the ray to the plane
		frac = d1 / (d1 - d2);
		if (d1 > 0)
		{
			firstface = f;
			for (i=0 ; i<3 ; i++)
				p1[i] = p1[i] + frac *(p2[i] - p1[i]);
		}
		else
		{
			for (i=0 ; i<3 ; i++)
				p2[i] = p1[i] + frac *(p2[i] - p1[i]);
		}
	}

	// find distance p1 is along dir
	VectorSubtract (p1, origin, p1);
	d1 = DotProduct (p1, dir);

	*dist = d1;

	return firstface;
}

//PGM
face_t *Brush_Point (vec3_t origin, brush_t *b)
{
	face_t	*f;
	float	d1;

	for (f=b->brush_faces ; f ; f=f->next)
	{
		d1 = DotProduct (origin, f->plane.normal) - f->plane.dist;
		if (d1 > 0)
		{
			return NULL;	// point is on front side of face
		}
	}

	return b->brush_faces;
}
//PGM


void	Brush_AddToList (brush_t *b, brush_t *list)
{
	if (b->next || b->prev)
		Error ("Brush_AddToList: allready linked");
	
	if (list == &selected_brushes || list == &active_brushes)
	{
		if (b->patchBrush && list == &selected_brushes)
		{
			Patch_Select(b->pPatch);
		}
		if (b->terrainBrush && list == &selected_brushes) {
			Terrain_Select(b->pTerrain);
		}
	}
	b->next = list->next;
	list->next->prev = b;
	list->next = b;
	b->prev = list;
	
	// TTimo messaging
	DispatchRadiantMsg( RADIANT_SELECTION );	
}

void	Brush_RemoveFromList (brush_t *b)
{
	if (!b->next || !b->prev)
		Error ("Brush_RemoveFromList: not linked");
	
	if (b->patchBrush)
	{
		Patch_Deselect(b->pPatch);
		//Patch_Deselect(b->nPatchID);
	}
	if (b->terrainBrush)
	{
		Terrain_Deselect(b->pTerrain);
	}

	b->next->prev = b->prev;
	b->prev->next = b->next;
	b->next = b->prev = NULL;
}

/*
===============
SetFaceTexdef

Doesn't set the curve flags

NOTE : ( TTimo )
	never trust f->d_texture here, f->texdef and f->d_texture are out of sync when called by Brush_SetTexture
	use Texture_ForName() to find the right shader
	FIXME : send the right shader ( qtexture_t * ) in the parameters ?

TTimo: surface plugin, added an IPluginTexdef* parameter
		if not NULL, get ->Copy() of it into the face ( and remember to hook )
		if NULL, ask for a default
===============
*/
void SetFaceTexdef (brush_t *b, face_t *f, texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale, IPluginTexdef* pPlugTexdef) {
	int		oldFlags;
	int		oldContents;
	face_t	*tf;

	oldFlags = f->texdef.flags;
	oldContents = f->texdef.contents;
	if (g_qeglobals.m_bBrushPrimitMode)
	{
		f->texdef = *texdef;
		ConvertTexMatWithQTexture( brushprimit_texdef, NULL, &f->brushprimit_texdef, Texture_ForName( f->texdef.name ) );
	}
	else
		if (bFitScale)
		{
			f->texdef = *texdef;
			// fit the scaling of the texture on the actual plane
			vec3_t p1,p2,p3; // absolute coordinates
			// compute absolute coordinates
			ComputeAbsolute(f,p1,p2,p3);
			// compute the scale
			vec3_t vx,vy;
			VectorSubtract(p2,p1,vx);
			VectorNormalize(vx);
			VectorSubtract(p3,p1,vy);
			VectorNormalize(vy);
			// assign scale
			VectorScale(vx,texdef->scale[0],vx);
			VectorScale(vy,texdef->scale[1],vy);
			VectorAdd(p1,vx,p2);
			VectorAdd(p1,vy,p3);
			// compute back shift scale rot
			AbsoluteToLocal(f->plane,f,p1,p2,p3);
		}
		else
			f->texdef = *texdef;
	f->texdef.flags = (f->texdef.flags & ~SURF_KEEP) | (oldFlags & SURF_KEEP);
	f->texdef.contents = (f->texdef.contents & ~CONTENTS_KEEP) | (oldContents & CONTENTS_KEEP);

	// surface plugin
	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
#ifdef _DEBUG
		if (!f->pData)
			Sys_Printf("ERROR: unexpected IPluginTexdef* is NULL in SetFaceTexdef\n");
		else
#endif
			GETPLUGINTEXDEF(f)->DecRef();
		IPluginTexdef *pTexdef = NULL;
		if ( pPlugTexdef )
		{
			pTexdef = pPlugTexdef->Copy();
			pTexdef->Hook( f );
		}
		else
			pTexdef = g_SurfaceTable.m_pfnTexdefAlloc( f );
		f->pData = pTexdef;
	}

	// if this is a curve face, set all other curve faces to the same texdef
	if (f->texdef.flags & SURF_CURVE) 
	{
		for (tf = b->brush_faces ; tf ; tf = tf->next) 
		{
			if (tf->texdef.flags & SURF_CURVE) 
				tf->texdef = f->texdef;
		}
	}
}


void Brush_SetTexture (brush_t *b, texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale, IPluginTexdef* pTexdef)
{
	for (face_t* f = b->brush_faces ; f ; f = f->next) 
	{
		SetFaceTexdef (b, f, texdef, brushprimit_texdef, bFitScale, pTexdef);
	}
	Brush_Build( b );
	if (b->patchBrush)
	{
		//++timo clean
//		Sys_Printf("WARNING: Brush_SetTexture needs surface plugin code for patches\n");
		Patch_SetTexture(b->pPatch, texdef, pTexdef );
	}
	if (b->terrainBrush)
	{
		Terrain_SetTexture(b->pTerrain, texdef);
	}

}


qboolean ClipLineToFace (vec3_t p1, vec3_t p2, face_t *f)
{
	float	d1, d2, fr;
	int		i;
	float	*v;

	d1 = DotProduct (p1, f->plane.normal) - f->plane.dist;
	d2 = DotProduct (p2, f->plane.normal) - f->plane.dist;

	if (d1 >= 0 && d2 >= 0)
		return false;		// totally outside
	if (d1 <= 0 && d2 <= 0)
		return true;		// totally inside

	fr = d1 / (d1 - d2);

	if (d1 > 0)
		v = p1;
	else
		v = p2;

	for (i=0 ; i<3 ; i++)
		v[i] = p1[i] + fr*(p2[i] - p1[i]);

	return true;
}


int AddPlanept (float *f)
{
	int		i;

	for (i=0 ; i<g_qeglobals.d_num_move_points ; i++)
		if (g_qeglobals.d_move_points[i] == f)
			return 0;
	g_qeglobals.d_move_points[g_qeglobals.d_num_move_points++] = f;
	return 1;
}

/*
==============
Brush_SelectFaceForDragging

Adds the faces planepts to move_points, and
rotates and adds the planepts of adjacent face if shear is set
==============
*/
void Brush_SelectFaceForDragging (brush_t *b, face_t *f, qboolean shear)
{
	int		i;
	face_t	*f2;
	winding_t	*w;
	float	d;
	brush_t	*b2;
	int		c;

	if (b->owner->eclass->fixedsize)
		return;

	c = 0;
	for (i=0 ; i<3 ; i++)
		c += AddPlanept (f->planepts[i]);
	if (c == 0)
		return;		// allready completely added

	// select all points on this plane in all brushes the selection
	for (b2=selected_brushes.next ; b2 != &selected_brushes ; b2 = b2->next)
	{
		if (b2 == b)
			continue;
		for (f2=b2->brush_faces ; f2 ; f2=f2->next)
		{
			for (i=0 ; i<3 ; i++)
				if (fabs(DotProduct(f2->planepts[i], f->plane.normal)
				-f->plane.dist) > ON_EPSILON)
					break;
			if (i==3)
			{	// move this face as well
				Brush_SelectFaceForDragging (b2, f2, shear);
				break;
			}
		}
	}


	// if shearing, take all the planes adjacent to 
	// selected faces and rotate their points so the
	// edge clipped by a selcted face has two of the points
	if (!shear)
		return;

	for (f2=b->brush_faces ; f2 ; f2=f2->next)
	{
		if (f2 == f)
			continue;
		w = Brush_MakeFaceWinding (b, f2);
		if (!w)
			continue;

		// any points on f will become new control points
		for (i=0 ; i<w->numpoints ; i++)
		{
			d = DotProduct (w->points[i], f->plane.normal) 
				- f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				break;
		}

		//
		// if none of the points were on the plane,
		// leave it alone
		//
		if (i != w->numpoints)
		{
			if (i == 0)
			{	// see if the first clockwise point was the
				// last point on the winding
				d = DotProduct (w->points[w->numpoints-1]
					, f->plane.normal) - f->plane.dist;
				if (d > -ON_EPSILON && d < ON_EPSILON)
					i = w->numpoints - 1;
			}

			AddPlanept (f2->planepts[0]);

			VectorCopy (w->points[i], f2->planepts[0]);
			if (++i == w->numpoints)
				i = 0;
			
			// see if the next point is also on the plane
			d = DotProduct (w->points[i]
				, f->plane.normal) - f->plane.dist;
			if (d > -ON_EPSILON && d < ON_EPSILON)
				AddPlanept (f2->planepts[1]);

			VectorCopy (w->points[i], f2->planepts[1]);
			if (++i == w->numpoints)
				i = 0;

			// the third point is never on the plane

			VectorCopy (w->points[i], f2->planepts[2]);
		}

		free(w);
	}
}

/*
==============
Brush_SideSelect

The mouse click did not hit the brush, so grab one or more side
planes for dragging
==============
*/
void Brush_SideSelect (brush_t *b, vec3_t origin, vec3_t dir
					   , qboolean shear)
{
	face_t	*f, *f2;
	vec3_t	p1, p2;

  //if (b->patchBrush)
  //  return;
    //Patch_SideSelect(b->nPatchID, origin, dir);
	for (f=b->brush_faces ; f ; f=f->next)
	{
		VectorCopy (origin, p1);
		VectorMA (origin, 16384, dir, p2);

		for (f2=b->brush_faces ; f2 ; f2=f2->next)
		{
			if (f2 == f)
				continue;
			ClipLineToFace (p1, p2, f2);
		}

		if (f2)
			continue;

		if (VectorCompare (p1, origin))
			continue;
		if (ClipLineToFace (p1, p2, f))
			continue;

		Brush_SelectFaceForDragging (b, f, shear);
	}

	
}

void Brush_BuildWindings( brush_t *b, bool bSnap )
{
	winding_t *w;
	face_t    *face;
	vec_t      v;

	if (bSnap)
		Brush_SnapPlanepts( b );

	// clear the mins/maxs bounds
	b->mins[0] = b->mins[1] = b->mins[2] = 99999;
	b->maxs[0] = b->maxs[1] = b->maxs[2] = -99999;

	Brush_MakeFacePlanes (b);

	face = b->brush_faces;

	float fCurveColor = 1.0;

	for ( ; face ; face=face->next)
	{
		int i, j;
		free(face->face_winding);
		w = face->face_winding = Brush_MakeFaceWinding (b, face);
		face->d_texture = Texture_ForName( face->texdef.name );

		if (!w)
			continue;
	
		for (i=0 ; i<w->numpoints ; i++)
		{
			// add to bounding box
			for (j=0 ; j<3 ; j++)
			{
				v = w->points[i][j];
				if (v > b->maxs[j])
					b->maxs[j] = v;
				if (v < b->mins[j])
					b->mins[j] = v;
			}
		}
		// setup s and t vectors, and set color
		//if (!g_PrefsDlg.m_bGLLighting)
    //{
		  Face_SetColor (b, face, fCurveColor);
    //}

		fCurveColor -= .10;
		if (fCurveColor <= 0)
			fCurveColor = 1.0;

		// computing ST coordinates for the windings
		if (g_qeglobals.m_bBrushPrimitMode)
		{
			if (g_qeglobals.bNeedConvert)
			{
				// we have parsed old brushes format and need conversion
				// convert old brush texture representation to new format
				FaceToBrushPrimitFace(face);
#ifdef _DEBUG
				// use old texture coordinates code to check against
			    for (i=0 ; i<w->numpoints ; i++)
					EmitTextureCoordinates( w->points[i], face->d_texture, face);
#endif
			}
			// use new texture representation to compute texture coordinates
			// in debug mode we will check against old code and warn if there are differences
			EmitBrushPrimitTextureCoordinates(face,w);
		}
		else
		{
		    for (i=0 ; i<w->numpoints ; i++)
				EmitTextureCoordinates( w->points[i], face->d_texture, face);
		}
	}
}

/*
==================
Brush_RemoveEmptyFaces

Frees any overconstraining faces
==================
*/
void Brush_RemoveEmptyFaces ( brush_t *b )
{
	face_t	*f, *next;

	f = b->brush_faces;
	b->brush_faces = NULL;

	for ( ; f ; f=next)
	{
		next = f->next;
		if (!f->face_winding)
			Face_Free (f);
		else
		{
			f->next = b->brush_faces;
			b->brush_faces = f;
		}

	}
}

void Brush_SnapToGrid(brush_t *pb)
{
	for (face_t *f = pb->brush_faces ; f; f = f->next)
	{
		for (int i = 0 ;i < 3 ;i++)
		{
			for (int j = 0 ;j < 3 ; j++)
			{
				f->planepts[i][j] = floor (f->planepts[i][j] / g_qeglobals.d_gridsize + 0.5) * g_qeglobals.d_gridsize;
			}
		}
	}
	Brush_Build(pb);
}

void Brush_Rotate(brush_t *b, vec3_t vAngle, vec3_t vOrigin, bool bBuild)
{
	for (face_t* f=b->brush_faces ; f ; f=f->next)
	{
		for (int i=0 ; i<3 ; i++)
		{
			VectorRotate(f->planepts[i], vAngle, vOrigin, f->planepts[i]);
		}
	}
	if (bBuild)
	{
		Brush_Build(b, false, false);
	}
}

void Brush_Center(brush_t *b, vec3_t vNewCenter)
{
  vec3_t vMid;
  // get center of the brush
  for (int j = 0; j < 3; j++)
  {
    vMid[j] = b->mins[j] + abs((b->maxs[j] - b->mins[j]) * 0.5);
  }
  // calc distance between centers
  VectorSubtract(vNewCenter, vMid, vMid);
  Brush_Move(b, vMid, true);

}

// only designed for fixed size entity brushes
void Brush_Resize(brush_t *b, vec3_t vMin, vec3_t vMax)
{
  brush_t *b2 = Brush_Create(vMin, vMax, &b->brush_faces->texdef);

  face_t *next;
	for (face_t *f=b->brush_faces ; f ; f=next)
	{
		next = f->next;
		Face_Free( f );
	}

  b->brush_faces = b2->brush_faces;

	// unlink from active/selected list
	if (b2->next)
  Brush_RemoveFromList (b2);
  free(b2);
  Brush_Build(b, true);
}


eclass_t* HasModel(brush_t *b)
{
  vec3_t vMin, vMax;
  vMin[0] = vMin[1] = vMin[2] = 9999;
  vMax[0] = vMax[1] = vMax[2] = -9999;

  if (b->owner->md3Class != NULL)
  {
    return b->owner->md3Class;
  }

  if (Eclass_hasModel(b->owner->eclass, vMin, vMax))
  {
    return b->owner->eclass;
  }

  eclass_t *e = NULL;
  // FIXME: entity needs to track whether a cache hit failed and not ask again
  if (b->owner->eclass->nShowFlags & ECLASS_MISCMODEL)
  {
    char *pModel = ValueForKey(b->owner, "model");
    if (pModel != NULL && strlen(pModel) > 0)
    {
      e = GetCachedModel(b->owner, pModel, vMin, vMax);
      if (e != NULL)
      {
        // we need to scale the brush to the proper size based on the model load
        // recreate brush just like in load/save

		    VectorAdd (vMin, b->owner->origin, vMin);
		    VectorAdd (vMax, b->owner->origin, vMax);

        Brush_Resize(b, vMin, vMax);

/*
        //
        vec3_t vTemp, vTemp2;
        VectorSubtract(b->maxs, b->mins, vTemp);
        VectorSubtract(vMax, vMin, vTemp2);
        for (int i = 0; i < 3; i++)
        {
          if (vTemp[i] != 0)
          {
            vTemp2[i] /= vTemp[i];
          }
        }
        vec3_t vMid, vMid2;
        vMid[0] = vMid[1] = vMid[2] = 0.0;
        vMid2[0] = vMid2[1] = vMid2[2] = 0.0;

        for (int j = 0; j < 3; j++)
        {
          vMid2[j] = b->mins[j] + abs((b->maxs[j] - b->mins[j]) * 0.5);
        }

        //VectorSubtract(vMid2, vMid, vMid2);

		    for (face_t* f=b->brush_faces ; f ; f=f->next)
		    {
			    for (int i=0 ; i<3 ; i++)
			    {

            // scale
            VectorSubtract(f->planepts[i], vMid2, f->planepts[i]);
            f->planepts[i][0] *= vTemp2[0];
            f->planepts[i][1] *= vTemp2[1];
            f->planepts[i][2] *= vTemp2[2];
            VectorAdd(f->planepts[i], vMid2, f->planepts[i]);
          }
        }

        //Brush_Center(b, b->owner->origin);

        //Brush_SnapToGrid(b);
/*
        float a = FloatForKey (b->owner, "angle");
        if (a)
        {
          vec3_t vAngle;
          vAngle[0] = vAngle[1] = 0;
          vAngle[2] = a;
          Brush_Rotate(b, vAngle, b->owner->origin);
        }
        else
        {
          Brush_Build(b, true);
*/
//        }

        b->bModelFailed = false;
      }
      else
      {
        b->bModelFailed = true;
      }
    } 
  }
  return e;
}

static bool g_bInPaintedModel = false;
static bool g_bDoIt = false;
bool PaintedModel(brush_t *b, bool bOkToTexture)
{
    if (g_bInPaintedModel)
    { 
      return true;
    }
    
    if (g_PrefsDlg.m_nEntityShowState == ENTITY_BOX || b->bModelFailed)
    {
      return false;
    }
    else if (!IsBrushSelected(b) && (g_PrefsDlg.m_nEntityShowState & ENTITY_SELECTED_ONLY))
    {
	    return false;
    }

    g_bInPaintedModel = true;
    bool bReturn = false;

    eclass_t *pEclass = HasModel(b);

    if (pEclass)
    {
      qglPushAttrib(GL_ALL_ATTRIB_BITS);
      entitymodel *model = pEclass->model;


      float a = FloatForKey (b->owner, "angle");
      while (model != NULL)
      {
        if (bOkToTexture == false || g_PrefsDlg.m_nEntityShowState & ENTITY_WIREFRAME || model->nTextureBind == -1)	// skinned
        {
	        qglDisable( GL_CULL_FACE );
	        qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	        qglDisable(GL_TEXTURE_2D);
          qglColor3fv(pEclass->color);
        }
        else
        {
          qglColor3f(1, 1, 1);
          qglEnable(GL_TEXTURE_2D);
	        qglBindTexture( GL_TEXTURE_2D, model->nTextureBind );
        }
        vec3_t v;
        
        int i,j;
        VectorAdd(b->maxs, b->mins, v);
        VectorScale(v, 0.5, v);
        VectorCopy(b->owner->origin, v);

	      
        //for (i = 0; i < 3; i++)
        //{
        //  v[i] -= (pEclass->mins[i] - b->mins[i]);
        //}

        //if (model->nModelPosition)
        //{
		      //v[2] = b->mins[2] - (pEclass->mins[2]);
        //}

        float s, c;
	      if (a)
        {
		      s = sin (a/180*Q_PI);
		      c = cos (a/180*Q_PI);
        }

        vec3_t vSin;
        vec3_t vCos;
        VectorClear(vSin);
        VectorClear(vCos);
        for ( j = 0; j < 3; j++)
        {
          if (b->owner->vRotation[j])
          {
            vSin[j] = sin(b->owner->vRotation[j]/180*Q_PI);
            vCos[j] = cos(b->owner->vRotation[j]/180*Q_PI);
          }
        }


	      qglBegin (GL_TRIANGLES);

        vec5_t vTest[3];
        for (i = 0; i < model->nTriCount; i++)
        {
          for (j = 0; j < 3; j++)
          {
#if 1
            float x = model->pTriList[i].v[j][0] + v[0];
            float y = model->pTriList[i].v[j][1] + v[1];
            if (a)
            {
              float x2 = (((x - v[0]) * c) - ((y - v[1]) * s)) + v[0];
              float y2 = (((x - v[0]) * s) + ((y - v[1]) * c)) + v[1];
              x = x2;
              y = y2;
            }
            //qglTexCoord2f (pEclass->pTriList[i].st[j][0] / pEclass->nSkinWidth, pEclass->pTriList[i].st[j][1] / pEclass->nSkinHeight);
            qglTexCoord2f (model->pTriList[i].st[j][0], model->pTriList[i].st[j][1]);
            qglVertex3f(x, y, model->pTriList[i].v[j][2] + v[2]);
#else
            float x = model->pTriList[i].v[j][0] + v[0];
            float y = model->pTriList[i].v[j][1] + v[1];
            float z = model->pTriList[i].v[j][2] + v[2];

            if (b->owner->vRotation[0])
            {
              float y2 = (((y - v[1]) * vCos[0]) - ((z - v[2]) * vSin[0])) + v[1];
              float z2 = (((y - v[1]) * vSin[0]) + ((z - v[2]) * vCos[0])) + v[2];
              y = y2;
              z = z2;
            }
            if (b->owner->vRotation[1])
            {
              float z2 = (((z - v[2]) * vCos[1]) - ((x - v[0]) * vSin[1])) + v[2];
              float x2 = (((z - v[2]) * vSin[1]) + ((x - v[0]) * vCos[1])) + v[0];
              x = x2;
              z = z2;
            }
            if (b->owner->vRotation[2])
            {
              float x2 = (((x - v[0]) * vCos[2]) - ((y - v[1]) * vSin[2])) + v[0];
              float y2 = (((x - v[0]) * vSin[2]) + ((y - v[1]) * vCos[2])) + v[1];
              x = x2;
              y = y2;
            }
            qglTexCoord2f (model->pTriList[i].st[j][0], model->pTriList[i].st[j][1]);
            qglVertex3f(x, y, z);
#endif
            if (g_bDoIt)
            {
              vTest[j][0] = x;
              vTest[j][1] = y;
              vTest[j][2] = model->pTriList[i].v[j][2] + v[2];
              vTest[j][3] = model->pTriList[i].st[j][0];
              vTest[j][4] = model->pTriList[i].st[j][1];
            }

          }
          if (g_bDoIt)
          {
            Patch_FromTriangle(vTest[0], vTest[1], vTest[2]);
          }
        }
        qglEnd();
        if (g_PrefsDlg.m_nEntityShowState & ENTITY_WIREFRAME)	// skinned
        {
          qglEnable(GL_CULL_FACE );
          qglEnable(GL_TEXTURE_2D);
	        qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        }
        else
        {
	        qglDisable(GL_TEXTURE_2D);
        }
        model = model->pNext;
      }

      if (g_bDoIt)
      {
        g_bDoIt = false;
      }

      vec3_t vColor;
      VectorScale(pEclass->color, 0.50, vColor);

      vec3_t vCenter, vMin, vMax;
      VectorCopy(b->owner->origin, vCenter);

		  qglColor3fv(vColor);
      qglPointSize(4);

      qglBegin(GL_POINTS);
      qglVertex3fv(b->owner->origin);
      qglEnd();

      qglBegin(GL_LINES);
      vCenter[0] -= 8;
      qglVertex3fv(vCenter);
      vCenter[0] += 16;
      qglVertex3fv(vCenter);
      vCenter[0] -= 8;
      vCenter[1] -= 8;
      qglVertex3fv(vCenter);
      vCenter[1] += 16;
      qglVertex3fv(vCenter);
      vCenter[1] -= 8;
      vCenter[2] -= 8;
      qglVertex3fv(vCenter);
      vCenter[2] += 16;
      qglVertex3fv(vCenter);
      vCenter[2] -= 8;
      qglEnd();

      VectorCopy(vCenter, vMin);
      VectorCopy(vCenter, vMax);
      vMin[0] -= 4;
      vMin[1] -= 4;
      vMin[2] -= 4;
      vMax[0] += 4;
      vMax[1] += 4;
      vMax[2] += 4;

		    qglBegin(GL_LINE_LOOP);
          qglVertex3f(vMin[0],vMin[1],vMin[2]);
          qglVertex3f(vMax[0],vMin[1],vMin[2]);
          qglVertex3f(vMax[0],vMax[1],vMin[2]);
          qglVertex3f(vMin[0],vMax[1],vMin[2]);
		    qglEnd();
		    
		    qglBegin(GL_LINE_LOOP);
	        qglVertex3f(vMin[0],vMin[1],vMax[2]);
		      qglVertex3f(vMax[0],vMin[1],vMax[2]);
		      qglVertex3f(vMax[0],vMax[1],vMax[2]);
		      qglVertex3f(vMin[0],vMax[1],vMax[2]);
		    qglEnd();

		    qglBegin(GL_LINES);
  		    qglVertex3f(vMin[0],vMin[1],vMin[2]);
		      qglVertex3f(vMin[0],vMin[1],vMax[2]);
		      qglVertex3f(vMin[0],vMax[1],vMax[2]);
		      qglVertex3f(vMin[0],vMax[1],vMin[2]);
		      qglVertex3f(vMax[0],vMin[1],vMin[2]);
		      qglVertex3f(vMax[0],vMin[1],vMax[2]);
		      qglVertex3f(vMax[0],vMax[1],vMax[2]);
		      qglVertex3f(vMax[0],vMax[1],vMin[2]);
		    qglEnd();


	    if (g_PrefsDlg.m_nEntityShowState & ENTITY_BOXED)
	    {
		    qglColor3fv(pEclass->color);

        vec3_t mins, maxs;
        VectorCopy(b->mins, mins);
        VectorCopy(b->maxs, maxs);
/*
        if (a)
        {
          vec3_t vAngle;
          vAngle[0] = vAngle[1] = 0;
          vAngle[2] = a;
          VectorRotate(mins, vAngle, b->owner->origin, mins);
          VectorRotate(maxs, vAngle, b->owner->origin, maxs);
        }
*/
		    qglBegin(GL_LINE_LOOP);
          qglVertex3f(mins[0],mins[1],mins[2]);
          qglVertex3f(maxs[0],mins[1],mins[2]);
          qglVertex3f(maxs[0],maxs[1],mins[2]);
          qglVertex3f(mins[0],maxs[1],mins[2]);
		    qglEnd();
		    
		    qglBegin(GL_LINE_LOOP);
	        qglVertex3f(mins[0],mins[1],maxs[2]);
		      qglVertex3f(maxs[0],mins[1],maxs[2]);
		      qglVertex3f(maxs[0],maxs[1],maxs[2]);
		      qglVertex3f(mins[0],maxs[1],maxs[2]);
		    qglEnd();

		    qglBegin(GL_LINES);
  		    qglVertex3f(mins[0],mins[1],mins[2]);
		      qglVertex3f(mins[0],mins[1],maxs[2]);
		      qglVertex3f(mins[0],maxs[1],maxs[2]);
		      qglVertex3f(mins[0],maxs[1],mins[2]);
		      qglVertex3f(maxs[0],mins[1],mins[2]);
		      qglVertex3f(maxs[0],mins[1],maxs[2]);
		      qglVertex3f(maxs[0],maxs[1],maxs[2]);
		      qglVertex3f(maxs[0],maxs[1],mins[2]);
		    qglEnd();
      }
	    qglPopAttrib();
      bReturn = true;
    }
    else
    {
      b->bModelFailed = true;
    }

  g_bInPaintedModel = false;
  return bReturn;
}
/*
//++timo moved out to mahlib.h
//++timo remove
void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * Q_PI / 180;
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * Q_PI / 180;
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * Q_PI / 180;
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}
*/
void FacingVectors (entity_t *e, vec3_t forward, vec3_t right, vec3_t up)
{
	int			angleVal;
	vec3_t		angles;

	angleVal = IntForKey(e, "angle");
	if (angleVal == -1)				// up
	{
		VectorSet(angles, 270, 0, 0);
	}
	else if(angleVal == -2)		// down
	{
		VectorSet(angles, 90, 0, 0);
	}
	else
	{
		VectorSet(angles, 0, angleVal, 0);
	}

	AngleVectors(angles, forward, right, up);
}

void Brush_DrawFacingAngle (brush_t *b, entity_t *e)
{
	vec3_t	forward, right, up;
	vec3_t	endpoint, tip1, tip2;
	vec3_t	start;
	float	dist;

	VectorAdd(e->brushes.onext->mins, e->brushes.onext->maxs, start);
	VectorScale(start, 0.5, start);
	dist = (b->maxs[0] - start[0]) * 2.5;

	FacingVectors (e, forward, right, up);
	VectorMA (start, dist, forward, endpoint);

	dist = (b->maxs[0] - start[0]) * 0.5;
	VectorMA (endpoint, -dist, forward, tip1);
	VectorMA (tip1, -dist, up, tip1);
	VectorMA (tip1, 2*dist, up, tip2);

	qglColor4f (1, 1, 1, 1);
	qglLineWidth (4);
	qglBegin (GL_LINES);
	qglVertex3fv (start);
	qglVertex3fv (endpoint);
	qglVertex3fv (endpoint);
	qglVertex3fv (tip1);
	qglVertex3fv (endpoint);
	qglVertex3fv (tip2);
	qglEnd ();
	qglLineWidth (1);
}

void DrawLight(brush_t *b)
{
	vec3_t vTriColor;
	bool bTriPaint = false;

  vTriColor[0] = vTriColor[2] = 1.0;
  vTriColor[1]  = 1.0;
  bTriPaint = true;
  CString strColor = ValueForKey(b->owner, "_color");
  if (strColor.GetLength() > 0)
  {
    float fR, fG, fB;
	  int n = sscanf(strColor,"%f %f %f", &fR, &fG, &fB);
    if (n == 3)
    {
      vTriColor[0] = fR;
      vTriColor[1] = fG;
      vTriColor[2] = fB;
    }
  }
  qglColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);

  vec3_t vCorners[4];
  float fMid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

  vCorners[0][0] = b->mins[0];
  vCorners[0][1] = b->mins[1];
  vCorners[0][2] = fMid;

  vCorners[1][0] = b->mins[0];
  vCorners[1][1] = b->maxs[1];
  vCorners[1][2] = fMid;

  vCorners[2][0] = b->maxs[0];
  vCorners[2][1] = b->maxs[1];
  vCorners[2][2] = fMid;

  vCorners[3][0] = b->maxs[0];
  vCorners[3][1] = b->mins[1];
  vCorners[3][2] = fMid;

  vec3_t vTop, vBottom;

  vTop[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
  vTop[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
  vTop[2] = b->maxs[2];

  VectorCopy(vTop, vBottom);
  vBottom[2] = b->mins[2];

  vec3_t vSave;
  VectorCopy(vTriColor, vSave);

  qglBegin(GL_TRIANGLE_FAN);
  qglVertex3fv(vTop);
  for (int i = 0; i <= 3; i++)
  {
    vTriColor[0] *= 0.95;
    vTriColor[1] *= 0.95;
    vTriColor[2] *= 0.95;
    qglColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
    qglVertex3fv(vCorners[i]);
  }
  qglVertex3fv(vCorners[0]);
  qglEnd();
  
  VectorCopy(vSave, vTriColor);
  vTriColor[0] *= 0.95;
  vTriColor[1] *= 0.95;
  vTriColor[2] *= 0.95;

  qglBegin(GL_TRIANGLE_FAN);
  qglVertex3fv(vBottom);
  qglVertex3fv(vCorners[0]);
  for (i = 3; i >= 0; i--)
  {
    vTriColor[0] *= 0.95;
    vTriColor[1] *= 0.95;
    vTriColor[2] *= 0.95;
    qglColor3f(vTriColor[0], vTriColor[1], vTriColor[2]);
    qglVertex3fv(vCorners[i]);
  }
  qglEnd();

  // check for DOOM lights
  CString str = ValueForKey(b->owner, "light_right");
  if (str.GetLength() > 0) {
    vec3_t vRight, vUp, vTarget, vTemp;
    GetVectorForKey (b->owner, "light_right", vRight);
    GetVectorForKey (b->owner, "light_up", vUp);
    GetVectorForKey (b->owner, "light_target", vTarget);

    qglColor3f(0, 1, 0);
		qglBegin(GL_LINE_LOOP);
    VectorAdd(vTarget, b->owner->origin, vTemp);
    VectorAdd(vTemp, vRight, vTemp);
    VectorAdd(vTemp, vUp, vTemp);
    qglVertex3fv(b->owner->origin);
    qglVertex3fv(vTemp);
    VectorAdd(vTarget, b->owner->origin, vTemp);
    VectorAdd(vTemp, vUp, vTemp);
    VectorSubtract(vTemp, vRight, vTemp);
    qglVertex3fv(b->owner->origin);
    qglVertex3fv(vTemp);
    VectorAdd(vTarget, b->owner->origin, vTemp);
    VectorAdd(vTemp, vRight, vTemp);
    VectorSubtract(vTemp, vUp, vTemp);
    qglVertex3fv(b->owner->origin);
    qglVertex3fv(vTemp);
    VectorAdd(vTarget, b->owner->origin, vTemp);
    VectorSubtract(vTemp, vUp, vTemp);
    VectorSubtract(vTemp, vRight, vTemp);
    qglVertex3fv(b->owner->origin);
    qglVertex3fv(vTemp);
    qglEnd();

  }

}

void Brush_Draw( brush_t *b )
{
	face_t			*face;
	int				i, order;
	qtexture_t		*prev = 0;
	winding_t *w;

	if ( b->owner && ( b->owner->eclass->nShowFlags & ECLASS_PLUGINENTITY ) )
	{
		b->owner->pPlugEnt->CamRender();
		return;
	}
	
	// (TTimo) NOTE: added by build 173, I check after pPlugEnt so it doesn't interfere ?
	if (b->hiddenBrush)
	{
		return;
	}

	if (b->patchBrush)
	{
		//Patch_DrawCam(b->nPatchID);
		Patch_DrawCam(b->pPatch);
		//if (!g_bPatchShowBounds)
		return;
	}
	
	if (b->terrainBrush)
	{
		Terrain_DrawCam(b->pTerrain);
		return;
	}

	int nDrawMode = g_pParentWnd->GetCamera()->Camera().draw_mode;
	
	if (b->owner->eclass->fixedsize)
	{
		
		if (!(g_qeglobals.d_savedinfo.exclude & EXCLUDE_ANGLES) && (b->owner->eclass->nShowFlags & ECLASS_ANGLE))
		{
			Brush_DrawFacingAngle(b, b->owner);
		}
		
		if (g_PrefsDlg.m_bNewLightDraw && (b->owner->eclass->nShowFlags & ECLASS_LIGHT))
		{
			DrawLight(b);
			return;
		}
		if (nDrawMode == cd_texture || nDrawMode == cd_light)
			qglDisable (GL_TEXTURE_2D);
		
		// if we are wireframing models
		bool bp = (b->bModelFailed) ? false : PaintedModel(b, true);
		
		if (nDrawMode == cd_texture || nDrawMode == cd_light)
			qglEnable (GL_TEXTURE_2D);
		
		if (bp)
			return;
	}
	
	// guarantee the texture will be set first
	prev = NULL;
	for (face = b->brush_faces,order = 0 ; face ; face=face->next, order++)
	{
		w = face->face_winding;
		if (!w)
		{
			continue;		// freed face
		}
		
		if (g_qeglobals.d_savedinfo.exclude & EXCLUDE_CAULK)
		{
			if (strstr(face->texdef.name, "caulk"))
			{
				continue;
			}
		}
		
#if 0
		if (b->alphaBrush)
		{
			if (!(face->texdef.flags & SURF_ALPHA))
				continue;
			//--qglPushAttrib(GL_ALL_ATTRIB_BITS);
			qglDisable(GL_CULL_FACE);
			//--qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			//--qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			//--qglDisable(GL_DEPTH_TEST);
			//--qglBlendFunc (GL_SRC_ALPHA, GL_DST_ALPHA);
			//--qglEnable (GL_BLEND);
		}
#endif
		
		if ((nDrawMode == cd_texture || nDrawMode == cd_light) && face->d_texture != prev)
		{
			// set the texture for this face
			prev = face->d_texture;
			qglBindTexture( GL_TEXTURE_2D, face->d_texture->texture_number );
		}
		
		
		
		if (!b->patchBrush)
		{
			if (face->texdef.flags & SURF_TRANS33) 
				qglColor4f ( face->d_color[0], face->d_color[1], face->d_color[2], 0.33 );
			else if ( face->texdef.flags & SURF_TRANS66) 
				qglColor4f ( face->d_color[0], face->d_color[1], face->d_color[2], 0.66 );
			else
				qglColor3fv( face->d_color );
		}
		else
		{
			qglColor4f ( face->d_color[0], face->d_color[1], face->d_color[2], 0.13 );
		}
		
		// shader drawing stuff
		if (face->d_texture->bFromShader)
		{
			// setup shader drawing
			qglColor4f ( face->d_color[0], face->d_color[1], face->d_color[2], face->d_texture->fTrans );
			
		}
		
		// draw the polygon
		
		//if (nDrawMode == cd_light)
		//{
		if (g_PrefsDlg.m_bGLLighting)
		{
			qglNormal3fv(face->plane.normal);
		}
		//}
		
		qglBegin(GL_POLYGON);
		//if (nDrawMode == cd_light)
		
		for (i=0 ; i<w->numpoints ; i++)
		{
			if (nDrawMode == cd_texture || nDrawMode == cd_light)
				qglTexCoord2fv( &w->points[i][3] );
			qglVertex3fv(w->points[i]);
		}
		qglEnd();
	}
	
#if 0
	if (b->alphaBrush)
	{
		//--qglPopAttrib();
		qglEnable(GL_CULL_FACE);
		//--qglDisable (GL_BLEND);
		//--qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
#endif
	
	if (b->owner->eclass->fixedsize && (nDrawMode == cd_texture || nDrawMode == cd_light))
		qglEnable (GL_TEXTURE_2D);
	
	qglBindTexture( GL_TEXTURE_2D, 0 );
}



void Face_Draw( face_t *f )
{
	int i;

	if ( f->face_winding == 0 )
		return;
	qglBegin( GL_POLYGON );
	for ( i = 0 ; i < f->face_winding->numpoints; i++)
		qglVertex3fv( f->face_winding->points[i] );
	qglEnd();
}

void Brush_DrawXY(brush_t *b, int nViewType)
{
	face_t *face;
	int     order;
	winding_t *w;
	int        i;

	if (b->hiddenBrush)
	{
		return;
	}

	if (b->patchBrush)
	{
		//Patch_DrawXY(b->nPatchID);
		Patch_DrawXY(b->pPatch);
		if (!g_bPatchShowBounds)
			return;
	}

	if (b->terrainBrush)
	{
		Terrain_DrawXY(b->pTerrain, b->owner);
	}
                     

	if (b->owner->eclass->fixedsize)
	{
		if (g_PrefsDlg.m_bNewLightDraw && (b->owner->eclass->nShowFlags & ECLASS_LIGHT))
		{
			vec3_t vCorners[4];
			float fMid = b->mins[2] + (b->maxs[2] - b->mins[2]) / 2;

			vCorners[0][0] = b->mins[0];
			vCorners[0][1] = b->mins[1];
			vCorners[0][2] = fMid;

			vCorners[1][0] = b->mins[0];
			vCorners[1][1] = b->maxs[1];
			vCorners[1][2] = fMid;

			vCorners[2][0] = b->maxs[0];
			vCorners[2][1] = b->maxs[1];
			vCorners[2][2] = fMid;

			vCorners[3][0] = b->maxs[0];
			vCorners[3][1] = b->mins[1];
			vCorners[3][2] = fMid;

			vec3_t vTop, vBottom;

			vTop[0] = b->mins[0] + ((b->maxs[0] - b->mins[0]) / 2);
			vTop[1] = b->mins[1] + ((b->maxs[1] - b->mins[1]) / 2);
			vTop[2] = b->maxs[2];

			VectorCopy(vTop, vBottom);
			vBottom[2] = b->mins[2];

			qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
			qglBegin(GL_TRIANGLE_FAN);
			qglVertex3fv(vTop);
			qglVertex3fv(vCorners[0]);
			qglVertex3fv(vCorners[1]);
			qglVertex3fv(vCorners[2]);
			qglVertex3fv(vCorners[3]);
			qglVertex3fv(vCorners[0]);
			qglEnd();
			qglBegin(GL_TRIANGLE_FAN);
			qglVertex3fv(vBottom);
			qglVertex3fv(vCorners[0]);
			qglVertex3fv(vCorners[3]);
			qglVertex3fv(vCorners[2]);
			qglVertex3fv(vCorners[1]);
			qglVertex3fv(vCorners[0]);
			qglEnd();
			DrawBrushEntityName (b);
			return;
		}
		else if (b->owner->eclass->nShowFlags & ECLASS_MISCMODEL)
		{
			if (PaintedModel(b, false))
			return;
		}
	}

	for (face = b->brush_faces,order = 0 ; face ; face=face->next, order++)
	{
		// only draw polygons facing in a direction we care about
    if (nViewType == XY)
    {
		  if (face->plane.normal[2] <= 0)
			  continue;
    }
    else
    {
      if (nViewType == XZ)
      {
        if (face->plane.normal[1] <= 0)
          continue;
      }
      else 
      {
        if (face->plane.normal[0] <= 0)
          continue;
      }
    }

		w = face->face_winding;
		if (!w)
			continue;

    //if (b->alphaBrush && !(face->texdef.flags & SURF_ALPHA))
    //  continue;

		// draw the polygon
		qglBegin(GL_LINE_LOOP);
    for (i=0 ; i<w->numpoints ; i++)
		  qglVertex3fv(w->points[i]);
		qglEnd();
	}

	DrawBrushEntityName (b);

}

/*
============
Brush_Move
============
*/
void Brush_Move (brush_t *b, const vec3_t move, bool bSnap)
{
  int i;
  face_t *f;

  for (f=b->brush_faces ; f ; f=f->next)
  {
    vec3_t vTemp;
    VectorCopy(move, vTemp);

    if (g_PrefsDlg.m_bTextureLock)
      Face_MoveTexture(f, vTemp);
    
    for (i=0 ; i<3 ; i++)
      VectorAdd (f->planepts[i], move, f->planepts[i]);
  }
  Brush_Build( b, bSnap );


  if (b->patchBrush)
  {
    //Patch_Move(b->nPatchID, move);
    Patch_Move(b->pPatch, move);
  }

  if (b->terrainBrush)
  {
    Terrain_Move(b->pTerrain, move);
  }


  // PGM - keep the origin vector up to date on fixed size entities.
  if(b->owner->eclass->fixedsize)
  {
    VectorAdd(b->owner->origin, move, b->owner->origin);
	  //VectorAdd(b->maxs, b->mins, b->owner->origin);
	  //VectorScale(b->owner->origin, 0.5, b->owner->origin);
  }
}



void Brush_Print(brush_t* b)
{
  int nFace = 0;
  for (face_t* f = b->brush_faces ; f ; f=f->next)
  {
    Sys_Printf("Face %i\n", nFace++);
    Sys_Printf("%f %f %f\n", f->planepts[0][0], f->planepts[0][1], f->planepts[0][2]);
    Sys_Printf("%f %f %f\n", f->planepts[1][0], f->planepts[1][1], f->planepts[1][2]);
    Sys_Printf("%f %f %f\n", f->planepts[2][0], f->planepts[2][1], f->planepts[2][2]);
  }
 }



/*
=============
Brush_MakeSided

Makes the current brushhave the given number of 2d sides and turns it into a cone
=============
*/
void Brush_MakeSidedCone(int sides)
{
	int		i;
	vec3_t	mins, maxs;
	brush_t	*b;
	texdef_t	*texdef;
	face_t	*f;
	vec3_t	mid;
	float	width;
	float	sv, cv;

	if (sides < 3)
	{
		Sys_Status ("Bad sides number", 0);
		return;
	}

	if (!QE_SingleBrush ())
	{
		Sys_Status ("Must have a single brush selected", 0 );
		return;
	}

	b = selected_brushes.next;
	VectorCopy (b->mins, mins);
	VectorCopy (b->maxs, maxs);
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free (b);

	// find center of brush
	width = 8;
	for (i=0 ; i<2 ; i++)
	{
		mid[i] = (maxs[i] + mins[i])*0.5;
		if (maxs[i] - mins[i] > width)
			width = maxs[i] - mins[i];
	}
	width /= 2;

	b = Brush_Alloc();

	// create bottom face
	f = Face_Alloc();
	f->texdef = *texdef;
	f->next = b->brush_faces;
	b->brush_faces = f;

	f->planepts[0][0] = mins[0];f->planepts[0][1] = mins[1];f->planepts[0][2] = mins[2];
	f->planepts[1][0] = maxs[0];f->planepts[1][1] = mins[1];f->planepts[1][2] = mins[2];
	f->planepts[2][0] = maxs[0];f->planepts[2][1] = maxs[1];f->planepts[2][2] = mins[2];

	for (i=0 ; i<sides ; i++)
	{
		f = Face_Alloc();
		f->texdef = *texdef;
		f->next = b->brush_faces;
		b->brush_faces = f;

		sv = sin (i*3.14159265*2/sides);
		cv = cos (i*3.14159265*2/sides);


		f->planepts[0][0] = floor(mid[0]+width*cv+0.5);
		f->planepts[0][1] = floor(mid[1]+width*sv+0.5);
		f->planepts[0][2] = mins[2];

		f->planepts[1][0] = mid[0];
		f->planepts[1][1] = mid[1];
		f->planepts[1][2] = maxs[2];

		f->planepts[2][0] = floor(f->planepts[0][0] - width * sv + 0.5);
		f->planepts[2][1] = floor(f->planepts[0][1] + width * cv + 0.5);
		f->planepts[2][2] = maxs[2];

	}

	Brush_AddToList (b, &selected_brushes);

	Entity_LinkBrush (world_entity, b);

	Brush_Build( b );

	Sys_UpdateWindows (W_ALL);
}

/*
=============
Brush_MakeSided

Makes the current brushhave the given number of 2d sides and turns it into a sphere
=============

*/
void Brush_MakeSidedSphere(int sides)
{
	int		i,j;
	vec3_t	mins, maxs;
	brush_t	*b;
	texdef_t	*texdef;
	face_t	*f;
	vec3_t	mid;

	if (sides < 4)
	{
		Sys_Status ("Bad sides number", 0);
		return;
	}

	if (!QE_SingleBrush ())
	{
		Sys_Status ("Must have a single brush selected", 0 );
		return;
	}

	b = selected_brushes.next;
	VectorCopy (b->mins, mins);
	VectorCopy (b->maxs, maxs);
	texdef = &g_qeglobals.d_texturewin.texdef;

	Brush_Free (b);

	// find center of brush
	float radius = 8;
	for (i=0 ; i<2 ; i++)
	{
		mid[i] = (maxs[i] + mins[i])*0.5;
		if (maxs[i] - mins[i] > radius)
			radius = maxs[i] - mins[i];
	}
	radius /= 2;

	b = Brush_Alloc();

	float dt = float(2 * Q_PI / sides);
	float dp = float(Q_PI / sides);
  float t,p;
	for(i=0; i <= sides-1; i++)
  {
		for(j=0;j <= sides-2; j++)
		{
			t = i * dt;
			p = float(j * dp - Q_PI / 2);

      f = Face_Alloc();
	    f->texdef = *texdef;
	    f->next = b->brush_faces;
	    b->brush_faces = f;

      VectorPolar(f->planepts[0], radius, t, p);
      VectorPolar(f->planepts[1], radius, t, p + dp);
      VectorPolar(f->planepts[2], radius, t + dt, p + dp);

      for (int k = 0; k < 3; k++)
        VectorAdd(f->planepts[k], mid, f->planepts[k]);
		}
  }

  p = float((sides - 1) * dp - Q_PI / 2);
	for(i = 0; i <= sides-1; i++)
	{
		t = i * dt;

    f = Face_Alloc();
	  f->texdef = *texdef;
	  f->next = b->brush_faces;
	  b->brush_faces = f;

    VectorPolar(f->planepts[0], radius, t, p);
    VectorPolar(f->planepts[1], radius, t + dt, p + dp);
    VectorPolar(f->planepts[2], radius, t + dt, p);

    for (int k = 0; k < 3; k++)
      VectorAdd(f->planepts[k], mid, f->planepts[k]);
	}

	Brush_AddToList (b, &selected_brushes);

	Entity_LinkBrush (world_entity, b);

	Brush_Build( b );

	Sys_UpdateWindows (W_ALL);
}

void Face_FitTexture( face_t * face, int nHeight, int nWidth )
{
  winding_t *w;
  vec3_t   mins,maxs;
  int i;
  float width, height, temp;
  float rot_width, rot_height;
  float cosv,sinv,ang;
  float min_t, min_s, max_t, max_s;
  float s,t;
	vec3_t	vecs[2];
  vec3_t   coords[4];
	texdef_t	*td;

  if (nHeight < 1)
  {
    nHeight = 1;
  }
  if (nWidth < 1)
  {
    nWidth = 1;
  }

  ClearBounds (mins, maxs);

	td = &face->texdef;
	w = face->face_winding;
	if (!w)
	{
    return;
	}
  for (i=0 ; i<w->numpoints ; i++)
  {
    AddPointToBounds( w->points[i], mins, maxs );
  }
   // 
   // get the current angle
   //
	ang = td->rotate / 180 * Q_PI;
	sinv = sin(ang);
	cosv = cos(ang);

	// get natural texture axis
	TextureAxisFromPlane(&face->plane, vecs[0], vecs[1]);

  min_s = DotProduct( mins, vecs[0] );
  min_t = DotProduct( mins, vecs[1] );
  max_s = DotProduct( maxs, vecs[0] );
  max_t = DotProduct( maxs, vecs[1] );
  width = max_s - min_s;
  height = max_t - min_t;
  coords[0][0] = min_s;
  coords[0][1] = min_t;
  coords[1][0] = max_s;
  coords[1][1] = min_t;
  coords[2][0] = min_s;
  coords[2][1] = max_t;
  coords[3][0] = max_s;
  coords[3][1] = max_t;
  min_s = min_t = 99999;
  max_s = max_t = -99999;
  for (i=0; i<4; i++)
  {
    s = cosv * coords[i][0] - sinv * coords[i][1];
	  t = sinv * coords[i][0] + cosv * coords[i][1];
    if (i&1)
    {
      if (s > max_s) 
      {
        max_s = s;
      }
    }
    else
    {
      if (s < min_s) 
      {
        min_s = s;
      }
      if (i<2)
      {
        if (t < min_t) 
        {
          min_t = t;
        }
      }
      else
      {
        if (t > max_t) 
        {
          max_t = t;
        }
      }
    }
  }
  rot_width =  (max_s - min_s);
  rot_height = (max_t - min_t);
  td->scale[0] = -(rot_width/((float)(face->d_texture->width*nWidth)));
  td->scale[1] = -(rot_height/((float)(face->d_texture->height*nHeight)));

  td->shift[0] = min_s/td->scale[0];
  temp = (int)(td->shift[0] / (face->d_texture->width*nWidth));
  temp = (temp+1)*face->d_texture->width*nWidth;
  td->shift[0] = (int)(temp - td->shift[0])%(face->d_texture->width*nWidth);

  td->shift[1] = min_t/td->scale[1];
  temp = (int)(td->shift[1] / (face->d_texture->height*nHeight));
  temp = (temp+1)*(face->d_texture->height*nHeight);
  td->shift[1] = (int)(temp - td->shift[1])%(face->d_texture->height*nHeight);
}

void Brush_FitTexture( brush_t *b, int nHeight, int nWidth )
{
	face_t *face;

	for (face = b->brush_faces ; face ; face=face->next)
  {
    Face_FitTexture( face, nHeight, nWidth );
  }
}


