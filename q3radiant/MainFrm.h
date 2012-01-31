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
// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__330BBF0A_731C_11D1_B539_00AA00A410FC__INCLUDED_)
#define AFX_MAINFRM_H__330BBF0A_731C_11D1_B539_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "LstToolBar.h"
#include "XYWnd.h"
#include "TexWnd.h"
#include "ZWnd.h"
#include "CamWnd.h"
#include "RADEditWnd.h"
#include "TextureBar.h"
#include "PlugInManager.h"
#include "PlugIn.h"
#include "groupdlg.h"


const int RAD_SHIFT =   0x01;
const int RAD_ALT =     0x02;
const int RAD_CONTROL = 0x04;
const int RAD_PRESS   = 0x08;

struct SCommandInfo
{
  char* m_strCommand;
  unsigned int   m_nKey;
  unsigned int   m_nModifiers;
  unsigned int m_nCommand;
};

struct SKeyInfo
{
  char* m_strName;
  unsigned int m_nVKKey;
};




class CMainFrame : public CFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();
  void HandleKey(UINT nChar, UINT nRepCnt, UINT nFlags, bool bDown = true) 
  {
    if (bDown)
      OnKeyDown(nChar, nRepCnt, nFlags);
    else
      OnKeyUp(nChar, nRepCnt, nFlags);
  };

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
  void UpdatePatchToolbarButtons();
  void NudgeSelection(int nDirection, int nAmount);
	void UpdateTextureBar();
  void SetButtonMenuStates();
	void SetTexValStatus();
	void SetGridStatus();
	void RoutineProcessing();
	CXYWnd* ActiveXY();
	void UpdateWindows(int nBits);
	void SetStatusText(int nPane, const char* pText);
	void UpdateStatusText();
	void SetWindowStyle(int nStyle);
	virtual ~CMainFrame();
  CXYWnd* GetXYWnd() {return m_pXYWnd;};
  CXYWnd* GetXZWnd() {return m_pXZWnd;};
  CXYWnd* GetYZWnd() {return m_pYZWnd;};
  CCamWnd* GetCamera() {return m_pCamWnd;};
  CTexWnd* GetTexWnd() {return m_pTexWnd;};
  void SetActiveXY(CXYWnd* p) 
  {
    if (m_pActiveXY)
      m_pActiveXY->SetActive(false);

    m_pActiveXY = p;

    if (m_pActiveXY)
      m_pActiveXY->SetActive(true);

  };
  int CurrentStyle() { return m_nCurrentStyle; };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CLstToolBar m_wndToolBar;
	CLstToolBar m_wndScaleBar;
	CDialogBar m_wndHelpBar;
	CTextureBar m_wndTextureBar;
  CSplitterWnd m_wndSplit;
  CSplitterWnd m_wndSplit2;
  CSplitterWnd m_wndSplit3;
  CXYWnd* m_pXYWnd;
  CXYWnd* m_pYZWnd;
  CXYWnd* m_pXZWnd;
  CCamWnd* m_pCamWnd;
  CTexWnd* m_pTexWnd;
  CZWnd* m_pZWnd;
  CRADEditWnd* m_pEditWnd;
  int m_nCurrentStyle;
  CString m_strStatus[15];
  CXYWnd* m_pActiveXY;
  bool m_bCamPreview;
  CPlugInManager m_PlugInMgr;
  int m_nNextPlugInID;

// Generated message map functions
protected:
	bool m_bDoLoop;
	bool m_bSplittersOK;
	void CreateQEChildren();
  	void LoadCommandMap();
	void ShowMenuItemKeyBindings(CMenu *pMenu);
  	void SetEntityCheck();
	afx_msg LRESULT OnBSPStatus(UINT wParam, long lParam);
	afx_msg LRESULT OnBSPDone(UINT wParam, long lParam);
