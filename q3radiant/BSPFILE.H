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
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS		1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	2048
#define	MAX_MAP_PATHS		2048
#define	MAX_MAP_ENTSTRING	0x20000
#define	MAX_MAP_TEXTURES	1024
#define	MAX_MAP_TEXINFO		8192

#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_MIPTEX		0x200000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x100000

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

#define BSPVERSION	34

typedef struct
{
	int		fileofs, filelen;
} lump_t;

#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_TEXTURES		2
#define	LUMP_VERTEXES		3
#define	LUMP_VISIBILITY		4
#define	LUMP_NODES			5
#define	LUMP_TEXINFO		6
#define	LUMP_FACES			7
#define	LUMP_LIGHTING		8
#define	LUMP_LEAFS			9
#define	LUMP_LEAFFACES		10
#define	LUMP_LEAFBRUSHES	11
#define	LUMP_EDGES			12
#define	LUMP_SURFEDGES		13
#define	LUMP_MODELS			14
#define	LUMP_PATHS			15
#define	LUMP_BRUSHES		16
#define	LUMP_BRUSHSIDES		17
#define	LUMP_POP			18

#define	HEADER_LUMPS	18

typedef struct
{
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} dmodel_t;

typedef struct
{
	int			nummiptex;
	int			dataofs[4];		// [nummiptex]
} dmiptexlump_t;

#define	MIPLEVELS	4
typedef struct miptex_s
{
	char		name[16];
	unsigned	width, height;
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
	int			flags;
	int			value;
} miptex_t;


typedef struct
{
	float	point[3];
} dvertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are allways opposites

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_THINWATER		64		// translucent faces

#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_MONSTER		128
#define	CONTENTS_PLAYERCLIP		256
#define	CONTENTS_MONSTERCLIP	512


// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		1024
#define	CONTENTS_CURRENT_90		2048
#define	CONTENTS_CURRENT_180	4096
#define	CONTENTS_CURRENT_270	8192
#define	CONTENTS_CURRENT_UP		16384
#define	CONTENTS_CURRENT_DOWN	32768

#define	CONTENTS_ORIGIN			65536		// removed before processing



// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dnode_t;

typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			miptex;
	int			flags;			// miptex flags + overrides
	int			value;			// light emition, etc
} texinfo_t;

#define	TEX_SPECIAL		1		// sky or slime, no lightmap or 256 subdivision
#define	SURF_LIGHT		2

#define	SURF_WATER		4
#define	SURF_SLIME		8
#define	SURF_LAVA		16
#define	SURF_WINDOW		32

#define	SURF_SKY		64
#define	SURF_MIRROR		128

#define	SURF_SLIPPERY	256

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct
{
	unsigned short	planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dface_t;

typedef struct
{
	int			contents;			// OR of all brushes
	int			visofs;				// -1 = no visibility info

	short		mins[3];			// for frustum culling
	short		maxs[3];

	unsigned short		firstleafface;
	unsigned short		numleaffaces;

	unsigned short		firstleafbrush;
	unsigned short		numleafbrushes;
} dleaf_t;


typedef struct
{
	unsigned short	planenum;		// facing out of the leaf
	short	texinfo;
} dbrushside_t;

typedef struct
{
	int			firstside;
	int			numsides;
	int			contents;
} dbrush_t;

typedef struct
{
	float		origin[3];
	float		angles[3];
	int			next, prev;
	int			flags;
	float		speed;
} dpath_t;

//============================================================================

#ifndef QUAKE_GAME

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the utilities get to be lazy and just use large static arrays

extern	int			nummodels;
extern	dmodel_t	dmodels[MAX_MAP_MODELS];

extern	int			visdatasize;
extern	byte		dvisdata[MAX_MAP_VISIBILITY];

extern	int			lightdatasize;
extern	byte		dlightdata[MAX_MAP_LIGHTING];

extern	int			texdatasize;
extern	byte		dtexdata[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

extern	int			entdatasize;
extern	char		dentdata[MAX_MAP_ENTSTRING];

extern	int			numleafs;
extern	dleaf_t		dleafs[MAX_MAP_LEAFS];

extern	int			numplanes;
extern	dplane_t	dplanes[MAX_MAP_PLANES];

extern	int			numvertexes;
extern	dvertex_t	dvertexes[MAX_MAP_VERTS];

extern	int			numnodes;
extern	dnode_t		dnodes[MAX_MAP_NODES];

extern	int			numtexinfo;
extern	texinfo_t	texinfo[MAX_MAP_TEXINFO];

extern	int			numfaces;
extern	dface_t		dfaces[MAX_MAP_FACES];

extern	int			numedges;
extern	dedge_t		dedges[MAX_MAP_EDGES];

extern	int			numleaffaces;
extern	unsigned short	dleaffaces[MAX_MAP_LEAFFACES];

extern	int			numleafbrushes;
extern	unsigned short	dleafbrushes[MAX_MAP_LEAFBRUSHES];

extern	int			numsurfedges;
extern	int			dsurfedges[MAX_MAP_SURFEDGES];

extern	int			numpaths;
extern	dpath_t		dpaths[MAX_MAP_PATHS];

extern	int			numbrushes;
extern	dbrush_t	dbrushes[MAX_MAP_BRUSHES];

extern	int			numbrushsides;
extern	dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];


void DecompressVis (byte *in, byte *decompressed);
int CompressVis (byte *vis, byte *dest);

void	LoadBSPFile (char *filename);
void	WriteBSPFile (char *filename);
void	PrintBSPFileSizes (void);

//===============


typedef struct epair_s
{
	struct epair_s	*next;
	char	*key;
	char	*value;
} epair_t;

typedef struct
{
	vec3_t		origin;
	int			firstbrush;
	int			numbrushes;
	epair_t		*epairs;
} entity_t;

extern	int			num_entities;
extern	entity_t	entities[MAX_MAP_ENTITIES];

void	ParseEntities (void);
void	UnparseEntities (void);

void 	SetKeyValue (entity_t *ent, char *key, char *value);
char 	*ValueForKey (entity_t *ent, char *key);
// will return "" if not present

vec_t	FloatForKey (entity_t *ent, char *key);
void 	GetVectorForKey (entity_t *ent, char *key, vec3_t vec);

epair_t *ParseEpair (void);

void PrintEntity (entity_t *ent);

extern	int		r_leaftovis[MAX_MAP_LEAFS];
extern	int		r_vistoleaf[MAX_MAP_LEAFS];
extern	int		r_numvisleafs;

#endif
