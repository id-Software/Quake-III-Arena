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
// $Date: 2000/02/10 03:00:20 $
// $Log: ISelectedFace.h,v $
// Revision 1.1.1.4.2.1  2000/02/10 03:00:20  ttimo
// adding IShaders.h
//
// Revision 1.1.1.4  2000/01/18 00:17:12  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:42  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:10  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:35  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
// Revision 1.1.1.3  1999/12/29 18:31:27  TBesset
// Q3Radiant public version
//
// Revision 1.2  1999/11/22 17:46:45  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.4.3  1999/11/15 22:39:40  Timo & Christine
// bug fixing / additional features after update 173 from Robert
//
// Revision 1.1.4.2  1999/11/14 16:26:10  Timo & Christine
// first beta of the ritualmap surface plugin
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

#ifndef __ISELECTEDFACE_H_
#define __ISELECTEDFACE_H_

// define a GUID for this interface so plugins can access and reference it
// {5416A2A0-8633-11d3-8EF3-88B61F3A3B12}
static const GUID QERSelectedFaceTable_GUID = 
{ 0x5416a2a0, 0x8633, 0x11d3, { 0x8e, 0xf3, 0x88, 0xb6, 0x1f, 0x3a, 0x3b, 0x12 } };

//++timo TODO: this interface needs some cleanup with the new texture / shaders interface

// retrieve the texture number to bind to
typedef int		(WINAPI* PFN_GETTEXTURENUMBER)	();
// winding_t is assumed to have MAX_POINTS_ON_WINDING allocated
typedef int		(WINAPI* PFN_GETFACEINFO)		(_QERFaceData*, winding_t* );
// tell editor to update the selected face data
typedef int		(WINAPI* PFN_SETFACEINFO)		(_QERFaceData*);
// retrieving some texture information
typedef void	(WINAPI* PFN_GETTEXTURESIZE)	( int Size[2] );
// loading the qtexture_t from the texture name
typedef qtexture_t* (WINAPI* PFN_TEXTUREFORNAME) ( const char *name );
// straight func pointer to Select_SetTexture
// last parameter must be casted to an IPluginTexdef
typedef void	(WINAPI* PFN_SELECT_SETTEXTURE)		(texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale, void* pPlugTexdef );

// NOTE: some things in there are not really related to the selected face
// having some stuff moved into a textures-dedicated part ?
struct _QERSelectedFaceTable
{
	int m_nSize;
	PFN_GETTEXTURENUMBER	m_pfnGetTextureNumber;
	PFN_GETFACEINFO			m_pfnGetFaceInfo;
	PFN_SETFACEINFO			m_pfnSetFaceInfo;
	PFN_GETTEXTURESIZE		m_pfnGetTextureSize;
	PFN_TEXTUREFORNAME		m_pfnTextureForName;
	PFN_SELECT_SETTEXTURE	m_pfnSelect_SetTexture;
};

#endif