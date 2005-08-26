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

/*****************************************************************************
 * name:		be_aas_reach.c
 *
 * desc:		reachability calculations
 *
 * $Archive: /MissionPack/code/botlib/be_aas_reach.c $
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_libvar.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern int Sys_MilliSeconds(void);


extern botlib_import_t botimport;

//#define REACH_DEBUG

//NOTE: all travel times are in hundreth of a second
//maximum number of reachability links
#define AAS_MAX_REACHABILITYSIZE			65536
//number of areas reachability is calculated for each frame
#define REACHABILITYAREASPERCYCLE			15
//number of units reachability points are placed inside the areas
#define INSIDEUNITS							2
#define INSIDEUNITS_WALKEND					5
#define INSIDEUNITS_WALKSTART				0.1
#define INSIDEUNITS_WATERJUMP				15
//area flag used for weapon jumping
#define AREA_WEAPONJUMP						8192	//valid area to weapon jump to
//number of reachabilities of each type
int reach_swim;			//swim
int reach_equalfloor;	//walk on floors with equal height
int reach_step;			//step up
int reach_walk;			//walk of step
int reach_barrier;		//jump up to a barrier
int reach_waterjump;	//jump out of water
int reach_walkoffledge;	//walk of a ledge
int reach_jump;			//jump
int reach_ladder;		//climb or descent a ladder
int reach_teleport;		//teleport
int reach_elevator;		//use an elevator
int reach_funcbob;		//use a func bob
int reach_grapple;		//grapple hook
int reach_doublejump;	//double jump
int reach_rampjump;		//ramp jump
int reach_strafejump;	//strafe jump (just normal jump but further)
int reach_rocketjump;	//rocket jump
int reach_bfgjump;		//bfg jump
int reach_jumppad;		//jump pads
//if true grapple reachabilities are skipped
int calcgrapplereach;
//linked reachability
typedef struct aas_lreachability_s
{
	int areanum;					//number of the reachable area
	int facenum;					//number of the face towards the other area
	int edgenum;					//number of the edge towards the other area
	vec3_t start;					//start point of inter area movement
	vec3_t end;						//end point of inter area movement
	int traveltype;					//type of travel required to get to the area
	unsigned short int traveltime;	//travel time of the inter area movement
	//
	struct aas_lreachability_s *next;
} aas_lreachability_t;
//temporary reachabilities
aas_lreachability_t *reachabilityheap;	//heap with reachabilities
aas_lreachability_t *nextreachability;	//next free reachability from the heap
aas_lreachability_t **areareachability;	//reachability links for every area
int numlreachabilities;

//===========================================================================
// returns the surface area of the given face
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_FaceArea(aas_face_t *face)
{
	int i, edgenum, side;
	float total;
	vec_t *v;
	vec3_t d1, d2, cross;
	aas_edge_t *edge;

	edgenum = aasworld.edgeindex[face->firstedge];
	side = edgenum < 0;
	edge = &aasworld.edges[abs(edgenum)];
	v = aasworld.vertexes[edge->v[side]];

	total = 0;
	for (i = 1; i < face->numedges - 1; i++)
	{
		edgenum = aasworld.edgeindex[face->firstedge + i];
		side = edgenum < 0;
		edge = &aasworld.edges[abs(edgenum)];
		VectorSubtract(aasworld.vertexes[edge->v[side]], v, d1);
		VectorSubtract(aasworld.vertexes[edge->v[!side]], v, d2);
		CrossProduct(d1, d2, cross);
		total += 0.5 * VectorLength(cross);
	} //end for
	return total;
} //end of the function AAS_FaceArea
//===========================================================================
// returns the volume of an area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_AreaVolume(int areanum)
{
	int i, edgenum, facenum, side;
	vec_t d, a, volume;
	vec3_t corner;
	aas_plane_t *plane;
	aas_edge_t *edge;
	aas_face_t *face;
	aas_area_t *area;

	area = &aasworld.areas[areanum];
	facenum = aasworld.faceindex[area->firstface];
	face = &aasworld.faces[abs(facenum)];
	edgenum = aasworld.edgeindex[face->firstedge];
	edge = &aasworld.edges[abs(edgenum)];
	//
	VectorCopy(aasworld.vertexes[edge->v[0]], corner);

	//make tetrahedrons to all other faces
	volume = 0;
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = abs(aasworld.faceindex[area->firstface + i]);
		face = &aasworld.faces[facenum];
		side = face->backarea != areanum;
		plane = &aasworld.planes[face->planenum ^ side];
		d = -(DotProduct (corner, plane->normal) - plane->dist);
		a = AAS_FaceArea(face);
		volume += d * a;
	} //end for

	volume /= 3;
	return volume;
} //end of the function AAS_AreaVolume
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_BestReachableLinkArea(aas_link_t *areas)
{
	aas_link_t *link;

	for (link = areas; link; link = link->next_area)
	{
		if (AAS_AreaGrounded(link->areanum) || AAS_AreaSwim(link->areanum))
		{
			return link->areanum;
		} //end if
	} //end for
	//
	for (link = areas; link; link = link->next_area)
	{
		if (link->areanum) return link->areanum;
		//FIXME: this is a bad idea when the reachability is not yet
		// calculated when the level items are loaded
		if (AAS_AreaReachability(link->areanum))
			return link->areanum;
	} //end for
	return 0;
} //end of the function AAS_BestReachableLinkArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_GetJumpPadInfo(int ent, vec3_t areastart, vec3_t absmins, vec3_t absmaxs, vec3_t velocity)
{
	int modelnum, ent2;
	float speed, height, gravity, time, dist, forward;
	vec3_t origin, angles, teststart, ent2origin;
	aas_trace_t trace;
	char model[MAX_EPAIRKEY];
	char target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY];

	//
	AAS_FloatForBSPEpairKey(ent, "speed", &speed);
	if (!speed) speed = 1000;
	VectorClear(angles);
	//get the mins, maxs and origin of the model
	AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
	if (model[0]) modelnum = atoi(model+1);
	else modelnum = 0;
	AAS_BSPModelMinsMaxsOrigin(modelnum, angles, absmins, absmaxs, origin);
	VectorAdd(origin, absmins, absmins);
	VectorAdd(origin, absmaxs, absmaxs);
	VectorAdd(absmins, absmaxs, origin);
	VectorScale (origin, 0.5, origin);

	//get the start areas
	VectorCopy(origin, teststart);
	teststart[2] += 64;
	trace = AAS_TraceClientBBox(teststart, origin, PRESENCE_CROUCH, -1);
	if (trace.startsolid)
	{
		botimport.Print(PRT_MESSAGE, "trigger_push start solid\n");
		VectorCopy(origin, areastart);
	} //end if
	else
	{
		VectorCopy(trace.endpos, areastart);
	} //end else
	areastart[2] += 0.125;
	//
	//AAS_DrawPermanentCross(origin, 4, 4);
	//get the target entity
	AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY);
	for (ent2 = AAS_NextBSPEntity(0); ent2; ent2 = AAS_NextBSPEntity(ent2))
	{
		if (!AAS_ValueForBSPEpairKey(ent2, "targetname", targetname, MAX_EPAIRKEY)) continue;
		if (!strcmp(targetname, target)) break;
	} //end for
	if (!ent2)
	{
		botimport.Print(PRT_MESSAGE, "trigger_push without target entity %s\n", target);
		return qfalse;
	} //end if
	AAS_VectorForBSPEpairKey(ent2, "origin", ent2origin);
	//
	height = ent2origin[2] - origin[2];
	gravity = aassettings.phys_gravity;
	time = sqrt( height / ( 0.5 * gravity ) );
	if (!time)
	{
		botimport.Print(PRT_MESSAGE, "trigger_push without time\n");
		return qfalse;
	} //end if
	// set s.origin2 to the push velocity
	VectorSubtract ( ent2origin, origin, velocity);
	dist = VectorNormalize( velocity);
	forward = dist / time;
	//FIXME: why multiply by 1.1
	forward *= 1.1f;
	VectorScale(velocity, forward, velocity);
	velocity[2] = time * gravity;
	return qtrue;
} //end of the function AAS_GetJumpPadInfo
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_BestReachableFromJumpPadArea(vec3_t origin, vec3_t mins, vec3_t maxs)
{
	int area2num, ent, bot_visualizejumppads, bestareanum;
	float volume, bestareavolume;
	vec3_t areastart, cmdmove, bboxmins, bboxmaxs;
	vec3_t absmins, absmaxs, velocity;
	aas_clientmove_t move;
	aas_link_t *areas, *link;
	char classname[MAX_EPAIRKEY];

#ifdef BSPC
	bot_visualizejumppads = 0;
#else
	bot_visualizejumppads = LibVarValue("bot_visualizejumppads", "0");
#endif
	VectorAdd(origin, mins, bboxmins);
	VectorAdd(origin, maxs, bboxmaxs);
	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		if (strcmp(classname, "trigger_push")) continue;
		//
		if (!AAS_GetJumpPadInfo(ent, areastart, absmins, absmaxs, velocity)) continue;
		//get the areas the jump pad brush is in
		areas = AAS_LinkEntityClientBBox(absmins, absmaxs, -1, PRESENCE_CROUCH);
		for (link = areas; link; link = link->next_area)
		{
			if (AAS_AreaJumpPad(link->areanum)) break;
		} //end for
		if (!link)
		{
			botimport.Print(PRT_MESSAGE, "trigger_push not in any jump pad area\n");
			AAS_UnlinkFromAreas(areas);
			continue;
		} //end if
		//
		//botimport.Print(PRT_MESSAGE, "found a trigger_push with velocity %f %f %f\n", velocity[0], velocity[1], velocity[2]);
		//
		VectorSet(cmdmove, 0, 0, 0);
		Com_Memset(&move, 0, sizeof(aas_clientmove_t));
		area2num = 0;
		AAS_ClientMovementHitBBox(&move, -1, areastart, PRESENCE_NORMAL, qfalse,
								velocity, cmdmove, 0, 30, 0.1f, bboxmins, bboxmaxs, bot_visualizejumppads);
		if (move.frames < 30)
		{
			bestareanum = 0;
			bestareavolume = 0;
			for (link = areas; link; link = link->next_area)
			{
				if (!AAS_AreaJumpPad(link->areanum)) continue;
				volume = AAS_AreaVolume(link->areanum);
				if (volume >= bestareavolume)
				{
					bestareanum = link->areanum;
					bestareavolume = volume;
				} //end if
			} //end if
			AAS_UnlinkFromAreas(areas);
			return bestareanum;
		} //end if
		AAS_UnlinkFromAreas(areas);
	} //end for
	return 0;
} //end of the function AAS_BestReachableFromJumpPadArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_BestReachableArea(vec3_t origin, vec3_t mins, vec3_t maxs, vec3_t goalorigin)
{
	int areanum, i, j, k, l;
	aas_link_t *areas;
	vec3_t absmins, absmaxs;
	//vec3_t bbmins, bbmaxs;
	vec3_t start, end;
	aas_trace_t trace;

	if (!aasworld.loaded)
	{
		botimport.Print(PRT_ERROR, "AAS_BestReachableArea: aas not loaded\n");
		return 0;
	} //end if
	//find a point in an area
	VectorCopy(origin, start);
	areanum = AAS_PointAreaNum(start);
	//while no area found fudge around a little
	for (i = 0; i < 5 && !areanum; i++)
	{
		for (j = 0; j < 5 && !areanum; j++)
		{
			for (k = -1; k <= 1 && !areanum; k++)
			{
				for (l = -1; l <= 1 && !areanum; l++)
				{
					VectorCopy(origin, start);
					start[0] += (float) j * 4 * k;
					start[1] += (float) j * 4 * l;
					start[2] += (float) i * 4;
					areanum = AAS_PointAreaNum(start);
				} //end for
			} //end for
		} //end for
	} //end for
	//if an area was found
	if (areanum)
	{
		//drop client bbox down and try again
		VectorCopy(start, end);
		start[2] += 0.25;
		end[2] -= 50;
		trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
		if (!trace.startsolid)
		{
			areanum = AAS_PointAreaNum(trace.endpos);
			VectorCopy(trace.endpos, goalorigin);
			//FIXME: cannot enable next line right now because the reachability
			// does not have to be calculated when the level items are loaded
			//if the origin is in an area with reachability
			//if (AAS_AreaReachability(areanum)) return areanum;
			if (areanum) return areanum;
		} //end if
		else
		{
			//it can very well happen that the AAS_PointAreaNum function tells that
			//a point is in an area and that starting a AAS_TraceClientBBox from that
			//point will return trace.startsolid qtrue
#if 0
			if (AAS_PointAreaNum(start))
			{
				Log_Write("point %f %f %f in area %d but trace startsolid", start[0], start[1], start[2], areanum);
				AAS_DrawPermanentCross(start, 4, LINECOLOR_RED);
			} //end if
			botimport.Print(PRT_MESSAGE, "AAS_BestReachableArea: start solid\n");
#endif
			VectorCopy(start, goalorigin);
			return areanum;
		} //end else
	} //end if
	//
	//AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bbmins, bbmaxs);
	//NOTE: the goal origin does not have to be in the goal area
	// because the bot will have to move towards the item origin anyway
	VectorCopy(origin, goalorigin);
	//
	VectorAdd(origin, mins, absmins);
	VectorAdd(origin, maxs, absmaxs);
	//add bounding box size
	//VectorSubtract(absmins, bbmaxs, absmins);
	//VectorSubtract(absmaxs, bbmins, absmaxs);
	//link an invalid (-1) entity
	areas = AAS_LinkEntityClientBBox(absmins, absmaxs, -1, PRESENCE_CROUCH);
	//get the reachable link arae
	areanum = AAS_BestReachableLinkArea(areas);
	//unlink the invalid entity
	AAS_UnlinkFromAreas(areas);
	//
	return areanum;
} //end of the function AAS_BestReachableArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetupReachabilityHeap(void)
{
	int i;

	reachabilityheap = (aas_lreachability_t *) GetClearedMemory(
						AAS_MAX_REACHABILITYSIZE * sizeof(aas_lreachability_t));
	for (i = 0; i < AAS_MAX_REACHABILITYSIZE-1; i++)
	{
		reachabilityheap[i].next = &reachabilityheap[i+1];
	} //end for
	reachabilityheap[AAS_MAX_REACHABILITYSIZE-1].next = NULL;
	nextreachability = reachabilityheap;
	numlreachabilities = 0;
} //end of the function AAS_InitReachabilityHeap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ShutDownReachabilityHeap(void)
{
	FreeMemory(reachabilityheap);
	numlreachabilities = 0;
} //end of the function AAS_ShutDownReachabilityHeap
//===========================================================================
// returns a reachability link
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_lreachability_t *AAS_AllocReachability(void)
{
	aas_lreachability_t *r;

	if (!nextreachability) return NULL;
	//make sure the error message only shows up once
	if (!nextreachability->next) AAS_Error("AAS_MAX_REACHABILITYSIZE");
	//
	r = nextreachability;
	nextreachability = nextreachability->next;
	numlreachabilities++;
	return r;
} //end of the function AAS_AllocReachability
//===========================================================================
// frees a reachability link
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeReachability(aas_lreachability_t *lreach)
{
	Com_Memset(lreach, 0, sizeof(aas_lreachability_t));

	lreach->next = nextreachability;
	nextreachability = lreach;
	numlreachabilities--;
} //end of the function AAS_FreeReachability
//===========================================================================
// returns qtrue if the area has reachability links
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaReachability(int areanum)
{
	if (areanum < 0 || areanum >= aasworld.numareas)
	{
		AAS_Error("AAS_AreaReachability: areanum %d out of range", areanum);
		return 0;
	} //end if
	return aasworld.areasettings[areanum].numreachableareas;
} //end of the function AAS_AreaReachability
//===========================================================================
// returns the surface area of all ground faces together of the area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_AreaGroundFaceArea(int areanum)
{
	int i;
	float total;
	aas_area_t *area;
	aas_face_t *face;

	total = 0;
	area = &aasworld.areas[areanum];
	for (i = 0; i < area->numfaces; i++)
	{
		face = &aasworld.faces[abs(aasworld.faceindex[area->firstface + i])];
		if (!(face->faceflags & FACE_GROUND)) continue;
		//
		total += AAS_FaceArea(face);
	} //end for
	return total;
} //end of the function AAS_AreaGroundFaceArea
//===========================================================================
// returns the center of a face
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FaceCenter(int facenum, vec3_t center)
{
	int i;
	float scale;
	aas_face_t *face;
	aas_edge_t *edge;

	face = &aasworld.faces[facenum];

	VectorClear(center);
	for (i = 0; i < face->numedges; i++)
	{
		edge = &aasworld.edges[abs(aasworld.edgeindex[face->firstedge + i])];
		VectorAdd(center, aasworld.vertexes[edge->v[0]], center);
		VectorAdd(center, aasworld.vertexes[edge->v[1]], center);
	} //end for
	scale = 0.5 / face->numedges;
	VectorScale(center, scale, center);
} //end of the function AAS_FaceCenter
//===========================================================================
// returns the maximum distance a player can fall before being damaged
// damage = deltavelocity*deltavelocity  * 0.0001
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_FallDamageDistance(void)
{
	float maxzvelocity, gravity, t;

	maxzvelocity = sqrt(30 * 10000);
	gravity = aassettings.phys_gravity;
	t = maxzvelocity / gravity;
	return 0.5 * gravity * t * t;
} //end of the function AAS_FallDamageDistance
//===========================================================================
// distance = 0.5 * gravity * t * t
// vel = t * gravity
// damage = vel * vel * 0.0001
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float AAS_FallDelta(float distance)
{
	float t, delta, gravity;

	gravity = aassettings.phys_gravity;
	t = sqrt(fabs(distance) * 2 / gravity);
	delta = t * gravity;
	return delta * delta * 0.0001;
} //end of the function AAS_FallDelta
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float AAS_MaxJumpHeight(float phys_jumpvel)
{
	float phys_gravity;

	phys_gravity = aassettings.phys_gravity;
	//maximum height a player can jump with the given initial z velocity
	return 0.5 * phys_gravity * (phys_jumpvel / phys_gravity) * (phys_jumpvel / phys_gravity);
} //end of the function MaxJumpHeight
//===========================================================================
// returns true if a player can only crouch in the area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_MaxJumpDistance(float phys_jumpvel)
{
	float phys_gravity, phys_maxvelocity, t;

	phys_gravity = aassettings.phys_gravity;
	phys_maxvelocity = aassettings.phys_maxvelocity;
	//time a player takes to fall the height
	t = sqrt(aassettings.rs_maxjumpfallheight / (0.5 * phys_gravity));
   //maximum distance
	return phys_maxvelocity * (t + phys_jumpvel / phys_gravity);
} //end of the function AAS_MaxJumpDistance
//===========================================================================
// returns true if a player can only crouch in the area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaCrouch(int areanum)
{
	if (!(aasworld.areasettings[areanum].presencetype & PRESENCE_NORMAL)) return qtrue;
	else return qfalse;
} //end of the function AAS_AreaCrouch
//===========================================================================
// returns qtrue if it is possible to swim in the area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaSwim(int areanum)
{
	if (aasworld.areasettings[areanum].areaflags & AREA_LIQUID) return qtrue;
	else return qfalse;
} //end of the function AAS_AreaSwim
//===========================================================================
// returns qtrue if the area contains a liquid
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaLiquid(int areanum)
{
	if (aasworld.areasettings[areanum].areaflags & AREA_LIQUID) return qtrue;
	else return qfalse;
} //end of the function AAS_AreaLiquid
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaLava(int areanum)
{
	return (aasworld.areasettings[areanum].contents & AREACONTENTS_LAVA);
} //end of the function AAS_AreaLava
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaSlime(int areanum)
{
	return (aasworld.areasettings[areanum].contents & AREACONTENTS_SLIME);
} //end of the function AAS_AreaSlime
//===========================================================================
// returns qtrue if the area contains ground faces
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaGrounded(int areanum)
{
	return (aasworld.areasettings[areanum].areaflags & AREA_GROUNDED);
} //end of the function AAS_AreaGround
//===========================================================================
// returns true if the area contains ladder faces
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaLadder(int areanum)
{
	return (aasworld.areasettings[areanum].areaflags & AREA_LADDER);
} //end of the function AAS_AreaLadder
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaJumpPad(int areanum)
{
	return (aasworld.areasettings[areanum].contents & AREACONTENTS_JUMPPAD);
} //end of the function AAS_AreaJumpPad
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaTeleporter(int areanum)
{
	return (aasworld.areasettings[areanum].contents & AREACONTENTS_TELEPORTER);
} //end of the function AAS_AreaTeleporter
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaClusterPortal(int areanum)
{
	return (aasworld.areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL);
} //end of the function AAS_AreaClusterPortal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaDoNotEnter(int areanum)
{
	return (aasworld.areasettings[areanum].contents & AREACONTENTS_DONOTENTER);
} //end of the function AAS_AreaDoNotEnter
//===========================================================================
// returns the time it takes perform a barrier jump
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
unsigned short int AAS_BarrierJumpTravelTime(void)
{
	return aassettings.phys_jumpvel / (aassettings.phys_gravity * 0.1);
} //end op the function AAS_BarrierJumpTravelTime
//===========================================================================
// returns true if there already exists a reachability from area1 to area2
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_ReachabilityExists(int area1num, int area2num)
{
	aas_lreachability_t *r;

	for (r = areareachability[area1num]; r; r = r->next)
	{
		if (r->areanum == area2num) return qtrue;
	} //end for
	return qfalse;
} //end of the function AAS_ReachabilityExists
//===========================================================================
// returns true if there is a solid just after the end point when going
// from start to end
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NearbySolidOrGap(vec3_t start, vec3_t end)
{
	vec3_t dir, testpoint;
	int areanum;

	VectorSubtract(end, start, dir);
	dir[2] = 0;
	VectorNormalize(dir);
	VectorMA(end, 48, dir, testpoint);

	areanum = AAS_PointAreaNum(testpoint);
	if (!areanum)
	{
		testpoint[2] += 16;
		areanum = AAS_PointAreaNum(testpoint);
		if (!areanum) return qtrue;
	} //end if
	VectorMA(end, 64, dir, testpoint);
	areanum = AAS_PointAreaNum(testpoint);
	if (areanum)
	{
		if (!AAS_AreaSwim(areanum) && !AAS_AreaGrounded(areanum)) return qtrue;
	} //end if
	return qfalse;
} //end of the function AAS_SolidGapTime
//===========================================================================
// searches for swim reachabilities between adjacent areas
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Swim(int area1num, int area2num)
{
	int i, j, face1num, face2num, side1;
	aas_area_t *area1, *area2;
	aas_areasettings_t *areasettings;
	aas_lreachability_t *lreach;
	aas_face_t *face1;
	aas_plane_t *plane;
	vec3_t start;

	if (!AAS_AreaSwim(area1num) || !AAS_AreaSwim(area2num)) return qfalse;
	//if the second area is crouch only
	if (!(aasworld.areasettings[area2num].presencetype & PRESENCE_NORMAL)) return qfalse;

	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];

	//if the areas are not near anough
	for (i = 0; i < 3; i++)
	{
		if (area1->mins[i] > area2->maxs[i] + 10) return qfalse;
		if (area1->maxs[i] < area2->mins[i] - 10) return qfalse;
	} //end for
	//find a shared face and create a reachability link
	for (i = 0; i < area1->numfaces; i++)
	{
		face1num = aasworld.faceindex[area1->firstface + i];
		side1 = face1num < 0;
		face1num = abs(face1num);
		//
		for (j = 0; j < area2->numfaces; j++)
		{
			face2num = abs(aasworld.faceindex[area2->firstface + j]);
			//
			if (face1num == face2num)
			{
				AAS_FaceCenter(face1num, start);
				//
				if (AAS_PointContents(start) & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER))
				{
					//
					face1 = &aasworld.faces[face1num];
					areasettings = &aasworld.areasettings[area1num];
					//create a new reachability link
					lreach = AAS_AllocReachability();
					if (!lreach) return qfalse;
					lreach->areanum = area2num;
					lreach->facenum = face1num;
					lreach->edgenum = 0;
					VectorCopy(start, lreach->start);
					plane = &aasworld.planes[face1->planenum ^ side1];
					VectorMA(lreach->start, -INSIDEUNITS, plane->normal, lreach->end);
					lreach->traveltype = TRAVEL_SWIM;
					lreach->traveltime = 1;
					//if the volume of the area is rather small
					if (AAS_AreaVolume(area2num) < 800)
						lreach->traveltime += 200;
					//if (!(AAS_PointContents(start) & MASK_WATER)) lreach->traveltime += 500;
					//link the reachability
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					reach_swim++;
					return qtrue;
				} //end if
			} //end if
		} //end for
	} //end for
	return qfalse;
} //end of the function AAS_Reachability_Swim
//===========================================================================
// searches for reachabilities between adjacent areas with equal floor
// heights
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_EqualFloorHeight(int area1num, int area2num)
{
	int i, j, edgenum, edgenum1, edgenum2, foundreach, side;
	float height, bestheight, length, bestlength;
	vec3_t dir, start, end, normal, invgravity, gravitydirection = {0, 0, -1};
	vec3_t edgevec;
	aas_area_t *area1, *area2;
	aas_face_t *face1, *face2;
	aas_edge_t *edge;
	aas_plane_t *plane2;
	aas_lreachability_t lr, *lreach;

	if (!AAS_AreaGrounded(area1num) || !AAS_AreaGrounded(area2num)) return qfalse;

	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];
	//if the areas are not near anough in the x-y direction
	for (i = 0; i < 2; i++)
	{
		if (area1->mins[i] > area2->maxs[i] + 10) return qfalse;
		if (area1->maxs[i] < area2->mins[i] - 10) return qfalse;
	} //end for
	//if area 2 is too high above area 1
	if (area2->mins[2] > area1->maxs[2]) return qfalse;
	//
	VectorCopy(gravitydirection, invgravity);
	VectorInverse(invgravity);
	//
	bestheight = 99999;
	bestlength = 0;
	foundreach = qfalse;
	Com_Memset(&lr, 0, sizeof(aas_lreachability_t)); //make the compiler happy
	//
	//check if the areas have ground faces with a common edge
	//if existing use the lowest common edge for a reachability link
	for (i = 0; i < area1->numfaces; i++)
	{
		face1 = &aasworld.faces[abs(aasworld.faceindex[area1->firstface + i])];
		if (!(face1->faceflags & FACE_GROUND)) continue;
		//
		for (j = 0; j < area2->numfaces; j++)
		{
			face2 = &aasworld.faces[abs(aasworld.faceindex[area2->firstface + j])];
			if (!(face2->faceflags & FACE_GROUND)) continue;
			//if there is a common edge
			for (edgenum1 = 0; edgenum1 < face1->numedges; edgenum1++)
			{
				for (edgenum2 = 0; edgenum2 < face2->numedges; edgenum2++)
				{
					if (abs(aasworld.edgeindex[face1->firstedge + edgenum1]) !=
							abs(aasworld.edgeindex[face2->firstedge + edgenum2]))
									continue;
					edgenum = aasworld.edgeindex[face1->firstedge + edgenum1];
					side = edgenum < 0;
					edge = &aasworld.edges[abs(edgenum)];
					//get the length of the edge
					VectorSubtract(aasworld.vertexes[edge->v[1]],
								aasworld.vertexes[edge->v[0]], dir);
					length = VectorLength(dir);
					//get the start point
					VectorAdd(aasworld.vertexes[edge->v[0]],
								aasworld.vertexes[edge->v[1]], start);
					VectorScale(start, 0.5, start);
					VectorCopy(start, end);
					//get the end point several units inside area2
					//and the start point several units inside area1
					//NOTE: normal is pointing into area2 because the
					//face edges are stored counter clockwise
					VectorSubtract(aasworld.vertexes[edge->v[side]],
								aasworld.vertexes[edge->v[!side]], edgevec);
					plane2 = &aasworld.planes[face2->planenum];
					CrossProduct(edgevec, plane2->normal, normal);
					VectorNormalize(normal);
					//
					//VectorMA(start, -1, normal, start);
					VectorMA(end, INSIDEUNITS_WALKEND, normal, end);
					VectorMA(start, INSIDEUNITS_WALKSTART, normal, start);
					end[2] += 0.125;
					//
					height = DotProduct(invgravity, start);
					//NOTE: if there's nearby solid or a gap area after this area
					//disabled this crap
					//if (AAS_NearbySolidOrGap(start, end)) height += 200;
					//NOTE: disabled because it disables reachabilities to very small areas
					//if (AAS_PointAreaNum(end) != area2num) continue;
					//get the longest lowest edge
					if (height < bestheight ||
							(height < bestheight + 1 && length > bestlength))
					{
						bestheight = height;
						bestlength = length;
						//create a new reachability link
						lr.areanum = area2num;
						lr.facenum = 0;
						lr.edgenum = edgenum;
						VectorCopy(start, lr.start);
						VectorCopy(end, lr.end);
						lr.traveltype = TRAVEL_WALK;
						lr.traveltime = 1;
						foundreach = qtrue;
					} //end if
				} //end for
			} //end for
		} //end for
	} //end for
	if (foundreach)
	{
		//create a new reachability link
		lreach = AAS_AllocReachability();
		if (!lreach) return qfalse;
		lreach->areanum = lr.areanum;
		lreach->facenum = lr.facenum;
		lreach->edgenum = lr.edgenum;
		VectorCopy(lr.start, lreach->start);
		VectorCopy(lr.end, lreach->end);
		lreach->traveltype = lr.traveltype;
		lreach->traveltime = lr.traveltime;
		lreach->next = areareachability[area1num];
		areareachability[area1num] = lreach;
		//if going into a crouch area
		if (!AAS_AreaCrouch(area1num) && AAS_AreaCrouch(area2num))
		{
			lreach->traveltime += aassettings.rs_startcrouch;
		} //end if
		/*
		//NOTE: if there's nearby solid or a gap area after this area
		if (!AAS_NearbySolidOrGap(lreach->start, lreach->end))
		{
			lreach->traveltime += 100;
		} //end if
		*/
		//avoid rather small areas
		//if (AAS_AreaGroundFaceArea(lreach->areanum) < 500) lreach->traveltime += 100;
		//
		reach_equalfloor++;
		return qtrue;
	} //end if
	return qfalse;
} //end of the function AAS_Reachability_EqualFloorHeight
//===========================================================================
// searches step, barrier, waterjump and walk off ledge reachabilities
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(int area1num, int area2num)
{
	int i, j, k, l, edge1num, edge2num, areas[10], numareas;
	int ground_bestarea2groundedgenum, ground_foundreach;
	int water_bestarea2groundedgenum, water_foundreach;
	int side1, area1swim, faceside1, groundface1num;
	float dist, dist1, dist2, diff, invgravitydot, ortdot;
	float x1, x2, x3, x4, y1, y2, y3, y4, tmp, y;
	float length, ground_bestlength, water_bestlength, ground_bestdist, water_bestdist;
	vec3_t v1, v2, v3, v4, tmpv, p1area1, p1area2, p2area1, p2area2;
	vec3_t normal, ort, edgevec, start, end, dir;
	vec3_t ground_beststart, ground_bestend, ground_bestnormal;
	vec3_t water_beststart, water_bestend, water_bestnormal;
	vec3_t invgravity = {0, 0, 1};
	vec3_t testpoint;
	aas_plane_t *plane;
	aas_area_t *area1, *area2;
	aas_face_t *groundface1, *groundface2, *ground_bestface1, *water_bestface1;
	aas_edge_t *edge1, *edge2;
	aas_lreachability_t *lreach;
	aas_trace_t trace;

	//must be able to walk or swim in the first area
	if (!AAS_AreaGrounded(area1num) && !AAS_AreaSwim(area1num)) return qfalse;
	//
	if (!AAS_AreaGrounded(area2num) && !AAS_AreaSwim(area2num)) return qfalse;
	//
	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];
	//if the first area contains a liquid
	area1swim = AAS_AreaSwim(area1num);
	//if the areas are not near anough in the x-y direction
	for (i = 0; i < 2; i++)
	{
		if (area1->mins[i] > area2->maxs[i] + 10) return qfalse;
		if (area1->maxs[i] < area2->mins[i] - 10) return qfalse;
	} //end for
	//
	ground_foundreach = qfalse;
	ground_bestdist = 99999;
	ground_bestlength = 0;
	ground_bestarea2groundedgenum = 0;
	//
	water_foundreach = qfalse;
	water_bestdist = 99999;
	water_bestlength = 0;
	water_bestarea2groundedgenum = 0;
	//
	for (i = 0; i < area1->numfaces; i++)
	{
		groundface1num = aasworld.faceindex[area1->firstface + i];
		faceside1 = groundface1num < 0;
		groundface1 = &aasworld.faces[abs(groundface1num)];
		//if this isn't a ground face
		if (!(groundface1->faceflags & FACE_GROUND))
		{
			//if we can swim in the first area
			if (area1swim)
			{
				//face plane must be more or less horizontal
				plane = &aasworld.planes[groundface1->planenum ^ (!faceside1)];
				if (DotProduct(plane->normal, invgravity) < 0.7) continue;
			} //end if
			else
			{
				//if we can't swim in the area it must be a ground face
				continue;
			} //end else
		} //end if
		//
		for (k = 0; k < groundface1->numedges; k++)
		{
			edge1num = aasworld.edgeindex[groundface1->firstedge + k];
			side1 = (edge1num < 0);
			//NOTE: for water faces we must take the side area 1 is
			// on into account because the face is shared and doesn't
			// have to be oriented correctly
			if (!(groundface1->faceflags & FACE_GROUND)) side1 = (side1 == faceside1);
			edge1num = abs(edge1num);
			edge1 = &aasworld.edges[edge1num];
			//vertexes of the edge
			VectorCopy(aasworld.vertexes[edge1->v[!side1]], v1);
			VectorCopy(aasworld.vertexes[edge1->v[side1]], v2);
			//get a vertical plane through the edge
			//NOTE: normal is pointing into area 2 because the
			//face edges are stored counter clockwise
			VectorSubtract(v2, v1, edgevec);
			CrossProduct(edgevec, invgravity, normal);
			VectorNormalize(normal);
			dist = DotProduct(normal, v1);
			//check the faces from the second area
			for (j = 0; j < area2->numfaces; j++)
			{
				groundface2 = &aasworld.faces[abs(aasworld.faceindex[area2->firstface + j])];
				//must be a ground face
				if (!(groundface2->faceflags & FACE_GROUND)) continue;
				//check the edges of this ground face
				for (l = 0; l < groundface2->numedges; l++)
				{
					edge2num = abs(aasworld.edgeindex[groundface2->firstedge + l]);
					edge2 = &aasworld.edges[edge2num];
					//vertexes of the edge
					VectorCopy(aasworld.vertexes[edge2->v[0]], v3);
					VectorCopy(aasworld.vertexes[edge2->v[1]], v4);
					//check the distance between the two points and the vertical plane
					//through the edge of area1
					diff = DotProduct(normal, v3) - dist;
					if (diff < -0.1 || diff > 0.1) continue;
					diff = DotProduct(normal, v4) - dist;
					if (diff < -0.1 || diff > 0.1) continue;
					//
					//project the two ground edges into the step side plane
					//and calculate the shortest distance between the two
					//edges if they overlap in the direction orthogonal to
					//the gravity direction
					CrossProduct(invgravity, normal, ort);
					invgravitydot = DotProduct(invgravity, invgravity);
					ortdot = DotProduct(ort, ort);
					//projection into the step plane
					//NOTE: since gravity is vertical this is just the z coordinate
					y1 = v1[2];//DotProduct(v1, invgravity) / invgravitydot;
					y2 = v2[2];//DotProduct(v2, invgravity) / invgravitydot;
					y3 = v3[2];//DotProduct(v3, invgravity) / invgravitydot;
					y4 = v4[2];//DotProduct(v4, invgravity) / invgravitydot;
					//
					x1 = DotProduct(v1, ort) / ortdot;
					x2 = DotProduct(v2, ort) / ortdot;
					x3 = DotProduct(v3, ort) / ortdot;
					x4 = DotProduct(v4, ort) / ortdot;
					//
					if (x1 > x2)
					{
						tmp = x1; x1 = x2; x2 = tmp;
						tmp = y1; y1 = y2; y2 = tmp;
						VectorCopy(v1, tmpv); VectorCopy(v2, v1); VectorCopy(tmpv, v2);
					} //end if
					if (x3 > x4)
					{
						tmp = x3; x3 = x4; x4 = tmp;
						tmp = y3; y3 = y4; y4 = tmp;
						VectorCopy(v3, tmpv); VectorCopy(v4, v3); VectorCopy(tmpv, v4);
					} //end if
					//if the two projected edge lines have no overlap
					if (x2 <= x3 || x4 <= x1)
					{
//						Log_Write("lines no overlap: from area %d to %d\r\n", area1num, area2num);
						continue;
					} //end if
					//if the two lines fully overlap
					if ((x1 - 0.5 < x3 && x4 < x2 + 0.5) &&
							(x3 - 0.5 < x1 && x2 < x4 + 0.5))
					{
						dist1 = y3 - y1;
						dist2 = y4 - y2;
						VectorCopy(v1, p1area1);
						VectorCopy(v2, p2area1);
						VectorCopy(v3, p1area2);
						VectorCopy(v4, p2area2);
					} //end if
					else
					{
						//if the points are equal
						if (x1 > x3 - 0.1 && x1 < x3 + 0.1)
						{
							dist1 = y3 - y1;
							VectorCopy(v1, p1area1);
							VectorCopy(v3, p1area2);
						} //end if
						else if (x1 < x3)
						{
							y = y1 + (x3 - x1) * (y2 - y1) / (x2 - x1);
							dist1 = y3 - y;
							VectorCopy(v3, p1area1);
							p1area1[2] = y;
							VectorCopy(v3, p1area2);
						} //end if
						else
						{
							y = y3 + (x1 - x3) * (y4 - y3) / (x4 - x3);
							dist1 = y - y1;
							VectorCopy(v1, p1area1);
							VectorCopy(v1, p1area2);
							p1area2[2] = y;
						} //end if
						//if the points are equal
						if (x2 > x4 - 0.1 && x2 < x4 + 0.1)
						{
							dist2 = y4 - y2;
							VectorCopy(v2, p2area1);
							VectorCopy(v4, p2area2);
						} //end if
						else if (x2 < x4)
						{
							y = y3 + (x2 - x3) * (y4 - y3) / (x4 - x3);
							dist2 = y - y2;
							VectorCopy(v2, p2area1);
							VectorCopy(v2, p2area2);
							p2area2[2] = y;
						} //end if
						else
						{
							y = y1 + (x4 - x1) * (y2 - y1) / (x2 - x1);
							dist2 = y4 - y;
							VectorCopy(v4, p2area1);
							p2area1[2] = y;
							VectorCopy(v4, p2area2);
						} //end else
					} //end else
					//if both distances are pretty much equal
					//then we take the middle of the points
					if (dist1 > dist2 - 1 && dist1 < dist2 + 1)
					{
						dist = dist1;
						VectorAdd(p1area1, p2area1, start);
						VectorScale(start, 0.5, start);
						VectorAdd(p1area2, p2area2, end);
						VectorScale(end, 0.5, end);
					} //end if
					else if (dist1 < dist2)
					{
						dist = dist1;
						VectorCopy(p1area1, start);
						VectorCopy(p1area2, end);
					} //end else if
					else
					{
						dist = dist2;
						VectorCopy(p2area1, start);
						VectorCopy(p2area2, end);
					} //end else
					//get the length of the overlapping part of the edges of the two areas
					VectorSubtract(p2area2, p1area2, dir);
					length = VectorLength(dir);
					//
					if (groundface1->faceflags & FACE_GROUND)
					{
						//if the vertical distance is smaller
						if (dist < ground_bestdist ||
								//or the vertical distance is pretty much the same
								//but the overlapping part of the edges is longer
								(dist < ground_bestdist + 1 && length > ground_bestlength))
						{
							ground_bestdist = dist;
							ground_bestlength = length;
							ground_foundreach = qtrue;
							ground_bestarea2groundedgenum = edge1num;
							ground_bestface1 = groundface1;
							//best point towards area1
							VectorCopy(start, ground_beststart);
							//normal is pointing into area2
							VectorCopy(normal, ground_bestnormal);
							//best point towards area2
							VectorCopy(end, ground_bestend);
						} //end if
					} //end if
					else
					{
						//if the vertical distance is smaller
						if (dist < water_bestdist ||
								//or the vertical distance is pretty much the same
								//but the overlapping part of the edges is longer
								(dist < water_bestdist + 1 && length > water_bestlength))
						{
							water_bestdist = dist;
							water_bestlength = length;
							water_foundreach = qtrue;
							water_bestarea2groundedgenum = edge1num;
							water_bestface1 = groundface1;
							//best point towards area1
							VectorCopy(start, water_beststart);
							//normal is pointing into area2
							VectorCopy(normal, water_bestnormal);
							//best point towards area2
							VectorCopy(end, water_bestend);
						} //end if
					} //end else
				} //end for
			} //end for
		} //end for
	} //end for
	//
	// NOTE: swim reachabilities are already filtered out
	//
	// Steps
	//
	//        ---------
	//        |          step height -> TRAVEL_WALK
	//--------|
	//
	//        ---------
	//~~~~~~~~|          step height and low water -> TRAVEL_WALK
	//--------|
	//
	//~~~~~~~~~~~~~~~~~~
	//        ---------
	//        |          step height and low water up to the step -> TRAVEL_WALK
	//--------|
	//
	//check for a step reachability
	if (ground_foundreach)
	{
		//if area2 is higher but lower than the maximum step height
		//NOTE: ground_bestdist >= 0 also catches equal floor reachabilities
		if (ground_bestdist >= 0 && ground_bestdist < aassettings.phys_maxstep)
		{
			//create walk reachability from area1 to area2
			lreach = AAS_AllocReachability();
			if (!lreach) return qfalse;
			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = ground_bestarea2groundedgenum;
			VectorMA(ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start);
			VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
			lreach->traveltype = TRAVEL_WALK;
			lreach->traveltime = 0;//1;
			//if going into a crouch area
			if (!AAS_AreaCrouch(area1num) && AAS_AreaCrouch(area2num))
			{
				lreach->traveltime += aassettings.rs_startcrouch;
			} //end if
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//NOTE: if there's nearby solid or a gap area after this area
			/*
			if (!AAS_NearbySolidOrGap(lreach->start, lreach->end))
			{
				lreach->traveltime += 100;
			} //end if
			*/
			//avoid rather small areas
			//if (AAS_AreaGroundFaceArea(lreach->areanum) < 500) lreach->traveltime += 100;
			//
			reach_step++;
			return qtrue;
		} //end if
	} //end if
	//
	// Water Jumps
	//
	//        ---------
	//        |
	//~~~~~~~~|
	//        |
	//        |          higher than step height and water up to waterjump height -> TRAVEL_WATERJUMP
	//--------|
	//
	//~~~~~~~~~~~~~~~~~~
	//        ---------
	//        |
	//        |
	//        |
	//        |          higher than step height and low water up to the step -> TRAVEL_WATERJUMP
	//--------|
	//
	//check for a waterjump reachability
	if (water_foundreach)
	{
		//get a test point a little bit towards area1
		VectorMA(water_bestend, -INSIDEUNITS, water_bestnormal, testpoint);
		//go down the maximum waterjump height
		testpoint[2] -= aassettings.phys_maxwaterjump;
		//if there IS water the sv_maxwaterjump height below the bestend point
		if (aasworld.areasettings[AAS_PointAreaNum(testpoint)].areaflags & AREA_LIQUID)
		{
			//don't create rediculous water jump reachabilities from areas very far below
			//the water surface
			if (water_bestdist < aassettings.phys_maxwaterjump + 24)
			{
				//waterjumping from or towards a crouch only area is not possible in Quake2
				if ((aasworld.areasettings[area1num].presencetype & PRESENCE_NORMAL) &&
						(aasworld.areasettings[area2num].presencetype & PRESENCE_NORMAL))
				{
					//create water jump reachability from area1 to area2
					lreach = AAS_AllocReachability();
					if (!lreach) return qfalse;
					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = water_bestarea2groundedgenum;
					VectorCopy(water_beststart, lreach->start);
					VectorMA(water_bestend, INSIDEUNITS_WATERJUMP, water_bestnormal, lreach->end);
					lreach->traveltype = TRAVEL_WATERJUMP;
					lreach->traveltime = aassettings.rs_waterjump;
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					//we've got another waterjump reachability
					reach_waterjump++;
					return qtrue;
				} //end if
			} //end if
		} //end if
	} //end if
	//
	// Barrier Jumps
	//
	//        ---------
	//        |
	//        |
	//        |
	//        |         higher than step height lower than barrier height -> TRAVEL_BARRIERJUMP
	//--------|
	//
	//        ---------
	//        |
	//        |
	//        |
	//~~~~~~~~|         higher than step height lower than barrier height
	//--------|         and a thin layer of water in the area to jump from -> TRAVEL_BARRIERJUMP
	//
	//check for a barrier jump reachability
	if (ground_foundreach)
	{
		//if area2 is higher but lower than the maximum barrier jump height
		if (ground_bestdist > 0 && ground_bestdist < aassettings.phys_maxbarrier)
		{
			//if no water in area1 or a very thin layer of water on the ground
			if (!water_foundreach || (ground_bestdist - water_bestdist < 16))
			{
				//cannot perform a barrier jump towards or from a crouch area in Quake2
				if (!AAS_AreaCrouch(area1num) && !AAS_AreaCrouch(area2num))
				{
					//create barrier jump reachability from area1 to area2
					lreach = AAS_AllocReachability();
					if (!lreach) return qfalse;
					lreach->areanum = area2num;
					lreach->facenum = 0;
					lreach->edgenum = ground_bestarea2groundedgenum;
					VectorMA(ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start);
					VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
					lreach->traveltype = TRAVEL_BARRIERJUMP;
					lreach->traveltime = aassettings.rs_barrierjump;//AAS_BarrierJumpTravelTime();
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					//we've got another barrierjump reachability
					reach_barrier++;
					return qtrue;
				} //end if
			} //end if
		} //end if
	} //end if
	//
	// Walk and Walk Off Ledge
	//
	//--------|
	//        |          can walk or step back -> TRAVEL_WALK
	//        ---------
	//
	//--------|
	//        |
	//        |
	//        |
	//        |          cannot walk/step back -> TRAVEL_WALKOFFLEDGE
	//        ---------
	//
	//--------|
	//        |
	//        |~~~~~~~~
	//        |
	//        |          cannot step back but can waterjump back -> TRAVEL_WALKOFFLEDGE
	//        ---------  FIXME: create TRAVEL_WALK reach??
	//
	//check for a walk or walk off ledge reachability
	if (ground_foundreach)
	{
		if (ground_bestdist < 0)
		{
			if (ground_bestdist > -aassettings.phys_maxstep)
			{
				//create walk reachability from area1 to area2
				lreach = AAS_AllocReachability();
				if (!lreach) return qfalse;
				lreach->areanum = area2num;
				lreach->facenum = 0;
				lreach->edgenum = ground_bestarea2groundedgenum;
				VectorMA(ground_beststart, INSIDEUNITS_WALKSTART, ground_bestnormal, lreach->start);
				VectorMA(ground_bestend, INSIDEUNITS_WALKEND, ground_bestnormal, lreach->end);
				lreach->traveltype = TRAVEL_WALK;
				lreach->traveltime = 1;
				lreach->next = areareachability[area1num];
				areareachability[area1num] = lreach;
				//we've got another walk reachability
				reach_walk++;
				return qtrue;
			} //end if
			// if no maximum fall height set or less than the max
			if (!aassettings.rs_maxfallheight || fabs(ground_bestdist) < aassettings.rs_maxfallheight) {
				//trace a bounding box vertically to check for solids
				VectorMA(ground_bestend, INSIDEUNITS, ground_bestnormal, ground_bestend);
				VectorCopy(ground_bestend, start);
				start[2] = ground_beststart[2];
				VectorCopy(ground_bestend, end);
				end[2] += 4;
				trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
				//if no solids were found
				if (!trace.startsolid && trace.fraction >= 1.0)
				{
					//the trace end point must be in the goal area
					trace.endpos[2] += 1;
					if (AAS_PointAreaNum(trace.endpos) == area2num)
					{
						//if not going through a cluster portal
						numareas = AAS_TraceAreas(start, end, areas, NULL, sizeof(areas) / sizeof(int));
						for (i = 0; i < numareas; i++)
							if (AAS_AreaClusterPortal(areas[i]))
								break;
						if (i >= numareas)
						{
							//create a walk off ledge reachability from area1 to area2
							lreach = AAS_AllocReachability();
							if (!lreach) return qfalse;
							lreach->areanum = area2num;
							lreach->facenum = 0;
							lreach->edgenum = ground_bestarea2groundedgenum;
							VectorCopy(ground_beststart, lreach->start);
							VectorCopy(ground_bestend, lreach->end);
							lreach->traveltype = TRAVEL_WALKOFFLEDGE;
							lreach->traveltime = aassettings.rs_startwalkoffledge + fabs(ground_bestdist) * 50 / aassettings.phys_gravity;
							//if falling from too high and not falling into water
							if (!AAS_AreaSwim(area2num) && !AAS_AreaJumpPad(area2num))
							{
								if (AAS_FallDelta(ground_bestdist) > aassettings.phys_falldelta5)
								{
									lreach->traveltime += aassettings.rs_falldamage5;
								} //end if
								if (AAS_FallDelta(ground_bestdist) > aassettings.phys_falldelta10)
								{
									lreach->traveltime += aassettings.rs_falldamage10;
								} //end if
							} //end if
							lreach->next = areareachability[area1num];
							areareachability[area1num] = lreach;
							//
							reach_walkoffledge++;
							//NOTE: don't create a weapon (rl, bfg) jump reachability here
							//because it interferes with other reachabilities
							//like the ladder reachability
							return qtrue;
						} //end if
					} //end if
				} //end if
			} //end if
		} //end else
	} //end if
	return qfalse;
} //end of the function AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge
//===========================================================================
// returns the distance between the two vectors
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float VectorDistance(vec3_t v1, vec3_t v2)
{
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
} //end of the function VectorDistance
//===========================================================================
// returns true if the first vector is between the last two vectors
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int VectorBetweenVectors(vec3_t v, vec3_t v1, vec3_t v2)
{
	vec3_t dir1, dir2;

	VectorSubtract(v, v1, dir1);
	VectorSubtract(v, v2, dir2);
	return (DotProduct(dir1, dir2) <= 0);
} //end of the function VectorBetweenVectors
//===========================================================================
// returns the mid point between the two vectors
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void VectorMiddle(vec3_t v1, vec3_t v2, vec3_t middle)
{
	VectorAdd(v1, v2, middle);
	VectorScale(middle, 0.5, middle);
} //end of the function VectorMiddle
//===========================================================================
// calculate a range of points closest to each other on both edges
//
// Parameter:			beststart1		start of the range of points on edge v1-v2
//						beststart2		end of the range of points  on edge v1-v2
//						bestend1		start of the range of points on edge v3-v4
//						bestend2		end of the range of points  on edge v3-v4
//						bestdist		best distance so far
// Returns:				-
// Changes Globals:		-
//===========================================================================
/*
float AAS_ClosestEdgePoints(vec3_t v1, vec3_t v2, vec3_t v3, vec3_t v4,
							aas_plane_t *plane1, aas_plane_t *plane2,
							vec3_t beststart, vec3_t bestend, float bestdist)
{
	vec3_t dir1, dir2, p1, p2, p3, p4;
	float a1, a2, b1, b2, dist;
	int founddist;

	//edge vectors
	VectorSubtract(v2, v1, dir1);
	VectorSubtract(v4, v3, dir2);
	//get the horizontal directions
	dir1[2] = 0;
	dir2[2] = 0;
	//
	// p1 = point on an edge vector of area2 closest to v1
	// p2 = point on an edge vector of area2 closest to v2
	// p3 = point on an edge vector of area1 closest to v3
	// p4 = point on an edge vector of area1 closest to v4
	//
	if (dir2[0])
	{
		a2 = dir2[1] / dir2[0];
		b2 = v3[1] - a2 * v3[0];
		//point on the edge vector of area2 closest to v1
		p1[0] = (DotProduct(v1, dir2) - (a2 * dir2[0] + b2 * dir2[1])) / dir2[0];
		p1[1] = a2 * p1[0] + b2;
		//point on the edge vector of area2 closest to v2
		p2[0] = (DotProduct(v2, dir2) - (a2 * dir2[0] + b2 * dir2[1])) / dir2[0];
		p2[1] = a2 * p2[0] + b2;
	} //end if
	else
	{
		//point on the edge vector of area2 closest to v1
		p1[0] = v3[0];
		p1[1] = v1[1];
		//point on the edge vector of area2 closest to v2
		p2[0] = v3[0];
		p2[1] = v2[1];
	} //end else
	//
	if (dir1[0])
	{
		//
		a1 = dir1[1] / dir1[0];
		b1 = v1[1] - a1 * v1[0];
		//point on the edge vector of area1 closest to v3
		p3[0] = (DotProduct(v3, dir1) - (a1 * dir1[0] + b1 * dir1[1])) / dir1[0];
		p3[1] = a1 * p3[0] + b1;
		//point on the edge vector of area1 closest to v4
		p4[0] = (DotProduct(v4, dir1) - (a1 * dir1[0] + b1 * dir1[1])) / dir1[0];
		p4[1] = a1 * p4[0] + b1;
	} //end if
	else
	{
		//point on the edge vector of area1 closest to v3
		p3[0] = v1[0];
		p3[1] = v3[1];
		//point on the edge vector of area1 closest to v4
		p4[0] = v1[0];
		p4[1] = v4[1];
	} //end else
	//start with zero z-coordinates
	p1[2] = 0;
	p2[2] = 0;
	p3[2] = 0;
	p4[2] = 0;
	//calculate the z-coordinates from the ground planes
	p1[2] = (plane2->dist - DotProduct(plane2->normal, p1)) / plane2->normal[2];
	p2[2] = (plane2->dist - DotProduct(plane2->normal, p2)) / plane2->normal[2];
	p3[2] = (plane1->dist - DotProduct(plane1->normal, p3)) / plane1->normal[2];
	p4[2] = (plane1->dist - DotProduct(plane1->normal, p4)) / plane1->normal[2];
	//
	founddist = qfalse;
	//
	if (VectorBetweenVectors(p1, v3, v4))
	{
		dist = VectorDistance(v1, p1);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			VectorMiddle(beststart, v1, beststart);
			VectorMiddle(bestend, p1, bestend);
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart);
			VectorCopy(p1, bestend);
		} //end if
		founddist = qtrue;
	} //end if
	if (VectorBetweenVectors(p2, v3, v4))
	{
		dist = VectorDistance(v2, p2);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			VectorMiddle(beststart, v2, beststart);
			VectorMiddle(bestend, p2, bestend);
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart);
			VectorCopy(p2, bestend);
		} //end if
		founddist = qtrue;
	} //end else if
	if (VectorBetweenVectors(p3, v1, v2))
	{
		dist = VectorDistance(v3, p3);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			VectorMiddle(beststart, p3, beststart);
			VectorMiddle(bestend, v3, bestend);
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(p3, beststart);
			VectorCopy(v3, bestend);
		} //end if
		founddist = qtrue;
	} //end else if
	if (VectorBetweenVectors(p4, v1, v2))
	{
		dist = VectorDistance(v4, p4);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			VectorMiddle(beststart, p4, beststart);
			VectorMiddle(bestend, v4, bestend);
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(p4, beststart);
			VectorCopy(v4, bestend);
		} //end if
		founddist = qtrue;
	} //end else if
	//if no shortest distance was found the shortest distance
	//is between one of the vertexes of edge1 and one of edge2
	if (!founddist)
	{
		dist = VectorDistance(v1, v3);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart);
			VectorCopy(v3, bestend);
		} //end if
		dist = VectorDistance(v1, v4);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart);
			VectorCopy(v4, bestend);
		} //end if
		dist = VectorDistance(v2, v3);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart);
			VectorCopy(v3, bestend);
		} //end if
		dist = VectorDistance(v2, v4);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart);
			VectorCopy(v4, bestend);
		} //end if
	} //end if
	return bestdist;
} //end of the function AAS_ClosestEdgePoints*/

