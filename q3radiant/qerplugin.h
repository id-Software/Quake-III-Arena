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
// QERadiant PlugIns
//
//

#ifndef __QERPLUGIN_H__
#define __QERPLUGIN_H__

#include <windows.h>
#include "qertypes.h"

#define QER_PLUG_VERSION_1 1.00
#define QER_PLUG_VERSION 1.70f

#define QER_MAX_NAMELEN 1024

// the editor will look for plugins in two places, the plugins path 
// under the application path, and the path under the basepath as defined
// in the project (.qe4) file.
//
// you can drop any number of new texture, model format DLL's in the standard plugin path
// but only one plugin that overrides map loading/saving, surface dialog, surface flags, etc.. 
// should be used at one time.. if multiples are loaded then the last one loaded will be the 
// active one
//
// type of services the plugin supplies, pass any combo of these flags
// it is assumed the plugin will have a matching function as defined below
// to correlate to the implied functionality
// 
// FIXME: after specing this crap i went to a simpler model so this may disappear
#define QER_PLUG_GAME_TEXTURE       0x0001      // defines new texture format
#define QER_PLUG_GAME_MODEL         0x0002      // defines new model format
#define QER_PLUG_GAME_MAP           0x0004      // handles map load/save
#define QER_PLUG_GAME_SURFACEDLG    0x0008      // handles surface dialog
#define QER_PLUG_GAME_SURFACEFLAGS  0x0010      // renames surface/content names

// basics
#define QERPLUG_INIT "QERPlug_Init"
#define QERPLUG_GETNAME "QERPlug_GetName"
#define QERPLUG_GETCOMMANDLIST "QERPlug_GetCommandList"
#define QERPLUG_DISPATCH "QERPlug_Dispatch"
#define QERPLUG_GETFUNCTABLE "QERPlug_GetFuncTable"

// FIXME: not used, probably should remove
#define QERPLUG_GETSERVICETPE "QERPlug_GetServiceType"

// game stuff
#define QERPLUG_GETTEXTUREINFO "QERPlug_GetTextureInfo"   // gets a texture info structure
#define QERPLUG_LOADTEXTURE    "QERPlug_LoadTexture"      // loads a texture, will return an RGBA structure
                                                          // and any surface flags/contents for it
#define QERPLUG_GETTMODELINFO "QERPlug_GetModelInfo"      // gets a model info structure
#define QERPLUG_LOADMODEL     "QERPlug_LoadModel"         // loads model data from a plugin
#define QERPLUG_DOSURFACEDLG  "QERPlug_DoSurfaceDlg"      // runs the surface dialog in a plugin
                                                          // this is going to get icky
#define QERPLUG_GETSURFACEFLAGS "QERPlug_GetSurfaceFlags" // gets a list of surface/content flag names from a plugin

struct _QERTextureInfo
{
  char m_TextureExtension[QER_MAX_NAMELEN];   // the extension these textures have
  qboolean m_bHiColor;    // if textures are NOT high color, the default 
                      // palette (as described inthe qe4 file will be used for gamma correction)
                      // if they are high color, gamma and shading are computed on the fly 
                      // based on the rgba data
  //--bool m_bIsShader;   // will probably do q3 shaders this way when i merge
  qboolean m_bWadStyle;   // if this is true, the plugin will be presented with the texture path
                      // defined in the .qe4 file and is expected to preload all the textures
  qboolean m_bHalfLife;   // causes brushes to be saved/parsed without the surface contents/flags/value
};

struct _QERTextureLoad    // returned by a plugin
{
  _QERTextureLoad()
  { 
    memset(reinterpret_cast<void*>(this), 0, sizeof(_QERTextureLoad));
  };

  ~_QERTextureLoad()
  {
    delete []m_pRGBA;
    delete []m_pName;
  };

  void makeSpace(int nSize)
  {
    m_pRGBA = new unsigned char[nSize+1];
  };

  void setName(const char* p)
  {
    m_pName = new char[strlen(p)+1];
    strcpy(m_pName, p);
  };


  unsigned char *m_pRGBA; // rgba data (alpha channel is supported and drawn appropriately)
  int m_nWidth;           // width
  int m_nHeight;          // height
  int m_nContents;        // default contents
  int m_nFlags;           // "" flags
  int m_nValue;           // "" value
  char *m_pName;          // name to be referenced in map, build tools, etc.
};

