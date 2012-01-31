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
// $Date: 2000/02/13 17:25:04 $
// $Log: isurfaceplugin.h,v $
// Revision 1.1.1.4.2.1  2000/02/13 17:25:04  ttimo
// started cleanup
//
// Revision 1.1.1.4  2000/01/18 00:17:13  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:44  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:13  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:28  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:45  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:28  TBesset
// Q3Radiant public version
// Revision 1.1.1.3  1999/12/29 18:31:28  TBesset
// Q3Radiant public version
//
// Revision 1.2  1999/11/22 17:46:48  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.2.4  1999/11/20 12:13:02  Timo & Christine
// first release to Wolfen and Spog
// with TexTool and DeQbsp3 plugin
//
// Revision 1.1.2.3  1999/11/16 14:23:13  Timo & Christine
// merged 173 into Q3Radiant, EARadiant branch
// see Timo\changelog.txt for merge log
//
// Revision 1.1.2.2  1999/11/14 16:26:15  Timo & Christine
// first beta of the ritualmap surface plugin
//
// Revision 1.1.2.1  1999/10/08 16:28:15  Timo & Christine
// started plugin extensions for EA features support in Q3Radiant
// MEAN files plugin, and Surface Properties plugin
//
//
// DESCRIPTION:
// 
//

#ifndef __ISURFACEPLUGIN_H_
#define __ISURFACEPLUGIN_H_

#ifndef Q3RADIANT

// this is plugin included, define miptex_t
//++timo FIXME: move definition into qertypes.h ? or assume plugin knows ?
#define	MIPLEVELS	4
typedef struct miptex_s
{
	char		name[32];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	char		animname[32];			// next frame in animation chain
	int			flags;
	int			contents;
	int			value;
} miptex_t;

#endif

/*
// there's a void* in each qtexture_t, must be casted to a IPluginQTexture*
class IPluginQTexture
{
public:
	// Increment the number of references to this object
	virtual void IncRef () = 0;
	// Decrement the reference count
	virtual void DecRef () = 0;
	// Init for a miptex_t
	virtual void InitForMiptex( miptex_t* ) = 0;
	// set default texdef
	virtual void SetDefaultTexdef() = 0;
};

// use this macro
#define GETPLUGINQTEXTURE(pQTexture) (static_cast<IPluginQTexture *>(pQTexture->pData))
*/

// there's a void* in each qtexture_t, must be casted to a IPluginTexdef*
// there's a void* in each face_t, must be casted to a IPluginTexdef*
// NOTE: IPluginTexdef stores a pointer to the qtexture_t or face_t it's stored in
// members of IPluginTexdef often access the qtexture_t or face_t they are connected to

// Write texdef needs a function pointer, because Radiant either writes into a FILE or a CMemFile
typedef void (WINAPI* PFN_QERAPP_MAPPRINTF) ( char *text, ... );

class IPluginTexdef
{
public:
	// Increment the number of references to this object
	virtual void IncRef () = 0;
	// Decrement the reference count
	virtual void DecRef () = 0;
	// Build a copy of this one, sets refCount to 1
	virtual IPluginTexdef* Copy() = 0;
	// refers to a face_t -----------------------
	// Parse texdef
	virtual void ParseTexdef () = 0;
	// Write texdef
	virtual void WriteTexdef (PFN_QERAPP_MAPPRINTF MapPrintf) = 0;
	// hook to a face
	virtual void Hook( face_t* ) = 0;
	// ------------------------------------------
	// refers to a qtexture_t -------------------
	// Init for a miptex_t
	//++timo NOTE: miptex_t is used only for .WAL format .. a bit outdated
	virtual void InitForMiptex( miptex_t* ) = 0;
	// set default texdef
	virtual void SetDefaultTexdef() = 0;
	// ------------------------------------------
	// refers to a patchMesh_t ------------------
	virtual void ParsePatchTexdef () = 0;
	// write texdef for a patch
	virtual void WritePatchTexdef (PFN_QERAPP_MAPPRINTF MapPrintf) = 0;
	// hook to a patch
	virtual void Hook( patchMesh_t* ) = 0;
	// ------------------------------------------
};

