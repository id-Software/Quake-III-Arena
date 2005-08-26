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
// faces.c

#include "qbsp.h"
#include "l_mem.h"

/*

  some faces will be removed before saving, but still form nodes:

  the insides of sky volumes
  meeting planes of different water current volumes

*/

// undefine for dumb linear searches
#define	USE_HASHING

#define	INTEGRAL_EPSILON	0.01
#define	POINT_EPSILON		0.5
#define	OFF_EPSILON			0.5

int	c_merge;
int	c_subdivide;

int	c_totalverts;
int	c_uniqueverts;
int	c_degenerate;
int	c_tjunctions;
int	c_faceoverflows;
int	c_facecollapse;
int	c_badstartverts;

#define	MAX_SUPERVERTS	512
int	superverts[MAX_SUPERVERTS];
int	numsuperverts;

face_t		*edgefaces[MAX_MAP_EDGES][2];
int		firstmodeledge = 1;
int		firstmodelface;

int	c_tryedges;

vec3_t	edge_dir;
vec3_t	edge_start;
vec_t	edge_len;

int		num_edge_verts;
int		edge_verts[MAX_MAP_VERTS];

face_t *NewFaceFromFace (face_t *f);

//===========================================================================

typedef struct hashvert_s
{
	struct hashvert_s	*next;
	int		num;
} hashvert_t;


#define	HASH_SIZE	64


int	vertexchain[MAX_MAP_VERTS];		// the next vertex in a hash chain
int	hashverts[HASH_SIZE*HASH_SIZE];	// a vertex number, or 0 for no verts

face_t		*edgefaces[MAX_MAP_EDGES][2];

//============================================================================


unsigned HashVec (vec3_t vec)
{
	int			x, y;

	x = (4096 + (int)(vec[0]+0.5)) >> 7;
	y = (4096 + (int)(vec[1]+0.5)) >> 7;

	if ( x < 0 || x >= HASH_SIZE || y < 0 || y >= HASH_SIZE )
		Error ("HashVec: point outside valid range");
	
	return y*HASH_SIZE + x;
}

#ifdef USE_HASHING
/*
=============
GetVertex

Uses hashing
=============
*/
int	GetVertexnum (vec3_t in)
{
	int			h;
	int			i;
	float		*p;
	vec3_t		vert;
	int			vnum;

	c_totalverts++;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(in[i] - Q_rint(in[i])) < INTEGRAL_EPSILON)
			vert[i] = Q_rint(in[i]);
		else
			vert[i] = in[i];
	}
	
	h = HashVec (vert);
	
	for (vnum=hashverts[h] ; vnum ; vnum=vertexchain[vnum])
	{
		p = dvertexes[vnum].point;
		if ( fabs(p[0]-vert[0])<POINT_EPSILON
		&& fabs(p[1]-vert[1])<POINT_EPSILON
		&& fabs(p[2]-vert[2])<POINT_EPSILON )
			return vnum;
	}
	
// emit a vertex
	if (numvertexes == MAX_MAP_VERTS)
		Error ("numvertexes == MAX_MAP_VERTS");

	dvertexes[numvertexes].point[0] = vert[0];
	dvertexes[numvertexes].point[1] = vert[1];
	dvertexes[numvertexes].point[2] = vert[2];

	vertexchain[numvertexes] = hashverts[h];
	hashverts[h] = numvertexes;

	c_uniqueverts++;

	numvertexes++;
		
	return numvertexes-1;
}
#else
/*
==================
GetVertexnum

Dumb linear search
==================
*/
int	GetVertexnum (vec3_t v)
{
	int			i, j;
	dvertex_t	*dv;
	vec_t		d;

	c_totalverts++;

	// make really close values exactly integral
	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(v[i] - (int)(v[i]+0.5)) < INTEGRAL_EPSILON )
			v[i] = (int)(v[i]+0.5);
		if (v[i] < -4096 || v[i] > 4096)
			Error ("GetVertexnum: outside +/- 4096");
	}

	// search for an existing vertex match
	for (i=0, dv=dvertexes ; i<numvertexes ; i++, dv++)
	{
		for (j=0 ; j<3 ; j++)
		{
			d = v[j] - dv->point[j];
			if ( d > POINT_EPSILON || d < -POINT_EPSILON)
				break;
		}
		if (j == 3)
			return i;		// a match
	}

	// new point
	if (numvertexes == MAX_MAP_VERTS)
		Error ("MAX_MAP_VERTS");
	VectorCopy (v, dv->point);
	numvertexes++;
	c_uniqueverts++;

	return numvertexes-1;
}
#endif


