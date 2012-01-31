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
#include "mru.h"
#include "PrefsDlg.h"

////////////////////////////////////////////////////////////////////////////
// BSP frontend plugin
// global flag for BSP frontend plugin is g_qeglobals.bBSPFrontendPlugin
_QERPlugBSPFrontendTable g_BSPFrontendTable;
CStringArray g_BSPFrontendCommands;

extern CEdit* g_pEdit;

int	screen_width;
int	screen_height;
qboolean	have_quit;

int	update_bits;

HANDLE	bsp_process;

//===========================================

void Sys_MarkMapModified (void)
{
	char			title[1024];

	if (modified != 1)
	{
		modified = true;	// mark the map as changed
		sprintf (title, "%s *", currentmap);

		QE_ConvertDOSToUnixName( title, title );
		Sys_SetTitle (title);
	}
}


void Sys_SetTitle (char *text)
{
	SetWindowText (g_qeglobals.d_hwndMain, text);
}

HCURSOR	waitcursor;

void Sys_BeginWait (void)
{
	waitcursor = SetCursor (LoadCursor (NULL, IDC_WAIT));
}

void Sys_EndWait (void)
{
	if (waitcursor)
	{
		SetCursor (waitcursor);
		waitcursor = NULL;
	}
}


void Sys_GetCursorPos (int *x, int *y)
{
	POINT lpPoint;

	GetCursorPos (&lpPoint);
	*x = lpPoint.x;
	*y = lpPoint.y;
}

void Sys_SetCursorPos (int x, int y)
{
	SetCursorPos (x, y);
}


void Sys_Beep (void)
{
	MessageBeep (MB_ICONASTERISK);
}

char	*TranslateString (char *buf)
{
	static	char	buf2[32768];
	int		i, l;
	char	*out;

	l = strlen(buf);
	out = buf2;
	for (i=0 ; i<l ; i++)
	{
		if (buf[i] == '\n')
		{
			*out++ = '\r';
			*out++ = '\n';
		}
		else
			*out++ = buf[i];
	}
	*out++ = 0;

	return buf2;
}


void Sys_ClearPrintf (void)
{
	char	text[4];
	text[0] = 0;

  SendMessage (g_qeglobals.d_hwndEdit, WM_SETTEXT, 0,	(LPARAM)text);
}


#define SCROLLBACK_MAX_LINES	600
#define SCROLLBACK_DEL_CHARS	500

void Sys_Printf (char *text, ...)
{
	va_list argptr;
	char	buf[32768];
	char	*out;
	LRESULT	result;				// PGM
	DWORD	oldPosS, oldPosE;	// PGM

	va_start (argptr,text);
	vsprintf (buf, text,argptr);
	va_end (argptr);

	out = TranslateString (buf);

#ifdef LATER
	Sys_Status(out);
#else
//PGM
	result = SendMessage (g_qeglobals.d_hwndEdit, EM_GETLINECOUNT, 0, 0);
	if(result > SCROLLBACK_MAX_LINES)
	{
		char	replaceText[5];
		
		replaceText[0] = '\0';

		SendMessage (g_qeglobals.d_hwndEdit, WM_SETREDRAW, (WPARAM)0, (LPARAM)0);
		SendMessage (g_qeglobals.d_hwndEdit, EM_GETSEL, (WPARAM)&oldPosS, (LPARAM)&oldPosE);
		SendMessage (g_qeglobals.d_hwndEdit, EM_SETSEL, 0, SCROLLBACK_DEL_CHARS);
		SendMessage (g_qeglobals.d_hwndEdit, EM_REPLACESEL, (WPARAM)0, (LPARAM)replaceText);
		SendMessage (g_qeglobals.d_hwndEdit, EM_SETSEL, oldPosS, oldPosE);
		SendMessage (g_qeglobals.d_hwndEdit, WM_SETREDRAW, (WPARAM)1, (LPARAM)0);
	}
//PGM

	SendMessage (g_qeglobals.d_hwndEdit, EM_REPLACESEL, 0, (LPARAM)out);
#endif

}


