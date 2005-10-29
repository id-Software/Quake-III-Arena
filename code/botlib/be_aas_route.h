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
 * name:		be_aas_route.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_route.h $
 *
 *****************************************************************************/

#ifdef AASINTERN
//initialize the AAS routing
void AAS_InitRouting(void);
//free the AAS routing caches
void AAS_FreeRoutingCaches(void);
//returns the travel time from start to end in the given area
unsigned short int AAS_AreaTravelTime(int areanum, vec3_t start, vec3_t end);
//
void AAS_CreateAllRoutingCache(void);
void AAS_WriteRouteCache(void);
//
void AAS_RoutingInfo(void);
#endif //AASINTERN

//returns the travel flag for the given travel type
int AAS_TravelFlagForType(int traveltype);
//return the travel flag(s) for traveling through this area
int AAS_AreaContentsTravelFlags(int areanum);
//returns the index of the next reachability for the given area
int AAS_NextAreaReachability(int areanum, int reachnum);
//returns the reachability with the given index
void AAS_ReachabilityFromNum(int num, struct aas_reachability_s *reach);
//returns a random goal area and goal origin
int AAS_RandomGoalArea(int areanum, int travelflags, int *goalareanum, vec3_t goalorigin);
//enable or disable an area for routing
int AAS_EnableRoutingArea(int areanum, int enable);
//returns the travel time within the given area from start to end
unsigned short int AAS_AreaTravelTime(int areanum, vec3_t start, vec3_t end);
//returns the travel time from the area to the goal area using the given travel flags
int AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
//predict a route up to a stop event
int AAS_PredictRoute(struct aas_predictroute_s *route, int areanum, vec3_t origin,
							int goalareanum, int travelflags, int maxareas, int maxtime,
							int stopevent, int stopcontents, int stoptfl, int stopareanum);