struct _QERModelInfo
{
  char m_ModelExtension[QER_MAX_NAMELEN];
  bool m_bSkinned;
  bool m_bMultipart;
};

struct _QERModelLoad
{
  // vertex and skin data
};


// hook functions
// FIXME: none of the hook stuff works for v1.00
#define  QERPLUG_MAPLOAD "QERPlug_MapLoad"
#define  QERPLUG_MAPSAVE "QERPlug_MapSave"

//=========================================
// plugin functions version 1.0
typedef LPCSTR (WINAPI* PFN_QERPLUG_INIT)(HMODULE hApp, HWND hwndMain);
typedef LPCSTR (WINAPI* PFN_QERPLUG_GETNAME)();
typedef LPCSTR (WINAPI* PFN_QERPLUG_GETCOMMANDLIST)();
typedef void (WINAPI* PFN_QERPLUG_DISPATCH)(LPCSTR p, vec3_t vMin, vec3_t vMax, BOOL bSingleBrush);
typedef LPVOID (WINAPI* PFN_QERPLUG_GETFUNCTABLE)();
typedef void (WINAPI* PFN_QERPLUG_MAPLOAD)();
typedef void (WINAPI* PFN_QERPLUG_MAPSAVE)();

// editor defined plugin dispatches
// these are used to signal the completion after a 'Get' function is called in the editor
#define QERPLUG_DISPATCH_POINTDONE "PointDone"
#define QERPLUG_DISPATCH_BRUSHDONE "BrushDone"

// v1.5
//
// Texture loading
// returns a ptr to _QERTextureInfo
typedef LPVOID (WINAPI* PFN_QERPLUG_GETTEXTUREINFO)();
//
// loads a texture by calling the texture load func in the editor (defined below)
// transparency (for water, fog, lava, etc.. ) can be emulated in the editor
// by passing in appropriate alpha data or by setting the appropriate surface flags
// expected by q2 (which the editor will use.. )
typedef void (WINAPI* PFN_QERPLUG_LOADTEXTURE)(LPCSTR pFilename); 

// v1.6
typedef LPVOID (WINAPI* PFN_QERPLUG_GETSURFACEFLAGS)();

// v1.7
// if exists in plugin, gets called between INIT and GETCOMMANDLIST
// the plugin can register the EClasses he wants to handle
//++timo TODO: this has got to move into the table, and be requested by QERPlug_RequestInterface
//++timo FIXME: the LPVOID parameter must be casted to an IEpair interface
#define QERPLUG_REGISTERPLUGINENTITIES "QERPlug_RegisterPluginEntities"
typedef void (WINAPI* PFN_QERPLUG_REGISTERPLUGINENTITIES)( LPVOID );
// if exists in plugin, gets called between INIT and GETCOMMANDLIST
// the plugin can Init all it needs for surface properties
#define QERPLUG_INITSURFACEPROPERTIES "QERPlug_InitSurfaceProperties"
typedef void (WINAPI* PFN_QERPLUG_INITSURFACEPROPERTIES)();
// if Radiant needs to use a particular set of commands, it can request the plugin to fill a func table
// this is similar to PFN_QERAPP_REQUESTINTERFACE
#define QERPLUG_REQUESTINTERFACE "QERPlug_RequestInterface"
typedef int (WINAPI* PFN_QERPLUG_REQUESTINTERFACE)( REFGUID, LPVOID );

//=========================================
// editor functions

// There are 3 potential brush handle lists
// 1. the list that contains brushes a plugin creates using CreateBrushHandle
// 2. the selected brush list (brushes the user has selected)
// 3. the active brush list (brushes in the map that are not selected)
// 
// In general, the same things can be done to brush handles (face manip, delete brushhandle, etc.. ) in each
// list. There are a few exceptions. 
// 1. You cannot commit a selected or active brush handle to the map. This is because it is already in the map. 
// 2. You cannot bind brush handles from the selected or active brush list to an entity. As of v1.0 of the plugins
// the only way for a plugin to create entities is to create a brush handles (or a list of handles) and then bind
// them to an entity. This will commit the brush(s) and/or the entities to the map as well.
// 
// To use the active or selected brush lists, you must first allocate them (which returns a count) and then
// release them when you are finish manipulating brushes in one of those lists. 

//++timo NOTE : the #defines here are never used, but can help finding where things are done in the editor

