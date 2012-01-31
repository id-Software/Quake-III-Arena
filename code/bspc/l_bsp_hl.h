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
// upper design bounds

#define	HL_MAX_MAP_HULLS			4

#define	HL_MAX_MAP_MODELS			400
#define	HL_MAX_MAP_BRUSHES		4096
#define	HL_MAX_MAP_ENTITIES		1024
#define	HL_MAX_MAP_ENTSTRING		(128*1024)

#define	HL_MAX_MAP_PLANES			32767
#define	HL_MAX_MAP_NODES			32767		// because negative shorts are contents
#define	HL_MAX_MAP_CLIPNODES		32767		//
#define	HL_MAX_MAP_LEAFS			8192
#define	HL_MAX_MAP_VERTS			65535
#define	HL_MAX_MAP_FACES			65535
#define	HL_MAX_MAP_MARKSURFACES 65535
#define	HL_MAX_MAP_TEXINFO		8192
#define	HL_MAX_MAP_EDGES			256000
#define	HL_MAX_MAP_SURFEDGES		512000
#define	HL_MAX_MAP_TEXTURES		512
#define	HL_MAX_MAP_MIPTEX			0x200000
#define	HL_MAX_MAP_LIGHTING		0x200000
#define	HL_MAX_MAP_VISIBILITY	0x200000

#define	HL_MAX_MAP_PORTALS		65536

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================


#define HL_BSPVERSION	30
#define HL_TOOLVERSION	2


typedef struct
{
	int		fileofs, filelen;
} hl_lump_t;

#define	HL_LUMP_ENTITIES	0
#define	HL_LUMP_PLANES		1
#define	HL_LUMP_TEXTURES	2
#define	HL_LUMP_VERTEXES	3
#define	HL_LUMP_VISIBILITY	4
#define	HL_LUMP_NODES		5
#define	HL_LUMP_TEXINFO	6
#define	HL_LUMP_FACES		7
#define	HL_LUMP_LIGHTING	8
#define	HL_LUMP_CLIPNODES	9
#define	HL_LUMP_LEAFS		10
#define	HL_LUMP_MARKSURFACES 11
#define	HL_LUMP_EDGES		12
#define	HL_LUMP_SURFEDGES	13
#define	HL_LUMP_MODELS		14

#define	HL_HEADER_LUMPS	15

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int		headnode[HL_MAX_MAP_HULLS];
	int		visleafs;		// not including the solid leaf 0
	int		firstface, numfaces;
} hl_dmodel_t;

typedef struct
{
	int			version;	
	hl_lump_t	lumps[HL_HEADER_LUMPS];
} hl_dheader_t;

typedef struct
{
	int			nummiptex;
	int			dataofs[4];		// [nummiptex]
} hl_dmiptexlump_t;

#define	MIPLEVELS	4
typedef struct hl_miptex_s
{
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} hl_miptex_t;


typedef struct
{
	float	point[3];
} hl_dvertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

typedef struct
{
	float	normal[3];
	float	dist;
	int	type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} hl_dplane_t;



#define	HL_CONTENTS_EMPTY				-1
#define	HL_CONTENTS_SOLID				-2
#define	HL_CONTENTS_WATER				-3
#define	HL_CONTENTS_SLIME				-4
#define	HL_CONTENTS_LAVA				-5
#define	HL_CONTENTS_SKY				-6
#define	HL_CONTENTS_ORIGIN			-7		// removed at csg time
#define	HL_CONTENTS_CLIP				-8		// changed to contents_solid

#define	HL_CONTENTS_CURRENT_0		-9
#define	HL_CONTENTS_CURRENT_90		-10
#define	HL_CONTENTS_CURRENT_180		-11
#define	HL_CONTENTS_CURRENT_270		-12
#define	HL_CONTENTS_CURRENT_UP		-13
#define	HL_CONTENTS_CURRENT_DOWN	-14

#define HL_CONTENTS_TRANSLUCENT		-15

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	int		planenum;
	short		children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} hl_dnode_t;

typedef struct
{
	int		planenum;
	short		children[2];	// negative numbers are contents
} hl_dclipnode_t;


