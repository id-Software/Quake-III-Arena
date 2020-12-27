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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		be_aas_sample.c
 *
 * desc:		AAS environment sampling
 *
 * $Archive: /MissionPack/code/botlib/be_aas_sample.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#ifndef BSPC
#include "l_libvar.h"
#endif
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_interface.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"


//#define AAS_SAMPLE_DEBUG

#define BBOX_NORMAL_EPSILON		0.001

#define ON_EPSILON					0 //0.0005

#define TRACEPLANE_EPSILON			0.125

typedef struct aas_tracestack_s
{
	vec3_t start;		//start point of the piece of line to trace
	vec3_t end;			//end point of the piece of line to trace
	int planenum;		//last plane used as splitter
	int nodenum;		//node found after splitting with planenum
} aas_tracestack_t;

int numaaslinks;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs)
{
	int index;
	//bounding box size for each presence type
	vec3_t boxmins[3] = {{0, 0, 0}, {-15, -15, -24}, {-15, -15, -24}};
	vec3_t boxmaxs[3] = {{0, 0, 0}, { 15,  15,  32}, { 15,  15,   8}};

	if (presencetype == PRESENCE_NORMAL) index = 1;
	else if (presencetype == PRESENCE_CROUCH) index = 2;
	else
	{
		botimport.Print(PRT_FATAL, "AAS_PresenceTypeBoundingBox: unknown presence type\n");
		index = 2;
	} //end if
	VectorCopy(boxmins[index], mins);
	VectorCopy(boxmaxs[index], maxs);
} //end of the function AAS_PresenceTypeBoundingBox
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitAASLinkHeap(void)
{
	int i, max_aaslinks;

	max_aaslinks = aasworld.linkheapsize;
	//if there's no link heap present
	if (!aasworld.linkheap)
	{
#ifdef BSPC
		max_aaslinks = 6144;
#else
		max_aaslinks = (int) LibVarValue("max_aaslinks", "6144");
#endif
		if (max_aaslinks < 0) max_aaslinks = 0;
		aasworld.linkheapsize = max_aaslinks;
		aasworld.linkheap = (aas_link_t *) GetHunkMemory(max_aaslinks * sizeof(aas_link_t));
	} //end if
	//link the links on the heap
	aasworld.linkheap[0].prev_ent = NULL;
	aasworld.linkheap[0].next_ent = &aasworld.linkheap[1];
	for (i = 1; i < max_aaslinks-1; i++)
	{
		aasworld.linkheap[i].prev_ent = &aasworld.linkheap[i - 1];
		aasworld.linkheap[i].next_ent = &aasworld.linkheap[i + 1];
	} //end for
	aasworld.linkheap[max_aaslinks-1].prev_ent = &aasworld.linkheap[max_aaslinks-2];
	aasworld.linkheap[max_aaslinks-1].next_ent = NULL;
	//pointer to the first free link
	aasworld.freelinks = &aasworld.linkheap[0];
	//
	numaaslinks = max_aaslinks;
} //end of the function AAS_InitAASLinkHeap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeAASLinkHeap(void)
{
	if (aasworld.linkheap) FreeMemory(aasworld.linkheap);
	aasworld.linkheap = NULL;
	aasworld.linkheapsize = 0;
} //end of the function AAS_FreeAASLinkHeap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_link_t *AAS_AllocAASLink(void)
{
	aas_link_t *link;

	link = aasworld.freelinks;
	if (!link)
	{
#ifndef BSPC
		if (botDeveloper)
#endif
		{
			botimport.Print(PRT_FATAL, "empty aas link heap\n");
		} //end if
		return NULL;
	} //end if
	if (aasworld.freelinks) aasworld.freelinks = aasworld.freelinks->next_ent;
	if (aasworld.freelinks) aasworld.freelinks->prev_ent = NULL;
	numaaslinks--;
	return link;
} //end of the function AAS_AllocAASLink
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_DeAllocAASLink(aas_link_t *link)
{
	if (aasworld.freelinks) aasworld.freelinks->prev_ent = link;
	link->prev_ent = NULL;
	link->next_ent = aasworld.freelinks;
	link->prev_area = NULL;
	link->next_area = NULL;
	aasworld.freelinks = link;
	numaaslinks++;
} //end of the function AAS_DeAllocAASLink
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitAASLinkedEntities(void)
{
	if (!aasworld.loaded) return;
	if (aasworld.arealinkedentities) FreeMemory(aasworld.arealinkedentities);
	aasworld.arealinkedentities = (aas_link_t **) GetClearedHunkMemory(
						aasworld.numareas * sizeof(aas_link_t *));
} //end of the function AAS_InitAASLinkedEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeAASLinkedEntities(void)
{
	if (aasworld.arealinkedentities) FreeMemory(aasworld.arealinkedentities);
	aasworld.arealinkedentities = NULL;
} //end of the function AAS_InitAASLinkedEntities
//===========================================================================
// returns the AAS area the point is in
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PointAreaNum(vec3_t point)
{
	int nodenum;
	vec_t	dist;
	aas_node_t *node;
	aas_plane_t *plane;

	if (!aasworld.loaded)
	{
		botimport.Print(PRT_ERROR, "AAS_PointAreaNum: aas not loaded\n");
		return 0;
	} //end if

	//start with node 1 because node zero is a dummy used for solid leafs
	nodenum = 1;
	while (nodenum > 0)
	{
//		botimport.Print(PRT_MESSAGE, "[%d]", nodenum);
#ifdef AAS_SAMPLE_DEBUG
		if (nodenum >= aasworld.numnodes)
		{
			botimport.Print(PRT_ERROR, "nodenum = %d >= aasworld.numnodes = %d\n", nodenum, aasworld.numnodes);
			return 0;
		} //end if
#endif //AAS_SAMPLE_DEBUG
		node = &aasworld.nodes[nodenum];
#ifdef AAS_SAMPLE_DEBUG
		if (node->planenum < 0 || node->planenum >= aasworld.numplanes)
		{
			botimport.Print(PRT_ERROR, "node->planenum = %d >= aasworld.numplanes = %d\n", node->planenum, aasworld.numplanes);
			return 0;
		} //end if
#endif //AAS_SAMPLE_DEBUG
		plane = &aasworld.planes[node->planenum];
		dist = DotProduct(point, plane->normal) - plane->dist;
		if (dist > 0) nodenum = node->children[0];
		else nodenum = node->children[1];
	} //end while
	if (!nodenum)
	{
#ifdef AAS_SAMPLE_DEBUG
		botimport.Print(PRT_MESSAGE, "in solid\n");
#endif //AAS_SAMPLE_DEBUG
		return 0;
	} //end if
	return -nodenum;
} //end of the function AAS_PointAreaNum
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_PointReachabilityAreaIndex( vec3_t origin )
{
	int areanum, cluster, i, index;

	if (!aasworld.initialized)
		return 0;

	if ( !origin )
	{
		index = 0;
		for (i = 0; i < aasworld.numclusters; i++)
		{
			index += aasworld.clusters[i].numreachabilityareas;
		} //end for
		return index;
	} //end if

	areanum = AAS_PointAreaNum( origin );
	if ( !areanum || !AAS_AreaReachability(areanum) )
		return 0;
	cluster = aasworld.areasettings[areanum].cluster;
	areanum = aasworld.areasettings[areanum].clusterareanum;
	if (cluster < 0)
	{
		cluster = aasworld.portals[-cluster].frontcluster;
		areanum = aasworld.portals[-cluster].clusterareanum[0];
	} //end if

	index = 0;
	for (i = 0; i < cluster; i++)
	{
		index += aasworld.clusters[i].numreachabilityareas;
	} //end for
	index += areanum;
	return index;
} //end of the function AAS_PointReachabilityAreaIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaCluster(int areanum)
{
	if (areanum <= 0 || areanum >= aasworld.numareas)
	{
		botimport.Print(PRT_ERROR, "AAS_AreaCluster: invalid area number\n");
		return 0;
	} //end if
	return aasworld.areasettings[areanum].cluster;
} //end of the function AAS_AreaCluster
//===========================================================================
// returns the presence types of the given area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaPresenceType(int areanum)
{
	if (!aasworld.loaded) return 0;
	if (areanum <= 0 || areanum >= aasworld.numareas)
	{
		botimport.Print(PRT_ERROR, "AAS_AreaPresenceType: invalid area number\n");
		return 0;
	} //end if
	return aasworld.areasettings[areanum].presencetype;
} //end of the function AAS_AreaPresenceType
//===========================================================================
// returns the presence type at the given point
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PointPresenceType(vec3_t point)
{
	int areanum;

	if (!aasworld.loaded) return 0;

	areanum = AAS_PointAreaNum(point);
	if (!areanum) return PRESENCE_NONE;
	return aasworld.areasettings[areanum].presencetype;
} //end of the function AAS_PointPresenceType
//===========================================================================
// calculates the minimum distance between the origin of the box and the
// given plane when both will collide on the given side of the plane
//
// normal	=	normal vector of plane to calculate distance from
// mins		=	minimums of box relative to origin
// maxs		=	maximums of box relative to origin
// side		=	side of the plane we want to calculate the distance from
//					0 normal vector side
//					1 not normal vector side
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
vec_t AAS_BoxOriginDistanceFromPlane(vec3_t normal, vec3_t mins, vec3_t maxs, int side)
{
	vec3_t v1, v2;
	int i;

	//swap maxs and mins when on the other side of the plane
	if (side)
	{
		//get a point of the box that would be one of the first
		//to collide with the plane
		for (i = 0; i < 3; i++)
		{
			if (normal[i] > BBOX_NORMAL_EPSILON) v1[i] = maxs[i];
			else if (normal[i] < -BBOX_NORMAL_EPSILON) v1[i] = mins[i];
			else v1[i] = 0;
		} //end for
	} //end if
	else
	{
		//get a point of the box that would be one of the first
		//to collide with the plane
		for (i = 0; i < 3; i++)
		{
			if (normal[i] > BBOX_NORMAL_EPSILON) v1[i] = mins[i];
			else if (normal[i] < -BBOX_NORMAL_EPSILON) v1[i] = maxs[i];
			else v1[i] = 0;
		} //end for
	} //end else
	//
	VectorCopy(normal, v2);
	VectorInverse(v2);
//	VectorNegate(normal, v2);
	return DotProduct(v1, v2);
} //end of the function AAS_BoxOriginDistanceFromPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_AreaEntityCollision(int areanum, vec3_t start, vec3_t end,
										int presencetype, int passent, aas_trace_t *trace)
{
	int collision;
	vec3_t boxmins, boxmaxs;
	aas_link_t *link;
	bsp_trace_t bsptrace;

	AAS_PresenceTypeBoundingBox(presencetype, boxmins, boxmaxs);

	Com_Memset(&bsptrace, 0, sizeof(bsp_trace_t)); //make compiler happy
	//assume no collision
	bsptrace.fraction = 1;
	collision = qfalse;
	for (link = aasworld.arealinkedentities[areanum]; link; link = link->next_ent)
	{
		//ignore the pass entity
		if (link->entnum == passent) continue;
		//
		if (AAS_EntityCollision(link->entnum, start, boxmins, boxmaxs, end,
												CONTENTS_SOLID|CONTENTS_PLAYERCLIP, &bsptrace))
		{
			collision = qtrue;
		} //end if
	} //end for
	if (collision)
	{
		trace->startsolid = bsptrace.startsolid;
		trace->ent = bsptrace.ent;
		VectorCopy(bsptrace.endpos, trace->endpos);
		trace->area = 0;
		trace->planenum = 0;
		return qtrue;
	} //end if
	return qfalse;
} //end of the function AAS_AreaEntityCollision
//===========================================================================
// recursive subdivision of the line by the BSP tree.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_trace_t AAS_TraceClientBBox(vec3_t start, vec3_t end, int presencetype,
																				int passent)
{
	int side, nodenum, tmpplanenum;
	float front, back, frac;
	vec3_t cur_start, cur_end, cur_mid, v1, v2;
	aas_tracestack_t tracestack[127];
	aas_tracestack_t *tstack_p;
	aas_node_t *aasnode;
	aas_plane_t *plane;
	aas_trace_t trace;

	//clear the trace structure
	Com_Memset(&trace, 0, sizeof(aas_trace_t));

	if (!aasworld.loaded) return trace;
	
	tstack_p = tracestack;
	//we start with the whole line on the stack
	VectorCopy(start, tstack_p->start);
	VectorCopy(end, tstack_p->end);
	tstack_p->planenum = 0;
	//start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1;		//starting at the root of the tree
	tstack_p++;
	
	while (1)
	{
		//pop up the stack
		tstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if (tstack_p < tracestack)
		{
			tstack_p++;
			//nothing was hit
			trace.startsolid = qfalse;
			trace.fraction = 1.0;
			//endpos is the end of the line
			VectorCopy(end, trace.endpos);
			//nothing hit
			trace.ent = 0;
			trace.area = 0;
			trace.planenum = 0;
			return trace;
		} //end if
		//number of the current node to test the line against
		nodenum = tstack_p->nodenum;
		//if it is an area
		if (nodenum < 0)
		{
#ifdef AAS_SAMPLE_DEBUG
			if (-nodenum > aasworld.numareasettings)
			{
				botimport.Print(PRT_ERROR, "AAS_TraceBoundingBox: -nodenum out of range\n");
				return trace;
			} //end if
#endif //AAS_SAMPLE_DEBUG
			//botimport.Print(PRT_MESSAGE, "areanum = %d, must be %d\n", -nodenum, AAS_PointAreaNum(start));
			//if can't enter the area because it hasn't got the right presence type
			if (!(aasworld.areasettings[-nodenum].presencetype & presencetype))
			{
				//if the start point is still the initial start point
				//NOTE: no need for epsilons because the points will be
				//exactly the same when they're both the start point
				if (tstack_p->start[0] == start[0] &&
						tstack_p->start[1] == start[1] &&
						tstack_p->start[2] == start[2])
				{
					trace.startsolid = qtrue;
					trace.fraction = 0.0;
					VectorClear(v1);
				} //end if
				else
				{
					trace.startsolid = qfalse;
					VectorSubtract(end, start, v1);
					VectorSubtract(tstack_p->start, start, v2);
					trace.fraction = VectorLength(v2) / VectorNormalize(v1);
					VectorMA(tstack_p->start, -0.125, v1, tstack_p->start);
				} //end else
				VectorCopy(tstack_p->start, trace.endpos);
				trace.ent = 0;
				trace.area = -nodenum;
//				VectorSubtract(end, start, v1);
				trace.planenum = tstack_p->planenum;
				//always take the plane with normal facing towards the trace start
				plane = &aasworld.planes[trace.planenum];
				if (DotProduct(v1, plane->normal) > 0) trace.planenum ^= 1;
				return trace;
			} //end if
			else
			{
				if (passent >= 0)
				{
					if (AAS_AreaEntityCollision(-nodenum, tstack_p->start,
													tstack_p->end, presencetype, passent,
													&trace))
					{
						if (!trace.startsolid)
						{
							VectorSubtract(end, start, v1);
							VectorSubtract(trace.endpos, start, v2);
							trace.fraction = VectorLength(v2) / VectorLength(v1);
						} //end if
						return trace;
					} //end if
				} //end if
			} //end else
			trace.lastarea = -nodenum;
			continue;
		} //end if
		//if it is a solid leaf
		if (!nodenum)
		{
			//if the start point is still the initial start point
			//NOTE: no need for epsilons because the points will be
			//exactly the same when they're both the start point
			if (tstack_p->start[0] == start[0] &&
					tstack_p->start[1] == start[1] &&
					tstack_p->start[2] == start[2])
			{
				trace.startsolid = qtrue;
				trace.fraction = 0.0;
				VectorClear(v1);
			} //end if
			else
			{
				trace.startsolid = qfalse;
				VectorSubtract(end, start, v1);
				VectorSubtract(tstack_p->start, start, v2);
				trace.fraction = VectorLength(v2) / VectorNormalize(v1);
				VectorMA(tstack_p->start, -0.125, v1, tstack_p->start);
			} //end else
			VectorCopy(tstack_p->start, trace.endpos);
			trace.ent = 0;
			trace.area = 0;	//hit solid leaf
//			VectorSubtract(end, start, v1);
			trace.planenum = tstack_p->planenum;
			//always take the plane with normal facing towards the trace start
			plane = &aasworld.planes[trace.planenum];
			if (DotProduct(v1, plane->normal) > 0) trace.planenum ^= 1;
			return trace;
		} //end if
#ifdef AAS_SAMPLE_DEBUG
		if (nodenum > aasworld.numnodes)
		{
			botimport.Print(PRT_ERROR, "AAS_TraceBoundingBox: nodenum out of range\n");
			return trace;
		} //end if
#endif //AAS_SAMPLE_DEBUG
		//the node to test against
		aasnode = &aasworld.nodes[nodenum];
		//start point of current line to test against node
		VectorCopy(tstack_p->start, cur_start);
		//end point of the current line to test against node
		VectorCopy(tstack_p->end, cur_end);
		//the current node plane
		plane = &aasworld.planes[aasnode->planenum];

		switch(plane->type)
		{/*FIXME: wtf doesn't this work? obviously the axial node planes aren't always facing positive!!!
			//check for axial planes
			case PLANE_X:
			{
				front = cur_start[0] - plane->dist;
				back = cur_end[0] - plane->dist;
				break;
			} //end case
			case PLANE_Y:
			{
				front = cur_start[1] - plane->dist;
				back = cur_end[1] - plane->dist;
				break;
			} //end case
			case PLANE_Z:
			{
				front = cur_start[2] - plane->dist;
				back = cur_end[2] - plane->dist;
				break;
			} //end case*/
			default: //gee it's not an axial plane
			{
				front = DotProduct(cur_start, plane->normal) - plane->dist;
				back = DotProduct(cur_end, plane->normal) - plane->dist;
				break;
			} //end default
		} //end switch
		// bk010221 - old location of FPE hack and divide by zero expression
		//if the whole to be traced line is totally at the front of this node
		//only go down the tree with the front child
		if ((front >= -ON_EPSILON && back >= -ON_EPSILON))
		{
			//keep the current start and end point on the stack
			//and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[0];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			} //end if
		} //end if
		//if the whole to be traced line is totally at the back of this node
		//only go down the tree with the back child
		else if ((front < ON_EPSILON && back < ON_EPSILON))
		{
			//keep the current start and end point on the stack
			//and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[1];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			} //end if
		} //end if
		//go down the tree both at the front and back of the node
		else
		{
			tmpplanenum = tstack_p->planenum;
			// bk010221 - new location of divide by zero (see above)
			if ( front == back ) front -= 0.001f; // bk0101022 - hack/FPE 
                	//calculate the hitpoint with the node (split point of the line)
			//put the crosspoint TRACEPLANE_EPSILON pixels on the near side
			if (front < 0) frac = (front + TRACEPLANE_EPSILON)/(front-back);
			else frac = (front - TRACEPLANE_EPSILON)/(front-back); // bk010221
			//
			if (frac < 0)
				frac = 0.001f; //0
			else if (frac > 1)
				frac = 0.999f; //1
			//frac = front / (front-back);
			//
			cur_mid[0] = cur_start[0] + (cur_end[0] - cur_start[0]) * frac;
			cur_mid[1] = cur_start[1] + (cur_end[1] - cur_start[1]) * frac;
			cur_mid[2] = cur_start[2] + (cur_end[2] - cur_start[2]) * frac;

//			AAS_DrawPlaneCross(cur_mid, plane->normal, plane->dist, plane->type, LINECOLOR_RED);
			//side the front part of the line is on
			side = front < 0;
			//first put the end part of the line on the stack (back side)
			VectorCopy(cur_mid, tstack_p->start);
			//not necessary to store because still on stack
			//VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum = aasnode->children[!side];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			} //end if
			//now put the part near the start of the line on the stack so we will
			//continue with thats part first. This way we'll find the first
			//hit of the bbox
			VectorCopy(cur_start, tstack_p->start);
			VectorCopy(cur_mid, tstack_p->end);
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum = aasnode->children[side];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n");
				return trace;
			} //end if
		} //end else
	} //end while
