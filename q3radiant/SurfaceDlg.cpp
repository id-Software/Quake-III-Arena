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
// SurfaceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "SurfaceDlg.h"
#include "PrefsDlg.h"
#include "mainfrm.h"
#include "TextureLayout.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// surface properties plugin
// global flag for surface properties plugin is g_qeglobals.bSurfacePropertiesPlugin
_QERPlugSurfaceTable g_SurfaceTable;

/////////////////////////////////////////////////////////////////////////////
// CSurfaceDlg dialog

CSurfaceDlg g_dlgSurface;


CSurfaceDlg::CSurfaceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSurfaceDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSurfaceDlg)
	m_nHeight = 1;
	m_nWidth = 1;
	//}}AFX_DATA_INIT
}


void CSurfaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSurfaceDlg)
	DDX_Control(pDX, IDC_SPIN_WIDTH, m_wndWidth);
	DDX_Control(pDX, IDC_SPIN_HEIGHT, m_wndHeight);
	DDX_Control(pDX, IDC_SPIN_VSHIFT, m_wndVShift);
	DDX_Control(pDX, IDC_SPIN_VSCALE, m_wndVScale);
	DDX_Control(pDX, IDC_SPIN_ROTATE, m_wndRotate);
	DDX_Control(pDX, IDC_SPIN_HSHIFT, m_wndHShift);
	DDX_Control(pDX, IDC_SPIN_HSCALE, m_wndHScale);
	DDX_Text(pDX, IDC_EDIT_HEIGHT, m_nHeight);
	DDV_MinMaxInt(pDX, m_nHeight, 1, 32);
	DDX_Text(pDX, IDC_EDIT_WIDTH, m_nWidth);
	DDV_MinMaxInt(pDX, m_nWidth, 1, 32);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSurfaceDlg, CDialog)
	//{{AFX_MSG_MAP(CSurfaceDlg)
	ON_WM_HSCROLL()
	ON_WM_KEYDOWN()
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDAPPLY, OnApply)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(ID_BTN_CANCEL, OnBtnCancel)
	ON_BN_CLICKED(IDC_BTN_COLOR, OnBtnColor)
	ON_WM_CTLCOLOR()
	ON_WM_CREATE()
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_HSHIFT, OnDeltaPosSpin)
	ON_BN_CLICKED(IDC_BTN_PATCHDETAILS, OnBtnPatchdetails)
	ON_BN_CLICKED(IDC_BTN_PATCHNATURAL, OnBtnPatchnatural)
	ON_BN_CLICKED(IDC_BTN_PATCHRESET, OnBtnPatchreset)
	ON_BN_CLICKED(IDC_BTN_PATCHFIT, OnBtnPatchfit)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_HSCALE, OnDeltaPosSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_ROTATE, OnDeltaPosSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VSCALE, OnDeltaPosSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_VSHIFT, OnDeltaPosSpin)
	ON_BN_CLICKED(IDC_BTN_AXIAL, OnBtnAxial)
	ON_BN_CLICKED(IDC_BTN_BRUSHFIT, OnBtnBrushfit)
	ON_BN_CLICKED(IDC_BTN_FACEFIT, OnBtnFacefit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSurfaceDlg message handlers


/*
===================================================

  SURFACE INSPECTOR

===================================================
*/

texdef_t	g_old_texdef;
texdef_t	g_patch_texdef;
HWND		g_surfwin = NULL;
qboolean	g_changed_surface;

int	g_checkboxes[64] = { 
	IDC_CHECK1, IDC_CHECK2, IDC_CHECK3, IDC_CHECK4, 
	IDC_CHECK5, IDC_CHECK6, IDC_CHECK7, IDC_CHECK8, 
	IDC_CHECK9, IDC_CHECK10, IDC_CHECK11, IDC_CHECK12, 
	IDC_CHECK13, IDC_CHECK14, IDC_CHECK15, IDC_CHECK16,
	IDC_CHECK17, IDC_CHECK18, IDC_CHECK19, IDC_CHECK20,
	IDC_CHECK21, IDC_CHECK22, IDC_CHECK23, IDC_CHECK24,
	IDC_CHECK25, IDC_CHECK26, IDC_CHECK27, IDC_CHECK28,
	IDC_CHECK29, IDC_CHECK30, IDC_CHECK31, IDC_CHECK32,

	IDC_CHECK33, IDC_CHECK34, IDC_CHECK35, IDC_CHECK36,
	IDC_CHECK37, IDC_CHECK38, IDC_CHECK39, IDC_CHECK40,
	IDC_CHECK41, IDC_CHECK42, IDC_CHECK43, IDC_CHECK44,
	IDC_CHECK45, IDC_CHECK46, IDC_CHECK47, IDC_CHECK48,
	IDC_CHECK49, IDC_CHECK50, IDC_CHECK51, IDC_CHECK52,
	IDC_CHECK53, IDC_CHECK54, IDC_CHECK55, IDC_CHECK56,
	IDC_CHECK57, IDC_CHECK58, IDC_CHECK59, IDC_CHECK60,
	IDC_CHECK61, IDC_CHECK62, IDC_CHECK63, IDC_CHECK64
 };

/*
==============
SetTexMods

Set the fields to the current texdef
if one face selected -> will read this face texdef, else current texdef
if only patches selected, will read the patch texdef
===============
*/

bool g_bNewFace = false;
bool g_bNewApplyHandling = false;
bool g_bGatewayhack = false;

void CSurfaceDlg::SetTexMods()
{
	char	sz[128];
	texdef_t *pt;
	brushprimit_texdef_t	*bpt;
	// local copy if a width=2 height=2 qtetxture_t is needed
	brushprimit_texdef_t	local_bp;
	int		i;

	if (!g_surfwin)
		return;

	m_bPatchMode = false;

	if (OnlyPatchesSelected())
	{
		pt = &g_qeglobals.d_texturewin.texdef;
		if (QE_SingleBrush())
    {
			//strcpy(g_patch_texdef.name, Patch_GetTextureName());
			g_patch_texdef.SetName(Patch_GetTextureName());
    }
		else
    {
			//strcpy(g_patch_texdef.name, pt->name);
			g_patch_texdef.SetName(pt->name);
    }
		g_patch_texdef.contents = pt->contents;
		g_patch_texdef.flags = pt->flags;
		g_patch_texdef.value = pt->value;
		pt = &g_patch_texdef;
		m_bPatchMode = true;
	}
	else
	{
		if (g_bNewFace && g_ptrSelectedFaces.GetSize() > 0)
		{
      face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
			pt = &selFace->texdef;
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				// compute a texture matrix related to the default matrix width=2 height=2
				ConvertTexMatWithQTexture( &selFace->brushprimit_texdef, selFace->d_texture, &local_bp, NULL );
				bpt = &local_bp;
			}
		}
		else
		{
			pt = &g_qeglobals.d_texturewin.texdef;
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				bpt = &g_qeglobals.d_texturewin.brushprimit_texdef;
			}
		}
		// brush primitive mode : compute fake shift scale rot representation
		if (g_qeglobals.m_bBrushPrimitMode)
			TexMatToFakeTexCoords( bpt->coords, m_shift, &m_rotate, m_scale );
	}

	SendMessage (WM_SETREDRAW, 0, 0);

	::SetWindowText(GetDlgItem(IDC_TEXTURE)->GetSafeHwnd(), pt->name);

	if (m_bPatchMode)
		sprintf(sz, "%4.6f", pt->shift[0]);
	else
		if (g_qeglobals.m_bBrushPrimitMode)
			sprintf(sz, "%d", (int)m_shift[0]);
		else
			sprintf(sz, "%d", (int)pt->shift[0]);
	::SetWindowText(GetDlgItem(IDC_HSHIFT)->GetSafeHwnd(), sz);

	if (m_bPatchMode)
		sprintf(sz, "%4.6f", pt->shift[1]);
	else
		if (g_qeglobals.m_bBrushPrimitMode)
			sprintf(sz, "%d", (int)m_shift[1]);
		else
			sprintf(sz, "%d", (int)pt->shift[1]);
	::SetWindowText(GetDlgItem(IDC_VSHIFT)->GetSafeHwnd(), sz);

	sprintf(sz, m_bPatchMode ? "%4.6f" : "%4.6f", g_qeglobals.m_bBrushPrimitMode ? m_scale[0] : pt->scale[0]);
	::SetWindowText(GetDlgItem(IDC_HSCALE)->GetSafeHwnd(), sz);

	sprintf(sz, m_bPatchMode ? "%4.6f" : "%4.6f", g_qeglobals.m_bBrushPrimitMode ? m_scale[1] : pt->scale[1]);
	::SetWindowText(GetDlgItem(IDC_VSCALE)->GetSafeHwnd(), sz);

	//++timo compute BProtate as int ..
	sprintf(sz, "%d", g_qeglobals.m_bBrushPrimitMode ? (int)m_rotate : (int)pt->rotate);
	::SetWindowText(GetDlgItem(IDC_ROTATE)->GetSafeHwnd(), sz);

	sprintf(sz, "%d", (int)pt->value);
	::SetWindowText(GetDlgItem(IDC_VALUE)->GetSafeHwnd(), sz);

	for (i=0 ; i<32 ; i++)
		::SendMessage(GetDlgItem(g_checkboxes[i])->GetSafeHwnd(), BM_SETCHECK, !!(pt->flags&(1<<i)), 0 );
	for (i=0 ; i<32 ; i++)
		::SendMessage(GetDlgItem(g_checkboxes[32+i])->GetSafeHwnd(), BM_SETCHECK, !!(pt->contents&(1<<i)), 0 );

	SendMessage (WM_SETREDRAW, 1, 0);
	InvalidateRect (NULL, true);
}

