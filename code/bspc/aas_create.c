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
#include "aas_gsubdiv.h"
#include "aas_facemerging.h"
#include "aas_areamerging.h"
#include "aas_edgemelting.h"
#include "aas_prunenodes.h"
#include "aas_cfg.h"
#include "../game/surfaceflags.h"

//#define AW_DEBUG
//#define L_DEBUG

#define AREAONFACESIDE(face, area)		(face->frontarea != area)

tmp_aas_t tmpaasworld;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitTmpAAS(void)
{
	//tmp faces
	tmpaasworld.numfaces = 0;
	tmpaasworld.facenum = 0;
	tmpaasworld.faces = NULL;
	//tmp convex areas
	tmpaasworld.numareas = 0;
	tmpaasworld.areanum = 0;
	tmpaasworld.areas = NULL;
	//tmp nodes
	tmpaasworld.numnodes = 0;
	tmpaasworld.nodes = NULL;
	//
	tmpaasworld.nodebuffer = NULL;
} //end of the function AAS_InitTmpAAS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeTmpAAS(void)
{
	tmp_face_t *f, *nextf;
	tmp_area_t *a, *nexta;
	tmp_nodebuf_t *nb, *nextnb;

	//free all the faces
	for (f = tmpaasworld.faces; f; f = nextf)
	{
		nextf = f->l_next;
		if (f->winding) FreeWinding(f->winding);
		FreeMemory(f);
	} //end if
	//free all tmp areas
	for (a = tmpaasworld.areas; a; a = nexta)
	{
		nexta = a->l_next;
		if (a->settings) FreeMemory(a->settings);
		FreeMemory(a);
	} //end for
	//free all the tmp nodes
	for (nb = tmpaasworld.nodebuffer; nb; nb = nextnb)
	{
		nextnb = nb->next;
		FreeMemory(nb);
	} //end for
} //end of the function AAS_FreeTmpAAS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_face_t *AAS_AllocTmpFace(void)
{
	tmp_face_t *tmpface;

	tmpface = (tmp_face_t *) GetClearedMemory(sizeof(tmp_face_t));
	tmpface->num = tmpaasworld.facenum++;
	tmpface->l_prev = NULL;
	tmpface->l_next = tmpaasworld.faces;
	if (tmpaasworld.faces) tmpaasworld.faces->l_prev = tmpface;
	tmpaasworld.faces = tmpface;
	tmpaasworld.numfaces++;
	return tmpface;
} //end of the function AAS_AllocTmpFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeTmpFace(tmp_face_t *tmpface)
{
	if (tmpface->l_next) tmpface->l_next->l_prev = tmpface->l_prev;
	if (tmpface->l_prev) tmpface->l_prev->l_next = tmpface->l_next;
	else tmpaasworld.faces = tmpface->l_next;
	//free the winding
	if (tmpface->winding) FreeWinding(tmpface->winding);
	//free the face
	FreeMemory(tmpface);
	tmpaasworld.numfaces--;
} //end of the function AAS_FreeTmpFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_area_t *AAS_AllocTmpArea(void)
{
	tmp_area_t *tmparea;

	tmparea = (tmp_area_t *) GetClearedMemory(sizeof(tmp_area_t));
	tmparea->areanum = tmpaasworld.areanum++;
	tmparea->l_prev = NULL;
	tmparea->l_next = tmpaasworld.areas;
	if (tmpaasworld.areas) tmpaasworld.areas->l_prev = tmparea;
	tmpaasworld.areas = tmparea;
	tmpaasworld.numareas++;
	return tmparea;
} //end of the function AAS_AllocTmpArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeTmpArea(tmp_area_t *tmparea)
{
	if (tmparea->l_next) tmparea->l_next->l_prev = tmparea->l_prev;
	if (tmparea->l_prev) tmparea->l_prev->l_next = tmparea->l_next;
	else tmpaasworld.areas = tmparea->l_next;
	if (tmparea->settings) FreeMemory(tmparea->settings);
	FreeMemory(tmparea);
	tmpaasworld.numareas--;
} //end of the function AAS_FreeTmpArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_AllocTmpNode(void)
{
	tmp_nodebuf_t *nodebuf;

	if (!tmpaasworld.nodebuffer ||
			tmpaasworld.nodebuffer->numnodes >= NODEBUF_SIZE)
	{
		nodebuf = (tmp_nodebuf_t *) GetClearedMemory(sizeof(tmp_nodebuf_t));
		nodebuf->next = tmpaasworld.nodebuffer;
		nodebuf->numnodes = 0;
		tmpaasworld.nodebuffer = nodebuf;
	} //end if
	tmpaasworld.numnodes++;
	return &tmpaasworld.nodebuffer->nodes[tmpaasworld.nodebuffer->numnodes++];
} //end of the function AAS_AllocTmpNode
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeTmpNode(tmp_node_t *tmpnode)
{
	tmpaasworld.numnodes--;
} //end of the function AAS_FreeTmpNode
//===========================================================================
// returns true if the face is a gap from the given side
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_GapFace(tmp_face_t *tmpface, int side)
{
	vec3_t invgravity;

	//if the face is a solid or ground face it can't be a gap
	if (tmpface->faceflags & (FACE_GROUND | FACE_SOLID)) return 0;

	VectorCopy(cfg.phys_gravitydirection, invgravity);
	VectorInverse(invgravity);

	return (DotProduct(invgravity, mapplanes[tmpface->planenum ^ side].normal) > cfg.phys_maxsteepness);
} //end of the function AAS_GapFace
//===========================================================================
// returns true if the face is a ground face
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_GroundFace(tmp_face_t *tmpface)
{
	vec3_t invgravity;

	//must be a solid face
	if (!(tmpface->faceflags & FACE_SOLID)) return 0;

	VectorCopy(cfg.phys_gravitydirection, invgravity);
	VectorInverse(invgravity);

	return (DotProduct(invgravity, mapplanes[tmpface->planenum].normal) > cfg.phys_maxsteepness);
} //end of the function AAS_GroundFace
//===========================================================================
// adds the side of a face to an area
//
// side :	0 = front side
//				1 = back side
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AddFaceSideToArea(tmp_face_t *tmpface, int side, tmp_area_t *tmparea)
{
	int tmpfaceside;

	if (side)
	{
		if (tmpface->backarea) Error("AAS_AddFaceSideToArea: already a back area\n");
	} //end if
	else
	{
		if (tmpface->frontarea) Error("AAS_AddFaceSideToArea: already a front area\n");
	} //end else

	if (side) tmpface->backarea = tmparea;
	else tmpface->frontarea = tmparea;

	if (tmparea->tmpfaces)
	{
		tmpfaceside = tmparea->tmpfaces->frontarea != tmparea;
		tmparea->tmpfaces->prev[tmpfaceside] = tmpface;
	} //end if
	tmpface->next[side] = tmparea->tmpfaces;
	tmpface->prev[side] = NULL;
	tmparea->tmpfaces = tmpface;
} //end of the function AAS_AddFaceSideToArea
//===========================================================================
// remove (a side of) a face from an area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveFaceFromArea(tmp_face_t *tmpface, tmp_area_t *tmparea)
{
	int side, prevside, nextside;

	if (tmpface->frontarea != tmparea &&
			tmpface->backarea != tmparea)
	{
		Error("AAS_RemoveFaceFromArea: face not part of the area");
	} //end if
	side = tmpface->frontarea != tmparea;
	if (tmpface->prev[side])
	{
		prevside = tmpface->prev[side]->frontarea != tmparea;
		tmpface->prev[side]->next[prevside] = tmpface->next[side];
	} //end if
	else
	{
		tmparea->tmpfaces = tmpface->next[side];
	} //end else
	if (tmpface->next[side])
	{
		nextside = tmpface->next[side]->frontarea != tmparea;
		tmpface->next[side]->prev[nextside] = tmpface->prev[side];
	} //end if
	//remove the area number from the face depending on the side
	if (side) tmpface->backarea = NULL;
	else tmpface->frontarea = NULL;
	tmpface->prev[side] = NULL;
	tmpface->next[side] = NULL;
} //end of the function AAS_RemoveFaceFromArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CheckArea(tmp_area_t *tmparea)
{
	int side;
	tmp_face_t *face;
	plane_t *plane;
	vec3_t wcenter, acenter = {0, 0, 0};
	vec3_t normal;
	float n, dist;

	if (tmparea->invalid) Log_Print("AAS_CheckArea: invalid area\n");
	for (n = 0, face = tmparea->tmpfaces; face; face = face->next[side])
	{
		//side of the face the area is on
		side = face->frontarea != tmparea;
		WindingCenter(face->winding, wcenter);
		VectorAdd(acenter, wcenter, acenter);
		n++;
	} //end for
	n = 1 / n;
	VectorScale(acenter, n, acenter);
	for (face = tmparea->tmpfaces; face; face = face->next[side])
	{
		//side of the face the area is on
		side = face->frontarea != tmparea;

#ifdef L_DEBUG
		if (WindingError(face->winding))
		{
			Log_Write("AAS_CheckArea: area %d face %d: %s\r\n", tmparea->areanum,
						face->num, WindingErrorString());
		} //end if
#endif L_DEBUG

		plane = &mapplanes[face->planenum ^ side];

		if (DotProduct(plane->normal, acenter) - plane->dist < 0)
		{
			Log_Print("AAS_CheckArea: area %d face %d is flipped\n", tmparea->areanum, face->num);
			Log_Print("AAS_CheckArea: area %d center is %f %f %f\n", tmparea->areanum, acenter[0], acenter[1], acenter[2]);
		} //end if
		//check if the winding plane is the same as the face plane
		WindingPlane(face->winding, normal, &dist);
		plane = &mapplanes[face->planenum];
#ifdef L_DEBUG
		if (fabs(dist - plane->dist) > 0.4 ||
				fabs(normal[0] - plane->normal[0]) > 0.0001 ||
				fabs(normal[1] - plane->normal[1]) > 0.0001 ||
				fabs(normal[2] - plane->normal[2]) > 0.0001)
		{
			Log_Write("AAS_CheckArea: area %d face %d winding plane unequal to face plane\r\n",
										tmparea->areanum, face->num);
		} //end if
#endif L_DEBUG
	} //end for
} //end of the function AAS_CheckArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CheckFaceWindingPlane(tmp_face_t *face)
{
	float dist, sign1, sign2;
	vec3_t normal;
	plane_t *plane;
	winding_t *w;

	//check if the winding plane is the same as the face plane
	WindingPlane(face->winding, normal, &dist);
	plane = &mapplanes[face->planenum];
	//
	sign1 = DotProduct(plane->normal, normal);
	//
	if (fabs(dist - plane->dist) > 0.4 ||
			fabs(normal[0] - plane->normal[0]) > 0.0001 ||
			fabs(normal[1] - plane->normal[1]) > 0.0001 ||
			fabs(normal[2] - plane->normal[2]) > 0.0001)
	{
		VectorInverse(normal);
		dist = -dist;
		if (fabs(dist - plane->dist) > 0.4 ||
				fabs(normal[0] - plane->normal[0]) > 0.0001 ||
				fabs(normal[1] - plane->normal[1]) > 0.0001 ||
				fabs(normal[2] - plane->normal[2]) > 0.0001)
		{
			Log_Write("AAS_CheckFaceWindingPlane: face %d winding plane unequal to face plane\r\n",
									face->num);
			//
			sign2 = DotProduct(plane->normal, normal);
			if ((sign1 < 0 && sign2 > 0) ||
					(sign1 > 0 && sign2 < 0))
			{
				Log_Write("AAS_CheckFaceWindingPlane: face %d winding reversed\r\n",
									face->num);
				w = face->winding;
				face->winding = ReverseWinding(w);
				FreeWinding(w);
			} //end if
		} //end if
		else
		{
			Log_Write("AAS_CheckFaceWindingPlane: face %d winding reversed\r\n",
									face->num);
			w = face->winding;
			face->winding = ReverseWinding(w);
			FreeWinding(w);
		} //end else
	} //end if
} //end of the function AAS_CheckFaceWindingPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CheckAreaWindingPlanes(void)
{
	int side;
	tmp_area_t *tmparea;
	tmp_face_t *face;

	Log_Write("AAS_CheckAreaWindingPlanes:\r\n");
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
		if (tmparea->invalid) continue;
		for (face = tmparea->tmpfaces; face; face = face->next[side])
		{
			side = face->frontarea != tmparea;
			AAS_CheckFaceWindingPlane(face);
		} //end for
	} //end for
} //end of the function AAS_CheckAreaWindingPlanes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FlipAreaFaces(tmp_area_t *tmparea)
{
	int side;
	tmp_face_t *face;
	plane_t *plane;
	vec3_t wcenter, acenter = {0, 0, 0};
	//winding_t *w;
	float n;

	for (n = 0, face = tmparea->tmpfaces; face; face = face->next[side])
	{
		if (!face->frontarea) Error("face %d has no front area\n", face->num);
		//side of the face the area is on
		side = face->frontarea != tmparea;
		WindingCenter(face->winding, wcenter);
		VectorAdd(acenter, wcenter, acenter);
		n++;
	} //end for
	n = 1 / n;
	VectorScale(acenter, n, acenter);
	for (face = tmparea->tmpfaces; face; face = face->next[side])
	{
		//side of the face the area is on
		side = face->frontarea != tmparea;

		plane = &mapplanes[face->planenum ^ side];

		if (DotProduct(plane->normal, acenter) - plane->dist < 0)
		{
			Log_Print("area %d face %d flipped: front area %d, back area %d\n", tmparea->areanum, face->num,
					face->frontarea ? face->frontarea->areanum : 0,
					face->backarea ? face->backarea->areanum : 0);
			/*
			face->planenum = face->planenum ^ 1;
			w = face->winding;
			face->winding = ReverseWinding(w);
			FreeWinding(w);
			*/
		} //end if
#ifdef L_DEBUG
		{
			float dist;
			vec3_t normal;

			//check if the winding plane is the same as the face plane
			WindingPlane(face->winding, normal, &dist);
			plane = &mapplanes[face->planenum];
			if (fabs(dist - plane->dist) > 0.4 ||
					fabs(normal[0] - plane->normal[0]) > 0.0001 ||
					fabs(normal[1] - plane->normal[1]) > 0.0001 ||
					fabs(normal[2] - plane->normal[2]) > 0.0001)
			{
				Log_Write("area %d face %d winding plane unequal to face plane\r\n",
											tmparea->areanum, face->num);
			} //end if
		}
#endif
	} //end for
} //end of the function AAS_FlipAreaFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveAreaFaceColinearPoints(void)
{
	int side;
	tmp_face_t *face;
	tmp_area_t *tmparea;

	//FIXME: loop over the faces instead of area->faces
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
		for (face = tmparea->tmpfaces; face; face = face->next[side])
		{
			side = face->frontarea != tmparea;
			RemoveColinearPoints(face->winding);
//			RemoveEqualPoints(face->winding, 0.1);
		} //end for
	} //end for
} //end of the function AAS_RemoveAreaFaceColinearPoints
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveTinyFaces(void)
{
	int side, num;
	tmp_face_t *face, *nextface;
	tmp_area_t *tmparea;

	//FIXME: loop over the faces instead of area->faces
	Log_Write("AAS_RemoveTinyFaces\r\n");
	num = 0;
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
		for (face = tmparea->tmpfaces; face; face = nextface)
		{
			side = face->frontarea != tmparea;
			nextface = face->next[side];
			//
			if (WindingArea(face->winding) < 1)
			{
				if (face->frontarea) AAS_RemoveFaceFromArea(face, face->frontarea);
				if (face->backarea) AAS_RemoveFaceFromArea(face, face->backarea);
				AAS_FreeTmpFace(face);
				//Log_Write("area %d face %d is tiny\r\n", tmparea->areanum, face->num);
				num++;
			} //end if
		} //end for
	} //end for
	Log_Write("%d tiny faces removed\r\n", num);
} //end of the function AAS_RemoveTinyFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CreateAreaSettings(void)
{
	int i, flags, side, numgrounded, numladderareas, numliquidareas;
	tmp_face_t *face;
	tmp_area_t *tmparea;

	numgrounded = 0;
	numladderareas = 0;
	numliquidareas = 0;
	Log_Write("AAS_CreateAreaSettings\r\n");
	i = 0;
	qprintf("%6d areas provided with settings", i);
	for (tmparea = tmpaasworld.areas; tmparea; tmparea = tmparea->l_next)
	{
		//if the area is invalid there no need to create settings for it
		if (tmparea->invalid) continue;

		tmparea->settings = (tmp_areasettings_t *) GetClearedMemory(sizeof(tmp_areasettings_t));
		tmparea->settings->contents = tmparea->contents;
		tmparea->settings->modelnum = tmparea->modelnum;
		flags = 0;
		for (face = tmparea->tmpfaces; face; face = face->next[side])
		{
			side = face->frontarea != tmparea;
			flags |= face->faceflags;
		} //end for
		tmparea->settings->areaflags = 0;
		if (flags & FACE_GROUND)
		{
			tmparea->settings->areaflags |= AREA_GROUNDED;
			numgrounded++;
		} //end if
		if (flags & FACE_LADDER)
		{
			tmparea->settings->areaflags |= AREA_LADDER;
			numladderareas++;
		} //end if
		if (tmparea->contents & (AREACONTENTS_WATER |
											AREACONTENTS_SLIME |
											AREACONTENTS_LAVA))
		{
			tmparea->settings->areaflags |= AREA_LIQUID;
			numliquidareas++;
		} //end if
		//presence type of the area
		tmparea->settings->presencetype = tmparea->presencetype;
		//
		qprintf("\r%6d", ++i);
	} //end for
	qprintf("\n");
#ifdef AASINFO
	Log_Print("%6d grounded areas\n", numgrounded);
	Log_Print("%6d ladder areas\n", numladderareas);
	Log_Print("%6d liquid areas\n", numliquidareas);
#endif //AASINFO
} //end of the function AAS_CreateAreaSettings
//===========================================================================
// create a tmp AAS area from a leaf node
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_CreateArea(node_t *node)
{
	int pside;
	int areafaceflags;
	portal_t	*p;
	tmp_face_t *tmpface;
	tmp_area_t *tmparea;
	tmp_node_t *tmpnode;
	vec3_t up = {0, 0, 1};

	//create an area from this leaf
	tmparea = AAS_AllocTmpArea();
	tmparea->tmpfaces = NULL;
	//clear the area face flags
	areafaceflags = 0;
	//make aas faces from the portals
	for (p = node->portals; p; p = p->next[pside])
	{
		pside = (p->nodes[1] == node);
		//don't create faces from very small portals
//		if (WindingArea(p->winding) < 1) continue;
		//if there's already a face created for this portal
		if (p->tmpface)
		{
			//add the back side of the face to the area
			AAS_AddFaceSideToArea(p->tmpface, 1, tmparea);
		} //end if
		else
		{
			tmpface = AAS_AllocTmpFace();
			//set the face pointer at the portal so we can see from
			//the portal there's a face created for it
			p->tmpface = tmpface;
			//FIXME: test this change
			//tmpface->planenum = (p->planenum & ~1) | pside;
			tmpface->planenum = p->planenum ^ pside;
			if (pside) tmpface->winding = ReverseWinding(p->winding);
			else tmpface->winding = CopyWinding(p->winding);
#ifdef L_DEBUG
			//
			AAS_CheckFaceWindingPlane(tmpface);
#endif //L_DEBUG
			//if there's solid at the other side of the portal
			if (p->nodes[!pside]->contents & (CONTENTS_SOLID | CONTENTS_PLAYERCLIP))
			{
				tmpface->faceflags |= FACE_SOLID;
			} //end if
			//else there is no solid at the other side and if there
			//is a liquid at this side
			else if (node->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA))
			{
				tmpface->faceflags |= FACE_LIQUID;
				//if there's no liquid at the other side
				if (!(p->nodes[!pside]->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA)))
				{
					tmpface->faceflags |= FACE_LIQUIDSURFACE;
				} //end if
			} //end else
			//if there's ladder contents at other side of the portal
			if ((p->nodes[pside]->contents & CONTENTS_LADDER) ||
					(p->nodes[!pside]->contents & CONTENTS_LADDER))
			{

				//NOTE: doesn't have to be solid at the other side because
				// when standing one can use a crouch area (which is not solid)
				// as a ladder
				// imagine a ladder one can walk underthrough,
				// under the ladder against the ladder is a crouch area
				// the (vertical) sides of this crouch area area also used as
				// ladder sides when standing (not crouched)
				tmpface->faceflags |= FACE_LADDER;
			} //end if
			//if it is possible to stand on the face
			if (AAS_GroundFace(tmpface))
			{
				tmpface->faceflags |= FACE_GROUND;
			} //end if
			//
			areafaceflags |= tmpface->faceflags;
			//no aas face number yet (zero is a dummy in the aasworld faces)
			tmpface->aasfacenum = 0;
			//add the front side of the face to the area
			AAS_AddFaceSideToArea(tmpface, 0, tmparea);
		} //end else
	} //end for
	qprintf("\r%6d", tmparea->areanum);
	//presence type in the area
	tmparea->presencetype = ~node->expansionbboxes & cfg.allpresencetypes;
	//
	tmparea->contents = 0;
	if (node->contents & CONTENTS_CLUSTERPORTAL) tmparea->contents |= AREACONTENTS_CLUSTERPORTAL;
	if (node->contents & CONTENTS_MOVER) tmparea->contents |= AREACONTENTS_MOVER;
	if (node->contents & CONTENTS_TELEPORTER) tmparea->contents |= AREACONTENTS_TELEPORTER;
	if (node->contents & CONTENTS_JUMPPAD) tmparea->contents |= AREACONTENTS_JUMPPAD;
	if (node->contents & CONTENTS_DONOTENTER) tmparea->contents |= AREACONTENTS_DONOTENTER;
	if (node->contents & CONTENTS_WATER) tmparea->contents |= AREACONTENTS_WATER;
	if (node->contents & CONTENTS_LAVA) tmparea->contents |= AREACONTENTS_LAVA;
	if (node->contents & CONTENTS_SLIME) tmparea->contents |= AREACONTENTS_SLIME;
	if (node->contents & CONTENTS_NOTTEAM1) tmparea->contents |= AREACONTENTS_NOTTEAM1;
	if (node->contents & CONTENTS_NOTTEAM2) tmparea->contents |= AREACONTENTS_NOTTEAM2;

	//store the bsp model that's inside this node
	tmparea->modelnum = node->modelnum;
	//sorta check for flipped area faces (remove??)
	AAS_FlipAreaFaces(tmparea);
	//check if the area is ok (remove??)
	AAS_CheckArea(tmparea);
	//
	tmpnode = AAS_AllocTmpNode();
	tmpnode->planenum = 0;
	tmpnode->children[0] = 0;
	tmpnode->children[1] = 0;
	tmpnode->tmparea = tmparea;
	//
	return tmpnode;
} //end of the function AAS_CreateArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_CreateAreas_r(node_t *node)
{
	tmp_node_t *tmpnode;

	//recurse down to leafs
	if (node->planenum != PLANENUM_LEAF)
	{
		//the first tmp node is a dummy
		tmpnode = AAS_AllocTmpNode();
		tmpnode->planenum = node->planenum;
		tmpnode->children[0] = AAS_CreateAreas_r(node->children[0]);
		tmpnode->children[1] = AAS_CreateAreas_r(node->children[1]);
		return tmpnode;
	} //end if
	//areas won't be created for solid leafs
	if (node->contents & CONTENTS_SOLID)
	{
		//just return zero for a solid leaf (in tmp AAS NULL is a solid leaf)
		return NULL;
	} //end if

	return AAS_CreateArea(node);
} //end of the function AAS_CreateAreas_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CreateAreas(node_t *node)
{
	Log_Write("AAS_CreateAreas\r\n");
	qprintf("%6d areas created", 0);
	tmpaasworld.nodes = AAS_CreateAreas_r(node);
	qprintf("\n");
	Log_Write("%6d areas created\r\n", tmpaasworld.numareas);
} //end of the function AAS_CreateAreas
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_PrintNumGroundFaces(void)
{
	tmp_face_t *tmpface;
	int numgroundfaces = 0;

	for (tmpface = tmpaasworld.faces; tmpface; tmpface = tmpface->l_next)
	{
		if (tmpface->faceflags & FACE_GROUND)
		{
			numgroundfaces++;
		} //end if
	} //end for
	qprintf("%6d ground faces\n", numgroundfaces);
} //end of the function AAS_PrintNumGroundFaces
//===========================================================================
// checks the number of shared faces between the given two areas
// since areas are convex they should only have ONE shared face
// however due to crappy face merging there are sometimes several
// shared faces
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CheckAreaSharedFaces(tmp_area_t *tmparea1, tmp_area_t *tmparea2)
{
	int numsharedfaces, side;
	tmp_face_t *face1, *sharedface;

	if (tmparea1->invalid || tmparea2->invalid) return;

	sharedface = NULL;
	numsharedfaces = 0;
	for (face1 = tmparea1->tmpfaces; face1; face1 = face1->next[side])
	{
		side = face1->frontarea != tmparea1;
		if (face1->backarea == tmparea2 || face1->frontarea == tmparea2)
		{
			sharedface = face1;
			numsharedfaces++;
		} //end if
	} //end if
	if (!sharedface) return;
	//the areas should only have one shared face
	if (numsharedfaces > 1)
	{
		Log_Write("---- tmp area %d and %d have %d shared faces\r\n",
									tmparea1->areanum, tmparea2->areanum, numsharedfaces);
		for (face1 = tmparea1->tmpfaces; face1; face1 = face1->next[side])
		{
			side = face1->frontarea != tmparea1;
			if (face1->backarea == tmparea2 || face1->frontarea == tmparea2)
			{
				Log_Write("face %d, planenum = %d, face->frontarea = %d face->backarea = %d\r\n",
								face1->num, face1->planenum, face1->frontarea->areanum, face1->backarea->areanum);
			} //end if
		} //end if
	} //end if
} //end of the function AAS_CheckAreaSharedFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CheckSharedFaces(void)
{
	tmp_area_t *tmparea1, *tmparea2;

	for (tmparea1 = tmpaasworld.areas; tmparea1; tmparea1 = tmparea1->l_next)
	{
		for (tmparea2 = tmpaasworld.areas; tmparea2; tmparea2 = tmparea2->l_next)
		{
			if (tmparea1 == tmparea2) continue;
			AAS_CheckAreaSharedFaces(tmparea1, tmparea2);
		} //end for
	} //end for
} //end of the function AAS_CheckSharedFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FlipFace(tmp_face_t *face)
{
	tmp_area_t *frontarea, *backarea;
	winding_t *w;

	frontarea = face->frontarea;
	backarea = face->backarea;
	//must have an area at both sides before flipping is allowed
	if (!frontarea || !backarea) return;
	//flip the face winding
	w = face->winding;
	face->winding = ReverseWinding(w);
	FreeWinding(w);
	//flip the face plane
	face->planenum ^= 1;
	//flip the face areas
	AAS_RemoveFaceFromArea(face, frontarea);
	AAS_RemoveFaceFromArea(face, backarea);
	AAS_AddFaceSideToArea(face, 1, frontarea);
	AAS_AddFaceSideToArea(face, 0, backarea);
} //end of the function AAS_FlipFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
/*
void AAS_FlipAreaSharedFaces(tmp_area_t *tmparea1, tmp_area_t *tmparea2)
{
	int numsharedfaces, side, area1facing, area2facing;
	tmp_face_t *face1, *sharedface;

	if (tmparea1->invalid || tmparea2->invalid) return;

	sharedface = NULL;
	numsharedfaces = 0;
	area1facing = 0;		//number of shared faces facing towards area 1
	area2facing = 0;		//number of shared faces facing towards area 2
	for (face1 = tmparea1->tmpfaces; face1; face1 = face1->next[side])
	{
		side = face1->frontarea != tmparea1;
		if (face1->backarea == tmparea2 || face1->frontarea == tmparea2)
		{
			sharedface = face1;
			numsharedfaces++;
			if (face1->frontarea == tmparea1) area1facing++;
			else area2facing++;
		} //end if
	} //end if
	if (!sharedface) return;
	//if there's only one shared face
	if (numsharedfaces <= 1) return;
	//if all the shared faces are facing to the same area
	if (numsharedfaces == area1facing || numsharedfaces == area2facing) return;
	//
	do
	{
		for (face1 = tmparea1->tmpfaces; face1; face1 = face1->next[side])
		{
			side = face1->frontarea != tmparea1;
			if (face1->backarea == tmparea2 || face1->frontarea == tmparea2)
			{
				if (face1->frontarea != tmparea1)
				{
					AAS_FlipFace(face1);
					break;
				} //end if
			} //end if
		} //end for
	} while(face1);
} //end of the function AAS_FlipAreaSharedFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FlipSharedFaces(void)
{
	int i;
	tmp_area_t *tmparea1, *tmparea2;

	i = 0;
	qprintf("%6d areas checked for shared face flipping", i);
	for (tmparea1 = tmpaasworld.areas; tmparea1; tmparea1 = tmparea1->l_next)
	{
		if (tmparea1->invalid) continue;
		for (tmparea2 = tmpaasworld.areas; tmparea2; tmparea2 = tmparea2->l_next)
		{
			if (tmparea2->invalid) continue;
			if (tmparea1 == tmparea2) continue;
			AAS_FlipAreaSharedFaces(tmparea1, tmparea2);
		} //end for
		qprintf("\r%6d", ++i);
	} //end for
	Log_Print("\r%6d areas checked for shared face flipping\n", i);
} //end of the function AAS_FlipSharedFaces
*/
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FlipSharedFaces(void)
{
	int i, side1, side2;
	tmp_area_t *tmparea1;
	tmp_face_t *face1, *face2;

	i = 0;
	qprintf("%6d areas checked for shared face flipping", i);
	for (tmparea1 = tmpaasworld.areas; tmparea1; tmparea1 = tmparea1->l_next)
	{
		if (tmparea1->invalid) continue;
		for (face1 = tmparea1->tmpfaces; face1; face1 = face1->next[side1])
		{
			side1 = face1->frontarea != tmparea1;
			if (!face1->frontarea || !face1->backarea) continue;
			//
			for (face2 = face1->next[side1]; face2; face2 = face2->next[side2])
			{
				side2 = face2->frontarea != tmparea1;
				if (!face2->frontarea || !face2->backarea) continue;
				//
				if (face1->frontarea == face2->backarea &&
					face1->backarea == face2->frontarea)
				{
					AAS_FlipFace(face2);
				} //end if
				//recheck side
				side2 = face2->frontarea != tmparea1;
			} //end for
		} //end for
		qprintf("\r%6d", ++i);
	} //end for
	qprintf("\n");
	Log_Write("%6d areas checked for shared face flipping\r\n", i);
} //end of the function AAS_FlipSharedFaces
//===========================================================================
// creates an .AAS file with the given name
// a MAP should be loaded before calling this
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Create(char *aasfile)
{
	entity_t	*e;
	tree_t *tree;
	double start_time;

	//for a possible leak file
	strcpy(source, aasfile);
	StripExtension(source);
	//the time started
	start_time = I_FloatTime();
	//set the default number of threads (depends on number of processors)
	ThreadSetDefault();
	//set the global entity number to the world model
	entity_num = 0;
	//the world entity
	e = &entities[entity_num];
	//process the whole world
	tree = ProcessWorldBrushes(e->firstbrush, e->firstbrush + e->numbrushes);
	//if the conversion is cancelled
	if (cancelconversion)
	{
		Tree_Free(tree);
		return;
	} //end if
	//display BSP tree creation time
	Log_Print("BSP tree created in %5.0f seconds\n", I_FloatTime() - start_time);
	//prune the bsp tree
	Tree_PruneNodes(tree->headnode);
	//if the conversion is cancelled
	if (cancelconversion)
	{
		Tree_Free(tree);
		return;
	} //end if
	//create the tree portals
	MakeTreePortals(tree);
	//if the conversion is cancelled
	if (cancelconversion)
	{
		Tree_Free(tree);
		return;
	} //end if
	//Marks all nodes that can be reached by entites
	if (FloodEntities(tree))
	{
		//fill out nodes that can't be reached
		FillOutside(tree->headnode);
	} //end if
	else
	{
		LeakFile(tree);
		Error("**** leaked ****\n");
		return;
	} //end else
	//create AAS from the BSP tree
	//==========================================
	//initialize tmp aas
	AAS_InitTmpAAS();
	//create the convex areas from the leaves
	AAS_CreateAreas(tree->headnode);
	//free the BSP tree because it isn't used anymore
	if (freetree) Tree_Free(tree);
	//try to merge area faces
	AAS_MergeAreaFaces();
	//do gravitational subdivision
	AAS_GravitationalSubdivision();
	//merge faces if possible
	AAS_MergeAreaFaces();
	AAS_RemoveAreaFaceColinearPoints();
	//merge areas if possible
	AAS_MergeAreas();
	//NOTE: prune nodes directly after area merging
	AAS_PruneNodes();
	//flip shared faces so they are all facing to the same area
	AAS_FlipSharedFaces();
	AAS_RemoveAreaFaceColinearPoints();
	//merge faces if possible
	AAS_MergeAreaFaces();
	//merge area faces in the same plane
	AAS_MergeAreaPlaneFaces();
	//do ladder subdivision
	AAS_LadderSubdivision();
	//FIXME: melting is buggy
	AAS_MeltAreaFaceWindings();
	//remove tiny faces
	AAS_RemoveTinyFaces();
	//create area settings
	AAS_CreateAreaSettings();
	//check if the winding plane is equal to the face plane
	//AAS_CheckAreaWindingPlanes();
	//
	//AAS_CheckSharedFaces();
	//==========================================
	//if the conversion is cancelled
	if (cancelconversion)
	{
		Tree_Free(tree);
		AAS_FreeTmpAAS();
		return;
	} //end if
	//store the created AAS stuff in the AAS file format and write the file
	AAS_StoreFile(aasfile);
	//free the temporary AAS memory
	AAS_FreeTmpAAS();
	//display creation time
	Log_Print("\nAAS created in %5.0f seconds\n", I_FloatTime() - start_time);
} //end of the function AAS_Create
