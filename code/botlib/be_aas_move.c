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
 * name:		be_aas_move.c
 *
 * desc:		AAS
 *
 * $Archive: /MissionPack/code/botlib/be_aas_move.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

extern botlib_import_t botimport;

aas_settings_t aassettings;

//#define AAS_MOVE_DEBUG

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_DropToFloor(vec3_t origin, vec3_t mins, vec3_t maxs)
{
	vec3_t end;
	bsp_trace_t trace;

	VectorCopy(origin, end);
	end[2] -= 100;
	trace = AAS_Trace(origin, mins, maxs, end, 0, CONTENTS_SOLID);
	if (trace.startsolid) return qfalse;
	VectorCopy(trace.endpos, origin);
	return qtrue;
} //end of the function AAS_DropToFloor
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_InitSettings(void)
{
	aassettings.phys_gravitydirection[0]	= 0;
	aassettings.phys_gravitydirection[1]	= 0;
	aassettings.phys_gravitydirection[2]	= -1;
	aassettings.phys_friction				= LibVarValue("phys_friction", "6");
	aassettings.phys_stopspeed				= LibVarValue("phys_stopspeed", "100");
	aassettings.phys_gravity				= LibVarValue("phys_gravity", "800");
	aassettings.phys_waterfriction			= LibVarValue("phys_waterfriction", "1");
	aassettings.phys_watergravity			= LibVarValue("phys_watergravity", "400");
	aassettings.phys_maxvelocity			= LibVarValue("phys_maxvelocity", "320");
	aassettings.phys_maxwalkvelocity		= LibVarValue("phys_maxwalkvelocity", "320");
	aassettings.phys_maxcrouchvelocity		= LibVarValue("phys_maxcrouchvelocity", "100");
	aassettings.phys_maxswimvelocity		= LibVarValue("phys_maxswimvelocity", "150");
	aassettings.phys_walkaccelerate			= LibVarValue("phys_walkaccelerate", "10");
	aassettings.phys_airaccelerate			= LibVarValue("phys_airaccelerate", "1");
	aassettings.phys_swimaccelerate			= LibVarValue("phys_swimaccelerate", "4");
	aassettings.phys_maxstep				= LibVarValue("phys_maxstep", "19");
	aassettings.phys_maxsteepness			= LibVarValue("phys_maxsteepness", "0.7");
	aassettings.phys_maxwaterjump			= LibVarValue("phys_maxwaterjump", "18");
	aassettings.phys_maxbarrier				= LibVarValue("phys_maxbarrier", "33");
	aassettings.phys_jumpvel				= LibVarValue("phys_jumpvel", "270");
	aassettings.phys_falldelta5				= LibVarValue("phys_falldelta5", "40");
	aassettings.phys_falldelta10			= LibVarValue("phys_falldelta10", "60");
	aassettings.rs_waterjump				= LibVarValue("rs_waterjump", "400");
	aassettings.rs_teleport					= LibVarValue("rs_teleport", "50");
	aassettings.rs_barrierjump				= LibVarValue("rs_barrierjump", "100");
	aassettings.rs_startcrouch				= LibVarValue("rs_startcrouch", "300");
	aassettings.rs_startgrapple				= LibVarValue("rs_startgrapple", "500");
	aassettings.rs_startwalkoffledge		= LibVarValue("rs_startwalkoffledge", "70");
	aassettings.rs_startjump				= LibVarValue("rs_startjump", "300");
	aassettings.rs_rocketjump				= LibVarValue("rs_rocketjump", "500");
	aassettings.rs_bfgjump					= LibVarValue("rs_bfgjump", "500");
	aassettings.rs_jumppad					= LibVarValue("rs_jumppad", "250");
	aassettings.rs_aircontrolledjumppad		= LibVarValue("rs_aircontrolledjumppad", "300");
	aassettings.rs_funcbob					= LibVarValue("rs_funcbob", "300");
	aassettings.rs_startelevator			= LibVarValue("rs_startelevator", "50");
	aassettings.rs_falldamage5				= LibVarValue("rs_falldamage5", "300");
	aassettings.rs_falldamage10				= LibVarValue("rs_falldamage10", "500");
	aassettings.rs_maxfallheight			= LibVarValue("rs_maxfallheight", "0");
	aassettings.rs_maxjumpfallheight		= LibVarValue("rs_maxjumpfallheight", "450");
} //end of the function AAS_InitSettings
//===========================================================================
// returns qtrue if the bot is against a ladder
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AgainstLadder(vec3_t origin)
{
	int areanum, i, facenum, side;
	vec3_t org;
	aas_plane_t *plane;
	aas_face_t *face;
	aas_area_t *area;

	VectorCopy(origin, org);
	areanum = AAS_PointAreaNum(org);
	if (!areanum)
	{
		org[0] += 1;
		areanum = AAS_PointAreaNum(org);
		if (!areanum)
		{
			org[1] += 1;
			areanum = AAS_PointAreaNum(org);
			if (!areanum)
			{
				org[0] -= 2;
				areanum = AAS_PointAreaNum(org);
				if (!areanum)
				{
					org[1] -= 2;
					areanum = AAS_PointAreaNum(org);
				} //end if
			} //end if
		} //end if
	} //end if
	//if in solid... wrrr shouldn't happen
	if (!areanum) return qfalse;
	//if not in a ladder area
	if (!(aasworld.areasettings[areanum].areaflags & AREA_LADDER)) return qfalse;
	//if a crouch only area
	if (!(aasworld.areasettings[areanum].presencetype & PRESENCE_NORMAL)) return qfalse;
	//
	area = &aasworld.areas[areanum];
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = aasworld.faceindex[area->firstface + i];
		side = facenum < 0;
		face = &aasworld.faces[abs(facenum)];
		//if the face isn't a ladder face
		if (!(face->faceflags & FACE_LADDER)) continue;
		//get the plane the face is in
		plane = &aasworld.planes[face->planenum ^ side];
		//if the origin is pretty close to the plane
		if (abs(DotProduct(plane->normal, origin) - plane->dist) < 3)
		{
			if (AAS_PointInsideFace(abs(facenum), origin, 0.1f)) return qtrue;
		} //end if
	} //end for
	return qfalse;
} //end of the function AAS_AgainstLadder
//===========================================================================
// returns qtrue if the bot is on the ground
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_OnGround(vec3_t origin, int presencetype, int passent)
{
	aas_trace_t trace;
	vec3_t end, up = {0, 0, 1};
	aas_plane_t *plane;

	VectorCopy(origin, end);
	end[2] -= 10;

	trace = AAS_TraceClientBBox(origin, end, presencetype, passent);

	//if in solid
	if (trace.startsolid) return qfalse;
	//if nothing hit at all
	if (trace.fraction >= 1.0) return qfalse;
	//if too far from the hit plane
	if (origin[2] - trace.endpos[2] > 10) return qfalse;
	//check if the plane isn't too steep
	plane = AAS_PlaneFromNum(trace.planenum);
	if (DotProduct(plane->normal, up) < aassettings.phys_maxsteepness) return qfalse;
	//the bot is on the ground
	return qtrue;
} //end of the function AAS_OnGround
//===========================================================================
// returns qtrue if a bot at the given position is swimming
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Swimming(vec3_t origin)
{
	vec3_t testorg;

	VectorCopy(origin, testorg);
	testorg[2] -= 2;
	if (AAS_PointContents(testorg) & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER)) return qtrue;
	return qfalse;
} //end of the function AAS_Swimming
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
static vec3_t VEC_UP			= {0, -1,  0};
static vec3_t MOVEDIR_UP		= {0,  0,  1};
static vec3_t VEC_DOWN		= {0, -2,  0};
static vec3_t MOVEDIR_DOWN	= {0,  0, -1};