typedef struct hl_texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int		miptex;
	int		flags;
} hl_texinfo_t;
#define	TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} hl_dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct
{
	short		planenum;
	short		side;

	int		firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int		lightofs;		// start of [numstyles*surfsize] samples
} hl_dface_t;


#define	AMBIENT_WATER	0
#define	AMBIENT_SKY		1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS			4		// automatic ambient sounds

// leaf 0 is the generic HL_CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int			contents;
	int			visofs;				// -1 = no visibility info

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short		firstmarksurface;
	unsigned short		nummarksurfaces;

	byte		ambient_level[NUM_AMBIENTS];
} hl_dleaf_t;


//============================================================================

#ifndef QUAKE_GAME

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the utilities get to be lazy and just use large static arrays

extern	int				hl_nummodels;
extern	hl_dmodel_t		*hl_dmodels;//[MAX_MAP_MODELS];
extern	int				hl_dmodels_checksum;

extern	int				hl_visdatasize;
extern	byte				*hl_dvisdata;//[MAX_MAP_VISIBILITY];
extern	int				hl_dvisdata_checksum;

extern	int				hl_lightdatasize;
extern	byte				*hl_dlightdata;//[MAX_MAP_LIGHTING];
extern	int				hl_dlightdata_checksum;

extern	int				hl_texdatasize;
extern	byte				*hl_dtexdata;//[MAX_MAP_MIPTEX]; // (dmiptexlump_t)
extern	int				hl_dtexdata_checksum;

extern	int				hl_entdatasize;
extern	char				*hl_dentdata;//[MAX_MAP_ENTSTRING];
extern	int				hl_dentdata_checksum;

extern	int				hl_numleafs;
extern	hl_dleaf_t		*hl_dleafs;//[MAX_MAP_LEAFS];
extern	int				hl_dleafs_checksum;

extern	int				hl_numplanes;
extern	hl_dplane_t		*hl_dplanes;//[MAX_MAP_PLANES];
extern	int				hl_dplanes_checksum;

extern	int				hl_numvertexes;
extern	hl_dvertex_t	*hl_dvertexes;//[MAX_MAP_VERTS];
extern	int				hl_dvertexes_checksum;

extern	int				hl_numnodes;
extern	hl_dnode_t		*hl_dnodes;//[MAX_MAP_NODES];
extern	int				hl_dnodes_checksum;

extern	int				hl_numtexinfo;
extern	hl_texinfo_t	*hl_texinfo;//[MAX_MAP_TEXINFO];
extern	int				hl_texinfo_checksum;

extern	int				hl_numfaces;
extern	hl_dface_t		*hl_dfaces;//[MAX_MAP_FACES];
extern	int				hl_dfaces_checksum;

extern	int				hl_numclipnodes;
extern	hl_dclipnode_t	*hl_dclipnodes;//[MAX_MAP_CLIPNODES];
extern	int				hl_dclipnodes_checksum;

extern	int				hl_numedges;
extern	hl_dedge_t		*hl_dedges;//[MAX_MAP_EDGES];
extern	int				hl_dedges_checksum;

extern	int				hl_nummarksurfaces;
extern	unsigned short	*hl_dmarksurfaces;//[MAX_MAP_MARKSURFACES];
extern	int				hl_dmarksurfaces_checksum;

extern	int				hl_numsurfedges;
extern	int				*hl_dsurfedges;//[MAX_MAP_SURFEDGES];
extern	int				hl_dsurfedges_checksum;

int FastChecksum(void *buffer, int bytes);

void HL_AllocMaxBSP(void);
void HL_FreeMaxBSP(void);

void HL_DecompressVis(byte *in, byte *decompressed);
int HL_CompressVis(byte *vis, byte *dest);

void HL_LoadBSPFile(char *filename, int offset, int length);
void HL_WriteBSPFile(char *filename);
void HL_PrintBSPFileSizes(void);
void HL_PrintBSPFileSizes(void);
void HL_ParseEntities(void);
void HL_UnparseEntities(void);

#endif
