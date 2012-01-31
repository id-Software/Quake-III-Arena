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
#if !defined(AFX_XYWND_H__44B4BA04_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
#define AFX_XYWND_H__44B4BA04_781B_11D1_B53C_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// XYWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CXYWnd window

#include "qe3.h"
#include "CamWnd.h"

const int SCALE_X = 0x01;
const int SCALE_Y = 0x02;
const int SCALE_Z = 0x04;


typedef void (PFNPathCallback)(bool, int);
// as i didn't really encapsulate anything this
// should really be a struct..
class CClipPoint
{
public:
  CClipPoint(){ Reset(); };
  void Reset(){ m_ptClip[0] = m_ptClip[1] = m_ptClip[2] = 0.0; m_bSet = false; m_pVec3 = NULL;};
  bool Set(){ return m_bSet; };
  void Set(bool b) { m_bSet = b; };
  void UpdatePointPtr() { if (m_pVec3) VectorCopy(m_ptClip, *m_pVec3); };
  void SetPointPtr(vec3_t* p) { m_pVec3 = p; };
  vec3_t m_ptClip;      // the 3d point
  vec3_t* m_pVec3;      // optional ptr for 3rd party updates
  CPoint m_ptScreen;    // the onscreen xy point (for mousability)
  bool m_bSet;
  operator vec3_t&() {return m_ptClip;};
  operator vec3_t*() {return &m_ptClip;};
};

class CXYWnd : public CWnd
{
  DECLARE_DYNCREATE(CXYWnd);
// Construction
public:
	CXYWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXYWnd)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
  bool AreaSelectOK();
  vec3_t& RotateOrigin();
  vec3_t& Rotation();
  void UndoClear();
  bool UndoAvailable();
  void KillPathMode();
  void Undo();
  void UndoCopy();
  void Copy();
  void Paste();
  void Redraw(unsigned int nBits);
  void VectorCopyXY(vec3_t in, vec3_t out);
	void PositionView();
	void FlipClip();
	void SplitClip();
	void Clip();
	vec3_t& GetOrigin();
	void SetOrigin(vec3_t org);		// PGM
	void XY_Init();
  void XY_Overlay();
  void XY_Draw();
  void DrawZIcon();
  void DrawRotateIcon();
  void DrawCameraIcon();
  void XY_DrawBlockGrid();
  void XY_DrawGrid();
  void XY_MouseMoved (int x, int y, int buttons);
  void NewBrushDrag (int x, int y);
  qboolean DragDelta (int x, int y, vec3_t move);
  void XY_MouseUp(int x, int y, int buttons);
  void XY_MouseDown (int x, int y, int buttons);
  void XY_ToGridPoint (int x, int y, vec3_t point);
  void XY_ToPoint (int x, int y, vec3_t point);
  void SnapToPoint (int x, int y, vec3_t point);
  void SetActive(bool b) {m_bActive = b;};
  bool Active() {return m_bActive;};
  void DropClipPoint(UINT nFlags, CPoint point);

  bool RogueClipMode();
	bool ClipMode();
	void SetClipMode(bool bMode);
	void RetainClipMode(bool bMode);

  bool RotateMode();
  bool SetRotateMode(bool bMode);
  bool ScaleMode();
  void SetScaleMode(bool bMode);

  bool PathMode();
  void DropPathPoint(UINT nFlags, CPoint point);

  bool PointMode();
  void AddPointPoint(UINT nFlags, vec3_t* pVec);
  void SetPointMode(bool b);


	virtual ~CXYWnd();
  void SetViewType(int n);
  int GetViewType() {return  m_nViewType; };
  void SetScale(float f) {m_fScale = f;};
  float Scale() {return m_fScale;};
  int Width() {return m_nWidth;}
  int Height() {return m_nHeight;}
  bool m_bActive;

	// Generated message map functions
protected:
	int m_nUpdateBits;
	int m_nWidth;
  int m_nHeight;
  bool m_bTiming;
	float	m_fScale;
	float	m_TopClip;
  float m_BottomClip;
  bool m_bDirty;
	vec3_t m_vOrigin;
	CPoint m_ptCursor;
  bool m_bRButtonDown;

  int	m_nButtonstate;
  int m_nPressx;
  int m_nPressy;
  vec3_t m_vPressdelta;
  bool m_bPress_selection;

  friend CCamWnd;
  //friend C3DFXCamWnd;

  CMenu m_mnuDrop;
  int m_nViewType;

  unsigned int m_nTimerID;
  int m_nScrollFlags;
  CPoint m_ptDrag;
  CPoint m_ptDragAdj;
  CPoint m_ptDragTotal;

	void OriginalButtonUp(UINT nFlags, CPoint point);
	void OriginalButtonDown(UINT nFlags, CPoint point);
  void ProduceSplits(brush_t** pFront, brush_t** pBack);
  void ProduceSplitLists();
  void HandleDrop();
  void PaintSizeInfo(int nDim1, int nDim2, vec3_t vMinBounds, vec3_t vMaxBounds);

  void OnEntityCreate(unsigned int nID);
  CPoint m_ptDown;
	//{{AFX_MSG(CXYWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnSelectMouserotate();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XYWND_H__44B4BA04_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