void CSurfaceDlg::GrabPatchMods()
{
	char	sz[128];
	int i;
	bool b;
	texdef_t *pt = & g_patch_texdef;

  ::GetWindowText (GetDlgItem(IDC_HSHIFT)->GetSafeHwnd(), sz, 127);
	pt->shift[0] = atof(sz);

  ::GetWindowText (GetDlgItem(IDC_VSHIFT)->GetSafeHwnd(), sz, 127);
	pt->shift[1] = atof(sz);

  ::GetWindowText(GetDlgItem(IDC_HSCALE)->GetSafeHwnd(), sz, 127);
	pt->scale[0] = atof(sz);

  ::GetWindowText(GetDlgItem(IDC_VSCALE)->GetSafeHwnd(), sz, 127);
	pt->scale[1] = atof(sz);

  ::GetWindowText(GetDlgItem(IDC_ROTATE)->GetSafeHwnd(), sz, 127);
	pt->rotate = atof(sz);

  ::GetWindowText(GetDlgItem(IDC_VALUE)->GetSafeHwnd(), sz, 127);
	pt->value = atof(sz);

	pt->flags = 0;
	for (i=0 ; i<32 ; i++)
	{
    b = ::SendMessage(GetDlgItem(g_checkboxes[i])->GetSafeHwnd(), BM_GETCHECK, 0, 0);
		if (b != 1 && b != 0)
			continue;
		pt->flags |= b<<i;
	}

	pt->contents = 0;
	for (i=0 ; i<32 ; i++)
	{
    b = ::SendMessage(GetDlgItem(g_checkboxes[32+i])->GetSafeHwnd(), BM_GETCHECK, 0, 0);
		if (b != 1 && b != 0)
			continue;
		pt->contents |= b<<i;
	}

}

