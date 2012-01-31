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
// TexWnd.cpp : implementation file
//

#include "stdafx.h"
#include <assert.h>
#include "Radiant.h"
#include "TexWnd.h"
#include "qe3.h"
#include "io.h"
#include "PrefsDlg.h"
#include "shaderinfo.h"
#include "pakstuff.h"
#include "str.h"
#include "PrefsDlg.h"

Str m_gStr;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

qtexture_t *Texture_ForNamePath(char* name, char* pFullPath);
#define	TYP_MIPTEX	68
static unsigned	tex_palette[256];

qtexture_t	*notexture = NULL;
qtexture_t	*g_pluginTexture = NULL;

static qboolean	nomips = false;

#define	FONT_HEIGHT	10

HGLRC s_hglrcTexture = NULL;
HDC	 s_hdcTexture = NULL;

//int		texture_mode = GL_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_NEAREST;
//int		texture_mode = GL_NEAREST_MIPMAP_LINEAR;
//int		texture_mode = GL_LINEAR;
//int		texture_mode = GL_LINEAR_MIPMAP_NEAREST;
int		texture_mode = GL_LINEAR_MIPMAP_LINEAR;

// this is the global counter for GL bind numbers
int		texture_extension_number = 1;
int g_nCurrentTextureMenuName;

int g_nTextureOffset = 0;

// current active texture directory.  if empty, show textures in use
char		texture_directory[128];	// use if texture_showinuse is false
qboolean	texture_showinuse;

bool g_bFilterEnabled = false;
CString g_strFilter;

// texture layout functions
qtexture_t	*current_texture = NULL;
int			current_x, current_y, current_row;

int			texture_nummenus;
#define		MAX_TEXTUREDIRS	128
char		texture_menunames[MAX_TEXTUREDIRS][128];

qboolean	g_dontuse = true;		// set to true to load the texture but not flag as used

// void SelectTexture (int mx, int my, bool bShift = false);
void SelectTexture (int mx, int my, bool bShift, bool bFitScale=false);

void	Texture_MouseDown (int x, int y, int buttons);
void	Texture_MouseUp (int x, int y, int buttons);
void	Texture_MouseMoved (int x, int y, int buttons);

CPtrArray g_lstShaders;
CPtrArray g_lstSkinCache;

struct SkinInfo
{
  CString m_strName;
  int m_nTextureBind;
  SkinInfo(const char *pName, int n)
  {
    m_strName = pName;
    m_nTextureBind = n;
  };
  SkinInfo(){};
};

// checks wether a qtexture_t exists for a given name
//++timo FIXME: is this really any use? redundant.
bool ShaderQTextureExists(const char *pName)
{
  for (qtexture_t *q=g_qeglobals.d_qtextures ; q ; q=q->next)
  {
    if (!strcmp(q->name,  pName))
    {
      return true;
    }
  }
  return false;

}

CShaderInfo* hasShader(const char *pName)
{
  int nSize = g_lstShaders.GetSize();
  for (int i = 0; i < nSize; i++)
  {
    CShaderInfo *pInfo = reinterpret_cast<CShaderInfo*>(g_lstShaders.ElementAt(i));
    if (pInfo != NULL)
    {
      if (pInfo->m_strName.CompareNoCase(pName) == 0)
      {
        return pInfo;
      }
    }
  }
  return NULL;
}

// gets active texture extension
// 
// FIXME: fix this to be generic from project file
//
int GetTextureExtensionCount()
{
  return 2;
}

const char* GetTextureExtension(int nIndex)
{
  if ( nIndex == 0)
  {
    _QERTextureInfo *pInfo = g_pParentWnd->GetPlugInMgr().GetTextureInfo();
    const char *pTex = (pInfo != NULL) ? pInfo->m_TextureExtension : NULL;
    return (pTex == NULL) ? (g_PrefsDlg.m_bHiColorTextures == FALSE) ? "wal" : "tga" : pTex;
  }
  // return jpg for 2nd extension
  return "jpg";
}

void SortTextures(void)
{	
	qtexture_t	*q, *qtemp, *qhead, *qcur, *qprev;

	// standard insertion sort
	// Take the first texture from the list and
	// add it to our new list
	if ( g_qeglobals.d_qtextures == NULL)
		return;	

	qhead = g_qeglobals.d_qtextures;
	q = g_qeglobals.d_qtextures->next;
	qhead->next = NULL;
	
	// while there are still things on the old
	// list, keep adding them to the new list
	while (q)
	{
		qtemp = q;
		q = q->next;
		
		qprev = NULL;
		qcur = qhead;

		while (qcur)
		{
			// Insert it here?
			if (strcmp(qtemp->name, qcur->name) < 0)
			{
				qtemp->next = qcur;
				if (qprev)
					qprev->next = qtemp;
				else
					qhead = qtemp;
				break;
			}
			
			// Move on

			qprev = qcur;
			qcur = qcur->next;


			// is this one at the end?

			if (qcur == NULL)
			{
				qprev->next = qtemp;
				qtemp->next = NULL;
			}
		}


	}

	g_qeglobals.d_qtextures = qhead;
}

/*
==============
Texture_InitPalette
==============
*/
void Texture_InitPalette (byte *pal)
{
    int		r,g,b,v;
    int		i;
	int		inf;
	byte	gammatable[256];
	float	gamma;

	gamma = g_qeglobals.d_savedinfo.fGamma;

	if (gamma == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
	}
	else
	{
		for (i=0 ; i<256 ; i++)
		{
			inf = 255 * pow ( (float)( (i+0.5)/255.5 ), gamma ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

    for (i=0 ; i<256 ; i++)
    {
		  r = gammatable[pal[0]];
		  g = gammatable[pal[1]];
		  b = gammatable[pal[2]];
		  pal += 3;
		
		  v = (r<<24) + (g<<16) + (b<<8) + 255;
		  v = BigLong (v);
		
		  tex_palette[i] = v;
    }
}

void SetTexParameters (void)
{
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_mode );
	
	switch ( texture_mode )
	{
	case GL_NEAREST:
	case GL_NEAREST_MIPMAP_NEAREST:
	case GL_NEAREST_MIPMAP_LINEAR:
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		break;
	case GL_LINEAR:
	case GL_LINEAR_MIPMAP_NEAREST:
	case GL_LINEAR_MIPMAP_LINEAR:
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		break;
	}
}

/*
============
Texture_SetMode
============
*/
void Texture_SetMode(int iMenu)
{
	int	i, iMode;
	HMENU hMenu;
	qboolean texturing = true;

	hMenu = GetMenu(g_qeglobals.d_hwndMain);

	switch(iMenu) {
	case ID_VIEW_NEAREST:					
		iMode = GL_NEAREST;
		break;
	case ID_VIEW_NEARESTMIPMAP:
		iMode = GL_NEAREST_MIPMAP_NEAREST;
		break;
	case ID_VIEW_LINEAR:
		iMode = GL_NEAREST_MIPMAP_LINEAR;
		break;
	case ID_VIEW_BILINEAR:
		iMode = GL_LINEAR;
		break;
	case ID_VIEW_BILINEARMIPMAP:
		iMode = GL_LINEAR_MIPMAP_NEAREST;
		break;
	case ID_VIEW_TRILINEAR:
		iMode = GL_LINEAR_MIPMAP_LINEAR;
		break;

	case ID_TEXTURES_WIREFRAME:
		iMode = 0;
		texturing = false;
		break;

	case ID_TEXTURES_FLATSHADE:
		iMode = 0;
		texturing = false;
		break;

	}

	CheckMenuItem(hMenu, ID_VIEW_NEAREST, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_VIEW_NEARESTMIPMAP, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_VIEW_LINEAR, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_VIEW_BILINEARMIPMAP, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_VIEW_BILINEAR, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_VIEW_TRILINEAR, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_WIREFRAME, MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, ID_TEXTURES_FLATSHADE, MF_BYCOMMAND | MF_UNCHECKED);

	CheckMenuItem(hMenu, iMenu, MF_BYCOMMAND | MF_CHECKED);

	g_qeglobals.d_savedinfo.iTexMenu = iMenu;
	texture_mode = iMode;

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
    if (s_hdcTexture && s_hglrcTexture)
    {
      //if (!qwglMakeCurrent(g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase))
      if (!qwglMakeCurrent(s_hdcTexture, s_hglrcTexture))
		    Error ("wglMakeCurrent in LoadTexture failed");
    }
    else
      return;
  }

	if ( texturing )
		SetTexParameters ();

	if ( !texturing && iMenu == ID_TEXTURES_WIREFRAME)
	{
		g_pParentWnd->GetCamera()->Camera().draw_mode = cd_wire;
		Map_BuildBrushData();
		Sys_UpdateWindows (W_ALL);
		return;

	} else if ( !texturing && iMenu == ID_TEXTURES_FLATSHADE) {

		g_pParentWnd->GetCamera()->Camera().draw_mode = cd_solid;
		Map_BuildBrushData();
		Sys_UpdateWindows (W_ALL);
		return;
	}

	for (i=1 ; i<texture_extension_number ; i++)
	{
		qglBindTexture( GL_TEXTURE_2D, i );
		SetTexParameters ();
	}

	// select the default texture
	qglBindTexture( GL_TEXTURE_2D, 0 );

	qglFinish();

	if (g_pParentWnd->GetCamera()->Camera().draw_mode != cd_texture)
	{
		g_pParentWnd->GetCamera()->Camera().draw_mode = cd_texture;
		Map_BuildBrushData();
	}

	Sys_UpdateWindows (W_ALL);
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
void R_MipMap (byte *in, int &width, int &height)
{
	int		i, j;
	byte	*out;
	int		row;
	
	row = width * 4;
	width >>= 1;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=row)
	{
		for (j=0 ; j<width ; j++, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
		}
	}
}

/*
=================
Texture_LoadTexture
=================
*/
//++timo NOTE: miptex_t is used only for .WAL format .. a bit outdated
qtexture_t *Texture_LoadTexture (miptex_t *qtex)
{
  byte		*source;
  unsigned	char *dest;
  int			width, height, i, count;
	int			total[3];
  qtexture_t	*q;
      
  width = LittleLong(qtex->width);
  height = LittleLong(qtex->height);

  q = (qtexture_t*)qmalloc(sizeof(*q));

  q->width = width;
  q->height = height;

	q->flags = qtex->flags;
	q->value = qtex->value;
	q->contents = qtex->contents;

	dest = (unsigned char*)qmalloc (width*height*4);

  count = width*height;
  source = (byte *)qtex + LittleLong(qtex->offsets[0]);

	// The dib is upside down so we want to copy it into 
	// the buffer bottom up.

	total[0] = total[1] = total[2] = 0;
  for (i=0 ; i<count ; i++)
	{
		dest[i] = tex_palette[source[i]];

		total[0] += ((byte *)(dest+i))[0];
		total[1] += ((byte *)(dest+i))[1];
		total[2] += ((byte *)(dest+i))[2];
	}

	q->color[0] = (float)total[0]/(count*255);
	q->color[1] = (float)total[1]/(count*255);
	q->color[2] = (float)total[2]/(count*255);

  q->texture_number = texture_extension_number++;

  if (g_qeglobals.bSurfacePropertiesPlugin)
  {
	  // Timo
	  // Surface properties plugins can store their own data in an IPluginQTexture
	  q->pData = g_SurfaceTable.m_pfnQTextureAlloc( q );
	  GETPLUGINTEXDEF(q)->InitForMiptex( qtex );
  }

  //++timo is the m_bSGIOpenGL parameter still taken into account?
  if (g_PrefsDlg.m_bSGIOpenGL)
  {
    //if (!qwglMakeCurrent(g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase))
    if (!qwglMakeCurrent(s_hdcTexture, s_hglrcTexture))
		  Error ("wglMakeCurrent in LoadTexture failed");
  }

  qglBindTexture( GL_TEXTURE_2D, q->texture_number );

  //Handle3DfxTexturing(q, width, height, dest);

  SetTexParameters ();

  int nCount = MAX_TEXTURE_QUALITY - g_PrefsDlg.m_nTextureQuality;
  while (nCount-- > 0)
  {
    if (width > 16 && height > 16)
    {
      R_MipMap(dest, width, height);
    }
    else
    {
      break;
    }
  }

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
	  if (nomips)
    {
		  qglTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest);
    }
	  else
		  qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height,GL_RGBA, GL_UNSIGNED_BYTE, dest);
  }
  else
  {
	  if (nomips)
		  qglTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dest);
	  else
		  qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, width, height,GL_RGBA, GL_UNSIGNED_BYTE, dest);
  }

	free (dest);

	qglBindTexture( GL_TEXTURE_2D, 0 );

  return q;
}




