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
#include <process.h>
#include "mru.h"
#include "entityw.h"
#include "PrefsDlg.h"

static HWND      s_hwndToolbar;

BOOL SaveRegistryInfo(const char *pszName, void *pvBuf, long lSize);
BOOL LoadRegistryInfo(const char *pszName, void *pvBuf, long *plSize);

static HWND CreateMyStatusWindow(HINSTANCE hInst);
static HWND CreateToolBar(HINSTANCE hinst);

extern void WXY_Print( void );

/*
==============================================================================

  MENU

==============================================================================
*/

void OpenDialog (void);
void SaveAsDialog (bool bRegion);
qboolean ConfirmModified (void);
void  Select_Ungroup (void);

void QE_ExpandBspString (char *bspaction, char *out, char *mapname, bool useTemps)
{
	char	*in;
	char	src[2048];
	char	rsh[2048];
	char	base[2048];

	strcpy(src, mapname);
	strlwr(src);
	in = strstr(src, "maps/");
	if (!in)
	{
		in = strstr(src, "maps\\");
	}
	if (in)
	{
		in += 5;
		strcpy(base, in);
		in = base;
		while (*in)
		{
			if (*in == '\\')
			{
				*in = '/';
			}
			in++;
		}
	}
	else
	{
		ExtractFileName (mapname, base);
	}

  if (useTemps) {
    CString str;
    CString strExt = "map";
    if ( strstr(mapname, ".reg") ) {
      strExt = "reg";
    }
    str.Format("%s/maps/%i.%s", ValueForKey(g_qeglobals.d_project_entity, "remotebasepath"), ::GetTickCount(), strExt);
    CopyFile(mapname, str, FALSE);
	  sprintf (src, "-tempname %s %s/maps/%s", str, ValueForKey(g_qeglobals.d_project_entity, "remotebasepath"), base);
  } else {
	  sprintf (src, "%s/maps/%s", ValueForKey(g_qeglobals.d_project_entity, "remotebasepath"), base);
  }
	strcpy (rsh, ValueForKey(g_qeglobals.d_project_entity, "rshcmd"));

  QE_ConvertDOSToUnixName(src, src);

	in = ValueForKey( g_qeglobals.d_project_entity, bspaction );
	while (*in)
	{
		if (in[0] == '!')
		{
			strcpy (out, rsh);
			out += strlen(rsh);
			in++;
			continue;
		}
		if (in[0] == '$')
		{
			strcpy (out, src);
			out += strlen(src);
			in++;
			continue;
		}
		if (in[0] == '@')
		{
			*out++ = '"';
			in++;
			continue;
		}
		*out++ = *in++;
	}
	*out = 0;
}

void FindReplace(CString& strContents, const char* pTag, const char* pValue)
{
  if (strcmp(pTag, pValue) == 0)
    return;
  for (int nPos = strContents.Find(pTag); nPos >= 0; nPos = strContents.Find(pTag))
  {
    int nRightLen = strContents.GetLength() - strlen(pTag) - nPos;
    CString strLeft = strContents.Left(nPos);
    CString strRight = strContents.Right(nRightLen);
    strLeft += pValue;
    strLeft += strRight;
    strContents = strLeft;
  }
}



HWND g_hWnd = NULL;
HANDLE g_hToolThread = NULL;
CString g_strParams;

UINT ToolThread(LPVOID pParam)
{
  char* p = reinterpret_cast<char*>(pParam);
  if (g_PrefsDlg.m_bPAK)
    RunTools(p, g_hWnd, g_PrefsDlg.m_strPAKFile);
  else
    RunTools(p, g_hWnd, "");
  g_hToolThread = NULL;
  delete []p;
  return 0;
}

void ThreadTools(char* p)
{
  CWinThread* pThread = AfxBeginThread(ToolThread, reinterpret_cast<LPVOID>(p));
  g_hToolThread = pThread->m_hThread;
}

HWND g_hwndFoundIt = NULL;

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
  char buff[1024];
  const char* p = reinterpret_cast<const char*>(lParam);
  GetWindowText(hwnd, buff, 1024);
  if (!strcmpi(p, buff)) {
    g_hwndFoundIt = hwnd;
    return 1;
  }
  return 1;
}


HWND FindAnyWindow(const char *pTitle) {
  HWND hwndDesktop = GetDesktopWindow();
  g_hwndFoundIt = NULL;
  if ( hwndDesktop ) {
    EnumChildWindows(hwndDesktop, (WNDENUMPROC)EnumChildProc, reinterpret_cast<LPARAM>(pTitle));
  }
  return g_hwndFoundIt;
}


