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
#if !defined(AFX_SHADEREDIT_H__CFE2CBF1_E980_11D2_A509_0020AFEB881A__INCLUDED_)
#define AFX_SHADEREDIT_H__CFE2CBF1_E980_11D2_A509_0020AFEB881A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ShaderEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CShaderEdit frame

class CShaderEdit : public CFrameWnd
{
	DECLARE_DYNCREATE(CShaderEdit)
protected:
	CShaderEdit();           // protected constructor used by dynamic creation

  CStatusBar  m_StatusBar;
	CToolBar    m_ToolBar;

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShaderEdit)
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CShaderEdit();

	// Generated message map functions
	//{{AFX_MSG(CShaderEdit)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHADEREDIT_H__CFE2CBF1_E980_11D2_A509_0020AFEB881A__INCLUDED_)
