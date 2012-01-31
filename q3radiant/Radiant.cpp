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
// Radiant.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Radiant.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "RadiantDoc.h"
#include "RadiantView.h"
#include "PrefsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp

BEGIN_MESSAGE_MAP(CRadiantApp, CWinApp)
	//{{AFX_MSG_MAP(CRadiantApp)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp construction

CRadiantApp::CRadiantApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRadiantApp object

CRadiantApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp initialization

HINSTANCE g_hOpenGL32 = NULL;
HINSTANCE g_hOpenGL = NULL;
bool g_bBuildList = false;

BOOL CRadiantApp::InitInstance()
{
  //g_hOpenGL32 = ::LoadLibrary("opengl32.dll");
	// AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
  //AfxEnableMemoryTracking(FALSE);

	// VC7 says deprecated and no longer necessary
#if 0

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#endif

	// If there's a .INI file in the directory use it instead of registry
	char RadiantPath[_MAX_PATH];
	GetModuleFileName( NULL, RadiantPath, _MAX_PATH );

	// search for exe
	CFileFind Finder;
	Finder.FindFile( RadiantPath );
	Finder.FindNextFile();
	// extract root
	CString Root = Finder.GetRoot();
	// build root\*.ini
	CString IniPath = Root + "\\REGISTRY.INI";
	// search for ini file
	Finder.FindNextFile();
	if (Finder.FindFile( IniPath ))
	{
		Finder.FindNextFile();
		// use the .ini file instead of the registry
		free((void*)m_pszProfileName);
		m_pszProfileName=_tcsdup(_T(Finder.GetFilePath()));
		// look for the registry key for void* buffers storage ( these can't go into .INI files )
		int i=0;
		CString key;
		HKEY hkResult;
		DWORD dwDisp;
		DWORD type;
		char iBuf[3];
		do
		{
			sprintf( iBuf, "%d", i );
			key = "Software\\Q3Radiant\\IniPrefs" + CString(iBuf);
			// does this key exists ?
			if ( RegOpenKeyEx( HKEY_CURRENT_USER, key, 0, KEY_ALL_ACCESS, &hkResult ) != ERROR_SUCCESS )
			{
				// this key doesn't exist, so it's the one we'll use
				strcpy( g_qeglobals.use_ini_registry, key.GetBuffer(0) );
				RegCreateKeyEx( HKEY_CURRENT_USER, key, 0, NULL, 
					REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkResult, &dwDisp );
				RegSetValueEx( hkResult, "RadiantName", 0, REG_SZ, reinterpret_cast<CONST BYTE *>(RadiantPath), strlen( RadiantPath )+1 );
				RegCloseKey( hkResult );
				break;
			}
			else
			{
				char RadiantAux[ _MAX_PATH ];
				unsigned long size = _MAX_PATH;
				// the key exists, is it the one we are looking for ?
				RegQueryValueEx( hkResult, "RadiantName", 0, &type, reinterpret_cast<BYTE *>(RadiantAux), &size );
				RegCloseKey( hkResult );
				if ( !strcmp( RadiantAux, RadiantPath ) )
				{
					// got it !
					strcpy( g_qeglobals.use_ini_registry, key.GetBuffer(0) );
					break;
				}
			}
			i++;
		} while (1);
		g_qeglobals.use_ini = true;
	}
	else
	{
		// Change the registry key under which our settings are stored.
		// You should modify this string to be something appropriate
		// such as the name of your company or organization.
		SetRegistryKey("Q3Radiant");
		g_qeglobals.use_ini = false;
	}

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

//	CMultiDocTemplate* pDocTemplate;
//	pDocTemplate = new CMultiDocTemplate(
//		IDR_RADIANTYPE,
//		RUNTIME_CLASS(CRadiantDoc),
//		RUNTIME_CLASS(CMainFrame), // custom MDI child frame
//		RUNTIME_CLASS(CRadiantView));
//	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window

  g_PrefsDlg.LoadPrefs();

  int nMenu = IDR_MENU1;

  CString strOpenGL = (g_PrefsDlg.m_bSGIOpenGL) ? "opengl.dll" : "opengl32.dll";
  CString strGLU = (g_PrefsDlg.m_bSGIOpenGL) ? "glu.dll" : "glu32.dll";
  
  if (!QGL_Init(strOpenGL, strGLU))
  {
    g_PrefsDlg.m_bSGIOpenGL ^= 1;
    strOpenGL = (g_PrefsDlg.m_bSGIOpenGL) ? "opengl.dll" : "opengl32.dll";
    strGLU = (g_PrefsDlg.m_bSGIOpenGL) ? "glu.dll" : "glu32.dll";
    if (!QGL_Init(strOpenGL, strGLU))
    {
      AfxMessageBox("Failed to load OpenGL libraries. \"OPENGL32.DLL\" and \"OPENGL.DLL\" were tried");
      return FALSE;
    }
    g_PrefsDlg.SavePrefs();
  }

	CString strTemp = m_lpCmdLine;
  strTemp.MakeLower();
  if (strTemp.Find("builddefs") >= 0)
    g_bBuildList = true;

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(nMenu))
		return FALSE;

  if (pMainFrame->m_hAccelTable)
    ::DestroyAcceleratorTable(pMainFrame->m_hAccelTable);
  
  pMainFrame->LoadAccelTable(MAKEINTRESOURCE(IDR_MINIACCEL));

	m_pMainWnd = pMainFrame;

  // Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	//if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

  free((void*)m_pszHelpFilePath);
  CString strHelp = g_strAppPath;
  AddSlash(strHelp);
  strHelp += "Q3RManual.chm";
  m_pszHelpFilePath= _tcsdup(strHelp);


	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CRadiantApp commands

int CRadiantApp::ExitInstance() 
{
	// TODO: Add your specialized code here and/or call the base class
  //::FreeLibrary(g_hOpenGL32);
	QGL_Shutdown();
	return CWinApp::ExitInstance();
}

BOOL CRadiantApp::OnIdle(LONG lCount) 
{
	if (g_pParentWnd)
    g_pParentWnd->RoutineProcessing();
	return CWinApp::OnIdle(lCount);
}

void CRadiantApp::OnHelp() 
{
  ShellExecute(m_pMainWnd->GetSafeHwnd(), "open", m_pszHelpFilePath, NULL, NULL, SW_SHOW);
}
