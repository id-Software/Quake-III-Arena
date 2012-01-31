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
// PlugInManager.cpp: implementation of the CPlugInManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "io.h"
#include "Radiant.h"
#include "PlugInManager.h"
#include "PlugIn.h"
#include "DialogInfo.h"
#include "pakstuff.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlugInManager::CPlugInManager()
{
  m_pTexturePlug = NULL;
  m_pSurfaceListPlug = NULL;
  PatchesMode = EActivePatches;
}

CPlugInManager::~CPlugInManager()
{
  Cleanup();
}

void CPlugInManager::Init(const char * pPath)
{
	Cleanup();

	// set some globals
	g_qeglobals.bSurfacePropertiesPlugin = false;
	g_qeglobals.bBSPFrontendPlugin = false;
	
	CString strPath(pPath);
	strPath += "*.dll";
	
	bool bGo = true;
	
	struct _finddata_t fileinfo;
	int handle = _findfirst (strPath, &fileinfo);
	if (handle != -1)
	{
		do
		{
			strPath.Format("%s\\%s", pPath, fileinfo.name);
			CPlugIn *pPlug = new CPlugIn();
			if (pPlug->load(strPath))
			{
				if(FillFuncTable(pPlug))		// PGM
				{
					m_PlugIns.Add(pPlug);

					pPlug->RegisterPluginEntities();

					// if this thing handles surface properties
					pPlug->InitSurfacePlugin();
					
					// will test and init if it's a BSP frontend
					pPlug->InitBSPFrontendPlugin();

					g_pParentWnd->AddPlugInMenuItem(pPlug);
					
					// if this thing handles textures
					if (pPlug->getTextureInfo() != NULL)
					{
						this->m_pTexturePlug = pPlug;
						
						// if this is a wad style texture extension, have it load everything now
						if (pPlug->getTextureInfo()->m_bWadStyle)
						{
							CString strPath = ValueForKey(g_qeglobals.d_project_entity, "texturepath");
							pPlug->loadTexture(strPath);
						}
					}
					
					if (pPlug->getSurfaceFlags() != NULL)
					{
						this->m_pSurfaceListPlug = pPlug;
					}
				}
				else
				{
					delete pPlug;				// PGM
				}
			}
			else
			{
				delete pPlug;
			}
		} while (_findnext( handle, &fileinfo ) != -1);
		_findclose (handle);
	}
	
}

void CPlugInManager::Cleanup()
{
	int i;
	for (i = 0; i < m_PlugIns.GetSize(); i++)
	{
		CPlugIn *plug = reinterpret_cast<CPlugIn*>(m_PlugIns.GetAt(i));
		plug->free();
		delete plug;
	}
	m_PlugIns.RemoveAll();
	
	for (i = 0; i < m_BrushHandles.GetSize(); i++)
	{
		brush_t *pb = reinterpret_cast<brush_t*>(m_BrushHandles.GetAt(i));
		Brush_Free(pb);
	}
	m_BrushHandles.RemoveAll();
	
	for (i = 0; i < m_EntityHandles.GetSize(); i++)
	{
		entity_t *pe = reinterpret_cast<entity_t*>(m_EntityHandles.GetAt(i));
		Entity_Free(pe);
	}
	m_EntityHandles.RemoveAll();
	
	// patches
	// these are linked into the map
	m_PatchesHandles.RemoveAll();
	// these patches were allocated by Radiant on plugin request
	// if the list is not empty, it means either the plugin asked for allocation and never commited them to the map
	// in which case we are supposed to delete them
	// or it commited them but never called m_pfnReleasePatchHandles, in case the patches may have already been
	// erased and we are trying a second time, therefore crashing ..
	//++timo FIXME: for now I leave a leak warning, we'd need a table to keep track of commited patches
#ifdef _DEBUG
	if (m_PluginPatches.GetSize() != 0)
		Sys_Printf("WARNING: m_PluginPatches.GetSize() != 0 in CPlugInManager::Cleanup, possible leak\n");
#endif
/*	for (i = 0; i < m_PluginPatches.GetSize(); i++)
	{
		patchMesh_t *pMesh = reinterpret_cast<patchMesh_t*>(m_PluginPatches.GetAt(i));
		if (pMesh->pSymbiot)
			delete pMesh;
	}
	m_PluginPatches.RemoveAll(); */
}

void CPlugInManager::Dispatch(int n, const char * p)
{
  for (int i = 0; i < m_PlugIns.GetSize(); i++)
  {
    CPlugIn *plug = reinterpret_cast<CPlugIn*>(m_PlugIns.GetAt(i));
    if (plug->ownsCommandID(n))
    {
      vec3_t vMin, vMax;
    	if (selected_brushes.next == &selected_brushes)
      {
        vMin[0] = vMin[1] = vMin[2] = 0;
        VectorCopy(vMin, vMax);
      }
      else
      {
        Select_GetBounds (vMin, vMax);
      }
      plug->dispatchCommand(p, vMin, vMax, QE_SingleBrush(true));	// PGM -- added quiet
      break;
    }
  }
}

// creates a dummy brush in the active brushes list
// FIXME : is this one really USED ?
void WINAPI QERApp_CreateBrush(vec3_t vMin, vec3_t vMax)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	brush_t* pBrush = Brush_Create(vMin, vMax, &g_qeglobals.d_texturewin.texdef);
	Entity_LinkBrush (world_entity, pBrush);
	Brush_Build(pBrush);
	Brush_AddToList (pBrush, &active_brushes);
	Select_Brush(pBrush);
	Sys_UpdateWindows(W_ALL);
}

LPVOID CPlugInManager::CreateBrushHandle()
{
	brush_t *pb = Brush_Alloc();
	pb->numberId = g_nBrushId++;
  m_BrushHandles.Add(pb);
  return (LPVOID)pb;
}

void CPlugInManager::DeleteBrushHandle(void * vp)
{
  CPtrArray* pHandles[3];
  pHandles[0] = &m_SelectedBrushHandles;
  pHandles[1] = &m_ActiveBrushHandles;
  pHandles[2] = &m_BrushHandles;

  for (int j = 0; j < 3; j++)
  {
    for (int i = 0; i < pHandles[j]->GetSize(); i++)
    {
      brush_t *pb = reinterpret_cast<brush_t*>(pHandles[j]->GetAt(i));
      if (pb == reinterpret_cast<brush_t*>(vp))
      {
        if (j == 2)
        {
          // only remove it from the list if it is work area
          // this allows the selected and active list indexes to remain constant
          // throughout a session (i.e. between an allocate and release)
          pHandles[j]->RemoveAt(i);
        }
        Brush_Free(pb);
		Sys_MarkMapModified();		// PGM
        return;
      }
    }
  }
}

void CPlugInManager::CommitBrushHandleToMap(void * vp)
{
  g_bScreenUpdates = false; 
  for (int i = 0; i < m_BrushHandles.GetSize(); i++)
  {
    brush_t *pb = reinterpret_cast<brush_t*>(m_BrushHandles.GetAt(i));
    if (pb == reinterpret_cast<brush_t*>(vp))
    {
      m_BrushHandles.RemoveAt(i);
	  Entity_LinkBrush (world_entity, pb);
      Brush_Build(pb);
	  Brush_AddToList (pb, &active_brushes);
      Select_Brush(pb);
    }
  }
  g_bScreenUpdates = true; 
  Sys_UpdateWindows(W_ALL);
}