/*
==============
GetTexMods

Reads the fields to get the current texdef
in brush primitive mode, grab the fake shift scale rot and compute a new texture matrix
===============
*/
void CSurfaceDlg::GetTexMods()
{
	char	sz[128];
	texdef_t *pt;
	int		b;
	int		i;

	m_bPatchMode = false;

	if (OnlyPatchesSelected())
	{
		pt = &g_qeglobals.d_texturewin.texdef;
		m_bPatchMode = true;
	}
	else
	{
		if (g_bNewFace && g_ptrSelectedFaces.GetSize() > 0)
    {
      face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
			pt = &selFace->texdef;
    }
		else
    {
			pt = &g_qeglobals.d_texturewin.texdef;
    }
	}

	::GetWindowText (GetDlgItem(IDC_TEXTURE)->GetSafeHwnd(), sz, 127);
	//strncpy (pt->name, sz, sizeof(pt->name)-1);
	pt->SetName(sz);
	if (pt->name[0] <= ' ')
	{
		//strcpy (pt->name, "none");
		pt->SetName("none");
		::SetWindowText(GetDlgItem(IDC_TEXTURE)->GetSafeHwnd(), pt->name);
	}

	::GetWindowText (GetDlgItem(IDC_HSHIFT)->GetSafeHwnd(), sz, 127);
	( g_qeglobals.m_bBrushPrimitMode ? m_shift[0] : pt->shift[0] ) = atof(sz);

	::GetWindowText (GetDlgItem(IDC_VSHIFT)->GetSafeHwnd(), sz, 127);
	( g_qeglobals.m_bBrushPrimitMode ? m_shift[1] : pt->shift[1] ) = atof(sz);

	::GetWindowText(GetDlgItem(IDC_HSCALE)->GetSafeHwnd(), sz, 127);
	( g_qeglobals.m_bBrushPrimitMode ? m_scale[0] : pt->scale[0] ) = atof(sz);

	::GetWindowText(GetDlgItem(IDC_VSCALE)->GetSafeHwnd(), sz, 127);
	( g_qeglobals.m_bBrushPrimitMode ? m_scale[1] : pt->scale[1] ) = atof(sz);

	::GetWindowText(GetDlgItem(IDC_ROTATE)->GetSafeHwnd(), sz, 127);
	( g_qeglobals.m_bBrushPrimitMode ? m_rotate : pt->rotate ) = atof(sz);

	::GetWindowText(GetDlgItem(IDC_VALUE)->GetSafeHwnd(), sz, 127);
	pt->value = atof(sz);

	pt->flags = 0;
	for (i=0 ; i<32 ; i++)
	{
		b = ::SendMessage(GetDlgItem(g_checkboxes[i])->GetSafeHwnd(), BM_GETCHECK, 0, 0);
		if (b != 1 && b != 0)
			continue;
		pt->flags |= b<<i;
	}

	pt->contents = 0;
	for (i=0 ; i<32 ; i++)
	{
		b = ::SendMessage(GetDlgItem(g_checkboxes[32+i])->GetSafeHwnd(), BM_GETCHECK, 0, 0);
		if (b != 1 && b != 0)
			continue;
		pt->contents |= b<<i;
	}

	g_changed_surface = true;

	// a local copy of the texture matrix, given for a qtexture_t with width=2 height=2
	brushprimit_texdef_t	local_bp;
	brushprimit_texdef_t	*bpt;
	if (g_qeglobals.m_bBrushPrimitMode)
	{
    face_t *selFace = NULL;
		if (g_bNewFace && g_ptrSelectedFaces.GetSize() > 0)
    {
      selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
			bpt = &selFace->brushprimit_texdef;
    }
		else
    {
			bpt = &g_qeglobals.d_texturewin.brushprimit_texdef;
    }
		// compute texture matrix
		// the matrix returned must be understood as a qtexture_t with width=2 height=2
		FakeTexCoordsToTexMat( m_shift, m_rotate, m_scale, local_bp.coords );
		// copy the texture matrix in the global struct
		// fit the qtexture if we have a face selected, otherwise g_qeglobals.d_texturewin.brushprimit_texdef uses the basic qtexture_t with width=2 height=2

		ConvertTexMatWithQTexture( &local_bp, NULL, bpt, ( ( g_bNewFace && selFace ) ? selFace->d_texture : NULL ) );
	}
	Select_SetTexture(pt,&local_bp);

  //if (m_bPatchMode)
  //{
  //  Patch_SetTextureInfo(pt);
  //}

}

