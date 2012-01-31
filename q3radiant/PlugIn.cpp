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
// PlugIn.cpp: implementation of the CPlugIn class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Radiant.h"
#include "PlugIn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPlugIn::CPlugIn()
{
  m_hDLL = NULL;
  m_pQERPlugEntitiesFactory = NULL;
}

CPlugIn::~CPlugIn()
{
	if (m_pQERPlugEntitiesFactory)
		delete m_pQERPlugEntitiesFactory;
	if (m_hDLL != NULL)
		free();
}

bool CPlugIn::load(const char *p)
{
	m_hDLL = ::LoadLibrary(p);
	if (m_hDLL != NULL)
	{
		m_pfnInit = reinterpret_cast<PFN_QERPLUG_INIT>(::GetProcAddress(m_hDLL, QERPLUG_INIT));
		if (m_pfnInit != NULL)
		{
			m_strVersion = (*m_pfnInit)(AfxGetApp()->m_hInstance, g_pParentWnd->GetSafeHwnd());
			Sys_Printf("Loaded plugin > %s\n", m_strVersion);
			
			m_pfnGetName = reinterpret_cast<PFN_QERPLUG_GETNAME>(::GetProcAddress(m_hDLL, QERPLUG_GETNAME));
			if (m_pfnGetName != NULL)
			{
				m_strName = (*m_pfnGetName)();
			}
			
			m_pfnGetCommandList = reinterpret_cast<PFN_QERPLUG_GETCOMMANDLIST>(::GetProcAddress(m_hDLL, QERPLUG_GETCOMMANDLIST));
			if (m_pfnGetCommandList)
			{
				CString str = (*m_pfnGetCommandList)();
				char cTemp[1024];
				strcpy(cTemp, str);
				char* token = strtok(cTemp, ",;");
				if (token && *token == ' ')
				{
					while (*token == ' ')
						token++;
				}
				while (token != NULL)
				{
					m_CommandStrings.Add(token);
					token = strtok(NULL, ",;");
				}
			}
			
			m_pfnDispatch = reinterpret_cast<PFN_QERPLUG_DISPATCH>(::GetProcAddress(m_hDLL, QERPLUG_DISPATCH));
			m_pfnGetFuncTable = reinterpret_cast<PFN_QERPLUG_GETFUNCTABLE>(::GetProcAddress(m_hDLL, QERPLUG_GETFUNCTABLE));
			
			m_pfnGetTextureInfo = reinterpret_cast<PFN_QERPLUG_GETTEXTUREINFO>(::GetProcAddress(m_hDLL, QERPLUG_GETTEXTUREINFO));
			m_pfnLoadTexture = reinterpret_cast<PFN_QERPLUG_LOADTEXTURE>(::GetProcAddress(m_hDLL, QERPLUG_LOADTEXTURE));
			
			m_pfnGetSurfaceFlags = reinterpret_cast<PFN_QERPLUG_GETSURFACEFLAGS>(::GetProcAddress(m_hDLL, QERPLUG_GETSURFACEFLAGS));
			
			m_pfnRegisterPluginEntities = reinterpret_cast<PFN_QERPLUG_REGISTERPLUGINENTITIES>(::GetProcAddress(m_hDLL, QERPLUG_REGISTERPLUGINENTITIES));
			
			m_pfnInitSurfaceProperties = reinterpret_cast<PFN_QERPLUG_INITSURFACEPROPERTIES>(::GetProcAddress(m_hDLL, QERPLUG_INITSURFACEPROPERTIES));
			
			m_pfnRequestInterface = reinterpret_cast<PFN_QERPLUG_REQUESTINTERFACE>(::GetProcAddress(m_hDLL, QERPLUG_REQUESTINTERFACE));

			return (m_pfnDispatch != NULL && m_pfnGetFuncTable != NULL);
			//--return true;
		}
		Sys_Printf("FAILED to Load plugin > %s\n", p);
	}
	LPVOID lpMsgBuf;
	FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
	    FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
	    NULL,
	    GetLastError(),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	    (LPTSTR) &lpMsgBuf,
	    0,
	    NULL 
	);
	Sys_Printf("LoadLibrary failed on %s: GetLastError: %s\n", p, lpMsgBuf );
	// Free the buffer.
	LocalFree( lpMsgBuf );
	free();
	return false;
}

_QERTextureInfo* CPlugIn::getTextureInfo()
{
  if (m_pfnGetTextureInfo != NULL)
  {
    return reinterpret_cast<_QERTextureInfo*>((*m_pfnGetTextureInfo)());
  }
  return NULL;
}

void CPlugIn::loadTexture(LPCSTR pFilename)
{
  if (m_pfnLoadTexture != NULL)
  {
    (*m_pfnLoadTexture)(pFilename);
  }
}

LPVOID CPlugIn::getSurfaceFlags()
{
  if (m_pfnGetSurfaceFlags != NULL)
  {
    return reinterpret_cast<LPVOID>((*m_pfnGetSurfaceFlags)());
  }
  return NULL;
}

