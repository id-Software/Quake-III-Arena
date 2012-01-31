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
// TexEdit.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "TexEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTexEdit

CTexEdit::CTexEdit()
{
  m_pTexWnd = NULL;
}

CTexEdit::~CTexEdit()
{
}


BEGIN_MESSAGE_MAP(CTexEdit, CEdit)
	//{{AFX_MSG_MAP(CTexEdit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTexEdit message handlers

HBRUSH CTexEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
  if (nCtlColor == CTLCOLOR_EDIT)
  {
    pDC->SetBkColor(RGB(192,192,192));
	  return (HBRUSH)GetStockObject(LTGRAY_BRUSH);
  }
  return NULL;
}

void CTexEdit::OnChange() 
{
  CString str;
  GetWindowText(str);
  if (m_pTexWnd)
    m_pTexWnd->UpdateFilter(str);
}

int CTexEdit::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CEdit::OnCreate(lpCreateStruct) == -1)
		return -1;

  m_Font.CreatePointFont(100, "Arial");
	SetFont(&m_Font, FALSE);
	return 0;
}
