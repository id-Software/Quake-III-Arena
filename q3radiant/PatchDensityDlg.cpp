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
// PatchDensityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "PatchDensityDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchDensityDlg dialog


CPatchDensityDlg::CPatchDensityDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchDensityDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPatchDensityDlg)
	//}}AFX_DATA_INIT
}


void CPatchDensityDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatchDensityDlg)
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_wndWidth);
	DDX_Control(pDX, IDC_COMBO_HEIGHT, m_wndHeight);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPatchDensityDlg, CDialog)
	//{{AFX_MSG_MAP(CPatchDensityDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchDensityDlg message handlers

int g_nXLat[] = {3,5,7,9,11,13,15};

void CPatchDensityDlg::OnOK() 
{
  int nWidth = m_wndWidth.GetCurSel();
  int nHeight = m_wndHeight.GetCurSel();

  if (nWidth >= 0 && nWidth <= 6 && nHeight >= 0 && nHeight <= 6)
  {
	  Patch_GenericMesh(g_nXLat[nWidth], g_nXLat[nHeight], g_pParentWnd->ActiveXY()->GetViewType());
    Sys_UpdateWindows(W_ALL);
  }

  CDialog::OnOK();
}

BOOL CPatchDensityDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

  m_wndWidth.SetCurSel(0);
  m_wndHeight.SetCurSel(0);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