/*
=================
UpdateSpinners
=================
*/

void CSurfaceDlg::UpdateSpinners(bool bUp, int nID)
{
	texdef_t *pt;
  texdef_t td;

  if (m_bPatchMode)
  {
    td.rotate = 0.0;
    td.scale[0] = td.scale[1] = 0.0;
    td.shift[0] = td.shift[1] = 0.0;
    GrabPatchMods();

    pt = &g_patch_texdef;
    td.contents = pt->contents;
    td.flags = pt->flags;
    td.value = pt->value;

    if (nID == IDC_SPIN_ROTATE)
	  {
		  if (bUp)
			  td.rotate = pt->rotate;
		  else
			  td.rotate = -pt->rotate;
	  }
    else if (nID == IDC_SPIN_HSCALE)
	  {
		  if (bUp)
			  td.scale[0] = 1-pt->scale[0];
		  else
			  td.scale[0] = 1+pt->scale[0];
	  }
    else if (nID == IDC_SPIN_VSCALE)
	  {
		  if (bUp)
			  td.scale[1] = 1-pt->scale[1];
		  else
			  td.scale[1] = 1+pt->scale[1];
	  } 
	  
    else if (nID == IDC_SPIN_HSHIFT)
	  {
		  if (bUp)
			  td.shift[0] = pt->shift[0];
		  else
			  td.shift[0] = -pt->shift[0];
	  }
    else if (nID == IDC_SPIN_VSHIFT)
	  {
		  if (bUp)
			  td.shift[1] = pt->shift[1];
		  else
			  td.shift[1] = -pt->shift[1];
	  }
    pt = &g_qeglobals.d_texturewin.texdef;
    Patch_SetTextureInfo(&td);
  }
	else
	{
		// in brush primitive mode, will read up-to-date m_shift m_rotate m_scale
		GetTexMods ();
		if (g_bNewFace && g_ptrSelectedFaces.GetSize() > 0)
    {
      face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
			pt = &selFace->texdef;
    }
		else
    {
			pt = &g_qeglobals.d_texturewin.texdef;
    }
		if (nID == IDC_SPIN_ROTATE)
		{
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				if (bUp)
					m_rotate += 45;
				else
					m_rotate -= 45;
			}
			else
			{
				if (bUp)
					pt->rotate += 45;
				else
					pt->rotate -= 45;
				if (pt->rotate < 0)
					pt->rotate += 360;
				if (pt->rotate >= 360)
					pt->rotate -= 360;
			}
		}
		else if (nID == IDC_SPIN_HSCALE)
		{
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				if (bUp)
					m_scale[0] += 0.1;
				else
					m_scale[0] -= 0.1;
			}
			else
			{
				if (bUp)
					pt->scale[0] += 0.1;
				else
					pt->scale[0] -= 0.1;
			}
		}
		else if (nID == IDC_SPIN_VSCALE)
		{
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				if (bUp)
					m_scale[1] += 0.1;
				else
					m_scale[1] -= 0.1;
			}
			else
			{
				if (bUp)
					pt->scale[1] += 0.1;
				else
					pt->scale[1] -= 0.1;
			}
		}
		else if (nID == IDC_SPIN_HSHIFT)
		{
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				if (bUp)
					m_shift[0] += 8;
				else
					m_shift[0] -= 8;
			}
			else
			{
				if (bUp)
					pt->shift[0] += 8;
				else
					pt->shift[0] -= 8;
			}
		}
		else if (nID == IDC_SPIN_VSHIFT)
		{
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				if (bUp)
					m_shift[1] += 8;
				else
					m_shift[1] -= 8;
			}
			else
			{
				if (bUp)
					pt->shift[1] += 8;
				else
					pt->shift[1] -= 8;
			}
		}
	}
	// a local copy of the texture matrix, given for a qtexture_t with width=2 height=2
	brushprimit_texdef_t	local_bp;
	brushprimit_texdef_t	*bpt;
	if (g_qeglobals.m_bBrushPrimitMode)
	{
    face_t *selFace = NULL;
		if (g_bNewFace && g_ptrSelectedFaces.GetSize() > 0)
    {
      selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
			bpt = &selFace->brushprimit_texdef;
    }
		else
    {
			bpt = &g_qeglobals.d_texturewin.brushprimit_texdef;
    }
		// compute texture matrix
		// the matrix returned must be understood as a qtexture_t with width=2 height=2
		FakeTexCoordsToTexMat( m_shift, m_rotate, m_scale, local_bp.coords );
		// copy the texture matrix in the global struct
		// fit the qtexture if we have a face selected, otherwise g_qeglobals.d_texturewin.brushprimit_texdef uses the basic qtexture_t with width=2 height=2
		ConvertTexMatWithQTexture( &local_bp, NULL, bpt, ( ( g_bNewFace && selFace ) ? selFace->d_texture : NULL ) );
	}
	// brush primit : will update the widgets after reading back texture matrix and computing fake shift scale rot
	SetTexMods();
	g_changed_surface = true;
	Select_SetTexture(pt,&local_bp);
}