void CPlugInManager::AddFaceToBrushHandle(void * vp, vec3_t v1, vec3_t v2, vec3_t v3)
{
  brush_t *bp = FindBrushHandle(vp);
  if (bp != NULL)
  {
		face_t *f = Face_Alloc();
		f->texdef = g_qeglobals.d_texturewin.texdef;
		f->texdef.flags &= ~SURF_KEEP;
		f->texdef.contents &= ~CONTENTS_KEEP;
		f->next = bp->brush_faces;
		bp->brush_faces = f;
		VectorCopy (v1, f->planepts[0]);
		VectorCopy (v2, f->planepts[1]);
		VectorCopy (v3, f->planepts[2]);
  }
}

brush_t* CPlugInManager::FindBrushHandle(void * vp)
{
  CPtrArray* pHandles[4];
  pHandles[0] = &m_SelectedBrushHandles;
  pHandles[1] = &m_ActiveBrushHandles;
  pHandles[2] = &m_BrushHandles;
  pHandles[3] = &m_EntityBrushHandles;

  for (int j = 0; j < 4; j++)
  {
    for (int i = 0; i < pHandles[j]->GetSize(); i++)
    {
      brush_t *pb = reinterpret_cast<brush_t*>(pHandles[j]->GetAt(i));
      if (pb == reinterpret_cast<brush_t*>(vp))
      {
        return pb;
      }
    }
  }
  return NULL;
}

patchMesh_t* CPlugInManager::FindPatchHandle(int index)
{
	switch (PatchesMode)
	{
	case EActivePatches:
	case ESelectedPatches:
		if ( index < m_PatchesHandles.GetSize() )
		{
			brush_t *pb = reinterpret_cast<brush_t *>(m_PatchesHandles.GetAt(index));
			return pb->pPatch;
		}
#ifdef _DEBUG
		Sys_Printf("WARNING: out of bounds in CPlugInManager::FindPatchHandle\n");
#endif
		break;
	case EAllocatedPatches:
		if ( index < m_PluginPatches.GetSize() )
		{
			patchMesh_t *pPatch = reinterpret_cast<patchMesh_t *>(m_PluginPatches.GetAt(index));
			return pPatch;
		}
#ifdef _DEBUG
		Sys_Printf("WARNING: out of bounds in CPlugInManager::FindPatchHandle\n");
#endif
		break;
	}
	return NULL;
}

LPVOID WINAPI QERApp_CreateBrushHandle()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  return g_pParentWnd->GetPlugInMgr().CreateBrushHandle();
}

void WINAPI QERApp_DeleteBrushHandle(LPVOID vp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  g_pParentWnd->GetPlugInMgr().DeleteBrushHandle(vp);
}

void WINAPI QERApp_CommitBrushHandleToMap(LPVOID vp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  g_pParentWnd->GetPlugInMgr().CommitBrushHandleToMap(vp);
}

void WINAPI QERApp_AddFace(LPVOID vp, vec3_t v1, vec3_t v2, vec3_t v3)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  g_pParentWnd->GetPlugInMgr().AddFaceToBrushHandle(vp, v1, v2, v3);
}

void WINAPI QERApp_DeleteSelection()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  Select_Delete();
}

void WINAPI QERApp_SysMsg(LPCSTR pMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString str = pMsg;
  Sys_Printf(str.GetBuffer(0));
}

// NOTE: called only in case of plugin error. We can try a plugin refresh instead of a straight crash ?
void WINAPI QERApp_ErrorMsg(LPCSTR pMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString str = pMsg;
	Error(str.GetBuffer(0));
}

void WINAPI QERApp_InfoMsg(LPCSTR pMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  ShowInfoDialog(pMsg);
}

void WINAPI QERApp_HideInfoMsg()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  HideInfoDialog();
}

//=====
//PGM
void WINAPI QERApp_PositionView(vec3_t	v1, vec3_t v2)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_pParentWnd->ActiveXY()->SetOrigin(v1);
	// FIXME - implement this!
	Sys_UpdateWindows(W_ALL);
}
//PGM
//=====

//FIXME: this AcquirePath stuff is pretty much a mess and needs cleaned up
bool g_bPlugWait = false;
bool g_bPlugOK = false;
int  g_nPlugCount = 0;

void _PlugDone(bool b, int n)
{
  g_bPlugWait = false;
  g_bPlugOK = b;
  g_nPlugCount = n;
}

void WINAPI QERApp_GetPoints(int nMax, _QERPointData *pData, LPCSTR pMsg)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  ShowInfoDialog(pMsg);
  g_bPlugWait = true;
  g_bPlugOK = false;
  g_nPlugCount = 0;
//  g_nPlugCount=nMax-1;
  AcquirePath(nMax, &_PlugDone);
  while (g_bPlugWait)
  {
    MSG msg;
    if (::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) 
    { 
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  HideInfoDialog();
  
  pData->m_nCount = 0;
  pData->m_pVectors = NULL;

  if (g_bPlugOK && g_nPlugCount > 0)
  {
    pData->m_nCount = g_nPlugCount;
    pData->m_pVectors = reinterpret_cast<vec3_t*>(qmalloc(g_nPlugCount * sizeof(vec3_t)));
    vec3_t *pOut = pData->m_pVectors;
    for (int i = 0; i < g_nPlugCount; i++)
    {
	memcpy(pOut, &g_PathPoints[i],sizeof(vec3_t));
	pOut++;
    }
  }
}

void WINAPI QERApp_AddFaceData(LPVOID pv, _QERFaceData *pData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	brush_t* pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
  if (pBrush != NULL)
  {
		face_t *f = Face_Alloc();
		f->texdef = g_qeglobals.d_texturewin.texdef;
		f->texdef.flags = pData->m_nFlags;
    f->texdef.contents = pData->m_nContents;
    f->texdef.value = pData->m_nValue;
    f->texdef.rotate = pData->m_fRotate;
    f->texdef.shift[0] = pData->m_fShift[0];
    f->texdef.shift[1] = pData->m_fShift[1];
    f->texdef.scale[0] = pData->m_fScale[0];
    f->texdef.scale[1] = pData->m_fScale[1];
    //strcpy(f->texdef.name, pData->m_TextureName);
    f->texdef.SetName(pData->m_TextureName);
		f->next = pBrush->brush_faces;
		pBrush->brush_faces = f;
		VectorCopy (pData->m_v1, f->planepts[0]);
		VectorCopy (pData->m_v2, f->planepts[1]);
		VectorCopy (pData->m_v3, f->planepts[2]);
		Sys_MarkMapModified();		// PGM
  }
}

int WINAPI QERApp_GetFaceCount(LPVOID pv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
  brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
  if (pBrush != NULL)
  {
	  for (face_t *f = pBrush->brush_faces ; f; f = f->next)
    {
      n++;
    }
  }
  return n;
}

_QERFaceData* WINAPI QERApp_GetFaceData(LPVOID pv, int nFaceIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  static _QERFaceData face;
  int n = 0;
  brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);

  if (pBrush != NULL)
  {
    for (face_t *f = pBrush->brush_faces ; f; f = f->next)
    {

#ifdef _DEBUG
		if (!pBrush->brush_faces)
		{
			Sys_Printf( "Warning : pBrush->brush_faces is NULL in QERApp_GetFaceData\n" );
			return NULL;
		}
#endif

      if (n == nFaceIndex)
      {
        face.m_nContents = f->texdef.contents;
        face.m_nFlags = f->texdef.flags;
        face.m_fRotate = f->texdef.rotate;
        face.m_fScale[0] = f->texdef.scale[0];
        face.m_fScale[1] = f->texdef.scale[1];
        face.m_fShift[0] = f->texdef.shift[0];
        face.m_fShift[1] = f->texdef.shift[1];
        face.m_nValue = f->texdef.value;
        strcpy(face.m_TextureName, f->texdef.name);
        VectorCopy(f->planepts[0], face.m_v1);
        VectorCopy(f->planepts[1], face.m_v2);
        VectorCopy(f->planepts[2], face.m_v3);
        return &face;
      }
      n++;
    }
  }
  return NULL;
}

