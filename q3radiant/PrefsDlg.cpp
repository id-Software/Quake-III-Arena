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
// PrefsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PrefsDlg.h"
#include "shlobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define PREF_SECTION      "Prefs"
#define INTERNAL_SECTION  "Internals"
#define MOUSE_KEY         "MouseButtons"
#define WINDOW_KEY        "QE4StyleWindows"
#define LAYOUT_KEY        "WindowLayout"
#define Q2_KEY            "Quake2Dir"
#define RUNQ2_KEY         "RunQuake2Run"
#define TLOCK_KEY         "TextureLock"
#define RLOCK_KEY         "RotateLock"
#define LOADLAST_KEY      "LoadLast"
#define LOADLASTMAP_KEY   "LoadLastMap"
#define LASTPROJ_KEY      "LastProject"
#define LASTMAP_KEY       "LastMap"
#define RUN_KEY           "RunBefore"
#define _3DFX_KEY         "Use3Dfx"
#define FACE_KEY          "NewFaceGrab"
#define BSP_KEY           "InternalBSP"
#define RCLICK_KEY        "NewRightClick"
#define VERTEX_KEY        "NewVertex"
#define AUTOSAVE_KEY      "Autosave"
#define AUTOSAVETIME_KEY  "AutosaveMinutes"
#define PAK_KEY           "UsePAK"
#define NEWAPPLY_KEY      "ApplyDismissesSurface"
#define HACK_KEY          "Gatewayescapehack"
#define TEXTURE_KEY       "NewTextureWindowStuff"
#define TINYBRUSH_KEY     "CleanTinyBrushes"
#define TINYSIZE_KEY      "CleanTinyBrusheSize"
#define SNAPSHOT_KEY      "Snapshots"
#define PAKFILE_KEY       "PAKFile"
#define STATUS_KEY        "StatusPointSize"
#define MOVESPEED_KEY     "MoveSpeed"
#define ANGLESPEED_KEY    "AngleSpeed"
#define SETGAME_KEY       "UseSetGame"
#define CAMXYUPDATE_KEY   "CamXYUpdate"
#define LIGHTDRAW_KEY     "NewLightStyle"
#define WHATGAME_KEY      "WhichGame"
#define CUBICCLIP_KEY     "CubicClipping"
#define CUBICSCALE_KEY    "CubicScale"
#define ALTEDGE_KEY       "ALTEdgeDrag"
#define TEXTUREBAR_KEY    "UseTextureBar"
#define FACECOLORS_KEY    "FaceColors"
#define QE4PAINT_KEY      "QE4Paint"
#define SNAPT_KEY         "SnapT"
#define XZVIS_KEY         "XZVIS"
#define YZVIS_KEY         "YZVIS"
#define ZVIS_KEY          "ZVIS"
#define SIZEPAINT_KEY     "SizePainting"
#define DLLENTITIES_KEY   "DLLEntities"
#define WIDETOOLBAR_KEY   "WideToolBar"
#define NOCLAMP_KEY       "NoClamp"
#define PREFAB_KEY        "PrefabPath"
#define USERINI_KEY       "UserINIPath"
#define ROTATION_KEY      "Rotation"
#define SGIOPENGL_KEY     "SGIOpenGL"
#define BUGGYICD_KEY      "BuggyICD"
#define HICOLOR_KEY       "HiColorTextures"
#define CHASEMOUSE_KEY    "ChaseMouse"
#define ENTITYSHOW_KEY    "EntityShow"
#define TEXTURESCALE_KEY  "TextureScale"
#define TEXTURESCROLLBAR_KEY  "TextureScrollbar"
#define DISPLAYLISTS_KEY  "UseDisplayLists"
#define NORMALIZECOLORS_KEY  "NormalizeColors"
#define SHADERS_KEY       "UseShaders"
#define SWITCHCLIP_KEY     "SwitchClipKey"
#define SELWHOLEENTS_KEY		"SelectWholeEntitiesKey"
#define TEXTURESUBSET_KEY		"UseTextureSubsetLoading"
#define TEXTUREQUALITY_KEY 	"TextureQuality"
#define SHOWSHADERS_KEY 	"ShowShaders"
#define SHADERTEST_KEY 	  "ShaderTest"
#define GLLIGHTING_KEY "UseGLLighting"
#define LOADSHADERS_KEY "LoadShaders"
#define NOSTIPPLE_KEY "NoStipple"
#define UNDOLEVELS_KEY "UndoLevels"