/*
==================
FaceFromSuperverts

The faces vertexes have been added to the superverts[] array,
and there may be more there than can be held in a face (MAXEDGES).

If less, the faces vertexnums[] will be filled in, otherwise
face will reference a tree of split[] faces until all of the
vertexnums can be added.

superverts[base] will become face->vertexnums[0], and the others
will be circularly filled in.
==================
*/
void FaceFromSuperverts (node_t *node, face_t *f, int base)
{
	face_t	*newf;
	int		remaining;
	int		i;

	remaining = numsuperverts;
	while (remaining > MAXEDGES)
	{	// must split into two faces, because of vertex overload
		c_faceoverflows++;

		newf = f->split[0] = NewFaceFromFace (f);
		newf = f->split[0];
		newf->next = node->faces;
		node->faces = newf;

		newf->numpoints = MAXEDGES;
		for (i=0 ; i<MAXEDGES ; i++)
			newf->vertexnums[i] = superverts[(i+base)%numsuperverts];

		f->split[1] = NewFaceFromFace (f);
		f = f->split[1];
		f->next = node->faces;
		node->faces = f;

		remaining -= (MAXEDGES-2);
		base = (base+MAXEDGES-1)%numsuperverts;
	}

	// copy the vertexes back to the face
	f->numpoints = remaining;
	for (i=0 ; i<remaining ; i++)
		f->vertexnums[i] = superverts[(i+base)%numsuperverts];
}


/*
==================
EmitFaceVertexes
==================
*/
void EmitFaceVertexes (node_t *node, face_t *f)
{
	winding_t	*w;
	int			i;

	if (f->merged || f->split[0] || f->split[1])
		return;

	w = f->w;
	for (i=0 ; i<w->numpoints ; i++)
	{
		if (noweld)
		{	// make every point unique
			if (numvertexes == MAX_MAP_VERTS)
				Error ("MAX_MAP_VERTS");
			superverts[i] = numvertexes;
			VectorCopy (w->p[i], dvertexes[numvertexes].point);
			numvertexes++;
			c_uniqueverts++;
			c_totalverts++;
		}
		else
			superverts[i] = GetVertexnum (w->p[i]);
	}
	numsuperverts = w->numpoints;

	// this may fragment the face if > MAXEDGES
	FaceFromSuperverts (node, f, 0);
}

/*
==================
EmitVertexes_r
==================
*/
void EmitVertexes_r (node_t *node)
{
	int		i;
	face_t	*f;

	if (node->planenum == PLANENUM_LEAF)
		return;

	for (f=node->faces ; f ; f=f->next)
	{
		EmitFaceVertexes (node, f);
	}

	for (i=0 ; i<2 ; i++)
		EmitVertexes_r (node->children[i]);
}


#ifdef USE_HASHING
/*
==========
FindEdgeVerts

Uses the hash tables to cut down to a small number
==========
*/
void FindEdgeVerts (vec3_t v1, vec3_t v2)
{
	int		x1, x2, y1, y2, t;
	int		x, y;
	int		vnum;

#if 0
{
	int		i;
	num_edge_verts = numvertexes-1;
	for (i=0 ; i<numvertexes-1 ; i++)
		edge_verts[i] = i+1;
}
#endif

	x1 = (4096 + (int)(v1[0]+0.5)) >> 7;
	y1 = (4096 + (int)(v1[1]+0.5)) >> 7;
	x2 = (4096 + (int)(v2[0]+0.5)) >> 7;
	y2 = (4096 + (int)(v2[1]+0.5)) >> 7;

	if (x1 > x2)
	{
		t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y1 > y2)
	{
		t = y1;
		y1 = y2;
		y2 = t;
	}
#if 0
	x1--;
	x2++;
	y1--;
	y2++;
	if (x1 < 0)
		x1 = 0;
	if (x2 >= HASH_SIZE)
		x2 = HASH_SIZE;
	if (y1 < 0)
		y1 = 0;
	if (y2 >= HASH_SIZE)
		y2 = HASH_SIZE;
#endif
	num_edge_verts = 0;
	for (x=x1 ; x <= x2 ; x++)
	{
		for (y=y1 ; y <= y2 ; y++)
		{
			for (vnum=hashverts[y*HASH_SIZE+x] ; vnum ; vnum=vertexchain[vnum])
			{
				edge_verts[num_edge_verts++] = vnum;
			}
		}
	}
}