void WINAPI QERApp_SetFaceData(LPVOID pv, int nFaceIndex, _QERFaceData *pData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int n = 0;
	brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);

	if (pBrush != NULL)
	{
		for (face_t *f = pBrush->brush_faces ; f; f = f->next)
		{
			if (n == nFaceIndex)
			{
				f->texdef.flags = pData->m_nFlags;
				f->texdef.contents = pData->m_nContents;
				f->texdef.value = pData->m_nValue;
				f->texdef.rotate = pData->m_fRotate;
				f->texdef.shift[0] = pData->m_fShift[0];
				f->texdef.shift[1] = pData->m_fShift[1];
				f->texdef.scale[0] = pData->m_fScale[0];
				f->texdef.scale[1] = pData->m_fScale[1];
				//strcpy(f->texdef.name, pData->m_TextureName);
				f->texdef.SetName(pData->m_TextureName);
				VectorCopy(pData->m_v1, f->planepts[0]);
				VectorCopy(pData->m_v2, f->planepts[1]);
				VectorCopy(pData->m_v3, f->planepts[2]);
				Sys_MarkMapModified();		// PGM
				return;						// PGM
			}
			n++;
		}
	}
}

void WINAPI QERApp_DeleteFace(LPVOID pv, int nFaceIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
  brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
  if (pBrush != NULL)
  {
    face_t *pPrev = pBrush->brush_faces;
	  for (face_t *f = pBrush->brush_faces; f; f = f->next)
    {
      if (n == nFaceIndex)
      {
        pPrev->next = f->next;
			  Face_Free (f);
		Sys_MarkMapModified();		// PGM
        return;
      }
      n++;
      pPrev = f;
    }
  }
}

//==========
//PGM
void WINAPI QERApp_BuildBrush (LPVOID pv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
	if (pBrush != NULL)
	{
		Brush_Build(pBrush);
		Sys_UpdateWindows(W_ALL);
	}
}

//Timo : another version with bConvert flag
//++timo since 1.7 is not compatible with earlier plugin versions, remove this one and update QERApp_BuildBrush
void WINAPI QERApp_BuildBrush2 (LPVOID pv, int bConvert)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
	if (pBrush != NULL)
	{
		Brush_Build( pBrush, true, true, bConvert );
		Sys_UpdateWindows(W_ALL);
	}
}

void WINAPI QERApp_SelectBrush (LPVOID pv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
	if (pBrush != NULL)
	{
		Select_Brush(pBrush, false);
		Sys_UpdateWindows(W_ALL);
	}

}

void WINAPI QERApp_DeselectBrush (LPVOID pv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// FIXME - implement this!
}

void WINAPI QERApp_ResetPlugins()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  g_pParentWnd->OnPluginsRefresh();
}

void WINAPI QERApp_DeselectAllBrushes ()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	Select_Deselect();
	Sys_UpdateWindows(W_ALL);
}
//PGM
//==========

void WINAPI QERApp_TextureBrush(LPVOID pv, LPCSTR pName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  brush_t *pBrush = g_pParentWnd->GetPlugInMgr().FindBrushHandle(pv);
  if (pBrush != NULL)
  {
	  for (face_t *f = pBrush->brush_faces ; f; f = f->next)
    {
      //strcpy(f->texdef.name, pName);
      f->texdef.SetName(pName);
    }
    Sys_MarkMapModified();		// PGM
  }
}

int WINAPI QERApp_SelectedBrushCount()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
	for (brush_t *pb = selected_brushes.next ; pb != &selected_brushes ; pb = pb->next)
	{
    n++;
  }
  return n;
}

int WINAPI QERApp_ActiveBrushCount()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
	for (brush_t *pb = active_brushes.next ; pb != &active_brushes ; pb = pb->next)
	{
    n++;
  }
  return n;
}

int WINAPI QERApp_AllocateSelectedBrushHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
	for (brush_t *pb = selected_brushes.next ; pb != &selected_brushes ; pb = pb->next)
	{
    n++;
    g_pParentWnd->GetPlugInMgr().GetSelectedHandles().Add(pb);
  }
  return n;
}

int WINAPI QERApp_AllocateActiveBrushHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
	for (brush_t *pb = active_brushes.next ; pb != &active_brushes ; pb = pb->next)
	{
    n++;
    g_pParentWnd->GetPlugInMgr().GetActiveHandles().Add(pb);
  }
  return n;
}

void WINAPI QERApp_ReleaseSelectedBrushHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  g_pParentWnd->GetPlugInMgr().GetSelectedHandles().RemoveAll();
  Sys_UpdateWindows(W_ALL);
}

void WINAPI QERApp_ReleaseActiveBrushHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  g_pParentWnd->GetPlugInMgr().GetActiveHandles().RemoveAll();
  Sys_UpdateWindows(W_ALL);
}

LPVOID WINAPI QERApp_GetActiveBrushHandle(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  if (nIndex < g_pParentWnd->GetPlugInMgr().GetActiveHandles().GetSize())
  {
    return reinterpret_cast<LPVOID>(g_pParentWnd->GetPlugInMgr().GetActiveHandles().GetAt(nIndex));
  }
  return NULL;
}

LPVOID WINAPI QERApp_GetSelectedBrushHandle(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  if (nIndex < g_pParentWnd->GetPlugInMgr().GetSelectedHandles().GetSize())
  {
    return reinterpret_cast<LPVOID>(g_pParentWnd->GetPlugInMgr().GetSelectedHandles().GetAt(nIndex));
  }
  return NULL;
}

int WINAPI QERApp_TextureCount()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	Texture_StartPos ();
  int x, y;
  int n = 0;
	while (1)
	{
		qtexture_t *q = Texture_NextPos (&x, &y);
		if (!q)
			break;
    n++;
  }
  return n;
}

LPCSTR WINAPI QERApp_GetTexture(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  static char name[QER_MAX_NAMELEN];
	Texture_StartPos ();
  int x, y;
  int n = 0;
	while (1)
	{
		qtexture_t *q = Texture_NextPos (&x, &y);
		if (!q)
			break;
    if (n == nIndex)
    {
      strcpy(name, q->name);
      return name;
    }
    n++;
  }
  return NULL;
}

