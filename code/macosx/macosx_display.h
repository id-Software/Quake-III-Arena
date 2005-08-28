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
#include "tr_local.h"
#include "macosx_local.h"

@class NSDictionary;

extern NSDictionary *Sys_GetMatchingDisplayMode(qboolean allowStretchedModes);

extern void Sys_StoreGammaTables();
extern void Sys_GetGammaTable(glwgamma_t *table);
extern void Sys_SetScreenFade(glwgamma_t *table, float fraction);

extern void Sys_FadeScreens();
extern void Sys_FadeScreen(CGDirectDisplayID display);
extern void Sys_UnfadeScreens();
extern void Sys_UnfadeScreen(CGDirectDisplayID display, glwgamma_t *table);
extern void Sys_ReleaseAllDisplays();

