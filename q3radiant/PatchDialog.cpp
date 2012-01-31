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
// PatchDialog.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "PatchDialog.h"
#include "TextureLayout.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatchDialog dialog

CPatchDialog g_PatchDialog;

CPatchDialog::CPatchDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPatchDialog)
	m_strName = _T("");
	m_fS = 0.0f;
	m_fT = 0.0f;
	m_fX = 0.0f;
	m_fY = 0.0f;
	m_fZ = 0.0f;
	m_fHScale = 0.05f;
	m_fHShift = 0.05f;
	m_fRotate = 45;
	m_fVScale = 0.05f;
	m_fVShift = 0.05f;
	//}}AFX_DATA_INIT
	m_Patch = NULL;
}


void CPatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatchDialog)
	DDX_Control(pDX, IDC_SPIN_VSHIFT, m_wndVShift);
	DDX_Control(pDX, IDC_SPIN_VSCALE, m_wndVScale);
	DDX_Control(pDX, IDC_SPIN_ROTATE, m_wndRotate);
	DDX_Control(pDX, IDC_SPIN_HSHIFT, m_wndHShift);
	DDX_Control(pDX, IDC_SPIN_HSCALE, m_wndHScale);
	DDX_Control(pDX, IDC_COMBO_TYPE, m_wndType);
	DDX_Control(pDX, IDC_COMBO_ROW, m_wndRows);
	DDX_Control(pDX, IDC_COMBO_COL, m_wndCols);
	DDX_Text(pDX, IDC_EDIT_NAME, m_strName);
	DDX_Text(pDX, IDC_EDIT_S, m_fS);
	DDX_Text(pDX, IDC_EDIT_T, m_fT);
	DDX_Text(pDX, IDC_EDIT_X, m_fX);
	DDX_Text(pDX, IDC_EDIT_Y, m_fY);
	DDX_Text(pDX, IDC_EDIT_Z, m_fZ);
	DDX_Text(pDX, IDC_HSCALE, m_fHScale);
	DDX_Text(pDX, IDC_HSHIFT, m_fHShift);
	DDX_Text(pDX, IDC_ROTATE, m_fRotate);
	DDX_Text(pDX, IDC_VSCALE, m_fVScale);
	DDX_Text(pDX, IDC_VSHIFT, m_fVShift);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPatchDialog, CDialog)
	//{{AFX_MSG_MAP(CPatchDialog)
	ON_BN_CLICKED(IDC_BTN_PATCHDETAILS, OnBtnPatchdetails)
	ON_BN_CLICKED(IDC_BTN_PATCHFIT, OnBtnPatchfit)
	ON_BN_CLICKED(IDC_BTN_PATCHNATURAL, OnBtnPatchnatural)
	ON_BN_CLICKED(IDC_BTN_PATCHRESET, OnBtnPatchreset)
	ON_CBN_SELCHANGE(IDC_COMBO_COL, OnSelchangeComboCol)
	ON_CBN_SELCHANGE(IDC_COMBO_ROW, OnSelchangeComboRow)
	ON_CBN_SELCHANGE(IDC_COMBO_TYPE, OnSelchangeComboType)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_HSCALE, OnDeltaposSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ROTATE, OnDeltaposSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VSCALE, OnDeltaposSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VSHIFT, OnDeltaposSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_HSHIFT, OnDeltaposSpin)
	ON_WM_DESTROY()
	ON_BN_CLICKED(ID_APPLY, OnApply)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatchDialog message handlers

void CPatchDialog::OnBtnPatchdetails() 
{
  Patch_NaturalizeSelected(true);
  Sys_UpdateWindows(W_ALL);
}

void CPatchDialog::OnBtnPatchfit() 
{
  Patch_FitTexturing();
  Sys_UpdateWindows(W_ALL);
}

void CPatchDialog::OnBtnPatchnatural() 
{
  Patch_NaturalizeSelected();
  Sys_UpdateWindows(W_ALL);
}

void CPatchDialog::OnBtnPatchreset() 
{
  CTextureLayout dlg;
  if (dlg.DoModal() == IDOK)
  {
    Patch_ResetTexturing(dlg.m_fX, dlg.m_fY);
  }
  Sys_UpdateWindows(W_ALL);
}

void CPatchDialog::OnSelchangeComboCol() 
{
  UpdateRowColInfo();
}

void CPatchDialog::OnSelchangeComboRow() 
{
  UpdateRowColInfo();
}

void CPatchDialog::OnSelchangeComboType() 
{
	// TODO: Add your control notification handler code here
	
}

void CPatchDialog::OnOK() 
{
  m_Patch = NULL;
	
	CDialog::OnOK();
}

void CPatchDialog::OnDeltaposSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
  UpdateSpinners((pNMUpDown->iDelta > 0), pNMUpDown->hdr.idFrom);
	*pResult = 0;
}