// brush manipulation routines
#define QERAPP_CREATEBRUSH "QERApp_CreateBrush"
#define QERAPP_CREATEBRUSHHANDLE "QERApp_CreateBrushHandle"
#define QERAPP_DELETEBRUSHHANDLE "QERApp_DeleteBrushHandle"
#define QERAPP_COMMITBRUSHHANDLETOMAP "QERApp_CommitBrushHandleToMap"
//++timo not implemented .. remove
// #define QERAPP_BINDHANDLESTOENTITY "QERApp_BindHandlesToEntity"
#define QERAPP_ADDFACE "QERApp_AddFace"
#define QERAPP_ADDFACEDATA "QERApp_AddFaceData"
#define QERAPP_GETFACECOUNT "QERApp_GetFaceCount"
#define QERAPP_GETFACEDATA "QERApp_GetFaceData"
#define QERAPP_SETFACEDATA "QERApp_SetFaceData"
#define QERAPP_DELETEFACE "QERApp_DeleteFace"
#define QERAPP_TEXTUREBRUSH "QERApp_TextureBrush"
#define QERAPP_BUILDBRUSH "QERApp_BuildBrush"					// PGM
#define QERAPP_SELECTEDBRUSHCOUNT "QERApp_SelectedBrushCount"
#define QERAPP_ALLOCATESELECTEDBRUSHHANDLES "QERApp_AllocateSelectedBrushHandles"
#define QERAPP_RELEASESELECTEDBRUSHHANDLES "QERApp_ReleaseSelectedBrushHandles"
#define QERAPP_GETSELECTEDBRUSHHANDLE "QERApp_GetSelectedBrushHandle"
#define QERAPP_ACTIVEBRUSHCOUNT "QERApp_ActiveBrushCount"
#define QERAPP_ALLOCATEACTIVEBRUSHHANDLES "QERApp_AllocateActiveBrushHandles"
#define QERAPP_RELEASEACTIVEBRUSHHANDLES "QERApp_ReleaseActiveBrushHandles"
#define QERAPP_GETACTIVEBRUSHHANDLE "QERApp_GetActiveBrushHandle"

// texture stuff
#define QERAPP_TEXTURECOUNT "QERApp_TextureCount"
#define QERAPP_GETTEXTURE "QERApp_GetTexture"
#define QERAPP_GETCURRENTTEXTURE "QERApp_GetCurrentTexture"
#define QERAPP_SETCURRENTTEXTURE "QERApp_SetCurrentTexture"

// selection 
#define QERAPP_DELETESELECTION "QERApp_DeleteSelection"
#define QERAPP_SELECTBRUSH "QERApp_SelectBrush"					// PGM
#define QERAPP_DESELECTBRUSH "QERApp_DeselectBrush"				// PGM
#define QERAPP_DESELECTALLBRUSHES "QERApp_DeselectAllBrushes"	// PGM

// data gathering
#define QERAPP_GETPOINTS "QERApp_GetPoints"
#define QERAPP_SELECTBRUSHES "QERApp_GetBrushes"

// entity class stuff
// the entity handling is very basic for 1.0
#define QERAPP_GETECLASSCOUNT "QERApp_GetEClassCount"
#define QERAPP_GETECLASS "QERApp_GetEClass"

// misc
#define QERAPP_SYSMSG "QERApp_SysMsg"
#define QERAPP_INFOMSG "QERApp_InfoMsg"
#define QERAPP_HIDEINFOMSG "QERApp_HideInfoMsg"
#define QERAPP_POSITIONVIEW	"QERApp_PositionView"			// PGM
#define QERAPP_RESET_PLUGINS "QERApp_ResetPlugins"

// texture loading
#define QERAPP_LOADTEXTURERGBA "QERApp_LoadTextureRGBA"

// FIXME: the following are not implemented yet
// hook registrations
#define QERAPP_REGISTER_MAPLOADFUNC "QERApp_Register_MapLoadFunc"
#define QERAPP_REGISTER_MAPSAVEFUNC "QERApp_Register_MapSaveFunc"

// FIXME: the following are not implemented yet
#define QERAPP_REGISTER_PROJECTLOADFUNC "QERApp_Register_ProjectLoadFunc"
#define QERAPP_REGISTER_MOUSEHANDLER "QERApp_Register_MouseHandler"
#define QERAPP_REGISTER_KEYHANDLER "QERApp_Register_KeyHandler"

