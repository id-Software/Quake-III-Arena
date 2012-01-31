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
// GroupDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "GroupDlg.h"
#include "NameDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IMG_PATCH 0
#define IMG_BRUSH 1
#define IMG_GROUP 2
#define IMG_ENTITY 3
#define IMG_ENTITYGROUP 4
#define IMG_MODEL 5
#define IMG_SCRIPT 6

// misc group support
#define MAX_GROUPS 4096
#define GROUP_DELIMETER '@'
#define GROUPNAME "QER_Group_%i"

CGroupDlg g_wndGroup;
CGroupDlg *g_pGroupDlg = &g_wndGroup;

// group_t are loaded / saved through "group_info" entities
// they hold epairs for group settings and additionnal access info (tree nodes)
group_t *g_pGroups = NULL;

void Group_Add(entity_t *e)
{
  group_t *g = (group_t*)qmalloc(sizeof(group_t));
  g->epairs = e->epairs;
  g->next = NULL;
  e->epairs = NULL;
  // create a new group node
  HTREEITEM hItem = g_wndGroup.m_wndTree.GetSelectedItem();
  TVINSERTSTRUCT tvInsert;
  memset(&tvInsert, 0, sizeof(TVINSERTSTRUCT));
  tvInsert.item.iImage = IMG_GROUP;
  tvInsert.item.iSelectedImage = tvInsert.item.iImage;
	//++timo wasat?
  // tvInsert.hParent = (hItem) ? hItem : m_hWorld;
  tvInsert.hParent = g_wndGroup.m_hWorld;
  tvInsert.hInsertAfter = NULL;
  tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
  char *pipo = ValueForKey(e->epairs, "group");
  tvInsert.item.pszText = _T(ValueForKey(g->epairs, "group"));
	g->itemOwner = g_wndGroup.m_wndTree.InsertItem(&tvInsert);
  g->next = g_pGroups;
	g_pGroups = g;
}

group_t* Group_Alloc(char *name)
{
  group_t *g = (group_t*)qmalloc(sizeof(group_t));
	SetKeyValue( g->epairs, "group", name );
  return g;
}

group_t* Group_ForName(const char * name)
{
	group_t *g = g_pGroups;
	while (g != NULL)
	{
		if (strcmp( ValueForKey(g->epairs,"group"), name ) == 0)
			break;
		g = g->next;
	}
	return g;
}

void Group_AddToItem(brush_t *b, HTREEITEM item)
{
  char cBuff[1024];
  int nImage = IMG_BRUSH;
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  const char *pName = NULL;
  const char *pNamed = Brush_GetKeyValue(b, "name");
 
  if (!b->owner || (b->owner == world_entity))
  {
    if (b->patchBrush) 
    {
      pName = "Generic Patch";
      nImage = IMG_PATCH;
    } 
    else 
    {
      pName = "Generic Brush";
      nImage = IMG_BRUSH;
    }
  } 
  else 
  {
    pName = b->owner->eclass->name;
    if (b->owner->eclass->fixedsize) 
    {
      nImage = IMG_ENTITY;
    } 
    else 
    {
      nImage = IMG_ENTITYGROUP;
    }
  }

  strcpy(cBuff, pName);

  TVINSERTSTRUCT tvInsert;
  memset(&tvInsert, 0, sizeof(TVINSERTSTRUCT));
  tvInsert.item.iImage = (b->patchBrush) ? IMG_PATCH : IMG_BRUSH;
  tvInsert.item.iSelectedImage = tvInsert.item.iImage;
  tvInsert.hParent = item;
  tvInsert.hInsertAfter = NULL;
  tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
  tvInsert.item.pszText = cBuff;
  HTREEITEM itemNew = g_pGroupDlg->m_wndTree.InsertItem(&tvInsert);
  g_pGroupDlg->m_wndTree.SetItemData(itemNew, reinterpret_cast<DWORD>(b));
  b->itemOwner = itemNew;
  g_pGroupDlg->m_wndTree.RedrawWindow();

}

void Group_RemoveBrush(brush_t *b)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
	{
		return;
	}
	if (b->itemOwner)
	{
		g_pGroupDlg->m_wndTree.DeleteItem(b->itemOwner);
		b->itemOwner = NULL;
		g_pGroupDlg->m_wndTree.RedrawWindow();
	}
	DeleteKey(b->epairs, "group");
}

void Group_AddToWorld(brush_t *b)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  HTREEITEM itemParent = g_pGroupDlg->m_wndTree.GetRootItem();
  Group_AddToItem(b, itemParent);
}

