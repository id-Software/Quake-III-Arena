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
// DlgEvent.cpp : implementation file
//

#include "stdafx.h"
#include "DlgEvent.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgEvent dialog


CDlgEvent::CDlgEvent(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgEvent::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgEvent)
	m_strParm = _T("");
	m_event = 0;
	//}}AFX_DATA_INIT
}


void CDlgEvent::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgEvent)
	DDX_Text(pDX, IDC_EDIT_PARAM, m_strParm);
	DDX_Radio(pDX, IDC_RADIO_EVENT, m_event);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgEvent, CDialog)
	//{{AFX_MSG_MAP(CDlgEvent)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgEvent message handlers