LPCSTR WINAPI QERApp_GetCurrentTexture()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  return g_qeglobals.d_texturewin.texdef.name;
}

void WINAPI QERApp_SetCurrentTexture(LPCSTR strName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//++timo hu ?? tex is not initialized ?? can be any value ..
  texdef_t tex;
  //++timo added a brushprimit_texdef ..
  // smthg to be done here
  brushprimit_texdef_t brushprimit_tex;
  //strcpy(tex.name, strName);
  tex.SetName(strName);
  Texture_SetTexture(&tex,&brushprimit_tex);
}

int WINAPI QERApp_GetEClassCount()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
  for (eclass_t *e = eclass ; e ; e = e->next)
	{
    n++;
  }
  return n;
}

LPCSTR WINAPI QERApp_GetEClass(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  int n = 0;
  for (eclass_t *e = eclass ; e ; e = e->next)
	{
    if (n == nIndex)
    {
      return e->name;
    }
  }
  return NULL;
}

void WINAPI QERApp_LoadTextureRGBA(LPVOID vp)
{
  Texture_LoadFromPlugIn(vp);
}

// v1.70 code
// world_entity holds the worldspawn and is indexed as 0
// other entities are in the entities doubly linked list
// QERApp_GetEntityCount counts the entities like in any C array: [0..length-1]
int WINAPI QERApp_GetEntityCount()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	int n = 1;
	for (entity_t *pe = entities.next ; pe != &entities ; pe = pe->next)
	{
		n++;
	}
	return n;
}

// We don't store entities in CPtrArray, we need to walk the list
LPVOID WINAPI QERApp_GetEntityHandle(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nIndex==0)
		// looks for the worldspawn
		return static_cast<LPVOID>(world_entity);
	entity_t *pe = &entities;
	int n = 0;
	while ( n < nIndex )
	{
		pe = pe->next;
		n++;
	}
	return static_cast<LPVOID>(pe);
}

epair_t** WINAPI QERApp_GetEntityKeyValList(LPVOID vp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	entity_t *pe = static_cast<entity_t *>(vp);
	return &pe->epairs;
}

epair_t* WINAPI QERApp_AllocateEpair( char *key, char *val )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	epair_t *e = (epair_t*)qmalloc (sizeof(*e));
	e->key = (char*)qmalloc(strlen(key)+1);
	strcpy (e->key, key);
	e->value = (char*)qmalloc(strlen(val)+1);
	strcpy (e->value, val);
	return e;
}

void WINAPI QERApp_SetEntityKeyValList(LPVOID vp, epair_t* ep)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	entity_t *pe = static_cast<entity_t *>(vp);
	if (pe->epairs)
		Sys_Printf( "Warning : pe->epairs != NULL in QERApp_SetEntityKeyValList, will not set\n" );
	else
		pe->epairs = ep;
}

int WINAPI QERApp_AllocateEntityBrushHandles(LPVOID vp)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	entity_t *pe = static_cast<entity_t *>(vp);
	int n = 0;
	if (!pe->brushes.onext)
		return 0;
	g_pParentWnd->GetPlugInMgr().GetEntityBrushHandles().RemoveAll();
	for (brush_t *pb = pe->brushes.onext ; pb != &pe->brushes ; pb=pb->onext)
	{
		n++;
		g_pParentWnd->GetPlugInMgr().GetEntityBrushHandles().Add(pb);
	}
	return n;
}
	
void WINAPI QERApp_ReleaseEntityBrushHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_pParentWnd->GetPlugInMgr().GetEntityBrushHandles().RemoveAll();
}

LPVOID WINAPI QERApp_GetEntityBrushHandle(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nIndex < g_pParentWnd->GetPlugInMgr().GetEntityBrushHandles().GetSize())
		return g_pParentWnd->GetPlugInMgr().GetEntityBrushHandles().GetAt(nIndex);
	return NULL;
}

LPVOID WINAPI QERApp_CreateEntityHandle()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	entity_t *pe = reinterpret_cast<entity_t*>(qmalloc(sizeof(entity_t)));
	pe->brushes.onext = pe->brushes.oprev = &pe->brushes;
	g_pParentWnd->GetPlugInMgr().GetEntityHandles().Add(static_cast<LPVOID>(pe));
	return static_cast<LPVOID>(pe);
}

// the vpBrush needs to be in m_BrushHandles
//++timo we don't have allocation nor storage for vpEntity, no checks for this one
void WINAPI QERApp_CommitBrushHandleToEntity(LPVOID vpBrush, LPVOID vpEntity)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_pParentWnd->GetPlugInMgr().CommitBrushHandleToEntity(vpBrush, vpEntity);
	return;
}

char* WINAPI QERApp_ReadProjectKey(char* key)
{
	return ValueForKey(g_qeglobals.d_project_entity, key);
}

int WINAPI QERApp_ScanFileForEClass(char *filename )
{
	// set single class parsing
	parsing_single = true;
	Eclass_ScanFile(filename);
	if (eclass_found)
	{
		eclass_e->nShowFlags |= ECLASS_PLUGINENTITY;
		return 1;
	}
	return 0;
}

// the vpBrush needs to be in m_BrushHandles
//++timo add a debug check to see if we found the brush handle
// NOTE : seems there's no way to check vpEntity is valid .. this is dangerous
// links the brush to its entity, everything else is done when commiting the entity to the map
void CPlugInManager::CommitBrushHandleToEntity(LPVOID vpBrush, LPVOID vpEntity)
{
	brush_t* pb;
	entity_t* pe;
	for (int i=0 ; i < m_BrushHandles.GetSize() ; i++)
	{
		if (vpBrush == m_BrushHandles.GetAt(i))
		{
			m_BrushHandles.RemoveAt(i);
			pb = reinterpret_cast<brush_t*>(vpBrush);
			pe = reinterpret_cast<entity_t *>(vpEntity);
			Entity_LinkBrush (pe, pb);
		}
	}
	Sys_UpdateWindows(W_ALL);
}

// the vpEntity must be in m_EntityHandles
void WINAPI QERApp_CommitEntityHandleToMap(LPVOID vpEntity)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_pParentWnd->GetPlugInMgr().CommitEntityHandleToMap(vpEntity);
	return;
}

int WINAPI QERApp_LoadFile( const char *pLocation, void ** buffer )
{
	char cPath[1024];
	sprintf( cPath, "%s/%s", ValueForKey(g_qeglobals.d_project_entity, "basepath"), pLocation);
  int nSize = LoadFile( cPath, buffer);
  if (nSize == -1)
  {
    nSize = PakLoadAnyFile(cPath, buffer);
  }
	return nSize;
}

char * WINAPI QERApp_ExpandReletivePath (char *p)
{
	return ExpandReletivePath(p);
}

// NOTE: this is a simplified version
int WINAPI QERApp_HasShader( const char *pName )
{
	CShaderInfo *pInfo = hasShader( pName );
	if (pInfo)
		return 1;
	return 0;
}

