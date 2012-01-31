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
// ScaleDialog.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "ScaleDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScaleDialog dialog


CScaleDialog::CScaleDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CScaleDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScaleDialog)
	m_fZ = 1.0f;
	m_fX = 1.0f;
	m_fY = 1.0f;
	//}}AFX_DATA_INIT
}


void CScaleDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScaleDialog)
	DDX_Text(pDX, IDC_EDIT_Z, m_fZ);
	DDX_Text(pDX, IDC_EDIT_X, m_fX);
	DDX_Text(pDX, IDC_EDIT_Y, m_fY);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScaleDialog, CDialog)
	//{{AFX_MSG_MAP(CScaleDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScaleDialog message handlers
