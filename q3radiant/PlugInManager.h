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
// PlugInManager.h: interface for the CPlugInManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLUGINMANAGER_H__CFB18412_55FE_11D2_B082_00AA00A410FC__INCLUDED_)
#define AFX_PLUGINMANAGER_H__CFB18412_55FE_11D2_B082_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "plugin.h"

class CPlugInManager  
{
private:
  CObArray m_PlugIns;
  CPtrArray m_BrushHandles;
  CPtrArray m_SelectedBrushHandles;
  CPtrArray m_ActiveBrushHandles;

  CPlugIn *m_pTexturePlug;
  CPlugIn *m_pSurfaceListPlug;

  // v1.70
  // brushes of the current entity ( see m_SelectedBrushHandles and m_ActiveBrushHandles )
  CPtrArray m_EntityBrushHandles;
  // allocated entities, not commited yet ( see m_BrushHandles )
  CPtrArray m_EntityHandles;

  // tells in which array to look when given a patch index
  enum EPatchesMode { EActivePatches, ESelectedPatches, EAllocatedPatches } PatchesMode;
  // patches handles (brush_t*)
  CPtrArray m_PatchesHandles;
  // plugin-allocated patches, not commited yet (patchMesh_t*)
  CPtrArray m_PluginPatches;

public:
	CPlugIn * PluginForModule( HMODULE hPlug );
	CPtrArray& GetActiveHandles() {return m_ActiveBrushHandles; };
	CPtrArray& GetSelectedHandles() {return m_SelectedBrushHandles; };
	CPtrArray& GetPluginPatches() {return m_PluginPatches; };
	brush_t* FindBrushHandle(void *vp);
	patchMesh_t* FindPatchHandle(int index);
	int CreatePatchHandle();
	int AllocateActivePatchHandles();
	int AllocateSelectedPatchHandles();
	void CommitPatchHandleToMap(int index, patchMesh_t *pMesh, char *texName);
	void ReleasePatchesHandles() { m_PatchesHandles.RemoveAll(); m_PluginPatches.RemoveAll(); }
	void AddFaceToBrushHandle(void *vp, vec3_t v1, vec3_t v2, vec3_t v3);
	void CommitBrushHandleToMap(void *vp);
	void DeleteBrushHandle(LPVOID vp);
	LPVOID CreateBrushHandle();
	void Dispatch(int n, const char *p);
	void Cleanup();
	void Init(const char* pPath);
	CPlugInManager();
	virtual ~CPlugInManager();
	
	// the texture manager front ends the single load
	// addins (texture, model, map formats.. etc.)
	_QERTextureInfo* GetTextureInfo();
	void LoadTexture(const char *pFilename);
	
	LPVOID GetSurfaceFlags();
	
	// v1.70
	CPtrArray& GetEntityBrushHandles() {return m_EntityBrushHandles; };
	CPtrArray& GetEntityHandles() {return m_EntityHandles; };
	// the vpBrush needs to be in m_BrushHandles
	void CommitBrushHandleToEntity( LPVOID vpBrush, LPVOID vpEntity );
	// the vpEntity needs to be in m_EntityHandles
	void CommitEntityHandleToMap( LPVOID vpEntity );

protected:
	int FillFuncTable(CPlugIn *pPlug);		// PGM
};

#endif // !defined(AFX_PLUGINMANAGER_H__CFB18412_55FE_11D2_B082_00AA00A410FC__INCLUDED_)