void CSurfaceDlg::UpdateSpinners(int nScrollCode, int nPos, CScrollBar* pBar)
{
	texdef_t *pt;

	GetTexMods ();
  if (g_bNewFace && g_ptrSelectedFaces.GetSize() > 0)
  {
    face_t *selFace = reinterpret_cast<face_t*>(g_ptrSelectedFaces.GetAt(0));
		pt = &selFace->texdef;
  }
  else
  {
	  pt = &g_qeglobals.d_texturewin.texdef;
  }

	if ((nScrollCode != SB_LINEUP) && (nScrollCode != SB_LINEDOWN))
		return;
	
  if (pBar->GetSafeHwnd() == ::GetDlgItem(GetSafeHwnd(), IDC_ROTATEA))
	{
		if (nScrollCode == SB_LINEUP)
			pt->rotate += 45;
		else
			pt->rotate -= 45;

		if (pt->rotate < 0)
			pt->rotate += 360;

		if (pt->rotate >= 360)
			pt->rotate -= 360;
	}

  else if (pBar->GetSafeHwnd() == ::GetDlgItem(GetSafeHwnd(), IDC_HSCALEA))
	{
		if (nScrollCode == SB_LINEDOWN)
			pt->scale[0] -= 0.1;
		else
			pt->scale[0] += 0.1;
	}
	
  else if (pBar->GetSafeHwnd() == ::GetDlgItem(GetSafeHwnd(), IDC_VSCALEA))
	{
		if (nScrollCode == SB_LINEUP)
			pt->scale[1] += 0.1;
		else
			pt->scale[1] -= 0.1;
	} 
	
  else if (pBar->GetSafeHwnd() == ::GetDlgItem(GetSafeHwnd(), IDC_HSHIFTA))
	{
		if (nScrollCode == SB_LINEDOWN)
			pt->shift[0] -= 8;
		else
			pt->shift[0] += 8;
	}
	
  else if (pBar->GetSafeHwnd() == ::GetDlgItem(GetSafeHwnd(), IDC_VSHIFTA))
	{
		if (nScrollCode == SB_LINEUP)
			pt->shift[1] += 8;
		else
			pt->shift[1] -= 8;
	}

	SetTexMods();
	g_changed_surface = true;
	//++timo if !g_qeglobals.m_bBrushPrimitMode send a NULL brushprimit_texdef
	if (!g_qeglobals.m_bBrushPrimitMode)
	{
		Sys_Printf("Warning : non brush primitive mode call to CSurfaceDlg::GetTexMods broken\n");
		Sys_Printf("          ( Select_SetTexture not called )\n");
	}
//	Select_SetTexture(pt);
}