qtexture_t* WINAPI QERApp_Texture_ForName (const char *name)
{
	// if the texture is not loaded yet, this call will get it loaded
	// but: when we assign a GL bind number, we need to be in the g_qeglobals.d_hdcBase , g_qeglobals.d_hglrcBase GL context
	// the plugin may set the GL context to whatever he likes, but then load would fail
	// NOTE: is context switching time-consuming? then maybe the plugin could handle the context switch and only add a 
	// sanity check in debug mode here
	// read current context
	HDC pluginHDC = qwglGetCurrentDC();
	HGLRC pluginHGLRC = qwglGetCurrentContext();
	qwglMakeCurrent( g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase );
	qtexture_t* qtex = Texture_ForName( name );
	return qtex;
	qwglMakeCurrent( pluginHDC, pluginHGLRC );
}

void WINAPI QERApp_RadiantFree( void * buf )
{
	free( buf );
}

char* WINAPI QERApp_Token()
{
	return token;
}

char* WINAPI QERApp_GetMapName()
{
	return currentmap;
}

void CPlugInManager::CommitEntityHandleToMap(LPVOID vpEntity)
{
	entity_t *pe;
	eclass_t *e;
	brush_t		*b;
	vec3_t mins,maxs;
	bool has_brushes;
	for (int i=0 ; i < m_EntityHandles.GetSize() ; i++ )
	{
		if (vpEntity == m_EntityHandles.GetAt(i))
		{
			m_EntityHandles.RemoveAt(i);
			pe = reinterpret_cast<entity_t*>(vpEntity);
			// fill additional fields
			// straight copy from Entity_Parse
			// entity_t::origin
			GetVectorForKey (pe, "origin", pe->origin);
			// entity_t::eclass
			if (pe->brushes.onext == &pe->brushes)
				has_brushes = false;
			else
				has_brushes = true;
			e = Eclass_ForName (ValueForKey (pe, "classname"), has_brushes);
			pe->eclass = e;
			// fixedsize
			if (e->fixedsize)
			{
				if (pe->brushes.onext != &pe->brushes)
				{
					Sys_Printf("Warning : Fixed size entity with brushes in CPlugInManager::CommitEntityHandleToMap\n");
				}
				// create a custom brush
				VectorAdd(e->mins, pe->origin, mins);
				VectorAdd(e->maxs, pe->origin, maxs);
				float a = 0;
				if (e->nShowFlags & ECLASS_MISCMODEL)
				{
					char* p = ValueForKey(pe, "model");
					if (p != NULL && strlen(p) > 0)
					{
						vec3_t vMin, vMax;
						a = FloatForKey (pe, "angle");
				        if (GetCachedModel(pe, p, vMin, vMax))
				        {
						      // create a custom brush
						      VectorAdd (pe->md3Class->mins, pe->origin, mins);
						      VectorAdd (pe->md3Class->maxs, pe->origin, maxs);
				        }
					}
			    }
		
			    b = Brush_Create (mins, maxs, &e->texdef);

			    if (a)
			    {
					vec3_t vAngle;
					vAngle[0] = vAngle[1] = 0;
					vAngle[2] = a;
					Brush_Rotate(b, vAngle, pe->origin, false);
				}

				b->owner = pe;

				b->onext = pe->brushes.onext;
				b->oprev = &pe->brushes;
				pe->brushes.onext->oprev = b;
				pe->brushes.onext = b;
			}
			else
			{	// brush entity
				if (pe->brushes.next == &pe->brushes)
					Sys_Printf ("Warning: Brush entity with no brushes in CPlugInManager::CommitEntityHandleToMap\n");
			}

			// add brushes to the active brushes list
			// and build them along the way
			for (b=pe->brushes.onext ; b != &pe->brushes ; b=b->onext)
			{
				// convert between old brushes and brush primitive
				if (g_qeglobals.m_bBrushPrimitMode)
				{
					// we only filled the shift scale rot fields, needs conversion
					Brush_Build( b, true, true, true );
				}
				else
				{
					// we are using old brushes
					Brush_Build( b );
				}
				b->next = active_brushes.next;
				active_brushes.next->prev = b;
				b->prev = &active_brushes;
				active_brushes.next = b;
			}

			// handle worldspawn entities
			// if worldspawn has no brushes, use the new one
			if (!strcmp(ValueForKey (pe, "classname"), "worldspawn"))
			{
				if ( world_entity && ( world_entity->brushes.onext != &world_entity->brushes ) )
				{
					// worldspawn already has brushes
					Sys_Printf ("Commiting worldspawn as func_group\n");
					SetKeyValue(pe, "classname", "func_group");
					// add the entity to the end of the entity list
					pe->next = &entities;
					pe->prev = entities.prev;
					entities.prev->next = pe;
					entities.prev = pe;
					g_qeglobals.d_num_entities++;
				}
				else
				{
					// there's a worldspawn with no brushes, we assume the map is empty
					if ( world_entity )
					{
						Entity_Free( world_entity );
						world_entity = pe;
					}
					else
						Sys_Printf("Warning : unexpected world_entity == NULL in CommitEntityHandleToMap\n");
				}
			}
			else
			{
				// add the entity to the end of the entity list
				pe->next = &entities;
				pe->prev = entities.prev;
				entities.prev->next = pe;
				entities.prev = pe;
				g_qeglobals.d_num_entities++;
			}
		}
	}
}

void WINAPI QERApp_SetScreenUpdate(int bScreenUpdates)
{
	g_bScreenUpdates = bScreenUpdates; 
}

texturewin_t* WINAPI QERApp_QeglobalsTexturewin()
{
	return &g_qeglobals.d_texturewin;
}

_QERTextureInfo* CPlugInManager::GetTextureInfo()
{
  if (m_pTexturePlug != NULL)
  {
    return m_pTexturePlug->getTextureInfo();
  }
  else
  {
    return NULL;
  }
}

LPVOID CPlugInManager::GetSurfaceFlags()
{
  if (m_pSurfaceListPlug != NULL)
  {
    return m_pSurfaceListPlug->getSurfaceFlags();
  }
  else
  {
    return NULL;
  }
}

void CPlugInManager::LoadTexture(const char *pFilename)
{
  if (m_pTexturePlug != NULL)
  {
    m_pTexturePlug->loadTexture(pFilename);
  }
}

patchMesh_t* WINAPI QERApp_GetSelectedPatch( )
{
	for (brush_t *pb = selected_brushes.next ; pb != &selected_brushes ; pb = pb->next)
	{
		if (pb->patchBrush)
		{
			return pb->pPatch;
		}
	}
#ifdef _DEBUG
	Sys_Printf("WARNING: QERApp_GetSelectedPatchTexdef called with no patch selected\n");
#endif
	return NULL;
}

char* WINAPI QERApp_GetGamePath()
{
	return g_PrefsDlg.m_strQuake2.GetBuffer(0);
}

char* WINAPI QERApp_GetQERPath()
{
	return g_strAppPath.GetBuffer(0);
}

// patches in/out -----------------------------------
int WINAPI QERApp_AllocateActivePatchHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return g_pParentWnd->GetPlugInMgr().AllocateActivePatchHandles();
}

int CPlugInManager::AllocateActivePatchHandles()
{
	int n = 0;
	for (brush_t *pb = active_brushes.next ; pb != &active_brushes ; pb = pb->next)
	{
		if (pb->patchBrush)
		{
			n++;
			m_PatchesHandles.Add(pb);
		}
	}
	return n;
}

