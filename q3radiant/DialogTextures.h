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
#if !defined(AFX_DIALOGTEXTURES_H__F3F3F984_E47E_11D1_B61B_00AA00A410FC__INCLUDED_)
#define AFX_DIALOGTEXTURES_H__F3F3F984_E47E_11D1_B61B_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DialogTextures.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialogTextures dialog

class CDialogTextures : public CDialog
{
// Construction
public:
	CDialogTextures(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialogTextures)
	enum { IDD = IDD_DIALOG_TEXTURELIST };
	CListBox	m_wndList;
	//}}AFX_DATA
  int m_nSelection;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialogTextures)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDialogTextures)
	virtual void OnOK();
	afx_msg void OnDblclkListTextures();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOGTEXTURES_H__F3F3F984_E47E_11D1_B61B_00AA00A410FC__INCLUDED_)
