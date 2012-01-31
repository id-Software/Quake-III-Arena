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
#include <direct.h>  
#include <sys\stat.h> 

QEGlobals_t  g_qeglobals;

void WINAPI QE_CheckOpenGLForErrors(void)
{
  CString strMsg;
  int i = qglGetError();
  if (i != GL_NO_ERROR)
  {
    if (i == GL_OUT_OF_MEMORY)
    {
      //strMsg.Format("OpenGL out of memory error %s\nDo you wish to save before exiting?", qgluErrorString((GLenum)i));
      if (MessageBox(g_qeglobals.d_hwndMain, strMsg, "Q3Radiant Error", MB_YESNO) == IDYES)
      {
        Map_SaveFile(NULL, false);
      }
		  exit(1);
    }
    else
    {
      //strMsg.Format("Warning: OpenGL Error %s\n ", qgluErrorString((GLenum)i));
		  Sys_Printf (strMsg.GetBuffer(0));
    }
  }
}


char *ExpandReletivePath (char *p)
{
	static char	temp[1024];
	char	*base;

	if (!p || !p[0])
		return NULL;
	if (p[0] == '/' || p[0] == '\\')
		return p;

	base = ValueForKey(g_qeglobals.d_project_entity, "basepath");
	sprintf (temp, "%s/%s", base, p);
	return temp;
}

char *copystring (char *s)
{
	char	*b;
	b = (char*)malloc(strlen(s)+1);
	strcpy (b,s);
	return b;
}


bool DoesFileExist(const char* pBuff, long& lSize)
{
  CFile file;
  if (file.Open(pBuff, CFile::modeRead | CFile::shareDenyNone))
  {
    lSize += file.GetLength();
    file.Close();
    return true;
  }
  return false;
}


void Map_Snapshot()
{
  CString strMsg;
  // we need to do the following
  // 1. make sure the snapshot directory exists (create it if it doesn't)
  // 2. find out what the lastest save is based on number
  // 3. inc that and save the map
  CString strOrgPath, strOrgFile;
  ExtractPath_and_Filename(currentmap, strOrgPath, strOrgFile);
  AddSlash(strOrgPath);
  strOrgPath += "snapshots";
  bool bGo = true;
  struct _stat Stat;
  if (_stat(strOrgPath, &Stat) == -1)
  {
    bGo = (_mkdir(strOrgPath) != -1);
  }
  AddSlash(strOrgPath);
  if (bGo)
  {
    int nCount = 0;
    long lSize = 0;
    CString strNewPath = strOrgPath;
    strNewPath += strOrgFile;
    CString strFile;
    while (bGo)
    {
      strFile.Format("%s.%i", strNewPath, nCount);
      bGo = DoesFileExist(strFile, lSize);
      nCount++;
    }
    // strFile has the next available slot
    Map_SaveFile(strFile.GetBuffer(0), false);
		Sys_SetTitle (currentmap);
    if (lSize > 12 * 1024 * 1024) // total size of saves > 4 mb
    {
      Sys_Printf("The snapshot files in the [%s] directory total more than 4 megabytes. You might consider cleaning the directory up.", strOrgPath);
    }
  }
  else
  {
    strMsg.Format("Snapshot save failed.. unabled to create directory\n%s", strOrgPath);
    g_pParentWnd->MessageBox(strMsg);
  }
}
/*
===============
QE_CheckAutoSave

If five minutes have passed since making a change
and the map hasn't been saved, save it out.
===============
*/


void QE_CheckAutoSave( void )
{
	static clock_t s_start;
	clock_t        now;

	now = clock();

	if ( modified != 1 || !s_start)
	{
		s_start = now;
		return;
	}

	if ( now - s_start > ( CLOCKS_PER_SEC * 60 * g_PrefsDlg.m_nAutoSave))
	{

    if (g_PrefsDlg.m_bAutoSave)
    {
      CString strMsg = g_PrefsDlg.m_bSnapShots ? "Autosaving snapshot..." : "Autosaving...";
		  Sys_Printf(strMsg.GetBuffer(0));
      Sys_Printf("\n");
		  Sys_Status (strMsg.GetBuffer(0),0);

      // only snapshot if not working on a default map
      if (g_PrefsDlg.m_bSnapShots && stricmp(currentmap, "unnamed.map") != 0)
      {
        Map_Snapshot();
      }
      else
      {
		    Map_SaveFile (ValueForKey(g_qeglobals.d_project_entity, "autosave"), false);
      }

		  Sys_Status ("Autosaving...Saved.", 0 );
		  modified = 2;
    }
    else
    {
		  Sys_Printf ("Autosave skipped...\n");
		  Sys_Status ("Autosave skipped...", 0 );
    }
		s_start = now;
	}
}


