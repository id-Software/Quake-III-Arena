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
#if !defined(AFX_CAMWND_H__44B4BA03_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
#define AFX_CAMWND_H__44B4BA03_781B_11D1_B53C_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CamWnd.h : header file
//
/////////////////////////////////////////////////////////////////////////////
// CCamWnd window
class CXYWnd;

class CCamWnd : public CWnd
{
  DECLARE_DYNCREATE(CCamWnd);
// Construction
public:
	CCamWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCamWnd)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ShiftTexture_BrushPrimit(face_t *f, int x, int y);
	void ReInitGL();
	void BenchMark();
	CXYWnd* m_pXYFriend;
	void SetXYFriend(CXYWnd* pWnd);
	virtual ~CCamWnd();
  camera_t& Camera(){return m_Camera;};
  void Cam_MouseControl(float dtime);
  void Cam_ChangeFloor(qboolean up);

protected:
  void Cam_Init();
  void Cam_BuildMatrix();
  void Cam_PositionDrag();
  void Cam_MouseDown(int x, int y, int buttons);
  void Cam_MouseUp (int x, int y, int buttons);
  void Cam_MouseMoved (int x, int y, int buttons);
  void InitCull();
  qboolean CullBrush (brush_t *b);
  void Cam_Draw();


  brush_t* m_TransBrushes[MAX_MAP_BRUSHES];
  int m_nNumTransBrushes;
  camera_t m_Camera;
  int	m_nCambuttonstate;
  CPoint m_ptButton;
  CPoint m_ptCursor;
  CPoint m_ptLastCursor;
  face_t* m_pSide_select;
  vec3_t m_vCull1;
  vec3_t m_vCull2;
  int m_nCullv1[3];
  int m_nCullv2[3];
  bool m_bClipMode;

	// Generated message map functions
protected:
	void OriginalMouseDown(UINT nFlags, CPoint point);
	void OriginalMouseUp(UINT nFlags, CPoint point);
	//{{AFX_MSG(CCamWnd)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAMWND_H__44B4BA03_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
