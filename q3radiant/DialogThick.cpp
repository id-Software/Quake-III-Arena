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
// DialogThick.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "DialogThick.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogThick dialog


CDialogThick::CDialogThick(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogThick::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialogThick)
	m_bSeams = TRUE;
	m_nAmount = 8;
	//}}AFX_DATA_INIT
}


void CDialogThick::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogThick)
	DDX_Check(pDX, IDC_CHECK_SEAMS, m_bSeams);
	DDX_Text(pDX, IDC_EDIT_AMOUNT, m_nAmount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialogThick, CDialog)
	//{{AFX_MSG_MAP(CDialogThick)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogThick message handlers
