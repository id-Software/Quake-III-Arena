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
// EntityListDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Radiant.h"
#include "EntityListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEntityListDlg dialog


CEntityListDlg::CEntityListDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEntityListDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEntityListDlg)
	//}}AFX_DATA_INIT
}


void CEntityListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEntityListDlg)
	DDX_Control(pDX, IDC_LIST_ENTITY, m_lstEntity);
	DDX_Control(pDX, IDC_TREE_ENTITY, m_treeEntity);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEntityListDlg, CDialog)
	//{{AFX_MSG_MAP(CEntityListDlg)
	ON_BN_CLICKED(IDSELECT, OnSelect)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_ENTITY, OnSelchangedTreeEntity)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_ENTITY, OnDblclkTreeEntity)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntityListDlg message handlers

void CEntityListDlg::OnSelect() 
{
  HTREEITEM hItem = m_treeEntity.GetSelectedItem();
  if (hItem)
  {
    entity_t* pEntity = reinterpret_cast<entity_t*>(m_treeEntity.GetItemData(hItem));
    if (pEntity)
    {
      Select_Deselect();
	    Select_Brush (pEntity->brushes.onext);
    }
	}
  Sys_UpdateWindows(W_ALL);
}

BOOL CEntityListDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  CMapStringToPtr mapEntity;

  HTREEITEM hParent = m_treeEntity.InsertItem(world_entity->eclass->name);
  HTREEITEM hChild = m_treeEntity.InsertItem(world_entity->eclass->name, hParent);
  m_treeEntity.SetItemData(hChild, reinterpret_cast<DWORD>(world_entity));

	for (entity_t* pEntity=entities.next ; pEntity != &entities ; pEntity=pEntity->next)
	{
    hParent = NULL;
    if (mapEntity.Lookup(pEntity->eclass->name, reinterpret_cast<void*&>(hParent)) == FALSE)
    {
      hParent = m_treeEntity.InsertItem(pEntity->eclass->name);
      mapEntity.SetAt(pEntity->eclass->name, reinterpret_cast<void*>(hParent));
    }
    hChild = m_treeEntity.InsertItem(pEntity->eclass->name, hParent);
    m_treeEntity.SetItemData(hChild, reinterpret_cast<DWORD>(pEntity));
  }

  CRect rct;
  m_lstEntity.GetClientRect(rct);
  m_lstEntity.InsertColumn(0, "Key", LVCFMT_LEFT, rct.Width() / 2);
  m_lstEntity.InsertColumn(1, "Value", LVCFMT_LEFT, rct.Width() / 2);
  m_lstEntity.DeleteColumn(2);
  UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEntityListDlg::OnSelchangedTreeEntity(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
  HTREEITEM hItem = m_treeEntity.GetSelectedItem();
  m_lstEntity.DeleteAllItems();
  if (hItem)
  {
    CString strList;
    entity_t* pEntity = reinterpret_cast<entity_t*>(m_treeEntity.GetItemData(hItem));
    if (pEntity)
    {
	    for (epair_t* pEpair = pEntity->epairs ; pEpair ; pEpair = pEpair->next)
      {
		    if (strlen(pEpair->key) > 8)
          strList.Format("%s\t%s", pEpair->key, pEpair->value);
        else
          strList.Format("%s\t\t%s", pEpair->key, pEpair->value);
        int nParent = m_lstEntity.InsertItem(0, pEpair->key);
        m_lstEntity.SetItem(nParent, 1, LVIF_TEXT, pEpair->value, 0, 0, 0, reinterpret_cast<DWORD>(pEntity));

      }
    }
	}
	*pResult = 0;
}

void CEntityListDlg::OnDblclkListInfo() 
{
	// TODO: Add your control notification handler code here
	
}

void CEntityListDlg::OnDblclkTreeEntity(NMHDR* pNMHDR, LRESULT* pResult) 
{
  OnSelect();
	*pResult = 0;
}
