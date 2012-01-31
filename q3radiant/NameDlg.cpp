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
// NameDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "NameDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNameDlg dialog


CNameDlg::CNameDlg(const char *pName, CWnd* pParent /*=NULL*/)
	: CDialog(CNameDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNameDlg)
	m_strName = _T("");
	//}}AFX_DATA_INIT
  m_strCaption = pName;
}


void CNameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNameDlg)
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNameDlg, CDialog)
	//{{AFX_MSG_MAP(CNameDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNameDlg message handlers

void CNameDlg::OnOK() 
{
	CDialog::OnOK();
}

BOOL CNameDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

  SetWindowText(m_strCaption);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
