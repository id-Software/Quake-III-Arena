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
#include "stdafx.h"
#include "qe3.h"

void Curve_Invert (void) 
{
}

void Curve_MakeCurvedBrush (qboolean negative, qboolean top, qboolean bottom, 
					qboolean s1, qboolean s2, qboolean s3, qboolean s4) 
{
}


void Curve_BuildPoints (brush_t *b) 
{
}

void Curve_CameraDraw (brush_t *b) 
{
}

void Curve_XYDraw (brush_t *b) 
{
}

void Curve_WriteFile (char *name) 
{
}

void Curve_StripFakePlanes( brush_t *b )
{
}

void Curve_AddFakePlanes( brush_t *b ) 
{
}


void Patch_BrushToMesh(){};
void Patch_GenericMesh(int nWidth, int nHeight, int nOrientation){};
void Patch_ReadFile (char *name){};
void Patch_WriteFile (char *name){}; 
void Patch_BuildPoints (brush_t *b){};
void Patch_Move(int n, const vec3_t vMove){};
void Patch_ApplyMatrix(int n, const vec3_t vOrigin, const vec3_t vMatrix[3]){};
void Patch_EditPatch(int n){};
void Patch_Deselect(){};
void Patch_Delete(int n){};
void Patch_Select(int n){};
void Patch_Scale(int n, const vec3_t vOrigin, const vec3_t vAmt){};
void Patch_Cleanup(){};
bool g_bShowPatchBounds;
bool g_bPatchWireFrame;


