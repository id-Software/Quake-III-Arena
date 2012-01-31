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
// $Revision: 1.1.1.4.2.1 $
// $Author: ttimo $
// $Date: 2000/02/04 22:59:34 $
// $log$
// Revision 1.1.1.3  1999/12/29 18:31:26  TBesset
// Q3Radiant public version
//
// Revision 1.2  1999/11/22 17:46:45  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.4.1  1999/11/03 20:37:59  Timo & Christine
// MEAN plugin for Q3Radiant, alpha version
//
// Revision 1.1.2.1  1999/10/27 08:34:26  Timo & Christine
// preview version of the texture tools plugin is ready
// ( TexTool.dll plugin is in Q3Plugin module )
// plugins can draw in their own window using Radiant's qgl bindings
//
//
// DESCRIPTION:
// Quick interface hack for selected face interface
// this one really needs more work, but I'm in a hurry with TexTool

#include "stdafx.h"

// stores objects that want to be hooked into drawing in the XY window
//++timo TODO: add support for camera view, Z view ... (texture view?)
CPtrArray l_GLWindows;

int WINAPI QERApp_ISelectedFace_GetTextureNumber()
{
	if (g_ptrSelectedFaces.GetSize() > 0)
	{
		face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
		return selFace->d_texture->texture_number;
	}
	//++timo hu ? find the appropriate gl bind number
	return 0;
}

void WINAPI QERApp_HookXYGLWindow(IGLWindow* pGLW)
{
	l_GLWindows.Add( pGLW );
	pGLW->IncRef();
}

void WINAPI QERApp_UnHookGLWindow(IGLWindow* pGLW)
{
	for( int i = 0; i < l_GLWindows.GetSize(); i++ )
	{
		if (l_GLWindows.GetAt(i) == pGLW)
		{
			l_GLWindows.RemoveAt(i);
			pGLW->DecRef();
			return;
		}
	}
#ifdef _DEBUG
	Sys_Printf("ERROR: IGLWindow* not found in QERApp_UnHookGLWindow\n");
#endif
}

void DrawPluginEntities( VIEWTYPE vt )
{
	for(int i = 0; i<l_GLWindows.GetSize(); i++ )
		static_cast<IGLWindow*>(l_GLWindows.GetAt(i))->Draw( vt );
}