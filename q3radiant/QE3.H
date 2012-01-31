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
#ifndef __QE3_H__
#define __QE3_H__

// disable data conversion warnings for gl
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA
 
#include <windows.h>

extern "C"
{
#include "qgl.h"
}

#include <math.h>
#include <stdlib.h>
#include <malloc.h>

// this define to use HTREEITEM and MFC stuff in the headers
#define QERTYPES_USE_MFC
#include "qertypes.h"
#include "../libs/cmdlib.h"
#include "mathlib.h"
#include "parse.h"
#include "lbmlib.h"

#include <commctrl.h>
#include "afxres.h"
#include "resource.h"

#include "qedefs.h"

#include "qfiles.h"

#include "textures.h"
#include "brush.h"
#include "entity.h"
#include "map.h"
#include "select.h"
#include "splines/splines.h"

#include "camera.h"
#include "xy.h"
#include "z.h"
#include "mru.h"

#include "undo.h"

#include "terrain.h"

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


// set these before calling CheckParm
extern int myargc;
extern char **myargv;

double I_FloatTime (void);

void	Error (char *error, ...);
void	Warning (char *error, ...);
int		CheckParm (char *check);
void ParseCommandLine (char *lpCmdLine);


int 	ParseNum (char *str);


char *COM_Parse (char *data);

extern	char		com_token[1024];
extern	qboolean	com_eof;

#define	MAX_NUM_ARGVS	32
extern	int		argc;
extern	char	*argv[MAX_NUM_ARGVS];


typedef struct
{
	int		p1, p2;
	face_t	*f1, *f2;
} pedge_t;

typedef struct
{
	int		  iSize;
	int		  iTexMenu;		// nearest, linear, etc
	float	  fGamma;			// gamma for textures
	char	  szProject[256];	// last project loaded
	vec3_t	colors[COLOR_LAST];
	qboolean  show_names;
  qboolean  show_coordinates;
	int       exclude;
  int     m_nTextureTweak;
} SavedInfo_t;

//
// system functions
//
// TTimo NOTE: WINAPI funcs can be accessed by plugins
void    Sys_UpdateStatusBar( void );
void    WINAPI Sys_UpdateWindows (int bits);
void    Sys_Beep (void);
void    Sys_ClearPrintf (void);
void    Sys_Printf (char *text, ...);
double	Sys_DoubleTime (void);
void    Sys_GetCursorPos (int *x, int *y);
void    Sys_SetCursorPos (int x, int y);
void    Sys_SetTitle (char *text);
void    Sys_BeginWait (void);
void    Sys_EndWait (void);
void    Sys_Status(const char *psz, int part);

/*
** most of the QE globals are stored in this structure
*/
typedef struct
{
	qboolean d_showgrid;
	int      d_gridsize;
	
	int      d_num_entities;
	
	entity_t *d_project_entity;
	
	float     d_new_brush_bottom_z,
		      d_new_brush_top_z;
	
	HINSTANCE d_hInstance;
	
	HGLRC     d_hglrcBase;
	HDC       d_hdcBase;
	
	HWND      d_hwndMain;
	HWND      d_hwndCamera;
	HWND      d_hwndEdit;
	HWND      d_hwndEntity;
	HWND      d_hwndTexture;
	HWND      d_hwndXY;
	HWND      d_hwndZ;
	HWND      d_hwndStatus;
	HWND      d_hwndGroup;
	HWND      d_hwndMedia;
	
	vec3_t    d_points[MAX_POINTS];
	int       d_numpoints;
	pedge_t   d_edges[MAX_EDGES];
	int       d_numedges;

	// terrain variables
	float				d_terrainBrushSize;
	terrainnoise_t		d_terrainNoiseType;
	terrainfalloff_t	d_terrainFalloff;
	terrainbrush_t		d_terrainBrush;
	int					d_terrainWidth;
	int					d_terrainHeight;

	terrainVert_t	*d_terrapoints[MAX_TERRA_POINTS];
	int				d_numterrapoints;

	
	int       d_num_move_points;
	float    *d_move_points[4096];
	
	qtexture_t	*d_qtextures;
	
	texturewin_t d_texturewin;
	
	int	         d_pointfile_display_list;
	
	xy_t         d_xyOld;
	
	LPMRUMENU    d_lpMruMenu;
	
	SavedInfo_t  d_savedinfo;
	
	int          d_workcount;
	
	// connect entities uses the last two brushes selected
	int			 d_select_count;
	brush_t		*d_select_order[2];
	vec3_t       d_select_translate;    // for dragging w/o making new display lists
	select_t     d_select_mode;
	idPointListInterface *selectObject;	// 
	
	int		     d_font_list;
	
	int          d_parsed_brushes;
	
	qboolean	show_blocks;
	
	// Timo
	// tells if we are internally using brush primitive (texture coordinates and map format)
	// this is a shortcut for IntForKey( g_qeglobals.d_project_entity, "brush_primit" )
	// NOTE: must keep the two ones in sync
	BOOL m_bBrushPrimitMode;

	// used while importing brush data from file or memory buffer
	// tells if conversion between map format and internal preferences ( m_bBrushPrimitMode ) is needed
	qboolean	bNeedConvert;
	qboolean	bOldBrushes;
	qboolean	bPrimitBrushes;
	
	vec3_t  d_vAreaTL;
	vec3_t  d_vAreaBR;
	
	// tells if we are using .INI files for prefs instead of registry
	qboolean	use_ini;
	// even in .INI mode we use the registry for all void* prefs
	char		use_ini_registry[64];
	
	//Timo
	// tells we have surface properties plugin
	qboolean	bSurfacePropertiesPlugin;
	// tells we are using a BSP frontend plugin
	qboolean	bBSPFrontendPlugin;

	qboolean dontDrawSelectedOutlines;
	
} QEGlobals_t;