// FIXME: new primtives do not work in v1.00
// primitives are new types of things in the map
// for instance, the Q3 curves could have been done as 
// primitives instead of being built in 
// it will be a plugins responsibility to hook the map load and save funcs to load
// and/or save any additional data (like new primitives of some type)
// the editor will call each registered renderer during the rendering process to repaint
// any primitives the plugin owns
// each primitive object has a temporary sibling brush that lives in the map
// FIXME: go backwards on this a bit.. orient it more towards the temp brush mode as it will be cleaner
// basically a plugin will hook the map load and save and will add the primitives to the map.. this will
// produce a temporary 'primitive' brush and the appropriate renderer will be called as well as the 
// edit handler (for edge drags, sizes, rotates, etc.. ) and the vertex maker will be called when vertex
// mode is attemped on the brush.. there will need to be a GetPrimitiveBounds callback in the edit handler
// so the brush can resize appropriately as needed.. this might be the plugins responsibility to set the 
// sibling brushes size.. it will then be the plugins responsibility to hook map save to save the primitives
// as the editor will discard any temp primitive brushes.. (there probably needs to be some kind of sanity check
// here as far as keeping the brushes and the plugin in sync.. i suppose the edit handler can deal with all of that
// crap but it looks like a nice place for a mess)
#define QERAPP_REGISTER_PRIMITIVE "QERApp_Register_Primitive"
#define QERAPP_REGISTER_RENDERER "QERApp_Register_Renderer"
#define QERAPP_REGISTER_EDITHANDLER "QERApp_Register_EditHandler"
#define QERAPP_REGISTER_VERTEXMAKER "QERApp_Register_VertexMaker"
#define QERAPP_ADDPRIMITIVE "QERApp_AddPrimitive"

// v1.70
#define QERAPP_GETENTITYCOUNT "QERApp_GetEntityCount"
#define QERAPP_GETENTITYHANDLE "QERApp_GetEntityHandle"
//++timo not implemented for the moment
// #define QERAPP_GETENTITYINFO "QERApp_GetEntityInfo"
//++timo does the keyval need some more funcs to add/remove ?
// get the pointer and do the changes yourself
#define QERAPP_GETENTITYKEYVALLIST "QERApp_GetKeyValList"
#define QERAPP_ALLOCATEEPAIR "QERApp_AllocateEpair"
// will check current KeyVal list is NULL, otherwise use GetKeyValList
#define QERAPP_SETENTITYKEYVALLIST "QERApp_SetKeyValList"
#define QERAPP_ALLOCATEENTITYBRUSHHANDLES "QERApp_AllocateEntityBrushHandles"
#define QERAPP_RELEASEENTITYBRUSHHANDLES "QERApp_ReleaseEntityBrushHandles"
#define QERAPP_GETENTITYBRUSHHANDLE "QERApp_GetEntityBrushHandle"
#define QERAPP_CREATEENTITYHANDLE "QERApp_CreateEntityHandle"
#define QERAPP_COMMITBRUSHHANDLETOENTITY "QERApp_CommitBrushHandleToEntity"
#define QERAPP_COMMITENTITYHANDLETOMAP "QERApp_CommitEntityHandleToMap"
#define QERAPP_SETSCREENUPDATE "QERApp_SetScreenUpdate"
#define QERAPP_BUILDBRUSH2 "QERApp_BuildBrush2"

struct _QERPointData
{
  int     m_nCount;
  vec3_t *m_pVectors;
};

struct _QERFaceData
{
  char  m_TextureName[QER_MAX_NAMELEN];
  int   m_nContents;
  int   m_nFlags;
  int   m_nValue;
  float m_fShift[2];
  float m_fRotate;
  float m_fScale[2];
  vec3_t m_v1, m_v2, m_v3;
  // brush primitive additions
  qboolean m_bBPrimit;
  brushprimit_texdef_t brushprimit_texdef;
};

typedef void (WINAPI* PFN_QERAPP_CREATEBRUSH)(vec3_t vMin, vec3_t vMax);

typedef LPVOID (WINAPI* PFN_QERAPP_CREATEBRUSHHANDLE)();
typedef void (WINAPI* PFN_QERAPP_DELETEBRUSHHANDLE)(LPVOID pv);
typedef void (WINAPI* PFN_QERAPP_COMMITBRUSHHANDLETOMAP)(LPVOID pv);
typedef void (WINAPI* PFN_QERAPP_ADDFACE)(LPVOID pv, vec3_t v1, vec3_t v2, vec3_t v3);

