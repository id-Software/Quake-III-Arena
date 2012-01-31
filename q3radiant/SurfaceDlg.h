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
#if !defined(AFX_SURFACEDLG_H__D84E0C22_9EEA_11D1_B570_00AA00A410FC__INCLUDED_)
#define AFX_SURFACEDLG_H__D84E0C22_9EEA_11D1_B570_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SurfaceDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSurfaceDlg dialog

class CSurfaceDlg : public CDialog
{
	bool m_bPatchMode;
	// brush primitive fake shift scale rot coords
	float	m_shift[2];
	float	m_rotate;
	float	m_scale[2];

	// Construction
public:
	CSurfaceDlg(CWnd* pParent = NULL);   // standard constructor
  void SetTexMods();
  void GetTexMods();
  void GrabPatchMods();

// Dialog Data
	//{{AFX_DATA(CSurfaceDlg)
	enum { IDD = IDD_SURFACE };
	CSpinButtonCtrl	m_wndWidth;
	CSpinButtonCtrl	m_wndHeight;
	CSpinButtonCtrl	m_wndVShift;
	CSpinButtonCtrl	m_wndVScale;
	CSpinButtonCtrl	m_wndRotate;
	CSpinButtonCtrl	m_wndHShift;
	CSpinButtonCtrl	m_wndHScale;
	int		m_nHeight;
	int		m_nWidth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSurfaceDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:

  void UpdateSpinners(int nScrollCode, int nPos, CScrollBar* pBar);
  void UpdateSpinners(bool bUp, int nID);
	// Generated message map functions
	//{{AFX_MSG(CSurfaceDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnApply();
	virtual void OnOK();
	afx_msg void OnClose();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnBtnCancel();
	afx_msg void OnBtnColor();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBtnPatchdetails();
	afx_msg void OnBtnPatchnatural();
	afx_msg void OnBtnPatchreset();
	afx_msg void OnBtnPatchfit();
	afx_msg void OnBtnAxial();
	afx_msg void OnBtnBrushfit();
	afx_msg void OnBtnFacefit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SURFACEDLG_H__D84E0C22_9EEA_11D1_B570_00AA00A410FC__INCLUDED_)