float AAS_ClosestEdgePoints(vec3_t v1, vec3_t v2, vec3_t v3, vec3_t v4,
							aas_plane_t *plane1, aas_plane_t *plane2,
							vec3_t beststart1, vec3_t bestend1,
							vec3_t beststart2, vec3_t bestend2, float bestdist)
{
	vec3_t dir1, dir2, p1, p2, p3, p4;
	float a1, a2, b1, b2, dist, dist1, dist2;
	int founddist;

	//edge vectors
	VectorSubtract(v2, v1, dir1);
	VectorSubtract(v4, v3, dir2);
	//get the horizontal directions
	dir1[2] = 0;
	dir2[2] = 0;
	//
	// p1 = point on an edge vector of area2 closest to v1
	// p2 = point on an edge vector of area2 closest to v2
	// p3 = point on an edge vector of area1 closest to v3
	// p4 = point on an edge vector of area1 closest to v4
	//
	if (dir2[0])
	{
		a2 = dir2[1] / dir2[0];
		b2 = v3[1] - a2 * v3[0];
		//point on the edge vector of area2 closest to v1
		p1[0] = (DotProduct(v1, dir2) - (a2 * dir2[0] + b2 * dir2[1])) / dir2[0];
		p1[1] = a2 * p1[0] + b2;
		//point on the edge vector of area2 closest to v2
		p2[0] = (DotProduct(v2, dir2) - (a2 * dir2[0] + b2 * dir2[1])) / dir2[0];
		p2[1] = a2 * p2[0] + b2;
	} //end if
	else
	{
		//point on the edge vector of area2 closest to v1
		p1[0] = v3[0];
		p1[1] = v1[1];
		//point on the edge vector of area2 closest to v2
		p2[0] = v3[0];
		p2[1] = v2[1];
	} //end else
	//
	if (dir1[0])
	{
		//
		a1 = dir1[1] / dir1[0];
		b1 = v1[1] - a1 * v1[0];
		//point on the edge vector of area1 closest to v3
		p3[0] = (DotProduct(v3, dir1) - (a1 * dir1[0] + b1 * dir1[1])) / dir1[0];
		p3[1] = a1 * p3[0] + b1;
		//point on the edge vector of area1 closest to v4
		p4[0] = (DotProduct(v4, dir1) - (a1 * dir1[0] + b1 * dir1[1])) / dir1[0];
		p4[1] = a1 * p4[0] + b1;
	} //end if
	else
	{
		//point on the edge vector of area1 closest to v3
		p3[0] = v1[0];
		p3[1] = v3[1];
		//point on the edge vector of area1 closest to v4
		p4[0] = v1[0];
		p4[1] = v4[1];
	} //end else
	//start with zero z-coordinates
	p1[2] = 0;
	p2[2] = 0;
	p3[2] = 0;
	p4[2] = 0;
	//calculate the z-coordinates from the ground planes
	p1[2] = (plane2->dist - DotProduct(plane2->normal, p1)) / plane2->normal[2];
	p2[2] = (plane2->dist - DotProduct(plane2->normal, p2)) / plane2->normal[2];
	p3[2] = (plane1->dist - DotProduct(plane1->normal, p3)) / plane1->normal[2];
	p4[2] = (plane1->dist - DotProduct(plane1->normal, p4)) / plane1->normal[2];
	//
	founddist = qfalse;
	//
	if (VectorBetweenVectors(p1, v3, v4))
	{
		dist = VectorDistance(v1, p1);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, v1);
			dist2 = VectorDistance(beststart2, v1);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2)) VectorCopy(v1, beststart2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2)) VectorCopy(v1, beststart1);
			} //end else
			dist1 = VectorDistance(bestend1, p1);
			dist2 = VectorDistance(bestend2, p1);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2)) VectorCopy(p1, bestend2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2)) VectorCopy(p1, bestend1);
			} //end else
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart1);
			VectorCopy(v1, beststart2);
			VectorCopy(p1, bestend1);
			VectorCopy(p1, bestend2);
		} //end if
		founddist = qtrue;
	} //end if
	if (VectorBetweenVectors(p2, v3, v4))
	{
		dist = VectorDistance(v2, p2);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, v2);
			dist2 = VectorDistance(beststart2, v2);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2)) VectorCopy(v2, beststart2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2)) VectorCopy(v2, beststart1);
			} //end else
			dist1 = VectorDistance(bestend1, p2);
			dist2 = VectorDistance(bestend2, p2);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2)) VectorCopy(p2, bestend2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2)) VectorCopy(p2, bestend1);
			} //end else
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart1);
			VectorCopy(v2, beststart2);
			VectorCopy(p2, bestend1);
			VectorCopy(p2, bestend2);
		} //end if
		founddist = qtrue;
	} //end else if
	if (VectorBetweenVectors(p3, v1, v2))
	{
		dist = VectorDistance(v3, p3);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, p3);
			dist2 = VectorDistance(beststart2, p3);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2)) VectorCopy(p3, beststart2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2)) VectorCopy(p3, beststart1);
			} //end else
			dist1 = VectorDistance(bestend1, v3);
			dist2 = VectorDistance(bestend2, v3);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2)) VectorCopy(v3, bestend2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2)) VectorCopy(v3, bestend1);
			} //end else
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(p3, beststart1);
			VectorCopy(p3, beststart2);
			VectorCopy(v3, bestend1);
			VectorCopy(v3, bestend2);
		} //end if
		founddist = qtrue;
	} //end else if
	if (VectorBetweenVectors(p4, v1, v2))
	{
		dist = VectorDistance(v4, p4);
		if (dist > bestdist - 0.5 && dist < bestdist + 0.5)
		{
			dist1 = VectorDistance(beststart1, p4);
			dist2 = VectorDistance(beststart2, p4);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(beststart1, beststart2)) VectorCopy(p4, beststart2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(beststart1, beststart2)) VectorCopy(p4, beststart1);
			} //end else
			dist1 = VectorDistance(bestend1, v4);
			dist2 = VectorDistance(bestend2, v4);
			if (dist1 > dist2)
			{
				if (dist1 > VectorDistance(bestend1, bestend2)) VectorCopy(v4, bestend2);
			} //end if
			else
			{
				if (dist2 > VectorDistance(bestend1, bestend2)) VectorCopy(v4, bestend1);
			} //end else
		} //end if
		else if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(p4, beststart1);
			VectorCopy(p4, beststart2);
			VectorCopy(v4, bestend1);
			VectorCopy(v4, bestend2);
		} //end if
		founddist = qtrue;
	} //end else if
	//if no shortest distance was found the shortest distance
	//is between one of the vertexes of edge1 and one of edge2
	if (!founddist)
	{
		dist = VectorDistance(v1, v3);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart1);
			VectorCopy(v1, beststart2);
			VectorCopy(v3, bestend1);
			VectorCopy(v3, bestend2);
		} //end if
		dist = VectorDistance(v1, v4);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v1, beststart1);
			VectorCopy(v1, beststart2);
			VectorCopy(v4, bestend1);
			VectorCopy(v4, bestend2);
		} //end if
		dist = VectorDistance(v2, v3);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart1);
			VectorCopy(v2, beststart2);
			VectorCopy(v3, bestend1);
			VectorCopy(v3, bestend2);
		} //end if
		dist = VectorDistance(v2, v4);
		if (dist < bestdist)
		{
			bestdist = dist;
			VectorCopy(v2, beststart1);
			VectorCopy(v2, beststart2);
			VectorCopy(v4, bestend1);
			VectorCopy(v4, bestend2);
		} //end if
	} //end if
	return bestdist;
} //end of the function AAS_ClosestEdgePoints
//===========================================================================
// creates possible jump reachabilities between the areas
//
// The two closest points on the ground of the areas are calculated
// One of the points will be on an edge of a ground face of area1 and
// one on an edge of a ground face of area2.
// If there is a range of closest points the point in the middle of this range
// is selected.
// Between these two points there must be one or more gaps.
// If the gaps exist a potential jump is predicted.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Jump(int area1num, int area2num)
{
	int i, j, k, l, face1num, face2num, edge1num, edge2num, traveltype;
	int stopevent, areas[10], numareas;
	float phys_jumpvel, maxjumpdistance, maxjumpheight, height, bestdist, speed;
	vec_t *v1, *v2, *v3, *v4;
	vec3_t beststart, beststart2, bestend, bestend2;
	vec3_t teststart, testend, dir, velocity, cmdmove, up = {0, 0, 1}, sidewards;
	aas_area_t *area1, *area2;
	aas_face_t *face1, *face2;
	aas_edge_t *edge1, *edge2;
	aas_plane_t *plane1, *plane2, *plane;
	aas_trace_t trace;
	aas_clientmove_t move;
	aas_lreachability_t *lreach;

	if (!AAS_AreaGrounded(area1num) || !AAS_AreaGrounded(area2num)) return qfalse;
	//cannot jump from or to a crouch area
	if (AAS_AreaCrouch(area1num) || AAS_AreaCrouch(area2num)) return qfalse;
	//
	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];
	//
	phys_jumpvel = aassettings.phys_jumpvel;
	//maximum distance a player can jump
	maxjumpdistance = 2 * AAS_MaxJumpDistance(phys_jumpvel);
	//maximum height a player can jump with the given initial z velocity
	maxjumpheight = AAS_MaxJumpHeight(phys_jumpvel);

	//if the areas are not near anough in the x-y direction
	for (i = 0; i < 2; i++)
	{
		if (area1->mins[i] > area2->maxs[i] + maxjumpdistance) return qfalse;
		if (area1->maxs[i] < area2->mins[i] - maxjumpdistance) return qfalse;
	} //end for
	//if area2 is way to high to jump up to
	if (area2->mins[2] > area1->maxs[2] + maxjumpheight) return qfalse;
	//
	bestdist = 999999;
	//
	for (i = 0; i < area1->numfaces; i++)
	{
		face1num = aasworld.faceindex[area1->firstface + i];
		face1 = &aasworld.faces[abs(face1num)];
		//if not a ground face
		if (!(face1->faceflags & FACE_GROUND)) continue;
		//
		for (j = 0; j < area2->numfaces; j++)
		{
			face2num = aasworld.faceindex[area2->firstface + j];
			face2 = &aasworld.faces[abs(face2num)];
			//if not a ground face
			if (!(face2->faceflags & FACE_GROUND)) continue;
			//
			for (k = 0; k < face1->numedges; k++)
			{
				edge1num = abs(aasworld.edgeindex[face1->firstedge + k]);
				edge1 = &aasworld.edges[edge1num];
				for (l = 0; l < face2->numedges; l++)
				{
					edge2num = abs(aasworld.edgeindex[face2->firstedge + l]);
					edge2 = &aasworld.edges[edge2num];
					//calculate the minimum distance between the two edges
					v1 = aasworld.vertexes[edge1->v[0]];
					v2 = aasworld.vertexes[edge1->v[1]];
					v3 = aasworld.vertexes[edge2->v[0]];
					v4 = aasworld.vertexes[edge2->v[1]];
					//get the ground planes
					plane1 = &aasworld.planes[face1->planenum];
					plane2 = &aasworld.planes[face2->planenum];
					//
					bestdist = AAS_ClosestEdgePoints(v1, v2, v3, v4, plane1, plane2,
														beststart, bestend,
														beststart2, bestend2, bestdist);
				} //end for
			} //end for
		} //end for
	} //end for
	VectorMiddle(beststart, beststart2, beststart);
	VectorMiddle(bestend, bestend2, bestend);
	if (bestdist > 4 && bestdist < maxjumpdistance)
	{
//		Log_Write("shortest distance between %d and %d is %f\r\n", area1num, area2num, bestdist);
		// if very close and almost no height difference then the bot can walk
		if (bestdist <= 48 && fabs(beststart[2] - bestend[2]) < 8)
		{
			speed = 400;
			traveltype = TRAVEL_WALKOFFLEDGE;
		} //end if
		else if (AAS_HorizontalVelocityForJump(0, beststart, bestend, &speed))
		{
			//FIXME: why multiply with 1.2???
			speed *= 1.2f;
			traveltype = TRAVEL_WALKOFFLEDGE;
		} //end else if
		else
		{
			//get the horizontal speed for the jump, if it isn't possible to calculate this
			//speed (the jump is not possible) then there's no jump reachability created
			if (!AAS_HorizontalVelocityForJump(phys_jumpvel, beststart, bestend, &speed))
				return qfalse;
			speed *= 1.05f;
			traveltype = TRAVEL_JUMP;
			//
			//NOTE: test if the horizontal distance isn't too small
			VectorSubtract(bestend, beststart, dir);
			dir[2] = 0;
			if (VectorLength(dir) < 10)
				return qfalse;
		} //end if
		//
		VectorSubtract(bestend, beststart, dir);
		VectorNormalize(dir);
		VectorMA(beststart, 1, dir, teststart);
		//
		VectorCopy(teststart, testend);
		testend[2] -= 100;
		trace = AAS_TraceClientBBox(teststart, testend, PRESENCE_NORMAL, -1);
		//
		if (trace.startsolid)
			return qfalse;
		if (trace.fraction < 1)
		{
			plane = &aasworld.planes[trace.planenum];
			// if the bot can stand on the surface
			if (DotProduct(plane->normal, up) >= 0.7)
			{
				// if no lava or slime below
				if (!(AAS_PointContents(trace.endpos) & (CONTENTS_LAVA|CONTENTS_SLIME)))
				{
					if (teststart[2] - trace.endpos[2] <= aassettings.phys_maxbarrier)
						return qfalse;
				} //end if
			} //end if
		} //end if
		//
		VectorMA(bestend, -1, dir, teststart);
		//
		VectorCopy(teststart, testend);
		testend[2] -= 100;
		trace = AAS_TraceClientBBox(teststart, testend, PRESENCE_NORMAL, -1);
		//
		if (trace.startsolid)
			return qfalse;
		if (trace.fraction < 1)
		{
			plane = &aasworld.planes[trace.planenum];
			// if the bot can stand on the surface
			if (DotProduct(plane->normal, up) >= 0.7)
			{
				// if no lava or slime below
				if (!(AAS_PointContents(trace.endpos) & (CONTENTS_LAVA|CONTENTS_SLIME)))
				{
					if (teststart[2] - trace.endpos[2] <= aassettings.phys_maxbarrier)
						return qfalse;
				} //end if
			} //end if
		} //end if
		//
		// get command movement
		VectorClear(cmdmove);
		if ((traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMP)
			cmdmove[2] = aassettings.phys_jumpvel;
		else
			cmdmove[2] = 0;
		//
		VectorSubtract(bestend, beststart, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		CrossProduct(dir, up, sidewards);
		//
		stopevent = SE_HITGROUND|SE_ENTERWATER|SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE;
		if (!AAS_AreaClusterPortal(area1num) && !AAS_AreaClusterPortal(area2num))
			stopevent |= SE_TOUCHCLUSTERPORTAL;
		//
		for (i = 0; i < 3; i++)
		{
			//
			if (i == 1)
				VectorAdd(testend, sidewards, testend);
			else if (i == 2)
				VectorSubtract(bestend, sidewards, testend);
			else
				VectorCopy(bestend, testend);
			VectorSubtract(testend, beststart, dir);
			dir[2] = 0;
			VectorNormalize(dir);
			VectorScale(dir, speed, velocity);
			//
			AAS_PredictClientMovement(&move, -1, beststart, PRESENCE_NORMAL, qtrue,
										velocity, cmdmove, 3, 30, 0.1f,
										stopevent, 0, qfalse);
			// if prediction time wasn't enough to fully predict the movement
			if (move.frames >= 30)
				return qfalse;
			// don't enter slime or lava and don't fall from too high
			if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA))
				return qfalse;
			// never jump or fall through a cluster portal
			if (move.stopevent & SE_TOUCHCLUSTERPORTAL)
				return qfalse;
			//the end position should be in area2, also test a little bit back
			//because the predicted jump could have rushed through the area
			VectorMA(move.endpos, -64, dir, teststart);
			teststart[2] += 1;
			numareas = AAS_TraceAreas(move.endpos, teststart, areas, NULL, sizeof(areas) / sizeof(int));
			for (j = 0; j < numareas; j++)
			{
				if (areas[j] == area2num)
					break;
			} //end for
			if (j < numareas)
				break;
		}
		if (i >= 3)
			return qfalse;
		//
#ifdef REACH_DEBUG
		//create the reachability
		Log_Write("jump reachability between %d and %d\r\n", area1num, area2num);
#endif //REACH_DEBUG
		//create a new reachability link
		lreach = AAS_AllocReachability();
		if (!lreach) return qfalse;
		lreach->areanum = area2num;
		lreach->facenum = 0;
		lreach->edgenum = 0;
		VectorCopy(beststart, lreach->start);
		VectorCopy(bestend, lreach->end);
		lreach->traveltype = traveltype;

		VectorSubtract(bestend, beststart, dir);
		height = dir[2];
		dir[2] = 0;
		if ((traveltype & TRAVELTYPE_MASK) == TRAVEL_WALKOFFLEDGE && height > VectorLength(dir))
		{
			lreach->traveltime = aassettings.rs_startwalkoffledge + height * 50 / aassettings.phys_gravity;
		}
		else
		{
			lreach->traveltime = aassettings.rs_startjump + VectorDistance(bestend, beststart) * 240 / aassettings.phys_maxwalkvelocity;
		} //end if
		//
		if (!AAS_AreaJumpPad(area2num))
		{
			if (AAS_FallDelta(beststart[2] - bestend[2]) > aassettings.phys_falldelta5)
			{
				lreach->traveltime += aassettings.rs_falldamage5;
			} //end if
			else if (AAS_FallDelta(beststart[2] - bestend[2]) > aassettings.phys_falldelta10)
			{
				lreach->traveltime += aassettings.rs_falldamage10;
			} //end if
		} //end if
		lreach->next = areareachability[area1num];
		areareachability[area1num] = lreach;
		//
		if ((traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMP)
			reach_jump++;
		else
			reach_walkoffledge++;
	} //end if
	return qfalse;
} //end of the function AAS_Reachability_Jump
//===========================================================================
// create a possible ladder reachability from area1 to area2
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Ladder(int area1num, int area2num)
{
	int i, j, k, l, edge1num, edge2num, sharededgenum, lowestedgenum;
	int face1num, face2num, ladderface1num, ladderface2num;
	int ladderface1vertical, ladderface2vertical, firstv;
	float face1area, face2area, bestface1area, bestface2area;
	float phys_jumpvel, maxjumpheight;
	vec3_t area1point, area2point, v1, v2, up = {0, 0, 1};
	vec3_t mid, lowestpoint, start, end, sharededgevec, dir;
	aas_area_t *area1, *area2;
	aas_face_t *face1, *face2, *ladderface1, *ladderface2;
	aas_plane_t *plane1, *plane2;
	aas_edge_t *sharededge, *edge1;
	aas_lreachability_t *lreach;
	aas_trace_t trace;

	if (!AAS_AreaLadder(area1num) || !AAS_AreaLadder(area2num)) return qfalse;
	//
	phys_jumpvel = aassettings.phys_jumpvel;
	//maximum height a player can jump with the given initial z velocity
	maxjumpheight = AAS_MaxJumpHeight(phys_jumpvel);

	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];
	//
	ladderface1 = NULL;
	ladderface2 = NULL;
	ladderface1num = 0; //make compiler happy
	ladderface2num = 0; //make compiler happy
	bestface1area = -9999;
	bestface2area = -9999;
	sharededgenum = 0; //make compiler happy
	lowestedgenum = 0; //make compiler happy
	//
	for (i = 0; i < area1->numfaces; i++)
	{
		face1num = aasworld.faceindex[area1->firstface + i];
		face1 = &aasworld.faces[abs(face1num)];
		//if not a ladder face
		if (!(face1->faceflags & FACE_LADDER)) continue;
		//
		for (j = 0; j < area2->numfaces; j++)
		{
			face2num = aasworld.faceindex[area2->firstface + j];
			face2 = &aasworld.faces[abs(face2num)];
			//if not a ladder face
			if (!(face2->faceflags & FACE_LADDER)) continue;
			//check if the faces share an edge
			for (k = 0; k < face1->numedges; k++)
			{
				edge1num = aasworld.edgeindex[face1->firstedge + k];
				for (l = 0; l < face2->numedges; l++)
				{
					edge2num = aasworld.edgeindex[face2->firstedge + l];
					if (abs(edge1num) == abs(edge2num))
					{
						//get the face with the largest area
						face1area = AAS_FaceArea(face1);
						face2area = AAS_FaceArea(face2);
						if (face1area > bestface1area && face2area > bestface2area)
						{
							bestface1area = face1area;
							bestface2area = face2area;
							ladderface1 = face1;
							ladderface2 = face2;
							ladderface1num = face1num;
							ladderface2num = face2num;
							sharededgenum = edge1num;
						} //end if
						break;
					} //end if
				} //end for
				if (l != face2->numedges) break;
			} //end for
		} //end for
	} //end for
	//
	if (ladderface1 && ladderface2)
	{
		//get the middle of the shared edge
		sharededge = &aasworld.edges[abs(sharededgenum)];
		firstv = sharededgenum < 0;
		//
		VectorCopy(aasworld.vertexes[sharededge->v[firstv]], v1);
		VectorCopy(aasworld.vertexes[sharededge->v[!firstv]], v2);
		VectorAdd(v1, v2, area1point);
		VectorScale(area1point, 0.5, area1point);
		VectorCopy(area1point, area2point);
		//
		//if the face plane in area 1 is pretty much vertical
		plane1 = &aasworld.planes[ladderface1->planenum ^ (ladderface1num < 0)];
		plane2 = &aasworld.planes[ladderface2->planenum ^ (ladderface2num < 0)];
		//
		//get the points really into the areas
		VectorSubtract(v2, v1, sharededgevec);
		CrossProduct(plane1->normal, sharededgevec, dir);
		VectorNormalize(dir);
		//NOTE: 32 because that's larger than 16 (bot bbox x,y)
		VectorMA(area1point, -32, dir, area1point);
		VectorMA(area2point, 32, dir, area2point);
		//
		ladderface1vertical = abs(DotProduct(plane1->normal, up)) < 0.1;
		ladderface2vertical = abs(DotProduct(plane2->normal, up)) < 0.1;
		//there's only reachability between vertical ladder faces
		if (!ladderface1vertical && !ladderface2vertical) return qfalse;
		//if both vertical ladder faces
		if (ladderface1vertical && ladderface2vertical
					//and the ladder faces do not make a sharp corner
					&& DotProduct(plane1->normal, plane2->normal) > 0.7
					//and the shared edge is not too vertical
					&& abs(DotProduct(sharededgevec, up)) < 0.7)
		{
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach) return qfalse;
			lreach->areanum = area2num;
			lreach->facenum = ladderface1num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area1point, lreach->start);
			//VectorCopy(area2point, lreach->end);
			VectorMA(area2point, -3, plane1->normal, lreach->end);
			lreach->traveltype = TRAVEL_LADDER;
			lreach->traveltime = 10;
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_ladder++;
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach) return qfalse;
			lreach->areanum = area1num;
			lreach->facenum = ladderface2num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area2point, lreach->start);
			//VectorCopy(area1point, lreach->end);
			VectorMA(area1point, -3, plane1->normal, lreach->end);
			lreach->traveltype = TRAVEL_LADDER;
			lreach->traveltime = 10;
			lreach->next = areareachability[area2num];
			areareachability[area2num] = lreach;
			//
			reach_ladder++;
			//
			return qtrue;
		} //end if
		//if the second ladder face is also a ground face
		//create ladder end (just ladder) reachability and
		//walk off a ladder (ledge) reachability
		if (ladderface1vertical && (ladderface2->faceflags & FACE_GROUND))
		{
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach) return qfalse;
			lreach->areanum = area2num;
			lreach->facenum = ladderface1num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area1point, lreach->start);
			VectorCopy(area2point, lreach->end);
			lreach->end[2] += 16;
			VectorMA(lreach->end, -15, plane1->normal, lreach->end);
			lreach->traveltype = TRAVEL_LADDER;
			lreach->traveltime = 10;
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_ladder++;
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach) return qfalse;
			lreach->areanum = area1num;
			lreach->facenum = ladderface2num;
			lreach->edgenum = abs(sharededgenum);
			VectorCopy(area2point, lreach->start);
			VectorCopy(area1point, lreach->end);
			lreach->traveltype = TRAVEL_WALKOFFLEDGE;
			lreach->traveltime = 10;
			lreach->next = areareachability[area2num];
			areareachability[area2num] = lreach;
			//
			reach_walkoffledge++;
			//
			return qtrue;
		} //end if
		//
		if (ladderface1vertical)
		{
			//find lowest edge of the ladder face
			lowestpoint[2] = 99999;
			for (i = 0; i < ladderface1->numedges; i++)
			{
				edge1num = abs(aasworld.edgeindex[ladderface1->firstedge + i]);
				edge1 = &aasworld.edges[edge1num];
				//
				VectorCopy(aasworld.vertexes[edge1->v[0]], v1);
				VectorCopy(aasworld.vertexes[edge1->v[1]], v2);
				//
				VectorAdd(v1, v2, mid);
				VectorScale(mid, 0.5, mid);
				//
				if (mid[2] < lowestpoint[2])
				{
					VectorCopy(mid, lowestpoint);
					lowestedgenum = edge1num;
				} //end if
			} //end for
			//
			plane1 = &aasworld.planes[ladderface1->planenum];
			//trace down in the middle of this edge
			VectorMA(lowestpoint, 5, plane1->normal, start);
			VectorCopy(start, end);
			start[2] += 5;
			end[2] -= 100;
			//trace without entity collision
			trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
			//
			//
#ifdef REACH_DEBUG
			if (trace.startsolid)
			{
				Log_Write("trace from area %d started in solid\r\n", area1num);
			} //end if
#endif //REACH_DEBUG
			//
			trace.endpos[2] += 1;
			area2num = AAS_PointAreaNum(trace.endpos);
			//
			area2 = &aasworld.areas[area2num];
			for (i = 0; i < area2->numfaces; i++)
			{
				face2num = aasworld.faceindex[area2->firstface + i];
				face2 = &aasworld.faces[abs(face2num)];
				//
				if (face2->faceflags & FACE_LADDER)
				{
					plane2 = &aasworld.planes[face2->planenum];
					if (abs(DotProduct(plane2->normal, up)) < 0.1) break;
				} //end if
			} //end for
			//if from another area without vertical ladder faces
			if (i >= area2->numfaces && area2num != area1num &&
						//the reachabilities shouldn't exist already
						!AAS_ReachabilityExists(area1num, area2num) &&
						!AAS_ReachabilityExists(area2num, area1num))
			{
				//if the height is jumpable
				if (start[2] - trace.endpos[2] < maxjumpheight)
				{
					//create a new reachability link
					lreach = AAS_AllocReachability();
					if (!lreach) return qfalse;
					lreach->areanum = area2num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy(lowestpoint, lreach->start);
					VectorCopy(trace.endpos, lreach->end);
					lreach->traveltype = TRAVEL_LADDER;
					lreach->traveltime = 10;
					lreach->next = areareachability[area1num];
					areareachability[area1num] = lreach;
					//
					reach_ladder++;
					//create a new reachability link
					lreach = AAS_AllocReachability();
					if (!lreach) return qfalse;
					lreach->areanum = area1num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy(trace.endpos, lreach->start);
					//get the end point a little bit into the ladder
					VectorMA(lowestpoint, -5, plane1->normal, lreach->end);
					//get the end point a little higher
					lreach->end[2] += 10;
					lreach->traveltype = TRAVEL_JUMP;
					lreach->traveltime = 10;
					lreach->next = areareachability[area2num];
					areareachability[area2num] = lreach;
					//
					reach_jump++;	
					//
					return qtrue;
#ifdef REACH_DEBUG
					Log_Write("jump up to ladder reach between %d and %d\r\n", area2num, area1num);
#endif //REACH_DEBUG
				} //end if
#ifdef REACH_DEBUG
				else Log_Write("jump too high between area %d and %d\r\n", area2num, area1num);
#endif //REACH_DEBUG
			} //end if
			/*//if slime or lava below the ladder
			//try jump reachability from far towards the ladder
			if (aasworld.areasettings[area2num].contents & (AREACONTENTS_SLIME
													| AREACONTENTS_LAVA))
			{
				for (i = 20; i <= 120; i += 20)
				{
					//trace down in the middle of this edge
					VectorMA(lowestpoint, i, plane1->normal, start);
					VectorCopy(start, end);
					start[2] += 5;
					end[2] -= 100;
					//trace without entity collision
					trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
					//
					if (trace.startsolid) break;
					trace.endpos[2] += 1;
					area2num = AAS_PointAreaNum(trace.endpos);
					if (area2num == area1num) continue;
					//
					if (start[2] - trace.endpos[2] > maxjumpheight) continue;
					if (aasworld.areasettings[area2num].contents & (AREACONTENTS_SLIME
												| AREACONTENTS_LAVA)) continue;
					//
					//create a new reachability link
					lreach = AAS_AllocReachability();
					if (!lreach) return qfalse;
					lreach->areanum = area1num;
					lreach->facenum = ladderface1num;
					lreach->edgenum = lowestedgenum;
					VectorCopy(trace.endpos, lreach->start);
					VectorCopy(lowestpoint, lreach->end);
					lreach->end[2] += 5;
					lreach->traveltype = TRAVEL_JUMP;
					lreach->traveltime = 10;
					lreach->next = areareachability[area2num];
					areareachability[area2num] = lreach;
					//
					reach_jump++;
					//
					Log_Write("jump far to ladder reach between %d and %d\r\n", area2num, area1num);
					//
					break;
				} //end for
			} //end if*/
		} //end if
	} //end if
	return qfalse;
} //end of the function AAS_Reachability_Ladder
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_TravelFlagsForTeam(int ent)
{
	int notteam;

	if (!AAS_IntForBSPEpairKey(ent, "bot_notteam", &notteam))
		return 0;
	if (notteam == 1)
		return TRAVELFLAG_NOTTEAM1;
	if (notteam == 2)
		return TRAVELFLAG_NOTTEAM2;
	return 0;
} //end of the function AAS_TravelFlagsForTeam
//===========================================================================
// create possible teleporter reachabilities
// this is very game dependent.... :(
//
// classname = trigger_multiple or trigger_teleport
// target = "t1"
//
// classname = target_teleporter
// targetname = "t1"
// target = "t2"
//
// classname = misc_teleporter_dest
// targetname = "t2"
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_Teleport(void)
{
	int area1num, area2num;
	char target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY];
	char classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];
	int ent, dest;
	float angle;
	vec3_t origin, destorigin, mins, maxs, end, angles;
	vec3_t mid, velocity, cmdmove;
	aas_lreachability_t *lreach;
	aas_clientmove_t move;
	aas_trace_t trace;
	aas_link_t *areas, *link;

	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		if (!strcmp(classname, "trigger_multiple"))
		{
			AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
//#ifdef REACH_DEBUG
			botimport.Print(PRT_MESSAGE, "trigger_multiple model = \"%s\"\n", model);
//#endif REACH_DEBUG
			VectorClear(angles);
			AAS_BSPModelMinsMaxsOrigin(atoi(model+1), angles, mins, maxs, origin);
			//
			if (!AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY))
			{
				botimport.Print(PRT_ERROR, "trigger_multiple at %1.0f %1.0f %1.0f without target\n",
									origin[0], origin[1], origin[2]);
				continue;
			} //end if
			for (dest = AAS_NextBSPEntity(0); dest; dest = AAS_NextBSPEntity(dest))
			{
				if (!AAS_ValueForBSPEpairKey(dest, "classname", classname, MAX_EPAIRKEY)) continue;
				if (!strcmp(classname, "target_teleporter"))
				{
					if (!AAS_ValueForBSPEpairKey(dest, "targetname", targetname, MAX_EPAIRKEY)) continue;
					if (!strcmp(targetname, target))
					{
						break;
					} //end if
				} //end if
			} //end for
			if (!dest)
			{
				continue;
			} //end if
			if (!AAS_ValueForBSPEpairKey(dest, "target", target, MAX_EPAIRKEY))
			{
				botimport.Print(PRT_ERROR, "target_teleporter without target\n");
				continue;
			} //end if
		} //end else
		else if (!strcmp(classname, "trigger_teleport"))
		{
			AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
//#ifdef REACH_DEBUG
			botimport.Print(PRT_MESSAGE, "trigger_teleport model = \"%s\"\n", model);
//#endif REACH_DEBUG
			VectorClear(angles);
			AAS_BSPModelMinsMaxsOrigin(atoi(model+1), angles, mins, maxs, origin);
			//
			if (!AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY))
			{
				botimport.Print(PRT_ERROR, "trigger_teleport at %1.0f %1.0f %1.0f without target\n",
									origin[0], origin[1], origin[2]);
				continue;
			} //end if
		} //end if
		else
		{
			continue;
		} //end else
		//
		for (dest = AAS_NextBSPEntity(0); dest; dest = AAS_NextBSPEntity(dest))
		{
			//classname should be misc_teleporter_dest
			//but I've also seen target_position and actually any
			//entity could be used... burp
			if (AAS_ValueForBSPEpairKey(dest, "targetname", targetname, MAX_EPAIRKEY))
			{
				if (!strcmp(targetname, target))
				{
					break;
				} //end if
			} //end if
		} //end for
		if (!dest)
		{
			botimport.Print(PRT_ERROR, "teleporter without misc_teleporter_dest (%s)\n", target);
			continue;
		} //end if
		if (!AAS_VectorForBSPEpairKey(dest, "origin", destorigin))
		{
			botimport.Print(PRT_ERROR, "teleporter destination (%s) without origin\n", target);
			continue;
		} //end if
		//
		area2num = AAS_PointAreaNum(destorigin);
		//if not teleported into a teleporter or into a jumppad
		if (!AAS_AreaTeleporter(area2num) && !AAS_AreaJumpPad(area2num))
		{
			VectorCopy(destorigin, end);
			end[2] -= 64;
			trace = AAS_TraceClientBBox(destorigin, end, PRESENCE_CROUCH, -1);
			if (trace.startsolid)
			{
				botimport.Print(PRT_ERROR, "teleporter destination (%s) in solid\n", target);
				continue;
			} //end if
			area2num = AAS_PointAreaNum(trace.endpos);
			//
			/*
			if (!AAS_AreaTeleporter(area2num) &&
				!AAS_AreaJumpPad(area2num) &&
				!AAS_AreaGrounded(area2num))
			{
				VectorCopy(trace.endpos, destorigin);
			}
			else*/
			{
				//predict where you'll end up
				AAS_FloatForBSPEpairKey(dest, "angle", &angle);
				if (angle)
				{
					VectorSet(angles, 0, angle, 0);
					AngleVectors(angles, velocity, NULL, NULL);
					VectorScale(velocity, 400, velocity);
				} //end if
				else
				{
					VectorClear(velocity);
				} //end else
				VectorClear(cmdmove);
				AAS_PredictClientMovement(&move, -1, destorigin, PRESENCE_NORMAL, qfalse,
										velocity, cmdmove, 0, 30, 0.1f,
										SE_HITGROUND|SE_ENTERWATER|SE_ENTERSLIME|
										SE_ENTERLAVA|SE_HITGROUNDDAMAGE|SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER, 0, qfalse); //qtrue);
				area2num = AAS_PointAreaNum(move.endpos);
				if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA))
				{
					botimport.Print(PRT_WARNING, "teleported into slime or lava at dest %s\n", target);
				} //end if
				VectorCopy(move.endpos, destorigin);
			} //end else
		} //end if
		//
		//botimport.Print(PRT_MESSAGE, "teleporter brush origin at %f %f %f\n", origin[0], origin[1], origin[2]);
		//botimport.Print(PRT_MESSAGE, "teleporter brush mins = %f %f %f\n", mins[0], mins[1], mins[2]);
		//botimport.Print(PRT_MESSAGE, "teleporter brush maxs = %f %f %f\n", maxs[0], maxs[1], maxs[2]);
		VectorAdd(origin, mins, mins);
		VectorAdd(origin, maxs, maxs);
		//
		VectorAdd(mins, maxs, mid);
		VectorScale(mid, 0.5, mid);
		//link an invalid (-1) entity
		areas = AAS_LinkEntityClientBBox(mins, maxs, -1, PRESENCE_CROUCH);
		if (!areas) botimport.Print(PRT_MESSAGE, "trigger_multiple not in any area\n");
		//
		for (link = areas; link; link = link->next_area)
		{
			//if (!AAS_AreaGrounded(link->areanum)) continue;
			if (!AAS_AreaTeleporter(link->areanum)) continue;
			//
			area1num = link->areanum;
			//create a new reachability link
			lreach = AAS_AllocReachability();
			if (!lreach) break;
			lreach->areanum = area2num;
			lreach->facenum = 0;
			lreach->edgenum = 0;
			VectorCopy(mid, lreach->start);
			VectorCopy(destorigin, lreach->end);
			lreach->traveltype = TRAVEL_TELEPORT;
			lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
			lreach->traveltime = aassettings.rs_teleport;
			lreach->next = areareachability[area1num];
			areareachability[area1num] = lreach;
			//
			reach_teleport++;
		} //end for
		//unlink the invalid entity
		AAS_UnlinkFromAreas(areas);
	} //end for
} //end of the function AAS_Reachability_Teleport
//===========================================================================
// create possible elevator (func_plat) reachabilities
// this is very game dependent.... :(
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_Elevator(void)
{
	int area1num, area2num, modelnum, i, j, k, l, n, p;
	float lip, height, speed;
	char model[MAX_EPAIRKEY], classname[MAX_EPAIRKEY];
	int ent;
	vec3_t mins, maxs, origin, angles = {0, 0, 0};
	vec3_t pos1, pos2, mids, platbottom, plattop;
	vec3_t bottomorg, toporg, start, end, dir;
	vec_t xvals[8], yvals[8], xvals_top[8], yvals_top[8];
	aas_lreachability_t *lreach;
	aas_trace_t trace;

#ifdef REACH_DEBUG
	Log_Write("AAS_Reachability_Elevator\r\n");
#endif //REACH_DEBUG
	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		if (!strcmp(classname, "func_plat"))
		{
#ifdef REACH_DEBUG
			Log_Write("found func plat\r\n");
#endif //REACH_DEBUG
			if (!AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY))
			{
				botimport.Print(PRT_ERROR, "func_plat without model\n");
				continue;
			} //end if
			//get the model number, and skip the leading *
			modelnum = atoi(model+1);
			if (modelnum <= 0)
			{
				botimport.Print(PRT_ERROR, "func_plat with invalid model number\n");
				continue;
			} //end if
			//get the mins, maxs and origin of the model
			//NOTE: the origin is usually (0,0,0) and the mins and maxs
			//      are the absolute mins and maxs
			AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, origin);
			//
			AAS_VectorForBSPEpairKey(ent, "origin", origin);
			//pos1 is the top position, pos2 is the bottom
			VectorCopy(origin, pos1);
			VectorCopy(origin, pos2);
			//get the lip of the plat
			AAS_FloatForBSPEpairKey(ent, "lip", &lip);
			if (!lip) lip = 8;
			//get the movement height of the plat
			AAS_FloatForBSPEpairKey(ent, "height", &height);
			if (!height) height = (maxs[2] - mins[2]) - lip;
			//get the speed of the plat
			AAS_FloatForBSPEpairKey(ent, "speed", &speed);
			if (!speed) speed = 200;
			//get bottom position below pos1
			pos2[2] -= height;
			//
			//get a point just above the plat in the bottom position
			VectorAdd(mins, maxs, mids);
			VectorMA(pos2, 0.5, mids, platbottom);
			platbottom[2] = maxs[2] - (pos1[2] - pos2[2]) + 2;
			//get a point just above the plat in the top position
			VectorAdd(mins, maxs, mids);
			VectorMA(pos2, 0.5, mids, plattop);
			plattop[2] = maxs[2] + 2;
			//
			/*if (!area1num)
			{
				Log_Write("no grounded area near plat bottom\r\n");
				continue;
			} //end if*/
			//get the mins and maxs a little larger
			for (i = 0; i < 3; i++)
			{
				mins[i] -= 1;
				maxs[i] += 1;
			} //end for
			//
			//botimport.Print(PRT_MESSAGE, "platbottom[2] = %1.1f plattop[2] = %1.1f\n", platbottom[2], plattop[2]);
			//
			VectorAdd(mins, maxs, mids);
			VectorScale(mids, 0.5, mids);
			//
			xvals[0] = mins[0]; xvals[1] = mids[0]; xvals[2] = maxs[0]; xvals[3] = mids[0];
			yvals[0] = mids[1]; yvals[1] = maxs[1]; yvals[2] = mids[1]; yvals[3] = mins[1];
			//
			xvals[4] = mins[0]; xvals[5] = maxs[0]; xvals[6] = maxs[0]; xvals[7] = mins[0];
			yvals[4] = maxs[1]; yvals[5] = maxs[1]; yvals[6] = mins[1]; yvals[7] = mins[1];
			//find adjacent areas around the bottom of the plat
			for (i = 0; i < 9; i++)
			{
				if (i < 8) //check at the sides of the plat
				{
					bottomorg[0] = origin[0] + xvals[i];
					bottomorg[1] = origin[1] + yvals[i];
					bottomorg[2] = platbottom[2] + 16;
					//get a grounded or swim area near the plat in the bottom position
					area1num = AAS_PointAreaNum(bottomorg);
					for (k = 0; k < 16; k++)
					{
						if (area1num)
						{
							if (AAS_AreaGrounded(area1num) || AAS_AreaSwim(area1num)) break;
						} //end if
						bottomorg[2] += 4;
						area1num = AAS_PointAreaNum(bottomorg);
					} //end if
					//if in solid
					if (k >= 16)
					{
						continue;
					} //end if
				} //end if
				else //at the middle of the plat
				{
					VectorCopy(plattop, bottomorg);
					bottomorg[2] += 24;
					area1num = AAS_PointAreaNum(bottomorg);
					if (!area1num) continue;
					VectorCopy(platbottom, bottomorg);
					bottomorg[2] += 24;
				} //end else
				//look at adjacent areas around the top of the plat
				//make larger steps to outside the plat everytime
				for (n = 0; n < 3; n++)
				{
					for (k = 0; k < 3; k++)
					{
						mins[k] -= 4;
						maxs[k] += 4;
					} //end for
					xvals_top[0] = mins[0]; xvals_top[1] = mids[0]; xvals_top[2] = maxs[0]; xvals_top[3] = mids[0];
					yvals_top[0] = mids[1]; yvals_top[1] = maxs[1]; yvals_top[2] = mids[1]; yvals_top[3] = mins[1];
					//
					xvals_top[4] = mins[0]; xvals_top[5] = maxs[0]; xvals_top[6] = maxs[0]; xvals_top[7] = mins[0];
					yvals_top[4] = maxs[1]; yvals_top[5] = maxs[1]; yvals_top[6] = mins[1]; yvals_top[7] = mins[1];
					//
					for (j = 0; j < 8; j++)
					{
						toporg[0] = origin[0] + xvals_top[j];
						toporg[1] = origin[1] + yvals_top[j];
						toporg[2] = plattop[2] + 16;
						//get a grounded or swim area near the plat in the top position
						area2num = AAS_PointAreaNum(toporg);
						for (l = 0; l < 16; l++)
						{
							if (area2num)
							{
								if (AAS_AreaGrounded(area2num) || AAS_AreaSwim(area2num))
								{
									VectorCopy(plattop, start);
									start[2] += 32;
									VectorCopy(toporg, end);
									end[2] += 1;
									trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
									if (trace.fraction >= 1) break;
								} //end if
							} //end if
							toporg[2] += 4;
							area2num = AAS_PointAreaNum(toporg);
						} //end if
						//if in solid
						if (l >= 16) continue;
						//never create a reachability in the same area
						if (area2num == area1num) continue;
						//if the area isn't grounded
						if (!AAS_AreaGrounded(area2num)) continue;
						//if there already exists reachability between the areas
						if (AAS_ReachabilityExists(area1num, area2num)) continue;
						//if the reachability start is within the elevator bounding box
						VectorSubtract(bottomorg, platbottom, dir);
						VectorNormalize(dir);
						dir[0] = bottomorg[0] + 24 * dir[0];
						dir[1] = bottomorg[1] + 24 * dir[1];
						dir[2] = bottomorg[2];
						//
						for (p = 0; p < 3; p++)
							if (dir[p] < origin[p] + mins[p] || dir[p] > origin[p] + maxs[p]) break;
						if (p >= 3) continue;
						//create a new reachability link
						lreach = AAS_AllocReachability();
						if (!lreach) continue;
						lreach->areanum = area2num;
						//the facenum is the model number
						lreach->facenum = modelnum;
						//the edgenum is the height
						lreach->edgenum = (int) height;
						//
						VectorCopy(dir, lreach->start);
						VectorCopy(toporg, lreach->end);
						lreach->traveltype = TRAVEL_ELEVATOR;
						lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
						lreach->traveltime = aassettings.rs_startelevator + height * 100 / speed;
						lreach->next = areareachability[area1num];
						areareachability[area1num] = lreach;
						//don't go any further to the outside
						n = 9999;
						//
#ifdef REACH_DEBUG
						Log_Write("elevator reach from %d to %d\r\n", area1num, area2num);
#endif //REACH_DEBUG
						//
						reach_elevator++;
					} //end for
				} //end for
			} //end for
		} //end if
	} //end for
} //end of the function AAS_Reachability_Elevator
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
aas_lreachability_t *AAS_FindFaceReachabilities(vec3_t *facepoints, int numpoints, aas_plane_t *plane, int towardsface)
{
	int i, j, k, l;
	int facenum, edgenum, bestfacenum;
	float *v1, *v2, *v3, *v4;
	float bestdist, speed, hordist, dist;
	vec3_t beststart, beststart2, bestend, bestend2, tmp, hordir, testpoint;
	aas_lreachability_t *lreach, *lreachabilities;
	aas_area_t *area;
	aas_face_t *face;
	aas_edge_t *edge;
	aas_plane_t *faceplane, *bestfaceplane;

	//
	lreachabilities = NULL;
	bestfacenum = 0;
	bestfaceplane = NULL;
	//
	for (i = 1; i < aasworld.numareas; i++)
	{
		area = &aasworld.areas[i];
		// get the shortest distance between one of the func_bob start edges and
		// one of the face edges of area1
		bestdist = 999999;
		for (j = 0; j < area->numfaces; j++)
		{
			facenum = aasworld.faceindex[area->firstface + j];
			face = &aasworld.faces[abs(facenum)];
			//if not a ground face
			if (!(face->faceflags & FACE_GROUND)) continue;
			//get the ground planes
			faceplane = &aasworld.planes[face->planenum];
			//
			for (k = 0; k < face->numedges; k++)
			{
				edgenum = abs(aasworld.edgeindex[face->firstedge + k]);
				edge = &aasworld.edges[edgenum];
				//calculate the minimum distance between the two edges
				v1 = aasworld.vertexes[edge->v[0]];
				v2 = aasworld.vertexes[edge->v[1]];
				//
				for (l = 0; l < numpoints; l++)
				{
					v3 = facepoints[l];
					v4 = facepoints[(l+1) % numpoints];
					dist = AAS_ClosestEdgePoints(v1, v2, v3, v4, faceplane, plane,
													beststart, bestend,
													beststart2, bestend2, bestdist);
					if (dist < bestdist)
					{
						bestfacenum = facenum;
						bestfaceplane = faceplane;
						bestdist = dist;
					} //end if
				} //end for
			} //end for
		} //end for
		//
		if (bestdist > 192) continue;
		//
		VectorMiddle(beststart, beststart2, beststart);
		VectorMiddle(bestend, bestend2, bestend);
		//
		if (!towardsface)
		{
			VectorCopy(beststart, tmp);
			VectorCopy(bestend, beststart);
			VectorCopy(tmp, bestend);
		} //end if
		//
		VectorSubtract(bestend, beststart, hordir);
		hordir[2] = 0;
		hordist = VectorLength(hordir);
		//
		if (hordist > 2 * AAS_MaxJumpDistance(aassettings.phys_jumpvel)) continue;
		//the end point should not be significantly higher than the start point
		if (bestend[2] - 32 > beststart[2]) continue;
		//don't fall down too far
		if (bestend[2] < beststart[2] - 128) continue;
		//the distance should not be too far
		if (hordist > 32)
		{
			//check for walk off ledge
			if (!AAS_HorizontalVelocityForJump(0, beststart, bestend, &speed)) continue;
		} //end if
		//
		beststart[2] += 1;
		bestend[2] += 1;
		//
		if (towardsface) VectorCopy(bestend, testpoint);
		else VectorCopy(beststart, testpoint);
		testpoint[2] = 0;
		testpoint[2] = (bestfaceplane->dist - DotProduct(bestfaceplane->normal, testpoint)) / bestfaceplane->normal[2];
		//
		if (!AAS_PointInsideFace(bestfacenum, testpoint, 0.1f))
		{
			//if the faces are not overlapping then only go down
			if (bestend[2] - 16 > beststart[2]) continue;
		} //end if
		lreach = AAS_AllocReachability();
		if (!lreach) return lreachabilities;
		lreach->areanum = i;
		lreach->facenum = 0;
		lreach->edgenum = 0;
		VectorCopy(beststart, lreach->start);
		VectorCopy(bestend, lreach->end);
		lreach->traveltype = 0;
		lreach->traveltime = 0;
		lreach->next = lreachabilities;
		lreachabilities = lreach;
#ifndef BSPC
		if (towardsface) AAS_PermanentLine(lreach->start, lreach->end, 1);
		else AAS_PermanentLine(lreach->start, lreach->end, 2);
#endif
	} //end for
	return lreachabilities;
} //end of the function AAS_FindFaceReachabilities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_FuncBobbing(void)
{
	int ent, spawnflags, modelnum, axis;
	int i, numareas, areas[10];
	char classname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];
	vec3_t origin, move_end, move_start, move_start_top, move_end_top;
	vec3_t mins, maxs, angles = {0, 0, 0};
	vec3_t start_edgeverts[4], end_edgeverts[4], mid;
	vec3_t org, start, end, dir, points[10];
	float height;
	aas_plane_t start_plane, end_plane;
	aas_lreachability_t *startreach, *endreach, *nextstartreach, *nextendreach, *lreach;
	aas_lreachability_t *firststartreach, *firstendreach;

	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		if (strcmp(classname, "func_bobbing")) continue;
		AAS_FloatForBSPEpairKey(ent, "height", &height);
		if (!height) height = 32;
		//
		if (!AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY))
		{
			botimport.Print(PRT_ERROR, "func_bobbing without model\n");
			continue;
		} //end if
		//get the model number, and skip the leading *
		modelnum = atoi(model+1);
		if (modelnum <= 0)
		{
			botimport.Print(PRT_ERROR, "func_bobbing with invalid model number\n");
			continue;
		} //end if
		//if the entity has an origin set then use it
		if (!AAS_VectorForBSPEpairKey(ent, "origin", origin))
			VectorSet(origin, 0, 0, 0);
		//
		AAS_BSPModelMinsMaxsOrigin(modelnum, angles, mins, maxs, NULL);
		//
		VectorAdd(mins, origin, mins);
		VectorAdd(maxs, origin, maxs);
		//
		VectorAdd(mins, maxs, mid);
		VectorScale(mid, 0.5, mid);
		VectorCopy(mid, origin);
		//
		VectorCopy(origin, move_end);
		VectorCopy(origin, move_start);
		//
		AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
		// set the axis of bobbing
		if (spawnflags & 1) axis = 0;
		else if (spawnflags & 2) axis = 1;
		else axis = 2;
		//
		move_start[axis] -= height;
		move_end[axis] += height;
		//
		Log_Write("funcbob model %d, start = {%1.1f, %1.1f, %1.1f} end = {%1.1f, %1.1f, %1.1f}\n",
					modelnum, move_start[0], move_start[1], move_start[2], move_end[0], move_end[1], move_end[2]);
		//
