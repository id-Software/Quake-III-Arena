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

#define	Q1_MAX_MAP_HULLS		4

#define	Q1_MAX_MAP_MODELS		256
#define	Q1_MAX_MAP_BRUSHES		4096
#define	Q1_MAX_MAP_ENTITIES	1024
#define	Q1_MAX_MAP_ENTSTRING	65536

#define	Q1_MAX_MAP_PLANES		8192
#define	Q1_MAX_MAP_NODES		32767		// because negative shorts are contents
#define	Q1_MAX_MAP_CLIPNODES	32767		//
#define	Q1_MAX_MAP_LEAFS		32767		// 
#define	Q1_MAX_MAP_VERTS		65535
#define	Q1_MAX_MAP_FACES		65535
#define	Q1_MAX_MAP_MARKSURFACES 65535
#define	Q1_MAX_MAP_TEXINFO		4096
#define	Q1_MAX_MAP_EDGES		256000
#define	Q1_MAX_MAP_SURFEDGES	512000
#define	Q1_MAX_MAP_MIPTEX		0x200000
#define	Q1_MAX_MAP_LIGHTING	0x100000
#define	Q1_MAX_MAP_VISIBILITY	0x100000

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================


#define Q1_BSPVERSION	29

typedef struct
{
	int fileofs, filelen;
} q1_lump_t;

#define	Q1_LUMP_ENTITIES	0
#define	Q1_LUMP_PLANES		1
#define	Q1_LUMP_TEXTURES	2
#define	Q1_LUMP_VERTEXES	3
#define	Q1_LUMP_VISIBILITY	4
#define	Q1_LUMP_NODES		5
#define	Q1_LUMP_TEXINFO	6
#define	Q1_LUMP_FACES		7
#define	Q1_LUMP_LIGHTING	8
#define	Q1_LUMP_CLIPNODES	9
#define	Q1_LUMP_LEAFS		10
#define	Q1_LUMP_MARKSURFACES 11
#define	Q1_LUMP_EDGES		12
#define	Q1_LUMP_SURFEDGES	13
#define	Q1_LUMP_MODELS		14

#define	Q1_HEADER_LUMPS	15

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int		headnode[Q1_MAX_MAP_HULLS];
	int		visleafs;		// not including the solid leaf 0
	int		firstface, numfaces;
} q1_dmodel_t;

typedef struct
{
	int			version;	
	q1_lump_t	lumps[Q1_HEADER_LUMPS];
} q1_dheader_t;

typedef struct
{
	int		nummiptex;
	int		dataofs[4];		// [nummiptex]
} q1_dmiptexlump_t;

#define	MIPLEVELS	4
typedef struct q1_miptex_s
{
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} q1_miptex_t;


typedef struct
{
	float	point[3];
} q1_dvertex_t;


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
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} q1_dplane_t;



#define	Q1_CONTENTS_EMPTY		-1
#define	Q1_CONTENTS_SOLID		-2
#define	Q1_CONTENTS_WATER		-3
#define	Q1_CONTENTS_SLIME		-4
#define	Q1_CONTENTS_LAVA		-5
#define	Q1_CONTENTS_SKY		-6

// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	int		planenum;
	short		children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for sphere culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} q1_dnode_t;

typedef struct
{
	int		planenum;
	short		children[2];	// negative numbers are contents
} q1_dclipnode_t;


typedef struct q1_texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int		miptex;
	int		flags;
} q1_texinfo_t;
#define	TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} q1_dedge_t;

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
} q1_dface_t;



#define	AMBIENT_WATER	0
#define	AMBIENT_SKY		1
#define	AMBIENT_SLIME	2
#define	AMBIENT_LAVA	3

#define	NUM_AMBIENTS			4		// automatic ambient sounds

// leaf 0 is the generic Q1_CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int				contents;
	int				visofs;				// -1 = no visibility info

	short				mins[3];			// for frustum culling
	short				maxs[3];

	unsigned short	firstmarksurface;
	unsigned short	nummarksurfaces;

	byte		ambient_level[NUM_AMBIENTS];
} q1_dleaf_t;

//============================================================================

#ifndef QUAKE_GAME

// the utilities get to be lazy and just use large static arrays

extern	int				q1_nummodels;
extern	q1_dmodel_t		*q1_dmodels;//[MAX_MAP_MODELS];

extern	int				q1_visdatasize;
extern	byte				*q1_dvisdata;//[MAX_MAP_VISIBILITY];

extern	int				q1_lightdatasize;
extern	byte				*q1_dlightdata;//[MAX_MAP_LIGHTING];

extern	int				q1_texdatasize;
extern	byte				*q1_dtexdata;//[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

extern	int				q1_entdatasize;
extern	char				*q1_dentdata;//[MAX_MAP_ENTSTRING];

extern	int				q1_numleafs;
extern	q1_dleaf_t		*q1_dleafs;//[MAX_MAP_LEAFS];

extern	int				q1_numplanes;
extern	q1_dplane_t		*q1_dplanes;//[MAX_MAP_PLANES];

extern	int				q1_numvertexes;
extern	q1_dvertex_t	*q1_dvertexes;//[MAX_MAP_VERTS];

extern	int				q1_numnodes;
extern	q1_dnode_t		*q1_dnodes;//[MAX_MAP_NODES];

extern	int				q1_numtexinfo;
extern	q1_texinfo_t	*q1_texinfo;//[MAX_MAP_TEXINFO];

extern	int				q1_numfaces;
extern	q1_dface_t		*q1_dfaces;//[MAX_MAP_FACES];

extern	int				q1_numclipnodes;
extern	q1_dclipnode_t	*q1_dclipnodes;//[MAX_MAP_CLIPNODES];

extern	int				q1_numedges;
extern	q1_dedge_t		*q1_dedges;//[MAX_MAP_EDGES];

extern	int				q1_nummarksurfaces;
extern	unsigned short	*q1_dmarksurfaces;//[MAX_MAP_MARKSURFACES];

extern	int				q1_numsurfedges;
extern	int				*q1_dsurfedges;//[MAX_MAP_SURFEDGES];


void Q1_AllocMaxBSP(void);
void Q1_FreeMaxBSP(void);
void Q1_LoadBSPFile(char *filename, int offset, int length);
void Q1_WriteBSPFile(char *filename);
void Q1_PrintBSPFileSizes(void);
void Q1_ParseEntities(void);
void Q1_UnparseEntities(void);

#endif