double Sys_DoubleTime (void)
{
	return clock()/ 1000.0;
}

void PrintPixels (HDC hDC)
{
	int		i;
	PIXELFORMATDESCRIPTOR p[64];

	printf ("### flags color layer\n");
	for (i=1 ; i<64 ; i++)
	{
		if (!DescribePixelFormat ( hDC, i, sizeof(p[0]), &p[i]))
			break;
		printf ("%3i %5i %5i %5i\n", i,
			p[i].dwFlags,
			p[i].cColorBits,
			p[i].bReserved);
	}
	printf ("%i modes\n", i-1);
}



//==========================================================================

void QEW_StopGL( HWND hWnd, HGLRC hGLRC, HDC hDC )
{
  qwglMakeCurrent( NULL, NULL );
  qwglDeleteContext( hGLRC );
	ReleaseDC( hWnd, hDC );
}
		
int WINAPI QEW_SetupPixelFormat(HDC hDC, qboolean zbuffer )
{
    static PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								              // version number
		PFD_DRAW_TO_WINDOW |			      // support window
		PFD_SUPPORT_OPENGL |			      // support OpenGL
		PFD_DOUBLEBUFFER,				        // double buffered
		PFD_TYPE_RGBA,					        // RGBA type
		24,								              // 24-bit color depth
		0, 0, 0, 0, 0, 0,	              // color bits ignored
		0,								              // no alpha buffer
		0,								              // shift bit ignored
		0,								              // no accumulation buffer
		0, 0, 0, 0,						          // accum bits ignored
		32,							                // depth bits
		0,								              // no stencil buffer
		0,								              // no auxiliary buffer
		PFD_MAIN_PLANE,					        // main layer
		0,								              // reserved
		0, 0, 0							            // layer masks ignored
    };                              //
    int pixelformat = 0;            

	zbuffer = true;
	if ( !zbuffer )
		pfd.cDepthBits = 0;

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
    if ( (pixelformat = qwglChoosePixelFormat(hDC, &pfd)) == 0 )
    {
	    printf("%d",GetLastError());
      Error ("ChoosePixelFormat failed");
    }
  
    if (!qwglSetPixelFormat(hDC, pixelformat, &pfd))
      Error ("SetPixelFormat failed");
  }
  else
  {
    if ( (pixelformat = ChoosePixelFormat(hDC, &pfd)) == 0 )
    {
	    printf("%d",GetLastError());
      Error ("ChoosePixelFormat failed");
    }

    if (!SetPixelFormat(hDC, pixelformat, &pfd))
      Error ("SetPixelFormat failed");
  }

	return pixelformat;
}

/*
=================
Error

For abnormal program terminations
=================
*/
void Error (char *error, ...)
{
	va_list argptr;
	char	text[1024];
	char	text2[1024];
	int		err;
	
	err = GetLastError ();
	int i = qglGetError();
	
	va_start (argptr,error);
	vsprintf (text, error,argptr);
	va_end (argptr);
	
	sprintf (text2, "%s\nGetLastError() = %i - %i\nAn unrecoverable error has occured. Would you like to edit Preferences before exiting Q3Radiant?", text, err, i);
	
	if (MessageBox(g_qeglobals.d_hwndMain, text2, "Error", MB_YESNO) == IDYES)
	{
		g_PrefsDlg.LoadPrefs();
		g_PrefsDlg.DoModal();
	}
	
	exit (1);
}


void Warning (char *error, ...)
{
	va_list argptr;
	char	text[1024];
	int		err;

	err = GetLastError ();
  int i = qglGetError();

	va_start (argptr,error);
	vsprintf (text, error,argptr);
	va_end (argptr);

  Sys_Printf(text);
}


/*
======================================================================

FILE DIALOGS

======================================================================
*/
 