#ifndef BSPC
		/*
		AAS_DrawPermanentCross(move_start, 4, 1);
		AAS_DrawPermanentCross(move_end, 4, 2);
		*/
#endif
		//
		for (i = 0; i < 4; i++)
		{
			VectorCopy(move_start, start_edgeverts[i]);
			start_edgeverts[i][2] += maxs[2] - mid[2]; //+ bbox maxs z
			start_edgeverts[i][2] += 24;	//+ player origin to ground dist
		} //end for
		start_edgeverts[0][0] += maxs[0] - mid[0];
		start_edgeverts[0][1] += maxs[1] - mid[1];
		start_edgeverts[1][0] += maxs[0] - mid[0];
		start_edgeverts[1][1] += mins[1] - mid[1];
		start_edgeverts[2][0] += mins[0] - mid[0];
		start_edgeverts[2][1] += mins[1] - mid[1];
		start_edgeverts[3][0] += mins[0] - mid[0];
		start_edgeverts[3][1] += maxs[1] - mid[1];
		//
		start_plane.dist = start_edgeverts[0][2];
		VectorSet(start_plane.normal, 0, 0, 1);
		//
		for (i = 0; i < 4; i++)
		{
			VectorCopy(move_end, end_edgeverts[i]);
			end_edgeverts[i][2] += maxs[2] - mid[2]; //+ bbox maxs z
			end_edgeverts[i][2] += 24;	//+ player origin to ground dist
		} //end for
		end_edgeverts[0][0] += maxs[0] - mid[0];
		end_edgeverts[0][1] += maxs[1] - mid[1];
		end_edgeverts[1][0] += maxs[0] - mid[0];
		end_edgeverts[1][1] += mins[1] - mid[1];
		end_edgeverts[2][0] += mins[0] - mid[0];
		end_edgeverts[2][1] += mins[1] - mid[1];
		end_edgeverts[3][0] += mins[0] - mid[0];
		end_edgeverts[3][1] += maxs[1] - mid[1];
		//
		end_plane.dist = end_edgeverts[0][2];
		VectorSet(end_plane.normal, 0, 0, 1);
		//
