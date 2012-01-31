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
// qertypes.h
//
// common types
// merged from brush.h, etc. for plugin support
//
#ifndef _QERTYPE_H
#define _QERTYPE_H

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef boolean qboolean;
//typedef unsigned char byte;
#endif

#define	MAXPOINTS	16

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];

#include "splines/math_vector.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

class texdef_t
{
public:
  texdef_t()
  {
    name = new char[1];
    name[0] = '\0';
  }
  ~texdef_t()
  {
    delete []name;
    name = NULL;
  }

  const char *Name( void )
  {
	  if ( name ) {
		  return name;
	  }

	  return "";
  }

  void SetName(const char *p)
  {
    if (name)
    {
      delete []name;
    }
    if (p)
    {
      name = strcpy(new char[strlen(p)+1], p);
    }
    else
    {
      name = new char[1];
      name[0] = '\0';
    }
  }

  texdef_t& operator =(const texdef_t& rhs)
  {
    if (&rhs != this)
    {
      SetName(rhs.name);
      shift[0] = rhs.shift[0];
      shift[1] = rhs.shift[1];
      rotate = rhs.rotate;
      scale[0] = rhs.scale[0];
      scale[1] = rhs.scale[1];
      contents = rhs.contents;
      flags = rhs.flags;
      value = rhs.value;
    }
    return *this;
  }
	//char	name[128];
	char	*name;
	float	shift[2];
	float	rotate;
	float	scale[2];
	int		contents;
	int		flags;
	int		value;
};

// Timo
// new brush primitive texdef
typedef struct brushprimit_texdef_s
{
	vec_t	coords[2][3];
} brushprimit_texdef_t;

class texturewin_t
{
public:
  texturewin_t()
  {
  }
  ~texturewin_t()
  {
  }
	int			width, height;
	int			originy;
	// add brushprimit_texdef_t for brush primitive coordinates storage
	brushprimit_texdef_t	brushprimit_texdef;
	int m_nTotalHeight;
	// surface plugin, must be casted to a IPluginTexdef*
	void* pTexdef;
	texdef_t	texdef;
};

#define QER_TRANS     0x00000001
#define QER_NOCARVE   0x00000002

typedef struct qtexture_s
{
	struct	qtexture_s *next;
	char	name[64];		// includes partial directory and extension
  int		width,  height;
	int		contents;
	int		flags;
	int		value;
	int		texture_number;	// gl bind number
  
	// name of the .shader file
  char  shadername[1024]; // old shader stuff
  qboolean bFromShader;   // created from a shader
  float fTrans;           // amount of transparency
  int   nShaderFlags;     // qer_ shader flags
	vec3_t	color;			    // for flat shade mode
	qboolean	inuse;		    // true = is present on the level

	// cast this one to an IPluginQTexture if you are using it
	// NOTE: casting can be done with a GETPLUGINQTEXTURE defined in isurfaceplugin.h
	// TODO: if the __ISURFACEPLUGIN_H_ header is used, use a union { void *pData; IPluginQTexture *pPluginQTexture } kind of thing ?
	void					*pData;

	//++timo FIXME: this is the actual filename of the texture
	// this will be removed after shader code cleanup
	char filename[64];

} qtexture_t;

// NOTE: don't trust this definition!
// you should read float points[..][5]
// see NewWinding definition
#define MAX_POINTS_ON_WINDING 64
typedef struct
{
	int		numpoints;
	int		maxpoints;
	float 	points[8][5];			// variable sized
} winding_t;

typedef struct
{
    vec3_t	normal;
    double	dist;
    int		type;
} plane_t;

//++timo texdef and brushprimit_texdef are static
// TODO : do dynamic ?
typedef struct face_s
{
	struct face_s			*next;
	struct face_s			*original;		//used for vertex movement
	vec3_t					planepts[3];
	texdef_t				texdef;
	plane_t					plane;

	winding_t				*face_winding;

	vec3_t					d_color;
	qtexture_t				*d_texture;

	// Timo new brush primit texdef
	brushprimit_texdef_t	brushprimit_texdef;

	// cast this one to an IPluginTexdef if you are using it
	// NOTE: casting can be done with a GETPLUGINTEXDEF defined in isurfaceplugin.h
	// TODO: if the __ISURFACEPLUGIN_H_ header is used, use a union { void *pData; IPluginTexdef *pPluginTexdef } kind of thing ?
	void					*pData;
} face_t;

typedef struct {
	vec3_t	xyz;
	float	sideST[2];
	float	capST[2];
} curveVertex_t;

typedef struct {
	curveVertex_t	v[2];
} sideVertex_t;


#define	MIN_PATCH_WIDTH		3
#define	MIN_PATCH_HEIGHT 	3

#define	MAX_PATCH_WIDTH		16
#define	MAX_PATCH_HEIGHT	16

// patch type info
// type in lower 16 bits, flags in upper
// endcaps directly follow this patch in the list

// types
#define PATCH_GENERIC     0x00000000    // generic flat patch
#define PATCH_CYLINDER    0x00000001    // cylinder
#define PATCH_BEVEL       0x00000002    // bevel
#define PATCH_ENDCAP      0x00000004    // endcap
#define PATCH_HEMISPHERE  0x00000008    // hemisphere
#define PATCH_CONE        0x00000010    // cone
#define PATCH_TRIANGLE    0x00000020    // simple tri, assumes 3x3 patch

// behaviour styles
#define PATCH_CAP         0x00001000    // flat patch applied as a cap
#define PATCH_SEAM        0x00002000    // flat patch applied as a seam
#define PATCH_THICK       0x00004000    // patch applied as a thick portion