void UpdateSurfaceDialog()
{
	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
		g_SurfaceTable.m_pfnUpdateSurfaceDialog();
	}
	else
	{
		if (g_surfwin)
			g_dlgSurface.SetTexMods();
	}
	g_pParentWnd->UpdateTextureBar();
}

bool ByeByeSurfaceDialog();

void DoSurface (void)
{
	// surface properties plugin ?
	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
		g_SurfaceTable.m_pfnDoSurface();
		return;
	}

  g_bNewFace = g_PrefsDlg.m_bFace;
  g_bNewApplyHandling = g_PrefsDlg.m_bNewApplyHandling;
  g_bGatewayhack = g_PrefsDlg.m_bGatewayHack;
	// save current state for cancel
	g_old_texdef = g_qeglobals.d_texturewin.texdef;
	g_changed_surface = false;

  if (g_surfwin == NULL && g_dlgSurface.GetSafeHwnd() == NULL)
  {
    g_patch_texdef.scale[0] = 0.05;
    g_patch_texdef.scale[1] = 0.05;
    g_patch_texdef.shift[0] = 0.05;
    g_patch_texdef.shift[1] = 0.05;
	// use rotation increment from preferences
    g_patch_texdef.rotate = g_PrefsDlg.m_nRotation;

    g_dlgSurface.Create(IDD_SURFACE);
    CRect rct;
	  LONG lSize = sizeof(rct);
	  if (LoadRegistryInfo("Radiant::SurfaceWindow", &rct, &lSize))
      g_dlgSurface.SetWindowPos(NULL, rct.left, rct.top, 0,0, SWP_NOSIZE | SWP_SHOWWINDOW);

    Sys_UpdateWindows(W_ALL);
  }
  else
  {
    g_surfwin = g_dlgSurface.GetSafeHwnd();
	  g_dlgSurface.SetTexMods ();
    g_dlgSurface.ShowWindow(SW_SHOW);
  }
}		

