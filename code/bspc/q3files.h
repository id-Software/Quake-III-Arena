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
#ifndef __QFILES_H__
#define __QFILES_H__

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)


// the maximum size of game reletive pathnames
#define	MAX_QPATH		64


/*
========================================================================

PCX files are used for 8 bit images

========================================================================
* 

typedef struct {
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    unsigned char	data;			// unbounded
} pcx_t;


/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
* 

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


*/

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		4
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame

// vertex scales
#define	MD3_XYZ_SCALE		(1.0/64)

typedef struct md3Frame_s {
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s {
	char		name[MAX_QPATH];	// tag name
	vec3_t		origin;
	vec3_t		axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/

typedef struct {
	int		ident;				// 

	char	name[MAX_QPATH];	// polyset name

	int		flags;
	int		numFrames;			// all surfaces in a model should have the same

	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;

	int		numTriangles;
	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct {
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct {
	int			indexes[3];
} md3Triangle_t;

typedef struct {
	float		st[2];
} md3St_t;

typedef struct {
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;			
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;



/*
==============================================================================

  .BSP file format

==============================================================================
*/


#define Q3_BSP_IDENT	(('P'<<24)+('S'<<16)+('B'<<8)+'I')
		// little-endian "IBSP"

#define Q3_BSP_VERSION			46


// there shouldn't be any problem with increasing these values at the
// expense of more memory allocation in the utilities
#define	Q3_MAX_MAP_MODELS		0x400
#define	Q3_MAX_MAP_BRUSHES		0x8000
#define	Q3_MAX_MAP_ENTITIES	0x800
#define	Q3_MAX_MAP_ENTSTRING	0x10000
#define	Q3_MAX_MAP_SHADERS		0x400

#define	Q3_MAX_MAP_AREAS		0x100	// MAX_MAP_AREA_BYTES in q_shared must match!
#define	Q3_MAX_MAP_FOGS		0x100
#define	Q3_MAX_MAP_PLANES		0x10000
#define	Q3_MAX_MAP_NODES		0x10000
#define	Q3_MAX_MAP_BRUSHSIDES	0x10000
#define	Q3_MAX_MAP_LEAFS		0x10000
#define	Q3_MAX_MAP_LEAFFACES	0x10000
#define	Q3_MAX_MAP_LEAFBRUSHES	0x10000
#define	Q3_MAX_MAP_PORTALS		0x10000
#define	Q3_MAX_MAP_LIGHTING	0x400000
#define	Q3_MAX_MAP_LIGHTGRID	0x400000
#define	Q3_MAX_MAP_VISIBILITY	0x200000

#define	Q3_MAX_MAP_DRAW_SURFS	0x20000
#define	Q3_MAX_MAP_DRAW_VERTS	0x80000
#define	Q3_MAX_MAP_DRAW_INDEXES	0x80000


// key / value pair sizes in the entities lump
#define	Q3_MAX_KEY				32
#define	Q3_MAX_VALUE			1024

// the editor uses these predefined yaw angles to orient entities up or down
#define	ANGLE_UP			-1
#define	ANGLE_DOWN			-2

#define	LIGHTMAP_WIDTH		128
#define	LIGHTMAP_HEIGHT		128


//=============================================================================


typedef struct {
	int		fileofs, filelen;
} q3_lump_t;

#define	Q3_LUMP_ENTITIES		0
#define	Q3_LUMP_SHADERS		1
#define	Q3_LUMP_PLANES			2
#define	Q3_LUMP_NODES			3
#define	Q3_LUMP_LEAFS			4
#define	Q3_LUMP_LEAFSURFACES	5
#define	Q3_LUMP_LEAFBRUSHES	6
#define	Q3_LUMP_MODELS			7
#define	Q3_LUMP_BRUSHES		8
#define	Q3_LUMP_BRUSHSIDES		9
#define	Q3_LUMP_DRAWVERTS		10
#define	Q3_LUMP_DRAWINDEXES	11
#define	Q3_LUMP_FOGS			12
#define	Q3_LUMP_SURFACES		13
#define	Q3_LUMP_LIGHTMAPS		14
#define	Q3_LUMP_LIGHTGRID		15
#define	Q3_LUMP_VISIBILITY		16
#define	Q3_HEADER_LUMPS		17

typedef struct {
	int			ident;
	int			version;

	q3_lump_t		lumps[Q3_HEADER_LUMPS];
} q3_dheader_t;

typedef struct {
	float		mins[3], maxs[3];
	int			firstSurface, numSurfaces;
	int			firstBrush, numBrushes;
} q3_dmodel_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			surfaceFlags;
	int			contentFlags;
} q3_dshader_t;

// planes (x&~1) and (x&~1)+1 are allways opposites

typedef struct {
	float		normal[3];
	float		dist;
} q3_dplane_t;

typedef struct {
	int			planeNum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	int			mins[3];		// for frustom culling
	int			maxs[3];
} q3_dnode_t;

typedef struct {
	int			cluster;			// -1 = opaque cluster (do I still store these?)
	int			area;

	int			mins[3];			// for frustum culling
	int			maxs[3];

	int			firstLeafSurface;
	int			numLeafSurfaces;

	int			firstLeafBrush;
	int			numLeafBrushes;
} q3_dleaf_t;

typedef struct {
	int			planeNum;			// positive plane side faces out of the leaf
	int			shaderNum;
} q3_dbrushside_t;

typedef struct {
	int			firstSide;
	int			numSides;
	int			shaderNum;		// the shader that determines the contents flags
} q3_dbrush_t;

typedef struct {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} q3_dfog_t;

typedef struct {
	vec3_t		xyz;
	float		st[2];
	float		lightmap[2];
	vec3_t		normal;
	byte		color[4];
} q3_drawVert_t;

typedef enum {
	MST_BAD,
	MST_PLANAR,
	MST_PATCH,
	MST_TRIANGLE_SOUP,
	MST_FLARE
} q3_mapSurfaceType_t;

typedef struct {
	int			shaderNum;
	int			fogNum;
	int			surfaceType;

	int			firstVert;
	int			numVerts;

	int			firstIndex;
	int			numIndexes;

	int			lightmapNum;
	int			lightmapX, lightmapY;
	int			lightmapWidth, lightmapHeight;

	vec3_t		lightmapOrigin;
	vec3_t		lightmapVecs[3];	// for patches, [0] and [1] are lodbounds

	int			patchWidth;
	int			patchHeight;
} q3_dsurface_t;


#endif