/*
=================
Texture_LoadTexture
=================
*/
qtexture_t *Texture_LoadTGATexture (unsigned char* pPixels, int nWidth, int nHeight, char* pPath, int nFlags, int nContents, int nValue )
{
  int i, j, inf;
	byte	gammatable[256];
	float fGamma = g_qeglobals.d_savedinfo.fGamma;


  qtexture_t* q = (qtexture_t*)qmalloc(sizeof(*q));
  q->width = nWidth;
  q->height = nHeight;
	q->flags = nFlags;
	q->value = nValue;
	q->contents = nContents;

  int nCount = nWidth * nHeight;
  float total[3];
  total[0] = total[1] = total[2] = 0.0f;

  //++timo FIXME: move gamma table initialization somewhere else!
	if (fGamma == 1.0)
	{
		for (i=0 ; i<256 ; i++)
			gammatable[i] = i;
	}
	else
	{
		for (i=0 ; i<256 ; i++)
		{
			inf = 255 * pow ( (float)( (i+0.5)/255.5 ), fGamma ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}


  // all targas are stored internally as 32bit so rgba = 4 bytes
  for (i = 0 ; i < (nCount * 4) ; i += 4)
	{
    for (j = 0; j < 3; j++)
    {
	    total[j] += (pPixels+i)[j];
      byte b = (pPixels+i)[j];
      (pPixels+i)[j] = gammatable[b];
              
    }
	}

	q->color[0] = total[0] / (nCount * 255);
	q->color[1] = total[1] / (nCount * 255);
	q->color[2] = total[2] / (nCount * 255);


  q->texture_number = texture_extension_number++;

  if (g_qeglobals.bSurfacePropertiesPlugin)
  {
	  // Timo
	  // Surface properties plugins can store their own data in an IPluginQTexture
	  q->pData = g_SurfaceTable.m_pfnQTextureAlloc( q );
	  GETPLUGINTEXDEF(q)->SetDefaultTexdef();
  }

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
    //if (!qwglMakeCurrent(g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase))
    if (!qwglMakeCurrent(s_hdcTexture, s_hglrcTexture))
		  Error ("wglMakeCurrent in LoadTexture failed");
  }

  qglBindTexture( GL_TEXTURE_2D, q->texture_number );

  //Handle3DfxTexturing(q, width, height, dest);

  SetTexParameters();

  nCount = MAX_TEXTURE_QUALITY - g_PrefsDlg.m_nTextureQuality;
  while (nCount-- > 0)
  {
    if (nWidth > 16 && nHeight > 16)
    {
      R_MipMap(pPixels, nWidth, nHeight);
    }
    else
    {
      break;
    }
  }

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
	  if (nomips)
    {
		  qglTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
    }
	  else
		  qgluBuild2DMipmaps(GL_TEXTURE_2D, 4, nWidth, nHeight,GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
  }
  else
  {
	  if (nomips)
		  qglTexImage2D(GL_TEXTURE_2D, 0, 4, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
	  else
		  qgluBuild2DMipmaps(GL_TEXTURE_2D, 4, nWidth, nHeight,GL_RGBA, GL_UNSIGNED_BYTE, pPixels);
  }

	qglBindTexture( GL_TEXTURE_2D, 0 );

  return q;
}


qtexture_t *Texture_LoadTGATexture (unsigned char* pPixels, int nWidth, int nHeight, char *pPath)
{
  CString strName;
  CString strPath;
  ExtractPath_and_Filename(pPath, strPath, strName);
  AddSlash(strPath);
  strPath += "textureinfo.ini";
  strName.MakeLower();
  StripExtension (strName.GetBuffer(0));
  strName.ReleaseBuffer();
  
  int nFlags = GetPrivateProfileInt(strName, "Flags", 0, strPath);
  int nValue = GetPrivateProfileInt(strName, "Value", 0, strPath);
  int nContents = GetPrivateProfileInt(strName, "Contents", 0, strPath);
  return Texture_LoadTGATexture(pPixels, nWidth, nHeight, pPath, nFlags, nValue, nContents);
}


void Texture_LoadFromPlugIn(LPVOID vp)
{
  g_pluginTexture = notexture;
  _QERTextureLoad *pLoad = reinterpret_cast<_QERTextureLoad*>(vp);
  if (pLoad != NULL)
  {
	  qtexture_t	*q;
    q = Texture_LoadTGATexture(pLoad->m_pRGBA, pLoad->m_nWidth, pLoad->m_nHeight, NULL, pLoad->m_nFlags, pLoad->m_nContents, pLoad->m_nValue);
    if (q != NULL)
    {
		// to save duplicate code (since one always ends up getting forgotten and out of sync) this is now done later by caller
//		  strcpy (q->name, pLoad->m_pName);
//		  StripExtension (q->name);
//		  if (!g_dontuse)
//			q->inuse = true;
//	    q->next = g_qeglobals.d_qtextures;
//	    g_qeglobals.d_qtextures = q;
      g_pluginTexture = q;
    }
  }
}


/*
===============
Texture_CreateSolid

Create a single pixel texture of the apropriate color
===============
*/
qtexture_t *Texture_CreateSolid (const char *name)
{
	byte	data[4];
	qtexture_t	*q;

  q = (qtexture_t*)qmalloc(sizeof(*q));

  if (g_qeglobals.bSurfacePropertiesPlugin)
  {
	  // Timo
	  // Surface properties plugins can store their own data in an IPluginQTexture
	  q->pData = g_SurfaceTable.m_pfnQTextureAlloc( q );
	  GETPLUGINTEXDEF(q)->SetDefaultTexdef();
  }
	
	sscanf (name, "(%f %f %f)", &q->color[0], &q->color[1], &q->color[2]);

	data[0] = q->color[0]*255;
	data[1] = q->color[1]*255;
	data[2] = q->color[2]*255;
	data[3] = 255;

	q->width = q->height = 1;
	//q->width = q->height = 2;
  q->texture_number = texture_extension_number++;
	qglBindTexture( GL_TEXTURE_2D, q->texture_number );
	SetTexParameters ();

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
		qglTexImage2D(GL_TEXTURE_2D, 0, 3, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  }
  else
  {
	  if (nomips)
		  qglTexImage2D(GL_TEXTURE_2D, 0, 3, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	  else
		  qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, 1, 1,GL_RGBA, GL_UNSIGNED_BYTE, data);
  }
	qglBindTexture( GL_TEXTURE_2D, 0 );

	return q;
}


/*
=================
Texture_MakeDefault
=================
*/
qtexture_t* Texture_MakeDefault (void)
{
  qtexture_t	*q;
  byte		data[4][4];

  if (g_PrefsDlg.m_bSGIOpenGL)
  {
    if (s_hdcTexture && s_hglrcTexture)
    { 
       //if (!qwglMakeCurrent(g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase))
       if (!qwglMakeCurrent(s_hdcTexture, s_hglrcTexture))
		     Error ("wglMakeCurrent in LoadTexture failed");
    }
    else
      return NULL;
  }

  q = (qtexture_t*)qmalloc(sizeof(*q));
  
  strcpy (q->name, "notexture");
  q->width = q->height = 64;
  
  memset (data, 0, sizeof(data));
  data[0][2] = data[3][2] = 255;
  
  q->color[0] = 0;
  q->color[1] = 0;
  q->color[2] = 0.5;

  q->texture_number = texture_extension_number++;
  qglBindTexture( GL_TEXTURE_2D, q->texture_number );
  SetTexParameters ();

	if (nomips)
		qglTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		VERIFY(qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, 2, 2,GL_RGBA, GL_UNSIGNED_BYTE, data) == 0);

  qglBindTexture( GL_TEXTURE_2D, 0 );
  return q;
}


/*
=================
Texture_MakeNotexture
=================
*/
void Texture_MakeNotexture (void)
{
  notexture = Texture_MakeDefault();
  // Timo
  // Surface properties plugins can store their own data in an IPluginQTexture
  if (g_qeglobals.bSurfacePropertiesPlugin)
  {
	  notexture->pData = g_SurfaceTable.m_pfnQTextureAlloc( notexture );
	  GETPLUGINTEXDEF(notexture)->SetDefaultTexdef();
  }
}


void DemandLoadShaderTexture(qtexture_t *q, CShaderInfo *pShader)
{
  char cWork[1024];
  char cWork2[1024];
  strcpy(cWork, (pShader->m_strTextureName.GetLength() > 0) ? pShader->m_strTextureName  : pShader->m_strName);
  StripExtension(cWork);
  // TTimo: if the shader has a m_fTransValue != 1.0f, ignore the alpha channel when loading the texture
  // in some cases (common/weapclip) the 32bit .tga has an empty alpha channel,
  // causing a display bug in the camera view (brush does not seemed drawn since alpha==0 for the texture)
  // NOTE: the workaround is not perfect, the same texture may have been loaded earlier with it's alpha channel
  q = Texture_ForName (cWork, false, true, pShader->m_fTransValue != 1.0f, true, false);

  if (q == NULL || q == notexture) {
    sprintf(cWork2, "%s/%s",ValueForKey(g_qeglobals.d_project_entity, "basepath"), cWork);
    q = Texture_ForNamePath( cWork, cWork2);

    if (q == NULL || q == notexture) {
      q = Texture_ForName (cWork, false, true, pShader->m_fTransValue != 1.0f, true, true);
    }
  }

  if (q != NULL && q != notexture)
  {
    pShader->m_pQTexture = q;
    strcpy(q->shadername, pShader->m_strShaderName);
    q->bFromShader = true;
    q->fTrans = pShader->m_fTransValue;
    q->nShaderFlags = pShader->m_nFlags;
    strcpy(q->name, pShader->m_strName );
  }
  else
  {
    Sys_Printf("Could not load shader editor image %s\n", cWork);
  }
}


void LoadShader(char* pFilename, qtexture_t *q)
{
  char* pBuff = NULL;
  CString strTexture;
  int nSize = LoadFile(pFilename, reinterpret_cast<void**>(&pBuff));
  if (nSize == -1)
  {
    nSize = PakLoadAnyFile(pFilename, reinterpret_cast<void**>(&pBuff));
  }
  if (nSize > 0)
  {
    StartTokenParsing(pBuff);
    while (GetToken(true))
    {
      // first token should be the path + name.. (from base)
      CShaderInfo *pShader = new CShaderInfo();
      pShader->setName(token);
      pShader->m_strShaderName = pFilename;
      strTexture = token;
      bool bGood = true;
      float fTrans = 1.0;
      GetToken(true);
      if (strcmp(token, "{"))
      {
        bGood = false;
        break;
      }
      else
      {
        // we need to read until we hit a balanced }
        int nMatch = 1;
        while (nMatch > 0 && GetToken(true))
        {
          if (strcmp(token, "{") == 0)
          {
            nMatch++;
          }
          else if (strcmp(token, "}") == 0)
          {
            nMatch--;
          }
          else if (strcmpi(token, "qer_nocarve") == 0)
          {
            pShader->m_nFlags |= QER_NOCARVE;
          }
          else if (strcmpi(token, "qer_trans") == 0)
          {
            if (GetToken(true))
            {
              fTrans = atof(token);
            }
            pShader->m_nFlags |= QER_TRANS;
          }
          else if (strcmpi(token, "qer_editorimage") == 0)
          {
            if (GetToken(true))
            {
              char* pTex = copystring(token);
          		QE_ConvertDOSToUnixName( pTex, pTex );
              CString str = pTex;
              free (pTex);
              FindReplace(str, "textures/", "");
              FindReplace(str, ".tga", "");
              int nPos = str.Find('/');
              if (nPos == -1)
              {
                nPos = str.Find('\\');
              }
              if (nPos >= 0)
              {
                pShader->m_strTextureName = str;
		 					  pShader->m_strTextureName.MakeLower();	//avoid problems with case
/*
                else
                {
                  CString strPath = str.Left(nPos+1);
                  DeferredShaderLoad *deferred = new DeferredShaderLoad(str, pShader->m_strName, strPath); 
                  g_lstDeferred.Add(deferred);
                }
*/
              }
            }
          }
          else if (strcmpi(token, "surfaceparm") == 0)
          {
            //--while (GetToken(false))
            //--{
            //--
            //--}
            if (GetToken(true))
            {
              // next token should be a surface parm
              //--if (strcmpi(token, "trans") == 0)
              //--{
              //--  fTrans = 0.33;
              //--}
              if (strcmpi(token, "fog") == 0)
              {
                if (fTrans == 1.0) // has not been explicitly set by qer_trans
                {
                  fTrans = 0.35;
                }
              }
            }
          }
        }
        if (nMatch != 0)
        {
          bGood = false;
          break;
        }
      }
      //--if (bGood && q)
      if (bGood)
      {
        pShader->m_fTransValue = fTrans;
        g_lstShaders.Add(pShader);

        int n = g_PrefsDlg.m_nShader;
        if (g_PrefsDlg.m_nShader == CPrefsDlg::SHADER_ALL || (g_PrefsDlg.m_nShader == CPrefsDlg::SHADER_COMMON && strstr(pShader->m_strName, "common" )))
        {
// new 
          if (pShader->m_strTextureName.GetLength() > 0)
          {
            if (!ShaderQTextureExists(pShader->m_strName))
            {
              DemandLoadShaderTexture(q, pShader);
            }
          }
        }
// end new 
        //--q->bFromShader = true;
        //--q->fTrans = fTrans;

        //--// good texture here
        //--//Sys_Printf("Test load texture %s\n", strTexture);
        //--// FIXME.. this is a load of crap
        //--strcpy(dirstring, strTexture);
        //--QE_ConvertDOSToUnixName(dirstring, dirstring);
        //--strTexture = dirstring;
        //--FindReplace(strTexture, "textures/", "");
		    //--qtexture_t *q = Texture_ForName (strTexture.GetBuffer(0));
        //--if (q != NULL)
        //--{
        //--  q->bFromShader = true;
        //--  q->fTrans = fTrans;
        //--}
      }
      else
      {
        Sys_Printf("Error parsing shader at texture %s\n", strTexture);
      }

    }
    free (pBuff);
  }
  else
  {
    Sys_Printf("Unabled to read shader %s\n", pFilename);
  }
}


extern bool DoesFileExist(const char* pBuff, long& lSize);
CShaderInfo* SetNameShaderInfo(qtexture_t* q, const char* pPath, const char* pName)
{
  CShaderInfo *pInfo = hasShader(pName);
  if (pInfo)
  {
    strcpy(q->shadername, pInfo->m_strShaderName);
    q->bFromShader = true;
    q->fTrans = pInfo->m_fTransValue;
    q->nShaderFlags = pInfo->m_nFlags;
  }
  else
  {
    q->shadername[0] = 0;
  }
  strncpy (q->name, pName, sizeof(q->name) - 1);
	StripExtension (q->name);
  return pInfo;
}

void ReplaceQTexture(qtexture_t *pOld, qtexture_t *pNew, brush_t *pList)
{ 
	for (brush_t* pBrush = pList->next ; pBrush != pList; pBrush = pBrush->next)
	{
		if (pBrush->patchBrush)
		{
			Patch_ReplaceQTexture(pBrush, pOld, pNew);
		}
		if (pBrush->terrainBrush)
		{
			Terrain_ReplaceQTexture(pBrush->pTerrain, pOld, pNew);
		}

		for (face_t* pFace = pBrush->brush_faces; pFace; pFace = pFace->next)
		{
			if (pFace->d_texture == pOld)
			{
			pFace->d_texture = pNew;
			}
		}
		
		//Brush_Build(pBrush);
	}
}


void Texture_Remove(qtexture_t *q)
{
  qtexture_t* pTex = g_qeglobals.d_qtextures->next;
  if (q == g_qeglobals.d_qtextures)   // it is the head
  {
    g_qeglobals.d_qtextures->next = q->next->next;
    g_qeglobals.d_qtextures = q->next;
  }
  else
  {
    qtexture_t* pLast = g_qeglobals.d_qtextures;
    while (pTex != NULL && pTex != g_qeglobals.d_qtextures)
    {
      if (pTex == q)
      {
        pLast->next = q->next;
        break;
      }
      pLast = pTex;
      pTex = pTex->next;
    }
  }
  qglDeleteTextures(1, reinterpret_cast<const unsigned int*>(&q->texture_number));

  if (g_qeglobals.bSurfacePropertiesPlugin)
  {
	  // Timo
	  // Surface properties plugin
#ifdef _DEBUG
	  if ( !q->pData )
		  Sys_Printf("WARNING: found a qtexture_t* with no IPluginQTexture\n");
#endif
	  if ( q->pData )
		  GETPLUGINTEXDEF(q)->DecRef();
  }

  free(q);

}

/*
=================
Texture_MakeNoShadertexture

Make a default black/red check pattern texture
=================
*/
qtexture_t * Texture_MakeNoshadertexture( const char *name )
{
	qtexture_t	*q;
	byte		data[4][4];

	notexture = q = (qtexture_t*)qmalloc(sizeof(*q));
	q->width = q->height = 64;
	q->fTrans = 1;

	q = (qtexture_t*)qmalloc(sizeof(*q));
  strcpy (q->name, name);

	q->width = q->height = 64;
	q->fTrans = 1;

	memset (data, 0, sizeof(data));
	data[0][0] = data[3][0] = 255;

	q->color[0] = 0;
	q->color[1] = 0;
	q->color[2] = 0.5;

	q->texture_number = texture_extension_number++;
	qglBindTexture( GL_TEXTURE_2D, q->texture_number );
	SetTexParameters ();

	if (nomips)
		qglTexImage2D(GL_TEXTURE_2D, 0, 3, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		VERIFY(qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, 2, 2,GL_RGBA, GL_UNSIGNED_BYTE, data) == 0);

	qglBindTexture( GL_TEXTURE_2D, 0 );

	return q;
}



/*
===============
Texture_ForName
===============
*/
//bReload is set to true when called from DemandLoadShaderTexture because it should never re-use
//an already loaded texture
qtexture_t *Texture_ForName (const char *name, bool bReplace, bool bShader, bool bNoAlpha, bool bReload, bool makeShader)
{
  byte    *lump;
	qtexture_t	*q = NULL;
	char	filename[1024];
	
  if (name == NULL || strlen(name) == 0)
		return notexture;
	
	qtexture_t *pRemove = NULL;
	
	if (!bReload)
	{
		for (q=g_qeglobals.d_qtextures ; q ; q=q->next)
		{
			if (!strcmp(name,  q->name))
			{
				if (bReplace)
				{
					pRemove = q;
					//Texture_Remove(q);
					break;
				}
				else
				{
#ifdef _DEBUG
					// if the texture is already in memory, then the bNoAlpha flag doesn't have any influence
					if (bNoAlpha)
						Sys_Printf("WARNING: bNoAlpha flag on an already loaded texture\n");
#endif
					if (!g_dontuse)
					{
						q->inuse = true;
					}
					return q;
				}
			}
		}
	}

  // did not find it in the standard list
  // skip entity names (
  if (!bShader && name[0] != '(')
  {
    CShaderInfo* pShader = hasShader(name);
    if (pShader)
    {
      if (pShader->m_pQTexture == NULL)
      {
        DemandLoadShaderTexture(q, pShader);
      }
      q = pShader->m_pQTexture;
      //Sys_Printf ("used Shader %s.\n", pShader->m_strName);
    }
    if ( q != NULL)
    {
      return q;
    }
  }

	
	if (name[0] == '(')
	{
		q = Texture_CreateSolid (name);
		strncpy (q->name, name, sizeof(q->name)-1);
	}
	else
	{
		// FIXME: this is a mess.. need to move consolidate stuff
		// down to a single routine.. 
		// 
		// if plugins have a texture loader
		// {
		//   
		// }
		// else
		// 
		if (g_pParentWnd->GetPlugInMgr().GetTextureInfo() != NULL)
		{
			// rad: 12/19/98
			// if the plugin is not a wad style then we need to treat it normally
			// otherwise return without trying to explicitly load the texture
			// as it should have been loaded by the wad style plugin at init time
			CString strTex = GetTextureExtension(0);
			sprintf (filename, "%s\\%s.%s", ValueForKey (g_qeglobals.d_project_entity, "texturepath"), name, strTex);
			if (!g_pParentWnd->GetPlugInMgr().GetTextureInfo()->m_bWadStyle)
      {   
				g_pParentWnd->GetPlugInMgr().LoadTexture(filename);        
				if (g_pluginTexture)
					q = g_pluginTexture;
			}
			else
			{
				return notexture;
				// wadstyle.. if we get here then we do not have it
			}
		}
		else
      // we need to try several formats here, or would it be better if we are given a complete name
			if (g_PrefsDlg.m_bHiColorTextures == TRUE)
			{
		    char cWork[1024];
		  	sprintf (filename, "%s/%s.tga", ValueForKey (g_qeglobals.d_project_entity, "texturepath"), name);
    		QE_ConvertDOSToUnixName( cWork, filename );
		    strcpy(filename, cWork);
				Sys_Printf ("Loading %s...", name);
				unsigned char* pPixels = NULL;
				int nWidth;
				int nHeight;
				LoadImage(filename, &pPixels, &nWidth, &nHeight);
        if (pPixels == NULL)
        {
          // try jpg
          // blatant assumption of .tga should be fine since we sprintf'd it above
          int nLen = strlen(filename);
          filename[nLen-3] = 'j';
          filename[nLen-2] = 'p';
          filename[nLen-1] = 'g';
				  LoadImage(filename, &pPixels, &nWidth, &nHeight);
        }
				if (pPixels)
				{
					// if we were asked to ignore alpha channel, do it now (.TGA is the only supported file type with alpha channel)
					//if (bNoAlpha)
					if (TRUE)
					{
						unsigned char* iPix = pPixels;
						int nCount = nWidth * nHeight;
						for(iPix=pPixels+3; iPix-pPixels < nCount*4; iPix+=4)
							*iPix = 255;
					}
          // we'll be binding the GL texture now
          // need to check we are using a right GL context
          // with GL plugins that have their own window, the GL context may be the plugin's, in which case loading textures will bug
	        HDC currentHDC = qwglGetCurrentDC();
	        HGLRC currentHGLRC = qwglGetCurrentContext();
          //++timo FIXME: this may duplicate with qtexture_t* WINAPI QERApp_Texture_ForName (const char *name)
          //++timo FIXME: we need a list of lawfull GL contexts or something?
          // I'd rather always use the same GL context for binding...
          if (currentHDC != g_qeglobals.d_hdcBase || currentHGLRC != g_qeglobals.d_hglrcBase)
	          qwglMakeCurrent( g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase );
					q = Texture_LoadTGATexture(pPixels, nWidth, nHeight, NULL, 0, 0, 0);
          //++timo I don't set back the GL context .. I don't know how safe it is
          //qwglMakeCurrent( currentHDC, currentHGLRC );
    	//++timo storing the filename .. will be removed by shader code cleanup
		// this is dirty, and we sure miss some places were we should fill the filename info
		strcpy( q->filename, name );
					SetNameShaderInfo(q, filename, name);
					Sys_Printf ("done.\n", name);
					free(pPixels);
				}
			}
			else
			{
				// load the file
				sprintf (filename, "%s/%s.wal", ValueForKey (g_qeglobals.d_project_entity, "texturepath"),	name);
				Sys_Printf ("Loading %s...", name);
				if (LoadFile (filename, (void**)&lump) == -1)
				{
					sprintf (filename, "%s.wal", name);
					Sys_Printf("failed.. trying pak0.pak..");
					if(!PakLoadFile(filename, (void**)&lump))
					{
						Sys_Printf (" load failed!\n");
						return notexture;
					}
				}
				Sys_Printf("successful.\n");
				q = Texture_LoadTexture ((miptex_t *)lump);
				free (lump);
				strncpy (q->name, name, sizeof(q->name)-1);
				StripExtension (q->name);
			}
			
 			if (g_PrefsDlg.m_bSGIOpenGL)
			{
				if(!q)
					return notexture;
			}
			
	}// name[0] != '('
	
	if(!q)	// safety
  {
    if (bShader && !makeShader) {
      return q;
    }
  
    if (bShader)
    {
      q = Texture_MakeNoshadertexture( name );
	    Sys_Printf("failed, using default shader\n");
    }
    else
    {
      q = Texture_MakeDefault();
	    Sys_Printf("failed, using default\n");
    }
  }
	
  strncpy (q->name, name, sizeof(q->name)-1);
  if (name[0] != '(')
  {
    StripExtension (q->name);
  }

	if (!g_dontuse)
		q->inuse = true;
	q->next = g_qeglobals.d_qtextures;
	g_qeglobals.d_qtextures = q;
	
	if (pRemove != NULL)
	{
		ReplaceQTexture(pRemove, q, &active_brushes);
		ReplaceQTexture(pRemove, q, &filtered_brushes);
		Texture_Remove(pRemove);
	}
	
	return q;
}

/*
===============
Texture_ForNamePath
===============
*/
qtexture_t *Texture_ForNamePath(char* name, char* pFullPath)
{
  byte    *lump;
	qtexture_t	*q;
	char	filename[1024];

  if (strlen(name) == 0)
    return notexture;

	for (q=g_qeglobals.d_qtextures ; q ; q=q->next)
  {
	  if (!strcmp(name,  q->name))
		{
			if (!g_dontuse)
				q->inuse = true;
		    return q;
		}
  }

	if (name[0] == '(')
	{
		q = Texture_CreateSolid (name);
		strncpy (q->name, name, sizeof(q->name)-1);
	}
	else
	{
		// load the file
    if (g_PrefsDlg.m_bHiColorTextures == TRUE)
    {
      char cWork[1024];
      if (strstr(pFullPath, ".tga") == NULL) {
        sprintf(filename, "%s%s", pFullPath, ".tga");
      } else {
        strcpy(filename, pFullPath);
      }
  		QE_ConvertDOSToUnixName( cWork, filename );
	    strcpy(filename, cWork);
		  Sys_Printf ("Loading %s...", name);
      unsigned char* pPixels = NULL;
      int nWidth;
      int nHeight;
      LoadImage(filename, &pPixels, &nWidth, &nHeight);
      if (!pPixels)
      {
        // try jpg
        // blatant assumption of .tga should be fine since we sprintf'd it above
        int nLen = strlen(filename);
        filename[nLen-3] = 'j';
        filename[nLen-2] = 'p';
        filename[nLen-1] = 'g';
				LoadImage(filename, &pPixels, &nWidth, &nHeight);
      }
      if (pPixels)
      {
        q = Texture_LoadTGATexture(pPixels, nWidth, nHeight, NULL, 0, 0, 0);
    	//++timo storing the filename .. will be removed by shader code cleanup
		// this is dirty, and we sure miss some places were we should fill the filename info
		// NOTE: we store relative path, need to extract it
		strcpy( q->filename, name );

      }
      else
      {
        return notexture;
      }
      free(pPixels);
    }
    else
    {
      sprintf(filename, "%s%s", pFullPath, ".wal");
		  Sys_Printf ("Loading %s...", name);
		  if (LoadFile (filename, (void**)&lump) == -1)
      {
			  Sys_Printf (" load failed!\n");
			  return notexture;
      }
      Sys_Printf("successful.\n");
		  q = Texture_LoadTexture ((miptex_t *)lump);
		  free (lump);
    }
    if (g_PrefsDlg.m_bSGIOpenGL)
    {
		  if(!q)
			  return notexture;
    }
		strncpy (q->name, name, sizeof(q->name)-1);
		StripExtension (q->name);
	}

	if (!g_dontuse)
		q->inuse = true;
	q->next = g_qeglobals.d_qtextures;
	g_qeglobals.d_qtextures = q;

  return q;
}



/*
==================
FillTextureMenu

==================
*/
void FillTextureMenu (CStringArray* pArray)
{
	HMENU	hmenu;
	int		i;
	struct _finddata_t fileinfo;
	int		handle;
	char	dirstring[1024];
	char	*path;
	DIRLIST	*list = NULL, *temp;

  if (g_pParentWnd->GetPlugInMgr().GetTextureInfo() != NULL)
  {
    if (g_pParentWnd->GetPlugInMgr().GetTextureInfo()->m_bWadStyle)
      return;
  }

	hmenu = GetSubMenu (GetMenu(g_qeglobals.d_hwndMain), MENU_TEXTURE);

	// delete everything
	for (i=0 ; i<texture_nummenus ; i++)
		DeleteMenu (hmenu, CMD_TEXTUREWAD+i, MF_BYCOMMAND);

  texture_nummenus = 0;

	// add everything
  if (g_qeglobals.d_project_entity)
  {
    //--if (g_PrefsDlg.m_bUseShaders)
    //--{
	  //--  path = ValueForKey (g_qeglobals.d_project_entity, "basepath");
	  //--  sprintf (dirstring, "%s/scripts/*.shader", path);
    //--
    //--}
    //--else
    //--{
	    path = ValueForKey (g_qeglobals.d_project_entity, "texturepath");
	    sprintf (dirstring, "%s/*.*", path);
    //--}

	  handle = _findfirst (dirstring, &fileinfo);
	  if (handle != -1)
    {
	    do
	    {
        //--if (g_PrefsDlg.m_bUseShaders)
        //--{
		    //--  if ((fileinfo.attrib & _A_SUBDIR))
        //--    continue;
        //--}
        //--else
        //--{
		      if (!(fileinfo.attrib & _A_SUBDIR))
		        continue;
		      if (fileinfo.name[0] == '.')
		        continue;
        //--}
        // add this directory to the menu
	      AddToDirListAlphabetized(&list, fileinfo.name, FROMDISK);
	    } while (_findnext( handle, &fileinfo ) != -1);

	    _findclose (handle);
    }

    //--if (!g_PrefsDlg.m_bUseShaders)
    //--{
      GetPackTextureDirs(&list);
    //--}

	  for(temp = list; temp; temp = temp->next)
	  {
		  AppendMenu (hmenu, MF_ENABLED|MF_STRING, CMD_TEXTUREWAD+texture_nummenus, (LPCTSTR)temp->dirname);
		  strcpy (texture_menunames[texture_nummenus], temp->dirname);
      //--if (!g_PrefsDlg.m_bUseShaders)
      //--{
		    strcat (texture_menunames[texture_nummenus], "/");
      //--}
      if (pArray)
        pArray->Add(temp->dirname);
		  if (++texture_nummenus == MAX_TEXTUREDIRS)
		   break;
	  }

	  ClearDirList(&list);
  }


}


/*
==================
Texture_ClearInuse

A new map is being loaded, so clear inuse markers
==================
*/
void Texture_ClearInuse (void)
{
	qtexture_t	*q;

	for (q=g_qeglobals.d_qtextures ; q ; q=q->next)
    {
		q->inuse = false;
	}
}




void LoadShadersFromDir(const char *pPath)
{
  int nSize = g_lstShaders.GetSize();
  for (int i = 0; i < nSize; i++)
  {
    CShaderInfo *pInfo = reinterpret_cast<CShaderInfo*>(g_lstShaders.ElementAt(i));
    if (pInfo != NULL)
    {
      if (strstr(pInfo->m_strName, pPath) && pInfo->m_pQTexture == NULL && strstr(pInfo->m_strName, "models/player") == NULL)
      {
        qtexture_t *q = NULL;
        DemandLoadShaderTexture(q, pInfo);
      }
    }
  }
}


/*
==============
Texture_ShowDirectory
==============
*/
void	Texture_ShowDirectory (int menunum, bool bLinked)
{
	struct _finddata_t fileinfo;
	int		handle;
	char	name[1024];
	char	dirstring[1024];
	char	linkstring[1024];
	FILELIST			*list = NULL, *temp;
  CString strTemp;

  //Texture_Flush(false);
	//Select_Deselect();
	Texture_ClearInuse();
	texture_showinuse = false;
	strcpy (texture_directory, texture_menunames[menunum-CMD_TEXTUREWAD]);

  if (g_pParentWnd->GetPlugInMgr().GetTextureInfo() != NULL)
  {
    if (g_pParentWnd->GetPlugInMgr().GetTextureInfo()->m_bWadStyle)
      return;
  }

  // new
/*
  if (!g_PrefsDlg.m_bShaderTest)
  {
	g_dontuse = true;	// needed because this next piece of code calls Texture_ForName() internally! -slc
    LoadDeferred(texture_directory);
    g_dontuse = false;
  }
*/
  if (g_PrefsDlg.m_bHiColorTextures == FALSE)
  {
  }

	g_qeglobals.d_texturewin.originy = 0;

  //--if (g_PrefsDlg.m_bUseShaders)
  //--{
  //--  sprintf (dirstring, "%s/scripts/%s", ValueForKey (g_qeglobals.d_project_entity, "basepath"), texture_directory);
	//--  Sys_Printf("loading textures from shader %s\n", dirstring);
  //--  LoadShader(dirstring);
  //--}
  //--else
  //--{
	  Sys_Status("Loading textures\n", 0);

	  // load all image files
                                          
    sprintf (linkstring, "%s/textures/%stextureinfo.ini", ValueForKey (g_qeglobals.d_project_entity, "basepath"), texture_menunames[menunum-CMD_TEXTUREWAD]);

    for (int nExt = 0; nExt < GetTextureExtensionCount(); nExt++)
    {
      sprintf (dirstring, "%s/textures/%s*.%s", ValueForKey (g_qeglobals.d_project_entity, "basepath"), texture_menunames[menunum-CMD_TEXTUREWAD],GetTextureExtension(nExt));
      Sys_Printf ("Scanning %s\n", dirstring);
	    handle = _findfirst (dirstring, &fileinfo);

      if (handle == -1)
      {
        sprintf(dirstring, "%s/%s*.%s", ValueForKey (g_qeglobals.d_project_entity, "texturepath"), texture_menunames[menunum-CMD_TEXTUREWAD],GetTextureExtension(nExt));
        handle = _findfirst (dirstring, &fileinfo);
      }
      if (handle != -1)
	    {
		    do
  		  {
	  		  sprintf (name, "%s%s", texture_directory, fileinfo.name);
		  	  AddToFileListAlphabetized(&list, name, FROMDISK, 0, false);
  		  } while (_findnext( handle, &fileinfo ) != -1);
	  	  _findclose (handle);
  	  }
	    else
	    {
	      sprintf (dirstring, "%s*.%s", texture_menunames[menunum-CMD_TEXTUREWAD],GetTextureExtension(nExt));
  		  GetPackFileList(&list, dirstring);
	    }
    }

	  g_dontuse = true;
	  for(temp = list; temp; temp = temp->next)
	  {
		  if(temp->offset == -1)
			  sprintf(name, "%s", temp->filename);
		  else
			  sprintf(name, "%s%s", texture_menunames[menunum-CMD_TEXTUREWAD], temp->filename);
		  StripExtension (name);
      strTemp = name;
      strTemp.MakeLower();
      if ( strTemp.Find(".specular") >= 0 ||
           strTemp.Find(".glow") >= 0 ||
           strTemp.Find(".bump") >= 0 ||
           strTemp.Find(".diffuse") >= 0 ||
           strTemp.Find(".blend") >= 0 ||
           strTemp.Find(".alpha") >= 0
         )
        continue;
      else
      {
		    Texture_ForName (name, true);
      }
	  }

	  ClearFileList(&list);
  //--}


	g_dontuse = false;

  if (!bLinked)
  {

    for (int k = 0; k < 10; k++)
    {
      sprintf(name, "Path%d", k);
      if (GetPrivateProfileString("Include", name, "", dirstring, 1024, linkstring) > 0)
      {
        Texture_ShowDirectory(dirstring, true);
      }
    }

    LoadShadersFromDir(texture_directory);

    SortTextures();
	  
    sprintf (name, "Textures: %s", texture_directory);
	  SetWindowText(g_qeglobals.d_hwndEntity, name);

	  // select the first texture in the list
	  if (!g_qeglobals.d_texturewin.texdef.name[0])
		  SelectTexture (16, g_qeglobals.d_texturewin.height -16, false);
  }
}


// this can be combined with the above, but per usual i am in a hurry
//
void	Texture_ShowDirectory (char* pPath, bool bLinked)
{
	struct _finddata_t fileinfo;
	int		handle;
	char	name[1024];
	char	dirstring[1024];
	char	linkstring[1024];
	FILELIST			*list = NULL, *temp;

  //Texture_Flush(false);

	texture_showinuse = false;
	Texture_ClearInuse();
	strcpy (texture_directory, pPath);

  if (g_PrefsDlg.m_bHiColorTextures == FALSE)
  {
  }

	g_qeglobals.d_texturewin.originy = 0;
	Sys_Status("loading all textures\n", 0);

	// load all .wal files
  for (int nExt = 0; nExt < GetTextureExtensionCount(); nExt++)
  {
    sprintf(dirstring, "%s*.%s", pPath,GetTextureExtension(nExt));
                                          
	  Sys_Printf ("Scanning %s\n", dirstring);

	  handle = _findfirst (dirstring, &fileinfo);

    if (handle != -1)
	  {
		  do
		  {
			  sprintf (name, "%s%s", texture_directory, fileinfo.name);
			  AddToFileListAlphabetized(&list, name, FROMDISK, 0, false);
		  } while (_findnext( handle, &fileinfo ) != -1);
		  _findclose (handle);
	  }
	  else
	  {
		  //sprintf (dirstring, "%s*.wal", texture_menunames[menunum-CMD_TEXTUREWAD]);
		  //if(!GetPackFileList(&list, dirstring))
			  return;
	  }
  }

	g_dontuse = true;
	for(temp = list; temp; temp = temp->next)
	{
		if(temp->offset == -1)
			sprintf(name, "%s", temp->filename);
		else
		  sprintf(name, "%s%s", pPath, temp->filename);
		StripExtension (name);

    int nLen = strlen(name)-1;
    ASSERT(nLen > 0);
    while (name[nLen] != '\\')
      nLen--;
    // found first one
    nLen--;
    ASSERT(nLen > 0);
    while (name[nLen] != '\\')
      nLen--;
    ASSERT(nLen >= 0);
    QE_ConvertDOSToUnixName(name, name);
    Texture_ForName(&name[nLen+1]);

	}

	ClearFileList(&list);

	g_dontuse = false;

  SortTextures();

  if (!bLinked)
  {

    for (int k = 0; k < 10; k++)
    {
      sprintf(name, "Path%d", k);
      if (GetPrivateProfileString("Include", name, "", dirstring, 1024, linkstring) > 0)
      {
        Texture_ShowDirectory(dirstring, true);
      }
    }


	  sprintf (name, "Textures: %s", texture_directory);
	  SetWindowText(g_qeglobals.d_hwndEntity, name);

	  // select the first texture in the list
	  if (!g_qeglobals.d_texturewin.texdef.name[0])
		  SelectTexture (16, g_qeglobals.d_texturewin.height -16 ,false);
  }
}



void Texture_ResetPosition()
{
  SelectTexture (16, g_qeglobals.d_texturewin.height -16 ,false);
  g_qeglobals.d_texturewin.originy = 0;
}



/*
==================
Texture_SetInuse

==================
*/
void Texture_SetInuse (void)
{
	qtexture_t	*q;

	for (q=g_qeglobals.d_qtextures ; q ; q=q->next)
  {
		q->inuse = true;
	}
}


/*
==============
Texture_ShowAll
==============
*/
void	Texture_ShowAll()
{
  Texture_SetInuse();
	Sys_Printf("Showing all textures...\n");
	Sys_UpdateWindows (W_TEXTURE);
}

/*
==============
Texture_ShowInuse
==============
*/
void	Texture_ShowInuse (void)
{
	face_t	*f;
	brush_t	*b;
	char	name[1024];

	texture_showinuse = true;
	g_dontuse = false;

	g_qeglobals.d_texturewin.originy = 0;	

	Texture_ClearInuse();
	Sys_Status("Selecting active textures\n", 0);

	for (b=active_brushes.next ; b != NULL && b != &active_brushes ; b=b->next)
  {
    if (b->patchBrush)
    {
      Texture_ForName(b->pPatch->d_texture->name);
    }
    else
    {
		  for (f=b->brush_faces ; f ; f=f->next)
      {
			  Texture_ForName (f->texdef.name);
      }
    }
  }

	for (b=selected_brushes.next ; b != NULL && b != &selected_brushes ; b=b->next)
  {
    if (b->patchBrush)
    {
      Texture_ForName(b->pPatch->d_texture->name);
    }
    else
    {
		  for (f=b->brush_faces ; f ; f=f->next)
      {
			  Texture_ForName (f->texdef.name);
      }
    }
  }

	SortTextures();
	//SetInspectorMode(W_TEXTURE);
	Sys_UpdateWindows (W_TEXTURE);

	sprintf (name, "Textures: in use");
	SetWindowText(g_qeglobals.d_hwndEntity, name);

	// select the first texture in the list
	if (!g_qeglobals.d_texturewin.texdef.name[0])
  {
		SelectTexture (16, g_qeglobals.d_texturewin.height -16, false);
  }
}

/*
============================================================================

TEXTURE LAYOUT

============================================================================
*/

void Texture_StartPos (void)
{
	current_texture = g_qeglobals.d_qtextures;
	current_x = 8;
	current_y = -8;
	current_row = 0;
}

qtexture_t *Texture_NextPos (int *x, int *y)
{
	qtexture_t	*q;

	while (1)
	{
		q = current_texture;
		if (!q)
			return q;
		current_texture = current_texture->next;
		if (q->name[0] == '(')	// fake color texture
			continue;

    if (g_bFilterEnabled)
    {
      CString strName = q->name;
      int nPos = strName.Find('\\');
      if (nPos == -1)
        nPos = strName.Find('/');
      if (nPos >= 0)
        strName = strName.Right(strName.GetLength() - nPos - 1);
      if (strnicmp(g_strFilter.GetBuffer(0), strName, g_strFilter.GetLength()) == 0)
        break;
      else
        continue;
    }

    if (q->bFromShader && g_PrefsDlg.m_bShowShaders == FALSE)
    {
      continue;
    }

		if (q->inuse)
			break;			// always show in use
    
    if (!texture_showinuse && !strnicmp (q->name, texture_directory, strlen(texture_directory)))
			break;
		continue;
	}

  int nWidth = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->width * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->width;
  int nHeight = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->height * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->height;
	if (current_x + nWidth > g_qeglobals.d_texturewin.width-8 && current_row)
	{	// go to the next row unless the texture is the first on the row
		current_x = 8;
		current_y -= current_row + FONT_HEIGHT + 4;
		current_row = 0;
	}

	*x = current_x;
	*y = current_y;

	// Is our texture larger than the row? If so, grow the 
	// row height to match it

    if (current_row < nHeight)
		  current_row = nHeight;

	// never go less than 64, or the names get all crunched up
	current_x += nWidth < 64 ? 64 : nWidth;
	current_x += 8;

	return q;
}

/*
============================================================================

  MOUSE ACTIONS

============================================================================
*/

static	int	textures_cursorx, textures_cursory;


/*
============
Texture_SetTexture

brushprimit_texdef must be understood as a qtexture_t with width=2 height=2 ( the default one )
============
*/
void Texture_SetTexture (texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale, IPluginTexdef *pTexdef, bool bSetSelection )
{
	qtexture_t	*q;
	int			x,y;

	if (texdef->name[0] == '(')
	{
		Sys_Status("Can't select an entity texture\n", 0);
		return;
	}
	g_qeglobals.d_texturewin.texdef = *texdef;
	g_qeglobals.d_texturewin.texdef.flags &= ~SURF_KEEP;
	g_qeglobals.d_texturewin.texdef.contents &= ~CONTENTS_KEEP;
	// store the texture coordinates for new brush primitive mode
	// be sure that all the callers are using the default 2x2 texture
	if (g_qeglobals.m_bBrushPrimitMode)
	{
		g_qeglobals.d_texturewin.brushprimit_texdef = *brushprimit_texdef;
	}
	// surface properties plugin
	if (g_qeglobals.bSurfacePropertiesPlugin)
	{
		if (g_qeglobals.d_texturewin.pTexdef)
		{
			// decrement reference count
			static_cast<IPluginTexdef *>(g_qeglobals.d_texturewin.pTexdef)->DecRef();
			g_qeglobals.d_texturewin.pTexdef = NULL;
		}
		if (pTexdef)
		{
			g_qeglobals.d_texturewin.pTexdef = pTexdef->Copy();
		}
	}

	Sys_UpdateWindows (W_TEXTURE);

  g_dlgFind.updateTextures(texdef->name);

  if (!g_dlgFind.isOpen() && bSetSelection)
  {
    Select_SetTexture(texdef,brushprimit_texdef,bFitScale);
  }


	//plugins: send a message telling that the selected texture may have changed
	DispatchRadiantMsg( RADIANT_TEXTURE );

	// scroll origin so the texture is completely on screen
	Texture_StartPos ();
	while (1)
	{
		q = Texture_NextPos (&x, &y);
		if (!q)
			break;

    int nWidth = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->width * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->width;
    int nHeight = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->height * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->height;
		if (!strcmpi(texdef->name, q->name))
		{
			if (y > g_qeglobals.d_texturewin.originy)
			{
				g_qeglobals.d_texturewin.originy = y;
				Sys_UpdateWindows (W_TEXTURE);
				return;
			}

			if (y-nHeight-2*FONT_HEIGHT < g_qeglobals.d_texturewin.originy-g_qeglobals.d_texturewin.height)
			{
				g_qeglobals.d_texturewin.originy = y-nHeight-2*FONT_HEIGHT+g_qeglobals.d_texturewin.height;
				Sys_UpdateWindows (W_TEXTURE);
				return;
			}

			return;
		}
	}
}


HWND FindEditWindow()
{
  HWND hwnd = FindWindow("TEditPadForm", NULL);
  HWND hwndEdit = NULL;
  if (hwnd != NULL)
  {
    HWND hwndTab = FindWindowEx(hwnd, NULL, "TTabControl", NULL);
    if (hwndTab != NULL)
    {
      hwndEdit = FindWindowEx(hwndTab, NULL, "TRicherEdit", NULL);
    }
  }
  return hwndEdit;
}

void Delay(float fSeconds)
{
  DWORD dw = ::GetTickCount();
  DWORD dwTil = dw + (fSeconds * 1000);
  while (::GetTickCount() < dwTil)
  {
    MSG msg;
    if (::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) 
    { 
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}


void ViewShader(const char *pFile, const char *pName)
{
  CString str;
  char* pBuff = NULL;
  int nSize = LoadFile(pFile, reinterpret_cast<void**>(&pBuff));
  if (nSize == -1)
  {
    nSize = PakLoadAnyFile(pFile, reinterpret_cast<void**>(&pBuff));
  }
  if (nSize > 0)
  {
    str = pBuff;
  }
  int nStart = 0; 
  if (str.GetLength() > 0)
  {
    CString strFind = pName;
    CString strLook = str;
    strLook.MakeLower();
    strFind.MakeLower();
    int n = strLook.Find(strFind);
    if (n >= 0)
    {
      nStart = n;
    }
  }

  CString s= "editpad ";
  s += pFile;
  WinExec(s, SW_SHOWNORMAL);

  Delay(1.5);

  HWND hwndEdit = FindEditWindow();

  if (hwndEdit != NULL)
  {
    PostMessage(hwndEdit, EM_SETSEL, nStart, nStart);
  }
  else
  {
    Sys_Printf("Unable to load shader editor.\n");
  }


}

/*
==============
SelectTexture

  By mouse click
==============
*/
void SelectTexture (int mx, int my, bool bShift, bool bFitScale)
{
	int		x, y;
	qtexture_t	*q;
	texdef_t	tex;
	brushprimit_texdef_t brushprimit_tex;

	my += g_qeglobals.d_texturewin.originy-g_qeglobals.d_texturewin.height;
	
	Texture_StartPos ();
	while (1)
	{
		q = Texture_NextPos (&x, &y);
		if (!q)
			break;
		int nWidth = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->width * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->width;
		int nHeight = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->height * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->height;
		if (mx > x && mx - x < nWidth
			&& my < y && y - my < nHeight + FONT_HEIGHT)
		{
			if (bShift)
			{
				if (g_PrefsDlg.m_bHiColorTextures && q->shadername[0] != 0)
				{
					//CString s = "notepad ";
					//s += q->shadername;
					//WinExec(s, SW_SHOWNORMAL);	
	
					ViewShader(q->shadername, q->name);				

				}
			}
			memset (&tex, 0, sizeof(tex));
			memset (&brushprimit_tex, 0, sizeof(brushprimit_tex));
			if (g_qeglobals.m_bBrushPrimitMode)
			{
				// brushprimit fitted to a 2x2 texture
				brushprimit_tex.coords[0][0] = 1.0f;
				brushprimit_tex.coords[1][1] = 1.0f;
			}
			else
			{
				tex.scale[0] = (g_PrefsDlg.m_bHiColorTextures) ? 0.5 : 1;
				tex.scale[1] = (g_PrefsDlg.m_bHiColorTextures) ? 0.5 : 1;
			}
			tex.flags = q->flags;
			tex.value = q->value;
			tex.contents = q->contents;
			//strcpy (tex.name, q->name);
			tex.SetName(q->name);
			Texture_SetTexture ( &tex, &brushprimit_tex, bFitScale, GETPLUGINTEXDEF(q));
			CString strTex;
			CString strName = q->name;
			//int nPos = strName.Find('\\');
			//if (nPos == -1)
			//  nPos = strName.Find('/');
			//if (nPos >= 0)
			//  strName = strName.Right(strName.GetLength() - nPos - 1);
			strTex.Format("%s W: %i H: %i", strName.GetBuffer(0), q->width, q->height);
			g_pParentWnd->SetStatusText(3, strTex);
			return;
		}
	}

	Sys_Status("Did not select a texture\n", 0);
}

/*
==============
Texture_MouseDown
==============
*/
void Texture_MouseDown (int x, int y, int buttons)
{
	Sys_GetCursorPos (&textures_cursorx, &textures_cursory);

	// lbutton = select texture
	if (buttons == MK_LBUTTON || buttons == (MK_LBUTTON | MK_SHIFT) || buttons == (MK_LBUTTON | MK_CONTROL))
	{
    SelectTexture (x, g_qeglobals.d_texturewin.height - 1 - y, buttons & MK_SHIFT, buttons & MK_CONTROL);
    UpdateSurfaceDialog();
    UpdatePatchInspector();
	}
}

/*
==============
Texture_MouseUp
==============
*/
void Texture_MouseUp (int x, int y, int buttons)
{
}

/*
==============
Texture_MouseMoved
==============
*/
void Texture_MouseMoved (int x, int y, int buttons)
{
	int scale = 1;

	if ( buttons & MK_SHIFT )
		scale = 4;

	// rbutton = drag texture origin
	if (buttons & MK_RBUTTON)
	{
		Sys_GetCursorPos (&x, &y);
		if ( y != textures_cursory)
		{
			g_qeglobals.d_texturewin.originy += ( y-textures_cursory) * scale;
			if (g_qeglobals.d_texturewin.originy > 0)
				g_qeglobals.d_texturewin.originy = 0;
			Sys_SetCursorPos (textures_cursorx, textures_cursory);
      CWnd *pWnd = CWnd::FromHandle(g_qeglobals.d_hwndTexture);
      if (g_PrefsDlg.m_bTextureScrollbar && pWnd != NULL)
      {
        pWnd->SetScrollPos(SB_VERT, abs(g_qeglobals.d_texturewin.originy));
      }
		  InvalidateRect(g_qeglobals.d_hwndTexture, NULL, false);
		  UpdateWindow (g_qeglobals.d_hwndTexture);
		}
		return;
	}
}


/*
============================================================================

DRAWING

============================================================================
*/

int imax(int iFloor, int i) { if (i>iFloor) return iFloor; return i; }
HFONT ghFont = NULL;

/*
============
Texture_Draw2
============
*/
void Texture_Draw2 (int width, int height)
{
	qtexture_t	*q;
	int			x, y;
	char		*name;

	qglClearColor (
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][0],
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][1],
		g_qeglobals.d_savedinfo.colors[COLOR_TEXTUREBACK][2],
		0);
	qglViewport (0,0,width,height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();

	qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	qglDisable (GL_DEPTH_TEST);
	qglDisable(GL_BLEND);
	qglOrtho (0, width, g_qeglobals.d_texturewin.originy-height, g_qeglobals.d_texturewin.originy, -100, 100);
	qglEnable (GL_TEXTURE_2D);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	g_qeglobals.d_texturewin.width = width;
	g_qeglobals.d_texturewin.height = height;
	Texture_StartPos ();

	while (1)
	{
		q = Texture_NextPos (&x, &y);
		if (!q)
			break;

    int nWidth = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->width * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->width;
    int nHeight = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? q->height * ((float)g_PrefsDlg.m_nTextureScale / 100) : q->height;
		// Is this texture visible?
		if ( (y-nHeight-FONT_HEIGHT < g_qeglobals.d_texturewin.originy)
			&& (y > g_qeglobals.d_texturewin.originy - height) )
		{

			// if in use, draw a background
			if ((q->inuse && !texture_showinuse) || q->bFromShader)
			{
				qglLineWidth (1);

        if (q->bFromShader)
        {
				  qglColor3f (1,1,1);
        }
        else
        {
				  qglColor3f (0.5,1,0.5);
        }
				qglDisable (GL_TEXTURE_2D);

				qglBegin (GL_LINE_LOOP);
				qglVertex2f (x-1,y+1-FONT_HEIGHT);
				qglVertex2f (x-1,y-nHeight-1-FONT_HEIGHT);
				qglVertex2f (x+1+nWidth,y-nHeight-1-FONT_HEIGHT);
				qglVertex2f (x+1+nWidth,y+1-FONT_HEIGHT);
				qglEnd ();

				qglEnable (GL_TEXTURE_2D);
			}

			// Draw the texture
      float fScale = (g_PrefsDlg.m_bHiColorTextures == TRUE) ? ((float)g_PrefsDlg.m_nTextureScale / 100) : 1.0;

			qglBindTexture( GL_TEXTURE_2D, q->texture_number );
      QE_CheckOpenGLForErrors();
			qglColor3f (1,1,1);
			qglBegin (GL_QUADS);
			qglTexCoord2f (0,0);
			qglVertex2f (x,y-FONT_HEIGHT);
			qglTexCoord2f (1,0);
			qglVertex2f (x+nWidth,y-FONT_HEIGHT);
			qglTexCoord2f (1,1);
			qglVertex2f (x+nWidth,y-FONT_HEIGHT-nHeight);
			qglTexCoord2f (0,1);
			qglVertex2f (x,y-FONT_HEIGHT-nHeight);
			qglEnd ();

			// draw the selection border
			if (!strcmpi(g_qeglobals.d_texturewin.texdef.name, q->name))
			{
				qglLineWidth (3);
				qglColor3f (1,0,0);
				qglDisable (GL_TEXTURE_2D);

				qglBegin (GL_LINE_LOOP);
				qglVertex2f (x-4,y-FONT_HEIGHT+4);
				qglVertex2f (x-4,y-FONT_HEIGHT-nHeight-4);
				qglVertex2f (x+4+nWidth,y-FONT_HEIGHT-nHeight-4);
				qglVertex2f (x+4+nWidth,y-FONT_HEIGHT+4);
				qglEnd ();

				qglEnable (GL_TEXTURE_2D);
				qglLineWidth (1);
			}

			// draw the texture name
  	  qglColor3f (0,0,0);
			
      qglRasterPos2f (x, y-FONT_HEIGHT+2);

			// don't draw the directory name
			for (name = q->name ; *name && *name != '/' && *name != '\\' ; name++)
				;
			if (!*name)
				name = q->name;
			else
				name++;

      if (g_PrefsDlg.m_bHiColorTextures && q->shadername[0] != 0)
      {
        // slow as shit
        CString s = "[";
        s += name;
        s += "]";
			  qglCallLists (s.GetLength(), GL_UNSIGNED_BYTE, s.GetBuffer(0));
      }
      else
      {
			  qglCallLists (strlen(name), GL_UNSIGNED_BYTE, name);
      }
		}
	}

	g_qeglobals.d_texturewin.m_nTotalHeight = abs(y) + 100;
	// reset the current texture
	qglBindTexture( GL_TEXTURE_2D, 0 );
	qglFinish();
}


void Texture_Init (bool bHardInit)
{
	char	name[1024];
	byte	*pal = NULL;

  if (g_PrefsDlg.m_bHiColorTextures == FALSE)
  {
	  // load the palette
	  sprintf (name, "%s/pics/colormap.pcx", ValueForKey (g_qeglobals.d_project_entity, "basepath"));

	  Load256Image (name, NULL, &pal, NULL, NULL);
	  if (!pal)
    {
      // before dropping out, try to load it from the QERadiant directory
      CString strFile = g_strAppPath;
      AddSlash(strFile);
      strFile += "colormap.pcx";
	    Load256Image (strFile.GetBuffer(0), NULL, &pal, NULL, NULL);
	    if (!pal)
		    Sys_Printf ("Couldn't load %s or %s", name, strFile);
    }
    else
    {
	    Texture_InitPalette (pal);
	    free (pal);
    }
  }

	// create the fallback texture

  if (bHardInit)
  {
	  Texture_MakeNotexture();
	  g_qeglobals.d_qtextures = NULL;
  }
  LoadShaders();

}

void Texture_FlushUnused()
{
  CWaitCursor cursor;
  Texture_ShowInuse();
  if (g_qeglobals.d_qtextures)
  {
	  qtexture_t* pTex = g_qeglobals.d_qtextures->next;
    qtexture_t *pPrev = g_qeglobals.d_qtextures;
    while (pTex != NULL && pTex != g_qeglobals.d_qtextures)
    {
      qtexture_t* pNextTex = pTex->next;
  	  if (g_qeglobals.bSurfacePropertiesPlugin)
	    {
		    // Timo
		    // Surface properties plugin
#ifdef _DEBUG
  		  if ( !pTex->pData )
	  		  Sys_Printf("WARNING: found a qtexture_t* with no IPluginQTexture\n");
#endif
  		  if ( pTex->pData && pTex->inuse )
	  		  GETPLUGINTEXDEF(pTex)->DecRef();
  	  }

      if (!pTex->inuse)
      {
        unsigned int nTexture = pTex->texture_number;
        qglDeleteTextures(1, &nTexture);
        pPrev->next = pNextTex;
	      free(pTex);
      }
      else
      {
        pPrev = pTex;
      }
      pTex = pNextTex;
    }
  }
}

void Texture_Cleanup(CStringList *pList)
{
  if (g_qeglobals.d_qtextures)
  {
	  qtexture_t* pTex = g_qeglobals.d_qtextures->next;
    while (pTex != NULL && pTex != g_qeglobals.d_qtextures)
    {
      qtexture_t* pNextTex = pTex->next;
      if (pList)
      {
        if (pTex->name[0] != '(')
        {
          pList->AddTail(pTex->name);
        }
      }

  	  if (g_qeglobals.bSurfacePropertiesPlugin)
	    {
		    // Timo
		    // Surface properties plugin
#ifdef _DEBUG
  		  if ( !pTex->pData )
	  		  Sys_Printf("WARNING: found a qtexture_t* with no IPluginQTexture\n");
#endif
  		  if ( pTex->pData )
	  		  GETPLUGINTEXDEF(pTex)->DecRef();
  	  }
	    free(pTex);
      pTex = pNextTex;
    }
  }

  int nSize = g_lstSkinCache.GetSize();
  for (int i = 0; i < nSize; i++)
  {
    SkinInfo *pInfo = reinterpret_cast<SkinInfo*>(g_lstSkinCache.GetAt(i));
    delete pInfo;
  }

}

/*
==================
Texture_Flush
==================
*/
void Texture_Flush (bool bReload)
{
  if (!ConfirmModified())
    return;

  Map_New ();

  CWaitCursor cursor;
  CStringList strList;
  Texture_Init(false);
  Texture_Cleanup(&strList);

  GLuint* pGln = new GLuint[texture_extension_number-1];
  qglGenTextures(texture_extension_number-1, pGln);
  QE_CheckOpenGLForErrors();
  qglDeleteTextures(texture_extension_number-1, pGln);
  QE_CheckOpenGLForErrors();
  delete []pGln;
  texture_extension_number = 1;
	g_qeglobals.d_qtextures = NULL;

  if (bReload)
  {
    POSITION pos = strList.GetHeadPosition();
    while (pos)
    {
      CString strTex = strList.GetNext(pos);
		  Texture_ForName (strTex.GetBuffer(0));
    }
  }

}



/////////////////////////////////////////////////////////////////////////////
// CTexWnd
IMPLEMENT_DYNCREATE(CTexWnd, CWnd);

CTexWnd::CTexWnd()
{
  m_bNeedRange = true;
}

CTexWnd::~CTexWnd()
{
}


BEGIN_MESSAGE_MAP(CTexWnd, CWnd)
	//{{AFX_MSG_MAP(CTexWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PARENTNOTIFY()
	ON_WM_TIMER()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_TEXTURES_FLUSH, OnTexturesFlush)
  ON_BN_CLICKED(1200, OnShaderClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTexWnd message handlers

/*
============
WTexWndProc
============
*/
LONG WINAPI TexWndProc (
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
	int		xPos, yPos;
    RECT	rect;

    GetClientRect(hWnd, &rect);

    switch (uMsg)
    {
	case WM_CREATE:
    s_hdcTexture = GetDC(hWnd);
		QEW_SetupPixelFormat(s_hdcTexture, false);

		if ( ( s_hglrcTexture = qwglCreateContext( s_hdcTexture ) ) == 0 )
			Error( "wglCreateContext in WTex_WndProc failed" );

		if (!qwglShareLists( g_qeglobals.d_hglrcBase, s_hglrcTexture ) )
			Error( "wglShareLists in WTex_WndProc failed" );

    if (!qwglMakeCurrent( s_hdcTexture, s_hglrcTexture ))
		  Error ("wglMakeCurrent in WTex_WndProc failed");

	  g_qeglobals.d_hwndTexture = hWnd;
		return 0;

	case WM_DESTROY:
		//wglMakeCurrent( NULL, NULL );
		//wglDeleteContext( s_hglrcTexture );
		 ReleaseDC( hWnd, s_hdcTexture );
		return 0;
#if 0
	case WM_PAINT:
        { 
		    PAINTSTRUCT	ps;

		    BeginPaint(hWnd, &ps);

        if ( !qwglMakeCurrent( s_hdcTexture, s_hglrcTexture ) )
        //if ( !wglMakeCurrent( ps.hdc, s_hglrcTexture ) )
        {
          Sys_Printf("ERROR: wglMakeCurrent failed..\n ");
          Sys_Printf("Please restart Q3Radiant if the Texture view is not working\n");
        }
        else
        {
			    Texture_Draw2 (rect.right-rect.left, rect.bottom-rect.top - g_nTextureOffset);
			    qwglSwapBuffers(s_hdcTexture);
          TRACE("Texture Paint\n");
        }
		    EndPaint(hWnd, &ps);
        }
		return 0;
#endif
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		SetCapture( g_qeglobals.d_hwndTexture );
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		
		Texture_MouseDown (xPos, yPos - g_nTextureOffset, wParam);
		return 0;

	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		
		Texture_MouseUp (xPos, yPos - g_nTextureOffset, wParam);
		if (! (wParam & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)))
			ReleaseCapture ();
		return 0;

	case WM_MOUSEMOVE:
		xPos = (short)LOWORD(lParam);  // horizontal position of cursor 
		yPos = (short)HIWORD(lParam);  // vertical position of cursor 
		
		Texture_MouseMoved (xPos, yPos - g_nTextureOffset, wParam);
		return 0;
    }

    return DefWindowProc (hWnd, uMsg, wParam, lParam);
}



BOOL CTexWnd::PreCreateWindow(CREATESTRUCT& cs) 
{
  WNDCLASS wc;
  HINSTANCE hInstance = AfxGetInstanceHandle();
  if (::GetClassInfo(hInstance, TEXTURE_WINDOW_CLASS, &wc) == FALSE)
  {
    // Register a new class
  	memset (&wc, 0, sizeof(wc));
    wc.style         = CS_NOCLOSE | CS_OWNDC;
    wc.lpszClassName = TEXTURE_WINDOW_CLASS;
    wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
    wc.lpfnWndProc = TexWndProc;
    if (AfxRegisterClass(&wc) == FALSE)
      Error ("CZWnd RegisterClass: failed");
  }

  cs.lpszClass = TEXTURE_WINDOW_CLASS;
  cs.lpszName = "TEX";
  if (cs.style != QE3_CHILDSTYLE && cs.style != QE3_STYLE)
    cs.style = QE3_SPLITTER_STYLE;

	return CWnd::PreCreateWindow(cs);
}

int CTexWnd::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

  CRect rctEdit(8, 5, 20, 20);
  g_nTextureOffset = 0;

/*
  if (g_PrefsDlg.m_bShaderTest)
  {
    m_wndShaders.Create("Show Shaders", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, rctEdit, this, 1200);
    m_wndShaders.ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
    m_wndShaders.SetCheck(g_PrefsDlg.m_bShowShaders);
    g_nTextureOffset = 25;
  }
*/
  rctEdit.SetRect(8, g_nTextureOffset, 20, 20);
  m_wndFilter.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, rctEdit, this, 1201);
  m_wndFilter.ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
  m_wndFilter.SetTexWnd(this);

  g_nTextureOffset += 25;
  if (!g_PrefsDlg.m_bTextureWindow)
  {
    m_wndFilter.ShowWindow(SW_HIDE);
    g_nTextureOffset -= 25;
  }

  ShowScrollBar(SB_VERT, g_PrefsDlg.m_bTextureScrollbar);
  m_bNeedRange = true;

	return 0;
}

