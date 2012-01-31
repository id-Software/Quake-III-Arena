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
// CameraTargetDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CameraTargetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCameraTargetDlg dialog


CCameraTargetDlg::CCameraTargetDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCameraTargetDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCameraTargetDlg)
	m_nType = 0;
	m_strName = _T("");
	//}}AFX_DATA_INIT
}


void CCameraTargetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCameraTargetDlg)
	DDX_Radio(pDX, IDC_RADIO_FIXED, m_nType);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCameraTargetDlg, CDialog)
	//{{AFX_MSG_MAP(CCameraTargetDlg)
	ON_COMMAND(ID_POPUP_NEWCAMERA_FIXED, OnPopupNewcameraFixed)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCameraTargetDlg message handlers

void CCameraTargetDlg::OnPopupNewcameraFixed() 
{
	// TODO: Add your command handler code here
	
}
