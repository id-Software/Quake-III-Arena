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
// ShaderInfo.cpp: implementation of the CShaderInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Radiant.h"
#include "ShaderInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShaderInfo::CShaderInfo()
{
  m_fTransValue = 1.0;
  m_nFlags = 0;
  m_pQTexture = NULL;
}

CShaderInfo::~CShaderInfo()
{

}

void CShaderInfo::Parse(const char *pName)
{

}

void CShaderInfo::setName(char *pName)
{
  //--char path[1024];
  //--strcpy(path, pName);
  //--DefaultExtension(path, ".tga");
  m_strName = pName;
  m_strName.MakeLower();
  if (m_strName.Find("textures") == 0)
  {
    CString s = m_strName.Right(m_strName.GetLength() - strlen("textures") - 1);
    m_strName = s;
  }
}
