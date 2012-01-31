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
// GroupBar.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "GroupBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupBar dialog


CGroupBar::CGroupBar(CWnd* pParent /*=NULL*/)
	: CDialogBar()
{
	//{{AFX_DATA_INIT(CGroupBar)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CGroupBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupBar)
	DDX_Control(pDX, IDC_COMBO_GROUPS, m_wndGroupList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupBar, CDialogBar)
	//{{AFX_MSG_MAP(CGroupBar)
	ON_BN_CLICKED(IDC_BTN_ADDGROUP, OnBtnAddgroup)
	ON_BN_CLICKED(IDC_BTN_LISTGROUPS, OnBtnListgroups)
	ON_BN_CLICKED(IDC_BTN_REMOVEGROUP, OnBtnRemovegroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupBar message handlers

void CGroupBar::OnBtnAddgroup() 
{
	// TODO: Add your control notification handler code here
	
}

void CGroupBar::OnBtnListgroups() 
{
	// TODO: Add your control notification handler code here
	
}

void CGroupBar::OnBtnRemovegroup() 
{
	// TODO: Add your control notification handler code here
	
}
