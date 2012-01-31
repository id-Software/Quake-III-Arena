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
#include "stdafx.h"
#include "qe3.h"
#include "PrefsDlg.h"

BOOL CALLBACK EditCommandDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char	key[1024];
	char	value[1024];
	char	*temp;
	int		index;
	HWND	hOwner;
	
	hOwner = GetParent (hwndDlg);

	switch (uMsg)
    {
	case WM_INITDIALOG:
		index = SendDlgItemMessage (hOwner, IDC_CMD_LIST, LB_GETCURSEL, 0, 0);
    if (index >= 0)
    {
		  SendDlgItemMessage(hOwner, IDC_CMD_LIST, LB_GETTEXT, index, (LPARAM) (LPCTSTR) key);
		  temp = ValueForKey (g_qeglobals.d_project_entity, key);
		  strcpy (value, temp);
		  SetDlgItemText(hwndDlg, IDC_CMDMENUTEXT, key);
		  SetDlgItemText(hwndDlg, IDC_CMDCOMMAND, value);
    }
		return FALSE;
		break;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
			case IDOK:
				if (!GetDlgItemText(hwndDlg, IDC_CMDMENUTEXT, key, 64))
				{
					Sys_Printf ("Command not added\n");
					return FALSE;
				}

				if (!GetDlgItemText(hwndDlg, IDC_CMDCOMMAND, value, 64))
				{
					Sys_Printf ("Command not added\n");
					return FALSE;
				}

				//if (key[0] == 'b' && key[1] == 's' && key[2] == 'p')
				//{
					SetKeyValue (g_qeglobals.d_project_entity, key, value);
					FillBSPMenu ();
				//}
				//else
				//	Sys_Printf ("BSP commands must be preceded by \"bsp\"");

				EndDialog(hwndDlg, 1);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}
	}
	return FALSE;
}

BOOL CALLBACK AddCommandDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char	key[64];
	char	value[128];

	switch (uMsg)
    {
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
			case IDOK:
				if (!GetDlgItemText(hwndDlg, IDC_CMDMENUTEXT, key, 64))
				{
					Sys_Printf ("Command not added\n");
					return FALSE;
				}

				if (!GetDlgItemText(hwndDlg, IDC_CMDCOMMAND, value, 64))
				{
					Sys_Printf ("Command not added\n");
					return FALSE;
				}

				if (key[0] == 'b' && key[1] == 's' && key[2] == 'p')
				{
					SetKeyValue (g_qeglobals.d_project_entity, key, value);
					FillBSPMenu ();
				}
				else
					Sys_Printf ("BSP commands must be preceded by \"bsp\"");

				EndDialog(hwndDlg, 1);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}
	}
	return FALSE;
}

void UpdateBSPCommandList (HWND hwndDlg)
{
	int			i;
	epair_t		*ep;

	SendDlgItemMessage(hwndDlg, IDC_CMD_LIST, LB_RESETCONTENT, 0 , 0);

	i = 0;
	for (ep = g_qeglobals.d_project_entity->epairs ; ep ; ep=ep->next)
	{
		if (ep->key[0] == 'b' && ep->key[1] == 's' && ep->key[2] == 'p')
		{
			SendDlgItemMessage(hwndDlg, IDC_CMD_LIST, LB_ADDSTRING, i , (LPARAM) ep->key);
			i++;
		}
	}
}