#ifndef BSPC
#if 0
		for (i = 0; i < 4; i++)
		{
			AAS_PermanentLine(start_edgeverts[i], start_edgeverts[(i+1)%4], 1);
			AAS_PermanentLine(end_edgeverts[i], end_edgeverts[(i+1)%4], 1);
		} //end for
#endif
#endif
		VectorCopy(move_start, move_start_top);
		move_start_top[2] += maxs[2] - mid[2] + 24; //+ bbox maxs z
		VectorCopy(move_end, move_end_top);
		move_end_top[2] += maxs[2] - mid[2] + 24; //+ bbox maxs z
		//
		if (!AAS_PointAreaNum(move_start_top)) continue;
		if (!AAS_PointAreaNum(move_end_top)) continue;
		//
		for (i = 0; i < 2; i++)
		{
			firststartreach = firstendreach = NULL;
			//
			if (i == 0)
			{
				firststartreach = AAS_FindFaceReachabilities(start_edgeverts, 4, &start_plane, qtrue);
				firstendreach = AAS_FindFaceReachabilities(end_edgeverts, 4, &end_plane, qfalse);
			} //end if
			else
			{
				firststartreach = AAS_FindFaceReachabilities(end_edgeverts, 4, &end_plane, qtrue);
				firstendreach = AAS_FindFaceReachabilities(start_edgeverts, 4, &start_plane, qfalse);
			} //end else
			//
			//create reachabilities from start to end
			for (startreach = firststartreach; startreach; startreach = nextstartreach)
			{
				nextstartreach = startreach->next;
				//
				//trace = AAS_TraceClientBBox(startreach->start, move_start_top, PRESENCE_NORMAL, -1);
				//if (trace.fraction < 1) continue;
				//
				for (endreach = firstendreach; endreach; endreach = nextendreach)
				{
					nextendreach = endreach->next;
					//
					//trace = AAS_TraceClientBBox(endreach->end, move_end_top, PRESENCE_NORMAL, -1);
					//if (trace.fraction < 1) continue;
					//
					Log_Write("funcbob reach from area %d to %d\n", startreach->areanum, endreach->areanum);
					//
					//
					if (i == 0) VectorCopy(move_start_top, org);
					else VectorCopy(move_end_top, org);
					VectorSubtract(startreach->start, org, dir);
					dir[2] = 0;
					VectorNormalize(dir);
					VectorCopy(startreach->start, start);
					VectorMA(startreach->start, 1, dir, start);
					start[2] += 1;
					VectorMA(startreach->start, 16, dir, end);
					end[2] += 1;
					//
					numareas = AAS_TraceAreas(start, end, areas, points, 10);
					if (numareas <= 0) continue;
					if (numareas > 1) VectorCopy(points[1], startreach->start);
					else VectorCopy(end, startreach->start);
					//
					if (!AAS_PointAreaNum(startreach->start)) continue;
					if (!AAS_PointAreaNum(endreach->end)) continue;
					//
					lreach = AAS_AllocReachability();
					lreach->areanum = endreach->areanum;
					if (i == 0) lreach->edgenum = ((int)move_start[axis] << 16) | ((int) move_end[axis] & 0x0000ffff);
					else lreach->edgenum = ((int)move_end[axis] << 16) | ((int) move_start[axis] & 0x0000ffff);
					lreach->facenum = (spawnflags << 16) | modelnum;
					VectorCopy(startreach->start, lreach->start);
					VectorCopy(endreach->end, lreach->end);
#ifndef BSPC
//					AAS_DrawArrow(lreach->start, lreach->end, LINECOLOR_BLUE, LINECOLOR_YELLOW);
//					AAS_PermanentLine(lreach->start, lreach->end, 1);
#endif
					lreach->traveltype = TRAVEL_FUNCBOB;
					lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
					lreach->traveltime = aassettings.rs_funcbob;
					reach_funcbob++;
					lreach->next = areareachability[startreach->areanum];
					areareachability[startreach->areanum] = lreach;
					//
				} //end for
			} //end for
			for (startreach = firststartreach; startreach; startreach = nextstartreach)
			{
				nextstartreach = startreach->next;
				AAS_FreeReachability(startreach);
			} //end for
			for (endreach = firstendreach; endreach; endreach = nextendreach)
			{
				nextendreach = endreach->next;
				AAS_FreeReachability(endreach);
			} //end for
			//only go up with func_bobbing entities that go up and down
			if (!(spawnflags & 1) && !(spawnflags & 2)) break;
		} //end for
	} //end for
} //end of the function AAS_Reachability_FuncBobbing
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_JumpPad(void)
{
	int face2num, i, ret, area2num, visualize, ent, bot_visualizejumppads;
	//int modelnum, ent2;
	//float dist, time, height, gravity, forward;
	float speed, zvel, hordist;
	aas_face_t *face2;
	aas_area_t *area2;
	aas_lreachability_t *lreach;
	vec3_t areastart, facecenter, dir, cmdmove;
	vec3_t velocity, absmins, absmaxs;
	//vec3_t origin, ent2origin, angles, teststart;
	aas_clientmove_t move;
	//aas_trace_t trace;
	aas_link_t *areas, *link;
	//char target[MAX_EPAIRKEY], targetname[MAX_EPAIRKEY], model[MAX_EPAIRKEY];
	char classname[MAX_EPAIRKEY];

#ifdef BSPC
	bot_visualizejumppads = 0;
#else
	bot_visualizejumppads = LibVarValue("bot_visualizejumppads", "0");
#endif
	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		if (strcmp(classname, "trigger_push")) continue;
		//
		if (!AAS_GetJumpPadInfo(ent, areastart, absmins, absmaxs, velocity)) continue;
		/*
		//
		AAS_FloatForBSPEpairKey(ent, "speed", &speed);
		if (!speed) speed = 1000;
//		AAS_VectorForBSPEpairKey(ent, "angles", angles);
//		AAS_SetMovedir(angles, velocity);
//		VectorScale(velocity, speed, velocity);
		VectorClear(angles);
		//get the mins, maxs and origin of the model
		AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY);
		if (model[0]) modelnum = atoi(model+1);
		else modelnum = 0;
		AAS_BSPModelMinsMaxsOrigin(modelnum, angles, absmins, absmaxs, origin);
		VectorAdd(origin, absmins, absmins);
		VectorAdd(origin, absmaxs, absmaxs);
		//
#ifdef REACH_DEBUG
		botimport.Print(PRT_MESSAGE, "absmins = %f %f %f\n", absmins[0], absmins[1], absmins[2]);
		botimport.Print(PRT_MESSAGE, "absmaxs = %f %f %f\n", absmaxs[0], absmaxs[1], absmaxs[2]);
#endif REACH_DEBUG
		VectorAdd(absmins, absmaxs, origin);
		VectorScale (origin, 0.5, origin);

		//get the start areas
		VectorCopy(origin, teststart);
		teststart[2] += 64;
		trace = AAS_TraceClientBBox(teststart, origin, PRESENCE_CROUCH, -1);
		if (trace.startsolid)
		{
			botimport.Print(PRT_MESSAGE, "trigger_push start solid\n");
			VectorCopy(origin, areastart);
		} //end if
		else
		{
			VectorCopy(trace.endpos, areastart);
		} //end else
		areastart[2] += 0.125;
		//
		//AAS_DrawPermanentCross(origin, 4, 4);
		//get the target entity
		AAS_ValueForBSPEpairKey(ent, "target", target, MAX_EPAIRKEY);
		for (ent2 = AAS_NextBSPEntity(0); ent2; ent2 = AAS_NextBSPEntity(ent2))
		{
			if (!AAS_ValueForBSPEpairKey(ent2, "targetname", targetname, MAX_EPAIRKEY)) continue;
			if (!strcmp(targetname, target)) break;
		} //end for
		if (!ent2)
		{
			botimport.Print(PRT_MESSAGE, "trigger_push without target entity %s\n", target);
			continue;
		} //end if
		AAS_VectorForBSPEpairKey(ent2, "origin", ent2origin);
		//
		height = ent2origin[2] - origin[2];
		gravity = aassettings.sv_gravity;
		time = sqrt( height / ( 0.5 * gravity ) );
		if (!time)
		{
			botimport.Print(PRT_MESSAGE, "trigger_push without time\n");
			continue;
		} //end if
		// set s.origin2 to the push velocity
		VectorSubtract ( ent2origin, origin, velocity);
		dist = VectorNormalize( velocity);
		forward = dist / time;
		//FIXME: why multiply by 1.1
		forward *= 1.1;
		VectorScale(velocity, forward, velocity);
		velocity[2] = time * gravity;
		*/
		//get the areas the jump pad brush is in
		areas = AAS_LinkEntityClientBBox(absmins, absmaxs, -1, PRESENCE_CROUCH);
		/*
		for (link = areas; link; link = link->next_area)
		{
			if (link->areanum == 563)
			{
				ret = qfalse;
			}
		}
        */
		for (link = areas; link; link = link->next_area)
		{
			if (AAS_AreaJumpPad(link->areanum)) break;
		} //end for
		if (!link)
		{
			botimport.Print(PRT_MESSAGE, "trigger_push not in any jump pad area\n");
			AAS_UnlinkFromAreas(areas);
			continue;
		} //end if
		//
		botimport.Print(PRT_MESSAGE, "found a trigger_push with velocity %f %f %f\n", velocity[0], velocity[1], velocity[2]);
		//if there is a horizontal velocity check for a reachability without air control
		if (velocity[0] || velocity[1])
		{
			VectorSet(cmdmove, 0, 0, 0);
			//VectorCopy(velocity, cmdmove);
			//cmdmove[2] = 0;
			Com_Memset(&move, 0, sizeof(aas_clientmove_t));
			area2num = 0;
			for (i = 0; i < 20; i++)
			{
				AAS_PredictClientMovement(&move, -1, areastart, PRESENCE_NORMAL, qfalse,
										velocity, cmdmove, 0, 30, 0.1f,
										SE_HITGROUND|SE_ENTERWATER|SE_ENTERSLIME|
										SE_ENTERLAVA|SE_HITGROUNDDAMAGE|SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER, 0, bot_visualizejumppads);
				area2num = move.endarea;
				for (link = areas; link; link = link->next_area)
				{
					if (!AAS_AreaJumpPad(link->areanum)) continue;
					if (link->areanum == area2num) break;
				} //end if
				if (!link) break;
				VectorCopy(move.endpos, areastart);
				VectorCopy(move.velocity, velocity);
			} //end for
			if (area2num && i < 20)
			{
				for (link = areas; link; link = link->next_area)
				{
					if (!AAS_AreaJumpPad(link->areanum)) continue;
					if (AAS_ReachabilityExists(link->areanum, area2num)) continue;
					//create a rocket or bfg jump reachability from area1 to area2
					lreach = AAS_AllocReachability();
					if (!lreach)
					{
						AAS_UnlinkFromAreas(areas);
						return;
					} //end if
					lreach->areanum = area2num;
					//NOTE: the facenum is the Z velocity
					lreach->facenum = velocity[2];
					//NOTE: the edgenum is the horizontal velocity
					lreach->edgenum = sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1]);
					VectorCopy(areastart, lreach->start);
					VectorCopy(move.endpos, lreach->end);
					lreach->traveltype = TRAVEL_JUMPPAD;
					lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
					lreach->traveltime = aassettings.rs_jumppad;
					lreach->next = areareachability[link->areanum];
					areareachability[link->areanum] = lreach;
					//
					reach_jumppad++;
				} //end for
			} //end if
		} //end if
		//
		if (fabs(velocity[0]) > 100 || fabs(velocity[1]) > 100) continue;
		//check for areas we can reach with air control
		for (area2num = 1; area2num < aasworld.numareas; area2num++)
		{
			visualize = qfalse;
			/*
			if (area2num == 3568)
			{
				for (link = areas; link; link = link->next_area)
				{
					if (link->areanum == 3380)
					{
						visualize = qtrue;
						botimport.Print(PRT_MESSAGE, "bah\n");
					} //end if
				} //end for
			} //end if*/
			//never try to go back to one of the original jumppad areas
			//and don't create reachabilities if they already exist
			for (link = areas; link; link = link->next_area)
			{
				if (AAS_ReachabilityExists(link->areanum, area2num)) break;
				if (AAS_AreaJumpPad(link->areanum))
				{
					if (link->areanum == area2num) break;
				} //end if
			} //end if
			if (link) continue;
			//
			area2 = &aasworld.areas[area2num];
			for (i = 0; i < area2->numfaces; i++)
			{
				face2num = aasworld.faceindex[area2->firstface + i];
				face2 = &aasworld.faces[abs(face2num)];
				//if it is not a ground face
				if (!(face2->faceflags & FACE_GROUND)) continue;
				//get the center of the face
				AAS_FaceCenter(face2num, facecenter);
				//only go higher up
				if (facecenter[2] < areastart[2]) continue;
				//get the jumppad jump z velocity
				zvel = velocity[2];
				//get the horizontal speed for the jump, if it isn't possible to calculate this
				//speed
				ret = AAS_HorizontalVelocityForJump(zvel, areastart, facecenter, &speed);
				if (ret && speed < 150)
				{
					//direction towards the face center
					VectorSubtract(facecenter, areastart, dir);
					dir[2] = 0;
					hordist = VectorNormalize(dir);
					//if (hordist < 1.6 * facecenter[2] - areastart[2])
					{
						//get command movement
						VectorScale(dir, speed, cmdmove);
						//
						AAS_PredictClientMovement(&move, -1, areastart, PRESENCE_NORMAL, qfalse,
													velocity, cmdmove, 30, 30, 0.1f,
													SE_ENTERWATER|SE_ENTERSLIME|
													SE_ENTERLAVA|SE_HITGROUNDDAMAGE|
													SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER|SE_HITGROUNDAREA, area2num, visualize);
						//if prediction time wasn't enough to fully predict the movement
						//don't enter slime or lava and don't fall from too high
						if (move.frames < 30 && 
								!(move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
								&& (move.stopevent & (SE_HITGROUNDAREA|SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER)))
						{
							//never go back to the same jumppad
							for (link = areas; link; link = link->next_area)
							{
								if (link->areanum == move.endarea) break;
							}
							if (!link)
							{
								for (link = areas; link; link = link->next_area)
								{
									if (!AAS_AreaJumpPad(link->areanum)) continue;
									if (AAS_ReachabilityExists(link->areanum, area2num)) continue;
									//create a jumppad reachability from area1 to area2
									lreach = AAS_AllocReachability();
									if (!lreach)
									{
										AAS_UnlinkFromAreas(areas);
										return;
									} //end if
									lreach->areanum = move.endarea;
									//NOTE: the facenum is the Z velocity
									lreach->facenum = velocity[2];
									//NOTE: the edgenum is the horizontal velocity
									lreach->edgenum = sqrt(cmdmove[0] * cmdmove[0] + cmdmove[1] * cmdmove[1]);
									VectorCopy(areastart, lreach->start);
									VectorCopy(facecenter, lreach->end);
									lreach->traveltype = TRAVEL_JUMPPAD;
									lreach->traveltype |= AAS_TravelFlagsForTeam(ent);
									lreach->traveltime = aassettings.rs_aircontrolledjumppad;
									lreach->next = areareachability[link->areanum];
									areareachability[link->areanum] = lreach;
									//
									reach_jumppad++;
								} //end for
							}
						} //end if
					} //end if
				} //end for
			} //end for
		} //end for
		AAS_UnlinkFromAreas(areas);
	} //end for
} //end of the function AAS_Reachability_JumpPad
//===========================================================================
// never point at ground faces
// always a higher and pretty far area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_Grapple(int area1num, int area2num)
{
	int face2num, i, j, areanum, numareas, areas[20];
	float mingrappleangle, z, hordist;
	bsp_trace_t bsptrace;
	aas_trace_t trace;
	aas_face_t *face2;
	aas_area_t *area1, *area2;
	aas_lreachability_t *lreach;
	vec3_t areastart, facecenter, start, end, dir, down = {0, 0, -1};
	vec_t *v;

	//only grapple when on the ground or swimming
	if (!AAS_AreaGrounded(area1num) && !AAS_AreaSwim(area1num)) return qfalse;
	//don't grapple from a crouch area
	if (!(AAS_AreaPresenceType(area1num) & PRESENCE_NORMAL)) return qfalse;
	//NOTE: disabled area swim it doesn't work right
	if (AAS_AreaSwim(area1num)) return qfalse;
	//
	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];
	//don't grapple towards way lower areas
	if (area2->maxs[2] < area1->mins[2]) return qfalse;
	//
	VectorCopy(aasworld.areas[area1num].center, start);
	//if not a swim area
	if (!AAS_AreaSwim(area1num))
	{
		if (!AAS_PointAreaNum(start)) Log_Write("area %d center %f %f %f in solid?\r\n", area1num,
								start[0], start[1], start[2]);
		VectorCopy(start, end);
		end[2] -= 1000;
		trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
		if (trace.startsolid) return qfalse;
		VectorCopy(trace.endpos, areastart);
	} //end if
	else
	{
		if (!(AAS_PointContents(start) & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER))) return qfalse;
	} //end else
	//
	//start is now the start point
	//
	for (i = 0; i < area2->numfaces; i++)
	{
		face2num = aasworld.faceindex[area2->firstface + i];
		face2 = &aasworld.faces[abs(face2num)];
		//if it is not a solid face
		if (!(face2->faceflags & FACE_SOLID)) continue;
		//direction towards the first vertex of the face
		v = aasworld.vertexes[aasworld.edges[abs(aasworld.edgeindex[face2->firstedge])].v[0]];
		VectorSubtract(v, areastart, dir);
		//if the face plane is facing away
		if (DotProduct(aasworld.planes[face2->planenum].normal, dir) > 0) continue;
		//get the center of the face
		AAS_FaceCenter(face2num, facecenter);
		//only go higher up with the grapple
		if (facecenter[2] < areastart[2] + 64) continue;
		//only use vertical faces or downward facing faces
		if (DotProduct(aasworld.planes[face2->planenum].normal, down) < 0) continue;
		//direction towards the face center
		VectorSubtract(facecenter, areastart, dir);
		//
		z = dir[2];
		dir[2] = 0;
		hordist = VectorLength(dir);
		if (!hordist) continue;
		//if too far
		if (hordist > 2000) continue;
		//check the minimal angle of the movement
		mingrappleangle = 15; //15 degrees
		if (z / hordist < tan(2 * M_PI * mingrappleangle / 360)) continue;
		//
		VectorCopy(facecenter, start);
		VectorMA(facecenter, -500, aasworld.planes[face2->planenum].normal, end);
		//
		bsptrace = AAS_Trace(start, NULL, NULL, end, 0, CONTENTS_SOLID);
		//the grapple won't stick to the sky and the grapple point should be near the AAS wall
		if ((bsptrace.surface.flags & SURF_SKY) || (bsptrace.fraction * 500 > 32)) continue;
		//trace a full bounding box from the area center on the ground to
		//the center of the face
		VectorSubtract(facecenter, areastart, dir);
		VectorNormalize(dir);
		VectorMA(areastart, 4, dir, start);
		VectorCopy(bsptrace.endpos, end);
		trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
		VectorSubtract(trace.endpos, facecenter, dir);
		if (VectorLength(dir) > 24) continue;
		//
		VectorCopy(trace.endpos, start);
		VectorCopy(trace.endpos, end);
		end[2] -= AAS_FallDamageDistance();
		trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, -1);
		if (trace.fraction >= 1) continue;
		//area to end in
		areanum = AAS_PointAreaNum(trace.endpos);
		//if not in lava or slime
		if (aasworld.areasettings[areanum].contents & (AREACONTENTS_SLIME|AREACONTENTS_LAVA))
		{
			continue;
		} //end if
		//do not go the the source area
		if (areanum == area1num) continue;
		//don't create reachabilities if they already exist
		if (AAS_ReachabilityExists(area1num, areanum)) continue;
		//only end in areas we can stand
		if (!AAS_AreaGrounded(areanum)) continue;
		//never go through cluster portals!!
		numareas = AAS_TraceAreas(areastart, bsptrace.endpos, areas, NULL, 20);
		if (numareas >= 20) continue;
		for (j = 0; j < numareas; j++)
		{
			if (aasworld.areasettings[areas[j]].contents & AREACONTENTS_CLUSTERPORTAL) break;
		} //end for
		if (j < numareas) continue;
		//create a new reachability link
		lreach = AAS_AllocReachability();
		if (!lreach) return qfalse;
		lreach->areanum = areanum;
		lreach->facenum = face2num;
		lreach->edgenum = 0;
		VectorCopy(areastart, lreach->start);
		//VectorCopy(facecenter, lreach->end);
		VectorCopy(bsptrace.endpos, lreach->end);
		lreach->traveltype = TRAVEL_GRAPPLEHOOK;
		VectorSubtract(lreach->end, lreach->start, dir);
		lreach->traveltime = aassettings.rs_startgrapple + VectorLength(dir) * 0.25;
		lreach->next = areareachability[area1num];
		areareachability[area1num] = lreach;
		//
		reach_grapple++;
	} //end for
	//
	return qfalse;
} //end of the function AAS_Reachability_Grapple
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetWeaponJumpAreaFlags(void)
{
	int ent, i;
	vec3_t mins = {-15, -15, -15}, maxs = {15, 15, 15};
	vec3_t origin;
	int areanum, weaponjumpareas, spawnflags;
	char classname[MAX_EPAIRKEY];

	weaponjumpareas = 0;
	for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY)) continue;
		if (
			!strcmp(classname, "item_armor_body") ||
			!strcmp(classname, "item_armor_combat") ||
			!strcmp(classname, "item_health_mega") ||
			!strcmp(classname, "weapon_grenadelauncher") ||
			!strcmp(classname, "weapon_rocketlauncher") ||
			!strcmp(classname, "weapon_lightning") ||
			!strcmp(classname, "weapon_plasmagun") ||
			!strcmp(classname, "weapon_railgun") ||
			!strcmp(classname, "weapon_bfg") ||
			!strcmp(classname, "item_quad") ||
			!strcmp(classname, "item_regen") ||
			!strcmp(classname, "item_invulnerability"))
		{
			if (AAS_VectorForBSPEpairKey(ent, "origin", origin))
			{
				spawnflags = 0;
				AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);
				//if not a stationary item
				if (!(spawnflags & 1))
				{
					if (!AAS_DropToFloor(origin, mins, maxs))
					{
						botimport.Print(PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n",
														classname, origin[0], origin[1], origin[2]);
					} //end if
				} //end if
				//areanum = AAS_PointAreaNum(origin);
				areanum = AAS_BestReachableArea(origin, mins, maxs, origin);
				//the bot may rocket jump towards this area
				aasworld.areasettings[areanum].areaflags |= AREA_WEAPONJUMP;
				//
				//if (!AAS_AreaGrounded(areanum))
				//	botimport.Print(PRT_MESSAGE, "area not grounded\n");
				//
				weaponjumpareas++;
			} //end if
		} //end if
	} //end for
	for (i = 1; i < aasworld.numareas; i++)
	{
		if (aasworld.areasettings[i].contents & AREACONTENTS_JUMPPAD)
		{
			aasworld.areasettings[i].areaflags |= AREA_WEAPONJUMP;
			weaponjumpareas++;
		} //end if
	} //end for
	botimport.Print(PRT_MESSAGE, "%d weapon jump areas\n", weaponjumpareas);
} //end of the function AAS_SetWeaponJumpAreaFlags
//===========================================================================
// create a possible weapon jump reachability from area1 to area2
//
// check if there's a cool item in the second area
// check if area1 is lower than area2
// check if the bot can rocketjump from area1 to area2
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Reachability_WeaponJump(int area1num, int area2num)
{
	int face2num, i, n, ret, visualize;
	float speed, zvel, hordist;
	aas_face_t *face2;
	aas_area_t *area1, *area2;
	aas_lreachability_t *lreach;
	vec3_t areastart, facecenter, start, end, dir, cmdmove;// teststart;
	vec3_t velocity;
	aas_clientmove_t move;
	aas_trace_t trace;

	visualize = qfalse;
//	if (area1num == 4436 && area2num == 4318)
//	{
//		visualize = qtrue;
//	}
	if (!AAS_AreaGrounded(area1num) || AAS_AreaSwim(area1num)) return qfalse;
	if (!AAS_AreaGrounded(area2num)) return qfalse;
	//NOTE: only weapon jump towards areas with an interesting item in it??
	if (!(aasworld.areasettings[area2num].areaflags & AREA_WEAPONJUMP)) return qfalse;
	//
	area1 = &aasworld.areas[area1num];
	area2 = &aasworld.areas[area2num];
	//don't weapon jump towards way lower areas
	if (area2->maxs[2] < area1->mins[2]) return qfalse;
	//
	VectorCopy(aasworld.areas[area1num].center, start);
	//if not a swim area
	if (!AAS_PointAreaNum(start)) Log_Write("area %d center %f %f %f in solid?\r\n", area1num,
							start[0], start[1], start[2]);
	VectorCopy(start, end);
	end[2] -= 1000;
	trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
	if (trace.startsolid) return qfalse;
	VectorCopy(trace.endpos, areastart);
	//
	//areastart is now the start point
	//
	for (i = 0; i < area2->numfaces; i++)
	{
		face2num = aasworld.faceindex[area2->firstface + i];
		face2 = &aasworld.faces[abs(face2num)];
		//if it is not a solid face
		if (!(face2->faceflags & FACE_GROUND)) continue;
		//get the center of the face
		AAS_FaceCenter(face2num, facecenter);
		//only go higher up with weapon jumps
		if (facecenter[2] < areastart[2] + 64) continue;
		//NOTE: set to 2 to allow bfg jump reachabilities
		for (n = 0; n < 1/*2*/; n++)
		{
			//get the rocket jump z velocity
			if (n) zvel = AAS_BFGJumpZVelocity(areastart);
			else zvel = AAS_RocketJumpZVelocity(areastart);
			//get the horizontal speed for the jump, if it isn't possible to calculate this
			//speed (the jump is not possible) then there's no jump reachability created
			ret = AAS_HorizontalVelocityForJump(zvel, areastart, facecenter, &speed);
			if (ret && speed < 300)
			{
				//direction towards the face center
				VectorSubtract(facecenter, areastart, dir);
				dir[2] = 0;
				hordist = VectorNormalize(dir);
				//if (hordist < 1.6 * (facecenter[2] - areastart[2]))
				{
					//get command movement
					VectorScale(dir, speed, cmdmove);
					VectorSet(velocity, 0, 0, zvel);
					/*
					//get command movement
					VectorScale(dir, speed, velocity);
					velocity[2] = zvel;
					VectorSet(cmdmove, 0, 0, 0);
					*/
					//
					AAS_PredictClientMovement(&move, -1, areastart, PRESENCE_NORMAL, qtrue,
												velocity, cmdmove, 30, 30, 0.1f,
												SE_ENTERWATER|SE_ENTERSLIME|
												SE_ENTERLAVA|SE_HITGROUNDDAMAGE|
												SE_TOUCHJUMPPAD|SE_HITGROUND|SE_HITGROUNDAREA, area2num, visualize);
					//if prediction time wasn't enough to fully predict the movement
					//don't enter slime or lava and don't fall from too high
					if (move.frames < 30 && 
							!(move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
								&& (move.stopevent & (SE_HITGROUNDAREA|SE_TOUCHJUMPPAD)))
					{
						//create a rocket or bfg jump reachability from area1 to area2
						lreach = AAS_AllocReachability();
						if (!lreach) return qfalse;
						lreach->areanum = area2num;
						lreach->facenum = 0;
						lreach->edgenum = 0;
						VectorCopy(areastart, lreach->start);
						VectorCopy(facecenter, lreach->end);
						if (n)
						{
							lreach->traveltype = TRAVEL_BFGJUMP;
							lreach->traveltime = aassettings.rs_bfgjump;
						} //end if
						else
						{
							lreach->traveltype = TRAVEL_ROCKETJUMP;
							lreach->traveltime = aassettings.rs_rocketjump;
						} //end else
						lreach->next = areareachability[area1num];
						areareachability[area1num] = lreach;
						//
						reach_rocketjump++;
						return qtrue;
					} //end if
				} //end if
			} //end if
		} //end for
	} //end for
	//
	return qfalse;
} //end of the function AAS_Reachability_WeaponJump
//===========================================================================
// calculates additional walk off ledge reachabilities for the given area
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Reachability_WalkOffLedge(int areanum)
{
	int i, j, k, l, m, n, p, areas[10], numareas;
	int face1num, face2num, face3num, edge1num, edge2num, edge3num;
	int otherareanum, gap, reachareanum, side;
	aas_area_t *area, *area2;
	aas_face_t *face1, *face2, *face3;
	aas_edge_t *edge;
	aas_plane_t *plane;
	vec_t *v1, *v2;
	vec3_t sharededgevec, mid, dir, testend;
	aas_lreachability_t *lreach;
	aas_trace_t trace;

	if (!AAS_AreaGrounded(areanum) || AAS_AreaSwim(areanum)) return;
	//
	area = &aasworld.areas[areanum];
	//
	for (i = 0; i < area->numfaces; i++)
	{
		face1num = aasworld.faceindex[area->firstface + i];
		face1 = &aasworld.faces[abs(face1num)];
		//face 1 must be a ground face
		if (!(face1->faceflags & FACE_GROUND)) continue;
		//go through all the edges of this ground face
		for (k = 0; k < face1->numedges; k++)
		{
			edge1num = aasworld.edgeindex[face1->firstedge + k];
			//find another not ground face using this same edge
			for (j = 0; j < area->numfaces; j++)
			{
				face2num = aasworld.faceindex[area->firstface + j];
				face2 = &aasworld.faces[abs(face2num)];
				//face 2 may not be a ground face
				if (face2->faceflags & FACE_GROUND) continue;
				//compare all the edges
				for (l = 0; l < face2->numedges; l++)
				{
					edge2num = aasworld.edgeindex[face2->firstedge + l];
					if (abs(edge1num) == abs(edge2num))
					{
						//get the area at the other side of the face
						if (face2->frontarea == areanum) otherareanum = face2->backarea;
						else otherareanum = face2->frontarea;
						//
						area2 = &aasworld.areas[otherareanum];
						//if the other area is grounded!
						if (aasworld.areasettings[otherareanum].areaflags & AREA_GROUNDED)
						{
							//check for a possible gap
							gap = qfalse;
							for (n = 0; n < area2->numfaces; n++)
							{
								face3num = aasworld.faceindex[area2->firstface + n];
								//may not be the shared face of the two areas
								if (abs(face3num) == abs(face2num)) continue;
								//
								face3 = &aasworld.faces[abs(face3num)];
								//find an edge shared by all three faces
								for (m = 0; m < face3->numedges; m++)
								{
									edge3num = aasworld.edgeindex[face3->firstedge + m];
									//but the edge should be shared by all three faces
									if (abs(edge3num) == abs(edge1num))
									{
										if (!(face3->faceflags & FACE_SOLID))
										{
											gap = qtrue;
											break;
										} //end if
										//
										if (face3->faceflags & FACE_GROUND)
										{
											gap = qfalse;
											break;
										} //end if
										//FIXME: there are more situations to be handled
										gap = qtrue;
										break;
									} //end if
								} //end for
								if (m < face3->numedges) break;
							} //end for
							if (!gap) break;
						} //end if
						//check for a walk off ledge reachability
						edge = &aasworld.edges[abs(edge1num)];
						side = edge1num < 0;
						//
						v1 = aasworld.vertexes[edge->v[side]];
						v2 = aasworld.vertexes[edge->v[!side]];
						//
						plane = &aasworld.planes[face1->planenum];
						//get the points really into the areas
						VectorSubtract(v2, v1, sharededgevec);
						CrossProduct(plane->normal, sharededgevec, dir);
						VectorNormalize(dir);
						//
						VectorAdd(v1, v2, mid);
						VectorScale(mid, 0.5, mid);
						VectorMA(mid, 8, dir, mid);
						//
						VectorCopy(mid, testend);
						testend[2] -= 1000;
						trace = AAS_TraceClientBBox(mid, testend, PRESENCE_CROUCH, -1);
						//
						if (trace.startsolid)
						{
							//Log_Write("area %d: trace.startsolid\r\n", areanum);
							break;
						} //end if
						reachareanum = AAS_PointAreaNum(trace.endpos);
						if (reachareanum == areanum)
						{
							//Log_Write("area %d: same area\r\n", areanum);
							break;
						} //end if
						if (AAS_ReachabilityExists(areanum, reachareanum))
						{
							//Log_Write("area %d: reachability already exists\r\n", areanum);
							break;
						} //end if
						if (!AAS_AreaGrounded(reachareanum) && !AAS_AreaSwim(reachareanum))
						{
							//Log_Write("area %d, reach area %d: not grounded and not swim\r\n", areanum, reachareanum);
							break;
						} //end if
						//
						if (aasworld.areasettings[reachareanum].contents & (AREACONTENTS_SLIME
																						| AREACONTENTS_LAVA))
						{
							//Log_Write("area %d, reach area %d: lava or slime\r\n", areanum, reachareanum);
							break;
						} //end if
						//if not going through a cluster portal
						numareas = AAS_TraceAreas(mid, testend, areas, NULL, sizeof(areas) / sizeof(int));
						for (p = 0; p < numareas; p++)
							if (AAS_AreaClusterPortal(areas[p]))
								break;
						if (p < numareas)
							break;
						// if a maximum fall height is set and the bot would fall down further
						if (aassettings.rs_maxfallheight && fabs(mid[2] - trace.endpos[2]) > aassettings.rs_maxfallheight)
							break;
						//
						lreach = AAS_AllocReachability();
						if (!lreach) break;
						lreach->areanum = reachareanum;
						lreach->facenum = 0;
						lreach->edgenum = edge1num;
						VectorCopy(mid, lreach->start);
						VectorCopy(trace.endpos, lreach->end);
						lreach->traveltype = TRAVEL_WALKOFFLEDGE;
						lreach->traveltime = aassettings.rs_startwalkoffledge + fabs(mid[2] - trace.endpos[2]) * 50 / aassettings.phys_gravity;
						if (!AAS_AreaSwim(reachareanum) && !AAS_AreaJumpPad(reachareanum))
						{
							if (AAS_FallDelta(mid[2] - trace.endpos[2]) > aassettings.phys_falldelta5)
							{
								lreach->traveltime += aassettings.rs_falldamage5;
							} //end if
							else if (AAS_FallDelta(mid[2] - trace.endpos[2]) > aassettings.phys_falldelta10)
							{
								lreach->traveltime += aassettings.rs_falldamage10;
							} //end if
						} //end if
						lreach->next = areareachability[areanum];
						areareachability[areanum] = lreach;
						//we've got another walk off ledge reachability
						reach_walkoffledge++;
					} //end if
				} //end for
			} //end for
		} //end for
	} //end for
} //end of the function AAS_Reachability_WalkOffLedge
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_StoreReachability(void)
{
	int i;
	aas_areasettings_t *areasettings;
	aas_lreachability_t *lreach;
	aas_reachability_t *reach;

	if (aasworld.reachability) FreeMemory(aasworld.reachability);
	aasworld.reachability = (aas_reachability_t *) GetClearedMemory((numlreachabilities + 10) * sizeof(aas_reachability_t));
	aasworld.reachabilitysize = 1;
	for (i = 0; i < aasworld.numareas; i++)
	{
		areasettings = &aasworld.areasettings[i];
		areasettings->firstreachablearea = aasworld.reachabilitysize;
		areasettings->numreachableareas = 0;
		for (lreach = areareachability[i]; lreach; lreach = lreach->next)
		{
			reach = &aasworld.reachability[areasettings->firstreachablearea +
													areasettings->numreachableareas];
			reach->areanum = lreach->areanum;
			reach->facenum = lreach->facenum;
			reach->edgenum = lreach->edgenum;
			VectorCopy(lreach->start, reach->start);
			VectorCopy(lreach->end, reach->end);
			reach->traveltype = lreach->traveltype;
			reach->traveltime = lreach->traveltime;
			//
			areasettings->numreachableareas++;
		} //end for
		aasworld.reachabilitysize += areasettings->numreachableareas;
	} //end for
} //end of the function AAS_StoreReachability
//===========================================================================
//
// TRAVEL_WALK					100%	equal floor height + steps
// TRAVEL_CROUCH				100%
// TRAVEL_BARRIERJUMP			100%
// TRAVEL_JUMP					 80%
// TRAVEL_LADDER				100%	+ fall down from ladder + jump up to ladder
// TRAVEL_WALKOFFLEDGE			 90%	walk off very steep walls?
// TRAVEL_SWIM					100%
// TRAVEL_WATERJUMP				100%
// TRAVEL_TELEPORT				100%
// TRAVEL_ELEVATOR				100%
// TRAVEL_GRAPPLEHOOK			100%
// TRAVEL_DOUBLEJUMP			  0%
// TRAVEL_RAMPJUMP				  0%
// TRAVEL_STRAFEJUMP			  0%
// TRAVEL_ROCKETJUMP			100%	(currently limited towards areas with items)
// TRAVEL_BFGJUMP				  0%	(currently disabled)
// TRAVEL_JUMPPAD				100%
// TRAVEL_FUNCBOB				100%
//
// Parameter:			-
// Returns:				true if NOT finished
// Changes Globals:		-
//===========================================================================
int AAS_ContinueInitReachability(float time)
{
	int i, j, todo, start_time;
	static float framereachability, reachability_delay;
	static int lastpercentage;

	if (!aasworld.loaded) return qfalse;
	//if reachability is calculated for all areas
	if (aasworld.numreachabilityareas >= aasworld.numareas + 2) return qfalse;
	//if starting with area 1 (area 0 is a dummy)
	if (aasworld.numreachabilityareas == 1)
	{
		botimport.Print(PRT_MESSAGE, "calculating reachability...\n");
		lastpercentage = 0;
		framereachability = 2000;
		reachability_delay = 1000;
	} //end if
	//number of areas to calculate reachability for this cycle
	todo = aasworld.numreachabilityareas + (int) framereachability;
	start_time = Sys_MilliSeconds();
	//loop over the areas
	for (i = aasworld.numreachabilityareas; i < aasworld.numareas && i < todo; i++)
	{
		aasworld.numreachabilityareas++;
		//only create jumppad reachabilities from jumppad areas
		if (aasworld.areasettings[i].contents & AREACONTENTS_JUMPPAD)
		{
			continue;
		} //end if
		//loop over the areas
		for (j = 1; j < aasworld.numareas; j++)
		{
			if (i == j) continue;
			//never create reachabilities from teleporter or jumppad areas to regular areas
			if (aasworld.areasettings[i].contents & (AREACONTENTS_TELEPORTER|AREACONTENTS_JUMPPAD))
			{
				if (!(aasworld.areasettings[j].contents & (AREACONTENTS_TELEPORTER|AREACONTENTS_JUMPPAD)))
				{
					continue;
				} //end if
			} //end if
			//if there already is a reachability link from area i to j
			if (AAS_ReachabilityExists(i, j)) continue;
			//check for a swim reachability
			if (AAS_Reachability_Swim(i, j)) continue;
			//check for a simple walk on equal floor height reachability
			if (AAS_Reachability_EqualFloorHeight(i, j)) continue;
			//check for step, barrier, waterjump and walk off ledge reachabilities
			if (AAS_Reachability_Step_Barrier_WaterJump_WalkOffLedge(i, j)) continue;
			//check for ladder reachabilities
			if (AAS_Reachability_Ladder(i, j)) continue;
			//check for a jump reachability
			if (AAS_Reachability_Jump(i, j)) continue;
		} //end for
		//never create these reachabilities from teleporter or jumppad areas
		if (aasworld.areasettings[i].contents & (AREACONTENTS_TELEPORTER|AREACONTENTS_JUMPPAD))
		{
			continue;
		} //end if
		//loop over the areas
		for (j = 1; j < aasworld.numareas; j++)
		{
			if (i == j) continue;
			//
			if (AAS_ReachabilityExists(i, j)) continue;
			//check for a grapple hook reachability
			if (calcgrapplereach) AAS_Reachability_Grapple(i, j);
			//check for a weapon jump reachability
			AAS_Reachability_WeaponJump(i, j);
		} //end for
		//if the calculation took more time than the max reachability delay
		if (Sys_MilliSeconds() - start_time > (int) reachability_delay) break;
		//
		if (aasworld.numreachabilityareas * 1000 / aasworld.numareas > lastpercentage) break;
	} //end for
	//
	if (aasworld.numreachabilityareas == aasworld.numareas)
	{
		botimport.Print(PRT_MESSAGE, "\r%6.1f%%", (float) 100.0);
		botimport.Print(PRT_MESSAGE, "\nplease wait while storing reachability...\n");
		aasworld.numreachabilityareas++;
	} //end if
	//if this is the last step in the reachability calculations
	else if (aasworld.numreachabilityareas == aasworld.numareas + 1)
	{
		//create additional walk off ledge reachabilities for every area
		for (i = 1; i < aasworld.numareas; i++)
		{
			//only create jumppad reachabilities from jumppad areas
			if (aasworld.areasettings[i].contents & AREACONTENTS_JUMPPAD)
			{
				continue;
			} //end if
			AAS_Reachability_WalkOffLedge(i);
		} //end for
		//create jump pad reachabilities
		AAS_Reachability_JumpPad();
		//create teleporter reachabilities
		AAS_Reachability_Teleport();
		//create elevator (func_plat) reachabilities
		AAS_Reachability_Elevator();
		//create func_bobbing reachabilities
		AAS_Reachability_FuncBobbing();
		//
#ifdef DEBUG
		botimport.Print(PRT_MESSAGE, "%6d reach swim\n", reach_swim);
		botimport.Print(PRT_MESSAGE, "%6d reach equal floor\n", reach_equalfloor);
		botimport.Print(PRT_MESSAGE, "%6d reach step\n", reach_step);
		botimport.Print(PRT_MESSAGE, "%6d reach barrier\n", reach_barrier);
		botimport.Print(PRT_MESSAGE, "%6d reach waterjump\n", reach_waterjump);
		botimport.Print(PRT_MESSAGE, "%6d reach walkoffledge\n", reach_walkoffledge);
		botimport.Print(PRT_MESSAGE, "%6d reach jump\n", reach_jump);
		botimport.Print(PRT_MESSAGE, "%6d reach ladder\n", reach_ladder);
		botimport.Print(PRT_MESSAGE, "%6d reach walk\n", reach_walk);
		botimport.Print(PRT_MESSAGE, "%6d reach teleport\n", reach_teleport);
		botimport.Print(PRT_MESSAGE, "%6d reach funcbob\n", reach_funcbob);
		botimport.Print(PRT_MESSAGE, "%6d reach elevator\n", reach_elevator);
		botimport.Print(PRT_MESSAGE, "%6d reach grapple\n", reach_grapple);
		botimport.Print(PRT_MESSAGE, "%6d reach rocketjump\n", reach_rocketjump);
		botimport.Print(PRT_MESSAGE, "%6d reach jumppad\n", reach_jumppad);
#endif
		//*/
		//store all the reachabilities
		AAS_StoreReachability();
		//free the reachability link heap
		AAS_ShutDownReachabilityHeap();
		//
		FreeMemory(areareachability);
		//
		aasworld.numreachabilityareas++;
		//
		botimport.Print(PRT_MESSAGE, "calculating clusters...\n");
	} //end if
	else
	{
		lastpercentage = aasworld.numreachabilityareas * 1000 / aasworld.numareas;
		botimport.Print(PRT_MESSAGE, "\r%6.1f%%", (float) lastpercentage / 10);
	} //end else
	//not yet finished
	return qtrue;
} //end of the function AAS_ContinueInitReachability
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitReachability(void)
{
	if (!aasworld.loaded) return;

	if (aasworld.reachabilitysize)
	{
#ifndef BSPC
		if (!((int)LibVarGetValue("forcereachability")))
		{
			aasworld.numreachabilityareas = aasworld.numareas + 2;
			return;
		} //end if
#else
		aasworld.numreachabilityareas = aasworld.numareas + 2;
		return;
#endif //BSPC
	} //end if
#ifndef BSPC
	calcgrapplereach = LibVarGetValue("grapplereach");
#endif
	aasworld.savefile = qtrue;
	//start with area 1 because area zero is a dummy
	aasworld.numreachabilityareas = 1;
	////aasworld.numreachabilityareas = aasworld.numareas + 1;		//only calculate entity reachabilities
	//setup the heap with reachability links
	AAS_SetupReachabilityHeap();
	//allocate area reachability link array
	areareachability = (aas_lreachability_t **) GetClearedMemory(
									aasworld.numareas * sizeof(aas_lreachability_t *));
	//
	AAS_SetWeaponJumpAreaFlags();
} //end of the function AAS_InitReachable