void CTexWnd::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);
  CRect rctClient;
  GetClientRect(rctClient);
/*
  if (g_PrefsDlg.m_bShaderTest && m_wndShaders.GetSafeHwnd())
  {
    m_wndShaders.SetWindowPos(NULL, rctClient.left + 8, rctClient.top + 5, rctClient.right - 16, 20, 0);
  }
*/
  m_wndFilter.SetWindowPos(NULL, rctClient.left + 8, rctClient.top + 25, rctClient.right - 16, 20, 0);
  m_bNeedRange = true;
}

void CTexWnd::OnShaderClick()
{
  g_PrefsDlg.m_bShowShaders = (m_wndShaders.GetCheck() != 0);
  g_PrefsDlg.SavePrefs();
	RedrawWindow();
}

void CTexWnd::OnParentNotify(UINT message, LPARAM lParam) 
{
	CWnd::OnParentNotify(message, lParam);
}

int g_nLastLen = 0;
int g_nTimerHandle = -1;
char g_cLastChar;

void CTexWnd::UpdateFilter(const char* pFilter)
{
  if (g_nTimerHandle > 0)
    KillTimer(1);
  g_bFilterEnabled = false;
  if (pFilter)
  {
    g_strFilter = pFilter;
    if (g_strFilter.GetLength() > 0)
    {
      g_bFilterEnabled = true;
      if (g_pParentWnd->CurrentStyle() == QR_QE4 || g_pParentWnd->CurrentStyle() == QR_4WAY)
      {
        if (g_strFilter.GetLength() > g_nLastLen)
        {
          g_cLastChar = toupper(g_strFilter.GetAt(g_strFilter.GetLength()-1));
          if (g_cLastChar == 'N' || g_cLastChar == 'O') // one of the other popups
          {
            g_nTimerHandle = SetTimer(1, 800, NULL);   // half second timer
          }
        }
      }
    }
    g_nLastLen = g_strFilter.GetLength();
	  SortTextures();
  }
  Sys_UpdateWindows (W_TEXTURE);
}

