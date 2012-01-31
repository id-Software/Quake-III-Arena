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
// $Revision: 1.1.2.3 $
// $Author: ttimo $
// $Date: 2000/02/24 22:24:45 $
// $Log: IShaders.h,v $
// Revision 1.1.2.3  2000/02/24 22:24:45  ttimo
// RC2
//
// Revision 1.1.2.2  2000/02/11 03:52:30  ttimo
// working on the IShader interface
//
// Revision 1.1.2.1  2000/02/10 03:00:20  ttimo
// adding IShaders.h
//
//
// DESCRIPTION:
// a set of functions to manipulate textures in Radiant
// 

#ifndef __ISHADERS_H_
#define __ISHADERS_H_

// define a GUID for this interface so plugins can access and reference it
// {D42F798A-DF57-11d3-A3EE-0004AC96D4C3}
static const GUID QERShadersTable_GUID = 
{ 0xd42f798a, 0xdf57, 0x11d3, { 0xa3, 0xee, 0x0, 0x4, 0xac, 0x96, 0xd4, 0xc3 } };

//++timo NOTE: global strategy, when there's try before it means .. if the thing can't be found / loaded it will not
// create a default one

//++timo TODO: duplicate _QERFuncTable_1::m_pfnHasShader here?
//++timo FIXME: change the names to fit the changes we wanna do in the Radiant shader / textures code
//++timo NOTE: for Curry, this shader loading thingy is only provided so that we can update the Radiant texture window?
//             don't use yet .. will not implement yet
// get a shader, load it if needed
// returns NULL if shader doesn't exist
//++timo will reload the shader if already loaded? .. no, don't do that yet ..
//++timo I'm not happy with the name?
typedef qtexture_t* (WINAPI* PFN_TRYSHADERFORNAME) (const char* name);
// load a texture
// will not try loading a shader, will look for the actual image file ..
// returns NULL on file not found
// NOTE: don't put the .tga or .jpg filetype extension
// if returns NULL, it means the texture needs loading, use _QERSelectedFaceTable::m_pfnTextureForName
//++timo NOTE: all of this is hacks and workarounds, the shader code rewrite is supposed to solve all issues
typedef qtexture_t* (WINAPI* PFN_TRYTEXTUREFORNAME) (const char* filename);

struct _QERShadersTable
{
	int		m_nSize;
	// currently disabled
//	PFN_TRYLOADSHADER	m_pfnTryLoadShader;
	PFN_TRYTEXTUREFORNAME	m_pfnTryTextureForName;
};

#endif