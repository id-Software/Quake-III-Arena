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
 * name:		be_interface.c
 *
 * desc:		bot library interface
 *
 * $Archive: /MissionPack/code/botlib/be_interface.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "be_interface.h"

#include "be_ea.h"
#include "be_ai_weight.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
#include "be_ai_chat.h"
#include "be_ai_char.h"
#include "be_ai_gen.h"

//library globals in a structure
botlib_globals_t botlibglobals;

botlib_export_t be_botlib_export;
botlib_import_t botimport;
//
int botDeveloper;
//qtrue if the library is setup
int botlibsetup = qfalse;

//===========================================================================
//
// several functions used by the exported functions
//
//===========================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Sys_MilliSeconds(void)
{
	return clock() * 1000 / CLOCKS_PER_SEC;
} //end of the function Sys_MilliSeconds
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidClientNumber(int num, char *str)
{
	if (num < 0 || num > botlibglobals.maxclients)
	{
		//weird: the disabled stuff results in a crash
		botimport.Print(PRT_ERROR, "%s: invalid client number %d, [0, %d]\n",
										str, num, botlibglobals.maxclients);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean ValidEntityNumber(int num, char *str)
{
	if (num < 0 || num > botlibglobals.maxentities)
	{
		botimport.Print(PRT_ERROR, "%s: invalid entity number %d, [0, %d]\n",
										str, num, botlibglobals.maxentities);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotValidateClientNumber
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BotLibSetup(char *str)
{
	if (!botlibglobals.botlibsetup)
	{
		botimport.Print(PRT_ERROR, "%s: bot library used before being setup\n", str);
		return qfalse;
	} //end if
	return qtrue;
} //end of the function BotLibSetup

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibSetup(void)
{
	int		errnum;
	
	botDeveloper = LibVarGetValue("bot_developer");
 	memset( &botlibglobals, 0, sizeof(botlibglobals) );
	//initialize byte swapping (litte endian etc.)
//	Swap_Init();

	if(botDeveloper)
	{
		char *homedir, *gamedir, *basedir;
		char logfilename[MAX_OSPATH];

		homedir = LibVarGetString("homedir");
		gamedir = LibVarGetString("gamedir");
		basedir = LibVarGetString("com_basegame");

		if (*homedir)
		{
			if(*gamedir)
				Com_sprintf(logfilename, sizeof(logfilename), "%s%c%s%cbotlib.log", homedir, PATH_SEP, gamedir, PATH_SEP);
			else if(*basedir)
				Com_sprintf(logfilename, sizeof(logfilename), "%s%c%s%cbotlib.log", homedir, PATH_SEP, basedir, PATH_SEP);
			else
				Com_sprintf(logfilename, sizeof(logfilename), "%s%c" BASEGAME "%cbotlib.log", homedir, PATH_SEP, PATH_SEP);
		}
		else
			Com_sprintf(logfilename, sizeof(logfilename), "botlib.log");
	
		Log_Open(logfilename);
	}

	botimport.Print(PRT_MESSAGE, "------- BotLib Initialization -------\n");

	botlibglobals.maxclients = (int) LibVarValue("maxclients", "128");
	botlibglobals.maxentities = (int) LibVarValue("maxentities", "1024");

	errnum = AAS_Setup();			//be_aas_main.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = EA_Setup();			//be_ea.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupWeaponAI();	//be_ai_weap.c
	if (errnum != BLERR_NOERROR)return errnum;
	errnum = BotSetupGoalAI();		//be_ai_goal.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupChatAI();		//be_ai_chat.c
	if (errnum != BLERR_NOERROR) return errnum;
	errnum = BotSetupMoveAI();		//be_ai_move.c
	if (errnum != BLERR_NOERROR) return errnum;

	botlibsetup = qtrue;
	botlibglobals.botlibsetup = qtrue;

	return BLERR_NOERROR;
} //end of the function Export_BotLibSetup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibShutdown(void)
{
	if (!BotLibSetup("BotLibShutdown")) return BLERR_LIBRARYNOTSETUP;
#ifndef DEMO
	//DumpFileCRCs();
#endif //DEMO
	//
	BotShutdownChatAI();		//be_ai_chat.c
	BotShutdownMoveAI();		//be_ai_move.c
	BotShutdownGoalAI();		//be_ai_goal.c
	BotShutdownWeaponAI();		//be_ai_weap.c
	BotShutdownWeights();		//be_ai_weight.c
	BotShutdownCharacters();	//be_ai_char.c
	//shud down aas
	AAS_Shutdown();
	//shut down bot elemantary actions
	EA_Shutdown();
	//free all libvars
	LibVarDeAllocAll();
	//remove all global defines from the pre compiler
	PC_RemoveAllGlobalDefines();

	//dump all allocated memory
//	DumpMemory();
#ifdef DEBUG
	PrintMemoryLabels();
#endif
	//shut down library log file
	Log_Shutdown();
	//
	botlibsetup = qfalse;
	botlibglobals.botlibsetup = qfalse;
	// print any files still open
	PC_CheckOpenSourceHandles();
	//
	return BLERR_NOERROR;
} //end of the function Export_BotLibShutdown
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarSet(char *var_name, char *value)
{
	LibVarSet(var_name, value);
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarSet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibVarGet(char *var_name, char *value, int size)
{
	char *varvalue;

	varvalue = LibVarGetString(var_name);
	strncpy(value, varvalue, size-1);
	value[size-1] = '\0';
	return BLERR_NOERROR;
} //end of the function Export_BotLibVarGet
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibStartFrame(float time)
{
	if (!BotLibSetup("BotStartFrame")) return BLERR_LIBRARYNOTSETUP;
	return AAS_StartFrame(time);
} //end of the function Export_BotLibStartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibLoadMap(const char *mapname)
{
#ifdef DEBUG
	int starttime = Sys_MilliSeconds();
#endif
	int errnum;

	if (!BotLibSetup("BotLoadMap")) return BLERR_LIBRARYNOTSETUP;
	//
	botimport.Print(PRT_MESSAGE, "------------ Map Loading ------------\n");
	//startup AAS for the current map, model and sound index
	errnum = AAS_LoadMap(mapname);
	if (errnum != BLERR_NOERROR) return errnum;
	//initialize the items in the level
	BotInitLevelItems();		//be_ai_goal.h
	BotSetBrushModelTypes();	//be_ai_move.h
	//
	botimport.Print(PRT_MESSAGE, "-------------------------------------\n");
#ifdef DEBUG
	botimport.Print(PRT_MESSAGE, "map loaded in %d msec\n", Sys_MilliSeconds() - starttime);
#endif
	//
	return BLERR_NOERROR;
} //end of the function Export_BotLibLoadMap
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Export_BotLibUpdateEntity(int ent, bot_entitystate_t *state)
{
	if (!BotLibSetup("BotUpdateEntity")) return BLERR_LIBRARYNOTSETUP;
	if (!ValidEntityNumber(ent, "BotUpdateEntity")) return BLERR_INVALIDENTITYNUMBER;

	return AAS_UpdateEntity(ent, state);
} //end of the function Export_BotLibUpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_TestMovementPrediction(int entnum, vec3_t origin, vec3_t dir);
void ElevatorBottomCenter(aas_reachability_t *reach, vec3_t bottomcenter);
int BotGetReachabilityToGoal(vec3_t origin, int areanum,
									  int lastgoalareanum, int lastareanum,
									  int *avoidreach, float *avoidreachtimes, int *avoidreachtries,
									  bot_goal_t *goal, int travelflags, int movetravelflags,
									  struct bot_avoidspot_s *avoidspots, int numavoidspots, int *flags);

int AAS_PointLight(vec3_t origin, int *red, int *green, int *blue);

int AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int AAS_Reachability_WeaponJump(int area1num, int area2num);

int BotFuzzyPointReachabilityArea(vec3_t origin);

float BotGapDistance(vec3_t origin, vec3_t hordir, int entnum);

void AAS_FloodAreas(vec3_t origin);

int BotExportTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3)
{

//	return AAS_PointLight(parm2, NULL, NULL, NULL);

#ifdef DEBUG
	static int area = -1;
	static int line[2];
	int newarea, i, highlightarea, flood;
//	int reachnum;
	vec3_t eye, forward, right, end, origin;
//	vec3_t bottomcenter;
//	aas_trace_t trace;
//	aas_face_t *face;
//	aas_entity_t *ent;
//	bsp_trace_t bsptrace;
//	aas_reachability_t reach;
//	bot_goal_t goal;

	// clock_t start_time, end_time;
	vec3_t mins = {-16, -16, -24};
	vec3_t maxs = {16, 16, 32};

//	int areas[10], numareas;


	//return 0;

	if (!aasworld.loaded) return 0;

	/*
	if (parm0 & 1)
	{
		AAS_ClearShownPolygons();
		AAS_FloodAreas(parm2);
	} //end if
	return 0;
	*/
	for (i = 0; i < 2; i++) if (!line[i]) line[i] = botimport.DebugLineCreate();

//	AAS_ClearShownDebugLines();

	//if (AAS_AgainstLadder(parm2)) botimport.Print(PRT_MESSAGE, "against ladder\n");
	//BotOnGround(parm2, PRESENCE_NORMAL, 1, &newarea, &newarea);
	//botimport.Print(PRT_MESSAGE, "%f %f %f\n", parm2[0], parm2[1], parm2[2]);
	//*
	highlightarea = LibVarGetValue("bot_highlightarea");
	if (highlightarea > 0)
	{
		newarea = highlightarea;
	} //end if
	else
	{
		VectorCopy(parm2, origin);
		origin[2] += 0.5;
		//newarea = AAS_PointAreaNum(origin);
		newarea = BotFuzzyPointReachabilityArea(origin);
	} //end else

	botimport.Print(PRT_MESSAGE, "\rtravel time to goal (%d) = %d  ", botlibglobals.goalareanum,
		AAS_AreaTravelTimeToGoalArea(newarea, origin, botlibglobals.goalareanum, TFL_DEFAULT));
	//newarea = BotReachabilityArea(origin, qtrue);
	if (newarea != area)
	{
		botimport.Print(PRT_MESSAGE, "origin = %f, %f, %f\n", origin[0], origin[1], origin[2]);
		area = newarea;
		botimport.Print(PRT_MESSAGE, "new area %d, cluster %d, presence type %d\n",
					area, AAS_AreaCluster(area), AAS_PointPresenceType(origin));
		botimport.Print(PRT_MESSAGE, "area contents: ");
		if (aasworld.areasettings[area].contents & AREACONTENTS_WATER)
		{
			botimport.Print(PRT_MESSAGE, "water &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_LAVA)
		{
			botimport.Print(PRT_MESSAGE, "lava &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_SLIME)
		{
			botimport.Print(PRT_MESSAGE, "slime &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_JUMPPAD)
		{
			botimport.Print(PRT_MESSAGE, "jump pad &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			botimport.Print(PRT_MESSAGE, "cluster portal &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_VIEWPORTAL)
		{
			botimport.Print(PRT_MESSAGE, "view portal &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_DONOTENTER)
		{
			botimport.Print(PRT_MESSAGE, "do not enter &");
		} //end if
		if (aasworld.areasettings[area].contents & AREACONTENTS_MOVER)
		{
			botimport.Print(PRT_MESSAGE, "mover &");
		} //end if
		if (!aasworld.areasettings[area].contents)
		{
			botimport.Print(PRT_MESSAGE, "empty");
		} //end if
		botimport.Print(PRT_MESSAGE, "\n");
		botimport.Print(PRT_MESSAGE, "travel time to goal (%d) = %d\n", botlibglobals.goalareanum,
					AAS_AreaTravelTimeToGoalArea(newarea, origin, botlibglobals.goalareanum, TFL_DEFAULT|TFL_ROCKETJUMP));
		/*
		VectorCopy(origin, end);
		end[2] += 5;
		numareas = AAS_TraceAreas(origin, end, areas, NULL, 10);
		AAS_TraceClientBBox(origin, end, PRESENCE_CROUCH, -1);
		botimport.Print(PRT_MESSAGE, "num areas = %d, area = %d\n", numareas, areas[0]);
		*/
		/*
		botlibglobals.goalareanum = newarea;
		VectorCopy(parm2, botlibglobals.goalorigin);
		botimport.Print(PRT_MESSAGE, "new goal %2.1f %2.1f %2.1f area %d\n",
								origin[0], origin[1], origin[2], newarea);
		*/
	} //end if
	//*
	flood = LibVarGetValue("bot_flood");
	if (parm0 & 1)
	{
		if (flood)
		{
			AAS_ClearShownPolygons();
			AAS_ClearShownDebugLines();
			AAS_FloodAreas(parm2);
		}
		else
		{
			botlibglobals.goalareanum = newarea;
			VectorCopy(parm2, botlibglobals.goalorigin);
			botimport.Print(PRT_MESSAGE, "new goal %2.1f %2.1f %2.1f area %d\n",
									origin[0], origin[1], origin[2], newarea);
		}
	} //end if*/
	if (flood)
		return 0;
//	if (parm0 & BUTTON_USE)
//	{
//		botlibglobals.runai = !botlibglobals.runai;
//		if (botlibglobals.runai) botimport.Print(PRT_MESSAGE, "started AI\n");
//		else botimport.Print(PRT_MESSAGE, "stopped AI\n");
		//* /
		/*
		goal.areanum = botlibglobals.goalareanum;
		reachnum = BotGetReachabilityToGoal(parm2, newarea, 1,
										ms.avoidreach, ms.avoidreachtimes,
										&goal, TFL_DEFAULT);
		if (!reachnum)
		{
			botimport.Print(PRT_MESSAGE, "goal not reachable\n");
		} //end if
		else
		{
			AAS_ReachabilityFromNum(reachnum, &reach);
			AAS_ClearShownDebugLines();
			AAS_ShowArea(area, qtrue);
			AAS_ShowArea(reach.areanum, qtrue);
			AAS_DrawCross(reach.start, 6, LINECOLOR_BLUE);
			AAS_DrawCross(reach.end, 6, LINECOLOR_RED);
			//
			if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_ELEVATOR)
			{
				ElevatorBottomCenter(&reach, bottomcenter);
				AAS_DrawCross(bottomcenter, 10, LINECOLOR_GREEN);
			} //end if
		} //end else*/
//		botimport.Print(PRT_MESSAGE, "travel time to goal = %d\n",
//					AAS_AreaTravelTimeToGoalArea(area, origin, botlibglobals.goalareanum, TFL_DEFAULT));
//		botimport.Print(PRT_MESSAGE, "test rj from 703 to 716\n");
//		AAS_Reachability_WeaponJump(703, 716);
//	} //end if*/

/*	face = AAS_AreaGroundFace(newarea, parm2);
	if (face)
	{
		AAS_ShowFace(face - aasworld.faces);
	} //end if*/
	/*
	AAS_ClearShownDebugLines();
	AAS_ShowArea(newarea, parm0 & BUTTON_USE);
	AAS_ShowReachableAreas(area);
	*/
	AAS_ClearShownPolygons();
	AAS_ClearShownDebugLines();
	AAS_ShowAreaPolygons(newarea, 1, parm0 & 4);
	if (parm0 & 2) AAS_ShowReachableAreas(area);
	else
	{
		static int lastgoalareanum, lastareanum;
		static int avoidreach[MAX_AVOIDREACH];
		static float avoidreachtimes[MAX_AVOIDREACH];
		static int avoidreachtries[MAX_AVOIDREACH];
		int reachnum, resultFlags;
		bot_goal_t goal;
		aas_reachability_t reach;

		/*
		goal.areanum = botlibglobals.goalareanum;
		VectorCopy(botlibglobals.goalorigin, goal.origin);
		reachnum = BotGetReachabilityToGoal(origin, newarea,
									  lastgoalareanum, lastareanum,
									  avoidreach, avoidreachtimes, avoidreachtries,
									  &goal, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP,
									  NULL, 0, &resultFlags);
		AAS_ReachabilityFromNum(reachnum, &reach);
		AAS_ShowReachability(&reach);
		*/
		int curarea;
		vec3_t curorigin;

		goal.areanum = botlibglobals.goalareanum;
		VectorCopy(botlibglobals.goalorigin, goal.origin);
		VectorCopy(origin, curorigin);
		curarea = newarea;
		for ( i = 0; i < 100; i++ ) {
			if ( curarea == goal.areanum ) {
				break;
			}
			reachnum = BotGetReachabilityToGoal(curorigin, curarea,
										  lastgoalareanum, lastareanum,
										  avoidreach, avoidreachtimes, avoidreachtries,
										  &goal, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP, TFL_DEFAULT|TFL_FUNCBOB|TFL_ROCKETJUMP,
										  NULL, 0, &resultFlags);
			AAS_ReachabilityFromNum(reachnum, &reach);
			AAS_ShowReachability(&reach);
			VectorCopy(reach.end, origin);
			lastareanum = curarea;
			curarea = reach.areanum;
		}
	} //end else
	VectorClear(forward);
	//BotGapDistance(origin, forward, 0);
	/*
	if (parm0 & BUTTON_USE)
	{
		botimport.Print(PRT_MESSAGE, "test rj from 703 to 716\n");
		AAS_Reachability_WeaponJump(703, 716);
	} //end if*/

	AngleVectors(parm3, forward, right, NULL);
	//get the eye 16 units to the right of the origin
	VectorMA(parm2, 8, right, eye);
	//get the eye 24 units up
	eye[2] += 24;
	//get the end point for the line to be traced
	VectorMA(eye, 800, forward, end);

//	AAS_TestMovementPrediction(1, parm2, forward);
/*
    //trace the line to find the hit point
	trace = AAS_TraceClientBBox(eye, end, PRESENCE_NORMAL, 1);
	if (!line[0]) line[0] = botimport.DebugLineCreate();
	botimport.DebugLineShow(line[0], eye, trace.endpos, LINECOLOR_BLUE);
	//
	AAS_ClearShownDebugLines();
	if (trace.ent)
	{
		ent = &aasworld.entities[trace.ent];
		AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
	} //end if
*/

/*
	start_time = clock();
	for (i = 0; i < 2000; i++)
	{
		AAS_Trace2(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
//		AAS_TraceClientBBox(eye, end, PRESENCE_NORMAL, 1);
	} //end for
	end_time = clock();
	botimport.Print(PRT_MESSAGE, "me %lu clocks, %lu CLOCKS_PER_SEC\n", end_time - start_time, CLOCKS_PER_SEC);
	start_time = clock();
	for (i = 0; i < 2000; i++)
	{
		AAS_Trace(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	} //end for
	end_time = clock();
	botimport.Print(PRT_MESSAGE, "id %lu clocks, %lu CLOCKS_PER_SEC\n", end_time - start_time, CLOCKS_PER_SEC);
*/

    // TTimo: nested comments are BAD for gcc -Werror, use #if 0 instead..
#if 0
	AAS_ClearShownDebugLines();
	//bsptrace = AAS_Trace(eye, NULL, NULL, end, 1, MASK_PLAYERSOLID);
	bsptrace = AAS_Trace(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	if (!line[0]) line[0] = botimport.DebugLineCreate();
	botimport.DebugLineShow(line[0], eye, bsptrace.endpos, LINECOLOR_YELLOW);
	if (bsptrace.fraction < 1.0)
	{
		face = AAS_TraceEndFace(&trace);
		if (face)
		{
			AAS_ShowFace(face - aasworld.faces);
		} //end if
		
		AAS_DrawPlaneCross(bsptrace.endpos,
									bsptrace.plane.normal,
									bsptrace.plane.dist + bsptrace.exp_dist,
									bsptrace.plane.type, LINECOLOR_GREEN);
		if (trace.ent)
		{
			ent = &aasworld.entities[trace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		} //end if
	} //end if
	//bsptrace = AAS_Trace2(eye, NULL, NULL, end, 1, MASK_PLAYERSOLID);
	bsptrace = AAS_Trace2(eye, mins, maxs, end, 1, MASK_PLAYERSOLID);
	botimport.DebugLineShow(line[1], eye, bsptrace.endpos, LINECOLOR_BLUE);
	if (bsptrace.fraction < 1.0)
	{
		AAS_DrawPlaneCross(bsptrace.endpos,
									bsptrace.plane.normal,
									bsptrace.plane.dist,// + bsptrace.exp_dist,
									bsptrace.plane.type, LINECOLOR_RED);
		if (bsptrace.ent)
		{
			ent = &aasworld.entities[bsptrace.ent];
			AAS_ShowBoundingBox(ent->origin, ent->mins, ent->maxs);
		} //end if
	} //end if
#endif
#endif
	return 0;
} //end of the function BotExportTest


/*
============
Init_AAS_Export
============
*/
static void Init_AAS_Export( aas_export_t *aas ) {
	//--------------------------------------------
	// be_aas_entity.c
	//--------------------------------------------
	aas->AAS_EntityInfo = AAS_EntityInfo;
	//--------------------------------------------
	// be_aas_main.c
	//--------------------------------------------
	aas->AAS_Initialized = AAS_Initialized;
	aas->AAS_PresenceTypeBoundingBox = AAS_PresenceTypeBoundingBox;
	aas->AAS_Time = AAS_Time;
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	aas->AAS_PointAreaNum = AAS_PointAreaNum;
	aas->AAS_PointReachabilityAreaIndex = AAS_PointReachabilityAreaIndex;
	aas->AAS_TraceAreas = AAS_TraceAreas;
	aas->AAS_BBoxAreas = AAS_BBoxAreas;
	aas->AAS_AreaInfo = AAS_AreaInfo;
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	aas->AAS_PointContents = AAS_PointContents;
	aas->AAS_NextBSPEntity = AAS_NextBSPEntity;
	aas->AAS_ValueForBSPEpairKey = AAS_ValueForBSPEpairKey;
	aas->AAS_VectorForBSPEpairKey = AAS_VectorForBSPEpairKey;
	aas->AAS_FloatForBSPEpairKey = AAS_FloatForBSPEpairKey;
	aas->AAS_IntForBSPEpairKey = AAS_IntForBSPEpairKey;
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	aas->AAS_AreaReachability = AAS_AreaReachability;
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	aas->AAS_AreaTravelTimeToGoalArea = AAS_AreaTravelTimeToGoalArea;
	aas->AAS_EnableRoutingArea = AAS_EnableRoutingArea;
	aas->AAS_PredictRoute = AAS_PredictRoute;
	//--------------------------------------------
	// be_aas_altroute.c
	//--------------------------------------------
	aas->AAS_AlternativeRouteGoals = AAS_AlternativeRouteGoals;
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	aas->AAS_Swimming = AAS_Swimming;
	aas->AAS_PredictClientMovement = AAS_PredictClientMovement;
}

  
/*
============
Init_EA_Export
============
*/
static void Init_EA_Export( ea_export_t *ea ) {
	//ClientCommand elementary actions
	ea->EA_Command = EA_Command;
	ea->EA_Say = EA_Say;
	ea->EA_SayTeam = EA_SayTeam;

	ea->EA_Action = EA_Action;
	ea->EA_Gesture = EA_Gesture;
	ea->EA_Talk = EA_Talk;
	ea->EA_Attack = EA_Attack;
	ea->EA_Use = EA_Use;
	ea->EA_Respawn = EA_Respawn;
	ea->EA_Crouch = EA_Crouch;
	ea->EA_MoveUp = EA_MoveUp;
	ea->EA_MoveDown = EA_MoveDown;
	ea->EA_MoveForward = EA_MoveForward;
	ea->EA_MoveBack = EA_MoveBack;
	ea->EA_MoveLeft = EA_MoveLeft;
	ea->EA_MoveRight = EA_MoveRight;

	ea->EA_SelectWeapon = EA_SelectWeapon;
	ea->EA_Jump = EA_Jump;
	ea->EA_DelayedJump = EA_DelayedJump;
	ea->EA_Move = EA_Move;
	ea->EA_View = EA_View;
	ea->EA_GetInput = EA_GetInput;
	ea->EA_EndRegular = EA_EndRegular;
	ea->EA_ResetInput = EA_ResetInput;
}


/*
============
Init_AI_Export
============
*/
static void Init_AI_Export( ai_export_t *ai ) {
	//-----------------------------------
	// be_ai_char.h
	//-----------------------------------
	ai->BotLoadCharacter = BotLoadCharacter;
	ai->BotFreeCharacter = BotFreeCharacter;
	ai->Characteristic_Float = Characteristic_Float;
	ai->Characteristic_BFloat = Characteristic_BFloat;
	ai->Characteristic_Integer = Characteristic_Integer;
	ai->Characteristic_BInteger = Characteristic_BInteger;
	ai->Characteristic_String = Characteristic_String;
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	ai->BotAllocChatState = BotAllocChatState;
	ai->BotFreeChatState = BotFreeChatState;
	ai->BotQueueConsoleMessage = BotQueueConsoleMessage;
	ai->BotRemoveConsoleMessage = BotRemoveConsoleMessage;
	ai->BotNextConsoleMessage = BotNextConsoleMessage;
	ai->BotNumConsoleMessages = BotNumConsoleMessages;
	ai->BotInitialChat = BotInitialChat;
	ai->BotNumInitialChats = BotNumInitialChats;
	ai->BotReplyChat = BotReplyChat;
	ai->BotChatLength = BotChatLength;
	ai->BotEnterChat = BotEnterChat;
	ai->BotGetChatMessage = BotGetChatMessage;
	ai->StringContains = StringContains;
	ai->BotFindMatch = BotFindMatch;
	ai->BotMatchVariable = BotMatchVariable;
	ai->UnifyWhiteSpaces = UnifyWhiteSpaces;
	ai->BotReplaceSynonyms = BotReplaceSynonyms;
	ai->BotLoadChatFile = BotLoadChatFile;
	ai->BotSetChatGender = BotSetChatGender;
	ai->BotSetChatName = BotSetChatName;
	//-----------------------------------
	// be_ai_goal.h
	//-----------------------------------
	ai->BotResetGoalState = BotResetGoalState;
	ai->BotResetAvoidGoals = BotResetAvoidGoals;
	ai->BotRemoveFromAvoidGoals = BotRemoveFromAvoidGoals;
	ai->BotPushGoal = BotPushGoal;
	ai->BotPopGoal = BotPopGoal;
	ai->BotEmptyGoalStack = BotEmptyGoalStack;
	ai->BotDumpAvoidGoals = BotDumpAvoidGoals;
	ai->BotDumpGoalStack = BotDumpGoalStack;
	ai->BotGoalName = BotGoalName;
	ai->BotGetTopGoal = BotGetTopGoal;
	ai->BotGetSecondGoal = BotGetSecondGoal;
	ai->BotChooseLTGItem = BotChooseLTGItem;
	ai->BotChooseNBGItem = BotChooseNBGItem;
	ai->BotTouchingGoal = BotTouchingGoal;
	ai->BotItemGoalInVisButNotVisible = BotItemGoalInVisButNotVisible;
	ai->BotGetLevelItemGoal = BotGetLevelItemGoal;
	ai->BotGetNextCampSpotGoal = BotGetNextCampSpotGoal;
	ai->BotGetMapLocationGoal = BotGetMapLocationGoal;
	ai->BotAvoidGoalTime = BotAvoidGoalTime;
	ai->BotSetAvoidGoalTime = BotSetAvoidGoalTime;
	ai->BotInitLevelItems = BotInitLevelItems;
	ai->BotUpdateEntityItems = BotUpdateEntityItems;
	ai->BotLoadItemWeights = BotLoadItemWeights;
	ai->BotFreeItemWeights = BotFreeItemWeights;
	ai->BotInterbreedGoalFuzzyLogic = BotInterbreedGoalFuzzyLogic;
	ai->BotSaveGoalFuzzyLogic = BotSaveGoalFuzzyLogic;
	ai->BotMutateGoalFuzzyLogic = BotMutateGoalFuzzyLogic;
	ai->BotAllocGoalState = BotAllocGoalState;
	ai->BotFreeGoalState = BotFreeGoalState;
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	ai->BotResetMoveState = BotResetMoveState;
	ai->BotMoveToGoal = BotMoveToGoal;
	ai->BotMoveInDirection = BotMoveInDirection;
	ai->BotResetAvoidReach = BotResetAvoidReach;
	ai->BotResetLastAvoidReach = BotResetLastAvoidReach;
	ai->BotReachabilityArea = BotReachabilityArea;
	ai->BotMovementViewTarget = BotMovementViewTarget;
	ai->BotPredictVisiblePosition = BotPredictVisiblePosition;
	ai->BotAllocMoveState = BotAllocMoveState;
	ai->BotFreeMoveState = BotFreeMoveState;
	ai->BotInitMoveState = BotInitMoveState;
	ai->BotAddAvoidSpot = BotAddAvoidSpot;
	//-----------------------------------
	// be_ai_weap.h
	//-----------------------------------
	ai->BotChooseBestFightWeapon = BotChooseBestFightWeapon;
	ai->BotGetWeaponInfo = BotGetWeaponInfo;
	ai->BotLoadWeaponWeights = BotLoadWeaponWeights;
	ai->BotAllocWeaponState = BotAllocWeaponState;
	ai->BotFreeWeaponState = BotFreeWeaponState;
	ai->BotResetWeaponState = BotResetWeaponState;
	//-----------------------------------
	// be_ai_gen.h
	//-----------------------------------
	ai->GeneticParentsAndChildSelection = GeneticParentsAndChildSelection;
}


/*
============
GetBotLibAPI
============
*/
botlib_export_t *GetBotLibAPI(int apiVersion, botlib_import_t *import) {
	assert(import);
	botimport = *import;
	assert(botimport.Print);

	Com_Memset( &be_botlib_export, 0, sizeof( be_botlib_export ) );

	if ( apiVersion != BOTLIB_API_VERSION ) {
		botimport.Print( PRT_ERROR, "Mismatched BOTLIB_API_VERSION: expected %i, got %i\n", BOTLIB_API_VERSION, apiVersion );
		return NULL;
	}

	Init_AAS_Export(&be_botlib_export.aas);
	Init_EA_Export(&be_botlib_export.ea);
	Init_AI_Export(&be_botlib_export.ai);

	be_botlib_export.BotLibSetup = Export_BotLibSetup;
	be_botlib_export.BotLibShutdown = Export_BotLibShutdown;
	be_botlib_export.BotLibVarSet = Export_BotLibVarSet;
	be_botlib_export.BotLibVarGet = Export_BotLibVarGet;

	be_botlib_export.PC_AddGlobalDefine = PC_AddGlobalDefine;
	be_botlib_export.PC_LoadSourceHandle = PC_LoadSourceHandle;
	be_botlib_export.PC_FreeSourceHandle = PC_FreeSourceHandle;
	be_botlib_export.PC_ReadTokenHandle = PC_ReadTokenHandle;
	be_botlib_export.PC_SourceFileAndLine = PC_SourceFileAndLine;

	be_botlib_export.BotLibStartFrame = Export_BotLibStartFrame;
	be_botlib_export.BotLibLoadMap = Export_BotLibLoadMap;
	be_botlib_export.BotLibUpdateEntity = Export_BotLibUpdateEntity;
	be_botlib_export.Test = BotExportTest;

	return &be_botlib_export;
}