void CTexWnd::UpdatePrefs()
{
  if (!g_PrefsDlg.m_bTextureWindow)
  {
    m_wndFilter.ShowWindow(SW_HIDE);
    g_nTextureOffset = 0;
  }
  else
  {
    m_wndFilter.ShowWindow(SW_SHOW);
    g_nTextureOffset = 25;
  }
  ShowScrollBar(SB_VERT, g_PrefsDlg.m_bTextureScrollbar);
  m_bNeedRange = true;
  Invalidate();
  UpdateWindow();
}

void CTexWnd::FocusEdit()
{
  if (m_wndFilter.IsWindowVisible())
    m_wndFilter.SetFocus();
}

void CTexWnd::OnTimer(UINT nIDEvent) 
{
  KillTimer(1);
  g_nLastLen = 0;
  g_nTimerHandle = -1;
  ::SetFocus(g_qeglobals.d_hwndEntity);
  ::PostMessage(g_qeglobals.d_hwndEntity, WM_CHAR, g_cLastChar, 0);
}

void CTexWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  g_pParentWnd->HandleKey(nChar, nRepCnt, nFlags);
	//CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CTexWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  g_pParentWnd->HandleKey(nChar, nRepCnt, nFlags, false);
}

void CTexWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
  CRect rctClient;
  GetClientRect(rctClient);
  int nOld = g_qeglobals.d_texturewin.m_nTotalHeight;
  if (!qwglMakeCurrent(s_hdcTexture, s_hglrcTexture))
  //if ( !qwglMakeCurrent(dc.m_hDC, s_hglrcTexture ) )
  {
    Sys_Printf("ERROR: wglMakeCurrent failed..\n ");
    Sys_Printf("Please restart Q3Radiant if the Texture view is not working\n");
  }
  else
  {
    Texture_Draw2 (rctClient.right-rctClient.left, rctClient.bottom-rctClient.top - g_nTextureOffset);
		qwglSwapBuffers(s_hdcTexture);
    TRACE("Texture Paint\n");
  }
  if (g_PrefsDlg.m_bTextureScrollbar && (m_bNeedRange || g_qeglobals.d_texturewin.m_nTotalHeight != nOld))
  {
    m_bNeedRange = false;
    SetScrollRange(SB_VERT, 0, g_qeglobals.d_texturewin.m_nTotalHeight, TRUE);
  }
}

void CTexWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);

  int n = GetScrollPos(SB_VERT);;
  switch (nSBCode)
  {
    case SB_LINEUP :
    {
      n = (n - 15 >  0) ? n - 15 : 0; 
      break;
    }
    case SB_LINEDOWN :
    {
      n = (n + 15 < g_qeglobals.d_texturewin.m_nTotalHeight) ? n + 15 : n; 
      break;
    }
    case SB_PAGEUP :
    {
      n = (n - g_qeglobals.d_texturewin.height >  0) ? n - g_qeglobals.d_texturewin.height : 0; 
      break;
    }
    case SB_PAGEDOWN :
    {
      n = (n + g_qeglobals.d_texturewin.height < g_qeglobals.d_texturewin.m_nTotalHeight) ? n + g_qeglobals.d_texturewin.height : n; 
      break;
    }
    case SB_THUMBPOSITION :
    {
      n = nPos;
      break;
    }
    case SB_THUMBTRACK :
    {
      n = nPos;
      break;
    }
  }
  SetScrollPos(SB_VERT, n);
	g_qeglobals.d_texturewin.originy = -((int)n);
  Invalidate();
  UpdateWindow();
  //Sys_UpdateWindows(W_TEXTURE);
}

/*
and are the caps new caps?  anything done with older stuff will be fubar'd.. which brings up the point if you ever naturalize a cap, you cannot force it back to cap texturing.. i will add that too
*/