// styles
#define PATCH_BEZIER      0x00000000    // default bezier
#define PATCH_BSPLINE     0x10000000    // bspline

#define PATCH_TYPEMASK     0x00000fff    // 
#define PATCH_BTYPEMASK    0x0000f000    // 
#define PATCH_STYLEMASK    0xffff0000    // 

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
} drawVert_t;

// used in brush primitive AND entities
typedef struct epair_s
{
	struct epair_s	*next;
	char	*key;
	char	*value;
} epair_t;

struct brush_s;
typedef struct brush_s brush_t;

typedef struct {
  int	width, height;		// in control points, not patches
  int   contents, flags, value, type;
  qtexture_t *d_texture;
  drawVert_t ctrl[MAX_PATCH_WIDTH][MAX_PATCH_HEIGHT];
  brush_t *pSymbiot;
  qboolean bSelected;
  qboolean bOverlay;
  qboolean bDirty;
  int  nListID;
	epair_t *epairs;
  // cast this one to an IPluginTexdef if you are using it
  // NOTE: casting can be done with a GETPLUGINTEXDEF defined in isurfaceplugin.h
  // TODO: if the __ISURFACEPLUGIN_H_ header is used, use a union { void *pData; IPluginTexdef *pPluginTexdef } kind of thing ?
  void					*pData;
} patchMesh_t;

typedef struct {
	int				index;
	qtexture_t		*texture;
	texdef_t		texdef;
} terrainFace_t;

typedef struct {
	float			height;
	float			scale;
	terrainFace_t	tri;
	vec4_t			rgba;
	vec3_t			normal;
	vec3_t			xyz;
} terrainVert_t;

#define MAX_TERRAIN_TEXTURES 128
typedef struct {
	int				width, height;

	vec3_t			mins, maxs;
	vec3_t			origin;
	float			scale_x;
	float			scale_y;

	int				numtextures;
	qtexture_t		*textures[ MAX_TERRAIN_TEXTURES ];

	terrainVert_t	*heightmap;       // width * height

	epair_t			*epairs;

	brush_s			*pSymbiot;
	bool			bSelected;
	bool			bDirty;
	int				nListID;
} terrainMesh_t;

typedef struct brush_s
{
	struct brush_s	*prev, *next;	// links in active/selected
	struct brush_s	*oprev, *onext;	// links in entity
	struct entity_s	*owner;
	vec3_t	mins, maxs;
	face_t     *brush_faces;

	qboolean bModelFailed;
	//
	// curve brush extensions
	// all are derived from brush_faces
	qboolean	patchBrush;
	qboolean	hiddenBrush;
	qboolean	terrainBrush;
  
  //int nPatchID;

	patchMesh_t *pPatch;
	terrainMesh_t	*pTerrain;

	struct entity_s *pUndoOwner;

	int undoId;						//undo ID
	int redoId;						//redo ID
	int ownerId;					//entityId of the owner entity for undo

	// TTimo: HTREEITEM is MFC, some plugins really don't like it
#ifdef QERTYPES_USE_MFC
	int numberId;         // brush number
	HTREEITEM itemOwner;  // owner for grouping
#else
	int numberId;
	DWORD itemOwner;
#endif

	// brush primitive only
	epair_t *epairs;

} brush_t;


#define	MAX_FLAGS	8


typedef struct trimodel_t
{
  vec3_t v[3];
  float  st[3][2];
} trimodel;

typedef struct entitymodel_t
{
  struct entitymodel_t *pNext;
  int nTriCount;
  trimodel *pTriList;
  int nTextureBind;
  int nSkinWidth;
  int nSkinHeight;
  int	nModelPosition;
} entitymodel;


// eclass show flags

#define     ECLASS_LIGHT      0x00000001
#define     ECLASS_ANGLE      0x00000002
#define     ECLASS_PATH       0x00000004
#define     ECLASS_MISCMODEL  0x00000008
#define		ECLASS_PLUGINENTITY 0x00000010

typedef struct eclass_s
{
	struct eclass_s *next;
	char	*name;
	qboolean	fixedsize;
	qboolean	unknown;		// wasn't found in source
	vec3_t	mins, maxs;
	vec3_t	color;
	texdef_t	texdef;
	char	*comments;
	char	flagnames[MAX_FLAGS][32];

/*
  int nTriCount;
  trimodel *pTriList;
  int nTextureBind;
  int nSkinWidth, nSkinHeight;
*/
  entitymodel *model;
  char	*modelpath;
  char	*skinpath;
  int   nFrame;
  unsigned int nShowFlags;

  HMODULE	hPlug;
} eclass_t;

extern	eclass_t	*eclass;

/*
** window bits
*/
#define	W_CAMERA		  0x0001
#define	W_XY			    0x0002
#define	W_XY_OVERLAY	0x0004
#define	W_Z				    0x0008
#define	W_TEXTURE		  0x0010
#define	W_Z_OVERLAY		0x0020
#define W_CONSOLE		  0x0040
#define W_ENTITY		  0x0080
#define W_CAMERA_IFON 0x0100
#define W_XZ          0x0200  //--| only used for patch vertex manip stuff
#define W_YZ          0x0400  //--|
#define W_GROUP       0x0800 
#define W_MEDIA       0x1000 
#define	W_ALL			0xFFFFFFFF

// used in some Drawing routines
enum VIEWTYPE {YZ, XZ, XY};

enum terrainnoise_t { NOISE_NONE, NOISE_PLUS, NOISE_PLUSMINUS };
enum terrainbrush_t { TERRAIN_BRUSH_CIRCLE, TERRAIN_BRUSH_SQUARE };
enum terrainfalloff_t { TERRAIN_FALLOFF_LINEAR, TERRAIN_FALLOFF_CURVED };

#endif