#define MOUSE_DEF 1
#define WINDOW_DEF 0
#define Q2_DEF "c:\\quake2\\quake2.exe"
#define Q3_DEF "c:\\Program Files\\Quake III Arena\\quake3.exe"
#define PAKFILE_DEF "c:\\quake2\\baseq2\\pak0.pak"
#define RUNQ2_DEF 0
#define TLOCK_DEF 1
#define LOADLAST_DEF 1
#define RUN_DEF 0

/////////////////////////////////////////////////////////////////////////////
// CPrefsDlg dialog


CPrefsDlg::CPrefsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPrefsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPrefsDlg)
	m_strQuake2 = _T("c:\\quake3\\");
	m_nMouse = 1;
	m_nView = 0;
	m_bLoadLast = FALSE;
	m_bFace = FALSE;
	m_bInternalBSP = FALSE;
	m_bRightClick = FALSE;
	m_bRunQuake = FALSE;
	m_bSetGame = FALSE;
	m_bVertex = FALSE;
	m_bAutoSave = TRUE;
  m_bNewApplyHandling = FALSE;
	m_strAutoSave = _T("5");
	m_bPAK = FALSE;
	m_bLoadLastMap = FALSE;
	m_bTextureWindow = FALSE;
	m_bSnapShots = FALSE;
  m_fTinySize = 0.5;
  m_bCleanTiny = FALSE;
	m_strPAKFile = _T("c:\\quake2\\baseq2\\pak0.pak");
	m_nStatusSize = 10;
	m_bCamXYUpdate = FALSE;
	m_bNewLightDraw = FALSE;
	m_strPrefabPath = _T("");
	m_nWhatGame = 0;
	m_strWhatGame = _T("Quake3");
	m_bALTEdge = FALSE;
	m_bTextureBar = FALSE;
	m_bFaceColors = FALSE;
	m_bQE4Painting = TRUE;
	m_bSnapTToGrid = FALSE;
  m_bXZVis = FALSE;
  m_bYZVis = FALSE;
  m_bZVis = FALSE;
	m_bSizePaint = FALSE;
	m_bDLLEntities = FALSE;
	m_bWideToolbar = TRUE;
	m_bNoClamp = FALSE;
	m_strUserPath = _T("");
	m_nRotation = 0;
	m_bSGIOpenGL = FALSE;
	m_bBuggyICD = FALSE;
	m_bHiColorTextures = TRUE;
	m_bChaseMouse = FALSE;
	m_bTextureScrollbar = TRUE;
	m_bDisplayLists = TRUE;
	m_bShowShaders = FALSE;
	m_nShader = -1;
	m_bNoStipple = FALSE;
	//}}AFX_DATA_INIT
  //LoadPrefs();
  m_bSelectCurves = TRUE;
  m_bSelectTerrain = TRUE;
  m_nEntityShowState = 0;
  m_nTextureScale = 2;
  m_bSwitchClip = FALSE;
  m_bSelectWholeEntities = TRUE;
  m_nTextureQuality = 3;
  m_bShowShaders = TRUE;
  m_bGLLighting = FALSE;
  m_nShader = 0;
  m_nUndoLevels = 64;
}




void CPrefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrefsDlg)
	DDX_Control(pDX, IDC_SPIN_UNDO, m_wndUndoSpin);
	DDX_Control(pDX, IDC_SPIN_POINTSIZE, m_wndFontSpin);
	DDX_Control(pDX, IDC_SLIDER_TEXTUREQUALITY, m_wndTexturequality);
	DDX_Control(pDX, IDC_COMBO_WHATGAME, m_wndWhatGame);
	DDX_Control(pDX, IDC_SLIDER_CAMSPEED, m_wndCamSpeed);
	DDX_Control(pDX, IDC_SPIN_AUTOSAVE, m_wndSpin);
	DDX_Text(pDX, IDC_EDIT_QUAKE2, m_strQuake2);
	DDX_Radio(pDX, IDC_RADIO_MOUSE, m_nMouse);
	DDX_Radio(pDX, IDC_RADIO_VIEWTYPE, m_nView);
	DDX_Check(pDX, IDC_CHECK_LOADLAST, m_bLoadLast);
	DDX_Check(pDX, IDC_CHECK_FACE, m_bFace);
	DDX_Check(pDX, IDC_CHECK_INTERNALBSP, m_bInternalBSP);
	DDX_Check(pDX, IDC_CHECK_RIGHTCLICK, m_bRightClick);
	DDX_Check(pDX, IDC_CHECK_RUNQUAKE, m_bRunQuake);
	DDX_Check(pDX, IDC_CHECK_SETGAME, m_bSetGame);
	DDX_Check(pDX, IDC_CHECK_VERTEX, m_bVertex);
	DDX_Check(pDX, IDC_CHECK_AUTOSAVE, m_bAutoSave);
	DDX_Text(pDX, IDC_EDIT_AUTOSAVE, m_strAutoSave);
	DDX_Check(pDX, IDC_CHECK_PAK, m_bPAK);
	DDX_Check(pDX, IDC_CHECK_LOADLASTMAP, m_bLoadLastMap);
	DDX_Check(pDX, IDC_CHECK_TEXTUREWINDOW, m_bTextureWindow);
	DDX_Check(pDX, IDC_CHECK_SNAPSHOTS, m_bSnapShots);
	DDX_Text(pDX, IDC_EDIT_PAKFILE, m_strPAKFile);
	DDX_Text(pDX, IDC_EDIT_STATUSPOINTSIZE, m_nStatusSize);
	DDV_MinMaxInt(pDX, m_nStatusSize, 2, 14);
	DDX_Check(pDX, IDC_CHECK_CAMXYUPDATE, m_bCamXYUpdate);
	DDX_Check(pDX, IDC_CHECK_LIGHTDRAW, m_bNewLightDraw);
	DDX_Text(pDX, IDC_EDIT_PREFABPATH, m_strPrefabPath);
	DDX_CBString(pDX, IDC_COMBO_WHATGAME, m_strWhatGame);
	DDX_Check(pDX, IDC_CHECK_ALTDRAG, m_bALTEdge);
	DDX_Check(pDX, IDC_CHECK_TEXTURETOOLBAR, m_bTextureBar);
	DDX_Check(pDX, IDC_CHECK_FACECOLOR, m_bFaceColors);
	DDX_Check(pDX, IDC_CHECK_QE4PAINTING, m_bQE4Painting);
	DDX_Check(pDX, IDC_CHECK_SNAPT, m_bSnapTToGrid);
	DDX_Check(pDX, IDC_CHECK_SIZEPAINT, m_bSizePaint);
	DDX_Check(pDX, IDC_CHECK_DLLENTITIES, m_bDLLEntities);
	DDX_Check(pDX, IDC_CHECK_WIDETOOLBAR, m_bWideToolbar);
	DDX_Check(pDX, IDC_CHECK_NOCLAMP, m_bNoClamp);
	DDX_Text(pDX, IDC_EDIT_USERPATH, m_strUserPath);
	DDX_Text(pDX, IDC_EDIT_ROTATION, m_nRotation);
	DDX_Check(pDX, IDC_CHECK_SGIOPENGL, m_bSGIOpenGL);
	DDX_Check(pDX, IDC_CHECK_BUGGYICD, m_bBuggyICD);
	DDX_Check(pDX, IDC_CHECK_HICOLOR, m_bHiColorTextures);
	DDX_Check(pDX, IDC_CHECK_MOUSECHASE, m_bChaseMouse);
	DDX_Check(pDX, IDC_CHECK_TEXTURESCROLLBAR, m_bTextureScrollbar);
	DDX_Check(pDX, IDC_CHECK_DISPLAYLISTS, m_bDisplayLists);
	DDX_CBIndex(pDX, IDC_COMBO_SHADERS, m_nShader);
	DDX_Check(pDX, IDC_CHECK_NOSTIPPLE, m_bNoStipple);
	DDX_Text(pDX, IDC_EDIT_UNDOLEVELS, m_nUndoLevels);
	DDV_MinMaxInt(pDX, m_nUndoLevels, 1, 64);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrefsDlg, CDialog)
	//{{AFX_MSG_MAP(CPrefsDlg)
	ON_BN_CLICKED(IDC_BTN_BROWSE, OnBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_BROWSEPAK, OnBtnBrowsepak)
	ON_BN_CLICKED(IDC_BTN_BROWSEPREFAB, OnBtnBrowseprefab)
	ON_BN_CLICKED(IDC_BTN_BROWSEUSERINI, OnBtnBrowseuserini)
	ON_CBN_SELCHANGE(IDC_COMBO_WHATGAME, OnSelchangeComboWhatgame)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrefsDlg message handlers