void CTexWnd::OnTexturesFlush() 
{
	// TODO: Add your command handler code here
	
}

void LoadShaders()
{
	char	dirstring[1024];
	char	*path;
	//struct _finddata_t fileinfo;
	//int		handle;
  path = ValueForKey (g_qeglobals.d_project_entity, "basepath");
  sprintf (dirstring, "%s/scripts/shaderlist.txt", path);
  char *pBuff = NULL;
  
  int nLen = LoadFile(dirstring, reinterpret_cast<void**>(&pBuff));
  if (nLen == -1)
  {
    nLen = PakLoadAnyFile(dirstring, reinterpret_cast<void**>(&pBuff));
  }
  if (nLen > 0)
  {
    CStringList lst;
    StartTokenParsing(pBuff);
    nLen = 0;
    while (GetToken(true))
    {
      // each token should be a shader filename
      sprintf(dirstring, "%s/scripts/%s.shader", path, token);
      lst.AddTail(dirstring);
      nLen++;
    }
    POSITION pos = lst.GetHeadPosition();
    while (pos != NULL)
    {
      LoadShader(lst.GetAt(pos).GetBuffer(0), NULL);
      lst.GetNext(pos);
    }
    free(pBuff);
  }
  else
  {
    Sys_Printf("Unable to load shaderlist.txt, shaders not loaded!");
  }

/*
  handle = _findfirst (dirstring, &fileinfo);
  if (handle != -1)
  {
    do
    {
      if ((fileinfo.attrib & _A_SUBDIR))
        continue;
      sprintf(dirstring, "%s/scripts/%s", path, fileinfo.name);
      LoadShader(dirstring, NULL);
	  } while (_findnext( handle, &fileinfo ) != -1);

	  _findclose (handle);
  }
*/
}