const UINT wm_AddCommand = RegisterWindowMessage( "Q3MPC_AddCommand" );

CTime g_tBegin;
void RunBsp (char *command)
{
	char	sys[2048];
	char	batpath[2048];
	char	outputpath[2048];
	char	temppath[1024];
	char	name[2048];
	char	cWork[2048];
	FILE	*hFile;
	BOOL	ret;
	PROCESS_INFORMATION ProcessInformation;
	STARTUPINFO	startupinfo;
  HWND hwndPClient = NULL;

  g_hWnd = g_pParentWnd->GetSafeHwnd();
	SetInspectorMode(W_CONSOLE);
  g_tBegin = CTime::GetCurrentTime();

	
	DWORD	dwExitcode;
  ret = GetExitCodeProcess (g_hToolThread, &dwExitcode);
  if (dwExitcode != STILL_ACTIVE)
    g_hToolThread = NULL;

	if (bsp_process || g_hToolThread)
	{
		Sys_Printf ("BSP is still going...\n");
		return;
	}

  outputpath[0] = '\0';
	GetTempPath(512, temppath);

  CString strOutFile = temppath;
  AddSlash(strOutFile);
  strOutFile += "junk.txt";

	sprintf (outputpath, " >>%s\r\n", strOutFile);

  strcpy (name, currentmap);
	if (region_active)
	{
		Map_SaveFile (name, false);
		StripExtension (name);
		strcat (name, ".reg");
	}

	Map_SaveFile (name, region_active);

  // FIXME: this code just gets worse and worse
  CString strPath, strFile;

  char *rsh = ValueForKey(g_qeglobals.d_project_entity, "rshcmd");
  if (rsh == NULL)
  {
    ExtractPath_and_Filename(name, strPath, strFile);
    AddSlash(strPath);
    BuildShortPathName(strPath, cWork, 1024);
    strcat(cWork, strFile);
  }
  else
  {
    strcpy(cWork, name);
  }

  hwndPClient = FindWindow(NULL, "Q3Map Process Client");
  if ( hwndPClient == NULL ) {
    hwndPClient = FindAnyWindow("Q3Map Process Client");
  }

  Sys_Printf("Window info for Process Client %i\n", reinterpret_cast<int>(hwndPClient));

  bool processServer = (rsh && strlen(rsh) > 0 && hwndPClient);

  QE_ExpandBspString (command, sys, cWork, processServer);

  // if we can find the q3map process server running 
  // we will submit maps to it instead of via createprocess
  //
  if (processServer)
  {
    CString str;
    char cBuff[2048];
    char *pStart = sys;
    char *pEnd = strstr(pStart, "&&");
    while (pEnd)
    {
      int nLen = pEnd-pStart-1;
      strncpy(cBuff, pStart, nLen);
      cBuff[nLen] = 0;
      str = cBuff;
      FindReplace(str, rsh, "");
      str.TrimLeft(' ');
      str.TrimRight(' ');
      ATOM a = GlobalAddAtom(str);
      PostMessage(hwndPClient, wm_AddCommand, 0, (LPARAM)a);
      pStart = pEnd+2;
      pEnd = strstr(pStart, "&&");
    }
    str = pStart;
    FindReplace(str, rsh, "");
    str.TrimLeft(' ');
    str.TrimRight(' ');
    ATOM a = GlobalAddAtom(str);
    PostMessage(hwndPClient, wm_AddCommand, 0, (LPARAM)a);
    Sys_Printf("Commands sent to Q3Map Process Client\n");
    return;
  }

  CString strSys = sys;

  FindReplace(strSys, "&&", outputpath);
  strcpy(sys, strSys);
  strcat(sys, outputpath);

  if (g_PrefsDlg.m_bInternalBSP)
  {
    g_tBegin = CTime::GetCurrentTime();
    strSys.MakeLower();
    char* p = new char[strSys.GetLength()+1];
    strcpy(p, strSys.GetBuffer(0));
    ThreadTools(p);
  }
  else
  {
	  Sys_ClearPrintf ();
	  Sys_Printf ("==================\nRunning bsp command...\n");
	  Sys_Printf ("\n%s\n", sys);

	  //++timo removed the old way BSP commands .. dumping to junk.txt doesn't work on my win98 box
	  // FIXME : will most likely break Quake2 BSP commands, is fitted to a one-lined sys command
	  //
	  // write qe3bsp.bat
	  //

	  sprintf (batpath, "%sqe3bsp.bat", temppath);
	  hFile = fopen(batpath, "w");
	  if (!hFile)
		  Error ("Can't write to %s", batpath);
	  fprintf (hFile, sys);
	  fclose (hFile);

	  Pointfile_Delete ();

	  // delete junk.txt file
	  remove(strOutFile);

	  GetStartupInfo (&startupinfo);

	  ret = CreateProcess(
		  batpath,
		  NULL,
		  NULL,
		  NULL,
		  FALSE,
		  0,
		  NULL,
		  NULL,
		  &startupinfo,
		  &ProcessInformation
		  );

	  if (!ret)
		  Error ("CreateProcess failed");

	  bsp_process = ProcessInformation.hProcess;

	  Sleep (100);	// give the new process a chance to open it's window

	  BringWindowToTop( g_qeglobals.d_hwndMain );	// pop us back on top
#if 0
	  //
	  // write qe3bsp.bat
	  //
	  sprintf (batpath, "%sqe3bsp.bat", temppath);
	  hFile = fopen(batpath, "w");
	  if (!hFile)
		  Error ("Can't write to %s", batpath);
	  fprintf (hFile, sys);
	  fclose (hFile);

	  //
	  // write qe3bsp2.bat
	  //
	  sprintf (batpath, "%sqe3bsp2.bat", temppath);
	  hFile = fopen(batpath, "w");
	  if (!hFile)
		  Error ("Can't write to %s", batpath);
	  fprintf (hFile, "%sqe3bsp.bat > %s", temppath, outputpath);
	  fclose (hFile);

	  Pointfile_Delete ();

	  GetStartupInfo (&startupinfo);

	  ret = CreateProcess(
      batpath,		// pointer to name of executable module 
      NULL,			// pointer to command line string
      NULL,			// pointer to process security attributes 
      NULL,			// pointer to thread security attributes 
      FALSE,			// handle inheritance flag 
      0 /*DETACHED_PROCESS*/,		// creation flags
      NULL,			// pointer to new environment block 
      NULL,			// pointer to current directory name 
      &startupinfo,	// pointer to STARTUPINFO 
      &ProcessInformation 	// pointer to PROCESS_INFORMATION  
     );

	  if (!ret)
		  Error ("CreateProcess failed");

	  bsp_process = ProcessInformation.hProcess;

	  Sleep (100);	// give the new process a chance to open it's window

	  //BringWindowToTop( g_qeglobals.d_hwndMain );	// pop us back on top
	  //SetFocus (g_qeglobals.d_hwndCamera);
#endif
  }
}