int BuildShortPathName(const char* pPath, char* pBuffer, int nBufferLen)
{
  char *pFile = NULL;
  int nResult = GetFullPathName(pPath, nBufferLen, pBuffer, &pFile); 
  nResult = GetShortPathName(pPath, pBuffer, nBufferLen);
  if (nResult == 0)
    strcpy(pBuffer, pPath);                     // Use long filename
  return nResult;
}



const char *g_pPathFixups[]=
{
  "basepath",
  "remotebasepath",
  "entitypath",
  "texturepath",
  "autosave",
  "mapspath"
};

const int g_nPathFixupCount = sizeof(g_pPathFixups) / sizeof(const char*);

/*
===========
QE_LoadProject
===========
*/
qboolean QE_LoadProject (char *projectfile)
{
	char	*data;

	Sys_Printf ("QE_LoadProject (%s)\n", projectfile);
	
	if ( LoadFileNoCrash (projectfile, (void **)&data) == -1)
		return false;
	
	g_strProject = projectfile;
	
	CString strData = data;
	free(data);
	
	CString strQ2Path = g_PrefsDlg.m_strQuake2;
	CString strQ2File;
	ExtractPath_and_Filename(g_PrefsDlg.m_strQuake2, strQ2Path, strQ2File);
	AddSlash(strQ2Path);
	
	
	char* pBuff = new char[1024];
	
	BuildShortPathName(strQ2Path, pBuff, 1024);
	FindReplace(strData, "__Q2PATH", pBuff);
	BuildShortPathName(g_strAppPath, pBuff, 1024);
	FindReplace(strData, "__QERPATH", pBuff);
	
	char* pFile;
	if (GetFullPathName(projectfile, 1024, pBuff, &pFile))
	{
		g_PrefsDlg.m_strLastProject = pBuff;
		BuildShortPathName(g_PrefsDlg.m_strLastProject, pBuff, 1024);
		g_PrefsDlg.m_strLastProject = pBuff;
		g_PrefsDlg.SavePrefs();
		
		ExtractPath_and_Filename(pBuff, strQ2Path, strQ2File);
		int nLen = strQ2Path.GetLength();
		if (nLen > 0)
		{
			if (strQ2Path[nLen - 1] == '\\')
				strQ2Path.SetAt(nLen-1,'\0');
			char* pBuffer = strQ2Path.GetBufferSetLength(_MAX_PATH + 1);
			int n = strQ2Path.ReverseFind('\\');
			if (n >=0 )
				pBuffer[n + 1] = '\0';
			strQ2Path.ReleaseBuffer();
			FindReplace(strData, "__QEPROJPATH", strQ2Path);
		}
	}
	
	
	StartTokenParsing (strData.GetBuffer(0));
	g_qeglobals.d_project_entity = Entity_Parse (true);
	if (!g_qeglobals.d_project_entity)
		Error ("Couldn't parse %s", projectfile);

  for (int i = 0; i < g_nPathFixupCount; i++)
  {
    char *pPath = ValueForKey (g_qeglobals.d_project_entity, g_pPathFixups[i]);
    if (pPath[0] != '\\' && pPath[0] != '/')
    {
	    if (GetFullPathName(pPath, 1024, pBuff, &pFile))
      {
        SetKeyValue(g_qeglobals.d_project_entity, g_pPathFixups[i], pBuff);
      }
    }
  }

	delete []pBuff;

	// set here some default project settings you need
	if ( strlen( ValueForKey( g_qeglobals.d_project_entity, "brush_primit" ) ) == 0 )
	{
		SetKeyValue( g_qeglobals.d_project_entity, "brush_primit", "0" );
	}

	g_qeglobals.m_bBrushPrimitMode = IntForKey( g_qeglobals.d_project_entity, "brush_primit" );


	Eclass_InitForSourceDirectory (ValueForKey (g_qeglobals.d_project_entity, "entitypath"));
	FillClassList();		// list in entity window
	
	Map_New();
	
	
	FillTextureMenu();
	FillBSPMenu();
	
	return true;
}

/*
===========
QE_SaveProject
===========
*/
//extern char	*bsp_commands[256];

qboolean QE_SaveProject (const char* pProjectFile)
{
	//char	filename[1024];
	FILE	*fp;
	epair_t	*ep;

	//sprintf (filename, "%s\\%s.prj", g_projectdir, g_username);

	if (!(fp = fopen (pProjectFile, "w+")))
		Error ("Could not open project file!");
	
	fprintf (fp, "{\n");
	for (ep = g_qeglobals.d_project_entity->epairs; ep; ep=ep->next)
		fprintf (fp, "\"%s\" \"%s\"\n", ep->key, ep->value);
	fprintf (fp, "}\n");

	fclose (fp);
	
	return TRUE;
}



/*
===========
QE_KeyDown
===========
*/
#define	SPEED_MOVE	32
#define	SPEED_TURN	22.5