typedef void (WINAPI* PFN_QERAPP_ADDFACEDATA)(LPVOID pv, _QERFaceData *pData);
typedef int  (WINAPI* PFN_QERAPP_GETFACECOUNT)(LPVOID pv);
typedef _QERFaceData* (WINAPI* PFN_QERAPP_GETFACEDATA)(LPVOID pv, int nFaceIndex);
typedef void (WINAPI* PFN_QERAPP_SETFACEDATA)(LPVOID pv, int nFaceIndex, _QERFaceData *pData);
typedef void (WINAPI* PFN_QERAPP_DELETEFACE)(LPVOID pv, int nFaceIndex);
typedef void (WINAPI* PFN_QERAPP_TEXTUREBRUSH)(LPVOID pv, LPCSTR pName);
typedef void (WINAPI* PFN_QERAPP_BUILDBRUSH)(LPVOID pv);		// PGM
typedef void (WINAPI* PFN_QERAPP_SELECTBRUSH)(LPVOID pv);		// PGM
typedef void (WINAPI* PFN_QERAPP_DESELECTBRUSH)(LPVOID pv);		// PGM
typedef void (WINAPI* PFN_QERAPP_DESELECTALLBRUSHES)();			// PGM

typedef void (WINAPI* PFN_QERAPP_DELETESELECTION)();
typedef void (WINAPI* PFN_QERAPP_GETPOINTS)(int nMax, _QERPointData *pData, LPCSTR pMsg);
typedef void (WINAPI* PFN_QERAPP_SYSMSG)(LPCSTR pMsg);
typedef void (WINAPI* PFN_QERAPP_INFOMSG)(LPCSTR pMsg);
typedef void (WINAPI* PFN_QERAPP_HIDEINFOMSG)();
typedef void (WINAPI* PFN_QERAPP_POSITIONVIEW)(vec3_t v1, vec3_t v2);	//PGM

typedef int  (WINAPI* PFN_QERAPP_SELECTEDBRUSHCOUNT)();
typedef int (WINAPI* PFN_QERAPP_ALLOCATESELECTEDBRUSHHANDLES)();
typedef void (WINAPI* PFN_QERAPP_RELEASESELECTEDBRUSHHANDLES)();
typedef LPVOID (WINAPI* PFN_QERAPP_GETSELECTEDBRUSHHANDLE)(int nIndex);

typedef int  (WINAPI* PFN_QERAPP_ACTIVEBRUSHCOUNT)();
typedef int (WINAPI* PFN_QERAPP_ALLOCATEACTIVEBRUSHHANDLES)();
typedef void (WINAPI* PFN_QERAPP_RELEASEACTIVEBRUSHHANDLES)();
typedef LPVOID (WINAPI* PFN_QERAPP_GETACTIVEBRUSHHANDLE)(int nIndex);

typedef int  (WINAPI* PFN_QERAPP_TEXTURECOUNT)();
typedef LPCSTR (WINAPI* PFN_QERAPP_GETTEXTURE)(int nIndex);
typedef LPCSTR (WINAPI* PFN_QERAPP_GETCURRENTTEXTURE)();
typedef void (WINAPI* PFN_QERAPP_SETCURRENTTEXTURE)(LPCSTR pName);

typedef void (WINAPI* PFN_QERAPP_REGISTERMAPLOAD)(LPVOID vp);
typedef void (WINAPI* PFN_QERAPP_REGISTERMAPSAVE)(LPVOID vp);

typedef int (WINAPI* PFN_QERAPP_GETECLASSCOUNT)();
typedef LPCSTR (WINAPI* PFN_QERAPP_GETECLASS)(int nIndex);

typedef void (WINAPI* PFN_QERAPP_RESETPLUGINS)();
//--typedef int (WINAPI* PFN_QERAPP_GETENTITYCOUNT)();


// called by texture loaders for each texture loaded
typedef void (WINAPI* PFN_QERAPP_LOADTEXTURERGBA)(LPVOID vp);

//--typedef LPCSTR (WINAPI* PFN_QERAPP_GETENTITY)(int nIndex);