bool ByeByeSurfaceDialog()
{
	// surface properties plugin ?
	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
		return g_SurfaceTable.m_pfnByeByeSurfaceDialog();
	}

  if (g_surfwin)
  {
    if (g_bGatewayhack)
      PostMessage(g_surfwin, WM_COMMAND, IDAPPLY, 0);
    else
      PostMessage(g_surfwin, WM_COMMAND, IDCANCEL, 0);
    return true;
  }
  else return false;
}

BOOL CSurfaceDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  g_surfwin = GetSafeHwnd();
	SetTexMods ();

#ifdef QUAKE3
  GetDlgItem(IDC_CHECK32)->SetWindowText("Curve");
  GetDlgItem(IDC_CHECK64)->SetWindowText("Inverted");
#endif

  m_wndHScale.SetRange(0, 1000);
  m_wndVScale.SetRange(0, 1000);
  m_wndHShift.SetRange(0, 1000);
  m_wndVShift.SetRange(0, 1000);
  m_wndRotate.SetRange(0, 1000);
  m_wndWidth.SetRange(1, 32);
  m_wndHeight.SetRange(1, 32);

  LPVOID lpv = g_pParentWnd->GetPlugInMgr().GetSurfaceFlags();
  if (lpv != NULL)
  {
    int i = 0;
    char* p = reinterpret_cast<char*>(lpv);
    char* pBuff = new char[strlen(p)+1];
    strcpy(pBuff, p);
    char* pToken = strtok(pBuff, ";\0");
    while (pToken != NULL)
    {
      GetDlgItem(g_checkboxes[i++])->SetWindowText(pToken);
      pToken = strtok(NULL, ";\0");
    }
  }

  if (strstr(g_PrefsDlg.m_strWhatGame, "Quake3") != NULL) {
    for (int i=0 ; i < 64 ; i++) {
      ::EnableWindow(GetDlgItem(g_checkboxes[i])->GetSafeHwnd(), FALSE);
    }

  }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSurfaceDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
	UpdateSpinners(nSBCode, nPos, pScrollBar);
  Sys_UpdateWindows(W_CAMERA);
}

void CSurfaceDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSurfaceDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
	UpdateSpinners(nSBCode, nPos, pScrollBar);
	Sys_UpdateWindows(W_CAMERA);
}

void CSurfaceDlg::OnApply() 
{
	GetTexMods ();
  Sys_UpdateWindows(W_CAMERA);
  if (g_bNewApplyHandling)
    OnOK();
}

void CSurfaceDlg::OnOK() 
{
	GetTexMods();
  g_surfwin = NULL;
	CDialog::OnOK();
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnClose() 
{
  g_surfwin = NULL;
	CDialog::OnClose();
}

void CSurfaceDlg::OnCancel() 
{
  if (g_bGatewayhack)
    OnOK();
  else
    OnBtnCancel();
}

void CSurfaceDlg::OnDestroy() 
{
  if (GetSafeHwnd())
  {
    CRect rct;
    GetWindowRect(rct);
	  SaveRegistryInfo("Radiant::SurfaceWindow", &rct, sizeof(rct));
  }
	CDialog::OnDestroy();
  g_surfwin = NULL;
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnBtnCancel() 
{
	g_qeglobals.d_texturewin.texdef = g_old_texdef;
	if (g_changed_surface)
	{
		//++timo if !g_qeglobals.m_bBrushPrimitMode send a NULL brushprimit_texdef
		if (!g_qeglobals.m_bBrushPrimitMode)
		{
			Sys_Printf("Warning : non brush primitive mode call to CSurfaceDlg::GetTexMods broken\n");
			Sys_Printf("          ( Select_SetTexture not called )\n");
		}
//		Select_SetTexture(&g_qeglobals.d_texturewin.texdef);
	}
  g_surfwin = NULL;
  DestroyWindow();
}

void CSurfaceDlg::OnBtnColor() 
{
}

HBRUSH CSurfaceDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	return hbr;
}

int CSurfaceDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}

BOOL CSurfaceDlg::PreCreateWindow(CREATESTRUCT& cs) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CDialog::PreCreateWindow(cs);
}


void CSurfaceDlg::OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
  UpdateSpinners((pNMUpDown->iDelta > 0), pNMUpDown->hdr.idFrom);
	*pResult = 0;
}

void CSurfaceDlg::OnBtnPatchdetails() 
{
  Patch_NaturalizeSelected(true);
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnBtnPatchnatural() 
{
  Patch_NaturalizeSelected();
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnBtnPatchreset() 
{
  CTextureLayout dlg;
  if (dlg.DoModal() == IDOK)
  {
    Patch_ResetTexturing(dlg.m_fX, dlg.m_fY);
  }
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnBtnPatchfit() 
{
  Patch_FitTexturing();
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnBtnAxial() 
{
  Select_SetTexture (&g_qeglobals.d_texturewin.texdef, &g_qeglobals.d_texturewin.brushprimit_texdef, true);
	g_changed_surface = true;
  SetTexMods();
  Sys_UpdateWindows(W_ALL);
}

void CSurfaceDlg::OnBtnBrushfit() 
{
	// TODO: Add your control notification handler code here
	
}

void CSurfaceDlg::OnBtnFacefit() 
{
  UpdateData(TRUE);
  if (g_ptrSelectedFaces.GetSize() == 0)
  {
    brush_t *b;
		for (b=selected_brushes.next ; b != &selected_brushes ; b=b->next)
    {
      for (face_t* pFace = b->brush_faces; pFace; pFace = pFace->next)
      {
        g_ptrSelectedFaces.Add(pFace);
        g_ptrSelectedFaceBrushes.Add(b);
      }
    }
  }
  Select_FitTexture(m_nHeight, m_nWidth);
  SetTexMods();
	g_changed_surface = true;
  Sys_UpdateWindows(W_ALL);
}
