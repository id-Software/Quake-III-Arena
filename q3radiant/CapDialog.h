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
#if !defined(AFX_CAPDIALOG_H__10637162_2BD2_11D2_B030_00AA00A410FC__INCLUDED_)
#define AFX_CAPDIALOG_H__10637162_2BD2_11D2_B030_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CapDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCapDialog dialog

class CCapDialog : public CDialog
{
// Construction
public:
  static enum {BEVEL = 0, ENDCAP, IBEVEL, IENDCAP};
	CCapDialog(CWnd* pParent = NULL);   // standard constructor

  int getCapType() {return m_nCap;};
// Dialog Data
	//{{AFX_DATA(CCapDialog)
	enum { IDD = IDD_DIALOG_CAP };
	int		m_nCap;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCapDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCapDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAPDIALOG_H__10637162_2BD2_11D2_B030_00AA00A410FC__INCLUDED_)