//void *qmalloc (size_t size);
char *copystring (char *s);
char *ExpandReletivePath (char *p);

void Pointfile_Delete (void);
void WINAPI Pointfile_Check (void);
void Pointfile_Next (void);
void Pointfile_Prev (void);
void Pointfile_Clear (void);
void Pointfile_Draw( void );
void Pointfile_Load( void );

//
// drag.c
//
void Drag_Begin (int x, int y, int buttons,
		   vec3_t xaxis, vec3_t yaxis,
		   vec3_t origin, vec3_t dir);
void Drag_MouseMoved (int x, int y, int buttons);
void Drag_MouseUp (int nButtons = 0);

//
// csg.c
//
void CSG_MakeHollow (void);
void CSG_Subtract (void);
void CSG_Merge (void);

//
// vertsel.c
//

void SetupVertexSelection (void);
void SelectEdgeByRay (vec3_t org, vec3_t dir);
void SelectVertexByRay (vec3_t org, vec3_t dir);

void ConnectEntities (void);

extern	int	update_bits;

extern	int	screen_width;
extern	int	screen_height;

extern	HANDLE	bsp_process;
extern HANDLE g_hBSPOutput;
extern HANDLE g_hBSPInput;


char	*TranslateString (char *buf);

void ProjectDialog (void);

void FillTextureMenu (CStringArray* pArray = NULL);
void FillBSPMenu (void);

BOOL CALLBACK Win_Dialog (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
);


//
// win_cam.c
//
void WCam_Create (HINSTANCE hInstance);


//
// win_xy.c
//
void WXY_Create (HINSTANCE hInstance);

//
// win_z.c
//
void WZ_Create (HINSTANCE hInstance);

//
// win_ent.c
//


//
// win_main.c
//
void Main_Create (HINSTANCE hInstance);
extern BOOL SaveWindowState(HWND hWnd, const char *pszName);
extern BOOL LoadWindowState(HWND hWnd, const char *pszName);

extern BOOL SaveRegistryInfo(const char *pszName, void *pvBuf, long lSize);
extern BOOL LoadRegistryInfo(const char *pszName, void *pvBuf, long *plSize);

//
// entityw.c
//
BOOL CreateEntityWindow(HINSTANCE hInstance);
void FillClassList (void);
BOOL UpdateEntitySel(eclass_t *pec);	
void SetInspectorMode(int iType);
int DrawTexControls(HWND hWnd);
void SetSpawnFlags(void);
void GetSpawnFlags(void);
void SetKeyValuePairs(bool bClearMD3 = false);
extern void BuildGammaTable(float g);
BOOL GetSelectAllCriteria(CString &strKey, CString &strVal);


// win_dlg.c

void DoGamma(void);
void DoFind(void);
void DoRotate(void);
void DoSides(bool bCone = false, bool bSphere = false, bool bTorus = false);
void DoAbout(void);
void DoSurface();