void CPrefsDlg::OnBtnBrowse() 
{
  UpdateData(TRUE);
  CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "Executables (*.exe)|*.exe||", this);
  if (dlg.DoModal() == IDOK)
  {
    m_strQuake2 = dlg.GetPathName();
    UpdateData(FALSE);
  }
}

BOOL CPrefsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
  m_wndSpin.SetRange(1,60);
  m_wndCamSpeed.SetRange(10, 800);
  m_wndCamSpeed.SetPos(m_nMoveSpeed);

  this->m_wndTexturequality.SetRange(0, 3);
  this->m_wndTexturequality.SetPos(m_nTextureQuality);

  m_wndFontSpin.SetRange(4,24);
  m_wndUndoSpin.SetRange(1,64);

  m_wndWhatGame.AddString("Quake2"); 
  m_wndWhatGame.AddString("Quake3"); 

  GetDlgItem(IDC_CHECK_HICOLOR)->EnableWindow(TRUE);
  GetDlgItem(IDC_CHECK_NOCLAMP)->EnableWindow(TRUE);

  //GetDlgItem(IDC_CHECK_NOCLAMP)->EnableWindow(FALSE);

  m_wndWhatGame.SelectString(-1,m_strWhatGame);
  if (strstr(m_strWhatGame, "Quake3") != NULL)
  {
    GetDlgItem(IDC_EDIT_PAKFILE)->EnableWindow(FALSE);
    GetDlgItem(IDC_BTN_BROWSEPAK)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_INTERNALBSP)->EnableWindow(FALSE);
  }
  else
  {
    GetDlgItem(IDC_EDIT_PAKFILE)->EnableWindow(TRUE);
    GetDlgItem(IDC_BTN_BROWSEPAK)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK_INTERNALBSP)->EnableWindow(TRUE);
  }

  return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPrefsDlg::OnOK() 
{
  m_nMoveSpeed = m_wndCamSpeed.GetPos();
  m_nAngleSpeed = (float)m_nMoveSpeed * 0.50;
  this->m_nTextureQuality = m_wndTexturequality.GetPos();
	SavePrefs();

  if (g_pParentWnd)
    g_pParentWnd->SetGridStatus();
  Sys_UpdateWindows(W_ALL);
  Undo_SetMaxSize(m_nUndoLevels);
	CDialog::OnOK();
}