void Group_AddToProperGroup(brush_t *b)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
	{
		return;
	}
	// NOTE: we do a local copy of the "group" key because it gets erased by Group_RemoveBrush
	const char *pGroup = Brush_GetKeyValue(b, "group");
  // remove the entry in the tree if there's one
  if (b->itemOwner)
  {
		g_pGroupDlg->m_wndTree.DeleteItem(b->itemOwner);
		b->itemOwner = NULL;
		g_pGroupDlg->m_wndTree.RedrawWindow();
  }

	if (*pGroup != 0)
	{
		// find the item
		group_t *g = Group_ForName(pGroup);
		if (g)
			Group_AddToItem(b, g->itemOwner);
#ifdef _DEBUG
		else
			Sys_Printf("WARNING: unexpected Group_ForName not found in Group_AddToProperGroup\n");
#endif
	}
	else
	{
		Group_AddToWorld(b);
	}
}

void Group_AddToSelected(brush_t *b)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  HTREEITEM hItem = g_pGroupDlg->m_wndTree.GetSelectedItem();
  if (hItem == NULL)
  {
    hItem = g_pGroupDlg->m_wndTree.GetRootItem();
  }
  Group_AddToItem(b, hItem);
}

void Group_Save(FILE *f)
{
	group_t *g = g_pGroups;
	while (g)
  {
    fprintf(f,"{\n\"classname\" \"group_info\"\n\"group\" \"%s\"\n}\n", ValueForKey( g->epairs, "group" ));
    g = g->next;
  }
}

void Group_Init()
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
	// start by cleaning everything
  // clean the groups
  //++timo FIXME: we leak, delete the groups on the way (I don't have time to do it now)
#ifdef _DEBUG
  Sys_Printf("TODO: fix leak in Group_Init\n");
#endif
  group_t *g = g_pGroups;
  while (g)
  {
    epair_t *ep,*enext;
	  for (ep = g->epairs ; ep ; ep=enext )
	  {
		  enext = ep->next;
		  free (ep->key);
		  free (ep->value);
		  free (ep);
	  }
    g = g->next;
  }
  g_pGroups = NULL;
	g_wndGroup.m_wndTree.DeleteAllItems();
  TVINSERTSTRUCT tvInsert;
  memset(&tvInsert, 0, sizeof(TVINSERTSTRUCT));
  tvInsert.hParent = NULL;
  tvInsert.hInsertAfter = NULL;
  tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
  tvInsert.item.pszText = _T("World");
  tvInsert.item.iImage = IMG_GROUP;
  tvInsert.item.iSelectedImage = IMG_GROUP;
  HTREEITEM hWorld = g_wndGroup.m_wndTree.InsertItem(&tvInsert);
	// walk through all the brushes, remove the itemOwner key and add them back where they belong
	brush_t *b;
	for (b = active_brushes.next; b != &active_brushes; b = b->next)
	{
		b->itemOwner = NULL;
		Group_AddToProperGroup(b);
	}
	for (b = selected_brushes.next ; b != &selected_brushes ; b = b->next)
	{
		b->itemOwner = NULL;
		Group_AddToProperGroup(b);
	}
}

// scan through world_entity for groups in this map?
// we use GROUPNAME "QER_group_%i" to look for existing groups and their naming
//++timo FIXME: is this actually needed for anything?
void Group_GetListFromWorld(CStringArray *pArray)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }

  if (world_entity == NULL)
  {
    return;
  }

  pArray->RemoveAll();
  char cBuff[1024];
  for (int i =0; i < MAX_GROUPS; i++)
  {
    sprintf(cBuff, GROUPNAME, i);
    char *pGroup = ValueForKey(world_entity, cBuff);
    if (pGroup && strlen(pGroup) > 0)
    {
      pArray->Add(pGroup);
    }
    else
    {
      break;
    }
  }
}

void Group_RemoveListFromWorld()
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  CStringArray array;
  Group_GetListFromWorld(&array);
  int nCount = array.GetSize();
  for (int i = 0; i < nCount; i++)
  {
    DeleteKey(world_entity, array.GetAt(i));
  }
}

/*
void Group_SetListToWorld(CStringArray *pArray)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  char cBuff[1024];
  Group_RemoveListFromWorld();
  int nCount = pArray->GetSize();
  for (int i = 0; i < nCount; i++)
  {
    sprintf(cBuff, GROUPNAME, i);
    SetKeyValue(world_entity, cBuff, pArray->GetAt(i));
  }
}
*/

