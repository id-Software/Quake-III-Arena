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
#include "l_bsp_q1.h"
#include "aas_map.h"			//AAS_CreateMapBrushes

int q1_numbrushes;
int q1_numclipbrushes;

//#define Q1_PRINT

//===========================================================================
// water, slime and lava brush textures names always start with a *
// followed by the type: "slime", "lava" or otherwise water
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Q1_TextureContents(char *name)
{
	if (!Q_strcasecmp(name, "clip")) return CONTENTS_SOLID;
	if (name[0] == '*')
	{
		if (!Q_strncasecmp(name+1,"lava",4)) return CONTENTS_LAVA;
		else if (!Q_strncasecmp(name+1,"slime",5)) return CONTENTS_SLIME;
		else return CONTENTS_WATER;
	} //end if
	else if (!Q_strncasecmp(name, "sky", 3)) return CONTENTS_SOLID;
	else return CONTENTS_SOLID;
} //end of the function Q1_TextureContents
//===========================================================================
// Generates two new brushes, leaving the original
// unchanged
//
// modified for Half-Life because there are quite a lot of tiny node leaves
// in the Half-Life bsps
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_SplitBrush(bspbrush_t *brush, int planenum, int nodenum,
						 bspbrush_t **front, bspbrush_t **back)
{
	bspbrush_t *b[2];
	int i, j;
	winding_t *w, *cw[2], *midwinding;
	plane_t *plane, *plane2;
	side_t *s, *cs;
	float d, d_front, d_back;

	*front = *back = NULL;
	plane = &mapplanes[planenum];

	// check all points
	d_front = d_back = 0;
	for (i=0 ; i<brush->numsides ; i++)
	{
		w = brush->sides[i].winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
		{
			d = DotProduct (w->p[j], plane->normal) - plane->dist;
			if (d > 0 && d > d_front)
				d_front = d;
			if (d < 0 && d < d_back)
				d_back = d;
		} //end for
	} //end for

	if (d_front < 0.1) // PLANESIDE_EPSILON)
	{	// only on back
		*back = CopyBrush (brush);
		Log_Print("Q1_SplitBrush: only on back\n");
		return;
	} //end if
	if (d_back > -0.1) // PLANESIDE_EPSILON)
	{	// only on front
		*front = CopyBrush (brush);
		Log_Print("Q1_SplitBrush: only on front\n");
		return;
	} //end if

	// create a new winding from the split plane

	w = BaseWindingForPlane (plane->normal, plane->dist);
	for (i = 0; i < brush->numsides && w; i++)
	{
		plane2 = &mapplanes[brush->sides[i].planenum ^ 1];
		ChopWindingInPlace(&w, plane2->normal, plane2->dist, 0); // PLANESIDE_EPSILON);
	} //end for

	if (!w || WindingIsTiny(w))
	{	// the brush isn't really split
		int		side;

		Log_Print("Q1_SplitBrush: no split winding\n");
		side = BrushMostlyOnSide (brush, plane);
		if (side == PSIDE_FRONT)
			*front = CopyBrush (brush);
		if (side == PSIDE_BACK)
			*back = CopyBrush (brush);
		return;
	}

	if (WindingIsHuge(w))
	{
		Log_Print("Q1_SplitBrush: WARNING huge split winding\n");
	} //end of

	midwinding = w;

	// split it for real

	for (i = 0; i < 2; i++)
	{
		b[i] = AllocBrush (brush->numsides+1);
		b[i]->original = brush->original;
	} //end for

	// split all the current windings

	for (i=0 ; i<brush->numsides ; i++)
	{
		s = &brush->sides[i];
		w = s->winding;
		if (!w)
			continue;
		ClipWindingEpsilon (w, plane->normal, plane->dist,
			0 /*PLANESIDE_EPSILON*/, &cw[0], &cw[1]);
		for (j=0 ; j<2 ; j++)
		{
			if (!cw[j])
				continue;
#if 0
			if (WindingIsTiny (cw[j]))
			{
				FreeWinding (cw[j]);
				continue;
			}
#endif
			cs = &b[j]->sides[b[j]->numsides];
			b[j]->numsides++;
			*cs = *s;
//			cs->planenum = s->planenum;
//			cs->texinfo = s->texinfo;
//			cs->visible = s->visible;
//			cs->original = s->original;
			cs->winding = cw[j];
			cs->flags &= ~SFL_TESTED;
		} //end for
	} //end for


	// see if we have valid polygons on both sides

	for (i=0 ; i<2 ; i++)
	{
		BoundBrush (b[i]);
		for (j=0 ; j<3 ; j++)
		{
			if (b[i]->mins[j] < -4096 || b[i]->maxs[j] > 4096)
			{
				Log_Print("Q1_SplitBrush: bogus brush after clip\n");
				break;
			} //end if
		} //end for

		if (b[i]->numsides < 3 || j < 3)
		{
			FreeBrush (b[i]);
			b[i] = NULL;
			Log_Print("Q1_SplitBrush: numsides < 3\n");
		} //end if
	} //end for

	if ( !(b[0] && b[1]) )
	{
		if (!b[0] && !b[1])
			Log_Print("Q1_SplitBrush: split removed brush\n");
		else
			Log_Print("Q1_SplitBrush: split not on both sides\n");
		if (b[0])
		{
			FreeBrush (b[0]);
			*front = CopyBrush (brush);
		} //end if
		if (b[1])
		{
			FreeBrush (b[1]);
			*back = CopyBrush (brush);
		} //end if
		return;
	} //end if

	// add the midwinding to both sides
	for (i = 0; i < 2; i++)
	{
		cs = &b[i]->sides[b[i]->numsides];
		b[i]->numsides++;

		cs->planenum = planenum^i^1;
		cs->texinfo = 0;
		//store the node number in the surf to find the texinfo later on
		cs->surf = nodenum;
		//
		cs->flags &= ~SFL_VISIBLE;
		cs->flags &= ~SFL_TESTED;
		cs->flags &= ~SFL_TEXTURED;
		if (i==0)
			cs->winding = CopyWinding (midwinding);
		else
			cs->winding = midwinding;
	} //end for


{
	vec_t v1;
	int i;

	for (i=0 ; i<2 ; i++)
	{
		v1 = BrushVolume (b[i]);
		if (v1 < 1)
		{
			FreeBrush (b[i]);
			b[i] = NULL;
			Log_Print("Q1_SplitBrush: tiny volume after clip\n");
		} //end if
	} //end for
} //*/

	*front = b[0];
	*back = b[1];
} //end of the function Q1_SplitBrush
//===========================================================================
// returns true if the tree starting at nodenum has only solid leaves
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Q1_SolidTree_r(int nodenum)
{
	if (nodenum < 0)
	{
		switch(q1_dleafs[(-nodenum) - 1].contents)
		{
			case Q1_CONTENTS_EMPTY:
			{
				return false;
			} //end case
			case Q1_CONTENTS_SOLID:
#ifdef HLCONTENTS
			case Q1_CONTENTS_CLIP:
#endif HLCONTENTS
			case Q1_CONTENTS_SKY:
#ifdef HLCONTENTS
			case Q1_CONTENTS_TRANSLUCENT:
#endif HLCONTENTS
			{
				return true;
			} //end case
			case Q1_CONTENTS_WATER:
			case Q1_CONTENTS_SLIME:
			case Q1_CONTENTS_LAVA:
#ifdef HLCONTENTS
			//these contents should not be found in the BSP
			case Q1_CONTENTS_ORIGIN:
			case Q1_CONTENTS_CURRENT_0:
			case Q1_CONTENTS_CURRENT_90:
			case Q1_CONTENTS_CURRENT_180:
			case Q1_CONTENTS_CURRENT_270:
			case Q1_CONTENTS_CURRENT_UP:
			case Q1_CONTENTS_CURRENT_DOWN:
#endif HLCONTENTS
			default:
			{
				return false;
			} //end default
		} //end switch
		return false;
	} //end if
	if (!Q1_SolidTree_r(q1_dnodes[nodenum].children[0])) return false;
	if (!Q1_SolidTree_r(q1_dnodes[nodenum].children[1])) return false;
	return true;
} //end of the function Q1_SolidTree_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *Q1_CreateBrushes_r(bspbrush_t *brush, int nodenum)
{
	int planenum;
	bspbrush_t *front, *back;
	q1_dleaf_t *leaf;

	//if it is a leaf
	if (nodenum < 0)
	{
		leaf = &q1_dleafs[(-nodenum) - 1];
		if (leaf->contents != Q1_CONTENTS_EMPTY)
		{
#ifdef Q1_PRINT
			qprintf("\r%5i", ++q1_numbrushes);
#endif //Q1_PRINT
		} //end if
		switch(leaf->contents)
		{
			case Q1_CONTENTS_EMPTY:
			{
				FreeBrush(brush);
				return NULL;
			} //end case
			case Q1_CONTENTS_SOLID:
#ifdef HLCONTENTS
			case Q1_CONTENTS_CLIP:
#endif HLCONTENTS
			case Q1_CONTENTS_SKY:
#ifdef HLCONTENTS
			case Q1_CONTENTS_TRANSLUCENT:
#endif HLCONTENTS
			{
				brush->side = CONTENTS_SOLID;
				return brush;
			} //end case
			case Q1_CONTENTS_WATER:
			{
				brush->side = CONTENTS_WATER;
				return brush;
			} //end case
			case Q1_CONTENTS_SLIME:
			{
				brush->side = CONTENTS_SLIME;
				return brush;
			} //end case
			case Q1_CONTENTS_LAVA:
			{
				brush->side = CONTENTS_LAVA;
				return brush;
			} //end case
#ifdef HLCONTENTS
			//these contents should not be found in the BSP
			case Q1_CONTENTS_ORIGIN:
			case Q1_CONTENTS_CURRENT_0:
			case Q1_CONTENTS_CURRENT_90:
			case Q1_CONTENTS_CURRENT_180:
			case Q1_CONTENTS_CURRENT_270:
			case Q1_CONTENTS_CURRENT_UP:
			case Q1_CONTENTS_CURRENT_DOWN:
			{
				Error("Q1_CreateBrushes_r: found contents %d in Half-Life BSP", leaf->contents);
				return NULL;
			} //end case
#endif HLCONTENTS
			default:
			{
				Error("Q1_CreateBrushes_r: unknown contents %d in Half-Life BSP", leaf->contents);
				return NULL;
			} //end default
		} //end switch
		return NULL;
	} //end if
	//if the rest of the tree is solid
	/*if (Q1_SolidTree_r(nodenum))
	{
		brush->side = CONTENTS_SOLID;
		return brush;
	} //end if*/
	//
	planenum = q1_dnodes[nodenum].planenum;
	planenum = FindFloatPlane(q1_dplanes[planenum].normal, q1_dplanes[planenum].dist);
	//split the brush with the node plane
	Q1_SplitBrush(brush, planenum, nodenum, &front, &back);
	//free the original brush
	FreeBrush(brush);
	//every node must split the brush in two
	if (!front || !back)
	{
		Log_Print("Q1_CreateBrushes_r: WARNING node not splitting brush\n");
		//return NULL;
	} //end if
	//create brushes recursively
	if (front) front = Q1_CreateBrushes_r(front, q1_dnodes[nodenum].children[0]);
	if (back) back = Q1_CreateBrushes_r(back, q1_dnodes[nodenum].children[1]);
	//link the brushes if possible and return them
	if (front)
	{
		for (brush = front; brush->next; brush = brush->next);
		brush->next = back;
		return front;
	} //end if
	else
	{
		return back;
	} //end else
} //end of the function Q1_CreateBrushes_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *Q1_CreateBrushesFromBSP(int modelnum)
{
	bspbrush_t *brushlist;
	bspbrush_t *brush;
	q1_dnode_t *headnode;
	vec3_t mins, maxs;
	int i;

	//
	headnode = &q1_dnodes[q1_dmodels[modelnum].headnode[0]];
	//get the mins and maxs of the world
	VectorCopy(headnode->mins, mins);
	VectorCopy(headnode->maxs, maxs);
	//enlarge these mins and maxs
	for (i = 0; i < 3; i++)
	{
		mins[i] -= 8;
		maxs[i] += 8;
	} //end for
	//NOTE: have to add the BSP tree mins and maxs to the MAP mins and maxs
	AddPointToBounds(mins, map_mins, map_maxs);
	AddPointToBounds(maxs, map_mins, map_maxs);
	//
	if (!modelnum)
	{
		Log_Print("brush size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n",
							map_mins[0], map_mins[1], map_mins[2],
							map_maxs[0], map_maxs[1], map_maxs[2]);
	} //end if
	//create one huge brush containing the whole world
	brush = BrushFromBounds(mins, maxs);
	VectorCopy(mins, brush->mins);
	VectorCopy(maxs, brush->maxs);
	//
#ifdef Q1_PRINT
	qprintf("creating Quake brushes\n");
	qprintf("%5d brushes", q1_numbrushes = 0);
#endif //Q1_PRINT
	//create the brushes
	brushlist = Q1_CreateBrushes_r(brush, q1_dmodels[modelnum].headnode[0]);
	//
#ifdef Q1_PRINT
	qprintf("\n");
#endif //Q1_PRINT
	//now we've got a list with brushes!
	return brushlist;
} //end of the function Q1_CreateBrushesFromBSP
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
q1_dleaf_t *Q1_PointInLeaf(int startnode, vec3_t point)
{
	int nodenum;
	vec_t	dist;
	q1_dnode_t *node;
	q1_dplane_t *plane;

	nodenum = startnode;
	while (nodenum >= 0)
	{
		node = &q1_dnodes[nodenum];
		plane = &q1_dplanes[node->planenum];
		dist = DotProduct(point, plane->normal) - plane->dist;
		if (dist > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	} //end while

	return &q1_dleafs[-nodenum - 1];
} //end of the function Q1_PointInLeaf
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Q1_FaceArea(q1_dface_t *face)
{
	int i;
	float total;
	vec_t *v;
	vec3_t d1, d2, cross;
	q1_dedge_t *edge;

	edge = &q1_dedges[face->firstedge];
	v = q1_dvertexes[edge->v[0]].point;

	total = 0;
	for (i = 1; i < face->numedges - 1; i++)
	{
		edge = &q1_dedges[face->firstedge + i];
		VectorSubtract(q1_dvertexes[edge->v[0]].point, v, d1);
		VectorSubtract(q1_dvertexes[edge->v[1]].point, v, d2);
		CrossProduct(d1, d2, cross);
		total += 0.5 * VectorLength(cross);
	} //end for
	return total;
} //end of the function AAS_FaceArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_FacePlane(q1_dface_t *face, vec3_t normal, float *dist)
{
	vec_t *v1, *v2, *v3;
	vec3_t vec1, vec2;
	int side, edgenum;

	edgenum = q1_dsurfedges[face->firstedge];
	side = edgenum < 0;
	v1 = q1_dvertexes[q1_dedges[abs(edgenum)].v[side]].point;
	v2 = q1_dvertexes[q1_dedges[abs(edgenum)].v[!side]].point;
	edgenum = q1_dsurfedges[face->firstedge+1];
	side = edgenum < 0;
	v3 = q1_dvertexes[q1_dedges[abs(edgenum)].v[!side]].point;
	//
	VectorSubtract(v2, v1, vec1);
	VectorSubtract(v3, v1, vec2);

	CrossProduct(vec1, vec2, normal);
	VectorNormalize(normal);
	*dist = DotProduct(v1, normal);
} //end of the function Q1_FacePlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *Q1_MergeBrushes(bspbrush_t *brushlist, int modelnum)
{
	int nummerges, merged;
	bspbrush_t *b1, *b2, *tail, *newbrush, *newbrushlist;
	bspbrush_t *lastb2;

	if (!brushlist) return NULL;

	if (!modelnum) qprintf("%5d brushes merged", nummerges = 0);
	do
	{
		for (tail = brushlist; tail; tail = tail->next)
		{
			if (!tail->next) break;
		} //end for
		merged = 0;
		newbrushlist = NULL;
		for (b1 = brushlist; b1; b1 = brushlist)
		{
			lastb2 = b1;
			for (b2 = b1->next; b2; b2 = b2->next)
			{
				//can't merge brushes with different contents
				if (b1->side != b2->side) newbrush = NULL;
				else newbrush = TryMergeBrushes(b1, b2);
				//if a merged brush is created
				if (newbrush)
				{
					//copy the brush contents
					newbrush->side = b1->side;
					//add the new brush to the end of the list
					tail->next = newbrush;
					//remove the second brush from the list
					lastb2->next = b2->next;
					//remove the first brush from the list
					brushlist = brushlist->next;
					//free the merged brushes
					FreeBrush(b1);
					FreeBrush(b2);
					//get a new tail brush
					for (tail = brushlist; tail; tail = tail->next)
					{
						if (!tail->next) break;
					} //end for
					merged++;
					if (!modelnum) qprintf("\r%5d", nummerges++);
					break;
				} //end if
				lastb2 = b2;
			} //end for
			//if b1 can't be merged with any of the other brushes
			if (!b2)
			{
				brushlist = brushlist->next;
				//keep b1
				b1->next = newbrushlist;
				newbrushlist = b1;
			} //end else
		} //end for
		brushlist = newbrushlist;
	} while(merged);
	if (!modelnum) qprintf("\n");
	return newbrushlist;
} //end of the function Q1_MergeBrushes
//===========================================================================
// returns the amount the face and the winding overlap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Q1_FaceOnWinding(q1_dface_t *face, winding_t *winding)
{
	int i, edgenum, side;
	float dist, area;
	q1_dplane_t plane;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	winding_t *w;

	//
	w = CopyWinding(winding);
	memcpy(&plane, &q1_dplanes[face->planenum], sizeof(q1_dplane_t));
	//check on which side of the plane the face is
	if (face->side)
	{
		VectorNegate(plane.normal, plane.normal);
		plane.dist = -plane.dist;
	} //end if
	for (i = 0; i < face->numedges && w; i++)
	{
		//get the first and second vertex of the edge
		edgenum = q1_dsurfedges[face->firstedge + i];
		side = edgenum > 0;
		//if the face plane is flipped
		v1 = q1_dvertexes[q1_dedges[abs(edgenum)].v[side]].point;
		v2 = q1_dvertexes[q1_dedges[abs(edgenum)].v[!side]].point;
		//create a plane through the edge vector, orthogonal to the face plane
		//and with the normal vector pointing out of the face
		VectorSubtract(v1, v2, edgevec);
		CrossProduct(edgevec, plane.normal, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		ChopWindingInPlace(&w, normal, dist, 0.9); //CLIP_EPSILON
	} //end for
	if (w)
	{
		area = WindingArea(w);
		FreeWinding(w);
		return area;
	} //end if
	return 0;
} //end of the function Q1_FaceOnWinding
//===========================================================================
// returns a list with brushes created by splitting the given brush with
// planes that go through the face edges and are orthogonal to the face plane
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *Q1_SplitBrushWithFace(bspbrush_t *brush, q1_dface_t *face)
{
	int i, edgenum, side, planenum, splits;
	float dist;
	q1_dplane_t plane;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	bspbrush_t *front, *back, *brushlist;

	memcpy(&plane, &q1_dplanes[face->planenum], sizeof(q1_dplane_t));
	//check on which side of the plane the face is
	if (face->side)
	{
		VectorNegate(plane.normal, plane.normal);
		plane.dist = -plane.dist;
	} //end if
	splits = 0;
	brushlist = NULL;
	for (i = 0; i < face->numedges; i++)
	{
		//get the first and second vertex of the edge
		edgenum = q1_dsurfedges[face->firstedge + i];
		side = edgenum > 0;
		//if the face plane is flipped
		v1 = q1_dvertexes[q1_dedges[abs(edgenum)].v[side]].point;
		v2 = q1_dvertexes[q1_dedges[abs(edgenum)].v[!side]].point;
		//create a plane through the edge vector, orthogonal to the face plane
		//and with the normal vector pointing out of the face
		VectorSubtract(v1, v2, edgevec);
		CrossProduct(edgevec, plane.normal, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		planenum = FindFloatPlane(normal, dist);
		//split the current brush
		SplitBrush(brush, planenum, &front, &back);
		//if there is a back brush just put it in the list
		if (back)
		{
			//copy the brush contents
			back->side = brush->side;
			//
			back->next = brushlist;
			brushlist = back;
			splits++;
		} //end if
		if (!front)
		{
			Log_Print("Q1_SplitBrushWithFace: no new brush\n");
			FreeBrushList(brushlist);
			return NULL;
		} //end if
		//copy the brush contents
		front->side = brush->side;
		//continue splitting the front brush
		brush = front;
	} //end for
	if (!splits)
	{
		FreeBrush(front);
		return NULL;
	} //end if
	front->next = brushlist;
	brushlist = front;
	return brushlist;
} //end of the function Q1_SplitBrushWithFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *Q1_TextureBrushes(bspbrush_t *brushlist, int modelnum)
{
	float area, largestarea;
	int i, n, texinfonum, sn, numbrushes, ofs;
	int bestfacenum, sidenodenum;
	side_t *side;
	q1_dmiptexlump_t *miptexlump;
	q1_miptex_t *miptex;
	bspbrush_t *brush, *nextbrush, *prevbrush, *newbrushes, *brushlistend;
	vec_t defaultvec[4] = {1, 0, 0, 0};

	if (!modelnum) qprintf("texturing brushes\n");
	if (!modelnum) qprintf("%5d brushes", numbrushes = 0);
	//get a pointer to the last brush in the list
	for (brushlistend = brushlist; brushlistend; brushlistend = brushlistend->next)
	{
		if (!brushlistend->next) break;
	} //end for
	//there's no previous brush when at the start of the list
	prevbrush = NULL;
	//go over the brush list
	for (brush = brushlist; brush; brush = nextbrush)
	{
		nextbrush = brush->next;
		//find a texinfo for every brush side
		for (sn = 0; sn < brush->numsides; sn++)
		{
			side = &brush->sides[sn];
			//
			if (side->flags & SFL_TEXTURED) continue;
			//number of the node that created this brush side
			sidenodenum = side->surf;	//see midwinding in Q1_SplitBrush
			//no face found yet
			bestfacenum = -1;
			//minimum face size
			largestarea = 1;
			//if optimizing the texture placement and not going for the
			//least number of brushes
			if (!lessbrushes)
			{
				for (i = 0; i < q1_numfaces; i++)
				{
					//the face must be in the same plane as the node plane that created
					//this brush side
					if (q1_dfaces[i].planenum == q1_dnodes[sidenodenum].planenum)
					{
						//get the area the face and the brush side overlap
						area = Q1_FaceOnWinding(&q1_dfaces[i], side->winding);
						//if this face overlaps the brush side winding more than previous faces
						if (area > largestarea)
						{
							//if there already was a face for texturing this brush side with
							//a different texture
							if (bestfacenum >= 0 &&
									(q1_dfaces[bestfacenum].texinfo != q1_dfaces[i].texinfo))
							{
								//split the brush to fit the texture
								newbrushes = Q1_SplitBrushWithFace(brush, &q1_dfaces[i]);
								//if new brushes where created
								if (newbrushes)
								{
									//remove the current brush from the list
									if (prevbrush) prevbrush->next = brush->next;
									else brushlist = brush->next;
									if (brushlistend == brush)
									{
										brushlistend = prevbrush;
										nextbrush = newbrushes;
									} //end if
									//add the new brushes to the end of the list
									if (brushlistend) brushlistend->next = newbrushes;
									else brushlist = newbrushes;
									//free the current brush
									FreeBrush(brush);
									//don't forget about the prevbrush pointer at the bottom of
									//the outer loop
									brush = prevbrush;
									//find the end of the list
									for (brushlistend = brushlist; brushlistend; brushlistend = brushlistend->next)
									{
										if (!brushlistend->next) break;
									} //end for
									break;
								} //end if
								else
								{
									Log_Write("brush %d: no real texture split", numbrushes);
								} //end else
							} //end if
							else
							{
								//best face for texturing this brush side
								bestfacenum = i;
							} //end else
						} //end if
					} //end if
				} //end for
				//if the brush was split the original brush is removed
				//and we just continue with the next one in the list
				if (i < q1_numfaces) break;
			} //end if
			else
			{
				//find the face with the largest overlap with this brush side
				//for texturing the brush side
				for (i = 0; i < q1_numfaces; i++)
				{
					//the face must be in the same plane as the node plane that created
					//this brush side
					if (q1_dfaces[i].planenum == q1_dnodes[sidenodenum].planenum)
					{
						//get the area the face and the brush side overlap
						area = Q1_FaceOnWinding(&q1_dfaces[i], side->winding);
						//if this face overlaps the brush side winding more than previous faces
						if (area > largestarea)
						{
							largestarea = area;
							bestfacenum = i;
						} //end if
					} //end if
				} //end for
			} //end else
			//if a face was found for texturing this brush side
			if (bestfacenum >= 0)
			{
				//set the MAP texinfo values
				texinfonum = q1_dfaces[bestfacenum].texinfo;
				for (n = 0; n < 4; n++)
				{
					map_texinfo[texinfonum].vecs[0][n] = q1_texinfo[texinfonum].vecs[0][n];
					map_texinfo[texinfonum].vecs[1][n] = q1_texinfo[texinfonum].vecs[1][n];
				} //end for
				//make sure the two vectors aren't of zero length otherwise use the default
				//vector to prevent a divide by zero in the map writing
				if (VectorLength(map_texinfo[texinfonum].vecs[0]) < 0.01)
					memcpy(map_texinfo[texinfonum].vecs[0], defaultvec, sizeof(defaultvec));
				if (VectorLength(map_texinfo[texinfonum].vecs[1]) < 0.01)
					memcpy(map_texinfo[texinfonum].vecs[1], defaultvec, sizeof(defaultvec));
				//
				map_texinfo[texinfonum].flags = q1_texinfo[texinfonum].flags;
				map_texinfo[texinfonum].value = 0; //Q1 and HL texinfos don't have a value
				//the mip texture
				miptexlump = (q1_dmiptexlump_t *) q1_dtexdata;
				ofs = miptexlump->dataofs[q1_texinfo[texinfonum].miptex];
				if ( ofs > q1_texdatasize ) {
					ofs = miptexlump->dataofs[0];
				}
				miptex = (q1_miptex_t *)((byte *)miptexlump + ofs);
				//get the mip texture name
				strcpy(map_texinfo[texinfonum].texture, miptex->name);
				//no animations in Quake1 and Half-Life mip textures
				map_texinfo[texinfonum].nexttexinfo = -1;
				//store the texinfo number
				side->texinfo = texinfonum;
				//
				if (texinfonum > map_numtexinfo) map_numtexinfo = texinfonum;
				//this side is textured
				side->flags |= SFL_TEXTURED;
			} //end if
			else
			{
				//no texture for this side
				side->texinfo = TEXINFO_NODE;
				//this side is textured
				side->flags |= SFL_TEXTURED;
			} //end if
		} //end for
		//
		if (!modelnum && prevbrush != brush) qprintf("\r%5d", ++numbrushes);
		//previous brush in the list
		prevbrush = brush;
	} //end for
	if (!modelnum) qprintf("\n");
	//return the new list with brushes
	return brushlist;
} //end of the function Q1_TextureBrushes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_FixContentsTextures(bspbrush_t *brushlist)
{
	int i, texinfonum;
	bspbrush_t *brush;

	for (brush = brushlist; brush; brush = brush->next)
	{
		//only fix the textures of water, slime and lava brushes
		if (brush->side != CONTENTS_WATER &&
			brush->side != CONTENTS_SLIME &&
			brush->side != CONTENTS_LAVA) continue;
		//
		for (i = 0; i < brush->numsides; i++)
		{
			texinfonum = brush->sides[i].texinfo;
			if (Q1_TextureContents(map_texinfo[texinfonum].texture) == brush->side) break;
		} //end for
		//if no specific contents texture was found
		if (i >= brush->numsides)
		{
			texinfonum = -1;
			for (i = 0; i < map_numtexinfo; i++)
			{
				if (Q1_TextureContents(map_texinfo[i].texture) == brush->side)
				{
					texinfonum = i;
					break;
				} //end if
			} //end for
		} //end if
		//
		if (texinfonum >= 0)
		{
			//give all the brush sides this contents texture
			for (i = 0; i < brush->numsides; i++)
			{
				brush->sides[i].texinfo = texinfonum;
			} //end for
		} //end if
		else Log_Print("brush contents %d with wrong textures\n", brush->side);
		//
	} //end for
	/*
	for (brush = brushlist; brush; brush = brush->next)
	{
		//give all the brush sides this contents texture
		for (i = 0; i < brush->numsides; i++)
		{
			if (Q1_TextureContents(map_texinfo[texinfonum].texture) != brush->side)
			{
				Error("brush contents %d with wrong contents textures %s\n", brush->side,
							Q1_TextureContents(map_texinfo[texinfonum].texture));
			} //end if
		} //end for
	} //end for*/
} //end of the function Q1_FixContentsTextures
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_BSPBrushToMapBrush(bspbrush_t *bspbrush, entity_t *mapent)
{
	mapbrush_t *mapbrush;
	side_t *side;
	int i, besttexinfo;

	CheckBSPBrush(bspbrush);

	if (nummapbrushes >= MAX_MAPFILE_BRUSHES)
	Error ("nummapbrushes == MAX_MAPFILE_BRUSHES");

	mapbrush = &mapbrushes[nummapbrushes];
	mapbrush->original_sides = &brushsides[nummapbrushsides];
	mapbrush->entitynum = mapent - entities;
	mapbrush->brushnum = nummapbrushes - mapent->firstbrush;
	mapbrush->leafnum = -1;
	mapbrush->numsides = 0;

	besttexinfo = TEXINFO_NODE;
	for (i = 0; i < bspbrush->numsides; i++)
	{
		if (!bspbrush->sides[i].winding) continue;
		//
		if (nummapbrushsides >= MAX_MAPFILE_BRUSHSIDES)
			Error ("MAX_MAPFILE_BRUSHSIDES");
		side = &brushsides[nummapbrushsides];
		//the contents of the bsp brush is stored in the side variable
		side->contents = bspbrush->side;
		side->surf = 0;
		side->planenum = bspbrush->sides[i].planenum;
		side->texinfo = bspbrush->sides[i].texinfo;
		if (side->texinfo != TEXINFO_NODE)
		{
			//this brush side is textured
			side->flags |= SFL_TEXTURED;
			besttexinfo = side->texinfo;
		} //end if
		//
		nummapbrushsides++;
		mapbrush->numsides++;
	} //end for
	//
	if (besttexinfo == TEXINFO_NODE)
	{
		mapbrush->numsides = 0;
		q1_numclipbrushes++;
		return;
	} //end if
	//set the texinfo for all the brush sides without texture
	for (i = 0; i < mapbrush->numsides; i++)
	{
		if (mapbrush->original_sides[i].texinfo == TEXINFO_NODE)
		{
			mapbrush->original_sides[i].texinfo = besttexinfo;
		} //end if
	} //end for
	//contents of the brush
	mapbrush->contents = bspbrush->side;
	//
	if (create_aas)
	{
		//create the AAS brushes from this brush, add brush bevels
		AAS_CreateMapBrushes(mapbrush, mapent, true);
		return;
	} //end if
	//create windings for sides and bounds for brush
	MakeBrushWindings(mapbrush);
	//add brush bevels
	AddBrushBevels(mapbrush);
	//a new brush has been created
	nummapbrushes++;
	mapent->numbrushes++;
} //end of the function Q1_BSPBrushToMapBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_CreateMapBrushes(entity_t *mapent, int modelnum)
{
	bspbrush_t *brushlist, *brush, *nextbrush;
	int i;

	//create brushes from the model BSP tree
	brushlist = Q1_CreateBrushesFromBSP(modelnum);
	//texture the brushes and split them when necesary
	brushlist = Q1_TextureBrushes(brushlist, modelnum);
	//fix the contents textures of all brushes
	Q1_FixContentsTextures(brushlist);
	//
	if (!nobrushmerge)
	{
		brushlist = Q1_MergeBrushes(brushlist, modelnum);
		//brushlist = Q1_MergeBrushes(brushlist, modelnum);
	} //end if
	//
	if (!modelnum) qprintf("converting brushes to map brushes\n");
	if (!modelnum) qprintf("%5d brushes", i = 0);
	for (brush = brushlist; brush; brush = nextbrush)
	{
		nextbrush = brush->next;
		Q1_BSPBrushToMapBrush(brush, mapent);
		brush->next = NULL;
		FreeBrush(brush);
		if (!modelnum) qprintf("\r%5d", ++i);
	} //end for
	if (!modelnum) qprintf("\n");
} //end of the function Q1_CreateMapBrushes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_ResetMapLoading(void)
{
} //end of the function Q1_ResetMapLoading
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q1_LoadMapFromBSP(char *filename, int offset, int length)
{
	int i, modelnum;
	char *model, *classname;

	Log_Print("-- Q1_LoadMapFromBSP --\n");
	//the loaded map type
	loadedmaptype = MAPTYPE_QUAKE1;
	//
	qprintf("loading map from %s at %d\n", filename, offset);
	//load the Half-Life BSP file
	Q1_LoadBSPFile(filename, offset, length);
	//
	q1_numclipbrushes = 0;
	//CreatePath(path);
	//Q1_CreateQ2WALFiles(path);
	//parse the entities from the BSP
	Q1_ParseEntities();
	//clear the map mins and maxs
	ClearBounds(map_mins, map_maxs);
	//
	qprintf("creating Quake1 brushes\n");
	if (lessbrushes) qprintf("creating minimum number of brushes\n");
	else qprintf("placing textures correctly\n");
	//
	for (i = 0; i < num_entities; i++)
	{
		entities[i].firstbrush = nummapbrushes;
		entities[i].numbrushes = 0;
		//
		classname = ValueForKey(&entities[i], "classname");
		if (classname && !strcmp(classname, "worldspawn"))
		{
			modelnum = 0;
		} //end if
		else
		{
			//
			model = ValueForKey(&entities[i], "model");
			if (!model || *model != '*') continue;
			model++;
			modelnum = atoi(model);
		} //end else
		//create map brushes for the entity
		Q1_CreateMapBrushes(&entities[i], modelnum);
	} //end for
	//
	qprintf("%5d map brushes\n", nummapbrushes);
	qprintf("%5d clip brushes\n", q1_numclipbrushes);
} //end of the function Q1_LoadMapFromBSP