void CPrefsDlg::LoadPrefs()
{
  CString strBuff;
  CString strPrefab = g_strAppPath;
  AddSlash(strPrefab);
  strPrefab += "Prefabs\\";
  
  m_nMouse = AfxGetApp()->GetProfileInt(PREF_SECTION, MOUSE_KEY, MOUSE_DEF);
  if (m_nMouse == 0)
    m_nMouseButtons = 2;
  else
    m_nMouseButtons = 3;

  m_nView = AfxGetApp()->GetProfileInt(PREF_SECTION, WINDOW_KEY, WINDOW_DEF);
  m_strQuake2 = AfxGetApp()->GetProfileString(PREF_SECTION, Q2_KEY, Q2_DEF);
  m_bRunQuake = AfxGetApp()->GetProfileInt(PREF_SECTION, RUNQ2_KEY, RUNQ2_DEF);
  m_bTextureLock = AfxGetApp()->GetProfileInt(PREF_SECTION, TLOCK_KEY, TLOCK_DEF);
  m_bRotateLock = AfxGetApp()->GetProfileInt(PREF_SECTION, RLOCK_KEY, TLOCK_DEF);
  m_strLastProject = AfxGetApp()->GetProfileString(PREF_SECTION, LASTPROJ_KEY, "");
  m_strLastMap = AfxGetApp()->GetProfileString(PREF_SECTION, LASTMAP_KEY, "");
  m_bLoadLast = AfxGetApp()->GetProfileInt(PREF_SECTION, LOADLAST_KEY, LOADLAST_DEF);
  m_bRunBefore = AfxGetApp()->GetProfileInt(INTERNAL_SECTION, RUN_KEY, RUN_DEF);
  //m_b3Dfx = AfxGetApp()->GetProfileInt(PREF_SECTION, _3DFX_KEY, 0);
  m_bFace = AfxGetApp()->GetProfileInt(PREF_SECTION, FACE_KEY, 1);
  m_bInternalBSP = AfxGetApp()->GetProfileInt(PREF_SECTION, BSP_KEY, 0);
  m_bRightClick = AfxGetApp()->GetProfileInt(PREF_SECTION, RCLICK_KEY, 1);
  m_bVertex = AfxGetApp()->GetProfileInt(PREF_SECTION, VERTEX_KEY, 1);
  m_bAutoSave = AfxGetApp()->GetProfileInt(PREF_SECTION, AUTOSAVE_KEY, 1);
  m_bPAK = AfxGetApp()->GetProfileInt(PREF_SECTION, PAK_KEY, 1);
  m_bNewApplyHandling = AfxGetApp()->GetProfileInt(PREF_SECTION, NEWAPPLY_KEY, 0);
  m_bLoadLastMap = AfxGetApp()->GetProfileInt(PREF_SECTION, LOADLASTMAP_KEY, 0);
  m_bGatewayHack = AfxGetApp()->GetProfileInt(PREF_SECTION, HACK_KEY, 0);
  m_bTextureWindow = AfxGetApp()->GetProfileInt(PREF_SECTION, TEXTURE_KEY, 0);
  m_bCleanTiny = AfxGetApp()->GetProfileInt(PREF_SECTION, TINYBRUSH_KEY, 0);
  strBuff = AfxGetApp()->GetProfileString(PREF_SECTION, TINYSIZE_KEY, "0.5");
  m_fTinySize = atof(strBuff);
  m_nAutoSave = AfxGetApp()->GetProfileInt(PREF_SECTION, AUTOSAVETIME_KEY, 5);
  m_strAutoSave.Format("%i", m_nAutoSave);
  m_bSnapShots = AfxGetApp()->GetProfileInt(PREF_SECTION, SNAPSHOT_KEY, 0);
  m_strPAKFile = AfxGetApp()->GetProfileString(PREF_SECTION, PAKFILE_KEY, PAKFILE_DEF);
  m_nStatusSize = AfxGetApp()->GetProfileInt(PREF_SECTION, STATUS_KEY, 10);
  m_nMoveSpeed = AfxGetApp()->GetProfileInt(PREF_SECTION, MOVESPEED_KEY, 400);
  m_nAngleSpeed = AfxGetApp()->GetProfileInt(PREF_SECTION, ANGLESPEED_KEY, 300);
  m_bSetGame = AfxGetApp()->GetProfileInt(PREF_SECTION, SETGAME_KEY, 0);
	m_bCamXYUpdate = AfxGetApp()->GetProfileInt(PREF_SECTION, CAMXYUPDATE_KEY, 1);
  m_bNewLightDraw = AfxGetApp()->GetProfileInt(PREF_SECTION, LIGHTDRAW_KEY, 1);
  m_bCubicClipping = AfxGetApp()->GetProfileInt(PREF_SECTION, CUBICCLIP_KEY, 1);
  m_nCubicScale = AfxGetApp()->GetProfileInt(PREF_SECTION, CUBICSCALE_KEY, 13);
  m_bALTEdge = AfxGetApp()->GetProfileInt(PREF_SECTION, ALTEDGE_KEY, 0);
  m_bTextureBar = AfxGetApp()->GetProfileInt(PREF_SECTION, TEXTUREBAR_KEY, 0);
  m_strWhatGame = AfxGetApp()->GetProfileString(PREF_SECTION, WHATGAME_KEY, "Quake3");
  m_bFaceColors = AfxGetApp()->GetProfileInt(PREF_SECTION, FACECOLORS_KEY, 0);
  m_bQE4Painting = AfxGetApp()->GetProfileInt(PREF_SECTION, QE4PAINT_KEY, 1);
  m_bSnapTToGrid = AfxGetApp()->GetProfileInt(PREF_SECTION, SNAPT_KEY, 0);
  m_bXZVis = AfxGetApp()->GetProfileInt(PREF_SECTION, XZVIS_KEY, 0);
  m_bYZVis = AfxGetApp()->GetProfileInt(PREF_SECTION, YZVIS_KEY, 0);
  m_bZVis = AfxGetApp()->GetProfileInt(PREF_SECTION, ZVIS_KEY, 1);
  m_bSizePaint = AfxGetApp()->GetProfileInt(PREF_SECTION, SIZEPAINT_KEY, 0);
  m_bDLLEntities = AfxGetApp()->GetProfileInt(PREF_SECTION, DLLENTITIES_KEY, 0);
  m_bWideToolbar = AfxGetApp()->GetProfileInt(PREF_SECTION, WIDETOOLBAR_KEY, 1);
  m_bNoClamp = AfxGetApp()->GetProfileInt(PREF_SECTION, NOCLAMP_KEY, 0);
  m_strPrefabPath = AfxGetApp()->GetProfileString(PREF_SECTION, PREFAB_KEY, strPrefab);
  m_strUserPath = AfxGetApp()->GetProfileString(PREF_SECTION, USERINI_KEY, "");
  m_nRotation = AfxGetApp()->GetProfileInt(PREF_SECTION, ROTATION_KEY, 45);
  m_bSGIOpenGL = AfxGetApp()->GetProfileInt(PREF_SECTION, SGIOPENGL_KEY, 0);
  m_bBuggyICD = AfxGetApp()->GetProfileInt(PREF_SECTION, BUGGYICD_KEY, 0);
  m_bHiColorTextures = AfxGetApp()->GetProfileInt(PREF_SECTION, HICOLOR_KEY, 1);
  m_bChaseMouse = AfxGetApp()->GetProfileInt(PREF_SECTION, CHASEMOUSE_KEY, 1);
  m_nEntityShowState = AfxGetApp()->GetProfileInt(PREF_SECTION, ENTITYSHOW_KEY, 0);
  m_nTextureScale = AfxGetApp()->GetProfileInt(PREF_SECTION, TEXTURESCALE_KEY, 50);
  m_bTextureScrollbar = AfxGetApp()->GetProfileInt(PREF_SECTION, TEXTURESCROLLBAR_KEY, TRUE);
  m_bDisplayLists = AfxGetApp()->GetProfileInt(PREF_SECTION, DISPLAYLISTS_KEY, TRUE);
  m_bSwitchClip = AfxGetApp()->GetProfileInt(PREF_SECTION, SWITCHCLIP_KEY, TRUE);
  m_bSelectWholeEntities = AfxGetApp()->GetProfileInt(PREF_SECTION, SELWHOLEENTS_KEY, TRUE);
  m_nTextureQuality = AfxGetApp()->GetProfileInt(PREF_SECTION, TEXTUREQUALITY_KEY, 6);
  m_bShowShaders = AfxGetApp()->GetProfileInt(PREF_SECTION, SHOWSHADERS_KEY, TRUE);
  m_bGLLighting = AfxGetApp()->GetProfileInt(PREF_SECTION, GLLIGHTING_KEY, FALSE);
  m_nShader = AfxGetApp()->GetProfileInt(PREF_SECTION, LOADSHADERS_KEY, 0);
  m_bNoStipple = AfxGetApp()->GetProfileInt(PREF_SECTION, NOSTIPPLE_KEY, 0);
  m_nUndoLevels = AfxGetApp()->GetProfileInt(PREF_SECTION, UNDOLEVELS_KEY, 0);
  
  if (m_bRunBefore == FALSE)
  {
    SetGamePrefs();
  }
}