void DLLBuildDone()
{
  g_hToolThread = NULL;
  CTime tEnd = CTime::GetCurrentTime();
  CTimeSpan tElapsed = tEnd - g_tBegin;
  CString strElapsed;
  strElapsed.Format("Run time was %i hours, %i minutes and %i seconds", tElapsed.GetHours(), tElapsed.GetMinutes(), tElapsed.GetSeconds());
	Sys_Printf(strElapsed.GetBuffer(0));
	Pointfile_Check();

  if (g_PrefsDlg.m_bRunQuake == TRUE)
  {
    char cCurDir[1024];
    GetCurrentDirectory(1024, cCurDir);
    CString strExePath = g_PrefsDlg.m_strQuake2;
    CString strOrgPath;
    CString strOrgFile;
    ExtractPath_and_Filename(currentmap, strOrgPath, strOrgFile);
    if (g_PrefsDlg.m_bSetGame == TRUE) // run in place with set game.. don't copy map
    {
	    CString strBasePath = ValueForKey(g_qeglobals.d_project_entity, "basepath");
      strExePath += " +set game ";
      strExePath += strBasePath;
      WinExec(strExePath, SW_SHOW);
    }
    else
    {
      CString strCopyPath = strExePath;
      char* pBuffer = strCopyPath.GetBufferSetLength(_MAX_PATH + 1);
      pBuffer[strCopyPath.ReverseFind('\\') + 1] = '\0';
      strCopyPath.ReleaseBuffer();
      SetCurrentDirectory(strCopyPath);
      CString strOrgPath;
      CString strOrgFile;
      ExtractPath_and_Filename(currentmap, strOrgPath, strOrgFile);
      AddSlash(strCopyPath);
      FindReplace(strOrgFile, ".map", ".bsp");
      strCopyPath += "\\baseq2\\maps\\";
      strCopyPath += strOrgFile;
      AddSlash(strOrgPath);
      strOrgPath += strOrgFile;
      bool bRun = (strOrgPath.CompareNoCase(strCopyPath) == 0);
      if (!bRun)
        bRun = (CopyFile(strOrgPath, strCopyPath, FALSE) == TRUE);
      if (bRun)
      {
        FindReplace(strOrgFile, ".bsp", "");
        strExePath += " +map ";
        strExePath += strOrgFile;
        WinExec(strExePath, SW_SHOW);
      }
    }
    SetCurrentDirectory(cCurDir);
  }

}

