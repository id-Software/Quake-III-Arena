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
#if !defined(AFX_TEXWND_H__44B4BA05_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
#define AFX_TEXWND_H__44B4BA05_781B_11D1_B53C_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TexWnd.h : header file
//
#include "texedit.h"

/////////////////////////////////////////////////////////////////////////////
// CTexWnd window

class CTexWnd : public CWnd
{
  DECLARE_DYNCREATE(CTexWnd);
// Construction
public:
	CTexWnd();
  void UpdateFilter(const char* pFilter);
  void UpdatePrefs();
  void FocusEdit();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTexWnd)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTexWnd();

protected:
  CTexEdit m_wndFilter;
  CButton  m_wndShaders;
  bool m_bNeedRange;
	// Generated message map functions
protected:
	//{{AFX_MSG(CTexWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTexturesFlush();
	//}}AFX_MSG
	afx_msg void OnShaderClick();
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXWND_H__44B4BA05_781B_11D1_B53C_00AA00A410FC__INCLUDED_)