void CPrefsDlg::SavePrefs()
{
  if (GetSafeHwnd())
    UpdateData(TRUE);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, MOUSE_KEY, m_nMouse);
  if (m_nMouse == 0)
    m_nMouseButtons = 2;
  else
    m_nMouseButtons = 3;
  AfxGetApp()->WriteProfileInt(PREF_SECTION, WINDOW_KEY, m_nView);
  AfxGetApp()->WriteProfileString(PREF_SECTION, Q2_KEY, m_strQuake2);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, RUNQ2_KEY, m_bRunQuake);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, TLOCK_KEY, m_bTextureLock);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, RLOCK_KEY, m_bRotateLock);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, LOADLAST_KEY, m_bLoadLast);
  AfxGetApp()->WriteProfileString(PREF_SECTION, LASTPROJ_KEY, m_strLastProject);
  AfxGetApp()->WriteProfileString(PREF_SECTION, LASTMAP_KEY, m_strLastMap);
  AfxGetApp()->WriteProfileInt(INTERNAL_SECTION, RUN_KEY, m_bRunBefore);
  //AfxGetApp()->WriteProfileInt(PREF_SECTION, _3DFX_KEY, m_b3Dfx);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, FACE_KEY, m_bFace);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, BSP_KEY, m_bInternalBSP);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, RCLICK_KEY, m_bRightClick);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, VERTEX_KEY, m_bVertex);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, AUTOSAVE_KEY, m_bAutoSave);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, PAK_KEY, m_bPAK);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, LOADLASTMAP_KEY, m_bLoadLastMap);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, TEXTURE_KEY, m_bTextureWindow);
  m_nAutoSave = atoi(m_strAutoSave);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, AUTOSAVETIME_KEY, m_nAutoSave);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SNAPSHOT_KEY, m_bSnapShots);
  AfxGetApp()->WriteProfileString(PREF_SECTION, PAKFILE_KEY, m_strPAKFile);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, STATUS_KEY, m_nStatusSize);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SETGAME_KEY, m_bSetGame);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, CAMXYUPDATE_KEY, m_bCamXYUpdate);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, LIGHTDRAW_KEY, m_bNewLightDraw);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, MOVESPEED_KEY, m_nMoveSpeed);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, ANGLESPEED_KEY, m_nAngleSpeed);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, CUBICCLIP_KEY, m_bCubicClipping);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, CUBICSCALE_KEY, m_nCubicScale);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, ALTEDGE_KEY, m_bALTEdge);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, TEXTUREBAR_KEY, m_bTextureBar);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, FACECOLORS_KEY, m_bFaceColors);
  AfxGetApp()->WriteProfileString(PREF_SECTION, WHATGAME_KEY, m_strWhatGame);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, QE4PAINT_KEY, m_bQE4Painting);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SNAPT_KEY, m_bSnapTToGrid);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, XZVIS_KEY, m_bXZVis);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, YZVIS_KEY, m_bYZVis);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, ZVIS_KEY, m_bZVis);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SIZEPAINT_KEY, m_bSizePaint);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, DLLENTITIES_KEY, m_bDLLEntities);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, WIDETOOLBAR_KEY, m_bWideToolbar);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, NOCLAMP_KEY, m_bNoClamp);
  AfxGetApp()->WriteProfileString(PREF_SECTION, PREFAB_KEY, m_strPrefabPath);
  AfxGetApp()->WriteProfileString(PREF_SECTION, USERINI_KEY, m_strUserPath);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, ROTATION_KEY, m_nRotation);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SGIOPENGL_KEY, m_bSGIOpenGL);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, BUGGYICD_KEY, m_bBuggyICD);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, HICOLOR_KEY, m_bHiColorTextures);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, CHASEMOUSE_KEY, m_bChaseMouse);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, ENTITYSHOW_KEY, m_nEntityShowState);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, TEXTURESCALE_KEY, m_nTextureScale);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, TEXTURESCROLLBAR_KEY, m_bTextureScrollbar);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, DISPLAYLISTS_KEY, m_bDisplayLists);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SWITCHCLIP_KEY, m_bSwitchClip);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SELWHOLEENTS_KEY, m_bSelectWholeEntities);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, TEXTUREQUALITY_KEY, m_nTextureQuality);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, SHOWSHADERS_KEY, m_bShowShaders);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, GLLIGHTING_KEY, m_bGLLighting);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, LOADSHADERS_KEY, m_nShader);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, NOSTIPPLE_KEY, m_bNoStipple);
  AfxGetApp()->WriteProfileInt(PREF_SECTION, UNDOLEVELS_KEY, m_nUndoLevels);
}

                        

