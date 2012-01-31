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

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TryMergeFaces(tmp_face_t *face1, tmp_face_t *face2)
{
	winding_t *neww;

#ifdef DEBUG
	if (!face1->winding) Error("face1 %d without winding", face1->num);
	if (!face2->winding) Error("face2 %d without winding", face2->num);
#endif //DEBUG
	//
	if (face1->faceflags != face2->faceflags) return false;
	//NOTE: if the front or back area is zero this doesn't mean there's
	//a real area. It means there's solid at that side of the face
	//if both faces have the same front area
	if (face1->frontarea == face2->frontarea)
	{
		//if both faces have the same back area
		if (face1->backarea == face2->backarea)
		{
			//if the faces are in the same plane
			if (face1->planenum == face2->planenum)
			{
				//if they have both a front and a back area (no solid on either side)
				if (face1->frontarea && face1->backarea)
				{
					neww = MergeWindings(face1->winding, face2->winding,
								mapplanes[face1->planenum].normal);
				} //end if
				else
				{
					//this function is to be found in l_poly.c
					neww = TryMergeWinding(face1->winding, face2->winding,
									mapplanes[face1->planenum].normal);
				} //end else
				if (neww)
				{
					FreeWinding(face1->winding);
					face1->winding = neww;
					if (face2->frontarea) AAS_RemoveFaceFromArea(face2, face2->frontarea);
					if (face2->backarea) AAS_RemoveFaceFromArea(face2, face2->backarea);
					AAS_FreeTmpFace(face2);
					return true;
				} //end if
			} //end if
			else if ((face1->planenum & ~1) == (face2->planenum & ~1))
			{
				Log_Write("face %d and %d, same front and back area but flipped planes\r\n",
							face1->num, face2->num);
			} //end if
		} //end if
	} //end if
	return false;
} //end of the function AAS_TryMergeFaces
/*
int AAS_TryMergeFaces(tmp_face_t *face1, tmp_face_t *face2)
{
	winding_t *neww;

#ifdef DEBUG
	if (!face1->winding) Error("face1 %d without winding", face1->num);
	if (!face2->winding) Error("face2 %d without winding", face2->num);
#endif //DEBUG
	//if the faces are in the same plane
	if ((face1->planenum & ~1) != (face2->planenum & ~1)) return false;
//	if (face1->planenum != face2->planenum) return false;
	//NOTE: if the front or back area is zero this doesn't mean there's
	//a real area. It means there's solid at that side of the face
	//if both faces have the same front area
	if (face1->frontarea != face2->frontarea ||
		face1->backarea != face2->backarea)
	{
		if (!face1->frontarea || !face1->backarea ||
				!face2->frontarea || !face2->backarea) return false;
		else if (face1->frontarea != face2->backarea ||
					face1->backarea != face2->frontarea) return false;
//		return false;
	} //end if
	//this function is to be found in l_poly.c
	neww = TryMergeWinding(face1->winding, face2->winding,
					mapplanes[face1->planenum].normal);
	if (!neww) return false;
	//
	FreeWinding(face1->winding);
	face1->winding = neww;
	//remove face2
	if (face2->frontarea)
		AAS_RemoveFaceFromArea(face2, &tmpaasworld.areas[face2->frontarea]);
	if (face2->backarea)
		AAS_RemoveFaceFromArea(face2, &tmpaasworld.areas[face2->backarea]);
	return true;
} //end of the function AAS_TryMergeFaces*/
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_MergeAreaFaces(void)
{
	int num_facemerges = 0;
	int side1, side2, restart;
	tmp_area_t *tmparea, *lasttmparea;
	tmp_face_t *face1, *face2;

	Log_Write("AAS_MergeAreaFaces\r\n");
	qprintf("%6d face merges", num_facemerges);
	//NOTE: first convex area is a dummy
	lasttmparea = tmpaasworld.areas;
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
		restart = false;
		//
		if (tmparea->invalid) continue;
		//
		for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
		{
			side1 = face1->frontarea != tmparea;
			for (face2 = face1->next[side1]; face2; face2 = face2->next[side2])
			{
				side2 = face2->frontarea != tmparea;
				//if succesfully merged
				if (AAS_TryMergeFaces(face1, face2))
				{
					//start over again after merging two faces
					restart = true;
					num_facemerges++;
					qprintf("\r%6d", num_facemerges);
					AAS_CheckArea(tmparea);
					break;
				} //end if
			} //end for
			if (restart)
			{
				tmparea = lasttmparea;
				break;
			} //end if
		} //end for
		lasttmparea = tmparea;
	} //end for
	qprintf("\n");
	Log_Write("%6d face merges\r\n", num_facemerges);
} //end of the function AAS_MergeAreaFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_MergePlaneFaces(tmp_area_t *tmparea, int planenum)
{
	tmp_face_t *face1, *face2, *nextface2;
	winding_t *neww;
	int side1, side2;

	for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
	{
		side1 = face1->frontarea != tmparea;
		if (face1->planenum != planenum) continue;
		//
		for (face2 = face1->next[side1]; face2; face2 = nextface2)
		{
			side2 = face2->frontarea != tmparea;
			nextface2 = face2->next[side2];
			//
			if ((face2->planenum & ~1) != (planenum & ~1)) continue;
			//
			neww = MergeWindings(face1->winding, face2->winding,
								mapplanes[face1->planenum].normal);
			FreeWinding(face1->winding);
			face1->winding = neww;
			if (face2->frontarea) AAS_RemoveFaceFromArea(face2, face2->frontarea);
			if (face2->backarea) AAS_RemoveFaceFromArea(face2, face2->backarea);
			AAS_FreeTmpFace(face2);
			//
			nextface2 = face1->next[side1];
		} //end for
	} //end for
} //end of the function AAS_MergePlaneFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_CanMergePlaneFaces(tmp_area_t *tmparea, int planenum)
{
	tmp_area_t *frontarea, *backarea;
	tmp_face_t *face1;
	int side1, merge, faceflags;

	frontarea = backarea = NULL;
	merge = false;
	for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
	{
		side1 = face1->frontarea != tmparea;
		if ((face1->planenum & ~1) != (planenum & ~1)) continue;
		if (!frontarea && !backarea)
		{
			frontarea = face1->frontarea;
			backarea = face1->backarea;
			faceflags = face1->faceflags;
		} //end if
		else
		{
			if (frontarea != face1->frontarea) return false;
			if (backarea != face1->backarea) return false;
			if (faceflags != face1->faceflags) return false;
			merge = true;
		} //end else
	} //end for
	return merge;
} //end of the function AAS_CanMergePlaneFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_MergeAreaPlaneFaces(void)
{
	int num_facemerges = 0;
	int side1;
	tmp_area_t *tmparea, *nexttmparea;
	tmp_face_t *face1;

	Log_Write("AAS_MergePlaneFaces\r\n");
	qprintf("%6d plane face merges", num_facemerges);
	//NOTE: first convex area is a dummy
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = nexttmparea)
	{
		nexttmparea = tmparea->l_next;
		//
		if (tmparea->invalid) continue;
		//
		for (face1 = tmparea->tmpfaces; face1; face1 = face1->next[side1])
		{
			side1 = face1->frontarea != tmparea;
			//
			if (AAS_CanMergePlaneFaces(tmparea, face1->planenum))
			{
				AAS_MergePlaneFaces(tmparea, face1->planenum);
				nexttmparea = tmparea;
				num_facemerges++;
				qprintf("\r%6d", num_facemerges);
				break;
			} //end if
		} //end for
	} //end for
	qprintf("\n");
	Log_Write("%6d plane face merges\r\n", num_facemerges);
} //end of the function AAS_MergeAreaPlaneFaces