qboolean ConfirmModified (void)
{
	if (!modified)
		return true;

	if (MessageBox (g_qeglobals.d_hwndMain, "This will lose changes to the map"
		, "warning", MB_OKCANCEL) == IDCANCEL)
		return false;
	return true;
}

static OPENFILENAME ofn;       /* common dialog box structure   */ 
static char szDirName[MAX_PATH];    /* directory string              */ 
static char szFile[260];       /* filename string               */ 
static char szFileTitle[260];  /* file title string             */ 
static char szFilter[260] =     /* filter string                 */ 
	"Map file (*.map, *.reg)\0*.map\0*.reg\0\0";
static char szProjectFilter[260] =     /* filter string                 */ 
	"Q3Radiant project (*.qe4, *.prj)\0*.qe4\0*.prj\0\0";
static char chReplace;         /* string separator for szFilter */ 
static int i, cbString;        /* integer count variables       */ 
static HANDLE hf;              /* file handle                   */ 

void OpenDialog (void)
{
	/* 
	 * Obtain the system directory name and 
	 * store it in szDirName. 
	 */ 
 
	strcpy (szDirName, ValueForKey (g_qeglobals.d_project_entity, "mapspath") );
  if (strlen(szDirName) == 0)
  {
	  strcpy (szDirName, ValueForKey (g_qeglobals.d_project_entity, "basepath") );
	  strcat (szDirName, "\\maps");
  }

	/* Place the terminating null character in the szFile. */ 
 
	szFile[0] = '\0'; 
 
	/* Set the members of the OPENFILENAME structure. */ 
 
	ofn.lStructSize = sizeof(OPENFILENAME); 
	ofn.hwndOwner = g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter = szFilter; 
	ofn.nFilterIndex = 1; 
	ofn.lpstrFile = szFile; 
	ofn.nMaxFile = sizeof(szFile); 
	ofn.lpstrFileTitle = szFileTitle; 
	ofn.nMaxFileTitle = sizeof(szFileTitle); 
	ofn.lpstrInitialDir = szDirName; 
	ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | 
		OFN_FILEMUSTEXIST; 

	/* Display the Open dialog box. */ 
 
	if (!GetOpenFileName(&ofn))
		return;	// canceled
 
	// Add the file in MRU.
  //FIXME
	AddNewItem( g_qeglobals.d_lpMruMenu, ofn.lpstrFile);

	// Refresh the File menu.
  //FIXME
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(g_qeglobals.d_hwndMain),0), 
			ID_FILE_EXIT);

	/* Open the file. */ 
 
	Map_LoadFile (ofn.lpstrFile);	
}

void ProjectDialog (void)
{
	/* 
	 * Obtain the system directory name and 
	 * store it in szDirName. 
	 */ 
 
	strcpy (szDirName, ValueForKey(g_qeglobals.d_project_entity, "basepath") );
	strcat (szDirName, "\\scripts");

	/* Place the terminating null character in the szFile. */ 
 
	szFile[0] = '\0'; 
 
	/* Set the members of the OPENFILENAME structure. */ 
 
	ofn.lStructSize = sizeof(OPENFILENAME); 
	ofn.hwndOwner = g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter = szProjectFilter; 
	ofn.nFilterIndex = 1; 
	ofn.lpstrFile = szFile; 
	ofn.nMaxFile = sizeof(szFile); 
	ofn.lpstrFileTitle = szFileTitle; 
	ofn.nMaxFileTitle = sizeof(szFileTitle); 
	ofn.lpstrInitialDir = szDirName; 
	ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | 
		OFN_FILEMUSTEXIST; 

	/* Display the Open dialog box. */ 
 
	if (!GetOpenFileName(&ofn))
		return;	// canceled
 
	// Refresh the File menu.
	PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(g_qeglobals.d_hwndMain),0), 
			ID_FILE_EXIT);

	/* Open the file. */ 
	if (!QE_LoadProject(ofn.lpstrFile))
		Error ("Couldn't load project file");
}