void AAS_SetMovedir(vec3_t angles, vec3_t movedir)
{
	if (VectorCompare(angles, VEC_UP))
	{
		VectorCopy(MOVEDIR_UP, movedir);
	} //end if
	else if (VectorCompare(angles, VEC_DOWN))
	{
		VectorCopy(MOVEDIR_DOWN, movedir);
	} //end else if
	else
	{
		AngleVectors(angles, movedir, NULL, NULL);
	} //end else
} //end of the function AAS_SetMovedir
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_JumpReachRunStart(aas_reachability_t *reach, vec3_t runstart)
{
	vec3_t hordir, start, cmdmove;
	aas_clientmove_t move;

	//
	hordir[0] = reach->start[0] - reach->end[0];
	hordir[1] = reach->start[1] - reach->end[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	//start point
	VectorCopy(reach->start, start);
	start[2] += 1;
	//get command movement
	VectorScale(hordir, 400, cmdmove);
	//
	AAS_PredictClientMovement(&move, -1, start, PRESENCE_NORMAL, qtrue,
								vec3_origin, cmdmove, 1, 2, 0.1f,
								SE_ENTERWATER|SE_ENTERSLIME|SE_ENTERLAVA|
								SE_HITGROUNDDAMAGE|SE_GAP, 0, qfalse);
	VectorCopy(move.endpos, runstart);
	//don't enter slime or lava and don't fall from too high
	if (move.stopevent & (SE_ENTERSLIME|SE_ENTERLAVA|SE_HITGROUNDDAMAGE))
	{
		VectorCopy(start, runstart);
	} //end if
} //end of the function AAS_JumpReachRunStart
//===========================================================================
// returns the Z velocity when rocket jumping at the origin
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float AAS_WeaponJumpZVelocity(vec3_t origin, float radiusdamage)
{
	vec3_t kvel, v, start, end, forward, right, viewangles, dir;
	float	mass, knockback, points;
	vec3_t rocketoffset = {8, 8, -8};
	vec3_t botmins = {-16, -16, -24};
	vec3_t botmaxs = {16, 16, 32};
	bsp_trace_t bsptrace;

	//look down (90 degrees)
	viewangles[PITCH] = 90;
	viewangles[YAW] = 0;
	viewangles[ROLL] = 0;
	//get the start point shooting from
	VectorCopy(origin, start);
	start[2] += 8; //view offset Z
	AngleVectors(viewangles, forward, right, NULL);
	start[0] += forward[0] * rocketoffset[0] + right[0] * rocketoffset[1];
	start[1] += forward[1] * rocketoffset[0] + right[1] * rocketoffset[1];
	start[2] += forward[2] * rocketoffset[0] + right[2] * rocketoffset[1] + rocketoffset[2];
	//end point of the trace
	VectorMA(start, 500, forward, end);
	//trace a line to get the impact point
	bsptrace = AAS_Trace(start, NULL, NULL, end, 1, CONTENTS_SOLID);
	//calculate the damage the bot will get from the rocket impact
	VectorAdd(botmins, botmaxs, v);
	VectorMA(origin, 0.5, v, v);
	VectorSubtract(bsptrace.endpos, v, v);
	//
	points = radiusdamage - 0.5 * VectorLength(v);
	if (points < 0) points = 0;
	//the owner of the rocket gets half the damage
	points *= 0.5;
	//mass of the bot (p_client.c: PutClientInServer)
	mass = 200;
	//knockback is the same as the damage points
	knockback = points;
	//direction of the damage (from trace.endpos to bot origin)
	VectorSubtract(origin, bsptrace.endpos, dir);
	VectorNormalize(dir);
	//damage velocity
	VectorScale(dir, 1600.0 * (float)knockback / mass, kvel);	//the rocket jump hack...
	//rocket impact velocity + jump velocity
	return kvel[2] + aassettings.phys_jumpvel;
} //end of the function AAS_WeaponJumpZVelocity
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float AAS_RocketJumpZVelocity(vec3_t origin)
{
	//rocket radius damage is 120 (p_weapon.c: Weapon_RocketLauncher_Fire)
	return AAS_WeaponJumpZVelocity(origin, 120);
} //end of the function AAS_RocketJumpZVelocity
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float AAS_BFGJumpZVelocity(vec3_t origin)
{
	//bfg radius damage is 1000 (p_weapon.c: weapon_bfg_fire)
	return AAS_WeaponJumpZVelocity(origin, 120);
} //end of the function AAS_BFGJumpZVelocity
//===========================================================================
// applies ground friction to the given velocity
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_Accelerate(vec3_t velocity, float frametime, vec3_t wishdir, float wishspeed, float accel)
{
	// q2 style
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) {
		return;
	}
	accelspeed = accel*frametime*wishspeed;
	if (accelspeed > addspeed) {
		accelspeed = addspeed;
	}
	
	for (i=0 ; i<3 ; i++) {
		velocity[i] += accelspeed*wishdir[i];	
	}
} //end of the function AAS_Accelerate
//===========================================================================
// applies ground friction to the given velocity
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_ApplyFriction(vec3_t vel, float friction, float stopspeed,
													float frametime)
{
	float speed, control, newspeed;

	//horizontal speed
	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	if (speed)
	{
		control = speed < stopspeed ? stopspeed : speed;
		newspeed = speed - frametime * control * friction;
		if (newspeed < 0) newspeed = 0;
		newspeed /= speed;
		vel[0] *= newspeed;
		vel[1] *= newspeed;
	} //end if
} //end of the function AAS_ApplyFriction
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_ClipToBBox(aas_trace_t *trace, vec3_t start, vec3_t end, int presencetype, vec3_t mins, vec3_t maxs)
{
	int i, j, side;
	float front, back, frac, planedist;
	vec3_t bboxmins, bboxmaxs, absmins, absmaxs, dir, mid;

	AAS_PresenceTypeBoundingBox(presencetype, bboxmins, bboxmaxs);
	VectorSubtract(mins, bboxmaxs, absmins);
	VectorSubtract(maxs, bboxmins, absmaxs);
	//
	VectorCopy(end, trace->endpos);
	trace->fraction = 1;
	for (i = 0; i < 3; i++)
	{
		if (start[i] < absmins[i] && end[i] < absmins[i]) return qfalse;
		if (start[i] > absmaxs[i] && end[i] > absmaxs[i]) return qfalse;
	} //end for
	//check bounding box collision
	VectorSubtract(end, start, dir);
	frac = 1;
	for (i = 0; i < 3; i++)
	{
		//get plane to test collision with for the current axis direction
		if (dir[i] > 0) planedist = absmins[i];
		else planedist = absmaxs[i];
		//calculate collision fraction
		front = start[i] - planedist;
		back = end[i] - planedist;
		frac = front / (front-back);
		//check if between bounding planes of next axis
		side = i + 1;
		if (side > 2) side = 0;
		mid[side] = start[side] + dir[side] * frac;
		if (mid[side] > absmins[side] && mid[side] < absmaxs[side])
		{
			//check if between bounding planes of next axis
			side++;
			if (side > 2) side = 0;
			mid[side] = start[side] + dir[side] * frac;
			if (mid[side] > absmins[side] && mid[side] < absmaxs[side])
			{
				mid[i] = planedist;
				break;
			} //end if
		} //end if
	} //end for
	//if there was a collision
	if (i != 3)
	{
		trace->startsolid = qfalse;
		trace->fraction = frac;
		trace->ent = 0;
		trace->planenum = 0;
		trace->area = 0;
		trace->lastarea = 0;
		//trace endpos
		for (j = 0; j < 3; j++) trace->endpos[j] = start[j] + dir[j] * frac;
		return qtrue;
	} //end if
	return qfalse;
} //end of the function AAS_ClipToBBox
//===========================================================================
// predicts the movement
// assumes regular bounding box sizes
// NOTE: out of water jumping is not included
// NOTE: grappling hook is not included
//
// Parameter:			origin			: origin to start with
//						presencetype	: presence type to start with
//						velocity		: velocity to start with
//						cmdmove			: client command movement
//						cmdframes		: number of frame cmdmove is valid
//						maxframes		: maximum number of predicted frames
//						frametime		: duration of one predicted frame
//						stopevent		: events that stop the prediction
//						stopareanum		: stop as soon as entered this area
// Returns:				aas_clientmove_t
// Changes Globals:		-
//===========================================================================
int AAS_ClientMovementPrediction(struct aas_clientmove_s *move,
								int entnum, vec3_t origin,
								int presencetype, int onground,
								vec3_t velocity, vec3_t cmdmove,
								int cmdframes,
								int maxframes, float frametime,
								int stopevent, int stopareanum,
								vec3_t mins, vec3_t maxs, int visualize)
{
	float phys_friction, phys_stopspeed, phys_gravity, phys_waterfriction;
	float phys_watergravity;
	float phys_walkaccelerate, phys_airaccelerate, phys_swimaccelerate;
	float phys_maxwalkvelocity, phys_maxcrouchvelocity, phys_maxswimvelocity;
	float phys_maxstep, phys_maxsteepness, phys_jumpvel, friction;
	float gravity, delta, maxvel, wishspeed, accelerate;
	//float velchange, newvel;
	//int ax;
	int n, i, j, pc, step, swimming, crouch, event, jump_frame, areanum;
	int areas[20], numareas;
	vec3_t points[20];
	vec3_t org, end, feet, start, stepend, lastorg, wishdir;
	vec3_t frame_test_vel, old_frame_test_vel, left_test_vel;
	vec3_t up = {0, 0, 1};
	aas_plane_t *plane, *plane2;
	aas_trace_t trace, steptrace;
	
	if (frametime <= 0) frametime = 0.1f;
	//
	phys_friction = aassettings.phys_friction;
	phys_stopspeed = aassettings.phys_stopspeed;
	phys_gravity = aassettings.phys_gravity;
	phys_waterfriction = aassettings.phys_waterfriction;
	phys_watergravity = aassettings.phys_watergravity;
	phys_maxwalkvelocity = aassettings.phys_maxwalkvelocity;// * frametime;
	phys_maxcrouchvelocity = aassettings.phys_maxcrouchvelocity;// * frametime;
	phys_maxswimvelocity = aassettings.phys_maxswimvelocity;// * frametime;
	phys_walkaccelerate = aassettings.phys_walkaccelerate;
	phys_airaccelerate = aassettings.phys_airaccelerate;
	phys_swimaccelerate = aassettings.phys_swimaccelerate;
	phys_maxstep = aassettings.phys_maxstep;
	phys_maxsteepness = aassettings.phys_maxsteepness;
	phys_jumpvel = aassettings.phys_jumpvel * frametime;
	//
	Com_Memset(move, 0, sizeof(aas_clientmove_t));
	Com_Memset(&trace, 0, sizeof(aas_trace_t));
	//start at the current origin
	VectorCopy(origin, org);
	org[2] += 0.25;
	//velocity to test for the first frame
	VectorScale(velocity, frametime, frame_test_vel);
	//
	jump_frame = -1;
	//predict a maximum of 'maxframes' ahead
	for (n = 0; n < maxframes; n++)
	{
		swimming = AAS_Swimming(org);
		//get gravity depending on swimming or not
		gravity = swimming ? phys_watergravity : phys_gravity;
		//apply gravity at the START of the frame
		frame_test_vel[2] = frame_test_vel[2] - (gravity * 0.1 * frametime);
		//if on the ground or swimming
		if (onground || swimming)
		{
			friction = swimming ? phys_friction : phys_waterfriction;
			//apply friction
			VectorScale(frame_test_vel, 1/frametime, frame_test_vel);
			AAS_ApplyFriction(frame_test_vel, friction, phys_stopspeed, frametime);
			VectorScale(frame_test_vel, frametime, frame_test_vel);
		} //end if
		crouch = qfalse;
		//apply command movement
		if (n < cmdframes)
		{
			//ax = 0;
			maxvel = phys_maxwalkvelocity;
			accelerate = phys_airaccelerate;
			VectorCopy(cmdmove, wishdir);
			if (onground)
			{
				if (cmdmove[2] < -300)
				{
					crouch = qtrue;
					maxvel = phys_maxcrouchvelocity;
				} //end if
				//if not swimming and upmove is positive then jump
				if (!swimming && cmdmove[2] > 1)
				{
					//jump velocity minus the gravity for one frame + 5 for safety
					frame_test_vel[2] = phys_jumpvel - (gravity * 0.1 * frametime) + 5;
					jump_frame = n;
					//jumping so air accelerate
					accelerate = phys_airaccelerate;
				} //end if
				else
				{
					accelerate = phys_walkaccelerate;
				} //end else
				//ax = 2;
			} //end if
			if (swimming)
			{
				maxvel = phys_maxswimvelocity;
				accelerate = phys_swimaccelerate;
				//ax = 3;
			} //end if
			else
			{
				wishdir[2] = 0;
			} //end else
			//
			wishspeed = VectorNormalize(wishdir);
			if (wishspeed > maxvel) wishspeed = maxvel;
			VectorScale(frame_test_vel, 1/frametime, frame_test_vel);
			AAS_Accelerate(frame_test_vel, frametime, wishdir, wishspeed, accelerate);
			VectorScale(frame_test_vel, frametime, frame_test_vel);
			/*
			for (i = 0; i < ax; i++)
			{
				velchange = (cmdmove[i] * frametime) - frame_test_vel[i];
				if (velchange > phys_maxacceleration) velchange = phys_maxacceleration;
				else if (velchange < -phys_maxacceleration) velchange = -phys_maxacceleration;
				newvel = frame_test_vel[i] + velchange;
				//
				if (frame_test_vel[i] <= maxvel && newvel > maxvel) frame_test_vel[i] = maxvel;
				else if (frame_test_vel[i] >= -maxvel && newvel < -maxvel) frame_test_vel[i] = -maxvel;
				else frame_test_vel[i] = newvel;
			} //end for
			*/
		} //end if
		if (crouch)
		{
			presencetype = PRESENCE_CROUCH;
		} //end if
		else if (presencetype == PRESENCE_CROUCH)
		{
			if (AAS_PointPresenceType(org) & PRESENCE_NORMAL)
			{
				presencetype = PRESENCE_NORMAL;
			} //end if
		} //end else
		//save the current origin
		VectorCopy(org, lastorg);
		//move linear during one frame
		VectorCopy(frame_test_vel, left_test_vel);
		j = 0;
		do
		{
			VectorAdd(org, left_test_vel, end);
			//trace a bounding box
			trace = AAS_TraceClientBBox(org, end, presencetype, entnum);
			//
//#ifdef AAS_MOVE_DEBUG
			if (visualize)
			{
				if (trace.startsolid) botimport.Print(PRT_MESSAGE, "PredictMovement: start solid\n");
				AAS_DebugLine(org, trace.endpos, LINECOLOR_RED);
			} //end if
//#endif //AAS_MOVE_DEBUG
			//
			if (stopevent & (SE_ENTERAREA|SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER|SE_TOUCHCLUSTERPORTAL))
			{
				numareas = AAS_TraceAreas(org, trace.endpos, areas, points, 20);
				for (i = 0; i < numareas; i++)
				{
					if (stopevent & SE_ENTERAREA)
					{
						if (areas[i] == stopareanum)
						{
							VectorCopy(points[i], move->endpos);
							VectorScale(frame_test_vel, 1/frametime, move->velocity);
							move->endarea = areas[i];
							move->trace = trace;
							move->stopevent = SE_ENTERAREA;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						} //end if
					} //end if
					//NOTE: if not the first frame
					if ((stopevent & SE_TOUCHJUMPPAD) && n)
					{
						if (aasworld.areasettings[areas[i]].contents & AREACONTENTS_JUMPPAD)
						{
							VectorCopy(points[i], move->endpos);
							VectorScale(frame_test_vel, 1/frametime, move->velocity);
							move->endarea = areas[i];
							move->trace = trace;
							move->stopevent = SE_TOUCHJUMPPAD;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						} //end if
					} //end if
					if (stopevent & SE_TOUCHTELEPORTER)
					{
						if (aasworld.areasettings[areas[i]].contents & AREACONTENTS_TELEPORTER)
						{
							VectorCopy(points[i], move->endpos);
							move->endarea = areas[i];
							VectorScale(frame_test_vel, 1/frametime, move->velocity);
							move->trace = trace;
							move->stopevent = SE_TOUCHTELEPORTER;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						} //end if
					} //end if
					if (stopevent & SE_TOUCHCLUSTERPORTAL)
					{
						if (aasworld.areasettings[areas[i]].contents & AREACONTENTS_CLUSTERPORTAL)
						{
							VectorCopy(points[i], move->endpos);
							move->endarea = areas[i];
							VectorScale(frame_test_vel, 1/frametime, move->velocity);
							move->trace = trace;
							move->stopevent = SE_TOUCHCLUSTERPORTAL;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						} //end if
					} //end if
				} //end for
			} //end if
			//
			if (stopevent & SE_HITBOUNDINGBOX)
			{
				if (AAS_ClipToBBox(&trace, org, trace.endpos, presencetype, mins, maxs))
				{
					VectorCopy(trace.endpos, move->endpos);
					move->endarea = AAS_PointAreaNum(move->endpos);
					VectorScale(frame_test_vel, 1/frametime, move->velocity);
					move->trace = trace;
					move->stopevent = SE_HITBOUNDINGBOX;
					move->presencetype = presencetype;
					move->endcontents = 0;
					move->time = n * frametime;
					move->frames = n;
					return qtrue;
				} //end if
			} //end if
			//move the entity to the trace end point
			VectorCopy(trace.endpos, org);
			//if there was a collision
			if (trace.fraction < 1.0)
			{
				//get the plane the bounding box collided with
				plane = AAS_PlaneFromNum(trace.planenum);
				//
				if (stopevent & SE_HITGROUNDAREA)
				{
					if (DotProduct(plane->normal, up) > phys_maxsteepness)
					{
						VectorCopy(org, start);
						start[2] += 0.5;
						if (AAS_PointAreaNum(start) == stopareanum)
						{
							VectorCopy(start, move->endpos);
							move->endarea = stopareanum;
							VectorScale(frame_test_vel, 1/frametime, move->velocity);
							move->trace = trace;
							move->stopevent = SE_HITGROUNDAREA;
							move->presencetype = presencetype;
							move->endcontents = 0;
							move->time = n * frametime;
							move->frames = n;
							return qtrue;
						} //end if
					} //end if
				} //end if
				//assume there's no step
				step = qfalse;
				//if it is a vertical plane and the bot didn't jump recently
				if (plane->normal[2] == 0 && (jump_frame < 0 || n - jump_frame > 2))
				{
					//check for a step
					VectorMA(org, -0.25, plane->normal, start);
					VectorCopy(start, stepend);
					start[2] += phys_maxstep;
					steptrace = AAS_TraceClientBBox(start, stepend, presencetype, entnum);
					//
					if (!steptrace.startsolid)
					{
						plane2 = AAS_PlaneFromNum(steptrace.planenum);
						if (DotProduct(plane2->normal, up) > phys_maxsteepness)
						{
							VectorSubtract(end, steptrace.endpos, left_test_vel);
							left_test_vel[2] = 0;
							frame_test_vel[2] = 0;
//#ifdef AAS_MOVE_DEBUG
							if (visualize)
							{
								if (steptrace.endpos[2] - org[2] > 0.125)
								{
									VectorCopy(org, start);
									start[2] = steptrace.endpos[2];
									AAS_DebugLine(org, start, LINECOLOR_BLUE);
								} //end if
							} //end if
//#endif //AAS_MOVE_DEBUG
							org[2] = steptrace.endpos[2];
							step = qtrue;
						} //end if
					} //end if
				} //end if
				//
				if (!step)
				{
					//velocity left to test for this frame is the projection
					//of the current test velocity into the hit plane 
					VectorMA(left_test_vel, -DotProduct(left_test_vel, plane->normal),
										plane->normal, left_test_vel);
					//store the old velocity for landing check
					VectorCopy(frame_test_vel, old_frame_test_vel);
					//test velocity for the next frame is the projection
					//of the velocity of the current frame into the hit plane 
					VectorMA(frame_test_vel, -DotProduct(frame_test_vel, plane->normal),
										plane->normal, frame_test_vel);
					//check for a landing on an almost horizontal floor
					if (DotProduct(plane->normal, up) > phys_maxsteepness)
					{
						onground = qtrue;
					} //end if
					if (stopevent & SE_HITGROUNDDAMAGE)
					{
						delta = 0;
						if (old_frame_test_vel[2] < 0 &&
								frame_test_vel[2] > old_frame_test_vel[2] &&
								!onground)
						{
							delta = old_frame_test_vel[2];
						} //end if
						else if (onground)
						{
							delta = frame_test_vel[2] - old_frame_test_vel[2];
						} //end else
						if (delta)
						{
							delta = delta * 10;
							delta = delta * delta * 0.0001;
							if (swimming) delta = 0;
							// never take falling damage if completely underwater
							/*
							if (ent->waterlevel == 3) return;
							if (ent->waterlevel == 2) delta *= 0.25;
							if (ent->waterlevel == 1) delta *= 0.5;
							*/
							if (delta > 40)
							{
								VectorCopy(org, move->endpos);
								move->endarea = AAS_PointAreaNum(org);
								VectorCopy(frame_test_vel, move->velocity);
								move->trace = trace;
								move->stopevent = SE_HITGROUNDDAMAGE;
								move->presencetype = presencetype;
								move->endcontents = 0;
								move->time = n * frametime;
								move->frames = n;
								return qtrue;
							} //end if
						} //end if
					} //end if
				} //end if
			} //end if
			//extra check to prevent endless loop
			if (++j > 20) return qfalse;
		//while there is a plane hit
		} while(trace.fraction < 1.0);
		//if going down
		if (frame_test_vel[2] <= 10)
		{
			//check for a liquid at the feet of the bot
			VectorCopy(org, feet);
			feet[2] -= 22;
			pc = AAS_PointContents(feet);
			//get event from pc
			event = SE_NONE;
			if (pc & CONTENTS_LAVA) event |= SE_ENTERLAVA;
			if (pc & CONTENTS_SLIME) event |= SE_ENTERSLIME;
			if (pc & CONTENTS_WATER) event |= SE_ENTERWATER;
			//
			areanum = AAS_PointAreaNum(org);
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_LAVA)
				event |= SE_ENTERLAVA;
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_SLIME)
				event |= SE_ENTERSLIME;
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_WATER)
				event |= SE_ENTERWATER;
			//if in lava or slime
			if (event & stopevent)
			{
				VectorCopy(org, move->endpos);
				move->endarea = areanum;
				VectorScale(frame_test_vel, 1/frametime, move->velocity);
				move->stopevent = event & stopevent;
				move->presencetype = presencetype;
				move->endcontents = pc;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			} //end if
		} //end if
		//
		onground = AAS_OnGround(org, presencetype, entnum);
		//if onground and on the ground for at least one whole frame
		if (onground)
		{
			if (stopevent & SE_HITGROUND)
			{
				VectorCopy(org, move->endpos);
				move->endarea = AAS_PointAreaNum(org);
				VectorScale(frame_test_vel, 1/frametime, move->velocity);
				move->trace = trace;
				move->stopevent = SE_HITGROUND;
				move->presencetype = presencetype;
				move->endcontents = 0;
				move->time = n * frametime;
				move->frames = n;
				return qtrue;
			} //end if
		} //end if
		else if (stopevent & SE_LEAVEGROUND)
		{
			VectorCopy(org, move->endpos);
			move->endarea = AAS_PointAreaNum(org);
			VectorScale(frame_test_vel, 1/frametime, move->velocity);
			move->trace = trace;
			move->stopevent = SE_LEAVEGROUND;
			move->presencetype = presencetype;
			move->endcontents = 0;
			move->time = n * frametime;
			move->frames = n;
			return qtrue;
		} //end else if
		else if (stopevent & SE_GAP)
		{
			aas_trace_t gaptrace;

			VectorCopy(org, start);
			VectorCopy(start, end);
			end[2] -= 48 + aassettings.phys_maxbarrier;
			gaptrace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, -1);
			//if solid is found the bot cannot walk any further and will not fall into a gap
			if (!gaptrace.startsolid)
			{
				//if it is a gap (lower than one step height)
				if (gaptrace.endpos[2] < org[2] - aassettings.phys_maxstep - 1)
				{
					if (!(AAS_PointContents(end) & CONTENTS_WATER))
					{
						VectorCopy(lastorg, move->endpos);
						move->endarea = AAS_PointAreaNum(lastorg);
						VectorScale(frame_test_vel, 1/frametime, move->velocity);
						move->trace = trace;
						move->stopevent = SE_GAP;
						move->presencetype = presencetype;
						move->endcontents = 0;
						move->time = n * frametime;
						move->frames = n;
						return qtrue;
					} //end if
				} //end if
			} //end if
		} //end else if
	} //end for
	//
	VectorCopy(org, move->endpos);
	move->endarea = AAS_PointAreaNum(org);
	VectorScale(frame_test_vel, 1/frametime, move->velocity);
	move->stopevent = SE_NONE;
	move->presencetype = presencetype;
	move->endcontents = 0;
	move->time = n * frametime;
	move->frames = n;
	//
	return qtrue;
} //end of the function AAS_ClientMovementPrediction
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_PredictClientMovement(struct aas_clientmove_s *move,
								int entnum, vec3_t origin,
								int presencetype, int onground,
								vec3_t velocity, vec3_t cmdmove,
								int cmdframes,
								int maxframes, float frametime,
								int stopevent, int stopareanum, int visualize)
{
	vec3_t mins, maxs;
	return AAS_ClientMovementPrediction(move, entnum, origin, presencetype, onground,
										velocity, cmdmove, cmdframes, maxframes,
										frametime, stopevent, stopareanum,
										mins, maxs, visualize);
} //end of the function AAS_PredictClientMovement
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_ClientMovementHitBBox(struct aas_clientmove_s *move,
								int entnum, vec3_t origin,
								int presencetype, int onground,
								vec3_t velocity, vec3_t cmdmove,
								int cmdframes,
								int maxframes, float frametime,
								vec3_t mins, vec3_t maxs, int visualize)
{
	return AAS_ClientMovementPrediction(move, entnum, origin, presencetype, onground,
										velocity, cmdmove, cmdframes, maxframes,
										frametime, SE_HITBOUNDINGBOX, 0,
										mins, maxs, visualize);
} //end of the function AAS_ClientMovementHitBBox
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_TestMovementPrediction(int entnum, vec3_t origin, vec3_t dir)
{
	vec3_t velocity, cmdmove;
	aas_clientmove_t move;

	VectorClear(velocity);
	if (!AAS_Swimming(origin)) dir[2] = 0;
	VectorNormalize(dir);
	VectorScale(dir, 400, cmdmove);
	cmdmove[2] = 224;
	AAS_ClearShownDebugLines();
	AAS_PredictClientMovement(&move, entnum, origin, PRESENCE_NORMAL, qtrue,
									velocity, cmdmove, 13, 13, 0.1f, SE_HITGROUND, 0, qtrue);//SE_LEAVEGROUND);
	if (move.stopevent & SE_LEAVEGROUND)
	{
		botimport.Print(PRT_MESSAGE, "leave ground\n");
	} //end if
} //end of the function TestMovementPrediction
//===========================================================================
// calculates the horizontal velocity needed to perform a jump from start
// to end
//
// Parameter:			zvel	: z velocity for jump
//						start	: start position of jump
//						end		: end position of jump
//						*speed	: returned speed for jump
// Returns:				qfalse if too high or too far from start to end
// Changes Globals:		-
//===========================================================================
int AAS_HorizontalVelocityForJump(float zvel, vec3_t start, vec3_t end, float *velocity)
{
	float phys_gravity, phys_maxvelocity;
	float maxjump, height2fall, t, top;
	vec3_t dir;

	phys_gravity = aassettings.phys_gravity;
	phys_maxvelocity = aassettings.phys_maxvelocity;

	//maximum height a player can jump with the given initial z velocity
	maxjump = 0.5 * phys_gravity * (zvel / phys_gravity) * (zvel / phys_gravity);
	//top of the parabolic jump
	top = start[2] + maxjump;
	//height the bot will fall from the top
	height2fall = top - end[2];
	//if the goal is to high to jump to
	if (height2fall < 0)
	{
		*velocity = phys_maxvelocity;
		return 0;
	} //end if
	//time a player takes to fall the height
	t = sqrt(height2fall / (0.5 * phys_gravity));
  	//direction from start to end
	VectorSubtract(end, start, dir);
	//
	if ( (t + zvel / phys_gravity) == 0.0f ) {
		*velocity = phys_maxvelocity;
		return 0;
	}
	//calculate horizontal speed
	*velocity = sqrt(dir[0]*dir[0] + dir[1]*dir[1]) / (t + zvel / phys_gravity);
	//the horizontal speed must be lower than the max speed
	if (*velocity > phys_maxvelocity)
	{
		*velocity = phys_maxvelocity;
		return 0;
	} //end if
	return 1;
} //end of the function AAS_HorizontalVelocityForJump