/*
=============
DoColor

=============
*/

class CMyColorDialog : public CColorDialog 
{
  DECLARE_DYNCREATE(CMyColorDialog);
     // Construction
public:
     CMyColorDialog( COLORREF clrInit = 0, DWORD dwFlags = 0, CWnd*
pParentWnd = NULL );
     // Statics
protected:
     enum { NCUSTCOLORS = 16 };
     static COLORREF c_CustColors[NCUSTCOLORS];
     static COLORREF c_LastCustColors[NCUSTCOLORS];
     static bool c_NeedToInitCustColors;
protected:
     static void InitCustColors();
     static void SaveCustColors();
     // Dialog Data
protected:
     //{{AFX_DATA(CMyColorDialog)
     //}}AFX_DATA
     // Overrides
protected:
     // ClassWizard generate virtual function overrides
     //{{AFX_VIRTUAL(CMyColorDialog)
public:
     virtual int DoModal();
protected:
     virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
     //}}AFX_VIRTUAL
     // Implementation
protected:
     // Generated message map functions
     //{{AFX_MSG(CMyColorDialog)
     //}}AFX_MSG
     DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNCREATE(CMyColorDialog, CColorDialog)

bool CMyColorDialog::c_NeedToInitCustColors = true;
COLORREF CMyColorDialog::c_CustColors[];
COLORREF CMyColorDialog::c_LastCustColors[];

#define SECTION _T("Custom Colors")

void CMyColorDialog::InitCustColors() {
     for (int i = 0; i < NCUSTCOLORS; i++) {
          CString entry; entry.Format("%d",i);
          c_LastCustColors[i] = c_CustColors[i] =
		::AfxGetApp()->GetProfileInt(SECTION,entry,RGB(255,255,255));
     }
     c_NeedToInitCustColors= false;
}

void CMyColorDialog::SaveCustColors() {
     for (int i = 0; i < NCUSTCOLORS; i++) {
          if (c_LastCustColors[i] != c_CustColors[i]) {
               CString entry; entry.Format("%d",i);
               if (c_CustColors[i] == RGB(255,255,255)) {
                    ::AfxGetApp()->WriteProfileString(SECTION,entry,NULL);
               } else {
                    ::AfxGetApp()->WriteProfileInt(SECTION, entry,c_CustColors[i]);
               }
               c_LastCustColors[i] = c_CustColors[i];
          }
     }
}

CMyColorDialog::CMyColorDialog( COLORREF clrInit, DWORD dwFlags, 
		CWnd* pParentWnd) : CColorDialog(clrInit,dwFlags,pParentWnd)
{
     //{{AFX_DATA_INIT(CMyColorDialog)
     //}}AFX_DATA_INIT
     if (c_NeedToInitCustColors) {
          InitCustColors();
     }
     m_cc.lpCustColors = c_CustColors;
}

int CMyColorDialog::DoModal() {
     int code = CColorDialog::DoModal();
     SaveCustColors();
     return code;
}

void CMyColorDialog::DoDataExchange(CDataExchange* pDX) {
     // overridden (calls this base class)
     CColorDialog::DoDataExchange(pDX);
     //{{AFX_DATA_MAP(CMyColorDialog)
     //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMyColorDialog, CColorDialog)
//{{AFX_MSG_MAP(CMyColorDialog)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void DoNewColor(int* i1, int* i2, int* i3)
{
	COLORREF cr = (*i1) +
                ((*i2) <<8) +
		            ((*i3) <<16);
  CMyColorDialog dlg(cr, CC_FULLOPEN | CC_RGBINIT);
  if (dlg.DoModal() == IDOK)
  {
	  *i1 = (dlg.m_cc.rgbResult & 255);
	  *i2 = ((dlg.m_cc.rgbResult >> 8) & 255);
	  *i3 = ((dlg.m_cc.rgbResult >> 16) & 255);
  }

}


qboolean DoColor(int iIndex)
{

	COLORREF cr = (int)(g_qeglobals.d_savedinfo.colors[iIndex][0]*255) +
                (((int)(g_qeglobals.d_savedinfo.colors[iIndex][1]*255))<<8) +
		            (((int)(g_qeglobals.d_savedinfo.colors[iIndex][2]*255))<<16);
  CMyColorDialog dlg(cr, CC_FULLOPEN | CC_RGBINIT);
  if (dlg.DoModal() == IDOK)
  {
	  g_qeglobals.d_savedinfo.colors[iIndex][0] = (dlg.m_cc.rgbResult&255)/255.0;
	  g_qeglobals.d_savedinfo.colors[iIndex][1] = ((dlg.m_cc.rgbResult>>8)&255)/255.0;
	  g_qeglobals.d_savedinfo.colors[iIndex][2] = ((dlg.m_cc.rgbResult>>16)&255)/255.0;

	  /* 
	  ** scale colors so that at least one component is at 1.0F 
	  ** if this is meant to select an entity color
	  */
	  if ( iIndex == COLOR_ENTITY )
	  {
		  float largest = 0.0F;

		  if ( g_qeglobals.d_savedinfo.colors[iIndex][0] > largest )
			  largest = g_qeglobals.d_savedinfo.colors[iIndex][0];
		  if ( g_qeglobals.d_savedinfo.colors[iIndex][1] > largest )
			  largest = g_qeglobals.d_savedinfo.colors[iIndex][1];
		  if ( g_qeglobals.d_savedinfo.colors[iIndex][2] > largest )
			  largest = g_qeglobals.d_savedinfo.colors[iIndex][2];

		  if ( largest == 0.0F )
		  {
			  g_qeglobals.d_savedinfo.colors[iIndex][0] = 1.0F;
			  g_qeglobals.d_savedinfo.colors[iIndex][1] = 1.0F;
			  g_qeglobals.d_savedinfo.colors[iIndex][2] = 1.0F;
		  }
		  else
		  {
			  float scaler = 1.0F / largest;

			  g_qeglobals.d_savedinfo.colors[iIndex][0] *= scaler;
			  g_qeglobals.d_savedinfo.colors[iIndex][1] *= scaler;
			  g_qeglobals.d_savedinfo.colors[iIndex][2] *= scaler;
		  }
	  }

	  Sys_UpdateWindows (W_ALL);
	  return true;
  }
  else return false;

}


/* Copied from MSDN */

BOOL DoMru(HWND hWnd,WORD wId)
{
	char szFileName[128];
	OFSTRUCT of;
	BOOL fExist;

	GetMenuItem(g_qeglobals.d_lpMruMenu, wId, TRUE, szFileName, sizeof(szFileName));

	// Test if the file exists.

	fExist = OpenFile(szFileName ,&of,OF_EXIST) != HFILE_ERROR;

	if (fExist) {

		// Place the file on the top of MRU.
		AddNewItem(g_qeglobals.d_lpMruMenu,(LPSTR)szFileName);

		// Now perform opening this file !!!
		Map_LoadFile (szFileName);	
	}
	else
		// Remove the file on MRU.
		DelMenuItem(g_qeglobals.d_lpMruMenu,wId,TRUE);

	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(hWnd),0),
			ID_FILE_EXIT);

