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

#define CONVEX_EPSILON		0.3

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_RefreshMergedTree_r(tmp_node_t *tmpnode)
{
	tmp_area_t *tmparea;

	//if this is a solid leaf
	if (!tmpnode) return NULL;
	//if this is an area leaf
	if (tmpnode->tmparea)
	{
		tmparea = tmpnode->tmparea;
		while(tmparea->mergedarea) tmparea = tmparea->mergedarea;
		tmpnode->tmparea = tmparea;
		return tmpnode;
	} //end if
	//do the children recursively
	tmpnode->children[0] = AAS_RefreshMergedTree_r(tmpnode->children[0]);
	tmpnode->children[1] = AAS_RefreshMergedTree_r(tmpnode->children[1]);
	return tmpnode;
} //end of the function AAS_RefreshMergedTree_r
//===========================================================================
// returns true if the two given faces would create a non-convex area at
// the given sides, otherwise false is returned
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int NonConvex(tmp_face_t *face1, tmp_face_t *face2, int side1, int side2)
{
	int i;
	winding_t *w1, *w2;
	plane_t *plane1, *plane2;
	
	w1 = face1->winding;
	w2 = face2->winding;

	plane1 = &mapplanes[face1->planenum ^ side1];
	plane2 = &mapplanes[face2->planenum ^ side2];

	//check if one of the points of face1 is at the back of the plane of face2
	for (i = 0; i < w1->numpoints; i++)
	{
		if (DotProduct(plane2->normal, w1->p[i]) - plane2->dist < -CONVEX_EPSILON) return true;
	} //end for
	//check if one of the points of face2 is at the back of the plane of face1
	for (i = 0; i < w2->numpoints; i++)
	{
		if (DotProduct(plane1->normal, w2->p[i]) - plane1->dist < -CONVEX_EPSILON) return true;
	} //end for

	return false;
} //end of the function NonConvex
//===========================================================================
// try to merge the areas at both sides of the given face
//
// Parameter:				seperatingface		: face that seperates two areas
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TryMergeFaceAreas(tmp_face_t *seperatingface)
{
	int side1, side2, area1faceflags, area2faceflags;
	tmp_area_t *tmparea1, *tmparea2, *newarea;
	tmp_face_t *face1, *face2, *nextface1, *nextface2;

	tmparea1 = seperatingface->frontarea;
	tmparea2 = seperatingface->backarea;

	//areas must have the same presence type
	if (tmparea1->presencetype != tmparea2->presencetype) return false;
	//areas must have the same area contents
	if (tmparea1->contents != tmparea2->contents) return false;
	//areas must have the same bsp model inside (or both none)
	if (tmparea1->modelnum != tmparea2->modelnum) return false;

	area1faceflags = 0;
	area2faceflags = 0;
	for (face1 = tmparea1->tmpfaces; face1; face1 = face1->next[side1])
	{
		side1 = (face1->frontarea != tmparea1);
		//debug: check if the area belongs to the area
		if (face1->frontarea != tmparea1 &&
				face1->backarea != tmparea1) Error("face does not belong to area1");
		//just continue if the face is seperating the two areas
		//NOTE: a result of this is that ground and gap areas can
		//      be merged if the seperating face is the gap
		if ((face1->frontarea == tmparea1 &&
				face1->backarea == tmparea2) ||
				(face1->frontarea == tmparea2 &&
				face1->backarea == tmparea1)) continue;
		//get area1 face flags
		area1faceflags |= face1->faceflags;
		if (AAS_GapFace(face1, side1)) area1faceflags |= FACE_GAP;
		//
		for (face2 = tmparea2->tmpfaces; face2; face2 = face2->next[side2])
		{
			side2 = (face2->frontarea != tmparea2);
			//debug: check if the area belongs to the area
			if (face2->frontarea != tmparea2 &&
					face2->backarea != tmparea2) Error("face does not belong to area2");
			//just continue if the face is seperating the two areas
			//NOTE: a result of this is that ground and gap areas can
			//      be merged if the seperating face is the gap
			if ((face2->frontarea == tmparea1 &&
					face2->backarea == tmparea2) ||
					(face2->frontarea == tmparea2 &&
					face2->backarea == tmparea1)) continue;
			//get area2 face flags
			area2faceflags |= face2->faceflags;
			if (AAS_GapFace(face2, side2)) area2faceflags |= FACE_GAP;
			//if the two faces would create a non-convex area
			if (NonConvex(face1, face2, side1, side2)) return false;
		} //end for
	} //end for
	//if one area has gap faces (that aren't seperating the two areas)
	//and the other has ground faces (that aren't seperating the two areas),
	//the areas can't be merged
	if (((area1faceflags & FACE_GROUND) && (area2faceflags & FACE_GAP)) ||
			((area2faceflags & FACE_GROUND) && (area1faceflags & FACE_GAP)))
	{
//		Log_Print("   can't merge: ground/gap\n");
		return false;
	} //end if

//	Log_Print("merged area %d & %d to %d with %d faces\n", tmparea1->areanum, tmparea2->areanum, newarea->areanum, numfaces);
//	return false;
	//
	//AAS_CheckArea(tmparea1);
	//AAS_CheckArea(tmparea2);
	//create the new area
	newarea = AAS_AllocTmpArea();
	newarea->presencetype = tmparea1->presencetype;
	newarea->contents = tmparea1->contents;
	newarea->modelnum = tmparea1->modelnum;
	newarea->tmpfaces = NULL;

	//add all the faces (except the seperating ones) from the first area
	//to the new area
	for (face1 = tmparea1->tmpfaces; face1; face1 = nextface1)
	{
		side1 = (face1->frontarea != tmparea1);
		nextface1 = face1->next[side1];
		//don't add seperating faces
		if ((face1->frontarea == tmparea1 &&
				face1->backarea == tmparea2) ||
				(face1->frontarea == tmparea2 &&
				face1->backarea == tmparea1))
		{
			continue;
		} //end if
		//
		AAS_RemoveFaceFromArea(face1, tmparea1);
		AAS_AddFaceSideToArea(face1, side1, newarea);
	} //end for
	//add all the faces (except the seperating ones) from the second area
	//to the new area
	for (face2 = tmparea2->tmpfaces; face2; face2 = nextface2)
	{
		side2 = (face2->frontarea != tmparea2);
		nextface2 = face2->next[side2];
		//don't add seperating faces
		if ((face2->frontarea == tmparea1 &&
				face2->backarea == tmparea2) ||
				(face2->frontarea == tmparea2 &&
				face2->backarea == tmparea1))
		{
			continue;
		} //end if
		//
		AAS_RemoveFaceFromArea(face2, tmparea2);
		AAS_AddFaceSideToArea(face2, side2, newarea);
	} //end for
	//free all shared faces
	for (face1 = tmparea1->tmpfaces; face1; face1 = nextface1)
	{
		side1 = (face1->frontarea != tmparea1);
		nextface1 = face1->next[side1];
		//
		AAS_RemoveFaceFromArea(face1, face1->frontarea);
		AAS_RemoveFaceFromArea(face1, face1->backarea);
		AAS_FreeTmpFace(face1);
	} //end for
	//
	tmparea1->mergedarea = newarea;
	tmparea1->invalid = true;
	tmparea2->mergedarea = newarea;
	tmparea2->invalid = true;
	//
	AAS_CheckArea(newarea);
	AAS_FlipAreaFaces(newarea);
//	Log_Print("merged area %d & %d to %d with %d faces\n", tmparea1->areanum, tmparea2->areanum, newarea->areanum);
	return true;
} //end of the function AAS_TryMergeFaceAreas
//===========================================================================
// try to merge areas
// merged areas are added to the end of the convex area list so merging
// will be tried for those areas as well
//
// Parameter:				-
// Returns:					-
// Changes Globals:		tmpaasworld
//===========================================================================
/*
void AAS_MergeAreas(void)
{
	int side, nummerges;
	tmp_area_t *tmparea, *othertmparea;
	tmp_face_t *face;

	nummerges = 0;
	Log_Write("AAS_MergeAreas\r\n");
	qprintf("%6d areas merged", 1);
	//first merge grounded areas only
	//NOTE: this is useless because the area settings aren't available yet
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
//		Log_Print("checking area %d\n", i);
		//if the area is invalid
		if (tmparea->invalid)
		{
//			Log_Print("   area invalid\n");
			continue;
		} //end if
		//
//		if (!(tmparea->settings->areaflags & AREA_GROUNDED)) continue;
		//
		for (face = tmparea->tmpfaces; face; face = face->next[side])
		{
			side = (face->frontarea != tmparea);
			//if the face has both a front and back area
			if (face->frontarea && face->backarea)
			{
				//
				if (face->frontarea == tmparea) othertmparea = face->backarea;
				else othertmparea = face->frontarea;
//				if (!(othertmparea->settings->areaflags & AREA_GROUNDED)) continue;
//				Log_Print("  checking area %d with %d\n", face->frontarea, face->backarea);
				if (AAS_TryMergeFaceAreas(face))
				{
					qprintf("\r%6d", ++nummerges);
					break;
				} //end if
			} //end if
		} //end for
	} //end for
	//merge all areas
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
//		Log_Print("checking area %d\n", i);
		//if the area is invalid
		if (tmparea->invalid)
		{
//			Log_Print("   area invalid\n");
			continue;
		} //end if
		//
		for (face = tmparea->tmpfaces; face; face = face->next[side])
		{
			side = (face->frontarea != tmparea);
			//if the face has both a front and back area
			if (face->frontarea && face->backarea)
			{
//				Log_Print("  checking area %d with %d\n", face->frontarea, face->backarea);
				if (AAS_TryMergeFaceAreas(face))
				{
					qprintf("\r%6d", ++nummerges);
					break;
				} //end if
			} //end if
		} //end for
	} //end for
	Log_Print("\r%6d areas merged\n", nummerges);
	//refresh the merged tree
	AAS_RefreshMergedTree_r(tmpaasworld.nodes);
} //end of the function AAS_MergeAreas*/