public:
	void Nudge(int nDim, float fNudge);

  	CPlugInManager &GetPlugInMgr() {return m_PlugInMgr;};
  	void AddPlugInMenuItem(CPlugIn* pPlugIn);
	void CleanPlugInMenu();

  // these are public so i can easily reflect messages
  // from child windows..
	//{{AFX_MSG(CMainFrame)
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void ToggleCamera();
	afx_msg void OnFileClose();
	afx_msg void OnFileExit();
	afx_msg void OnFileLoadproject();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();
	afx_msg void OnFilePointfile();
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveas();
	afx_msg void OnView100();
	afx_msg void OnViewCenter();
	afx_msg void OnViewConsole();
	afx_msg void OnViewDownfloor();
	afx_msg void OnViewEntity();
	afx_msg void OnViewFront();
	afx_msg void OnViewShowblocks();
	afx_msg void OnViewShowclip();
	afx_msg void OnViewShowcoordinates();
	afx_msg void OnViewShowdetail();
	afx_msg void OnViewShowent();
	afx_msg void OnViewShowlights();
	afx_msg void OnViewShownames();
	afx_msg void OnViewShowpath();
	afx_msg void OnViewShowwater();
	afx_msg void OnViewShowworld();
	afx_msg void OnViewTexture();
	afx_msg void OnViewUpfloor();
	afx_msg void OnViewXy();
	afx_msg void OnViewZ100();
	afx_msg void OnViewZoomin();
	afx_msg void OnViewZoomout();
	afx_msg void OnViewZzoomin();
	afx_msg void OnViewZzoomout();
	afx_msg void OnViewSide();
	afx_msg void OnTexturesShowinuse();
	afx_msg void OnTexturesInspector();
	afx_msg void OnMiscBenchmark();
	afx_msg void OnMiscFindbrush();
	afx_msg void OnMiscGamma();
	afx_msg void OnMiscNextleakspot();
	afx_msg void OnMiscPreviousleakspot();
	afx_msg void OnMiscPrintxy();
	afx_msg void OnMiscSelectentitycolor();
	afx_msg void OnTexturebk();
	afx_msg void OnColorsMajor();
	afx_msg void OnColorsMinor();
	afx_msg void OnColorsXybk();
	afx_msg void OnBrush3sided();
	afx_msg void OnBrush4sided();
	afx_msg void OnBrush5sided();
	afx_msg void OnBrush6sided();
	afx_msg void OnBrush7sided();
	afx_msg void OnBrush8sided();
	afx_msg void OnBrush9sided();
	afx_msg void OnBrushArbitrarysided();
	afx_msg void OnBrushFlipx();
	afx_msg void OnBrushFlipy();
	afx_msg void OnBrushFlipz();
	afx_msg void OnBrushRotatex();
	afx_msg void OnBrushRotatey();
	afx_msg void OnBrushRotatez();
	afx_msg void OnRegionOff();
	afx_msg void OnRegionSetbrush();
	afx_msg void OnRegionSetselection();
	afx_msg void OnRegionSettallbrush();
	afx_msg void OnRegionSetxy();
	afx_msg void OnSelectionArbitraryrotation();
	afx_msg void OnSelectionClone();
	afx_msg void OnSelectionConnect();
	afx_msg void OnSelectionCsgsubtract();
	afx_msg void OnSelectionCsgmerge();
	afx_msg void OnSelectionNoOutline();
	afx_msg void OnSelectionDelete();
	afx_msg void OnSelectionDeselect();
	afx_msg void OnSelectionDragedges();
	afx_msg void OnSelectionDragvertecies();
	afx_msg void OnRaiseLowerTerrain();
	afx_msg void OnSelectionMakeDetail();
	afx_msg void OnSelectionMakeStructural();
	afx_msg void OnSelectionMakehollow();
	afx_msg void OnSelectionSelectcompletetall();
	afx_msg void OnSelectionSelectinside();
	afx_msg void OnSelectionSelectpartialtall();
	afx_msg void OnSelectionSelecttouching();
	afx_msg void OnSelectionUngroupentity();
	afx_msg void OnTexturesPopup();
	afx_msg void OnSplinesPopup();
	afx_msg void OnPopupSelection();
	afx_msg void OnViewChange();
	afx_msg void OnViewCameraupdate();
	afx_msg void OnUpdateViewCameraupdate(CCmdUI* pCmdUI);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnHelpAbout();
	afx_msg void OnViewClipper();
	afx_msg void OnCameraAngledown();
	afx_msg void OnCameraAngleup();
	afx_msg void OnCameraBack();
	afx_msg void OnCameraDown();
	afx_msg void OnCameraForward();
	afx_msg void OnCameraLeft();
	afx_msg void OnCameraRight();
	afx_msg void OnCameraStrafeleft();
	afx_msg void OnCameraStraferight();
	afx_msg void OnCameraUp();
	afx_msg void OnGridToggle();
	afx_msg void OnPrefs();
	afx_msg void OnTogglecamera();
	afx_msg void OnToggleconsole();
	afx_msg void OnToggleview();
	afx_msg void OnTogglez();
	afx_msg void OnToggleLock();
	afx_msg void OnEditMapinfo();
	afx_msg void OnEditEntityinfo();
	afx_msg void OnBrushScripts();
	afx_msg void OnViewNextview();
	afx_msg void OnHelpCommandlist();
	afx_msg void OnFileNewproject();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnFlipClip();
	afx_msg void OnClipSelected();
	afx_msg void OnSplitSelected();
	afx_msg void OnToggleviewXz();
	afx_msg void OnToggleviewYz();
	afx_msg void OnColorsBrush();
	afx_msg void OnColorsClipper();
	afx_msg void OnColorsGridtext();
	afx_msg void OnColorsSelectedbrush();
	afx_msg void OnColorsGridblock();
	afx_msg void OnColorsViewname();
	afx_msg void OnColorSetoriginal();
	afx_msg void OnColorSetqer();
	afx_msg void OnColorSetblack();
	afx_msg void OnSnaptogrid();
	afx_msg void OnSelectScale();
	afx_msg void OnSelectMouserotate();
	afx_msg void OnEditCopybrush();
	afx_msg void OnEditPastebrush();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnSelectionInvert();
	afx_msg void OnSelectionTextureDec();
	afx_msg void OnSelectionTextureFit();
	afx_msg void OnSelectionTextureInc();
	afx_msg void OnSelectionTextureRotateclock();
	afx_msg void OnSelectionTextureRotatecounter();
	afx_msg void OnSelectionTextureScaledown();
	afx_msg void OnSelectionTextureScaleup();
	afx_msg void OnSelectionTextureShiftdown();
	afx_msg void OnSelectionTextureShiftleft();
	afx_msg void OnSelectionTextureShiftright();
	afx_msg void OnSelectionTextureShiftup();
	afx_msg void OnGridNext();
	afx_msg void OnGridPrev();
	afx_msg void OnSelectionTextureScaleLeft();
	afx_msg void OnSelectionTextureScaleRight();
	afx_msg void OnTextureReplaceall();
	afx_msg void OnScalelockx();
	afx_msg void OnScalelocky();
	afx_msg void OnScalelockz();
	afx_msg void OnSelectMousescale();
	afx_msg void OnViewCubicclipping();
	afx_msg void OnFileImport();
	afx_msg void OnFileProjectsettings();
	afx_msg void OnUpdateFileImport(CCmdUI* pCmdUI);
	afx_msg void OnViewCubein();
	afx_msg void OnViewCubeout();
	afx_msg void OnFileSaveregion();
	afx_msg void OnUpdateFileSaveregion(CCmdUI* pCmdUI);
	afx_msg void OnSelectionMovedown();
	afx_msg void OnSelectionMoveup();
	afx_msg void OnToolbarMain();
	afx_msg void OnToolbarTexture();
	afx_msg void OnSelectionPrint();
	afx_msg void OnSelectionTogglesizepaint();
	afx_msg void OnBrushMakecone();
	afx_msg void OnTexturesLoad();
	afx_msg void OnToggleRotatelock();
	afx_msg void OnCurveBevel();
	afx_msg void OnCurveCylinder();
	afx_msg void OnCurveEighthsphere();
	afx_msg void OnCurveEndcap();
	afx_msg void OnCurveHemisphere();
	afx_msg void OnCurveInvertcurve();
	afx_msg void OnCurveQuarter();
	afx_msg void OnCurveSphere();
	afx_msg void OnFileImportmap();
	afx_msg void OnFileExportmap();
	afx_msg void OnEditLoadprefab();
	afx_msg void OnViewShowcurves();
	afx_msg void OnSelectionSelectNudgedown();
	afx_msg void OnSelectionSelectNudgeleft();
	afx_msg void OnSelectionSelectNudgeright();
	afx_msg void OnSelectionSelectNudgeup();
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTexturesLoadlist();
	afx_msg void OnDontselectcurve();
	afx_msg void OnConvertcurves();
	afx_msg void OnDynamicLighting();
	afx_msg void OnCurveSimplepatchmesh();
	afx_msg void OnPatchToggleBox();
	afx_msg void OnPatchWireframe();
	afx_msg void OnCurvePatchcone();
	afx_msg void OnCurvePatchtube();
	afx_msg void OnPatchWeld();
	afx_msg void OnCurvePatchbevel();
	afx_msg void OnCurvePatchendcap();
	afx_msg void OnCurvePatchinvertedbevel();
	afx_msg void OnCurvePatchinvertedendcap();
	afx_msg void OnPatchDrilldown();
	afx_msg void OnCurveInsertcolumn();
	afx_msg void OnCurveInsertrow();
	afx_msg void OnCurveDeletecolumn();
	afx_msg void OnCurveDeleterow();
	afx_msg void OnCurveInsertAddcolumn();
	afx_msg void OnCurveInsertAddrow();
	afx_msg void OnCurveInsertInsertcolumn();
	afx_msg void OnCurveInsertInsertrow();
	afx_msg void OnCurveNegative();
	afx_msg void OnCurveNegativeTextureX();
	afx_msg void OnCurveNegativeTextureY();
	afx_msg void OnCurveDeleteFirstcolumn();
	afx_msg void OnCurveDeleteFirstrow();
	afx_msg void OnCurveDeleteLastcolumn();
	afx_msg void OnCurveDeleteLastrow();
	afx_msg void OnPatchBend();
	afx_msg void OnPatchInsdel();
	afx_msg void OnPatchEnter();
	afx_msg void OnPatchTab();
	afx_msg void OnCurvePatchdensetube();
	afx_msg void OnCurvePatchverydensetube();
	afx_msg void OnCurveCap();
	afx_msg void OnCurveCapInvertedbevel();
	afx_msg void OnCurveCapInvertedendcap();
	afx_msg void OnCurveRedisperseCols();
	afx_msg void OnCurveRedisperseRows();
	afx_msg void OnPatchNaturalize();
	afx_msg void OnSnapToGrid();
	afx_msg void OnCurvePatchsquare();
	afx_msg void OnTerrainCreateFromBrush();
	afx_msg void OnTexturesTexturewindowscale10();
	afx_msg void OnTexturesTexturewindowscale100();
	afx_msg void OnTexturesTexturewindowscale200();
	afx_msg void OnTexturesTexturewindowscale25();
	afx_msg void OnTexturesTexturewindowscale50();
	afx_msg void OnTexturesFlush();
	afx_msg void OnCurveOverlayClear();
	afx_msg void OnCurveOverlaySet();
	afx_msg void OnCurveThicken();
	afx_msg void OnCurveCyclecap();
	afx_msg void OnCurveMatrixTranspose();
	afx_msg void OnTexturesReloadshaders();
	afx_msg void OnShowEntities();
	afx_msg void OnViewEntitiesasBoundingbox();
	afx_msg void OnViewEntitiesasSelectedskinned();
	afx_msg void OnViewEntitiesasSelectedwireframe();
	afx_msg void OnViewEntitiesasSkinned();
	afx_msg void OnViewEntitiesasSkinnedandboxed();
	afx_msg void OnViewEntitiesasWireframe();
	afx_msg void OnPluginsRefresh();
	afx_msg void OnViewShowhint();
	afx_msg void OnUpdateTexturesShowinuse(CCmdUI* pCmdUI);
	afx_msg void OnTexturesShowall();
	afx_msg void OnPatchInspector();
	afx_msg void OnViewOpengllighting();
	afx_msg void OnSelectAll();
	afx_msg void OnViewShowcaulk();
  afx_msg void OnCurveFreeze();
  afx_msg void OnCurveUnFreeze();
  afx_msg void OnCurveUnFreezeAll();
  afx_msg void OnSelectReselect();
	afx_msg void OnViewShowangles();
	afx_msg void OnEditSaveprefab();
	afx_msg void OnCurveMoreendcapsbevelsSquarebevel();
	afx_msg void OnCurveMoreendcapsbevelsSquareendcap();
	afx_msg void OnBrushPrimitivesSphere();
	afx_msg void OnViewCrosshair();
	afx_msg void OnViewHideshowHideselected();
	afx_msg void OnViewHideshowShowhidden();
	afx_msg void OnTexturesShadersShow();
	afx_msg void OnTexturesFlushUnused();
	afx_msg void OnViewGroups();
	afx_msg void OnDropGroupAddtoWorld();
	afx_msg void OnDropGroupName();
	afx_msg void OnDropGroupNewgroup();
	afx_msg void OnDropGroupRemove();
	afx_msg void OnSplinesMode();
	afx_msg void OnSplinesLoad();
	afx_msg void OnSplinesSave();
	afx_msg void OnSplinesEdit();
	afx_msg void OnSplineTest();
	afx_msg void OnSplinesTarget();
	afx_msg void OnSplinesTargetPoints();
	afx_msg void OnSplinesCameraPoints();
	afx_msg void OnPopupNewcameraInterpolated();
	afx_msg void OnPopupNewcameraSpline();
	afx_msg void OnPopupNewcameraFixed();
	//}}AFX_MSG
  afx_msg void OnMru(unsigned int nID);
  afx_msg void OnViewNearest(unsigned int nID);
  afx_msg void OnTextureWad(unsigned int nID);
  afx_msg void OnBspCommand(unsigned int nID);
  afx_msg void OnGrid1(unsigned int nID);
  afx_msg LRESULT OnDisplayChange(WPARAM wp, LPARAM lp);
  void CheckTextureScale(int id);
  afx_msg void OnPlugIn(unsigned int nID);

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__330BBF0A_731C_11D1_B539_00AA00A410FC__INCLUDED_)
