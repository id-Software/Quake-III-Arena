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
 * name:		be_aas_debug.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_debug.h $
 *
 *****************************************************************************/

//clear the shown debug lines
void AAS_ClearShownDebugLines(void);
//
void AAS_ClearShownPolygons(void);
//show a debug line
void AAS_DebugLine(vec3_t start, vec3_t end, int color);
//show a permenent line
void AAS_PermanentLine(vec3_t start, vec3_t end, int color);
//show a permanent cross
void AAS_DrawPermanentCross(vec3_t origin, float size, int color);
//draw a cross in the plane
void AAS_DrawPlaneCross(vec3_t point, vec3_t normal, float dist, int type, int color);
//show a bounding box
void AAS_ShowBoundingBox(vec3_t origin, vec3_t mins, vec3_t maxs);
//show a face
void AAS_ShowFace(int facenum);
//show an area
void AAS_ShowArea(int areanum, int groundfacesonly);
//
void AAS_ShowAreaPolygons(int areanum, int color, int groundfacesonly);
//draw a cros
void AAS_DrawCross(vec3_t origin, float size, int color);
//print the travel type
void AAS_PrintTravelType(int traveltype);
//draw an arrow
void AAS_DrawArrow(vec3_t start, vec3_t end, int linecolor, int arrowcolor);
//visualize the given reachability
void AAS_ShowReachability(struct aas_reachability_s *reach);
//show the reachable areas from the given area
void AAS_ShowReachableAreas(int areanum);