#else
/*
==========
FindEdgeVerts

Forced a dumb check of everything
==========
*/
void FindEdgeVerts (vec3_t v1, vec3_t v2)
{
	int		i;

	num_edge_verts = numvertexes-1;
	for (i=0 ; i<num_edge_verts ; i++)
		edge_verts[i] = i+1;
}
#endif

/*
==========
TestEdge

Can be recursively reentered
==========
*/
void TestEdge (vec_t start, vec_t end, int p1, int p2, int startvert)
{
	int		j, k;
	vec_t	dist;
	vec3_t	delta;
	vec3_t	exact;
	vec3_t	off;
	vec_t	error;
	vec3_t	p;

	if (p1 == p2)
	{
		c_degenerate++;
		return;		// degenerate edge
	}

	for (k=startvert ; k<num_edge_verts ; k++)
	{
		j = edge_verts[k];
		if (j==p1 || j == p2)
			continue;

		VectorCopy (dvertexes[j].point, p);

		VectorSubtract (p, edge_start, delta);
		dist = DotProduct (delta, edge_dir);
		if (dist <=start || dist >= end)
			continue;		// off an end
		VectorMA (edge_start, dist, edge_dir, exact);
		VectorSubtract (p, exact, off);
		error = VectorLength (off);

		if (fabs(error) > OFF_EPSILON)
			continue;		// not on the edge

		// break the edge
		c_tjunctions++;
		TestEdge (start, dist, p1, j, k+1);
		TestEdge (dist, end, j, p2, k+1);
		return;
	}

	// the edge p1 to p2 is now free of tjunctions
	if (numsuperverts >= MAX_SUPERVERTS)
		Error ("MAX_SUPERVERTS");
	superverts[numsuperverts] = p1;
	numsuperverts++;
}

/*
==================
FixFaceEdges

==================
*/
void FixFaceEdges (node_t *node, face_t *f)
{
	int		p1, p2;
	int		i;
	vec3_t	e2;
	vec_t	len;
	int		count[MAX_SUPERVERTS], start[MAX_SUPERVERTS];
	int		base;

	if (f->merged || f->split[0] || f->split[1])
		return;

	numsuperverts = 0;

	for (i=0 ; i<f->numpoints ; i++)
	{
		p1 = f->vertexnums[i];
		p2 = f->vertexnums[(i+1)%f->numpoints];

		VectorCopy (dvertexes[p1].point, edge_start);
		VectorCopy (dvertexes[p2].point, e2);

		FindEdgeVerts (edge_start, e2);

		VectorSubtract (e2, edge_start, edge_dir);
		len = VectorNormalize(edge_dir);

		start[i] = numsuperverts;
		TestEdge (0, len, p1, p2, 0);

		count[i] = numsuperverts - start[i];
	}

	if (numsuperverts < 3)
	{	// entire face collapsed
		f->numpoints = 0;
		c_facecollapse++;
		return;
	}

	// we want to pick a vertex that doesn't have tjunctions
	// on either side, which can cause artifacts on trifans,
	// especially underwater
	for (i=0 ; i<f->numpoints ; i++)
	{
		if (count[i] == 1 && count[(i+f->numpoints-1)%f->numpoints] == 1)
			break;
	}
	if (i == f->numpoints)
	{
		f->badstartvert = true;
		c_badstartverts++;
		base = 0;
	}
	else
	{	// rotate the vertex order
		base = start[i];
	}

	// this may fragment the face if > MAXEDGES
	FaceFromSuperverts (node, f, base);
}

/*
==================
FixEdges_r
==================
*/
void FixEdges_r (node_t *node)
{
	int		i;
	face_t	*f;

	if (node->planenum == PLANENUM_LEAF)
		return;

	for (f=node->faces ; f ; f=f->next)
		FixFaceEdges (node, f);

	for (i=0 ; i<2 ; i++)
		FixEdges_r (node->children[i]);
}

/*
===========
FixTjuncs

===========
*/
void FixTjuncs (node_t *headnode)
{
	// snap and merge all vertexes
	qprintf ("---- snap verts ----\n");
	memset (hashverts, 0, sizeof(hashverts));
	c_totalverts = 0;
	c_uniqueverts = 0;
	c_faceoverflows = 0;
	EmitVertexes_r (headnode);
	qprintf ("%i unique from %i\n", c_uniqueverts, c_totalverts);

	// break edges on tjunctions
	qprintf ("---- tjunc ----\n");
	c_tryedges = 0;
	c_degenerate = 0;
	c_facecollapse = 0;
	c_tjunctions = 0;
	if (!notjunc)
		FixEdges_r (headnode);
	qprintf ("%5i edges degenerated\n", c_degenerate);
	qprintf ("%5i faces degenerated\n", c_facecollapse);
	qprintf ("%5i edges added by tjunctions\n", c_tjunctions);
	qprintf ("%5i faces added by tjunctions\n", c_faceoverflows);
	qprintf ("%5i bad start verts\n", c_badstartverts);
}