int CountChar(const char *p, char c)
{
  int nCount = 0;
  int nLen = strlen(p)-1;
  while (nLen-- >= 0)
  {
    if (p[nLen] == c)
    {
      nCount++;
    }
  }
  return nCount;
}

/*
// this is not very efficient but should get the job done
// as our trees should never be too big
void Group_BuildTree(CTreeCtrl *pTree)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  CStringArray array;
  int i;
  CString strTemp;
  CString strRight;

  //++timo WARNING: this is very dangerous! delete all tree items, without checking the brushes
  pTree->DeleteAllItems();
  TVINSERTSTRUCT tvInsert;
  memset(&tvInsert, 0, sizeof(TVINSERTSTRUCT));
  tvInsert.hParent = NULL;
  tvInsert.hInsertAfter = NULL;
  tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
  tvInsert.item.pszText = _T("World");
  tvInsert.item.iImage = IMG_GROUP;
  tvInsert.item.iSelectedImage = IMG_GROUP;
  HTREEITEM hWorld = pTree->InsertItem(&tvInsert);

  Group_GetListFromWorld(&array);

  // groups use @ to delimit levels, the number of @ signs
  // start with ROOT item
  // nothing is a peer with world, it is ancestor to everything 
  // expects things in order so first entry should be a 2nd level item
  HTREEITEM itemParent = pTree->GetRootItem();
  HTREEITEM itemLast = itemParent;
  int nCount = array.GetSize();
  int nLastLevel = 1;
  for (i = 0; i < nCount; i++)
  {
    strTemp = array.GetAt(i);
    int nLevel = CountChar(strTemp, GROUP_DELIMETER);
    if (nLevel < nLastLevel)
    {
      int nLevelsUp = nLastLevel - nLevel;
      while (nLevelsUp-- > 0)
      {
        itemParent = pTree->GetParentItem(itemParent);
      }
    }
    else if (nLevel > nLastLevel)
    {
      itemParent = itemLast;
    }
    nLastLevel = nLevel;
    char *pLast = strrchr(strTemp, GROUP_DELIMETER);
    pLast++;
    itemLast = pTree->InsertItem(pLast, itemParent);
  }
}
*/

void DecomposeSiblingList(const char *p, CStringArray *pArray, CTreeCtrl *pTree, HTREEITEM itemChild)
{
  CString str = p;
  str += GROUP_DELIMETER;
  while (itemChild)
  {
    CString strAdd = str;
    strAdd += pTree->GetItemText(itemChild);
    // do not want to add brushes or things, just groups 
    if (pTree->GetItemData(itemChild) == 0)
    {
      pArray->Add(strAdd);
    }
    if (pTree->ItemHasChildren(itemChild))
    {
      HTREEITEM itemOffspring = pTree->GetChildItem(itemChild);
      DecomposeSiblingList(strAdd, pArray, pTree, itemOffspring); 
    }
    itemChild = pTree->GetNextSiblingItem(itemChild);
  }
}

/*
void Group_DecomposeTree(CTreeCtrl *pTree)
{
	if (!g_qeglobals.m_bBrushPrimitMode)
  {
    return;
  }
  CStringArray array;
  HTREEITEM itemParent = pTree->GetRootItem();
  if (pTree->ItemHasChildren(itemParent))
  {
    HTREEITEM itemChild = pTree->GetChildItem(itemParent);
    DecomposeSiblingList(pTree->GetItemText(itemParent), &array, pTree, itemChild);
  }
  Group_SetListToWorld(&array);
}
*/

/////////////////////////////////////////////////////////////////////////////
// CGroupDlg dialog


CGroupDlg::CGroupDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CGroupDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupDlg)
	DDX_Control(pDX, IDC_TREE_GROUP, m_wndTree);
	DDX_Control(pDX, IDC_BTN_EDIT, m_wndEdit);
	DDX_Control(pDX, IDC_BTN_DEL, m_wndDel);
	DDX_Control(pDX, IDC_BTN_ADD, m_wndAdd);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupDlg, CDialog)
	//{{AFX_MSG_MAP(CGroupDlg)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BTN_ADD, OnBtnAdd)
	ON_BN_CLICKED(IDC_BTN_DEL, OnBtnDel)
	ON_BN_CLICKED(IDC_BTN_EDIT, OnBtnEdit)
	ON_NOTIFY(NM_RCLICK, IDC_TREE_GROUP, OnRclickTreeGroup)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE_GROUP, OnEndlabeleditTreeGroup)
	ON_NOTIFY(NM_CLICK, IDC_TREE_GROUP, OnClickTreeGroup)
	ON_NOTIFY(TVN_SETDISPINFO, IDC_TREE_GROUP, OnSetdispinfoTreeGroup)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE_GROUP, OnBegindragTreeGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupDlg message handlers

void CGroupDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	CRect rct;
  GetClientRect(rct);

  if ( m_wndAdd.GetSafeHwnd())
  {
    //all borders at 4, spacing at 6
    CRect rctButton;
    m_wndAdd.GetWindowRect(rctButton);
    int nWidth = rctButton.Width();
    int nHeight = rctButton.Height();

    int nTop = rct.Height() - nHeight - 4;

    m_wndAdd.SetWindowPos(NULL, 4, nTop, 0, 0, SWP_NOSIZE);
    m_wndEdit.SetWindowPos(NULL, 8 + nWidth , nTop, 0, 0, SWP_NOSIZE);
    m_wndDel.SetWindowPos(NULL, 12 + (nWidth * 2), nTop, 0, 0, SWP_NOSIZE);
    rct.bottom = nTop;
    m_wndTree.SetWindowPos(NULL, rct.left + 4, rct.top + 4, rct.Width() - 8, rct.Height() - 8, SWP_SHOWWINDOW);
  }
}

BOOL CGroupDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_imgList.Create(IDB_BITMAP_GROUPS, 16, 0, ILC_COLOR);
	m_wndTree.SetImageList(&m_imgList, TVSIL_NORMAL);
	InitGroups();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGroupDlg::InitGroups()
{
	Group_Init();
}

// add a new group, put all selected brushes into the group
void CGroupDlg::OnBtnAdd() 
{
  CNameDlg dlg("New Group", this);
  if (dlg.DoModal() == IDOK)
  {
		// create a new group node
    HTREEITEM hItem = m_wndTree.GetSelectedItem();
    TVINSERTSTRUCT tvInsert;
    memset(&tvInsert, 0, sizeof(TVINSERTSTRUCT));
    tvInsert.item.iImage = IMG_GROUP;
    tvInsert.item.iSelectedImage = tvInsert.item.iImage;
		//++timo wasat?
    // tvInsert.hParent = (hItem) ? hItem : m_hWorld;
    tvInsert.hParent = m_hWorld;
    tvInsert.hInsertAfter = NULL;
    tvInsert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvInsert.item.pszText = _T(dlg.m_strName.GetBuffer(0));
		// create a new group
		group_t *g = Group_Alloc( dlg.m_strName.GetBuffer(0) );
		g->itemOwner = m_wndTree.InsertItem(&tvInsert);
		g->next = g_pGroups;
		g_pGroups = g;
		// now add the selected brushes
		// NOTE: it would be much faster to give the group_t for adding
		// but Select_AddToGroup is the standard way for all other cases
		Select_AddToGroup( dlg.m_strName.GetBuffer(0) );
  }
}

void CGroupDlg::OnBtnDel() 
{
}

void CGroupDlg::OnBtnEdit() 
{
}

BOOL CGroupDlg::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	return CDialog::OnChildNotify(message, wParam, lParam, pLResult);
}

BOOL CGroupDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
  return CDialog::OnNotify(wParam, lParam, pResult);
}

void CGroupDlg::OnRclickTreeGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CGroupDlg::OnEndlabeleditTreeGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
  const char *pText = pTVDispInfo->item.pszText;
  if (pText && strlen(pText) > 0)
  {
    HTREEITEM item = pTVDispInfo->item.hItem;
    if (m_wndTree.GetRootItem() != item)
    {
      m_wndTree.SetItemText(item, pText);
      if (pTVDispInfo->item.iImage != IMG_GROUP)
      {
        // if it is an entity
      }
    }
    else
    {
      Sys_Printf("Cannot rename the world\n");
    }
  }
  m_wndTree.RedrawWindow();
	*pResult = 0;
}

void CGroupDlg::OnClickTreeGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CGroupDlg::OnSetdispinfoTreeGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CGroupDlg::OnCancel()
{
  TreeView_EndEditLabelNow(m_wndTree.GetSafeHwnd(), TRUE);
}

void CGroupDlg::OnOK()
{
  TreeView_EndEditLabelNow(m_wndTree.GetSafeHwnd(), FALSE);
}

void CGroupDlg::OnBegindragTreeGroup(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}