/*
===============
ConnectEntities

Sets target / targetname on the two entities selected
from the first selected to the secon
===============
*/
void ConnectEntities (void)
{
	entity_t	*e1, *e2, *e;
	char		*target, *tn;
	int			maxtarg, targetnum;
	char		newtarg[32];

	if (g_qeglobals.d_select_count != 2)
	{
		Sys_Status ("Must have two brushes selected.", 0);
		Sys_Beep ();
		return;
	}

	e1 = g_qeglobals.d_select_order[0]->owner;
	e2 = g_qeglobals.d_select_order[1]->owner;

	if (e1 == world_entity || e2 == world_entity)
	{
		Sys_Status ("Can't connect to the world.", 0);
		Sys_Beep ();
		return;
	}

	if (e1 == e2)
	{
		Sys_Status ("Brushes are from same entity.", 0);
		Sys_Beep ();
		return;
	}

	target = ValueForKey (e1, "target");
	if (target && target[0])
		strcpy (newtarg, target);
	else
	{
		target = ValueForKey (e2, "targetname");
		if (target && target[0])
			strcpy (newtarg, target);
		else
		{
			// make a unique target value
			maxtarg = 0;
			for (e=entities.next ; e != &entities ; e=e->next)
			{
				tn = ValueForKey (e, "targetname");
				if (tn && tn[0])
				{
					targetnum = atoi(tn+1);
					if (targetnum > maxtarg)
						maxtarg = targetnum;
				}
			}
			sprintf (newtarg, "t%i", maxtarg+1);
		}
	}

	SetKeyValue (e1, "target", newtarg);
	SetKeyValue (e2, "targetname", newtarg);
	Sys_UpdateWindows (W_XY | W_CAMERA);

	Select_Deselect();
	Select_Brush (g_qeglobals.d_select_order[1]);
}

qboolean QE_SingleBrush (bool bQuiet)
{
	if ( (selected_brushes.next == &selected_brushes)
		|| (selected_brushes.next->next != &selected_brushes) )
	{
	  if (!bQuiet)
    {
	  	Sys_Printf ("Error: you must have a single brush selected\n");
	  }
		return false;
	}
	if (selected_brushes.next->owner->eclass->fixedsize)
	{
	  if (!bQuiet)
	  {
		  Sys_Printf ("Error: you cannot manipulate fixed size entities\n");
	  }
		return false;
	}

	return true;
}

void QE_Init (void)
{
	/*
	** initialize variables
	*/
	g_qeglobals.d_gridsize = 8;
	g_qeglobals.d_showgrid = true;

	/*
	** other stuff
	*/
	Texture_Init (true);
	//Cam_Init ();
	//XY_Init ();
	Z_Init ();
	Terrain_Init();
}

void WINAPI QE_ConvertDOSToUnixName( char *dst, const char *src )
{
	while ( *src )
	{
		if ( *src == '\\' )
			*dst = '/';
		else
			*dst = *src;
		dst++; src++;
	}
	*dst = 0;
}

int g_numbrushes, g_numentities;

void QE_CountBrushesAndUpdateStatusBar( void )
{
	static int      s_lastbrushcount, s_lastentitycount;
	static qboolean s_didonce;
	
	//entity_t   *e;
	brush_t	   *b, *next;

	g_numbrushes = 0;
	g_numentities = 0;
	
	if ( active_brushes.next != NULL )
	{
		for ( b = active_brushes.next ; b != NULL && b != &active_brushes ; b=next)
		{
			next = b->next;
			if (b->brush_faces )
			{
				if ( !b->owner->eclass->fixedsize)
					g_numbrushes++;
				else
					g_numentities++;
			}
		}
	}
/*
	if ( entities.next != NULL )
	{
		for ( e = entities.next ; e != &entities && g_numentities != MAX_MAP_ENTITIES ; e = e->next)
		{
			g_numentities++;
		}
	}
*/
	if ( ( ( g_numbrushes != s_lastbrushcount ) || ( g_numentities != s_lastentitycount ) ) || ( !s_didonce ) )
	{
		Sys_UpdateStatusBar();

		s_lastbrushcount = g_numbrushes;
		s_lastentitycount = g_numentities;
		s_didonce = true;
	}
}

char		com_token[1024];
qboolean	com_eof;

/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	time_t	t;
	
	time (&t);
	
	return t;
#if 0
// more precise, less portable
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}
	
	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
#endif
}


/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	int		c;
	int		len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			com_eof = true;
			return NULL;			// end of file;
		}
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		do
		{
			c = *data++;
			if (c=='\"')
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		} while (1);
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':')
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}



/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


int		argc;
char	*argv[MAX_NUM_ARGVS];

/*
============
ParseCommandLine
============
*/
void ParseCommandLine (char *lpCmdLine)
{
	argc = 1;
	argv[0] = "programname";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}
}



/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (char *check)
{
	int             i;

	for (i = 1;i<argc;i++)
	{
		if ( stricmp(check, argv[i]) )
			return i;
	}

	return 0;
}




/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex (char *hex)
{
	char    *str;
	int    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			Error ("Bad hex number: %s",hex);
		str++;
	}

	return num;
}


int ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}


