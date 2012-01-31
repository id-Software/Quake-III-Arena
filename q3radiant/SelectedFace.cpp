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
//-----------------------------------------------------------------------------
//
// $LogFile$
// $Revision: 1.1.1.4 $
// $Author: ttimo $
// $Date: 2000/01/18 00:18:13 $
// $Log: SelectedFace.cpp,v $
// Revision 1.1.1.4  2000/01/18 00:18:13  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:43  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:12  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:45  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:41  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:45  TBesset
// Q3Radiant public version
//
// Revision 1.2  1999/11/22 17:46:47  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.4.2  1999/11/14 16:26:13  Timo & Christine
// first beta of the ritualmap surface plugin
//
// Revision 1.1.4.1  1999/11/03 20:38:02  Timo & Christine
// MEAN plugin for Q3Radiant, alpha version
//
// Revision 1.1.2.1  1999/10/27 08:34:28  Timo & Christine
// preview version of the texture tools plugin is ready
// ( TexTool.dll plugin is in Q3Plugin module )
// plugins can draw in their own window using Radiant's qgl bindings
//
//
// DESCRIPTION:
// Quick interface hack for selected face interface
// this one really needs more work, but I'm in a hurry with TexTool

#include "stdafx.h"

HGLRC WINAPI QERApp_GetQeglobalsHGLRC()
{
	return g_qeglobals.d_hglrcBase;
}

// pWinding is supposed to have MAX_POINTS_ON_WINDING
int WINAPI QERApp_GetFaceInfo(_QERFaceData *pFaceData, winding_t *pWinding)
{
	int size;

	if (g_ptrSelectedFaces.GetSize() > 0)
	{
		if (!g_qeglobals.m_bBrushPrimitMode)
		{
			Sys_Printf("Warning: unexpected QERApp_GetFaceInfo out of brush primitive mode\n");
			return 0;
		}
		//++timo NOTE: let's put only what we need for now
    face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
		strcpy( pFaceData->m_TextureName, selFace->texdef.name );
		VectorCopy( selFace->planepts[0], pFaceData->m_v1 );
		VectorCopy( selFace->planepts[1], pFaceData->m_v2 );
		VectorCopy( selFace->planepts[2], pFaceData->m_v3 );
		pFaceData->m_bBPrimit = true;
		memcpy( &pFaceData->brushprimit_texdef, &selFace->brushprimit_texdef, sizeof(brushprimit_texdef_t) );
		size = (int)((winding_t *)0)->points[selFace->face_winding->numpoints];
		memcpy( pWinding, selFace->face_winding, size );
		return 1;
	}
	return 0;
}

int WINAPI QERApp_SetFaceInfo(_QERFaceData *pFaceData)
{
	if (g_ptrSelectedFaces.GetSize() > 0)
	{
		if (!g_qeglobals.m_bBrushPrimitMode)
		{
			Sys_Printf("Warning: unexpected QERApp_SetFaceInfo out of brush primitive mode\n");
			return 0;
		}
    face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
    brush_t *selBrush = reinterpret_cast<brush_t*>(g_ptrSelectedFaceBrushes.GetAt(0));
		//strcpy( selected_face->texdef.name, pFaceData->m_TextureName );
		selFace->texdef.SetName(pFaceData->m_TextureName);
		VectorCopy( pFaceData->m_v1, selFace->planepts[0] );
		VectorCopy( pFaceData->m_v2, selFace->planepts[1] );
		VectorCopy( pFaceData->m_v3, selFace->planepts[2] );
		memcpy( &selFace->brushprimit_texdef, &pFaceData->brushprimit_texdef, sizeof(brushprimit_texdef_t) );
		Brush_Build( selBrush );
		Sys_UpdateWindows(W_ALL);
		return 1;
	}
	return 0;
}

void WINAPI QERApp_GetTextureSize( int Size[2] )
{
	if (g_ptrSelectedFaces.GetSize() > 0)
	{
    face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
		Size[0] = selFace->d_texture->width;
		Size[1] = selFace->d_texture->height;
	}
	else
		Sys_Printf("WARNING: unexpected call to QERApp_GetTextureSize with no selected_face\n");
}