	return fExist;
}



/*
==============
Main_Create
==============
*/

void MFCCreate (HINSTANCE hInstance)
{
	HMENU hMenu = NULL;
	int i = sizeof(g_qeglobals.d_savedinfo);
  long l = i;

  g_qeglobals.d_savedinfo.exclude |= (EXCLUDE_HINT | EXCLUDE_CLIP);
	LoadRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, &l);

  int nOldSize = g_qeglobals.d_savedinfo.iSize;
	if (g_qeglobals.d_savedinfo.iSize != sizeof(g_qeglobals.d_savedinfo))
	{
		// fill in new defaults
		g_qeglobals.d_savedinfo.iSize = sizeof(g_qeglobals.d_savedinfo);
		g_qeglobals.d_savedinfo.fGamma = 1.0;
		g_qeglobals.d_savedinfo.iTexMenu = ID_VIEW_BILINEARMIPMAP;
    g_qeglobals.d_savedinfo.m_nTextureTweak = 1;
  
		//g_qeglobals.d_savedinfo.exclude = INCLUDE_EASY | INCLUDE_NORMAL | INCLUDE_HARD | INCLUDE_DEATHMATCH;
		g_qeglobals.d_savedinfo.show_coordinates = true;
		g_qeglobals.d_savedinfo.show_names       = false;

		for (i=0 ; i<3 ; i++)
		{
			g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][i] = 0.25;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDBACK][i] = 1.0;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDMINOR][i] = 0.75;
			g_qeglobals.d_savedinfo.colors[COLOR_GRIDMAJOR][i] = 0.5;
			g_qeglobals.d_savedinfo.colors[COLOR_CAMERABACK][i] = 0.25;
		}

		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDBLOCK][2] = 1.0;

		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_GRIDTEXT][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][0] = 1.0;
		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_SELBRUSHES][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_CLIPPER][2] = 1.0;

		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][0] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_BRUSHES][2] = 0.0;

		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][0] = 0.5;
		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][1] = 0.0;
		g_qeglobals.d_savedinfo.colors[COLOR_VIEWNAME][2] = 0.75;


    // old size was smaller, reload original prefs
    if (nOldSize < sizeof(g_qeglobals.d_savedinfo))
    {
      long l = nOldSize;
	    LoadRegistryInfo("SavedInfo", &g_qeglobals.d_savedinfo, &l);
    }

	}
	if ( ( hMenu = GetMenu( g_qeglobals.d_hwndMain ) ) != 0 )
	{
		// by default all of these are checked because that's how they're defined in the menu editor
		if ( !g_qeglobals.d_savedinfo.show_names )
			CheckMenuItem( hMenu, ID_VIEW_SHOWNAMES, MF_BYCOMMAND | MF_UNCHECKED );
		if ( !g_qeglobals.d_savedinfo.show_coordinates )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCOORDINATES, MF_BYCOMMAND | MF_UNCHECKED );

		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_LIGHTS )
			CheckMenuItem( hMenu, ID_VIEW_SHOWLIGHTS, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_ENT )
			CheckMenuItem( hMenu, ID_VIEW_ENTITY, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_PATHS )
			CheckMenuItem( hMenu, ID_VIEW_SHOWPATH, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WATER )
			CheckMenuItem( hMenu, ID_VIEW_SHOWWATER, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_WORLD )
			CheckMenuItem( hMenu, ID_VIEW_SHOWWORLD, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CLIP )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCLIP, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_HINT )
			CheckMenuItem( hMenu, ID_VIEW_SHOWHINT, MF_BYCOMMAND | MF_UNCHECKED );
		if ( g_qeglobals.d_savedinfo.exclude & EXCLUDE_CAULK )
			CheckMenuItem( hMenu, ID_VIEW_SHOWCAULK, MF_BYCOMMAND | MF_UNCHECKED );
	}

}