void CPlugIn::free()
{
  if (m_hDLL != NULL)
    ::FreeLibrary(m_hDLL);
  m_hDLL = NULL;
}

const char* CPlugIn::getVersionStr()
{
	return m_pfnGetName();
}

const char* CPlugIn::getMenuName()
{
  return m_strName;
}

int CPlugIn::getCommandCount()
{
  return m_CommandStrings.GetSize();
}

const char* CPlugIn::getCommand(int n)
{
  return m_CommandStrings.GetAt(n);
}

void CPlugIn::dispatchCommand(const char* p, vec3_t vMin, vec3_t vMax, BOOL bSingleBrush)
{
  if (m_pfnDispatch)
  {
    (*m_pfnDispatch)(p, vMin, vMax, bSingleBrush);
  }
}

void CPlugIn::addMenuID(int n)
{
  m_CommandIDs.Add(n);
}

bool CPlugIn::ownsCommandID(int n)
{
  for (int i = 0; i < m_CommandIDs.GetSize(); i++)
  {
    if (m_CommandIDs.GetAt(i) == n)
      return true;
  }
  return false;
}

void* CPlugIn::getFuncTable()
{
  if (m_pfnGetFuncTable)
  {
    return (*m_pfnGetFuncTable)();
  }
  return NULL;
}

void CPlugIn::RegisterPluginEntities()
{
	// if we found the QERPlug_RegisterPluginEntities export, it means this plugin provides his own entities
	if (m_pfnRegisterPluginEntities)
	{
		// resquest a _QERPlugEntitiesFactory
		if (m_pfnRequestInterface)
		{
			m_pQERPlugEntitiesFactory = new _QERPlugEntitiesFactory;
			m_pQERPlugEntitiesFactory->m_nSize = sizeof(_QERPlugEntitiesFactory);
			if (m_pfnRequestInterface( QERPlugEntitiesFactory_GUID, m_pQERPlugEntitiesFactory ))
			{
				// create an IEpair interface for the project settings
				CEpairsWrapper *pProjectEp = new CEpairsWrapper( g_qeglobals.d_project_entity );
				m_pfnRegisterPluginEntities( pProjectEp );
			}
			else
				Sys_Printf( "WARNING: failed to request QERPlugEntitiesFactory from plugin %s\n", m_strName.GetBuffer(0) );
		}
		else
			Sys_Printf( "WARNING: QERPlug_RequestInterface not found in %s\n", m_strName.GetBuffer(0) );
	}
}

void CPlugIn::InitBSPFrontendPlugin()
{
	if (m_pfnRequestInterface)
	{
		// request a _QERPlugBSPFrontendTable
		g_BSPFrontendTable.m_nSize = sizeof( _QERPlugBSPFrontendTable );
		if ( m_pfnRequestInterface( QERPlugBSPFrontendTable_GUID, &g_BSPFrontendTable ) )
		{
			g_qeglobals.bBSPFrontendPlugin = true;
		}
	}
}

void CPlugIn::InitSurfacePlugin()
{
	// if we found the QERPlug_InitSurfaceProperties export, it means this plugin does surface properties
	if (m_pfnInitSurfaceProperties)
	{
		if (g_qeglobals.bSurfacePropertiesPlugin)
		{
			Sys_Printf( "WARNING: conflict for surface properties plugins. %s ignored.\n", m_strName );
			return;
		}
		if (m_pfnRequestInterface)
		{
			// call the plugin surface properties init
			m_pfnInitSurfaceProperties();
			// request filling of the global _QERPlugSurfaceTable
			g_SurfaceTable.m_nSize = sizeof( g_SurfaceTable );
			if ( m_pfnRequestInterface( QERPlugSurfaceTable_GUID, &g_SurfaceTable ) )
			{
				// update the global so we know we have a surface properties plugin
				g_qeglobals.bSurfacePropertiesPlugin = true;
			}
			else
				Sys_Printf( "WARNING: _QERPlugSurfaceTable interface request failed for surface plugin\n" );
		}
		else
			Sys_Printf("WARNING: QERPlug_RequestInterface export not found in surface properties plugin.\n");
	}
}

// create a plugin entity
// e is the entity being created
// e->eclass is the plugin eclass info
// e->epairs will be accessed by the plugin entity through a IEpair interface
IPluginEntity * CPlugIn::CreatePluginEntity(entity_t *e)
{
	if (m_pQERPlugEntitiesFactory)
	{
		// create an IEpair interface for e->epairs
		CEpairsWrapper *pEp = new CEpairsWrapper( e );
		IPluginEntity *pEnt = m_pQERPlugEntitiesFactory->m_pfnCreateEntity( e->eclass, pEp );
		if ( pEnt )
			return pEnt;
		delete pEp;
		return NULL;
	}
	Sys_Printf("WARNING: unexpected m_pQERPlugEntitiesFactory is NULL in CPlugin::CreatePluginEntity\n");
	return NULL;
}
