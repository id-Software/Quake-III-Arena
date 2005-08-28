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

/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define SIN

#define SINBSPVERSION	41

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	SIN_MAX_MAP_MODELS		1024
#define	SIN_MAX_MAP_BRUSHES		8192
#define	SIN_MAX_MAP_ENTITIES	2048
#define	SIN_MAX_MAP_ENTSTRING	0x40000
#define	SIN_MAX_MAP_TEXINFO		8192

#define	SIN_MAX_MAP_AREAS		256
#define	SIN_MAX_MAP_AREAPORTALS	1024
#define	SIN_MAX_MAP_PLANES		65536
#define	SIN_MAX_MAP_NODES		65536
#define	SIN_MAX_MAP_BRUSHSIDES	65536
#define	SIN_MAX_MAP_LEAFS		65536
#define	SIN_MAX_MAP_VERTS		65536
#define	SIN_MAX_MAP_FACES		65536
#define	SIN_MAX_MAP_LEAFFACES	65536
#define	SIN_MAX_MAP_LEAFBRUSHES 65536
#define	SIN_MAX_MAP_PORTALS		65536
#define	SIN_MAX_MAP_EDGES		128000
#define	SIN_MAX_MAP_SURFEDGES	256000
#define	SIN_MAX_MAP_LIGHTING	0x320000
#define	SIN_MAX_MAP_VISIBILITY	0x280000

#ifdef SIN
#define	SIN_MAX_MAP_LIGHTINFO		8192
#endif

#ifdef SIN
#undef SIN_MAX_MAP_LIGHTING	//undef the Quake2 bsp version
#define	SIN_MAX_MAP_LIGHTING	0x300000
#endif

#ifdef SIN
#undef SIN_MAX_MAP_VISIBILITY	//undef the Quake2 bsp version
#define	SIN_MAX_MAP_VISIBILITY	0x280000
#endif

//=============================================================================

typedef struct
{
	int		fileofs, filelen;
} sin_lump_t;

#define	SIN_LUMP_ENTITIES		0
#define	SIN_LUMP_PLANES			1
#define	SIN_LUMP_VERTEXES		2
#define	SIN_LUMP_VISIBILITY		3
#define	SIN_LUMP_NODES			4
#define	SIN_LUMP_TEXINFO		5
#define	SIN_LUMP_FACES			6
#define	SIN_LUMP_LIGHTING		7
#define	SIN_LUMP_LEAFS			8
#define	SIN_LUMP_LEAFFACES		9
#define	SIN_LUMP_LEAFBRUSHES	10
#define	SIN_LUMP_EDGES			11
#define	SIN_LUMP_SURFEDGES		12
#define	SIN_LUMP_MODELS			13
#define	SIN_LUMP_BRUSHES		14
#define	SIN_LUMP_BRUSHSIDES		15
#define	SIN_LUMP_POP			16
#define	SIN_LUMP_AREAS			17
#define	SIN_LUMP_AREAPORTALS	18

#ifdef SIN
#define	SIN_LUMP_LIGHTINFO 	19
#define	SINHEADER_LUMPS		20
#endif

typedef struct
{
	int			ident;
	int			version;	
	sin_lump_t	lumps[SINHEADER_LUMPS];
} sin_dheader_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// submodels just draw faces
										// without walking the bsp tree
} sin_dmodel_t;

typedef struct
{
	float	point[3];
} sin_dvertex_t;


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
} sin_dplane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in q_shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#ifdef SIN
#define	CONTENTS_FENCE			4
#endif
// remaining contents are non-visible, and don't eat brushes

#ifdef SIN
#define	CONTENTS_DUMMYFENCE	0x1000
#endif

#ifdef SIN
#define	SURF_MASKED		0x2		// surface texture is masked
#endif

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp

#ifdef SIN
#define	SURF_NONLIT	   0x10	// surface is not lit
#define	SURF_NOFILTER  0x20	// surface is not bi-linear filtered
#endif

#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture

#define	SURF_HINT		0x100	// make a primary bsp splitter
#define	SURF_SKIP		0x200	// completely ignore, allowing non-closed brushes

#ifdef SIN
#define	SURF_CONVEYOR  0x40	// surface is not lit
#endif

