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
// ShaderInfo.h: interface for the CShaderInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHADERINFO_H__93B64600_A208_11D2_803D_0020AFEB881A__INCLUDED_)
#define AFX_SHADERINFO_H__93B64600_A208_11D2_803D_0020AFEB881A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CShaderInfo  
{
public:
	CString m_strName;
	CString m_strShaderName;
	CString m_strTextureName;
	CStringList m_strEditorParams;
	CStringList m_lstSurfaceParams;
  float m_fTransValue;
  int m_nFlags;
  qtexture_t *m_pQTexture;
	
  
  void Parse(const char *pName);
	CShaderInfo();
	virtual ~CShaderInfo();
  void setName(char *pName);

  

};

#endif // !defined(AFX_SHADERINFO_H__93B64600_A208_11D2_803D_0020AFEB881A__INCLUDED_)