int WINAPI QERApp_AllocateSelectedPatchHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return g_pParentWnd->GetPlugInMgr().AllocateSelectedPatchHandles();
}
	
int CPlugInManager::AllocateSelectedPatchHandles()
{
	int n = 0;
	for (brush_t *pb = selected_brushes.next ; pb != &selected_brushes ; pb = pb->next)
	{
		if (pb->patchBrush)
		{
			n++;
			m_PatchesHandles.Add(pb);
		}
	}
	return n;
}

void WINAPI QERApp_ReleasePatchHandles()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_pParentWnd->GetPlugInMgr().ReleasePatchesHandles();
}

patchMesh_t* WINAPI QERApp_GetPatchData(int index)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	static patchMesh_t patch;
	patchMesh_t *pPatch = g_pParentWnd->GetPlugInMgr().FindPatchHandle(index);
	if (pPatch)
	{
		memcpy( &patch, pPatch, sizeof(patchMesh_t) );
		return &patch;
	}
	return NULL;
}

void WINAPI QERApp_DeletePatch(int index)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	patchMesh_t *pPatch = g_pParentWnd->GetPlugInMgr().FindPatchHandle(index);
	if (pPatch)
	{
		brush_t *pb = pPatch->pSymbiot;
		Patch_Delete( pPatch );
		if (pb)
			Brush_Free( pb );
	}
#ifdef _DEBUG
	Sys_Printf("Warning: QERApp_DeletePatch: FindPatchHandle failed\n");
#endif
}

int WINAPI QERApp_CreatePatchHandle()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return g_pParentWnd->GetPlugInMgr().CreatePatchHandle();
}

int CPlugInManager::CreatePatchHandle()
{
	// NOTE: we can't call the AddBrushForPatch until we have filled the patchMesh_t structure
	patchMesh_t *pPatch = MakeNewPatch();
	m_PluginPatches.Add( pPatch );
	// change mode
	PatchesMode = EAllocatedPatches;
	return m_PluginPatches.GetSize()-1;
}

void WINAPI QERApp_CommitPatchHandleToMap(int index, patchMesh_t *pMesh, char *texName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	g_pParentWnd->GetPlugInMgr().CommitPatchHandleToMap(index, pMesh, texName);
}

void CPlugInManager::CommitPatchHandleToMap(int index, patchMesh_t *pMesh, char *texName)
{
	if (PatchesMode==EAllocatedPatches)
	{
		patchMesh_t *pPatch = reinterpret_cast<patchMesh_t *>( m_PluginPatches.GetAt(index) );
		memcpy( pPatch, pMesh, sizeof( patchMesh_t ) );
		// patch texturing, if none given use current texture
		if (texName)
			pPatch->d_texture = Texture_ForName(texName);
		if ( !pPatch->d_texture )
		{
			pPatch->d_texture = Texture_ForName(g_qeglobals.d_texturewin.texdef.name);
			// checking .. just in case
			if (!pPatch->d_texture)
			{
#ifdef _DEBUG
				Sys_Printf("WARNING: failed to set patch to current texture in CPlugInManager::CommitPatchHandleToMap\n");
#endif
				pPatch->d_texture = notexture;
			}
		}
		g_bScreenUpdates = false;
		// the bLinkToWorld flag in AddBrushForPatch takes care of Brush_AddToList Entity_linkBrush and Brush_Build
		brush_t *pb = AddBrushForPatch( pPatch, true );
		Select_Brush( pb );
		g_bScreenUpdates = true;
		Sys_UpdateWindows(W_ALL);
	}
	else
	{
		brush_t *pBrush = reinterpret_cast<brush_t *>( m_PatchesHandles.GetAt(index) );
		patchMesh_t *pPatch = pBrush->pPatch;
		pPatch->width = pMesh->width;
		pPatch->height = pMesh->height;
		pPatch->contents = pMesh->contents;
		pPatch->flags = pMesh->flags;
		pPatch->value = pMesh->value;
		pPatch->type = pMesh->type;
		memcpy( pPatch->ctrl, pMesh->ctrl, sizeof(drawVert_t)*MAX_PATCH_HEIGHT*MAX_PATCH_WIDTH );
		pPatch->bDirty = true;
	}
}