#ifdef SIN
#define	SURF_WAVY            0x400       // surface has waves
#define	SURF_RICOCHET		   0x800	      // projectiles bounce literally bounce off this surface
#define	SURF_PRELIT		      0x1000	   // surface has intensity information for pre-lighting
#define	SURF_MIRROR		      0x2000	   // surface is a mirror
#define	SURF_CONSOLE         0x4000	   // surface is a console
#define	SURF_USECOLOR        0x8000	   // surface is lit with non-lit * color
#define	SURF_HARDWAREONLY    0x10000     // surface has been damaged
#define	SURF_DAMAGE          0x20000     // surface can be damaged
#define	SURF_WEAK            0x40000     // surface has weak hit points
#define	SURF_NORMAL          0x80000     // surface has normal hit points
#define	SURF_ADD             0x100000    // surface will be additive
#define	SURF_ENVMAPPED       0x200000    // surface is envmapped
#define	SURF_RANDOMANIMATE   0x400000    // surface start animating on a random frame
#define	SURF_ANIMATE         0x800000    // surface animates
#define	SURF_RNDTIME         0x1000000   // time between animations is random
#define	SURF_TRANSLATE       0x2000000   // surface translates
#define	SURF_NOMERGE         0x4000000   // surface is not merged in csg phase
#define  SURF_TYPE_BIT0       0x8000000   // 0 bit of surface type
#define  SURF_TYPE_BIT1       0x10000000  // 1 bit of surface type
#define  SURF_TYPE_BIT2       0x20000000  // 2 bit of surface type
#define  SURF_TYPE_BIT3       0x40000000  // 3 bit of surface type

#define SURF_START_BIT        27
#define SURFACETYPE_FROM_FLAGS( x ) ( ( x >> (SURF_START_BIT) ) & 0xf )


#define  SURF_TYPE_SHIFT(x)   ( (x) << (SURF_START_BIT) ) // macro for getting proper bit mask

#define  SURF_TYPE_NONE       SURF_TYPE_SHIFT(0)
#define  SURF_TYPE_WOOD       SURF_TYPE_SHIFT(1)
#define  SURF_TYPE_METAL      SURF_TYPE_SHIFT(2)
#define  SURF_TYPE_STONE      SURF_TYPE_SHIFT(3)
#define  SURF_TYPE_CONCRETE   SURF_TYPE_SHIFT(4)
#define  SURF_TYPE_DIRT       SURF_TYPE_SHIFT(5)
#define  SURF_TYPE_FLESH      SURF_TYPE_SHIFT(6)
#define  SURF_TYPE_GRILL      SURF_TYPE_SHIFT(7)
#define  SURF_TYPE_GLASS      SURF_TYPE_SHIFT(8)
#define  SURF_TYPE_FABRIC     SURF_TYPE_SHIFT(9)
#define  SURF_TYPE_MONITOR    SURF_TYPE_SHIFT(10)
#define  SURF_TYPE_GRAVEL     SURF_TYPE_SHIFT(11)
#define  SURF_TYPE_VEGETATION SURF_TYPE_SHIFT(12)
#define  SURF_TYPE_PAPER      SURF_TYPE_SHIFT(13)
#define  SURF_TYPE_DUCT       SURF_TYPE_SHIFT(14)
#define  SURF_TYPE_WATER      SURF_TYPE_SHIFT(15)
#endif


typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} sin_dnode_t;

#ifdef SIN

typedef struct sin_lightvalue_s
{
    int		value;			// light emission, etc
    vec3_t	color;
    float	direct;
    float	directangle;
    float	directstyle;
    char		directstylename[32];
} sin_lightvalue_t;

typedef struct sin_texinfo_s
{
    float		vecs[2][4];		// [s/t][xyz offset]
    int			flags;			// miptex flags + overrides
    char		   texture[64];	// texture name (textures/*.wal)
    int			nexttexinfo;	// for animations, -1 = end of chain
    float      trans_mag;
    int        trans_angle;
    int        base_angle;
    float      animtime;
    float      nonlit;
    float      translucence;
    float      friction;
    float      restitution;
    vec3_t     color;
    char       groupname[32];
} sin_texinfo_t;

#endif //SIN

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} sin_dedge_t;

#ifdef MAXLIGHTMAPS
#undef MAXLIGHTMAPS
#endif
#define	MAXLIGHTMAPS	16
typedef struct
{
	unsigned short	planenum;
	short		side;

	int		firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	byte		styles[MAXLIGHTMAPS];
	int		lightofs;		// start of [numstyles*surfsize] samples
#ifdef SIN
   int      lightinfo;
#endif
} sin_dface_t;

typedef struct
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
} sin_dleaf_t;

typedef struct
{
	unsigned short	planenum;		// facing out of the leaf
	short	texinfo;
#ifdef SIN
   int lightinfo;
#endif
} sin_dbrushside_t;

typedef struct
{
	int			firstside;
	int			numsides;
	int			contents;
} sin_dbrush_t;

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
typedef struct
{
	int			numclusters;
	int			bitofs[8][2];	// bitofs[numclusters][2]
} sin_dvis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct
{
	int		portalnum;
	int		otherarea;
} sin_dareaportal_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} sin_darea_t;