// v1.70
typedef int (WINAPI* PFN_QERAPP_GETENTITYCOUNT)();
typedef LPVOID (WINAPI* PFN_QERAPP_GETENTITYHANDLE)(int nIndex);
typedef epair_t** (WINAPI* PFN_QERAPP_GETENTITYKEYVALLIST)(LPVOID vp);
typedef epair_t* (WINAPI* PFN_QERAPP_ALLOCATEEPAIR)( char*, char* );
typedef void (WINAPI* PFN_QERAPP_SETENTITYKEYVALLIST)(LPVOID vp, epair_t* ep);
typedef int (WINAPI* PFN_QERAPP_ALLOCATEENTITYBRUSHHANDLES)(LPVOID vp);
typedef void (WINAPI* PFN_QERAPP_RELEASEENTITYBRUSHHANDLES)();
typedef LPVOID (WINAPI* PFN_QERAPP_GETENTITYBRUSHHANDLE)(int nIndex);
typedef LPVOID (WINAPI* PFN_QERAPP_CREATEENTITYHANDLE)();
typedef void (WINAPI* PFN_QERAPP_COMMITBRUSHHANDLETOENTITY)( LPVOID vpBrush, LPVOID vpEntity);
typedef void (WINAPI* PFN_QERAPP_COMMITENTITYHANDLETOMAP)(LPVOID vp);
typedef void (WINAPI* PFN_QERAPP_SETSCREENUPDATE)(int bScreenUpdate);
// this one uses window flags defined in qertypes.h
typedef void (WINAPI* PFN_QERAPP_SYSUPDATEWINDOWS)(int bits);
//++timo remove this one
typedef void (WINAPI* PFN_QERAPP_BUILDBRUSH2)(LPVOID vp, int bConvert);
// Radiant functions for Plugin Entities
// will look for the value of the correponding key in the project epairs
typedef char* (WINAPI* PFN_QERAPP_READPROJECTKEY)(char* );
// will scan the file, parse the first EClass found, and associate it to the caller plugin
// will only read first EClass in the file, and return true/false
typedef int (WINAPI* PFN_QERAPP_SCANFILEFORECLASS)(char* );
// plugins can request additional interfaces from the editor when they need specific services
typedef int (WINAPI* PFN_QERAPP_REQUESTINTERFACE)( REFGUID, LPVOID );
// use this one for errors, Radiant will stop after the "edit preferences" dialog
typedef void (WINAPI* PFN_QERAPP_ERRORMSG)(LPCSTR pMsg);
// use to gain read access to the project epairs
// FIXME: removed, accessed through QERPlug_RegisterPluginEntities with the IEpair interface
// typedef void (WINAPI* PFN_QERAPP_GETPROJECTEPAIR)(epair_t **);
// used to allocate and read a buffer
//++timo NOTE: perhaps this would need moving to some kind of dedicated interface
typedef int (WINAPI* PFN_QERAPP_LOADFILE)(const char *pLocation, void ** buffer);
typedef char* (WINAPI* PFN_QERAPP_EXPANDRELETIVEPATH)(char *);
typedef void (WINAPI* PFN_QERAPP_QECONVERTDOSTOUNIXNAME)( char *dst, const char *src );
typedef int (WINAPI* PFN_QERAPP_HASSHADER)(const char *);
typedef int (WINAPI* PFN_QERAPP_TEXTURELOADSKIN)(char *pName, int *pnWidth, int *pnHeight);
// free memory previously allocated by Radiant
// NOTE: this is dirty
typedef void (WINAPI* PFN_QERAPP_RADIANTFREE)( void * );
// retrieves the path to the engine from the preferences dialog box
typedef char* (WINAPI* PFN_QERAPP_GETGAMEPATH)();
// retrieves full Radiant path
typedef char* (WINAPI* PFN_QERAPP_GETQERPATH)();