/*
=============================================================

REGISTRY INFO

=============================================================
*/

BOOL SaveRegistryInfo(const char *pszName, void *pvBuf, long lSize)
{
	LONG lres;
	DWORD dwDisp;
	HKEY  hKeyId;

	if (g_qeglobals.use_ini)
	{
		lres = RegCreateKeyEx(HKEY_CURRENT_USER, g_qeglobals.use_ini_registry, 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyId, &dwDisp);
	}
	else
	{
		lres = RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Q3Radiant\\Q3Radiant", 0, NULL, 
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyId, &dwDisp);
	}
    
	if (lres != ERROR_SUCCESS)
		return FALSE;

	lres = RegSetValueEx(hKeyId, pszName, 0, REG_BINARY, (unsigned char*)pvBuf, lSize);
    
	RegCloseKey(hKeyId);

	if (lres != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOL LoadRegistryInfo(const char *pszName, void *pvBuf, long *plSize)
{
	HKEY  hKey;
	long lres, lType, lSize;

	if (plSize == NULL)
		plSize = &lSize;

	if (g_qeglobals.use_ini)
	{
		lres = RegOpenKeyEx(HKEY_CURRENT_USER, g_qeglobals.use_ini_registry, 0, KEY_READ, &hKey);
	}
	else
	{
		lres = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Q3Radiant\\Q3Radiant", 0, KEY_READ, &hKey);
  }

	lres = RegQueryValueEx(hKey, pszName, NULL, (unsigned long*)&lType, (unsigned char*)pvBuf, (unsigned long*)plSize);

	RegCloseKey(hKey);

	if (lres != ERROR_SUCCESS)
	{
#ifdef _DEBUG
		char Message[1024];
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, 0, lres, 0, &(Message[0]), 1024, NULL );
		Sys_Printf( "WARNING: RegQueryValueEx failed in LoadRegistryInfo for %s : %s", pszName, Message );
#endif
		return FALSE;
	}

	return TRUE;
}