void CPrefsDlg::OnBtnBrowsepak() 
{
  UpdateData(TRUE);
  CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "PAK files (*.pak)|*.pak||", this);
  if (dlg.DoModal() == IDOK)
  {
    m_strPAKFile = dlg.GetPathName();
    UpdateData(FALSE);
  }
}

void CPrefsDlg::OnBtnBrowseprefab() 
{
  UpdateData(TRUE);
  BROWSEINFO bi;
  CString strPath;
  char* p = strPath.GetBuffer(MAX_PATH+1);
  bi.hwndOwner = GetSafeHwnd();
  bi.pidlRoot = NULL;
  bi.pszDisplayName = p;
  bi.lpszTitle = "Load textures from path";
  bi.ulFlags = 0;
  bi.lpfn = NULL;
  bi.lParam = NULL;
  bi.iImage = 0;
  LPITEMIDLIST pidlBrowse;
  pidlBrowse = SHBrowseForFolder(&bi);
  if (pidlBrowse)
  {
    SHGetPathFromIDList(pidlBrowse, p);
    strPath.ReleaseBuffer();
    AddSlash(strPath);
    m_strPrefabPath = strPath;
    UpdateData(FALSE);
  }
}

void CPrefsDlg::OnBtnBrowseuserini() 
{
  UpdateData(TRUE);
  CFileDialog dlg(true, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, "INI files (*.ini)|*.ini||", this);
  if (dlg.DoModal() == IDOK)
  {
    m_strUserPath = dlg.GetPathName();
    UpdateData(FALSE);
  }
}

