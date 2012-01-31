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
// #includes follow...
//-----------------------------------------------------------------------------
//
#include "StdAfx.h"                       // For: Precompiled Headers
//
//-----------------------------------------------------------------------------
// #defines, consts, etc follow...
//-----------------------------------------------------------------------------
//
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//
//-----------------------------------------------------------------------------
// Implementation follows...
//-----------------------------------------------------------------------------
//

// msvc 5.xx does not do well optimizing this code
#pragma optimize("",off)

HINSTANCE g_hQBSPDLL = NULL;

typedef void (WINAPI* PFN_QBSP)(char*, HWND, const char*);

void RunTools(char* pCommandLine, HWND hwnd, const char* pPAKFile)
{
  static PFN_QBSP lpfnQBSP = NULL;
  if (g_hQBSPDLL == NULL)
    g_hQBSPDLL = ::LoadLibrary("qbspdll.dll");
  else
  {
    ::FreeLibrary(g_hQBSPDLL);
    g_hQBSPDLL = ::LoadLibrary("qbspdll.dll");
    lpfnQBSP = NULL;
  }
  if (g_hQBSPDLL != NULL)
  {
    if (lpfnQBSP == NULL && g_hQBSPDLL != NULL)
    {
      FARPROC pFunction = ::GetProcAddress(g_hQBSPDLL, "FullProcess");
      lpfnQBSP = reinterpret_cast<PFN_QBSP>(pFunction);
    }
    if (lpfnQBSP != NULL)
    {
      (*lpfnQBSP)(pCommandLine, hwnd, pPAKFile);
    }
  }
}


#pragma optimize("",on)