extern void AddSlash(CString& strPath);
void SaveAsDialog (bool bRegion)
{ 
	strcpy (szDirName, ValueForKey (g_qeglobals.d_project_entity, "basepath") );
  CString strPath = szDirName;
  AddSlash(strPath);
  strPath += "maps";

	/* Place the terminating null character in the szFile. */ 
 
	szFile[0] = '\0'; 
 
	/* Set the members of the OPENFILENAME structure. */ 
 
	ofn.lStructSize = sizeof(OPENFILENAME); 
	ofn.hwndOwner = g_qeglobals.d_hwndCamera;
	ofn.lpstrFilter = szFilter; 
	ofn.nFilterIndex = 1; 
	ofn.lpstrFile = szFile; 
	ofn.nMaxFile = sizeof(szFile); 
	ofn.lpstrFileTitle = szFileTitle; 
	ofn.nMaxFileTitle = sizeof(szFileTitle); 
	ofn.lpstrInitialDir = strPath; 
	ofn.Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | 
		OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT; 

	/* Display the Open dialog box. */ 
 
	if (!GetSaveFileName(&ofn))
		return;	// canceled

  if (bRegion)
	  DefaultExtension (ofn.lpstrFile, ".reg");
  else
	  DefaultExtension (ofn.lpstrFile, ".map");

  if (!bRegion)
  {
	  strcpy (currentmap, ofn.lpstrFile);
    AddNewItem(g_qeglobals.d_lpMruMenu, ofn.lpstrFile);
	  PlaceMenuMRUItem(g_qeglobals.d_lpMruMenu,GetSubMenu(GetMenu(g_qeglobals.d_hwndMain),0),	ID_FILE_EXIT);
  }
	Map_SaveFile (ofn.lpstrFile, bRegion);	// ignore region
}

/*
=======================================================

Menu modifications

=======================================================
*/

/*
==================
FillBSPMenu

==================
*/
char	*bsp_commands[256];

void FillBSPMenu (void)
{
	HMENU	hmenu;
	epair_t	*ep;
	int		i;
	static int count;

	hmenu = GetSubMenu (GetMenu(g_qeglobals.d_hwndMain), MENU_BSP);

	for (i=0 ; i<count ; i++)
		DeleteMenu (hmenu, CMD_BSPCOMMAND+i, MF_BYCOMMAND);
	count = 0;

	if ( g_qeglobals.bBSPFrontendPlugin )
	{
		CString str = g_BSPFrontendTable.m_pfnGetBSPMenu();
		char cTemp[1024];
		strcpy(cTemp, str);
		char* token = strtok(cTemp, ",;");
		if (token && *token == ' ')
		{
			while (*token == ' ')
				token++;
		}
		i = 0;
		// first token is menu name
		ModifyMenu( GetMenu(g_qeglobals.d_hwndMain), MENU_BSP, MF_BYPOSITION, MENU_BSP, (LPCTSTR)token );
		// redraw the menu bar
		DrawMenuBar( g_qeglobals.d_hwndMain );
		token = strtok(NULL, ",;");
		while (token != NULL)
		{
			g_BSPFrontendCommands.Add(token);
			AppendMenu (hmenu, MF_ENABLED|MF_STRING, CMD_BSPCOMMAND+i, (LPCTSTR)token );
			token = strtok(NULL, ",;");
			i++;
		}
		count = i;
	}
	else
	{
		i = 0;
		for (ep = g_qeglobals.d_project_entity->epairs ; ep ; ep=ep->next)
		{
			if (ep->key[0] == 'b' && ep->key[1] == 's' && ep->key[2] == 'p')
			{
				bsp_commands[i] = ep->key;
				AppendMenu (hmenu, MF_ENABLED|MF_STRING,
					CMD_BSPCOMMAND+i, (LPCTSTR)ep->key);
				i++;
			}
		}
		count = i;
	}
}