//========================================================

int		c_faces;

face_t	*AllocFace (void)
{
	face_t	*f;

	f = GetMemory(sizeof(*f));
	memset (f, 0, sizeof(*f));
	c_faces++;

	return f;
}

face_t *NewFaceFromFace (face_t *f)
{
	face_t	*newf;

	newf = AllocFace ();
	*newf = *f;
	newf->merged = NULL;
	newf->split[0] = newf->split[1] = NULL;
	newf->w = NULL;
	return newf;
}

void FreeFace (face_t *f)
{
	if (f->w)
		FreeWinding (f->w);
	FreeMemory(f);
	c_faces--;
}

//========================================================

/*
==================
GetEdge

Called by writebsp.
Don't allow four way edges
==================
*/
int GetEdge2 (int v1, int v2,  face_t *f)
{
	dedge_t	*edge;
	int		i;

	c_tryedges++;

	if (!noshare)
	{
		for (i=firstmodeledge ; i < numedges ; i++)
		{
			edge = &dedges[i];
			if (v1 == edge->v[1] && v2 == edge->v[0]
			&& edgefaces[i][0]->contents == f->contents)
			{
				if (edgefaces[i][1])
	//				printf ("WARNING: multiple backward edge\n");
					continue;
				edgefaces[i][1] = f;
				return -i;
			}
	#if 0
			if (v1 == edge->v[0] && v2 == edge->v[1])
			{
				printf ("WARNING: multiple forward edge\n");
				return i;
			}
	#endif
		}
	}

// emit an edge
	if (numedges >= MAX_MAP_EDGES)
		Error ("numedges == MAX_MAP_EDGES");
	edge = &dedges[numedges];
	numedges++;
	edge->v[0] = v1;
	edge->v[1] = v2;
	edgefaces[numedges-1][0] = f;
	
	return numedges-1;
}

/*
===========================================================================

FACE MERGING

===========================================================================
*/

/*
=============
TryMerge

If two polygons share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the faces couldn't be merged, or the new face.
The originals will NOT be freed.
=============
*/
face_t *TryMerge (face_t *f1, face_t *f2, vec3_t planenormal)
{
	face_t		*newf;
	winding_t	*nw;

	if (!f1->w || !f2->w)
		return NULL;
	if (f1->texinfo != f2->texinfo)
		return NULL;
	if (f1->planenum != f2->planenum)	// on front and back sides
		return NULL;
	if (f1->contents != f2->contents)
		return NULL;
		

	nw = TryMergeWinding (f1->w, f2->w, planenormal);
	if (!nw)
		return NULL;

	c_merge++;
	newf = NewFaceFromFace (f1);
	newf->w = nw;

	f1->merged = newf;
	f2->merged = newf;

	return newf;
}

/*
===============
MergeNodeFaces
===============
*/
void MergeNodeFaces (node_t *node)
{
	face_t	*f1, *f2, *end;
	face_t	*merged;
	plane_t	*plane;

	plane = &mapplanes[node->planenum];
	merged = NULL;
	
	for (f1 = node->faces ; f1 ; f1 = f1->next)
	{
		if (f1->merged || f1->split[0] || f1->split[1])
			continue;

		for (f2 = node->faces ; f2 != f1 ; f2=f2->next)
		{
			if (f2->merged || f2->split[0] || f2->split[1])
				continue;

			//IDBUG: always passes the face's node's normal to TryMerge()
			//regardless of which side the face is on. Approximately 50% of
			//the time the face will be on the other side of node, and thus
			//the result of the convex/concave test in TryMergeWinding(),
			//which depends on the normal, is flipped. This causes faces
			//that shouldn't be merged to be merged and faces that
			//should be merged to not be merged. 
			//the following added line fixes this bug
			//thanks to: Alexander Malmberg <alexander@malmberg.org>
			plane = &mapplanes[f1->planenum];
			//
			merged = TryMerge (f1, f2, plane->normal);
			if (!merged)
				continue;

			// add merged to the end of the node face list 
			// so it will be checked against all the faces again
			for (end = node->faces ; end->next ; end = end->next)
			;
			merged->next = NULL;
			end->next = merged;
			break;
		}
	}
}

//=====================================================================

