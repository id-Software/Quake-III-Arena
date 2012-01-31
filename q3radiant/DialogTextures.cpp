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
// DialogTextures.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "DialogTextures.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialogTextures dialog


CDialogTextures::CDialogTextures(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogTextures::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDialogTextures)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDialogTextures::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDialogTextures)
	DDX_Control(pDX, IDC_LIST_TEXTURES, m_wndList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDialogTextures, CDialog)
	//{{AFX_MSG_MAP(CDialogTextures)
	ON_LBN_DBLCLK(IDC_LIST_TEXTURES, OnDblclkListTextures)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialogTextures message handlers

void CDialogTextures::OnOK() 
{
	m_nSelection = m_wndList.GetCurSel();
	CDialog::OnOK();
}

void CDialogTextures::OnDblclkListTextures() 
{
  OnOK();
}

BOOL CDialogTextures::OnInitDialog() 
{
	CDialog::OnInitDialog();
  CStringArray sa;
  FillTextureMenu(&sa);
  m_nSelection = -1;
  for (int i = 0; i < sa.GetSize(); i ++)
  {
    m_wndList.AddString(sa.GetAt(i));
  }
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
