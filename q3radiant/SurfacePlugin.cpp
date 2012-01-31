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
// $Date: 2000/01/18 00:18:14 $
// $Log: SurfacePlugin.cpp,v $
// Revision 1.1.1.4  2000/01/18 00:18:14  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:43  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:12  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:46  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:42  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:46  TBesset
// Q3Radiant public version
// Revision 1.1.1.3  1999/12/29 18:31:46  TBesset
// Q3Radiant public version
//
// Revision 1.1  1999/12/02 16:11:05  Timo & Christine
// commit before e-brain dump
// TexTool v1.1 support
//
//
//
// DESCRIPTION:
// implementation of isurfaceplugin-interface specifics
//++timo FIXME: all the patch related stuff needs moving to qerplugin.h

#include "stdafx.h"

void WINAPI QERApp_GetTwoSelectedPatch( patchMesh_t **p1, patchMesh_t **p2 )
{
	*p1 = NULL; *p2 = NULL;
	for (brush_t *pb = selected_brushes.next ; pb != &selected_brushes ; pb = pb->next)
	{
		if (pb->patchBrush)
		{
			if (!(*p1))
				*p1 = pb->pPatch;
			else if (!(*p2))
			{
				*p2 = pb->pPatch;
				return;
			}
		}
	}
#ifdef _DEBUG
	Sys_Printf("WARNING: QERApp_GetTwoSelectedPatch failed (did not find two patches)\n");
#endif
	return;
	
}