// this gets called when the plugin needs specific services, for example the OpenGL drawing interface
//++timo plugins should be able to dynamically register their own interfaces in there
int WINAPI QERApp_RequestInterface( REFGUID refGUID, LPVOID pInterface )
{
	if (IsEqualGUID( refGUID, QERQglTable_GUID ))
	{
		_QERQglTable *pQglTable = static_cast<_QERQglTable *>(pInterface);
		if ( pQglTable->m_nSize != sizeof(_QERQglTable) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERQglTable interface\n");
			return 0;
		}
		pQglTable->m_pfn_qglAlphaFunc = qglAlphaFunc;
		pQglTable->m_pfn_qglBegin = qglBegin;
		pQglTable->m_pfn_qglBindTexture = qglBindTexture;
		pQglTable->m_pfn_qglBlendFunc = qglBlendFunc;
		pQglTable->m_pfn_qglCallList = qglCallList;
		pQglTable->m_pfn_qglCallLists = qglCallLists;
		pQglTable->m_pfn_qglClear = qglClear;
		pQglTable->m_pfn_qglClearColor = qglClearColor;
		pQglTable->m_pfn_qglClearDepth = qglClearDepth;
		pQglTable->m_pfn_qglColor3f = qglColor3f;
		pQglTable->m_pfn_qglColor4f = qglColor4f;
		pQglTable->m_pfn_qglCullFace = qglCullFace;
		pQglTable->m_pfn_qglDisable = qglDisable;
		pQglTable->m_pfn_qglDeleteLists = qglDeleteLists;
		pQglTable->m_pfn_qglEnable = qglEnable;
		pQglTable->m_pfn_qglEnd = qglEnd;
		pQglTable->m_pfn_qglEndList = qglEndList;
		pQglTable->m_pfn_qglGenLists = qglGenLists;
		pQglTable->m_pfn_qglListBase = qglListBase;
		pQglTable->m_pfn_qglLoadIdentity = qglLoadIdentity;
		pQglTable->m_pfn_qglMatrixMode = qglMatrixMode;
		pQglTable->m_pfn_qglNewList = qglNewList;
		pQglTable->m_pfn_qglNormal3f = qglNormal3f;
		pQglTable->m_pfn_qglOrtho = qglOrtho;
		pQglTable->m_pfn_qglPointSize = qglPointSize;
		pQglTable->m_pfn_qglPolygonMode = qglPolygonMode;
		pQglTable->m_pfn_qglPopMatrix = qglPopMatrix;
		pQglTable->m_pfn_qglPushMatrix = qglPushMatrix;
		pQglTable->m_pfn_qglRotated = qglRotated;
		pQglTable->m_pfn_qglRotatef = qglRotatef;
		pQglTable->m_pfn_qglScalef = qglScalef;
		pQglTable->m_pfn_qglTexCoord2f = qglTexCoord2f;
		pQglTable->m_pfn_qglTexEnvf = qglTexEnvf;
		pQglTable->m_pfn_qglTexGenf = qglTexGenf;
		pQglTable->m_pfn_qglTexParameterf = qglTexParameterf;
		pQglTable->m_pfn_qglTranslated = qglTranslated;
		pQglTable->m_pfn_qglTranslatef = qglTranslatef;
		pQglTable->m_pfn_qglVertex2f = qglVertex2f;
		pQglTable->m_pfn_qglVertex3f = qglVertex3f;
		pQglTable->m_pfn_qglViewport = qglViewport;
		pQglTable->m_pfn_QE_CheckOpenGLForErrors = &QE_CheckOpenGLForErrors;
		pQglTable->m_pfn_QEW_SetupPixelFormat = &QEW_SetupPixelFormat;
		pQglTable->m_pfn_qwglCreateContext = qwglCreateContext;
		pQglTable->m_pfn_qwglDeleteContext = qwglDeleteContext;
		pQglTable->m_pfn_qwglMakeCurrent = qwglMakeCurrent;
		pQglTable->m_pfn_qwglShareLists = qwglShareLists;
		pQglTable->m_pfn_qwglSwapBuffers = qwglSwapBuffers;
		pQglTable->m_pfn_qwglUseFontBitmaps = qwglUseFontBitmaps;
		pQglTable->m_pfnGetQeglobalsHGLRC = &QERApp_GetQeglobalsHGLRC;
		pQglTable->m_pfn_qgluPerspective = qgluPerspective;
		pQglTable->m_pfn_qgluLookAt = qgluLookAt;
		pQglTable->m_pfnHookXYGLWindow = QERApp_HookXYGLWindow;
		pQglTable->m_pfnUnHookGLWindow = QERApp_UnHookGLWindow;

		return 1;
	}
	else if (IsEqualGUID( refGUID, QERSelectedFaceTable_GUID ))
	{
		_QERSelectedFaceTable *pSelectedFaceTable = static_cast<_QERSelectedFaceTable *>(pInterface);
		if ( pSelectedFaceTable->m_nSize != sizeof(_QERSelectedFaceTable) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERSelectedFaceTable interface\n");
			return 0;
		}
		pSelectedFaceTable->m_pfnGetTextureNumber = &QERApp_ISelectedFace_GetTextureNumber;
		pSelectedFaceTable->m_pfnGetFaceInfo = &QERApp_GetFaceInfo;
		pSelectedFaceTable->m_pfnSetFaceInfo = &QERApp_SetFaceInfo;
		pSelectedFaceTable->m_pfnGetTextureSize = &QERApp_GetTextureSize;
		pSelectedFaceTable->m_pfnTextureForName = &QERApp_Texture_ForName;
		pSelectedFaceTable->m_pfnSelect_SetTexture = &Select_SetTexture;
		return 1;
	}
	else if (IsEqualGUID( refGUID, QERPluginEntitiesTable_GUID ))
	{
		_QERPluginEntitiesTable *pPluginEntitiesTable = static_cast<_QERPluginEntitiesTable *>(pInterface);
		if ( pPluginEntitiesTable->m_nSize != sizeof(_QERPluginEntitiesTable) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERPluginEntitiesTable interface\n");
			return 0;
		}
		pPluginEntitiesTable->m_pfnEClassScanDir = &QERApp_EClassScanDir;
		return 1;
	}
	else if (IsEqualGUID( refGUID, QERScripLibTable_GUID ))
	{
		_QERScripLibTable *pScripLibTable = static_cast<_QERScripLibTable *>(pInterface);
		if ( pScripLibTable->m_nSize != sizeof( _QERScripLibTable ) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERScripLibTable\n");
			return 0;
		}
		pScripLibTable->m_pfnGetToken = &GetToken;
		pScripLibTable->m_pfnToken = &QERApp_Token;
		pScripLibTable->m_pfnUnGetToken = &UngetToken;
		return 1;
	}
	else if (IsEqualGUID( refGUID, QERAppSurfaceTable_GUID ))
	{
		_QERAppSurfaceTable *pSurfaceTable = static_cast<_QERAppSurfaceTable *>(pInterface);
		if ( pSurfaceTable->m_nSize != sizeof( _QERAppSurfaceTable ) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERAppSurfaceTable\n");
			return 0;
		}
		pSurfaceTable->m_pfnAnyPatchesSelected = &AnyPatchesSelected;
		pSurfaceTable->m_pfnOnlyPatchesSelected = &OnlyPatchesSelected;
		pSurfaceTable->m_pfnQeglobalsTexturewin = &QERApp_QeglobalsTexturewin;
		pSurfaceTable->m_pfnGetSelectedPatch = &QERApp_GetSelectedPatch;
		pSurfaceTable->m_pfnPatchRebuild = &Patch_Rebuild;
		pSurfaceTable->m_pfnGetTwoSelectedPatch = &QERApp_GetTwoSelectedPatch;
		return 1;
	}
	else if (IsEqualGUID( refGUID, QERAppBSPFrontendTable_GUID ))
	{
		_QERAppBSPFrontendTable *pBSPFrontendTable = static_cast<_QERAppBSPFrontendTable *>(pInterface);
		if ( pBSPFrontendTable->m_nSize != sizeof( _QERAppBSPFrontendTable ) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERAppBSPFrontendTable\n");
			return 0;
		}
		pBSPFrontendTable->m_pfnGetMapName = &QERApp_GetMapName;
		pBSPFrontendTable->m_pfnLoadPointFile = &Pointfile_Check;
		return 1;
	}
	else if (IsEqualGUID( refGUID, QERMessaging_GUID ))
	{
		_QERMessagingTable *pMessagingTable = static_cast<_QERMessagingTable *>(pInterface);
		if ( pMessagingTable->m_nSize != sizeof( _QERMessagingTable ) )
		{
			Sys_Printf("wrong m_nSize in plugin-requested _QERMessagingTable\n");
			return 0;
		}
		pMessagingTable->m_pfnHookWindow = QERApp_HookWindow;
		pMessagingTable->m_pfnUnHookWindow = QERApp_UnHookWindow;
		pMessagingTable->m_pfnGetXYWndWrapper = QERApp_GetXYWndWrapper;
		pMessagingTable->m_pfnHookListener = QERApp_HookListener;
		pMessagingTable->m_pfnUnHookListener = QERApp_UnHookListener;
		return 1;
	}
	else if (IsEqualGUID( refGUID, QERShadersTable_GUID ))
	{
		_QERShadersTable *pShadersTable = static_cast<_QERShadersTable *>(pInterface);
		if ( pShadersTable->m_nSize != sizeof( _QERShadersTable ) )
		{
			Sys_Printf("wring m_nSize in plugin-requested _QERShadersTable\n");
			return 0;
		}
		pShadersTable->m_pfnTryTextureForName = QERApp_TryTextureForName;
		return 1;
	}
	return 0;
}

