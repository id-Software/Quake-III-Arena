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
#if !defined(AFX_GROUPDLG_H__92368487_9E05_454E_A66F_23A9A94A753B__INCLUDED_)
#define AFX_GROUPDLG_H__92368487_9E05_454E_A66F_23A9A94A753B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupDlg dialog

class CGroupDlg : public CDialog
{
  CImageList m_imgList;
// Construction
public:
	virtual void OnOK();
	virtual void OnCancel();
	void InitGroups();
	CGroupDlg(CWnd* pParent = NULL);   // standard constructor

  HTREEITEM m_hWorld;

// Dialog Data
	//{{AFX_DATA(CGroupDlg)
	enum { IDD = IDD_DLG_GROUP };
	CTreeCtrl	m_wndTree;
	CButton	m_wndEdit;
	CButton	m_wndDel;
	CButton	m_wndAdd;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupDlg)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	afx_msg void OnBtnAdd();
	afx_msg void OnBtnDel();
	afx_msg void OnBtnEdit();
	afx_msg void OnRclickTreeGroup(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndlabeleditTreeGroup(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickTreeGroup(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetdispinfoTreeGroup(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBegindragTreeGroup(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

extern CGroupDlg *g_pGroupDlg;


#endif // !defined(AFX_GROUPDLG_H__92368487_9E05_454E_A66F_23A9A94A753B__INCLUDED_)