// patches in/out
// NOTE: this is a bit different from the brushes in/out, no LPVOID handles this time
// use int indexes instead
// if you call AllocateActivePatchHandles, you'll be playing with active patches
// AllocateSelectedPatcheHandles for selected stuff
// a call to CreatePatchHandle will move you to a seperate index table
typedef int				(WINAPI* PFN_QERAPP_ALLOCATEACTIVEPATCHHANDLES)		();
typedef int				(WINAPI* PFN_QERAPP_ALLOCATESELECTEDPATCHHANDLES)	();
typedef void			(WINAPI* PFN_QERAPP_RELEASEPATCHHANDLES)			();
typedef patchMesh_t*	(WINAPI* PFN_QERAPP_GETPATCHDATA)					(int);
typedef void			(WINAPI* PFN_QERAPP_DELETEPATCH)					(int);
typedef int				(WINAPI* PFN_QERAPP_CREATEPATCHHANDLE)				();
// when commiting, only a few patchMesh_t members are relevant:
//  int	width, height;		// in control points, not patches
//  int   contents, flags, value, type;
//  drawVert_t ctrl[MAX_PATCH_WIDTH][MAX_PATCH_HEIGHT];
// once you have commited the index is still available, if the patch handle was allocated by you
//   then you can re-use the index to commit other patches .. otherwise you can change existing patches
// NOTE: the handle thing for plugin-allocated patches is a bit silly (nobody's perfect)
// TODO: change current behaviour to an index = 0 to tell Radiant to allocate, other indexes to existing patches
// patch is selected after a commit
// you can add an optional texture / shader name .. if NULL will use the current texture
typedef void			(WINAPI* PFN_QERAPP_COMMITPATCHHANDLETOMAP)			(int, patchMesh_t* pMesh, char *texName);

// FIXME:
// add map format extensions
// add texture format handlers
// add surface dialog handler
// add model handler/displayer

// v1 func table
// Plugins need to declare one of these and implement the getfunctable as described above
struct _QERFuncTable_1
{
  float m_fVersion;
  int   m_nSize;
  PFN_QERAPP_CREATEBRUSH            m_pfnCreateBrush;
  PFN_QERAPP_CREATEBRUSHHANDLE      m_pfnCreateBrushHandle;
  PFN_QERAPP_DELETEBRUSHHANDLE      m_pfnDeleteBrushHandle;
  PFN_QERAPP_COMMITBRUSHHANDLETOMAP m_pfnCommitBrushHandle;
  PFN_QERAPP_ADDFACE                m_pfnAddFace;
  PFN_QERAPP_ADDFACEDATA            m_pfnAddFaceData;
  PFN_QERAPP_GETFACEDATA            m_pfnGetFaceData;
  PFN_QERAPP_GETFACECOUNT           m_pfnGetFaceCount;
  PFN_QERAPP_SETFACEDATA            m_pfnSetFaceData;
  PFN_QERAPP_DELETEFACE             m_pfnDeleteFace;
  PFN_QERAPP_TEXTUREBRUSH           m_pfnTextureBrush;
  PFN_QERAPP_BUILDBRUSH				m_pfnBuildBrush;				// PGM
  PFN_QERAPP_SELECTBRUSH			m_pfnSelectBrush;				// PGM
  PFN_QERAPP_DESELECTBRUSH			m_pfnDeselectBrush;				// PGM
  PFN_QERAPP_DESELECTALLBRUSHES		m_pfnDeselectAllBrushes;		// PGM

  PFN_QERAPP_DELETESELECTION        m_pfnDeleteSelection;
  PFN_QERAPP_GETPOINTS              m_pfnGetPoints;
  PFN_QERAPP_SYSMSG                 m_pfnSysMsg;
  PFN_QERAPP_INFOMSG                m_pfnInfoMsg;
  PFN_QERAPP_HIDEINFOMSG            m_pfnHideInfoMsg;
  PFN_QERAPP_POSITIONVIEW			m_pfnPositionView;				// PGM

  PFN_QERAPP_SELECTEDBRUSHCOUNT           m_pfnSelectedBrushCount;
  PFN_QERAPP_ALLOCATESELECTEDBRUSHHANDLES m_pfnAllocateSelectedBrushHandles;
  PFN_QERAPP_RELEASESELECTEDBRUSHHANDLES  m_pfnReleaseSelectedBrushHandles;
  PFN_QERAPP_GETSELECTEDBRUSHHANDLE       m_pfnGetSelectedBrushHandle;

  PFN_QERAPP_ACTIVEBRUSHCOUNT             m_pfnActiveBrushCount;
  PFN_QERAPP_ALLOCATEACTIVEBRUSHHANDLES   m_pfnAllocateActiveBrushHandles;
  PFN_QERAPP_RELEASEACTIVEBRUSHHANDLES    m_pfnReleaseActiveBrushHandles;
  PFN_QERAPP_GETACTIVEBRUSHHANDLE         m_pfnGetActiveBrushHandle;

  //++timo this would need to be removed and replaced by the IShaders interface
  PFN_QERAPP_TEXTURECOUNT                 m_pfnTextureCount;
  PFN_QERAPP_GETTEXTURE                   m_pfnGetTexture;
  PFN_QERAPP_GETCURRENTTEXTURE            m_pfnGetCurrentTexture;
  PFN_QERAPP_SETCURRENTTEXTURE            m_pfnSetCurrentTexture;