BOOL CPatchDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  m_wndHScale.SetRange(0, 1000);
  m_wndVScale.SetRange(0, 1000);
  m_wndHShift.SetRange(0, 1000);
  m_wndVShift.SetRange(0, 1000);
  m_wndRotate.SetRange(0, 1000);

  GetPatchInfo();

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CPatchDialog::GetPatchInfo()
{
  m_Patch = SinglePatchSelected();
  if (m_Patch != NULL)
  {
    CString str;
    int i;
    m_wndRows.ResetContent();
    for (i = 0; i < m_Patch->height; i++)
    {
      str.Format("%i", i);
      m_wndRows.AddString(str);
    }
    m_wndRows.SetCurSel(0);
    m_wndCols.ResetContent();
    for (i = 0; i < m_Patch->width; i++)
    {
      str.Format("%i", i);
      m_wndCols.AddString(str);
    }
    m_wndCols.SetCurSel(0);
  }
  UpdateRowColInfo();
}

void CPatchDialog::SetPatchInfo()
{

}

void DoPatchInspector()
{
  if (g_PatchDialog.GetSafeHwnd() == NULL)
  {
    g_PatchDialog.Create(IDD_DIALOG_PATCH);
    CRect rct;
	  LONG lSize = sizeof(rct);
	  if (LoadRegistryInfo("Radiant::PatchWindow", &rct, &lSize))
    {
      g_PatchDialog.SetWindowPos(NULL, rct.left, rct.top, 0,0, SWP_NOSIZE);
    }
  }
  g_PatchDialog.ShowWindow(SW_SHOW);
  g_PatchDialog.GetPatchInfo();
}

void UpdatePatchInspector()
{
  if (g_PatchDialog.GetSafeHwnd() != NULL)
  {
    g_PatchDialog.UpdateInfo();
  }

}

void CPatchDialog::OnDestroy() 
{
  if (GetSafeHwnd())
  {
    CRect rct;
    GetWindowRect(rct);
	  SaveRegistryInfo("Radiant::PatchWindow", &rct, sizeof(rct));
  }
	CDialog::OnDestroy();
}

void CPatchDialog::UpdateRowColInfo()
{
  m_fX = m_fY = m_fZ = m_fS = m_fT = 0.0;

  if (m_Patch != NULL)
  {
    int r = m_wndRows.GetCurSel();
    int c = m_wndCols.GetCurSel();
    if (r >= 0 && r < m_Patch->height && c >= 0 && c < m_Patch->width)
    {
      m_fX = m_Patch->ctrl[c][r].xyz[0];
      m_fY = m_Patch->ctrl[c][r].xyz[1];
      m_fZ = m_Patch->ctrl[c][r].xyz[2];
      m_fS = m_Patch->ctrl[c][r].st[0];
      m_fT = m_Patch->ctrl[c][r].st[1];
    }
  }
  UpdateData(FALSE);
}

void CPatchDialog::UpdateInfo()
{
  GetPatchInfo();
}

void CPatchDialog::OnApply() 
{
	UpdateData(TRUE);
  if (m_Patch != NULL)
  {
    int r = m_wndRows.GetCurSel();
    int c = m_wndCols.GetCurSel();
    if (r >= 0 && r < m_Patch->height && c >= 0 && c < m_Patch->width)
    {
      m_Patch->ctrl[c][r].xyz[0] = m_fX;
      m_Patch->ctrl[c][r].xyz[1] = m_fY;
      m_Patch->ctrl[c][r].xyz[2] = m_fZ;
      m_Patch->ctrl[c][r].st[0] = m_fS;
      m_Patch->ctrl[c][r].st[1] = m_fT;
      m_Patch->bDirty = true;
      Sys_UpdateWindows(W_ALL);
    }
  }
}

void CPatchDialog::UpdateSpinners(bool bUp, int nID)
{
  texdef_t td;

  td.rotate = 0.0;
  td.scale[0] = td.scale[1] = 0.0;
  td.shift[0] = td.shift[1] = 0.0;
  td.contents = 0;
  td.flags = 0;
  td.value = 0;


  UpdateData(TRUE);

  if (nID == IDC_SPIN_ROTATE)
  {
	  if (bUp)
		  td.rotate = m_fRotate;
		else
		  td.rotate = -m_fRotate;
	}
  else if (nID == IDC_SPIN_HSCALE)
	{
	  if (bUp)
	    td.scale[0] = 1 - m_fHScale;
	  else
		  td.scale[0] = 1 + m_fHScale;
  } 
  else if (nID == IDC_SPIN_VSCALE)
	{
	  if (bUp)
		  td.scale[1] = 1 - m_fVScale;
		else
		  td.scale[1] = 1 + m_fVScale;
  } 
	  
  else if (nID == IDC_SPIN_HSHIFT)
	{
	  if (bUp)
		  td.shift[0] = m_fHShift;
		else
		  td.shift[0] = -m_fHShift;
	}
  else if (nID == IDC_SPIN_VSHIFT)
	{
	  if (bUp)
	    td.shift[1] = m_fVShift;
		else
		  td.shift[1] = -m_fVShift;
	}
  
  Patch_SetTextureInfo(&td);
  Sys_UpdateWindows(W_CAMERA);
}