BOOL SaveWindowState(HWND hWnd, const char *pszName)
{
	RECT rc;
	GetWindowRect(hWnd, &rc);
	if (hWnd != g_qeglobals.d_hwndMain) // && g_pParentWnd->CurrentStyle() == QR_QE4)
  {
    if (::GetParent(hWnd) != g_qeglobals.d_hwndMain)
    {
      ::SetParent(hWnd, g_qeglobals.d_hwndMain);
    }
		MapWindowPoints(NULL, g_qeglobals.d_hwndMain, (POINT *)&rc, 2);

  }
	BOOL b = SaveRegistryInfo(pszName, &rc, sizeof(rc));
  return b;
}


BOOL LoadWindowState(HWND hWnd, const char *pszName)
{
	RECT rc;
  LONG lSize = sizeof(rc);

	if (LoadRegistryInfo(pszName, &rc, &lSize))
	{
		if (rc.left < 0)
			rc.left = 0;
		if (rc.top < 0)
			rc.top = 0;
		if (rc.right < rc.left + 16)
			rc.right = rc.left + 16;
		if (rc.bottom < rc.top + 16)
			rc.bottom = rc.top + 16;

		MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, 
				rc.bottom - rc.top, FALSE);
		return TRUE;
	}

	return FALSE;
}

/*
===============================================================

  STATUS WINDOW

===============================================================
*/

void Sys_UpdateStatusBar( void )
{
	extern int   g_numbrushes, g_numentities;

	char numbrushbuffer[100]="";

	sprintf( numbrushbuffer, "Brushes: %d Entities: %d", g_numbrushes, g_numentities );
  g_pParentWnd->SetStatusText(2, numbrushbuffer);
	//Sys_Status( numbrushbuffer, 2 );
}

void Sys_Status(const char *psz, int part )
{
	SendMessage(g_qeglobals.d_hwndStatus, SB_SETTEXT, part, (LPARAM)psz);
}

static HWND CreateMyStatusWindow(HINSTANCE hInst)
{
	HWND hWnd;
	int partsize[3] = { 300, 1100, -1 };

	hWnd = CreateWindowEx( WS_EX_TOPMOST, // no extended styles
            STATUSCLASSNAME,                 // status bar
            "",                              // no text 
            WS_CHILD | WS_BORDER | WS_VISIBLE,  // styles
            -100, -100, 10, 10,              // x, y, cx, cy
            g_qeglobals.d_hwndMain,          // parent window
            (HMENU)100,                      // window ID
            hInst,                           // instance
            NULL);							 // window data

	SendMessage( hWnd, SB_SETPARTS, 3, ( long ) partsize );
    
	return hWnd;
}

//==============================================================

