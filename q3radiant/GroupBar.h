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
#if !defined(AFX_GROUPBAR_H__926379A9_F46B_4E81_AD23_71BDDF55BDB3__INCLUDED_)
#define AFX_GROUPBAR_H__926379A9_F46B_4E81_AD23_71BDDF55BDB3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupBar dialog

class CGroupBar : public CDialogBar
{
// Construction
public:
	CGroupBar(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupBar)
	enum { IDD = IDD_GROUPBAR };
	CComboBox	m_wndGroupList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupBar)
	afx_msg void OnBtnAddgroup();
	afx_msg void OnBtnListgroups();
	afx_msg void OnBtnRemovegroup();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPBAR_H__926379A9_F46B_4E81_AD23_71BDDF55BDB3__INCLUDED_)
