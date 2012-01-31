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
// PlugIn.h: interface for the CPlugIn class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PLUGIN_H__B501A832_5755_11D2_B084_00AA00A410FC__INCLUDED_)
#define AFX_PLUGIN_H__B501A832_5755_11D2_B084_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CPlugIn : public CObject
{
private:
	HMODULE m_hDLL;
	PFN_QERPLUG_INIT m_pfnInit;
	PFN_QERPLUG_GETNAME m_pfnGetName;
	PFN_QERPLUG_GETCOMMANDLIST m_pfnGetCommandList;
	PFN_QERPLUG_DISPATCH m_pfnDispatch;
	PFN_QERPLUG_GETFUNCTABLE m_pfnGetFuncTable;
	PFN_QERPLUG_GETTEXTUREINFO m_pfnGetTextureInfo;
	PFN_QERPLUG_LOADTEXTURE m_pfnLoadTexture;
	PFN_QERPLUG_GETSURFACEFLAGS m_pfnGetSurfaceFlags;
	PFN_QERPLUG_REGISTERPLUGINENTITIES m_pfnRegisterPluginEntities;
	PFN_QERPLUG_INITSURFACEPROPERTIES m_pfnInitSurfaceProperties;
	PFN_QERPLUG_REQUESTINTERFACE m_pfnRequestInterface;
	CWordArray m_CommandIDs;
	CStringArray m_CommandStrings;
	CString m_strName;
	CString m_strVersion;
	
	// for plugins that provide plugin entities
	_QERPlugEntitiesFactory* m_pQERPlugEntitiesFactory;
	
public:
	void InitBSPFrontendPlugin();
	IPluginEntity * CreatePluginEntity( entity_t * );
	HMODULE GetDLLModule() { return m_hDLL; }
	void InitSurfacePlugin();
	void RegisterPluginEntities();
	void* getFuncTable();
	bool ownsCommandID(int n);
	void addMenuID(int n);
	CPlugIn();
	virtual ~CPlugIn();
	bool load(const char *p);
	void free();
	const char* getVersionStr();
	const char* getMenuName();
	int getCommandCount();
	const char* getCommand(int n);
	void dispatchCommand(const char* p, vec3_t vMin, vec3_t vMax, BOOL bSingleBrush);
	
	_QERTextureInfo *getTextureInfo();
	void loadTexture(LPCSTR pFilename);
	
	LPVOID getSurfaceFlags();
	
};

#endif // !defined(AFX_PLUGIN_H__B501A832_5755_11D2_B084_00AA00A410FC__INCLUDED_)