// use this macro
#define GETPLUGINTEXDEF(pFace) (static_cast<IPluginTexdef *>(pFace->pData))

// this one is used by Radiant to access the surface plugin

// {7DA59920-93D8-11d3-8EF3-0000E8E8657B}
static const GUID QERPlugSurfaceTable_GUID = 
{ 0x7da59920, 0x93d8, 0x11d3, { 0x8e, 0xf3, 0x0, 0x0, 0xe8, 0xe8, 0x65, 0x7b } };

typedef void (WINAPI* PFN_QERPLUG_DOSURFACE)			();
typedef bool (WINAPI* PFN_QERPLUG_BYEBYESURFACEDIALOG)	();
typedef void (WINAPI* PFN_QERPLUG_UPDATESURFACEDIALOG)	();
// allocates a new IPluginTexdef, linked to a face_t, refCount is set to 1
typedef IPluginTexdef* (WINAPI* PFN_QERPLUG_TEXDEFALLOC)	( face_t* );
// allocates a new IPluginTexdef, linked to a qtexture_t, refCount is set to 1
typedef IPluginTexdef* (WINAPI* PFN_QERPLUG_QTEXTUREALLOC)	( qtexture_t* );
// allocates a new IPluginTexdef, linked to a patchMesh_t, refCount is set to 1
typedef IPluginTexdef* (WINAPI* PFN_QERPLUG_PATCHALLOC)		( patchMesh_t* );

struct _QERPlugSurfaceTable
{
	int m_nSize;
	PFN_QERPLUG_BYEBYESURFACEDIALOG		m_pfnByeByeSurfaceDialog;
	PFN_QERPLUG_DOSURFACE				m_pfnDoSurface;
	PFN_QERPLUG_UPDATESURFACEDIALOG		m_pfnUpdateSurfaceDialog;
	PFN_QERPLUG_TEXDEFALLOC				m_pfnTexdefAlloc;
	PFN_QERPLUG_QTEXTUREALLOC			m_pfnQTextureAlloc;
	PFN_QERPLUG_PATCHALLOC				m_pfnPatchAlloc;
};

// this one is used by the plugin to access some Radiant stuff

// {42BAE4C0-9787-11d3-8EF3-0000E8E8657B}
static const GUID QERAppSurfaceTable_GUID = 
{ 0x42bae4c0, 0x9787, 0x11d3, { 0x8e, 0xf3, 0x0, 0x0, 0xe8, 0xe8, 0x65, 0x7b } };

typedef bool (WINAPI* PFN_PATCHESSELECTED)	();
// retrieve g_qeglobals.texturewin_t
//++timo FIXME: this should move in a dedicated table for all g_qeglobals stuff
typedef texturewin_t* (WINAPI* PFN_QEGLOBALSTEXTUREWIN) ();
// look for the first selected patch mesh
//++timo FIXME: this is a convenient func since there's no way to scan patches ( yet )
typedef patchMesh_t* (WINAPI* PFN_GETSELECTEDPATCH) ();
typedef void (WINAPI* PFN_PATCHREBUILD) (patchMesh_t *p);
//++timo FIXME: this one in particular is a hack
typedef void (WINAPI* PFN_GETTWOSELECTEDPATCH) (patchMesh_t **p1, patchMesh_t **p2);

struct _QERAppSurfaceTable
{
	int m_nSize;
	PFN_PATCHESSELECTED		m_pfnOnlyPatchesSelected;
	PFN_PATCHESSELECTED		m_pfnAnyPatchesSelected;
	PFN_QEGLOBALSTEXTUREWIN	m_pfnQeglobalsTexturewin;
	PFN_GETSELECTEDPATCH	m_pfnGetSelectedPatch;
	PFN_PATCHREBUILD		m_pfnPatchRebuild;
	PFN_GETTWOSELECTEDPATCH	m_pfnGetTwoSelectedPatch;
};

#endif