  PFN_QERAPP_GETECLASSCOUNT         m_pfnGetEClassCount;
  PFN_QERAPP_GETECLASS              m_pfnGetEClass;
  PFN_QERAPP_RESETPLUGINS           m_pfnResetPlugins;
  // v1.00 ends here
  // v1.50 starts here
  PFN_QERAPP_LOADTEXTURERGBA        m_pfnLoadTextureRGBA;
  // v1.50 ends here
  // v1.70 starts here
  PFN_QERAPP_GETENTITYCOUNT			m_pfnGetEntityCount;
  PFN_QERAPP_GETENTITYHANDLE		m_pfnGetEntityHandle;
  PFN_QERAPP_GETENTITYKEYVALLIST	m_pfnGetEntityKeyValList;
  PFN_QERAPP_SETENTITYKEYVALLIST	m_pfnSetEntityKeyValList;
  PFN_QERAPP_ALLOCATEENTITYBRUSHHANDLES	m_pfnAllocateEntityBrushHandles;
  PFN_QERAPP_RELEASEENTITYBRUSHHANDLES	m_pfnReleaseEntityBrushHandles;
  PFN_QERAPP_GETENTITYBRUSHHANDLE	m_pfnGetEntityBrushHandle;
  PFN_QERAPP_CREATEENTITYHANDLE		m_pfnCreateEntityHandle;
  PFN_QERAPP_COMMITBRUSHHANDLETOENTITY	m_pfnCommitBrushHandleToEntity;
  PFN_QERAPP_COMMITENTITYHANDLETOMAP	m_pfnCommitEntityHandleToMap;
  PFN_QERAPP_ALLOCATEEPAIR			m_pfnAllocateEpair;
  PFN_QERAPP_SETSCREENUPDATE		m_pfnSetScreenUpdate;
  PFN_QERAPP_SYSUPDATEWINDOWS		m_pfnSysUpdateWindows;
  PFN_QERAPP_BUILDBRUSH2			m_pfnBuildBrush2;
  // Radiant functions for Plugin Entities
  PFN_QERAPP_READPROJECTKEY			m_pfnReadProjectKey;
  PFN_QERAPP_SCANFILEFORECLASS		m_pfnScanFileForEClass;
  // plugins can request additional interfaces
  PFN_QERAPP_REQUESTINTERFACE		m_pfnRequestInterface;
  PFN_QERAPP_ERRORMSG				m_pfnErrorMsg;
  // loading a file into a buffer
  PFN_QERAPP_LOADFILE				m_pfnLoadFile;
  PFN_QERAPP_EXPANDRELETIVEPATH		m_pfnExpandReletivePath;
  PFN_QERAPP_QECONVERTDOSTOUNIXNAME	m_pfnQE_ConvertDOSToUnixName;
  PFN_QERAPP_HASSHADER				m_pfnHasShader;
  PFN_QERAPP_TEXTURELOADSKIN		m_pfnTexture_LoadSkin;
  PFN_QERAPP_RADIANTFREE			m_pfnRadiantFree;
  PFN_QERAPP_GETGAMEPATH			m_pfnGetGamePath;
  PFN_QERAPP_GETQERPATH				m_pfnGetQERPath;
  // patches in / out
  PFN_QERAPP_ALLOCATEACTIVEPATCHHANDLES		m_pfnAllocateActivePatchHandles;
  PFN_QERAPP_ALLOCATESELECTEDPATCHHANDLES	m_pfnAllocateSelectedPatchHandles;
  PFN_QERAPP_RELEASEPATCHHANDLES			m_pfnReleasePatchHandles;
  PFN_QERAPP_GETPATCHDATA					m_pfnGetPatchData;
  PFN_QERAPP_DELETEPATCH					m_pfnDeletePatch;
  PFN_QERAPP_CREATEPATCHHANDLE				m_pfnCreatePatchHandle;
  PFN_QERAPP_COMMITPATCHHANDLETOMAP			m_pfnCommitPatchHandleToMap;
};


//--typedef int (WINAPI*      PFN_QERAPP_BRUSHCOUNT     )();
//--typedef brush_t* (WINAPI* PFN_QERAPP_GETBRUSH       )();
//--typedef void (WINAPI*     PFN_QERAPP_DELETEBRUSH    )();

#endif