void CPrefsDlg::OnSelchangeComboWhatgame() 
{
  int n = m_wndWhatGame.GetCurSel();
  if (n >= 0)
  {
    m_wndWhatGame.GetLBText(n, m_strWhatGame);
  }
  SetGamePrefs();
}

void CPrefsDlg::SetGamePrefs()
{
  if (strstr(m_strWhatGame, "Quake3") != NULL)
  {
    m_bHiColorTextures = TRUE;
  	m_bWideToolbar = TRUE;
    m_strPAKFile = "PK3 files are loaded from the baseq3 path";
  	m_bInternalBSP = FALSE;
    if (GetSafeHwnd())
    {
      GetDlgItem(IDC_EDIT_PAKFILE)->EnableWindow(FALSE);
      GetDlgItem(IDC_BTN_BROWSEPAK)->EnableWindow(FALSE);
      GetDlgItem(IDC_CHECK_INTERNALBSP)->EnableWindow(FALSE);
    }
  }
  else
  {
    m_bHiColorTextures = FALSE;
  	m_bWideToolbar = FALSE;
    m_strPAKFile = PAKFILE_DEF;
    if (GetSafeHwnd())
    {
      GetDlgItem(IDC_EDIT_PAKFILE)->EnableWindow(TRUE);
      GetDlgItem(IDC_BTN_BROWSEPAK)->EnableWindow(TRUE);
      GetDlgItem(IDC_CHECK_INTERNALBSP)->EnableWindow(TRUE);
    }
  }
  SavePrefs();
}