/*
** QE function declarations
*/
void     QE_CheckAutoSave( void );
void     WINAPI QE_ConvertDOSToUnixName( char *dst, const char *src );
void     QE_CountBrushesAndUpdateStatusBar( void );
void     WINAPI QE_CheckOpenGLForErrors(void);
void     QE_ExpandBspString (char *bspaction, char *out, char *mapname, bool useTemps);
void     QE_Init (void);
qboolean QE_KeyDown (int key, int nFlags = 0);
qboolean QE_LoadProject (char *projectfile);
qboolean QE_SingleBrush (bool bQuiet = false);


// sys stuff
void Sys_MarkMapModified (void);

/*
** QE Win32 function declarations
*/
int  WINAPI QEW_SetupPixelFormat(HDC hDC, qboolean zbuffer );
void QEW_StopGL( HWND hWnd, HGLRC hGLRC, HDC hDC );

/*
** extern declarations
*/
extern QEGlobals_t   g_qeglobals;

//++timo clean (moved into qertypes.h)
//enum VIEWTYPE {YZ, XZ, XY};
qboolean IsBrushSelected(brush_t* bSel);

// curve brushes

void Curve_MakeCurvedBrush (qboolean negative, qboolean top, qboolean bottom, 
					qboolean s1, qboolean s2, qboolean s3, qboolean s4);

void Curve_Invert (void);

void Curve_AddFakePlanes( brush_t *B );
void Curve_StripFakePlanes( brush_t *B );
void Curve_BuildPoints (brush_t *b);
void Curve_XYDraw (brush_t *b);
void Curve_CameraDraw (brush_t *b);

void Curve_WriteFile (char *name);


// patch stuff

extern bool g_bSameView;
extern int  g_nPatchClickedView;
bool within(vec3_t vTest, vec3_t vTL, vec3_t vBR);

void Brush_RebuildBrush(brush_t *b, vec3_t vMins, vec3_t vMaxs );
patchMesh_t* MakeNewPatch();
brush_t* AddBrushForPatch(patchMesh_t *pm, bool bLinkToWorld = true);
brush_t* Patch_GenericMesh(int nWidth, int nHeight, int nOrientation = 2, bool bDeleteSource = true, bool bOverride = false);
void Patch_ReadFile (char *name);
void Patch_WriteFile (char *name); 
void Patch_BuildPoints (brush_t *b);
void Patch_Move(patchMesh_t *p, const vec3_t vMove, bool bRebuild = false);
//++timo had to add a default value for bSnap (see Patch_ApplyMatrix call from Select_ApplyMatrix in select.cpp)
void Patch_ApplyMatrix(patchMesh_t *p, const vec3_t vOrigin, const vec3_t vMatrix[3], bool bSnap = false);
void Patch_EditPatch();
void Patch_Deselect();
void Patch_Deselect(patchMesh_t *p);
void Patch_Delete(patchMesh_t *p);
int  Patch_MemorySize(patchMesh_t *p);
void Patch_Select(patchMesh_t *p);
void Patch_Scale(patchMesh_t *p, const vec3_t vOrigin, const vec3_t vAmt, bool bRebuilt = true);
void Patch_Cleanup();
void Patch_SetView(int n);
void Patch_SetTexture(patchMesh_t *p, texdef_t *tex_def, IPluginTexdef* pPlugTexdef = NULL);
void Patch_BrushToMesh(bool bCone = false, bool bBevel = false, bool bEndcap = false, bool bSquare = false, int nHeight = 3);
bool Patch_DragScale(patchMesh_t *p, vec3_t vAmt, vec3_t vMove);
void Patch_ReadBuffer(char* pBuff, bool bSelect = false);
void Patch_WriteFile (CMemFile* pMemFile);
void Patch_UpdateSelected(vec3_t vMove);
void Patch_AddRow(patchMesh_t *p);
brush_t* Patch_Parse(bool bOld);
void Patch_Write (patchMesh_t *p, FILE *f);
void Patch_Write (patchMesh_t *p, CMemFile *file);
void Patch_AdjustColumns(patchMesh_t *p, int nCols);
void Patch_AdjustRows(patchMesh_t *p, int nRows);
void Patch_AdjustSelected(bool bInsert, bool bColumn, bool bFlag);
patchMesh_t* Patch_Duplicate(patchMesh_t *pFrom);
void Patch_RotateTexture(patchMesh_t *p, float fAngle);
void Patch_ScaleTexture(patchMesh_t *p, float fx, float fy, bool bFixup = true);
void Patch_ShiftTexture(patchMesh_t *p, float fx, float fy);
void Patch_DrawCam(patchMesh_t *p);
void Patch_DrawXY(patchMesh_t *p);
void Patch_InsertColumn(patchMesh_t *p, bool bAdd);
void Patch_InsertRow(patchMesh_t *p, bool bAdd);
void Patch_RemoveRow(patchMesh_t *p, bool bFirst);
void Patch_RemoveColumn(patchMesh_t *p, bool bFirst);
void Patch_ToggleInverted();
void Patch_Restore(patchMesh_t *p);
void Patch_Save(patchMesh_t *p);
void Patch_SetTextureInfo(texdef_t* pt);
void Patch_NaturalTexturing();
void Patch_ResetTexturing(float fx, float fy);
void Patch_FitTexturing();
void Patch_BendToggle();
void Patch_StartInsDel();
void Patch_BendHandleTAB();
void Patch_BendHandleENTER();
void Patch_SelectBendNormal();
void Patch_SelectBendAxis();
bool WINAPI OnlyPatchesSelected();
bool WINAPI AnyPatchesSelected();
patchMesh_t* SinglePatchSelected();
void Patch_CapCurrent(bool bInvertedBevel = false, bool bInvertedEndcap = false);
void Patch_DisperseRows();
void Patch_DisperseColumns();
void Patch_NaturalizeSelected(bool bCap = false, bool bCycleCap = false);
void Patch_SelectAreaPoints();
void Patch_InvertTexture(bool bY);
void Patch_InsDelToggle();
void Patch_InsDelHandleTAB();
void Patch_InsDelHandleENTER();
void Patch_SetOverlays();
void Patch_ClearOverlays();
void Patch_Thicken(int nAmount, bool bSeam);
void Patch_Transpose();
void Patch_Freeze();
void Patch_UnFreeze(bool bAll);
const char* Patch_GetTextureName();
void Patch_FindReplaceTexture(brush_t *pb, const char *pFind, const char *pReplace, bool bForce);
void Patch_ReplaceQTexture(brush_t *pb, qtexture_t *pOld, qtexture_t *pNew);
void Select_SnapToGrid();
extern bool g_bPatchShowBounds;
extern bool g_bPatchWireFrame;
extern bool g_bPatchWeld;
extern bool g_bPatchDrillDown;
extern bool g_bPatchInsertMode;
extern bool g_bPatchBendMode;
extern vec3_t g_vBendOrigin;
void Patch_FromTriangle(vec5_t vx, vec5_t vy, vec5_t vz);
const char* Patch_GetKeyValue(patchMesh_t *p, const char *pKey);
void Patch_SetEpair(patchMesh_t *p, const char *pKey, const char *pValue);