int AAS_GroundArea(tmp_area_t *tmparea)
{
	tmp_face_t *face;
	int side;

	for (face = tmparea->tmpfaces; face; face = face->next[side])
	{
		side = (face->frontarea != tmparea);
		if (face->faceflags & FACE_GROUND) return true;
	} //end for
	return false;
} //end of the function AAS_GroundArea

void AAS_MergeAreas(void)
{
	int side, nummerges, merges, groundfirst;
	tmp_area_t *tmparea, *othertmparea;
	tmp_face_t *face;

	nummerges = 0;
	Log_Write("AAS_MergeAreas\r\n");
	qprintf("%6d areas merged", 1);
	//
	groundfirst = true;
	//for (i = 0; i < 4 || merges; i++)
	while(1)
	{
		//if (i < 2) groundfirst = true;
		//else groundfirst = false;
		//
		merges = 0;
		//first merge grounded areas only
		for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
		{
			//if the area is invalid
			if (tmparea->invalid)
			{
				continue;
			} //end if
			//
			if (groundfirst)
			{
				if (!AAS_GroundArea(tmparea)) continue;
			} //end if
			//
			for (face = tmparea->tmpfaces; face; face = face->next[side])
			{
				side = (face->frontarea != tmparea);
				//if the face has both a front and back area
				if (face->frontarea && face->backarea)
				{
					//
					if (face->frontarea == tmparea) othertmparea = face->backarea;
					else othertmparea = face->frontarea;
					//
					if (groundfirst)
					{
						if (!AAS_GroundArea(othertmparea)) continue;
					} //end if
					if (AAS_TryMergeFaceAreas(face))
					{
						qprintf("\r%6d", ++nummerges);
						merges++;
						break;
					} //end if
				} //end if
			} //end for
		} //end for
		if (!merges)
		{
			if (groundfirst) groundfirst = false;
			else break;
		} //end if
	} //end for
	qprintf("\n");
	Log_Write("%6d areas merged\r\n", nummerges);
	//refresh the merged tree
	AAS_RefreshMergedTree_r(tmpaasworld.nodes);
} //end of the function AAS_MergeAreas