/*
===============
SubdivideFace

Chop up faces that are larger than we want in the surface cache
===============
*/
void SubdivideFace (node_t *node, face_t *f)
{
	float		mins, maxs;
	vec_t		v;
	int			axis, i;
	texinfo_t	*tex;
	vec3_t		temp;
	vec_t		dist;
	winding_t	*w, *frontw, *backw;

	if (f->merged)
		return;

// special (non-surface cached) faces don't need subdivision
	tex = &texinfo[f->texinfo];

	if ( tex->flags & (SURF_WARP|SURF_SKY) )
	{
		return;
	}

	for (axis = 0 ; axis < 2 ; axis++)
	{
		while (1)
		{
			mins = 999999;
			maxs = -999999;
			
			VectorCopy (tex->vecs[axis], temp);
			w = f->w;
			for (i=0 ; i<w->numpoints ; i++)
			{
				v = DotProduct (w->p[i], temp);
				if (v < mins)
					mins = v;
				if (v > maxs)
					maxs = v;
			}
#if 0
			if (maxs - mins <= 0)
				Error ("zero extents");
#endif
			if (axis == 2)
			{	// allow double high walls
				if (maxs - mins <= subdivide_size/* *2 */)
					break;
			}
			else if (maxs - mins <= subdivide_size)
				break;
			
		// split it
			c_subdivide++;
			
			v = VectorNormalize (temp);

			dist = (mins + subdivide_size - 16)/v;

			ClipWindingEpsilon (w, temp, dist, ON_EPSILON, &frontw, &backw);
			if (!frontw || !backw)
				Error ("SubdivideFace: didn't split the polygon");

			f->split[0] = NewFaceFromFace (f);
			f->split[0]->w = frontw;
			f->split[0]->next = node->faces;
			node->faces = f->split[0];

			f->split[1] = NewFaceFromFace (f);
			f->split[1]->w = backw;
			f->split[1]->next = node->faces;
			node->faces = f->split[1];

			SubdivideFace (node, f->split[0]);
			SubdivideFace (node, f->split[1]);
			return;
		}
	}
}

void SubdivideNodeFaces (node_t *node)
{
	face_t	*f;

	for (f = node->faces ; f ; f=f->next)
	{
		SubdivideFace (node, f);
	}
}

//===========================================================================

int	c_nodefaces;


/*
============
FaceFromPortal

============
*/
face_t *FaceFromPortal (portal_t *p, int pside)
{
	face_t	*f;
	side_t	*side;

	side = p->side;
	if (!side)
		return NULL;	// portal does not bridge different visible contents

	f = AllocFace ();

	f->texinfo = side->texinfo;
	f->planenum = (side->planenum & ~1) | pside;
	f->portal = p;

	if ( (p->nodes[pside]->contents & CONTENTS_WINDOW)
		&& VisibleContents(p->nodes[!pside]->contents^p->nodes[pside]->contents) == CONTENTS_WINDOW )
		return NULL;	// don't show insides of windows

	if (pside)
	{
		f->w = ReverseWinding(p->winding);
		f->contents = p->nodes[1]->contents;
	}
	else
	{
		f->w = CopyWinding(p->winding);
		f->contents = p->nodes[0]->contents;
	}
	return f;
}


/*
===============
MakeFaces_r

If a portal will make a visible face,
mark the side that originally created it

  solid / empty : solid
  solid / water : solid
  water / empty : water
  water / water : none
===============
*/
void MakeFaces_r (node_t *node)
{
	portal_t	*p;
	int			s;

	// recurse down to leafs
	if (node->planenum != PLANENUM_LEAF)
	{
		MakeFaces_r (node->children[0]);
		MakeFaces_r (node->children[1]);

		// merge together all visible faces on the node
		if (!nomerge)
			MergeNodeFaces (node);
		if (!nosubdiv)
			SubdivideNodeFaces (node);

		return;
	}

	// solid leafs never have visible faces
	if (node->contents & CONTENTS_SOLID)
		return;

	// see which portals are valid
	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);

		p->face[s] = FaceFromPortal (p, s);
		if (p->face[s])
		{
			c_nodefaces++;
			p->face[s]->next = p->onnode->faces;
			p->onnode->faces = p->face[s];
		}
	}
}

/*
============
MakeFaces
============
*/
void MakeFaces (node_t *node)
{
	qprintf ("--- MakeFaces ---\n");
	c_merge = 0;
	c_subdivide = 0;
	c_nodefaces = 0;

	MakeFaces_r (node);

	qprintf ("%5i makefaces\n", c_nodefaces);
	qprintf ("%5i merged\n", c_merge);
	qprintf ("%5i subdivided\n", c_subdivide);
}