// group stuff
// group_t are loaded / saved through "group_info" entities
// they hold epairs for group settings and additionnal access info (tree nodes)
typedef struct group_s
{
	struct group_s	*next;
	epair_t *epairs;
	HTREEITEM itemOwner;
} group_t;

// NOTES: grouping only enabled in brush primitives mode
// grouping works by naming brushes and setting display properties
// the group hierarchy is not related with the map hierarchy (entity list, brushes etc.)
// brushes with no group are under the "world" node (default for all brushes)
// void Group_GetListFromWorld(CStringArray *pArray);
void Group_RemoveListFromWorld();
// void Group_SetListToWorld(CStringArray *pArray);
// void Group_BuildTree(CTreeCtrl *pTree);
// void Group_DecomposeTree(CTreeCtrl *pTree);
// save group_t as "classname" "group_info" things
void Group_Save(FILE *f);
// clean the brushes ownerItem, clean the treeview and rebuild everything
// is usually called when loading a new map, but may be called anytime
void Group_Init();
void Group_Add(entity_t *e);

// remove a brush from it's current group, will erase the "group" epair if any, and delete the tree control node
void Group_RemoveBrush(brush_t *b);
void Group_AddToWorld(brush_t *b);
// will remove brush of it's current group if any, and will add it wherever needed according to it's "group" key
void Group_AddToProperGroup(brush_t *b);
void Group_AddToSelected(brush_t *b);
// allocate a new group, set name
group_t* Group_Alloc(const char *name);
// we use entities to store information about the groups
// these entities are not linked into the world, and they have no brushes
// only loaded / saved in map file
group_t* Group_ForName(const char *name);