#define NUMBUTTONS 15
HWND CreateToolBar(HINSTANCE hinst)
{ 
    HWND hwndTB; 
    TBADDBITMAP tbab; 
    TBBUTTON tbb[NUMBUTTONS]; 
    
     // Ensure that the common control DLL is loaded. 

    InitCommonControls(); 
 
    // Create a toolbar that the user can customize and that has a 
    // tooltip associated with it. 

    hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, (LPSTR) NULL, 
        WS_CHILD | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE | WS_BORDER, 
        0, 0, 0, 0, g_qeglobals.d_hwndMain, (HMENU) IDR_TOOLBAR1, hinst, NULL); 
 
    // Send the TB_BUTTONSTRUCTSIZE message, which is required for 
    // backward compatibility. 

    SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
 
    // Add the bitmap containing button images to the toolbar. 

    tbab.hInst = hinst; 
    tbab.nID   = IDR_TOOLBAR1; 
    SendMessage(hwndTB, TB_ADDBITMAP, (WPARAM)NUMBUTTONS, (WPARAM) &tbab); 
 
    // Fill the TBBUTTON array with button information, and add the 
    // buttons to the toolbar. 

    tbb[0].iBitmap = 0; 
    tbb[0].idCommand = ID_BRUSH_FLIPX; 
    tbb[0].fsState = TBSTATE_ENABLED; 
    tbb[0].fsStyle = TBSTYLE_BUTTON; 
    tbb[0].dwData = 0; 
    tbb[0].iString = 0; 
 
    tbb[1].iBitmap = 2; 
    tbb[1].idCommand = ID_BRUSH_FLIPY; 
    tbb[1].fsState = TBSTATE_ENABLED; 
    tbb[1].fsStyle = TBSTYLE_BUTTON; 
    tbb[1].dwData = 0; 
    tbb[1].iString = 0; 
 
    tbb[2].iBitmap = 4; 
    tbb[2].idCommand = ID_BRUSH_FLIPZ; 
    tbb[2].fsState = TBSTATE_ENABLED;
    tbb[2].fsStyle = TBSTYLE_BUTTON; 
    tbb[2].dwData = 0; 
    tbb[2].iString = 0; 
 
    tbb[3].iBitmap = 1; 
    tbb[3].idCommand = ID_BRUSH_ROTATEX; 
    tbb[3].fsState = TBSTATE_ENABLED; 
    tbb[3].fsStyle = TBSTYLE_BUTTON; 
    tbb[3].dwData = 0; 
    tbb[3].iString = 0; 
 
    tbb[4].iBitmap = 3; 
    tbb[4].idCommand = ID_BRUSH_ROTATEY; 
    tbb[4].fsState = TBSTATE_ENABLED; 
    tbb[4].fsStyle = TBSTYLE_BUTTON; 
    tbb[4].dwData = 0; 
    tbb[4].iString = 0; 

    tbb[5].iBitmap = 5; 
    tbb[5].idCommand = ID_BRUSH_ROTATEZ; 
    tbb[5].fsState = TBSTATE_ENABLED; 
    tbb[5].fsStyle = TBSTYLE_BUTTON; 
    tbb[5].dwData = 0; 
    tbb[5].iString = 0; 

    tbb[6].iBitmap = 6; 
    tbb[6].idCommand = ID_SELECTION_SELECTCOMPLETETALL; 
    tbb[6].fsState = TBSTATE_ENABLED; 
    tbb[6].fsStyle = TBSTYLE_BUTTON; 
    tbb[6].dwData = 0; 
    tbb[6].iString = 0; 

    tbb[7].iBitmap = 7; 
    tbb[7].idCommand = ID_SELECTION_SELECTTOUCHING; 
    tbb[7].fsState = TBSTATE_ENABLED; 
    tbb[7].fsStyle = TBSTYLE_BUTTON; 
    tbb[7].dwData = 0; 
    tbb[7].iString = 0; 

    tbb[8].iBitmap = 8; 
    tbb[8].idCommand = ID_SELECTION_SELECTPARTIALTALL; 
    tbb[8].fsState = TBSTATE_ENABLED; 
    tbb[8].fsStyle = TBSTYLE_BUTTON; 
    tbb[8].dwData = 0; 
    tbb[8].iString = 0; 


    tbb[9].iBitmap = 9; 
    tbb[9].idCommand = ID_SELECTION_SELECTINSIDE; 
    tbb[9].fsState = TBSTATE_ENABLED; 
    tbb[9].fsStyle = TBSTYLE_BUTTON; 
    tbb[9].dwData = 0; 
    tbb[9].iString = 0; 

    tbb[10].iBitmap = 10; 
    tbb[10].idCommand = ID_SELECTION_CSGSUBTRACT; 
    tbb[10].fsState = TBSTATE_ENABLED; 
    tbb[10].fsStyle = TBSTYLE_BUTTON; 
    tbb[10].dwData = 0; 
    tbb[10].iString = 0; 


    tbb[11].iBitmap = 11;
    tbb[11].idCommand = ID_SELECTION_MAKEHOLLOW; 
    tbb[11].fsState = TBSTATE_ENABLED; 
    tbb[11].fsStyle = TBSTYLE_BUTTON; 
    tbb[11].dwData = 0; 
    tbb[11].iString = 0; 

    tbb[12].iBitmap = 12;
    tbb[12].idCommand = ID_TEXTURES_WIREFRAME; 
    tbb[12].fsState = TBSTATE_ENABLED; 
    tbb[12].fsStyle = TBSTYLE_BUTTON; 
    tbb[12].dwData = 0; 
    tbb[12].iString = 0; 

    tbb[13].iBitmap = 13;
    tbb[13].idCommand = ID_TEXTURES_FLATSHADE; 
    tbb[13].fsState = TBSTATE_ENABLED; 
    tbb[13].fsStyle = TBSTYLE_BUTTON; 
    tbb[13].dwData = 0; 
    tbb[13].iString = 0; 

    tbb[14].iBitmap = 14;
    tbb[14].idCommand = ID_VIEW_TRILINEAR; 
    tbb[14].fsState = TBSTATE_ENABLED; 
    tbb[14].fsStyle = TBSTYLE_BUTTON; 
    tbb[14].dwData = 0; 
    tbb[14].iString = 0; 

    SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM)NUMBUTTONS,
        (LPARAM) (LPTBBUTTON) &tbb); 
 
    ShowWindow(hwndTB, SW_SHOW); 

    return hwndTB; 
} 