//	return trace;
} //end of the function AAS_TraceClientBBox
//===========================================================================
// recursive subdivision of the line by the BSP tree.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas)
{
	int side, nodenum, tmpplanenum;
	int numareas;
	float front, back, frac;
	vec3_t cur_start, cur_end, cur_mid;
	aas_tracestack_t tracestack[127];
	aas_tracestack_t *tstack_p;
	aas_node_t *aasnode;
	aas_plane_t *plane;

	numareas = 0;
	areas[0] = 0;
	if (!aasworld.loaded) return numareas;

	tstack_p = tracestack;
	//we start with the whole line on the stack
	VectorCopy(start, tstack_p->start);
	VectorCopy(end, tstack_p->end);
	tstack_p->planenum = 0;
	//start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1;		//starting at the root of the tree
	tstack_p++;

	while (1)
	{
		//pop up the stack
		tstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if (tstack_p < tracestack)
		{
			return numareas;
		} //end if
		//number of the current node to test the line against
		nodenum = tstack_p->nodenum;
		//if it is an area
		if (nodenum < 0)
		{
#ifdef AAS_SAMPLE_DEBUG
			if (-nodenum > aasworld.numareasettings)
			{
				botimport.Print(PRT_ERROR, "AAS_TraceAreas: -nodenum = %d out of range\n", -nodenum);
				return numareas;
			} //end if
#endif //AAS_SAMPLE_DEBUG
			//botimport.Print(PRT_MESSAGE, "areanum = %d, must be %d\n", -nodenum, AAS_PointAreaNum(start));
			areas[numareas] = -nodenum;
			if (points) VectorCopy(tstack_p->start, points[numareas]);
			numareas++;
			if (numareas >= maxareas) return numareas;
			continue;
		} //end if
		//if it is a solid leaf
		if (!nodenum)
		{
			continue;
		} //end if
#ifdef AAS_SAMPLE_DEBUG
		if (nodenum > aasworld.numnodes)
		{
			botimport.Print(PRT_ERROR, "AAS_TraceAreas: nodenum out of range\n");
			return numareas;
		} //end if
#endif //AAS_SAMPLE_DEBUG
		//the node to test against
		aasnode = &aasworld.nodes[nodenum];
		//start point of current line to test against node
		VectorCopy(tstack_p->start, cur_start);
		//end point of the current line to test against node
		VectorCopy(tstack_p->end, cur_end);
		//the current node plane
		plane = &aasworld.planes[aasnode->planenum];

		switch(plane->type)
		{/*FIXME: wtf doesn't this work? obviously the node planes aren't always facing positive!!!
			//check for axial planes
			case PLANE_X:
			{
				front = cur_start[0] - plane->dist;
				back = cur_end[0] - plane->dist;
				break;
			} //end case
			case PLANE_Y:
			{
				front = cur_start[1] - plane->dist;
				back = cur_end[1] - plane->dist;
				break;
			} //end case
			case PLANE_Z:
			{
				front = cur_start[2] - plane->dist;
				back = cur_end[2] - plane->dist;
				break;
			} //end case*/
			default: //gee it's not an axial plane
			{
				front = DotProduct(cur_start, plane->normal) - plane->dist;
				back = DotProduct(cur_end, plane->normal) - plane->dist;
				break;
			} //end default
		} //end switch

		//if the whole to be traced line is totally at the front of this node
		//only go down the tree with the front child
		if (front > 0 && back > 0)
		{
			//keep the current start and end point on the stack
			//and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[0];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceAreas: stack overflow\n");
				return numareas;
			} //end if
		} //end if
		//if the whole to be traced line is totally at the back of this node
		//only go down the tree with the back child
		else if (front <= 0 && back <= 0)
		{
			//keep the current start and end point on the stack
			//and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[1];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceAreas: stack overflow\n");
				return numareas;
			} //end if
		} //end if
		//go down the tree both at the front and back of the node
		else
		{
			tmpplanenum = tstack_p->planenum;
			//calculate the hitpoint with the node (split point of the line)
			//put the crosspoint TRACEPLANE_EPSILON pixels on the near side
			if (front < 0) frac = (front)/(front-back);
			else frac = (front)/(front-back);
			if (frac < 0) frac = 0;
			else if (frac > 1) frac = 1;
			//frac = front / (front-back);
			//
			cur_mid[0] = cur_start[0] + (cur_end[0] - cur_start[0]) * frac;
			cur_mid[1] = cur_start[1] + (cur_end[1] - cur_start[1]) * frac;
			cur_mid[2] = cur_start[2] + (cur_end[2] - cur_start[2]) * frac;

//			AAS_DrawPlaneCross(cur_mid, plane->normal, plane->dist, plane->type, LINECOLOR_RED);
			//side the front part of the line is on
			side = front < 0;
			//first put the end part of the line on the stack (back side)
			VectorCopy(cur_mid, tstack_p->start);
			//not necessary to store because still on stack
			//VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum = aasnode->children[!side];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceAreas: stack overflow\n");
				return numareas;
			} //end if
			//now put the part near the start of the line on the stack so we will
			//continue with thats part first. This way we'll find the first
			//hit of the bbox
			VectorCopy(cur_start, tstack_p->start);
			VectorCopy(cur_mid, tstack_p->end);
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum = aasnode->children[side];
			tstack_p++;
			if (tstack_p >= &tracestack[127])
			{
				botimport.Print(PRT_ERROR, "AAS_TraceAreas: stack overflow\n");
				return numareas;
			} //end if
		} //end else
	} //end while