// FIXME: turn this into an MFC dialog
BOOL CALLBACK ProjectDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char		key[1024];
	char		value[1024];
	int			index;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_PRJBASEPATH, ValueForKey (g_qeglobals.d_project_entity, "basepath"));
		SetDlgItemText(hwndDlg, IDC_PRJMAPSPATH, ValueForKey (g_qeglobals.d_project_entity, "mapspath"));
		SetDlgItemText(hwndDlg, IDC_PRJRSHCMD, ValueForKey (g_qeglobals.d_project_entity, "rshcmd"));
		SetDlgItemText(hwndDlg, IDC_PRJREMOTEBASE, ValueForKey (g_qeglobals.d_project_entity, "remotebasepath"));
		SetDlgItemText(hwndDlg, IDC_PRJENTITYPATH, ValueForKey (g_qeglobals.d_project_entity, "entitypath"));
		SetDlgItemText(hwndDlg, IDC_PRJTEXPATH, ValueForKey (g_qeglobals.d_project_entity, "texturepath"));
		UpdateBSPCommandList (hwndDlg);
		// Timo
		// additional fields
		CheckDlgButton( hwndDlg, IDC_CHECK_BPRIMIT, (g_qeglobals.m_bBrushPrimitMode) ? BST_CHECKED : BST_UNCHECKED );
//		SendMessage( ::GetDlgItem( hwndDlg, IDC_CHECK_BPRIMIT ), BM_SETCHECK, (WPARAM) g_qeglobals.m_bBrushPrimitMode, 0 );
		return TRUE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
			case IDC_ADDCMD:
//				DialogBox(g_qeglobals.d_hInstance, (char *)IDD_ADDCMD, g_qeglobals.d_hwndMain, AddCommandDlgProc);
				DialogBox(g_qeglobals.d_hInstance, (char *)IDD_ADDCMD, hwndDlg, AddCommandDlgProc);
				UpdateBSPCommandList (hwndDlg);
				break;

			case IDC_EDITCMD:
//				DialogBox(g_qeglobals.d_hInstance, (char *)IDD_ADDCMD, g_qeglobals.d_hwndMain, EditCommandDlgProc);
				DialogBox(g_qeglobals.d_hInstance, (char *)IDD_ADDCMD, hwndDlg, EditCommandDlgProc);
				UpdateBSPCommandList (hwndDlg);
				break;

			case IDC_REMCMD:
				index = SendDlgItemMessage (hwndDlg, IDC_CMD_LIST, LB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hwndDlg, IDC_CMD_LIST, LB_GETTEXT, index, (LPARAM) (LPCTSTR) key);
				DeleteKey (g_qeglobals.d_project_entity, key);
				Sys_Printf ("Selected %d\n", index);
				UpdateBSPCommandList (hwndDlg);
				break;

			case IDOK:
				GetDlgItemText(hwndDlg, IDC_PRJBASEPATH, value, 1024);
				SetKeyValue (g_qeglobals.d_project_entity, "basepath", value);
				GetDlgItemText(hwndDlg, IDC_PRJMAPSPATH, value, 1024);
				SetKeyValue (g_qeglobals.d_project_entity, "mapspath", value);
				GetDlgItemText(hwndDlg, IDC_PRJRSHCMD, value, 1024);
				SetKeyValue (g_qeglobals.d_project_entity, "rshcmd", value);
				GetDlgItemText(hwndDlg, IDC_PRJREMOTEBASE, value, 1024);
				SetKeyValue (g_qeglobals.d_project_entity, "remotebasepath", value);
				GetDlgItemText(hwndDlg, IDC_PRJENTITYPATH, value, 1024);
				SetKeyValue (g_qeglobals.d_project_entity, "entitypath", value);
				GetDlgItemText(hwndDlg, IDC_PRJTEXPATH, value, 1024);
				SetKeyValue (g_qeglobals.d_project_entity, "texturepath", value);
				// Timo
				// read additional fields
				if ( IsDlgButtonChecked( hwndDlg, IDC_CHECK_BPRIMIT ) )
				{
					g_qeglobals.m_bBrushPrimitMode = TRUE;
				}
				else
				{
					g_qeglobals.m_bBrushPrimitMode = FALSE;
				}
				SetKeyValue ( g_qeglobals.d_project_entity, "brush_primit", ( g_qeglobals.m_bBrushPrimitMode ? "1" : "0" ) );

				EndDialog(hwndDlg, 1);
				QE_SaveProject(g_strProject);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}
	}
	return FALSE;
}

void DoProjectSettings()
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_PROJECT, g_qeglobals.d_hwndMain, ProjectDlgProc);
}