// Timo
// new brush primitive stuff
void ComputeAxisBase(vec3_t normal,vec3_t texS,vec3_t texT );
void FaceToBrushPrimitFace(face_t *f);
void EmitBrushPrimitTextureCoordinates(face_t *, winding_t *);
// EmitTextureCoordinates, is old code used for brush to brush primitive conversion
void EmitTextureCoordinates ( float *xyzst, qtexture_t *q, face_t *f);
void BrushPrimit_Parse(brush_t *);
// compute a fake shift scale rot representation from the texture matrix
void TexMatToFakeTexCoords( vec_t texMat[2][3], float shift[2], float *rot, float scale[2] );
void FakeTexCoordsToTexMat( float shift[2], float rot, float scale[2], vec_t texMat[2][3] );
void ConvertTexMatWithQTexture( brushprimit_texdef_t *texMat1, qtexture_t *qtex1, brushprimit_texdef_t *texMat2, qtexture_t *qtex2 );
// texture locking
void Face_MoveTexture_BrushPrimit(face_t *f, vec3_t delta);
void Select_ShiftTexture_BrushPrimit( face_t *f, int x, int y );
void RotateFaceTexture_BrushPrimit(face_t *f, int nAxis, float fDeg, vec3_t vOrigin );
// used in CCamWnd::ShiftTexture_BrushPrimit
void ComputeBest2DVector( vec3_t v, vec3_t X, vec3_t Y, int &x, int &y );


//
// eclass.cpp
//
extern qboolean parsing_single;
extern qboolean eclass_found;
extern eclass_t *eclass_e;
void Eclass_ScanFile( char *filename );

// for interfaces, we require main plugin header included
#include "qerplugin.h"

//
// SurfaceDlg.cpp and surface properties plugin
//
//++timo some patch in/out stuff is in there, needs to be moved out in a dedicated interface
void WINAPI Patch_Rebuild(patchMesh_t *p);
#include "isurfaceplugin.h"
extern _QERPlugSurfaceTable g_SurfaceTable;

//
// OpenGL interface
//
#include "igl.h"
HGLRC WINAPI QERApp_GetQeglobalsHGLRC();
void WINAPI QERApp_HookXYGLWindow(IGLWindow* pGLW);
void WINAPI QERApp_UnHookGLWindow(IGLWindow* pGLW);
void DrawPluginEntities( VIEWTYPE vt );

//
// ISelectedFace interface
//
#include "ISelectedFace.h"
int WINAPI QERApp_ISelectedFace_GetTextureNumber();
int WINAPI QERApp_GetFaceInfo(_QERFaceData *pFaceData, winding_t *pWinding);
int WINAPI QERApp_SetFaceInfo(_QERFaceData *pFaceData);
void WINAPI QERApp_GetTextureSize( int Size[2] );

//
// IEpairs interface
//
#include "IEpairs.h"
#include "EpairsWrapper.h"

//
// IPluginEntities interface
//
#include "IPluginEntities.h"
int WINAPI QERApp_EClassScanDir( char *dir, HMODULE plugID );

//
// ShaderInfo.cpp
//
#include "ShaderInfo.h"

//
// TexWnd.cpp
//
CShaderInfo* hasShader(const char *pName);

//
// IScripLib interface
// GetToken, UnGetToken, etc.
#include "IScriplib.h"
extern FILE *g_File;
void WINAPI QERApp_MapPrintf_FILE( char *text, ... );

//
// ISurfacePlugin interface
//
void WINAPI QERApp_GetTwoSelectedPatch( patchMesh_t **p1, patchMesh_t **p2 );

//
// IBSPFrontend interface
//
#include "IBSPFrontend.h"
extern _QERPlugBSPFrontendTable g_BSPFrontendTable;
extern CStringArray g_BSPFrontendCommands;

//
// IMessaging interface
#include "IMessaging.h"
#include "Messaging.h"
void WINAPI QERApp_HookWindow(IWindowListener* pListen);
void WINAPI QERApp_UnHookWindow(IWindowListener* pListen);
IXYWndWrapper* WINAPI QERApp_GetXYWndWrapper();
void WINAPI QERApp_HookListener(IListener* pListen, int Msg);
int  WINAPI QERApp_UnHookListener(IListener* pListen);
void DispatchRadiantMsg( int Msg );
// dispatch for IWindowListener entities
void DispatchOnMouseMove(UINT nFlags, int x, int y);
bool DispatchOnLButtonDown(UINT nFlags, int x, int y);
bool DispatchOnLButtonUp(UINT nFlags, int x, int y);

//
// IShaders interface
CShaderInfo* SetNameShaderInfo(qtexture_t* q, const char* pPath, const char* pName);
qtexture_t* Texture_LoadTGATexture (unsigned char* pPixels, int nWidth, int nHeight, char* pPath, int nFlags, int nContents, int nValue );
#include "IShaders.h"
qtexture_t* WINAPI QERApp_TryTextureForName(const char* name);



#endif