//	return numareas;
} //end of the function AAS_TraceAreas
//===========================================================================
// a simple cross product
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
// void AAS_OrthogonalToVectors(vec3_t v1, vec3_t v2, vec3_t res)
#define AAS_OrthogonalToVectors(v1, v2, res) \
	(res)[0] = ((v1)[1] * (v2)[2]) - ((v1)[2] * (v2)[1]);\
	(res)[1] = ((v1)[2] * (v2)[0]) - ((v1)[0] * (v2)[2]);\
	(res)[2] = ((v1)[0] * (v2)[1]) - ((v1)[1] * (v2)[0]);
//===========================================================================
// tests if the given point is within the face boundaries
//
// Parameter:				face		: face to test if the point is in it
//								pnormal	: normal of the plane to use for the face
//								point		: point to test if inside face boundaries
// Returns:					qtrue if the point is within the face boundaries
// Changes Globals:		-
//===========================================================================
qboolean AAS_InsideFace(aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon)
{
	int i, firstvertex, edgenum;
	vec3_t v0;
	vec3_t edgevec, pointvec, sepnormal;
	aas_edge_t *edge;
#ifdef AAS_SAMPLE_DEBUG
	int lastvertex = 0;
#endif //AAS_SAMPLE_DEBUG

	if (!aasworld.loaded) return qfalse;

	for (i = 0; i < face->numedges; i++)
	{
		edgenum = aasworld.edgeindex[face->firstedge + i];
		edge = &aasworld.edges[abs(edgenum)];
		//get the first vertex of the edge
		firstvertex = edgenum < 0;
		VectorCopy(aasworld.vertexes[edge->v[firstvertex]], v0);
		//edge vector
		VectorSubtract(aasworld.vertexes[edge->v[!firstvertex]], v0, edgevec);
		//
#ifdef AAS_SAMPLE_DEBUG
		if (lastvertex && lastvertex != edge->v[firstvertex])
		{
			botimport.Print(PRT_MESSAGE, "winding not counter clockwise\n");
		} //end if
		lastvertex = edge->v[!firstvertex];
#endif //AAS_SAMPLE_DEBUG
		//vector from first edge point to point possible in face
		VectorSubtract(point, v0, pointvec);
		//get a vector pointing inside the face orthogonal to both the
		//edge vector and the normal vector of the plane the face is in
		//this vector defines a plane through the origin (first vertex of
		//edge) and through both the edge vector and the normal vector
		//of the plane
		AAS_OrthogonalToVectors(edgevec, pnormal, sepnormal);
		//check on which side of the above plane the point is
		//this is done by checking the sign of the dot product of the
		//vector orthogonal vector from above and the vector from the
		//origin (first vertex of edge) to the point 
		//if the dotproduct is smaller than zero the point is outside the face
		if (DotProduct(pointvec, sepnormal) < -epsilon) return qfalse;
	} //end for
	return qtrue;
} //end of the function AAS_InsideFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_PointInsideFace(int facenum, vec3_t point, float epsilon)
{
	int i, firstvertex, edgenum;
	vec_t *v1, *v2;
	vec3_t edgevec, pointvec, sepnormal;
	aas_edge_t *edge;
	aas_plane_t *plane;
	aas_face_t *face;

	if (!aasworld.loaded) return qfalse;

	face = &aasworld.faces[facenum];
	plane = &aasworld.planes[face->planenum];
	//
	for (i = 0; i < face->numedges; i++)
	{
		edgenum = aasworld.edgeindex[face->firstedge + i];
		edge = &aasworld.edges[abs(edgenum)];
		//get the first vertex of the edge
		firstvertex = edgenum < 0;
		v1 = aasworld.vertexes[edge->v[firstvertex]];
		v2 = aasworld.vertexes[edge->v[!firstvertex]];
		//edge vector
		VectorSubtract(v2, v1, edgevec);
		//vector from first edge point to point possible in face
		VectorSubtract(point, v1, pointvec);
		//
		CrossProduct(edgevec, plane->normal, sepnormal);
		//
		if (DotProduct(pointvec, sepnormal) < -epsilon) return qfalse;
	} //end for
	return qtrue;
} //end of the function AAS_PointInsideFace
//===========================================================================
// returns the ground face the given point is above in the given area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_face_t *AAS_AreaGroundFace(int areanum, vec3_t point)
{
	int i, facenum;
	vec3_t up = {0, 0, 1};
	vec3_t normal;
	aas_area_t *area;
	aas_face_t *face;

	if (!aasworld.loaded) return NULL;

	area = &aasworld.areas[areanum];
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = aasworld.faceindex[area->firstface + i];
		face = &aasworld.faces[abs(facenum)];
		//if this is a ground face
		if (face->faceflags & FACE_GROUND)
		{
			//get the up or down normal
			if (aasworld.planes[face->planenum].normal[2] < 0) VectorNegate(up, normal);
			else VectorCopy(up, normal);
			//check if the point is in the face
			if (AAS_InsideFace(face, normal, point, 0.01f)) return face;
		} //end if
	} //end for
	return NULL;
} //end of the function AAS_AreaGroundFace
//===========================================================================
// returns the face the trace end position is situated in
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FacePlane(int facenum, vec3_t normal, float *dist)
{
	aas_plane_t *plane;

	plane = &aasworld.planes[aasworld.faces[facenum].planenum];
	VectorCopy(plane->normal, normal);
	*dist = plane->dist;
} //end of the function AAS_FacePlane
//===========================================================================
// returns the face the trace end position is situated in
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_face_t *AAS_TraceEndFace(aas_trace_t *trace)
{
	int i, facenum;
	aas_area_t *area;
	aas_face_t *face, *firstface = NULL;

	if (!aasworld.loaded) return NULL;

	//if started in solid no face was hit
	if (trace->startsolid) return NULL;
	//trace->lastarea is the last area the trace was in
	area = &aasworld.areas[trace->lastarea];
	//check which face the trace.endpos was in
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = aasworld.faceindex[area->firstface + i];
		face = &aasworld.faces[abs(facenum)];
		//if the face is in the same plane as the trace end point
		if ((face->planenum & ~1) == (trace->planenum & ~1))
		{
			//firstface is used for optimization, if theres only one
			//face in the plane then it has to be the good one
			//if there are more faces in the same plane then always
			//check the one with the fewest edges first
/*			if (firstface)
			{
				if (firstface->numedges < face->numedges)
				{
					if (AAS_InsideFace(firstface,
						aasworld.planes[face->planenum].normal, trace->endpos))
					{
						return firstface;
					} //end if
					firstface = face;
				} //end if
				else
				{
					if (AAS_InsideFace(face,
						aasworld.planes[face->planenum].normal, trace->endpos))
					{
						return face;
					} //end if
				} //end else
			} //end if
			else
			{
				firstface = face;
			} //end else*/
			if (AAS_InsideFace(face,
						aasworld.planes[face->planenum].normal, trace->endpos, 0.01f)) return face;
		} //end if
	} //end for
	return firstface;
} //end of the function AAS_TraceEndFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_BoxOnPlaneSide2(vec3_t absmins, vec3_t absmaxs, aas_plane_t *p)
{
	int i, sides;
	float dist1, dist2;
	vec3_t corners[2];

	for (i = 0; i < 3; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = absmins[i];
			corners[1][i] = absmaxs[i];
		} //end if
		else
		{
			corners[1][i] = absmins[i];
			corners[0][i] = absmaxs[i];
		} //end else
	} //end for
	dist1 = DotProduct(p->normal, corners[0]) - p->dist;
	dist2 = DotProduct(p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0) sides = 1;
	if (dist2 < 0) sides |= 2;

	return sides;
} //end of the function AAS_BoxOnPlaneSide2
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
//int AAS_BoxOnPlaneSide(vec3_t absmins, vec3_t absmaxs, aas_plane_t *p)
#define AAS_BoxOnPlaneSide(absmins, absmaxs, p) (\
	( (p)->type < 3) ?\
	(\
		( (p)->dist <= (absmins)[(p)->type]) ?\
		(\
			1\
		)\
		:\
		(\
			( (p)->dist >= (absmaxs)[(p)->type]) ?\
			(\
				2\
			)\
			:\
			(\
				3\
			)\
		)\
	)\
	:\
	(\
		AAS_BoxOnPlaneSide2((absmins), (absmaxs), (p))\
	)\
) //end of the function AAS_BoxOnPlaneSide
//===========================================================================
// remove the links to this entity from all areas
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_UnlinkFromAreas(aas_link_t *areas)
{
	aas_link_t *link, *nextlink;

	for (link = areas; link; link = nextlink)
	{
		//next area the entity is linked in
		nextlink = link->next_area;
		//remove the entity from the linked list of this area
		if (link->prev_ent) link->prev_ent->next_ent = link->next_ent;
		else aasworld.arealinkedentities[link->areanum] = link->next_ent;
		if (link->next_ent) link->next_ent->prev_ent = link->prev_ent;
		//deallocate the link structure
		AAS_DeAllocAASLink(link);
	} //end for
} //end of the function AAS_UnlinkFromAreas
//===========================================================================
// link the entity to the areas the bounding box is totally or partly
// situated in. This is done with recursion down the tree using the
// bounding box to test for plane sides
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