BOOL CALLBACK GammaDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char sz[256];

	switch (uMsg)
    {
	case WM_INITDIALOG:
		sprintf(sz, "%1.1f", g_qeglobals.d_savedinfo.fGamma);		
		SetWindowText(GetDlgItem(hwndDlg, IDC_G_EDIT), sz);
		return TRUE;
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		
		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_G_EDIT), sz, 255);
			g_qeglobals.d_savedinfo.fGamma = atof(sz);
			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}
	}
	return FALSE;
}



void DoGamma(void)
{
	if ( DialogBox(g_qeglobals.d_hInstance, (char *)IDD_GAMMA, g_qeglobals.d_hwndMain, GammaDlgProc))
	{
	}
}		

//================================================


void SelectBrush (int entitynum, int brushnum)
{
	entity_t	*e;
	brush_t		*b;
	int			i;

	if (entitynum == 0)
		e = world_entity;
	else
	{
		e = entities.next;
		while (--entitynum)
		{
			e=e->next;
			if (e == &entities)
			{
				Sys_Status ("No such entity.", 0);
				return;
			}
		}
	}

	b = e->brushes.onext;
	if (b == &e->brushes)
	{
		Sys_Status ("No such brush.", 0);
		return;
	}
	while (brushnum--)
	{
		b=b->onext;
		if (b == &e->brushes)
		{
			Sys_Status ("No such brush.", 0);
			return;
		}
	}

	Brush_RemoveFromList (b);
	Brush_AddToList (b, &selected_brushes);


	Sys_UpdateWindows (W_ALL);
	for (i=0 ; i<3 ; i++)
  {
    if (g_pParentWnd->GetXYWnd())
      g_pParentWnd->GetXYWnd()->GetOrigin()[i] = (b->mins[i] + b->maxs[i])/2;

    if (g_pParentWnd->GetXZWnd())
      g_pParentWnd->GetXZWnd()->GetOrigin()[i] = (b->mins[i] + b->maxs[i])/2;
    
    if (g_pParentWnd->GetYZWnd())
      g_pParentWnd->GetYZWnd()->GetOrigin()[i] = (b->mins[i] + b->maxs[i])/2;
  }

	Sys_Status ("Selected.", 0);
}

/*
=================
GetSelectionIndex
=================
*/
void GetSelectionIndex (int *ent, int *brush)
{
	brush_t		*b, *b2;
	entity_t	*entity;

	*ent = *brush = 0;

	b = selected_brushes.next;
	if (b == &selected_brushes)
		return;

	// find entity
	if (b->owner != world_entity)
	{
		(*ent)++;
		for (entity = entities.next ; entity != &entities 
			; entity=entity->next, (*ent)++)
		;
	}

	// find brush
	for (b2=b->owner->brushes.onext 
		; b2 != b && b2 != &b->owner->brushes
		; b2=b2->onext, (*brush)++)
	;
}

BOOL CALLBACK FindBrushDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char entstr[256];
	char brushstr[256];
	HWND	h;
	int		ent, brush;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		// set entity and brush number
		GetSelectionIndex (&ent, &brush);
		sprintf (entstr, "%i", ent);
		sprintf (brushstr, "%i", brush);
		SetWindowText(GetDlgItem(hwndDlg, IDC_FIND_ENTITY), entstr);
		SetWindowText(GetDlgItem(hwndDlg, IDC_FIND_BRUSH), brushstr);

		h = GetDlgItem(hwndDlg, IDC_FIND_ENTITY);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
			case IDOK:
				GetWindowText(GetDlgItem(hwndDlg, IDC_FIND_ENTITY), entstr, 255);
				GetWindowText(GetDlgItem(hwndDlg, IDC_FIND_BRUSH), brushstr, 255);
				SelectBrush (atoi(entstr), atoi(brushstr));
				EndDialog(hwndDlg, 1);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwndDlg, 0);
				return TRUE;
		}	
	}
	return FALSE;
}



