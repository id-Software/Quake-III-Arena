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
//++timo TODO : the whole CTextureBar has to be modified for the new texture code

// TextureBar.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "TextureBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextureBar dialog


CTextureBar::CTextureBar()
	: CDialogBar()
{
	//{{AFX_DATA_INIT(CTextureBar)
	m_nHShift = 0;
	m_nHScale = 0;
	m_nRotate = 0;
	m_nVShift = 0;
	m_nVScale = 0;
	m_nRotateAmt = 45;
	//}}AFX_DATA_INIT
}


void CTextureBar::DoDataExchange(CDataExchange* pDX)
{
	CDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextureBar)
	DDX_Control(pDX, IDC_SPIN_ROTATE, m_spinRotate);
	DDX_Control(pDX, IDC_SPIN_VSCALE, m_spinVScale);
	DDX_Control(pDX, IDC_SPIN_VSHIFT, m_spinVShift);
	DDX_Control(pDX, IDC_SPIN_HSCALE, m_spinHScale);
	DDX_Control(pDX, IDC_SPIN_HSHIFT, m_spinHShift);
	DDX_Text(pDX, IDC_HSHIFT, m_nHShift);
	DDX_Text(pDX, IDC_HSCALE, m_nHScale);
	DDX_Text(pDX, IDC_ROTATE, m_nRotate);
	DDX_Text(pDX, IDC_VSHIFT, m_nVShift);
	DDX_Text(pDX, IDC_VSCALE, m_nVScale);
	DDX_Text(pDX, IDC_EDIT_ROTATEAMT, m_nRotateAmt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextureBar, CDialogBar)
	//{{AFX_MSG_MAP(CTextureBar)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_HSHIFT, OnDeltaposSpinHshift)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VSHIFT, OnDeltaposSpinVshift)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_HSCALE, OnDeltaposSpinHScale)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VSCALE, OnDeltaposSpinVScale)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ROTATE, OnDeltaposSpinRotate)
	ON_COMMAND(ID_SELECTION_PRINT, OnSelectionPrint)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_BTN_APPLYTEXTURESTUFF, OnBtnApplytexturestuff)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextureBar message handlers

void CTextureBar::OnDeltaposSpinHshift(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	*pResult = 0;

  if (pNMUpDown->iDelta < 0)
    Select_ShiftTexture(abs(g_qeglobals.d_savedinfo.m_nTextureTweak), 0);
  else
    Select_ShiftTexture(-abs(g_qeglobals.d_savedinfo.m_nTextureTweak), 0);
  GetSurfaceAttributes();
}

void CTextureBar::OnDeltaposSpinVshift(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
  if (pNMUpDown->iDelta < 0)
    Select_ShiftTexture(0, abs(g_qeglobals.d_savedinfo.m_nTextureTweak));
  else
    Select_ShiftTexture(0, -abs(g_qeglobals.d_savedinfo.m_nTextureTweak));
  GetSurfaceAttributes();
}

void CTextureBar::OnDeltaposSpinHScale(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
  if (pNMUpDown->iDelta < 0)
	  Select_ScaleTexture(abs(g_qeglobals.d_savedinfo.m_nTextureTweak),0);
  else
	  Select_ScaleTexture(-abs(g_qeglobals.d_savedinfo.m_nTextureTweak),0);
  GetSurfaceAttributes();
}

void CTextureBar::OnDeltaposSpinVScale(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
  if (pNMUpDown->iDelta < 0)
	  Select_ScaleTexture(0, abs(g_qeglobals.d_savedinfo.m_nTextureTweak));
  else
	  Select_ScaleTexture(0, -abs(g_qeglobals.d_savedinfo.m_nTextureTweak));
  GetSurfaceAttributes();
}

void CTextureBar::OnDeltaposSpinRotate(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	*pResult = 0;
  UpdateData(TRUE);
  if (pNMUpDown->iDelta < 0)
    Select_RotateTexture(abs(m_nRotateAmt));
  else
    Select_RotateTexture(-abs(m_nRotateAmt));
  GetSurfaceAttributes();
}


void CTextureBar::OnSelectionPrint() 
{
	// TODO: Add your command handler code here
	
}

int CTextureBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialogBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}


void CTextureBar::OnBtnApplytexturestuff() 
{
  SetSurfaceAttributes();
}

void CTextureBar::GetSurfaceAttributes()
{
  texdef_t* pt = (g_ptrSelectedFaces.GetSize() > 0) ? &(reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0)))->texdef : &g_qeglobals.d_texturewin.texdef;

  if (pt)
  {
    m_nHShift = pt->shift[0];
    m_nVShift = pt->shift[1];
    m_nHScale = pt->scale[0];
    m_nVScale = pt->scale[1];
    m_nRotate = pt->rotate;
    UpdateData(FALSE);
  }
}

//++timo implement brush primitive here
void CTextureBar::SetSurfaceAttributes()
{
  if (g_ptrSelectedFaces.GetSize() > 0)
  {
	  if (g_qeglobals.m_bBrushPrimitMode)
    {
		  Sys_Printf("Warning : brush primitive mode not implemented in CTextureBar");
    }
    face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));

	  texdef_t* pt = &selFace->texdef;
    UpdateData(TRUE);
    pt->shift[0] = m_nHShift;
    pt->shift[1] = m_nVShift;
    pt->scale[0] = m_nHScale; 
    pt->scale[1] = m_nVScale; 
    pt->rotate = m_nRotate; 
    Sys_UpdateWindows(W_CAMERA);
  }
}
