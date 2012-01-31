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
//-----------------------------------------------------------------------------
//
// $LogFile$
// $Revision: 1.1.1.4 $
// $Author: ttimo $
// $Date: 2000/01/18 00:17:34 $
// $Log: PluginEntities.cpp,v $
// Revision 1.1.1.4  2000/01/18 00:17:34  ttimo
// merging in for RC
//
// Revision 1.3  2000/01/17 23:53:42  TBesset
// ready for merge in sourceforge (RC candidate)
//
// Revision 1.2  2000/01/07 16:40:11  TBesset
// merged from BSP frontend
//
// Revision 1.1.1.3  1999/12/29 18:31:34  TBesset
// Q3Radiant public version
//
// Revision 1.1.1.1.2.1  1999/12/29 21:39:38  TBesset
// updated to update3 from Robert
//
// Revision 1.1.1.3  1999/12/29 18:31:34  TBesset
// Q3Radiant public version
// Revision 1.1.1.3  1999/12/29 18:31:34  TBesset
// Q3Radiant public version
//
// Revision 1.2  1999/11/22 17:46:46  Timo & Christine
// merged EARadiant into the main tree
// bug fixes for Q3Plugin / EAPlugin
// export for Robert
//
// Revision 1.1.2.1  1999/11/03 20:38:01  Timo & Christine
// MEAN plugin for Q3Radiant, alpha version
//
//
// DESCRIPTION:
// implementation of IPluginEntities specific interface
// 

#include "stdafx.h"
#include "io.h"

// is Eclass_InitForModelDirectory
int WINAPI QERApp_EClassScanDir( char *path, HMODULE hPlug )
{
	struct _finddata_t	fileinfo;
	int					handle;
	char				temp[ _MAX_PATH ];
	char				filebase[ _MAX_PATH ];
	char				filename[ _MAX_PATH ];
	char				*s;
	eclass_t			*e;
	QE_ConvertDOSToUnixName( temp, path );
	strcpy (filebase, path);
	s = filebase + strlen(filebase)-1;
	while (*s != '\\' && *s != '/' && s!=filebase)
		s--;
	*s = 0;
	handle = _findfirst (path, &fileinfo);
	if (handle != -1)
	   {
		do
		{			
			sprintf (filename, "%s\\%s", filebase, fileinfo.name);
			Eclass_ScanFile (filename);
			
			if (eclass_found)
            {
				e = eclass_e;
				e->modelpath = strdup( fileinfo.name );
				e->nShowFlags |= ECLASS_PLUGINENTITY;
				e->hPlug = hPlug;
            }
		} while (_findnext( handle, &fileinfo ) != -1);
		
		_findclose (handle);
	   }
	return 0;
}