void DoFind(void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_FINDBRUSH, g_qeglobals.d_hwndMain, FindBrushDlgProc);
}	
	
/*
===================================================

  ARBITRARY ROTATE

===================================================
*/


BOOL CALLBACK RotateDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char	str[256];
	HWND	h;
	float	v;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_FIND_ENTITY);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{ 
		
		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_ROTX), str, 255);
			v = atof(str);
			if (v)
				Select_RotateAxis (0, v);

			GetWindowText(GetDlgItem(hwndDlg, IDC_ROTY), str, 255);
			v = atof(str);
			if (v)
				Select_RotateAxis (1, v);

			GetWindowText(GetDlgItem(hwndDlg, IDC_ROTZ), str, 255);
			v = atof(str);
			if (v)
				Select_RotateAxis (2, v);

			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;
		}	
	}

	return FALSE;
}



void DoRotate(void)
{
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_ROTATE, g_qeglobals.d_hwndMain, RotateDlgProc);
}
		
/*
===================================================

  ARBITRARY SIDES

===================================================
*/

bool g_bDoCone = false;
bool g_bDoSphere = false;
BOOL CALLBACK SidesDlgProc (
    HWND hwndDlg,	// handle to dialog box
    UINT uMsg,	// message
    WPARAM wParam,	// first message parameter
    LPARAM lParam 	// second message parameter
   )
{
	char str[256];
	HWND	h;

	switch (uMsg)
    {
	case WM_INITDIALOG:
		h = GetDlgItem(hwndDlg, IDC_SIDES);
		SetFocus (h);
		return FALSE;

	case WM_COMMAND: 
		switch (LOWORD(wParam)) { 
		
		case IDOK:
			GetWindowText(GetDlgItem(hwndDlg, IDC_SIDES), str, 255);
      if (g_bDoCone)
			  Brush_MakeSidedCone(atoi(str));
      else if (g_bDoSphere)
			  Brush_MakeSidedSphere(atoi(str));
      else
			  Brush_MakeSided (atoi(str));

			EndDialog(hwndDlg, 1);
		break;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
		break;
	}	
	default:
		return FALSE;
	}
}


void DoSides(bool bCone, bool bSphere, bool bTorus)
{
  g_bDoCone = bCone;
  g_bDoSphere = bSphere;
  //g_bDoTorus = bTorus;
	DialogBox(g_qeglobals.d_hInstance, (char *)IDD_SIDES, g_qeglobals.d_hwndMain, SidesDlgProc);
}		


//======================================================================

/*
===================
DoAbout
===================
*/
BOOL CALLBACK AboutDlgProc( HWND hwndDlg,
						    UINT uMsg,
						    WPARAM wParam,
						    LPARAM lParam )
{
	switch (uMsg)
    {
	case WM_INITDIALOG:
		{
			char renderer[1024];
			char version[1024];
			char vendor[1024];
			char extensions[4096];

			sprintf( renderer, "Renderer:\t%s", qglGetString( GL_RENDERER ) );
			sprintf( version, "Version:\t\t%s", qglGetString( GL_VERSION ) );
			sprintf( vendor, "Vendor:\t\t%s", qglGetString( GL_VENDOR ) );
			sprintf( extensions, "%s", qglGetString( GL_EXTENSIONS ) );

			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLRENDERER ),   renderer );
			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLVERSION ),    version );
			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLVENDOR ),     vendor );
			SetWindowText( GetDlgItem( hwndDlg, IDC_ABOUT_GLEXTENSIONS ), extensions );

		}
		return TRUE;

	case WM_CLOSE:
		EndDialog( hwndDlg, 1 );
		return TRUE;

	case WM_COMMAND:
		if ( LOWORD( wParam ) == IDOK )
			EndDialog(hwndDlg, 1);
		return TRUE;
	}
	return FALSE;
}

void DoAbout(void)
{
	DialogBox( g_qeglobals.d_hInstance, ( char * ) IDD_ABOUT, g_qeglobals.d_hwndMain, AboutDlgProc );
}