typedef struct
{
	int nodenum;		//node found after splitting
} aas_linkstack_t;

aas_link_t *AAS_AASLinkEntity(vec3_t absmins, vec3_t absmaxs, int entnum)
{
	int side, nodenum;
	aas_linkstack_t linkstack[128];
	aas_linkstack_t *lstack_p;
	aas_node_t *aasnode;
	aas_plane_t *plane;
	aas_link_t *link, *areas;

	if (!aasworld.loaded)
	{
		botimport.Print(PRT_ERROR, "AAS_LinkEntity: aas not loaded\n");
		return NULL;
	} //end if

	areas = NULL;
	//
	lstack_p = linkstack;
	//we start with the whole line on the stack
	//start with node 1 because node zero is a dummy used for solid leafs
	lstack_p->nodenum = 1;		//starting at the root of the tree
	lstack_p++;
	
	while (1)
	{
		//pop up the stack
		lstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if (lstack_p < linkstack) break;
		//number of the current node to test the line against
		nodenum = lstack_p->nodenum;
		//if it is an area
		if (nodenum < 0)
		{
			//NOTE: the entity might have already been linked into this area
			// because several node children can point to the same area
			for (link = aasworld.arealinkedentities[-nodenum]; link; link = link->next_ent)
			{
				if (link->entnum == entnum) break;
			} //end for
			if (link) continue;
			//
			link = AAS_AllocAASLink();
			if (!link) return areas;
			link->entnum = entnum;
			link->areanum = -nodenum;
			//put the link into the double linked area list of the entity
			link->prev_area = NULL;
			link->next_area = areas;
			if (areas) areas->prev_area = link;
			areas = link;
			//put the link into the double linked entity list of the area
			link->prev_ent = NULL;
			link->next_ent = aasworld.arealinkedentities[-nodenum];
			if (aasworld.arealinkedentities[-nodenum])
					aasworld.arealinkedentities[-nodenum]->prev_ent = link;
			aasworld.arealinkedentities[-nodenum] = link;
			//
			continue;
		} //end if
		//if solid leaf
		if (!nodenum) continue;
		//the node to test against
		aasnode = &aasworld.nodes[nodenum];
		//the current node plane
		plane = &aasworld.planes[aasnode->planenum];
		//get the side(s) the box is situated relative to the plane
		side = AAS_BoxOnPlaneSide2(absmins, absmaxs, plane);
		//if on the front side of the node
		if (side & 1)
		{
			lstack_p->nodenum = aasnode->children[0];
			lstack_p++;
		} //end if
		if (lstack_p >= &linkstack[127])
		{
			botimport.Print(PRT_ERROR, "AAS_LinkEntity: stack overflow\n");
			break;
		} //end if
		//if on the back side of the node
		if (side & 2)
		{
			lstack_p->nodenum = aasnode->children[1];
			lstack_p++;
		} //end if
		if (lstack_p >= &linkstack[127])
		{
			botimport.Print(PRT_ERROR, "AAS_LinkEntity: stack overflow\n");
			break;
		} //end if
	} //end while
	return areas;
} //end of the function AAS_AASLinkEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_link_t *AAS_LinkEntityClientBBox(vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype)
{
	vec3_t mins, maxs;
	vec3_t newabsmins, newabsmaxs;

	AAS_PresenceTypeBoundingBox(presencetype, mins, maxs);
	VectorSubtract(absmins, maxs, newabsmins);
	VectorSubtract(absmaxs, mins, newabsmaxs);
	//relink the entity
	return AAS_AASLinkEntity(newabsmins, newabsmaxs, entnum);
} //end of the function AAS_LinkEntityClientBBox
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas)
{
	aas_link_t *linkedareas, *link;
	int num;

	linkedareas = AAS_AASLinkEntity(absmins, absmaxs, -1);
	num = 0;
	for (link = linkedareas; link; link = link->next_area)
	{
		areas[num] = link->areanum;
		num++;
		if (num >= maxareas)
			break;
	} //end for
	AAS_UnlinkFromAreas(linkedareas);
	return num;
} //end of the function AAS_BBoxAreas
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaInfo( int areanum, aas_areainfo_t *info )
{
	aas_areasettings_t *settings;
	if (!info)
		return 0;
	if (areanum <= 0 || areanum >= aasworld.numareas)
	{
		botimport.Print(PRT_ERROR, "AAS_AreaInfo: areanum %d out of range\n", areanum);
		return 0;
	} //end if
	settings = &aasworld.areasettings[areanum];
	info->cluster = settings->cluster;
	info->contents = settings->contents;
	info->flags = settings->areaflags;
	info->presencetype = settings->presencetype;
	VectorCopy(aasworld.areas[areanum].mins, info->mins);
	VectorCopy(aasworld.areas[areanum].maxs, info->maxs);
	VectorCopy(aasworld.areas[areanum].center, info->center);
	return sizeof(aas_areainfo_t);
} //end of the function AAS_AreaInfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_plane_t *AAS_PlaneFromNum(int planenum)
{
	if (!aasworld.loaded) return NULL;

	return &aasworld.planes[planenum];
} //end of the function AAS_PlaneFromNum
