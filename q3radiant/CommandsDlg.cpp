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
// CommandsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "CommandsDlg.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommandsDlg dialog


CCommandsDlg::CCommandsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCommandsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCommandsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCommandsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCommandsDlg)
	DDX_Control(pDX, IDC_LIST_COMMANDS, m_lstCommands);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCommandsDlg, CDialog)
	//{{AFX_MSG_MAP(CCommandsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommandsDlg message handlers

BOOL CCommandsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
  m_lstCommands.SetTabStops(96);
  int nCount = g_nCommandCount;

  CFile fileout;
  fileout.Open("c:/commandlist.txt", CFile::modeCreate | CFile::modeWrite);
  for (int n = 0; n < nCount; n++)
  {
    CString strLine;
    char c = g_Commands[n].m_nKey;
    CString strKeys = c;
    for (int k = 0; k < g_nKeyCount; k++)
    {
      if (g_Keys[k].m_nVKKey == g_Commands[n].m_nKey)
      {
        strKeys = g_Keys[k].m_strName;
        break;
      }
    }
    CString strMod("");
    if (g_Commands[n].m_nModifiers & RAD_SHIFT)
      strMod = "Shift";
    if (g_Commands[n].m_nModifiers & RAD_ALT)
      strMod += (strMod.GetLength() > 0) ? " + Alt" : "Alt";
    if (g_Commands[n].m_nModifiers & RAD_CONTROL)
      strMod += (strMod.GetLength() > 0) ? " + Control" : "Control";
    if (strMod.GetLength() > 0)
    {
      strMod += " + ";
    }
    strLine.Format("%s \t%s%s", g_Commands[n].m_strCommand, strMod, strKeys);
    m_lstCommands.AddString(strLine);

    strLine.Format("%s \t\t\t%s%s", g_Commands[n].m_strCommand, strMod, strKeys);

    fileout.Write(strLine, strLine.GetLength());
    fileout.Write("\r\n", 2);
  }
	fileout.Close();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
