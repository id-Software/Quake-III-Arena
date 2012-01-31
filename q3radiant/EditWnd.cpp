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
// EditWnd.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "EditWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditWnd
IMPLEMENT_DYNCREATE(CEditWnd, CWnd);

CEditWnd::CEditWnd()
{
}

CEditWnd::~CEditWnd()
{
}


BEGIN_MESSAGE_MAP(CEditWnd, CEdit)
	//{{AFX_MSG_MAP(CEditWnd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditWnd message handlers

BOOL CEditWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
  cs.style = WS_CHILD | WS_VSCROLL | ES_AUTOHSCROLL | ES_MULTILINE | WS_VISIBLE;
  cs.lpszClass = "EDIT";
	return CEdit::PreCreateWindow(cs);
}