//==============================================
void AddSlash(CString& strPath)
{
  if (strPath.GetLength() > 0)
  {
    if (strPath.GetAt(strPath.GetLength()-1) != '\\')
      strPath += '\\';
  }
}


bool ExtractPath_and_Filename(const char* pPath, CString& strPath, CString& strFilename)
{
  CString strPathName = pPath;
  int nSlash = strPathName.ReverseFind('\\');
  if (nSlash >= 0)
  {
    strPath = strPathName.Left(nSlash+1);
    strFilename = strPathName.Right(strPathName.GetLength() - nSlash - 1);
  }
  else strFilename = pPath;
  return true;
}


/*
===============
CheckBspProcess

See if the BSP is done yet
===============
*/
extern void FindReplace(CString& strContents, const char* pTag, const char* pValue);
extern CTime g_tBegin;

void CheckBspProcess (void)
{
	char	outputpath[1024];
	char	temppath[512];
	DWORD	exitcode;
	char	*out;
	BOOL	ret;

	if (!bsp_process)
		return;

	ret = GetExitCodeProcess (bsp_process, &exitcode);
	if (!ret)
		Error ("GetExitCodeProcess failed");
	if (exitcode == STILL_ACTIVE)
		return;

	bsp_process = 0;

	GetTempPath(512, temppath);
	sprintf (outputpath, "%sjunk.txt", temppath);

	LoadFile (outputpath, (void **)&out);
	Sys_Printf ("%s", out);
	Sys_Printf ("\ncompleted.\n");
	free (out);

	CTime tEnd = CTime::GetCurrentTime();
	CTimeSpan tElapsed = tEnd - g_tBegin;
	CString strElapsed;
	strElapsed.Format("Run time was %i hours, %i minutes and %i seconds", tElapsed.GetHours(), tElapsed.GetMinutes(), tElapsed.GetSeconds());
	Sys_Printf(strElapsed.GetBuffer(0));


	Sys_Beep ();
	Pointfile_Check();
	// run game if no PointFile and pref is set
	//++timo needs to stop after BSP if leaked .. does run through vis and light instead ..
	if (g_PrefsDlg.m_bRunQuake == TRUE  && !g_qeglobals.d_pointfile_display_list)
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
			//++timo modified for Quake3 !!
			strCopyPath += "baseq3\\maps\\";
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

extern int	cambuttonstate;

extern "C" {
void *Com_Allocate( int bytes ) {
	return malloc(bytes);
}

void Com_Dealloc( void *ptr ) {
	free(ptr);
}


void Com_Printf( const char *msg, ... ) {
	va_list argptr;
	char	buf[32768];
	char	*out;

	va_start (argptr,msg);
	vsprintf (buf, msg,argptr);
	va_end (argptr);

	out = TranslateString (buf);

	Sys_Printf(buf);
}

void Com_Error( int level, const char *error, ... ) {
	va_list argptr;
	char	buf[32768];
	char	*out;

	va_start (argptr,error);
	vsprintf (buf, error,argptr);
	va_end (argptr);

	out = TranslateString (buf);

	Sys_Printf(buf);
}

void Com_DPrintf( const char *msg, ... ) {
	return;
}

int FS_Write( const void *buffer, int len, fileHandle_t h ) {
	SafeWrite(reinterpret_cast<FILE*>(h), buffer, len);
	return len;
}

int FS_ReadFile( const char *qpath, void **buffer ) {
	CString strPath = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	AddSlash(strPath);
	strPath += qpath;
	return LoadFile(strPath, buffer);
}

void FS_FreeFile( void *buffer ) {
	Com_Dealloc(buffer);
}

fileHandle_t FS_FOpenFileWrite( const char *filename ) {
	CString strPath = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	AddSlash(strPath);
	strPath += filename;
	// bad casting ptr to int
	return reinterpret_cast<fileHandle_t>(SafeOpenWrite(strPath));
}

void FS_FCloseFile( fileHandle_t f ) {
	fclose(reinterpret_cast<FILE*>(f));
}



}