void FreeShaders()
{
  int nSize = g_lstShaders.GetSize();
  for (int i = 0; i < nSize; i++)
  {
    CShaderInfo *pInfo = reinterpret_cast<CShaderInfo*>(g_lstShaders.ElementAt(i));
    delete pInfo;
  }

  g_lstShaders.RemoveAll();
}

void ReloadShaders()
{
  FreeShaders();
  LoadShaders();
  qtexture_t* pTex = g_qeglobals.d_qtextures;
  while (pTex != NULL)
  {
    SetNameShaderInfo(pTex, NULL, pTex->name);
    pTex = pTex->next;
  }

}

int WINAPI Texture_LoadSkin(char *pName, int *pnWidth, int *pnHeight)
{
  byte *pic = NULL;
  byte *pic32 = NULL;
  int nTex = -1;

  strlwr(pName);
  QE_ConvertDOSToUnixName(pName, pName);

  int nSize = g_lstSkinCache.GetSize();
  for (int i = 0; i < nSize; i++)
  {
    SkinInfo *pInfo = reinterpret_cast<SkinInfo*>(g_lstSkinCache.GetAt(i));
    if (pInfo)
    {
      if (stricmp(pName, pInfo->m_strName) == 0)
      {
        return pInfo->m_nTextureBind;
      }
    }
  }

  LoadImage( pName, &pic32, pnWidth, pnHeight);
  if (pic32 != NULL)
  {

    nTex = texture_extension_number++;
    if (g_PrefsDlg.m_bSGIOpenGL)
    {
      //if (!qwglMakeCurrent(g_qeglobals.d_hdcBase, g_qeglobals.d_hglrcBase))
      if (!qwglMakeCurrent(s_hdcTexture, s_hglrcTexture))
		    Error ("wglMakeCurrent in LoadTexture failed");
    }

    qglBindTexture( GL_TEXTURE_2D, nTex);
    SetTexParameters ();

	  int nCount = MAX_TEXTURE_QUALITY - g_PrefsDlg.m_nTextureQuality;
	  while (nCount-- > 0)
	  {
	    if (*pnWidth > 16 && *pnHeight > 16)
	    {
	      R_MipMap(pic32, *pnWidth, *pnHeight);
	    }
	    else
	    {
	      break;
	    }
	  }

    if (g_PrefsDlg.m_bSGIOpenGL)
    {
	    if (nomips)
      {
		    qglTexImage2D(GL_TEXTURE_2D, 0, 3, *pnWidth, *pnHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic32);
      }
	    else
		    qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, *pnWidth, *pnHeight,GL_RGBA, GL_UNSIGNED_BYTE, pic32);
    }
    else
    {
	    if (nomips)
		    qglTexImage2D(GL_TEXTURE_2D, 0, 3, *pnWidth, *pnHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic32);
	    else
		    qgluBuild2DMipmaps(GL_TEXTURE_2D, 3, *pnWidth, *pnHeight,GL_RGBA, GL_UNSIGNED_BYTE, pic32);
    }
	  free (pic32);
	  qglBindTexture( GL_TEXTURE_2D, 0 );
  }

  SkinInfo *pInfo = new SkinInfo(pName, nTex);
  g_lstSkinCache.Add(pInfo);

  return nTex;
}