int CPlugInManager::FillFuncTable(CPlugIn *pPlug)
{
  _QERFuncTable_1 *pTable = reinterpret_cast<_QERFuncTable_1*>(pPlug->getFuncTable());
  if (pTable != NULL)
  {
    if (pTable->m_fVersion != QER_PLUG_VERSION)
    {
      Sys_Printf("Radiant plugin manager was built with version %.2f, Plugin %s is version %.2f\n", QER_PLUG_VERSION, pPlug->getVersionStr() , pTable->m_fVersion);
    }
    if (pTable->m_fVersion >= QER_PLUG_VERSION_1)
    {
      pTable->m_pfnCreateBrush = &QERApp_CreateBrush;
      pTable->m_pfnCreateBrushHandle = &QERApp_CreateBrushHandle;
      pTable->m_pfnDeleteBrushHandle = &QERApp_DeleteBrushHandle;
      pTable->m_pfnCommitBrushHandle = &QERApp_CommitBrushHandleToMap;
      pTable->m_pfnAddFace = &QERApp_AddFace;
      pTable->m_pfnAddFaceData = &QERApp_AddFaceData;
      pTable->m_pfnGetFaceData = &QERApp_GetFaceData;
      pTable->m_pfnGetFaceCount = &QERApp_GetFaceCount;
      pTable->m_pfnSetFaceData = &QERApp_SetFaceData;
      pTable->m_pfnDeleteFace = &QERApp_DeleteFace;
      pTable->m_pfnTextureBrush = &QERApp_TextureBrush;
	  pTable->m_pfnBuildBrush = &QERApp_BuildBrush;						// PGM
  	  pTable->m_pfnSelectBrush = &QERApp_SelectBrush;					// PGM
	  pTable->m_pfnDeselectBrush = &QERApp_DeselectBrush;				// PGM
	  pTable->m_pfnDeselectAllBrushes = &QERApp_DeselectAllBrushes;		// PGM
      pTable->m_pfnDeleteSelection = &QERApp_DeleteSelection;
      pTable->m_pfnGetPoints = &QERApp_GetPoints;
      pTable->m_pfnSysMsg = &QERApp_SysMsg;
      pTable->m_pfnInfoMsg = &QERApp_InfoMsg;
      pTable->m_pfnHideInfoMsg = &QERApp_HideInfoMsg;
      pTable->m_pfnPositionView = &QERApp_PositionView;					// PGM
      pTable->m_pfnSelectedBrushCount = &QERApp_SelectedBrushCount;
      pTable->m_pfnAllocateSelectedBrushHandles  = &QERApp_AllocateSelectedBrushHandles;
      pTable->m_pfnReleaseSelectedBrushHandles  = &QERApp_ReleaseSelectedBrushHandles;
      pTable->m_pfnGetSelectedBrushHandle = &QERApp_GetSelectedBrushHandle;
      pTable->m_pfnActiveBrushCount = &QERApp_ActiveBrushCount;
      pTable->m_pfnAllocateActiveBrushHandles = &QERApp_AllocateActiveBrushHandles;
      pTable->m_pfnReleaseActiveBrushHandles = &QERApp_ReleaseActiveBrushHandles;
      pTable->m_pfnGetActiveBrushHandle = &QERApp_GetActiveBrushHandle;
      pTable->m_pfnTextureCount = &QERApp_TextureCount;
      pTable->m_pfnGetTexture = &QERApp_GetTexture;
      pTable->m_pfnGetCurrentTexture = &QERApp_GetCurrentTexture;
      pTable->m_pfnSetCurrentTexture = &QERApp_SetCurrentTexture;
      pTable->m_pfnGetEClassCount = &QERApp_GetEClassCount;
      pTable->m_pfnGetEClass = &QERApp_GetEClass;
      pTable->m_pfnResetPlugins = &QERApp_ResetPlugins;
    }
    // end of v1.00
    if (pTable->m_fVersion >= 1.5f)
    {
      // v1.50 starts
      pTable->m_pfnLoadTextureRGBA = &QERApp_LoadTextureRGBA;
      // end of v1.50
    }
	if (pTable->m_fVersion >= 1.7f)
	{
		pTable->m_pfnGetEntityCount = &QERApp_GetEntityCount;
		pTable->m_pfnGetEntityHandle = &QERApp_GetEntityHandle;
		pTable->m_pfnGetEntityKeyValList = &QERApp_GetEntityKeyValList;
		pTable->m_pfnAllocateEpair = &QERApp_AllocateEpair;
		pTable->m_pfnSetEntityKeyValList = &QERApp_SetEntityKeyValList;
		pTable->m_pfnAllocateEntityBrushHandles = &QERApp_AllocateEntityBrushHandles;
		pTable->m_pfnReleaseEntityBrushHandles = &QERApp_ReleaseEntityBrushHandles;
		pTable->m_pfnGetEntityBrushHandle = &QERApp_GetEntityBrushHandle;
		pTable->m_pfnCreateEntityHandle = &QERApp_CreateEntityHandle;
		pTable->m_pfnCommitBrushHandleToEntity = &QERApp_CommitBrushHandleToEntity;
		pTable->m_pfnCommitEntityHandleToMap = &QERApp_CommitEntityHandleToMap;
		pTable->m_pfnSetScreenUpdate = &QERApp_SetScreenUpdate;
		pTable->m_pfnSysUpdateWindows = &Sys_UpdateWindows;
		pTable->m_pfnBuildBrush2 = &QERApp_BuildBrush2;
		pTable->m_pfnReadProjectKey = &QERApp_ReadProjectKey;
		pTable->m_pfnScanFileForEClass = &QERApp_ScanFileForEClass;
		pTable->m_pfnRequestInterface = &QERApp_RequestInterface;
		pTable->m_pfnErrorMsg = &QERApp_ErrorMsg;
		pTable->m_pfnLoadFile = &QERApp_LoadFile;
		pTable->m_pfnExpandReletivePath = &QERApp_ExpandReletivePath;
		pTable->m_pfnQE_ConvertDOSToUnixName = &QE_ConvertDOSToUnixName;
		pTable->m_pfnHasShader = &QERApp_HasShader;
		pTable->m_pfnTexture_LoadSkin = &Texture_LoadSkin;
		pTable->m_pfnRadiantFree = &QERApp_RadiantFree;
		pTable->m_pfnGetGamePath = &QERApp_GetGamePath;
		pTable->m_pfnGetQERPath = &QERApp_GetQERPath;
		// patches
		pTable->m_pfnAllocateActivePatchHandles = &QERApp_AllocateActivePatchHandles;
		pTable->m_pfnAllocateSelectedPatchHandles = &QERApp_AllocateSelectedPatchHandles;
		pTable->m_pfnReleasePatchHandles = &QERApp_ReleasePatchHandles;
		pTable->m_pfnGetPatchData = &QERApp_GetPatchData;
		pTable->m_pfnDeletePatch = &QERApp_DeletePatch;
		pTable->m_pfnCreatePatchHandle = &QERApp_CreatePatchHandle;
		pTable->m_pfnCommitPatchHandleToMap = &QERApp_CommitPatchHandleToMap;
	}
	return true;
  }
  else
  {
    Sys_Printf("Unable to load %s because the function tables are not the same size\n", pPlug->getVersionStr());
  }
  return false;
}

CPlugIn * CPlugInManager::PluginForModule(HMODULE hPlug)
{
	int i;
	for( i=0; i!=m_PlugIns.GetSize(); i++ )
	{
		if ( reinterpret_cast<CPlugIn *>(m_PlugIns[i])->GetDLLModule() == hPlug )
			return reinterpret_cast<CPlugIn *>(m_PlugIns[i]);
	}
	return NULL;
}

