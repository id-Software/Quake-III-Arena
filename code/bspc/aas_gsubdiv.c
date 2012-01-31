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
#include "../botlib/aasfile.h"
#include "aas_create.h"
#include "aas_store.h"
#include "aas_cfg.h"

#define FACECLIP_EPSILON			0.2
#define FACE_EPSILON					1.0

int numgravitationalsubdivisions = 0;
int numladdersubdivisions = 0;

//NOTE: only do gravitational subdivision BEFORE area merging!!!!!!!
//			because the bsp tree isn't refreshes like with ladder subdivision

//===========================================================================
// NOTE: the original face is invalid after splitting
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SplitFace(tmp_face_t *face, vec3_t normal, float dist,
							tmp_face_t **frontface, tmp_face_t **backface)
{
	winding_t *frontw, *backw;

	//
	*frontface = *backface = NULL;

	ClipWindingEpsilon(face->winding, normal, dist, FACECLIP_EPSILON, &frontw, &backw);

#ifdef DEBUG
	//
	if (frontw)
	{
		if (WindingIsTiny(frontw))
		{
			Log_Write("AAS_SplitFace: tiny back face\r\n");
			FreeWinding(frontw);
			frontw = NULL;
		} //end if
	} //end if
	if (backw)
	{
		if (WindingIsTiny(backw))
		{
			Log_Write("AAS_SplitFace: tiny back face\r\n");
			FreeWinding(backw);
			backw = NULL;
		} //end if
	} //end if
#endif //DEBUG
	//if the winding was split
	if (frontw)
	{
		//check bounds
		(*frontface) = AAS_AllocTmpFace();
		(*frontface)->planenum = face->planenum;
		(*frontface)->winding = frontw;
		(*frontface)->faceflags = face->faceflags;
	} //end if
	if (backw)
	{
		//check bounds
		(*backface) = AAS_AllocTmpFace();
		(*backface)->planenum = face->planenum;
		(*backface)->winding = backw;
		(*backface)->faceflags = face->faceflags;
	} //end if
} //end of the function AAS_SplitFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
winding_t *AAS_SplitWinding(tmp_area_t *tmparea, int planenum)
{
	tmp_face_t *face;
	plane_t *plane;
	int side;
	winding_t *splitwinding;

	//
	plane = &mapplanes[planenum];
	//create a split winding, first base winding for plane
	splitwinding = BaseWindingForPlane(plane->normal, plane->dist);
	//chop with all the faces of the area
	for (face = tmparea->tmpfaces; face && splitwinding; face = face->next[side])
	{
		//side of the face the original area was on
		side = face->frontarea != tmparea;
		plane = &mapplanes[face->planenum ^ side];
		ChopWindingInPlace(&splitwinding, plane->normal, plane->dist, 0); // PLANESIDE_EPSILON);
	} //end for
	return splitwinding;
} //end of the function AAS_SplitWinding
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TestSplitPlane(tmp_area_t *tmparea, vec3_t normal, float dist,
							int *facesplits, int *groundsplits, int *epsilonfaces)
{
	int j, side, front, back, planenum;
	float d, d_front, d_back;
	tmp_face_t *face;
	winding_t *w;

	*facesplits = *groundsplits = *epsilonfaces = 0;

	planenum = FindFloatPlane(normal, dist);

	w = AAS_SplitWinding(tmparea, planenum);
	if (!w) return false;
	FreeWinding(w);
	//
	for (face = tmparea->tmpfaces; face; face = face->next[side])
	{
		//side of the face the area is on
		side = face->frontarea != tmparea;

		if ((face->planenum & ~1) == (planenum & ~1))
		{
			Log_Print("AAS_TestSplitPlane: tried face plane as splitter\n");
			return false;
		} //end if
		w = face->winding;
		//reset distance at front and back side of plane
		d_front = d_back = 0;
		//reset front and back flags
		front = back = 0;
		for (j = 0; j < w->numpoints; j++)
		{
			d = DotProduct(w->p[j], normal) - dist;
			if (d > d_front) d_front = d;
			if (d < d_back) d_back = d;

			if (d > 0.4) // PLANESIDE_EPSILON)
				front = 1;
			if (d < -0.4) // PLANESIDE_EPSILON)
				back = 1;
		} //end for
		//check for an epsilon face
		if ( (d_front > FACECLIP_EPSILON && d_front < FACE_EPSILON)
			|| (d_back < -FACECLIP_EPSILON && d_back > -FACE_EPSILON) )
		{
			(*epsilonfaces)++;
		} //end if
		//if the face has points at both sides of the plane
		if (front && back)
		{
			(*facesplits)++;
			if (face->faceflags & FACE_GROUND)
			{
				(*groundsplits)++;
			} //end if
		} //end if
	} //end for
	return true;
} //end of the function AAS_TestSplitPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SplitArea(tmp_area_t *tmparea, int planenum, tmp_area_t **frontarea, tmp_area_t **backarea)
{
	int side;
	tmp_area_t *facefrontarea, *facebackarea, *faceotherarea;
	tmp_face_t *face, *frontface, *backface, *splitface, *nextface;
	winding_t *splitwinding;
	plane_t *splitplane;

/*
#ifdef AW_DEBUG
	int facesplits, groundsplits, epsilonface;
	Log_Print("\n----------------------\n");
	Log_Print("splitting area %d\n", areanum);
	Log_Print("with normal = \'%f %f %f\', dist = %f\n", normal[0], normal[1], normal[2], dist);
	AAS_TestSplitPlane(areanum, normal, dist,
										&facesplits, &groundsplits, &epsilonface);
	Log_Print("face splits = %d\nground splits = %d\n", facesplits, groundsplits);
	if (epsilonface) Log_Print("aaahh epsilon face\n");
#endif //AW_DEBUG*/
	//the original area

	AAS_FlipAreaFaces(tmparea);
	AAS_CheckArea(tmparea);
	//
	splitplane = &mapplanes[planenum];
/*	//create a split winding, first base winding for plane
	splitwinding = BaseWindingForPlane(splitplane->normal, splitplane->dist);
	//chop with all the faces of the area
	for (face = tmparea->tmpfaces; face && splitwinding; face = face->next[side])
	{
		//side of the face the original area was on
		side = face->frontarea != tmparea->areanum;
		plane = &mapplanes[face->planenum ^ side];
		ChopWindingInPlace(&splitwinding, plane->normal, plane->dist, 0); // PLANESIDE_EPSILON);
	} //end for*/
	splitwinding = AAS_SplitWinding(tmparea, planenum);
	if (!splitwinding)
	{
/*
#ifdef DEBUG
		AAS_TestSplitPlane(areanum, normal, dist,
											&facesplits, &groundsplits, &epsilonface);
		Log_Print("\nface splits = %d\nground splits = %d\n", facesplits, groundsplits);
		if (epsilonface) Log_Print("aaahh epsilon face\n");
#endif //DEBUG*/
		Error("AAS_SplitArea: no split winding when splitting area %d\n", tmparea->areanum);
	} //end if
	//create a split face
	splitface = AAS_AllocTmpFace();
	//get the map plane
	splitface->planenum = planenum;
	//store the split winding
	splitface->winding = splitwinding;
	//the new front area
	(*frontarea) = AAS_AllocTmpArea();
	(*frontarea)->presencetype = tmparea->presencetype;
	(*frontarea)->contents = tmparea->contents;
	(*frontarea)->modelnum = tmparea->modelnum;
	(*frontarea)->tmpfaces = NULL;
	//the new back area
	(*backarea) = AAS_AllocTmpArea();
	(*backarea)->presencetype = tmparea->presencetype;
	(*backarea)->contents = tmparea->contents;
	(*backarea)->modelnum = tmparea->modelnum;
	(*backarea)->tmpfaces = NULL;
	//add the split face to the new areas
	AAS_AddFaceSideToArea(splitface, 0, (*frontarea));
	AAS_AddFaceSideToArea(splitface, 1, (*backarea));

	//split all the faces of the original area
	for (face = tmparea->tmpfaces; face; face = nextface)
	{
		//side of the face the original area was on
		side = face->frontarea != tmparea;
		//next face of the original area
		nextface = face->next[side];
		//front area of the face
		facefrontarea = face->frontarea;
		//back area of the face
		facebackarea = face->backarea;
		//remove the face from both the front and back areas
		if (facefrontarea) AAS_RemoveFaceFromArea(face, facefrontarea);
		if (facebackarea) AAS_RemoveFaceFromArea(face, facebackarea);
		//split the face
		AAS_SplitFace(face, splitplane->normal, splitplane->dist, &frontface, &backface);
		//free the original face
		AAS_FreeTmpFace(face);
		//get the number of the area at the other side of the face
		if (side) faceotherarea = facefrontarea;
		else faceotherarea = facebackarea;
		//if there is an area at the other side of the original face
		if (faceotherarea)
		{
			if (frontface) AAS_AddFaceSideToArea(frontface, !side, faceotherarea);
			if (backface) AAS_AddFaceSideToArea(backface, !side, faceotherarea);
		} //end if
		//add the front and back part left after splitting the original face to the new areas
		if (frontface) AAS_AddFaceSideToArea(frontface, side, (*frontarea));
		if (backface) AAS_AddFaceSideToArea(backface, side, (*backarea));
	} //end for

	if (!(*frontarea)->tmpfaces) Log_Print("AAS_SplitArea: front area without faces\n");
	if (!(*backarea)->tmpfaces) Log_Print("AAS_SplitArea: back area without faces\n");

	tmparea->invalid = true;
/*
#ifdef AW_DEBUG
	for (i = 0, face = frontarea->tmpfaces; face; face = face->next[side])
	{
		side = face->frontarea != frontarea->areanum;
		i++;
	} //end for
	Log_Print("created front area %d with %d faces\n", frontarea->areanum, i);

	for (i = 0, face = backarea->tmpfaces; face; face = face->next[side])
	{
		side = face->frontarea != backarea->areanum;
		i++;
	} //end for
	Log_Print("created back area %d with %d faces\n", backarea->areanum, i);
#endif //AW_DEBUG*/

	AAS_FlipAreaFaces((*frontarea));
	AAS_FlipAreaFaces((*backarea));
	//
	AAS_CheckArea((*frontarea));
	AAS_CheckArea((*backarea));
} //end of the function AAS_SplitArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_FindBestAreaSplitPlane(tmp_area_t *tmparea, vec3_t normal, float *dist)
{
	int side1, side2;
	int foundsplitter, facesplits, groundsplits, epsilonfaces, bestepsilonfaces;
	float bestvalue, value;
	tmp_face_t *face1, *face2;
	vec3_t tmpnormal, invgravity;
	float tmpdist;

	//get inverse of gravity direction
	VectorCopy(cfg.phys_gravitydirection, invgravity);
	VectorInverse(invgravity);

	foundsplitter = false;
	bestvalue = -999999;
	bestepsilonfaces = 0;
	//
#ifdef AW_DEBUG
	Log_Print("finding split plane for area %d\n", tmparea->areanum);
#endif //AW_DEBUG
	for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
	{
		//side of the face the area is on
		side1 = face1->frontarea != tmparea;
		//
		if (WindingIsTiny(face1->winding))
		{
			Log_Write("gsubdiv: area %d has a tiny winding\r\n", tmparea->areanum);
			continue;
		} //end if
		//if the face isn't a gap or ground there's no split edge
		if (!(face1->faceflags & FACE_GROUND) && !AAS_GapFace(face1, side1)) continue;
		//
		for (face2 = face1->next[side1]; face2; face2 = face2->next[side2])
		{
			//side of the face the area is on
			side2 = face2->frontarea != tmparea;
			//
			if (WindingIsTiny(face1->winding))
			{
				Log_Write("gsubdiv: area %d has a tiny winding\r\n", tmparea->areanum);
				continue;
			} //end if
			//if the face isn't a gap or ground there's no split edge
			if (!(face2->faceflags & FACE_GROUND) && !AAS_GapFace(face2, side2)) continue;
			//only split between gaps and ground
			if (!(((face1->faceflags & FACE_GROUND) && AAS_GapFace(face2, side2)) ||
					((face2->faceflags & FACE_GROUND) && AAS_GapFace(face1, side1)))) continue;
			//find a plane seperating the windings of the faces
			if (!FindPlaneSeperatingWindings(face1->winding, face2->winding, invgravity,
														tmpnormal, &tmpdist)) continue;
#ifdef AW_DEBUG
			Log_Print("normal = \'%f %f %f\', dist = %f\n",
							tmpnormal[0], tmpnormal[1], tmpnormal[2], tmpdist);
#endif //AW_DEBUG
			//get metrics for this vertical plane
			if (!AAS_TestSplitPlane(tmparea, tmpnormal, tmpdist,
										&facesplits, &groundsplits, &epsilonfaces))
			{
				continue;
			} //end if
#ifdef AW_DEBUG
			Log_Print("face splits = %d\nground splits = %d\n",
							facesplits, groundsplits);
#endif //AW_DEBUG
			value = 100 - facesplits - 2 * groundsplits;
			//avoid epsilon faces
			value += epsilonfaces * -1000;
			if (value > bestvalue)
			{
				VectorCopy(tmpnormal, normal);
				*dist = tmpdist;
				bestvalue = value;
				bestepsilonfaces = epsilonfaces;
				foundsplitter = true;
			} //end if
		} //end for
	} //end for
	if (bestepsilonfaces)
	{
		Log_Write("found %d epsilon faces trying to split area %d\r\n",
									epsilonfaces, tmparea->areanum);
	} //end else
	return foundsplitter;
} //end of the function AAS_FindBestAreaSplitPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_SubdivideArea_r(tmp_node_t *tmpnode)
{
	int planenum;
	tmp_area_t *frontarea, *backarea;
	tmp_node_t *tmpnode1, *tmpnode2;
	vec3_t normal;
	float dist;

	if (AAS_FindBestAreaSplitPlane(tmpnode->tmparea, normal, &dist))
	{
		qprintf("\r%6d", ++numgravitationalsubdivisions);
		//
		planenum = FindFloatPlane(normal, dist);
		//split the area
		AAS_SplitArea(tmpnode->tmparea, planenum, &frontarea, &backarea);
		//
		tmpnode->tmparea = NULL;
		tmpnode->planenum = FindFloatPlane(normal, dist);
		//
		tmpnode1 = AAS_AllocTmpNode();
		tmpnode1->planenum = 0;
		tmpnode1->tmparea = frontarea;
		//
		tmpnode2 = AAS_AllocTmpNode();
		tmpnode2->planenum = 0;
		tmpnode2->tmparea = backarea;
		//subdivide the areas created by splitting recursively
		tmpnode->children[0] = AAS_SubdivideArea_r(tmpnode1);
		tmpnode->children[1] = AAS_SubdivideArea_r(tmpnode2);
	} //end if
	return tmpnode;
} //end of the function AAS_SubdivideArea_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_GravitationalSubdivision_r(tmp_node_t *tmpnode)
{
	//if this is a solid leaf
	if (!tmpnode) return NULL;
	//negative so it's an area
	if (tmpnode->tmparea) return AAS_SubdivideArea_r(tmpnode);
	//do the children recursively
	tmpnode->children[0] = AAS_GravitationalSubdivision_r(tmpnode->children[0]);
	tmpnode->children[1] = AAS_GravitationalSubdivision_r(tmpnode->children[1]);
	return tmpnode;
} //end of the function AAS_GravitationalSubdivision_r
//===========================================================================
// NOTE: merge faces and melt edges first
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_GravitationalSubdivision(void)
{
	Log_Write("AAS_GravitationalSubdivision\r\n");
	numgravitationalsubdivisions = 0;
	qprintf("%6i gravitational subdivisions", numgravitationalsubdivisions);
	//start with the head node
	AAS_GravitationalSubdivision_r(tmpaasworld.nodes);
	qprintf("\n");
	Log_Write("%6i gravitational subdivisions\r\n", numgravitationalsubdivisions);
} //end of the function AAS_GravitationalSubdivision
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_RefreshLadderSubdividedTree_r(tmp_node_t *tmpnode, tmp_area_t *tmparea,
												  tmp_node_t *tmpnode1, tmp_node_t *tmpnode2, int planenum)
{
	//if this is a solid leaf
	if (!tmpnode) return NULL;
	//negative so it's an area
	if (tmpnode->tmparea)
	{
		if (tmpnode->tmparea == tmparea)
		{
			tmpnode->tmparea = NULL;
			tmpnode->planenum = planenum;
			tmpnode->children[0] = tmpnode1;
			tmpnode->children[1] = tmpnode2;
		} //end if
		return tmpnode;
	} //end if
	//do the children recursively
	tmpnode->children[0] = AAS_RefreshLadderSubdividedTree_r(tmpnode->children[0],
									tmparea, tmpnode1, tmpnode2, planenum);
	tmpnode->children[1] = AAS_RefreshLadderSubdividedTree_r(tmpnode->children[1],
									tmparea, tmpnode1, tmpnode2, planenum);
	return tmpnode;
} //end of the function AAS_RefreshLadderSubdividedTree_r
//===========================================================================
// find an area with ladder faces and ground faces that are not connected
// split the area with a horizontal plane at the lowest vertex of all
// ladder faces in the area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_LadderSubdivideArea_r(tmp_node_t *tmpnode)
{
	int side1, i, planenum;
	int foundladderface, foundgroundface;
	float dist;
	tmp_area_t *tmparea, *frontarea, *backarea;
	tmp_face_t *face1;
	tmp_node_t *tmpnode1, *tmpnode2;
	vec3_t lowestpoint, normal = {0, 0, 1};
	plane_t *plane;
	winding_t *w;

	tmparea = tmpnode->tmparea;
	//skip areas with a liquid
	if (tmparea->contents & (AREACONTENTS_WATER
									| AREACONTENTS_LAVA
									| AREACONTENTS_SLIME)) return tmpnode;
	//must be possible to stand in the area
	if (!(tmparea->presencetype & PRESENCE_NORMAL)) return tmpnode;
	//
	foundladderface = false;
	foundgroundface = false;
	lowestpoint[2] = 99999;
	//
	for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
	{
		//side of the face the area is on
		side1 = face1->frontarea != tmparea;
		//if the face is a ladder face
		if (face1->faceflags & FACE_LADDER)
		{
			plane = &mapplanes[face1->planenum];
			//the ladder face plane should be pretty much vertical
			if (DotProduct(plane->normal, normal) > -0.1)
			{
				foundladderface = true;
				//find lowest point
				for (i = 0; i < face1->winding->numpoints; i++)
				{
					if (face1->winding->p[i][2] < lowestpoint[2])
					{
						VectorCopy(face1->winding->p[i], lowestpoint);
					} //end if
				} //end for
			} //end if
		} //end if
		else if (face1->faceflags & FACE_GROUND)
		{
			foundgroundface = true;
		} //end else if
	} //end for
	//
	if ((!foundladderface) || (!foundgroundface)) return tmpnode;
	//
	for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
	{
		//side of the face the area is on
		side1 = face1->frontarea != tmparea;
		//if the face isn't a ground face
		if (!(face1->faceflags & FACE_GROUND)) continue;
		//the ground plane
		plane = &mapplanes[face1->planenum];
		//get the difference between the ground plane and the lowest point
		dist = DotProduct(plane->normal, lowestpoint) - plane->dist;
		//if the lowest point is very near one of the ground planes
		if (dist > -1 && dist < 1)
		{
			return tmpnode;
		} //end if
	} //end for
	//
	dist = DotProduct(normal, lowestpoint);
	planenum = FindFloatPlane(normal, dist);
	//
	w = AAS_SplitWinding(tmparea, planenum);
	if (!w) return tmpnode;
	FreeWinding(w);
	//split the area with a horizontal plane through the lowest point
	qprintf("\r%6d", ++numladdersubdivisions);
	//
	AAS_SplitArea(tmparea, planenum, &frontarea, &backarea);
	//
	tmpnode->tmparea = NULL;
	tmpnode->planenum = planenum;
	//
	tmpnode1 = AAS_AllocTmpNode();
	tmpnode1->planenum = 0;
	tmpnode1->tmparea = frontarea;
	//
	tmpnode2 = AAS_AllocTmpNode();
	tmpnode2->planenum = 0;
	tmpnode2->tmparea = backarea;
	//subdivide the areas created by splitting recursively
	tmpnode->children[0] = AAS_LadderSubdivideArea_r(tmpnode1);
	tmpnode->children[1] = AAS_LadderSubdivideArea_r(tmpnode2);
	//refresh the tree
	AAS_RefreshLadderSubdividedTree_r(tmpaasworld.nodes, tmparea, tmpnode1, tmpnode2, planenum);
	//
	return tmpnode;
} //end of the function AAS_LadderSubdivideArea_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_LadderSubdivision_r(tmp_node_t *tmpnode)
{
	//if this is a solid leaf
	if (!tmpnode) return 0;
	//negative so it's an area
	if (tmpnode->tmparea) return AAS_LadderSubdivideArea_r(tmpnode);
	//do the children recursively
	tmpnode->children[0] = AAS_LadderSubdivision_r(tmpnode->children[0]);
	tmpnode->children[1] = AAS_LadderSubdivision_r(tmpnode->children[1]);
	return tmpnode;
} //end of the function AAS_LadderSubdivision_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_LadderSubdivision(void)
{
	Log_Write("AAS_LadderSubdivision\r\n");
	numladdersubdivisions = 0;
	qprintf("%6i ladder subdivisions", numladdersubdivisions);
	//start with the head node
	AAS_LadderSubdivision_r(tmpaasworld.nodes);
	//
	qprintf("\n");
	Log_Write("%6i ladder subdivisions\r\n", numladdersubdivisions);
} //end of the function AAS_LadderSubdivision
