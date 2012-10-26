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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"
#include "../renderer/tr_public.h"
#include "tr_extratypes.h"
#include "tr_extramath.h"
#include "tr_fbo.h"
#include "tr_postprocess.h"
#include "qgl.h"
#include "../renderer/iqm.h"

#define GL_INDEX_TYPE		GL_UNSIGNED_INT
typedef unsigned int glIndex_t;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

#ifndef ARRAY_SIZE
#	define ARRAY_SIZE(arr)                          (sizeof(arr) / sizeof(arr[0]))
#endif

// everything that is needed by the backend needs
// to be double buffered to allow it to run in
// parallel on a dual cpu machine
#define	SMP_FRAMES		2

// 14 bits
// can't be increased without changing bit packing for drawsurfs
// see QSORT_SHADERNUM_SHIFT
#define SHADERNUM_BITS	14
#define MAX_SHADERS		(1<<SHADERNUM_BITS)

//#define MAX_SHADER_STATES 2048
#define MAX_STATES_PER_SHADER 32
#define MAX_STATE_NAME 32

#define	MAX_FBOS      64
#define MAX_VISCOUNTS 5
#define MAX_VBOS      4096
#define MAX_IBOS      4096

#define MAX_CALC_PSHADOWS    64
#define MAX_DRAWN_PSHADOWS    16 // do not increase past 32, because bit flags are used on surfaces
#define PSHADOW_MAP_SIZE      512

#define USE_VERT_TANGENT_SPACE

typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark
} dlight_t;


// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
#ifdef REACTION
	// JBravo: Mirrored models
	qboolean	mirrored;		// mirrored matrix, needs reversed culling
#endif
	vec3_t		lightDir;		// normalized direction towards light
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;	// 32 bit rgba packed
	vec3_t		directedLight;
} trRefEntity_t;


typedef struct {
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelMatrix[16];
	float		transformMatrix[16];
} orientationr_t;

typedef enum
{
	IMGTYPE_COLORALPHA, // for color, lightmap, diffuse, and specular
	IMGTYPE_NORMAL,
	IMGTYPE_NORMALHEIGHT,
	IMGTYPE_DELUXE, // normals are swizzled, deluxe are not
} imgType_t;

typedef enum
{
	IMGFLAG_NONE           = 0x0000,
	IMGFLAG_MIPMAP         = 0x0001,
	IMGFLAG_PICMIP         = 0x0002,
	IMGFLAG_CUBEMAP        = 0x0004,
	IMGFLAG_NO_COMPRESSION = 0x0010,
	IMGFLAG_NOLIGHTSCALE   = 0x0020,
	IMGFLAG_CLAMPTOEDGE    = 0x0040,
	IMGFLAG_SRGB           = 0x0080,
	IMGFLAG_GENNORMALMAP   = 0x0100,
} imgFlags_t;

typedef struct image_s {
	char		imgName[MAX_QPATH];		// game path, including extension
	int			width, height;				// source image
	int			uploadWidth, uploadHeight;	// after power of two and picmip but not including clamp to MAX_TEXTURE_SIZE
	GLuint		texnum;					// gl texture binding

	int			frameUsed;			// for texture usage in frame statistics

	int			internalFormat;
	int			TMU;				// only needed for voodoo2

	imgType_t   type;
	imgFlags_t  flags;

	struct image_s*	next;
} image_t;

typedef enum
{
	VBO_USAGE_STATIC,
	VBO_USAGE_DYNAMIC
} vboUsage_t;

typedef struct VBO_s
{
	char            name[MAX_QPATH];

	uint32_t        vertexesVBO;
	int             vertexesSize;	// amount of memory data allocated for all vertices in bytes
	uint32_t        ofs_xyz;
	uint32_t        ofs_normal;
	uint32_t        ofs_st;
	uint32_t        ofs_lightmap;
	uint32_t        ofs_vertexcolor;
	uint32_t        ofs_lightdir;
#ifdef USE_VERT_TANGENT_SPACE
	uint32_t        ofs_tangent;
	uint32_t        ofs_bitangent;
#endif
	uint32_t        stride_xyz;
	uint32_t        stride_normal;
	uint32_t        stride_st;
	uint32_t        stride_lightmap;
	uint32_t        stride_vertexcolor;
	uint32_t        stride_lightdir;
#ifdef USE_VERT_TANGENT_SPACE
	uint32_t        stride_tangent;
	uint32_t        stride_bitangent;
#endif
	uint32_t        size_xyz;
	uint32_t        size_normal;

	int             attribs;
} VBO_t;

typedef struct IBO_s
{
	char            name[MAX_QPATH];

	uint32_t        indexesVBO;
	int             indexesSize;	// amount of memory data allocated for all triangles in bytes
//  uint32_t        ofsIndexes;
} IBO_t;

//===============================================================================

typedef enum {
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
} deform_t;

// deformVertexes types that can be handled by the GPU
typedef enum
{
	// do not edit: same as genFunc_t

	DGEN_NONE,
	DGEN_WAVE_SIN,
	DGEN_WAVE_SQUARE,
	DGEN_WAVE_TRIANGLE,
	DGEN_WAVE_SAWTOOTH,
	DGEN_WAVE_INVERSE_SAWTOOTH,
	DGEN_WAVE_NOISE,

	// do not edit until this line

	DGEN_BULGE,
	DGEN_MOVE
} deformGen_t;

typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST,
	AGEN_FRESNEL
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_EXACT_VERTEX_LIT,	// like CGEN_EXACT_VERTEX but takes a light direction from the lightgrid
	CGEN_VERTEX_LIT,		// like CGEN_VERTEX but takes a light direction from the lightgrid
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_FOG,				// standard fog
	CGEN_CONST				// fixed color
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	genFunc_t	func;

	float base;
	float amplitude;
	float phase;
	float frequency;
} waveForm_t;

#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
} deformStage_t;


typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;


#define	MAX_IMAGE_ANIMATIONS	8

typedef struct {
	image_t			*image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t	*texMods;

	int				videoMapHandle;
	qboolean		isLightmap;
	qboolean		vertexLightmap;
	qboolean		isVideoMap;
} textureBundle_t;

enum
{
	TB_COLORMAP    = 0,
	TB_DIFFUSEMAP  = 0,
	TB_LIGHTMAP    = 1,
	TB_LEVELSMAP   = 1,
	TB_SHADOWMAP   = 1,
	TB_NORMALMAP   = 2,
	TB_DELUXEMAP   = 3,
	TB_SHADOWMAP2  = 3,
	TB_SPECULARMAP = 4,
	TB_SHADOWMAP3  = 5,
	NUM_TEXTURE_BUNDLES = 6
};

typedef enum
{
	// material shader stage types
	ST_COLORMAP = 0,			// vanilla Q3A style shader treatening
	ST_DIFFUSEMAP = 0,          // treat color and diffusemap the same
	ST_NORMALMAP,
	ST_NORMALPARALLAXMAP,
	ST_SPECULARMAP,
	ST_GLSL
} stageType_t;

typedef struct {
	qboolean		active;
	
	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	byte			constantColor[4];			// for CGEN_CONST and AGEN_CONST

	unsigned		stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	qboolean		isDetail;

	stageType_t     type;
	struct shaderProgram_s *glslShaderGroup;
	int glslShaderIndex;
	vec2_t materialInfo;
} shaderStage_t;

struct shaderCommands_s;

// any change in the LIGHTMAP_* defines here MUST be reflected in
// R_FindShader() in tr_bsp.c
#define LIGHTMAP_2D         -4	// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX  -3	// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE -2
#define LIGHTMAP_NONE       -1

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6], *innerbox[6];
} skyParms_t;

typedef struct {
	vec3_t	color;
	float	depthForOpaque;
} fogParms_t;


typedef struct shader_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			lightmapIndex;			// for a shader to match, both name and lightmapIndex must match

	int			index;					// this shader == tr.shaders[index]
	int			sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]

	float		sort;					// lower numbered shaders draw before higher numbered

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean	explicitlyDefined;		// found in a .shader file

	int			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	int			contentFlags;

	qboolean	entityMergable;			// merge across entites optimizable (smoke, blood)

	qboolean	isSky;
	skyParms_t	sky;
	fogParms_t	fogParms;

	float		portalRange;			// distance to fog out at
	qboolean	isPortal;

	int			multitextureEnv;		// 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean	polygonOffset;			// set for decals and other items that must be offset 
	qboolean	noMipMaps;				// for console fonts, 2D elements, etc.
	qboolean	noPicMip;				// for images that must always be full resolution

	fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals

	int         vertexAttribs;          // not all shaders will need all data to be gathered

	int			numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	int			numUnfoggedPasses;
	shaderStage_t	*stages[MAX_SHADER_STAGES];		

	void		(*optimalStageIteratorFunc)( void );

  float clampTime;                                  // time this shader is clamped to
  float timeOffset;                                 // current time offset for this shader

  int numStates;                                    // if non-zero this is a state shader
  struct shader_s *currentShader;                   // current state if this is a state shader
  struct shader_s *parentShader;                    // current state if this is a state shader
  int currentState;                                 // current state index for cycle purposes
  long expireTime;                                  // time in milliseconds this expires

  struct shader_s *remappedShader;                  // current shader this one is remapped too

  int shaderStates[MAX_STATES_PER_SHADER];          // index to valid shader states

	struct	shader_s	*next;
} shader_t;

static ID_INLINE qboolean ShaderRequiresCPUDeforms(const shader_t * shader)
{
	if(shader->numDeforms)
	{
		const deformStage_t *ds = &shader->deforms[0];

		if (shader->numDeforms > 1)
			return qtrue;

		switch (ds->deformation)
		{
			case DEFORM_WAVE:
			case DEFORM_BULGE:
				return qfalse;

			default:
				return qtrue;
		}
	}

	return qfalse;
}

typedef struct shaderState_s {
  char shaderName[MAX_QPATH];     // name of shader this state belongs to
  char name[MAX_STATE_NAME];      // name of this state
  char stateShader[MAX_QPATH];    // shader this name invokes
  int cycleTime;                  // time this cycle lasts, <= 0 is forever
  shader_t *shader;
} shaderState_t;

enum
{
	ATTR_INDEX_POSITION       = 0,
	ATTR_INDEX_TEXCOORD0      = 1,
	ATTR_INDEX_TEXCOORD1      = 2,
	ATTR_INDEX_TANGENT        = 3,
	ATTR_INDEX_BITANGENT      = 4,
	ATTR_INDEX_NORMAL         = 5,
	ATTR_INDEX_COLOR          = 6,
	ATTR_INDEX_PAINTCOLOR     = 7,
	ATTR_INDEX_LIGHTDIRECTION = 8,
	ATTR_INDEX_BONE_INDEXES   = 9,
	ATTR_INDEX_BONE_WEIGHTS   = 10,

	// GPU vertex animations
	ATTR_INDEX_POSITION2      = 11,
	ATTR_INDEX_TANGENT2       = 12,
	ATTR_INDEX_BITANGENT2     = 13,
	ATTR_INDEX_NORMAL2        = 14
};

enum
{
	GLS_SRCBLEND_ZERO					= (1 << 0),
	GLS_SRCBLEND_ONE					= (1 << 1),
	GLS_SRCBLEND_DST_COLOR				= (1 << 2),
	GLS_SRCBLEND_ONE_MINUS_DST_COLOR	= (1 << 3),
	GLS_SRCBLEND_SRC_ALPHA				= (1 << 4),
	GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA	= (1 << 5),
	GLS_SRCBLEND_DST_ALPHA				= (1 << 6),
	GLS_SRCBLEND_ONE_MINUS_DST_ALPHA	= (1 << 7),
	GLS_SRCBLEND_ALPHA_SATURATE			= (1 << 8),

	GLS_SRCBLEND_BITS					= GLS_SRCBLEND_ZERO
											| GLS_SRCBLEND_ONE
											| GLS_SRCBLEND_DST_COLOR
											| GLS_SRCBLEND_ONE_MINUS_DST_COLOR
											| GLS_SRCBLEND_SRC_ALPHA
											| GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA
											| GLS_SRCBLEND_DST_ALPHA
											| GLS_SRCBLEND_ONE_MINUS_DST_ALPHA
											| GLS_SRCBLEND_ALPHA_SATURATE,

	GLS_DSTBLEND_ZERO					= (1 << 9),
	GLS_DSTBLEND_ONE					= (1 << 10),
	GLS_DSTBLEND_SRC_COLOR				= (1 << 11),
	GLS_DSTBLEND_ONE_MINUS_SRC_COLOR	= (1 << 12),
	GLS_DSTBLEND_SRC_ALPHA				= (1 << 13),
	GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA	= (1 << 14),
	GLS_DSTBLEND_DST_ALPHA				= (1 << 15),
	GLS_DSTBLEND_ONE_MINUS_DST_ALPHA	= (1 << 16),

	GLS_DSTBLEND_BITS					= GLS_DSTBLEND_ZERO
											| GLS_DSTBLEND_ONE
											| GLS_DSTBLEND_SRC_COLOR
											| GLS_DSTBLEND_ONE_MINUS_SRC_COLOR
											| GLS_DSTBLEND_SRC_ALPHA
											| GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA
											| GLS_DSTBLEND_DST_ALPHA
											| GLS_DSTBLEND_ONE_MINUS_DST_ALPHA,

	GLS_DEPTHMASK_TRUE					= (1 << 17),

	GLS_POLYMODE_LINE					= (1 << 18),

	GLS_DEPTHTEST_DISABLE				= (1 << 19),

	GLS_DEPTHFUNC_LESS					= (1 << 20),
	GLS_DEPTHFUNC_EQUAL					= (1 << 21),

	GLS_DEPTHFUNC_BITS					= GLS_DEPTHFUNC_LESS
											| GLS_DEPTHFUNC_EQUAL,

	GLS_ATEST_GT_0						= (1 << 22),
	GLS_ATEST_LT_128					= (1 << 23),
	GLS_ATEST_GE_128					= (1 << 24),
//	GLS_ATEST_GE_CUSTOM					= (1 << 25),

	GLS_ATEST_BITS						= GLS_ATEST_GT_0
											| GLS_ATEST_LT_128
											| GLS_ATEST_GE_128,
//											| GLS_ATEST_GT_CUSTOM,

	GLS_REDMASK_FALSE					= (1 << 26),
	GLS_GREENMASK_FALSE					= (1 << 27),
	GLS_BLUEMASK_FALSE					= (1 << 28),
	GLS_ALPHAMASK_FALSE					= (1 << 29),

	GLS_COLORMASK_BITS					= GLS_REDMASK_FALSE
											| GLS_GREENMASK_FALSE
											| GLS_BLUEMASK_FALSE
											| GLS_ALPHAMASK_FALSE,

	GLS_STENCILTEST_ENABLE				= (1 << 30),

	GLS_DEFAULT							= GLS_DEPTHMASK_TRUE
};

enum
{
	ATTR_POSITION =       0x0001,
	ATTR_TEXCOORD =       0x0002,
	ATTR_LIGHTCOORD =     0x0004,
	ATTR_TANGENT =        0x0008,
	ATTR_BITANGENT =      0x0010,
	ATTR_NORMAL =         0x0020,
	ATTR_COLOR =          0x0040,
	ATTR_PAINTCOLOR =     0x0080,
	ATTR_LIGHTDIRECTION = 0x0100,
	ATTR_BONE_INDEXES =   0x0200,
	ATTR_BONE_WEIGHTS =   0x0400,

	// for .md3 interpolation
	ATTR_POSITION2 =      0x0800,
	ATTR_TANGENT2 =       0x1000,
	ATTR_BITANGENT2 =     0x2000,
	ATTR_NORMAL2 =        0x4000,

	ATTR_DEFAULT = ATTR_POSITION,
	ATTR_BITS =	ATTR_POSITION |
				ATTR_TEXCOORD |
				ATTR_LIGHTCOORD |
				ATTR_TANGENT |
				ATTR_BITANGENT |
				ATTR_NORMAL |
				ATTR_COLOR |
				ATTR_PAINTCOLOR |
				ATTR_LIGHTDIRECTION |
				ATTR_BONE_INDEXES |
				ATTR_BONE_WEIGHTS |
				ATTR_POSITION2 |
				ATTR_TANGENT2 |
				ATTR_BITANGENT2 |
				ATTR_NORMAL2
};

enum
{
	GENERICDEF_USE_DEFORM_VERTEXES  = 0x0001,
	GENERICDEF_USE_TCGEN            = 0x0002,
	GENERICDEF_USE_VERTEX_ANIMATION = 0x0004,
	GENERICDEF_USE_FOG              = 0x0008,
	GENERICDEF_USE_RGBAGEN          = 0x0010,
	GENERICDEF_USE_LIGHTMAP         = 0x0020,
	GENERICDEF_ALL                  = 0x003F,
	GENERICDEF_COUNT                = 0x0040,
};

enum
{
	LIGHTDEF_USE_LIGHTMAP      = 0x0001,
	LIGHTDEF_USE_LIGHT_VECTOR  = 0x0002,
	LIGHTDEF_USE_LIGHT_VERTEX  = 0x0003,
	LIGHTDEF_LIGHTTYPE_MASK    = 0x0003,
	LIGHTDEF_USE_NORMALMAP     = 0x0004,
	LIGHTDEF_USE_SPECULARMAP   = 0x0008,
	LIGHTDEF_USE_DELUXEMAP     = 0x0010,
	LIGHTDEF_USE_PARALLAXMAP   = 0x0020,
	LIGHTDEF_USE_SHADOWMAP     = 0x0040,
	LIGHTDEF_TCGEN_ENVIRONMENT = 0x0080,
	LIGHTDEF_ENTITY            = 0x0100,
	LIGHTDEF_ALL               = 0x01FF,
	LIGHTDEF_COUNT             = 0x0200
};

enum
{
	GLSL_INT,
	GLSL_FLOAT,
	GLSL_FLOAT5,
	GLSL_VEC2,
	GLSL_VEC3,
	GLSL_VEC4,
	GLSL_MAT16
};

// Tr3B - shaderProgram_t represents a pair of one
// GLSL vertex and one GLSL fragment shader
typedef struct shaderProgram_s
{
	char            name[MAX_QPATH];

	GLhandleARB     program;
	GLhandleARB     vertexShader;
	GLhandleARB     fragmentShader;
	uint32_t        attribs;	// vertex array attributes

	// uniform parameters
	int   numUniforms;
	GLint *uniforms;
	GLint *uniformTypes;
	int   *uniformBufferOffsets;
	char  *uniformBuffer;
} shaderProgram_t;


enum
{
	TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX = 0,
	TEXTURECOLOR_UNIFORM_INVTEXRES,
	TEXTURECOLOR_UNIFORM_AUTOEXPOSUREMINMAX,
	TEXTURECOLOR_UNIFORM_TONEMINAVGMAXLINEAR,
	TEXTURECOLOR_UNIFORM_TEXTUREMAP,
	TEXTURECOLOR_UNIFORM_LEVELSMAP,
	TEXTURECOLOR_UNIFORM_COLOR,
	TEXTURECOLOR_UNIFORM_COUNT
};


enum
{
	FOGPASS_UNIFORM_FOGDISTANCE = 0,
	FOGPASS_UNIFORM_FOGDEPTH,
	FOGPASS_UNIFORM_FOGEYET,
	FOGPASS_UNIFORM_DEFORMGEN,
	FOGPASS_UNIFORM_DEFORMPARAMS,
	FOGPASS_UNIFORM_TIME,
	FOGPASS_UNIFORM_COLOR,
	FOGPASS_UNIFORM_MODELVIEWPROJECTIONMATRIX,
	FOGPASS_UNIFORM_VERTEXLERP,
	FOGPASS_UNIFORM_COUNT
};


enum
{
	DLIGHT_UNIFORM_DIFFUSEMAP = 0,
	DLIGHT_UNIFORM_DLIGHTINFO,
	DLIGHT_UNIFORM_DEFORMGEN,
	DLIGHT_UNIFORM_DEFORMPARAMS,
	DLIGHT_UNIFORM_TIME,
	DLIGHT_UNIFORM_COLOR,
	DLIGHT_UNIFORM_MODELVIEWPROJECTIONMATRIX,
	DLIGHT_UNIFORM_VERTEXLERP,
	DLIGHT_UNIFORM_COUNT
};


enum
{
	PSHADOW_UNIFORM_SHADOWMAP = 0,
	PSHADOW_UNIFORM_MODELVIEWPROJECTIONMATRIX,
	PSHADOW_UNIFORM_LIGHTFORWARD,
	PSHADOW_UNIFORM_LIGHTUP,
	PSHADOW_UNIFORM_LIGHTRIGHT,
	PSHADOW_UNIFORM_LIGHTORIGIN,
	PSHADOW_UNIFORM_LIGHTRADIUS,
	PSHADOW_UNIFORM_COUNT
};


enum
{
	GENERIC_UNIFORM_DIFFUSEMAP = 0,
	GENERIC_UNIFORM_LIGHTMAP,
	GENERIC_UNIFORM_NORMALMAP,
	GENERIC_UNIFORM_DELUXEMAP,
	GENERIC_UNIFORM_SPECULARMAP,
	GENERIC_UNIFORM_SHADOWMAP,
	GENERIC_UNIFORM_DIFFUSETEXMATRIX,
	//GENERIC_UNIFORM_NORMALTEXMATRIX,
	//GENERIC_UNIFORM_SPECULARTEXMATRIX,
	GENERIC_UNIFORM_TEXTURE1ENV,
	GENERIC_UNIFORM_VIEWORIGIN,
	GENERIC_UNIFORM_TCGEN0,
	GENERIC_UNIFORM_TCGEN0VECTOR0,
	GENERIC_UNIFORM_TCGEN0VECTOR1,
	GENERIC_UNIFORM_DEFORMGEN,
	GENERIC_UNIFORM_DEFORMPARAMS,
	GENERIC_UNIFORM_COLORGEN,
	GENERIC_UNIFORM_ALPHAGEN,
	GENERIC_UNIFORM_BASECOLOR,
	GENERIC_UNIFORM_VERTCOLOR,
	GENERIC_UNIFORM_AMBIENTLIGHT,
	GENERIC_UNIFORM_DIRECTEDLIGHT,
	GENERIC_UNIFORM_LIGHTORIGIN,
	GENERIC_UNIFORM_LIGHTRADIUS,
	GENERIC_UNIFORM_PORTALRANGE,
	GENERIC_UNIFORM_FOGDISTANCE,
	GENERIC_UNIFORM_FOGDEPTH,
	GENERIC_UNIFORM_FOGEYET,
	GENERIC_UNIFORM_FOGCOLORMASK,
	GENERIC_UNIFORM_MODELMATRIX,
	GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX,
	GENERIC_UNIFORM_TIME,
	GENERIC_UNIFORM_VERTEXLERP,
	GENERIC_UNIFORM_MATERIALINFO,
	GENERIC_UNIFORM_COUNT
};

enum
{
	SHADOWMASK_UNIFORM_SCREENDEPTHMAP = 0,
	SHADOWMASK_UNIFORM_SHADOWMAP,
	SHADOWMASK_UNIFORM_SHADOWMAP2,
	SHADOWMASK_UNIFORM_SHADOWMAP3,
	SHADOWMASK_UNIFORM_SHADOWMVP,
	SHADOWMASK_UNIFORM_SHADOWMVP2,
	SHADOWMASK_UNIFORM_SHADOWMVP3,
	SHADOWMASK_UNIFORM_VIEWORIGIN,
	SHADOWMASK_UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2
	SHADOWMASK_UNIFORM_VIEWFORWARD,
	SHADOWMASK_UNIFORM_VIEWLEFT,
	SHADOWMASK_UNIFORM_VIEWUP,
	SHADOWMASK_UNIFORM_COUNT
};

enum
{
	SSAO_UNIFORM_SCREENDEPTHMAP = 0,
	SSAO_UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2
	SSAO_UNIFORM_COUNT
};

enum
{
	DEPTHBLUR_UNIFORM_SCREENIMAGEMAP = 0,
	DEPTHBLUR_UNIFORM_SCREENDEPTHMAP,
	DEPTHBLUR_UNIFORM_VIEWINFO, // znear, zfar, width/2, height/2
	DEPTHBLUR_UNIFORM_COUNT
};

//
// Tr3B: these are fire wall functions to avoid expensive redundant glUniform* calls
//#define USE_UNIFORM_FIREWALL 1
//#define LOG_GLSL_UNIFORMS 1

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	stereoFrame_t	stereoFrame;

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];
	qboolean	areamaskModified;	// qtrue if areamask changed since last scene

	float		floatTime;			// tr.refdef.time / 1000.0

	float		blurFactor;

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s	*polys;

	int			numDrawSurfs;
	struct drawSurf_s	*drawSurfs;

	unsigned int dlightMask;
	int         num_pshadows;
	struct pshadow_s *pshadows;

	float       sunShadowMvp[3][16];
	float       sunDir[4];
	float       sunCol[4];
	float       sunAmbCol[4];
	float       colorScale;

	float       autoExposureMinMax[2];
	float       toneMinAvgMaxLinear[3];
} trRefdef_t;


//=================================================================================

// skins allow models to be retextured without modifying the model file
typedef struct {
	char		name[MAX_QPATH];
	shader_t	*shader;
} skinSurface_t;

typedef struct skin_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			numSurfaces;
	skinSurface_t	*surfaces[MD3_MAX_SURFACES];
} skin_t;


typedef struct {
	int			originalBrushNumber;
	vec3_t		bounds[2];

	unsigned	colorInt;				// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	qboolean	hasSurface;
	float		surface[4];
} fog_t;

typedef enum {
	VPF_NONE         = 0x00,
	VPF_SHADOWMAP    = 0x01,
	VPF_DEPTHSHADOW  = 0x02,
	VPF_DEPTHCLAMP   = 0x04,
	VPF_ORTHOGRAPHIC = 0x08,
	VPF_USESUNLIGHT  = 0x10,
} viewParmFlags_t;

typedef struct {
	orientationr_t	or;
	orientationr_t	world;
	vec3_t		pvsOrigin;			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
	viewParmFlags_t flags;
	int			frameSceneNum;		// copied from tr.frameSceneNum
	int			frameCount;			// copied from tr.frameCount
	cplane_t	portalPlane;		// clip anything behind this if mirroring
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	FBO_t		*targetFbo;
	float		fovX, fovY;
	float		projectionMatrix[16];
	cplane_t	frustum[5];
	vec3_t		visBounds[2];
	float		zFar;
	float       zNear;
	stereoFrame_t	stereoFrame;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/
typedef byte color4ub_t[4];

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MDV,
	SF_MD4,
#ifdef RAVENMD4
	SF_MDR,
#endif
	SF_IQM,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,
	SF_VBO_MESH,
	SF_VBO_MDVMESH,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct drawSurf_s {
	unsigned			sort;			// bit combination for fast compares
	surfaceType_t		*surface;		// any of surface*_t
} drawSurf_t;

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s {
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;

typedef struct srfDisplayList_s {
	surfaceType_t	surfaceType;
	int				listNum;
} srfDisplayList_t;


typedef struct srfFlare_s {
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
} srfFlare_t;

typedef struct
{
	vec3_t          xyz;
	vec2_t          st;
	vec2_t          lightmap;
	vec3_t          normal;
#ifdef USE_VERT_TANGENT_SPACE
	vec3_t          tangent;
	vec3_t          bitangent;
#endif
	vec3_t          lightdir;
	vec4_t			vertexColors;

#if DEBUG_OPTIMIZEVERTICES
	unsigned int    id;
#endif
} srfVert_t;

#ifdef USE_VERT_TANGENT_SPACE
#define srfVert_t_cleared(x) srfVert_t (x) = {{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0, 0}}
#else
#define srfVert_t_cleared(x) srfVert_t (x) = {{0, 0, 0}, {0, 0}, {0, 0}, {0, 0, 0}, {0, 0, 0},  {0, 0, 0, 0}}
#endif

typedef struct
{
	int             indexes[3];
	int             neighbors[3];
	vec4_t          plane;
	qboolean        facingLight;
	qboolean        degenerated;
} srfTriangle_t;


typedef struct srfGridMesh_s
{
	surfaceType_t   surfaceType;

	// dynamic lighting information
	int				dlightBits[SMP_FRAMES];
	int             pshadowBits[SMP_FRAMES];

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;

	int             numTriangles;
	srfTriangle_t  *triangles;

	int             numVerts;
	srfVert_t      *verts;

	// BSP VBO offsets
	int             firstVert;
	int             firstIndex;

	// static render data
	VBO_t          *vbo;		// points to bsp model VBO
	IBO_t          *ibo;
} srfGridMesh_t;


typedef struct
{
	surfaceType_t   surfaceType;

	// dynamic lighting information
	int			dlightBits[SMP_FRAMES];
	int         pshadowBits[SMP_FRAMES];

	// culling information
	cplane_t        plane;
//	vec3_t          bounds[2];

	// triangle definitions
	int             numTriangles;
	srfTriangle_t  *triangles;

	int             numVerts;
	srfVert_t      *verts;

	// BSP VBO offsets
	int             firstVert;
	int             firstIndex;

	// static render data
	VBO_t          *vbo;		// points to bsp model VBO
	IBO_t          *ibo;
} srfSurfaceFace_t;


// misc_models in maps are turned into direct geometry by xmap
typedef struct
{
	surfaceType_t   surfaceType;

	// dynamic lighting information
	int			dlightBits[SMP_FRAMES];
	int         pshadowBits[SMP_FRAMES];

	// culling information
//	vec3_t          bounds[2];

	// triangle definitions
	int             numTriangles;
	srfTriangle_t  *triangles;

	int             numVerts;
	srfVert_t      *verts;

	// BSP VBO offsets
	int             firstVert;
	int             firstIndex;

	// static render data
	VBO_t          *vbo;		// points to bsp model VBO
	IBO_t          *ibo;
} srfTriangles_t;

// inter-quake-model
typedef struct {
	int		num_vertexes;
	int		num_triangles;
	int		num_frames;
	int		num_surfaces;
	int		num_joints;
	struct srfIQModel_s	*surfaces;

	float		*positions;
	float		*texcoords;
	float		*normals;
	float		*tangents;
	byte		*blendIndexes;
	byte		*blendWeights;
	byte		*colors;
	int		*triangles;

	int		*jointParents;
	float		*poseMats;
	float		*bounds;
	char		*names;
} iqmData_t;

// inter-quake-model surface
typedef struct srfIQModel_s {
	surfaceType_t	surfaceType;
	char		name[MAX_QPATH];
	shader_t	*shader;
	iqmData_t	*data;
	int		first_vertex, num_vertexes;
	int		first_triangle, num_triangles;
} srfIQModel_t;

typedef struct srfVBOMesh_s
{
	surfaceType_t   surfaceType;

	struct shader_s *shader;	// FIXME move this to somewhere else
	int				fogIndex;

	// dynamic lighting information
	int			dlightBits[SMP_FRAMES];
	int         pshadowBits[SMP_FRAMES];

	// culling information
	vec3_t          bounds[2];

	// backEnd stats
	int             numIndexes;
	int             numVerts;
	int				firstIndex;

	// static render data
	VBO_t          *vbo;
	IBO_t          *ibo;
} srfVBOMesh_t;

typedef struct srfVBOMDVMesh_s
{
	surfaceType_t   surfaceType;

	struct mdvModel_s *mdvModel;
	struct mdvSurface_s *mdvSurface;

	// backEnd stats
	int             numIndexes;
	int             numVerts;

	// static render data
	VBO_t          *vbo;
	IBO_t          *ibo;
} srfVBOMDVMesh_t;

extern	void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

/*
==============================================================================

SHADOWS

==============================================================================
*/

typedef struct pshadow_s
{
	float sort;
	
	int    numEntities;
	int    entityNums[8];
	vec3_t entityOrigins[8];
	float  entityRadiuses[8];

	float viewRadius;
	vec3_t viewOrigin;

	vec3_t lightViewAxis[3];
	vec3_t lightOrigin;
	float  lightRadius;
	cplane_t cullPlane;
} pshadow_t;


/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

#define CULLINFO_NONE   0
#define CULLINFO_BOX    1
#define CULLINFO_SPHERE 2
#define CULLINFO_PLANE  4

typedef struct cullinfo_s {
	int             type;
	vec3_t          bounds[2];
	vec3_t			localOrigin;
	float			radius;
	cplane_t        plane;
} cullinfo_t;

typedef struct msurface_s {
	//int					viewCount;		// if == tr.viewCount, already added
	struct shader_s		*shader;
	int					fogIndex;
	cullinfo_t          cullinfo;

	surfaceType_t		*data;			// any of srf*_t
} msurface_t;


#define	CONTENTS_NODE		-1
typedef struct mnode_s {
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int             visCounts[MAX_VISCOUNTS];	// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_s	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	// leaf specific
	int			cluster;
	int			area;

	int         firstmarksurface;
	int			nummarksurfaces;
} mnode_t;

typedef struct {
	vec3_t		bounds[2];		// for culling
	int	        firstSurface;
	int			numSurfaces;
} bmodel_t;

typedef struct {
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	int			numBModels;
	bmodel_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	VBO_t          *vbo;
	IBO_t          *ibo;

	int         numWorldSurfaces;

	int			numsurfaces;
	msurface_t	*surfaces;
	int         *surfacesViewCount;
	int         *surfacesDlightBits;
	int			*surfacesPshadowBits;

	int			numMergedSurfaces;
	msurface_t	*mergedSurfaces;
	int         *mergedSurfacesViewCount;
	int         *mergedSurfacesDlightBits;
	int			*mergedSurfacesPshadowBits;

	int			nummarksurfaces;
	int         *marksurfaces;
	int         *viewSurfaces;

	int			numfogs;
	fog_t		*fogs;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];
	byte		*lightGridData;
	float		*hdrLightGrid;


	int			numClusters;
	int			clusterBytes;
	const byte	*vis;			// may be passed in by CM_LoadMap to save space

	byte		*novis;			// clusterBytes of 0xff

	char		*entityString;
	char		*entityParsePoint;
} world_t;


/*
==============================================================================
MDV MODELS - meta format for vertex animation models like .md2, .md3, .mdc
==============================================================================
*/
typedef struct
{
	float           bounds[2][3];
	float           localOrigin[3];
	float           radius;
} mdvFrame_t;

typedef struct
{
	float           origin[3];
	float           axis[3][3];
} mdvTag_t;

typedef struct
{
	char            name[MAX_QPATH];	// tag name
} mdvTagName_t;

typedef struct
{
	vec3_t          xyz;
	vec3_t          normal;
#ifdef USE_VERT_TANGENT_SPACE
	vec3_t          tangent;
	vec3_t          bitangent;
#endif
} mdvVertex_t;

typedef struct
{
	float           st[2];
} mdvSt_t;

typedef struct mdvSurface_s
{
	surfaceType_t   surfaceType;

	char            name[MAX_QPATH];	// polyset name

	int             numShaderIndexes;
	int				*shaderIndexes;

	int             numVerts;
	mdvVertex_t    *verts;
	mdvSt_t        *st;

	int             numTriangles;
	srfTriangle_t  *triangles;

	struct mdvModel_s *model;
} mdvSurface_t;

typedef struct mdvModel_s
{
	int             numFrames;
	mdvFrame_t     *frames;

	int             numTags;
	mdvTag_t       *tags;
	mdvTagName_t   *tagNames;

	int             numSurfaces;
	mdvSurface_t   *surfaces;

	int             numVBOSurfaces;
	srfVBOMDVMesh_t  *vboSurfaces;

	int             numSkins;
} mdvModel_t;


//======================================================================

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MD4,
#ifdef RAVENMD4
	MOD_MDR,
#endif
	MOD_IQM
} modtype_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;		// model = tr.models[model->index]

	int			dataSize;	// just for listing purposes
	bmodel_t	*bmodel;		// only if type == MOD_BRUSH
	mdvModel_t	*mdv[MD3_MAX_LODS];	// only if type == MOD_MESH
	void	*modelData;			// only if type == (MOD_MD4 | MOD_MDR | MOD_IQM)

	int			 numLods;
} model_t;


#define	MAX_MOD_KNOWN	1024

void		R_ModelInit (void);
model_t		*R_GetModelByHandle( qhandle_t hModel );
int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		R_Modellist_f (void);

//====================================================
extern	refimport_t		ri;

#define	MAX_DRAWIMAGES			2048
#define	MAX_SKINS				1024


#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

0 - 1	: dlightmap index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
2 - 6	: fog index
11 - 20	: entity index
21 - 31	: sorted shader index

	TTimo - 1.32
0-1   : dlightmap index
2-6   : fog index
7-16  : entity index
17-30 : sorted shader index

    SmileTheory - for pshadows
17-31 : sorted shader index
7-16  : entity index
2-6   : fog index
1     : pshadow flag
0     : dlight flag
*/
#define	QSORT_FOGNUM_SHIFT	2
#define	QSORT_REFENTITYNUM_SHIFT	7
#define	QSORT_SHADERNUM_SHIFT	(QSORT_REFENTITYNUM_SHIFT+REFENTITYNUM_BITS)
#if (QSORT_SHADERNUM_SHIFT+SHADERNUM_BITS) > 32
	#error "Need to update sorting, too many bits."
#endif
#define QSORT_PSHADOW_SHIFT     1

extern	int			gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)


// the renderer front end should never modify glstate_t
typedef struct {
	int			currenttextures[NUM_TEXTURE_BUNDLES];
	int			currenttmu;
	qboolean	finishCalled;
	int			texEnv[2];
	int			faceCulling;
	unsigned long	glStateBits;
	uint32_t		vertexAttribsState;
	uint32_t		vertexAttribPointersSet;
	uint32_t        vertexAttribsNewFrame;
	uint32_t        vertexAttribsOldFrame;
	float           vertexAttribsInterpolation;
	shaderProgram_t *currentProgram;
	FBO_t          *currentFBO;
	VBO_t          *currentVBO;
	IBO_t          *currentIBO;
	matrix_t        modelview;
	matrix_t        projection;
	matrix_t		modelviewProjection;
} glstate_t;

typedef enum {
	MI_NONE,
	MI_NVX,
	MI_ATI
} memInfo_t;

typedef enum {
	TCR_NONE = 0x0000,
	TCR_LATC = 0x0001,
	TCR_BPTC = 0x0002,
} textureCompressionRef_t;

// We can't change glConfig_t without breaking DLL/vms compatibility, so
// store extensions we have here.
typedef struct {
	qboolean    drawRangeElements;
	qboolean    multiDrawArrays;
	qboolean	occlusionQuery;

	int glslMajorVersion;
	int glslMinorVersion;

	memInfo_t   memInfo;

	qboolean framebufferObject;
	int maxRenderbufferSize;
	int maxColorAttachments;

	qboolean textureNonPowerOfTwo;
	qboolean textureFloat;
	qboolean halfFloatPixel;
	qboolean packedDepthStencil;
	textureCompressionRef_t textureCompression;
	
	qboolean framebufferMultisample;
	qboolean framebufferBlit;

	qboolean texture_srgb;
	qboolean framebuffer_srgb;

	qboolean depthClamp;
} glRefConfig_t;


typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	int     c_surfBatches;
	float	c_overDraw;
	
	int		c_vboVertexBuffers;
	int		c_vboIndexBuffers;
	int		c_vboVertexes;
	int		c_vboIndexes;

	int     c_staticVboDraws;
	int     c_dynamicVboDraws;

	int     c_multidraws;
	int     c_multidrawsMerged;

	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int     c_glslShaderBinds;
	int     c_genericDraws;
	int     c_lightallDraws;
	int     c_fogDraws;
	int     c_dlightDraws;

	int		msec;			// total msec for backend run
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
	int			smpFrame;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	or;
	backEndCounters_t	pc;
	qboolean	isHyperspace;
	trRefEntity_t	*currentEntity;
	qboolean	skyRenderedThisView;	// flag for drawing sun

#ifdef REACTION
	vec3_t					sunFlarePos;
	qboolean				viewHasSunFlare;
	qboolean                frameHasSunFlare;
#endif

	qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	byte		color2D[4];
	qboolean	vertexes2D;		// shader needs to be finished
	trRefEntity_t	entity2D;	// currentEntity will point at this when doing 2D rendering

	FBO_t *last2DFBO;
	qboolean    colorMask[4];
	qboolean    framePostProcessed;
	qboolean    depthFill;
} backEndState_t;

/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct {
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visIndex;
	int						visClusters[MAX_VISCOUNTS];
	int						visCounts[MAX_VISCOUNTS];	// incremented every time a new vis cluster is entered

	int						frameCount;		// incremented every frame
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						smpFrame;		// toggles from 0 to 1 every endFrame

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	qboolean				worldMapLoaded;
	qboolean				worldDeluxeMapping;
	vec2_t                  autoExposureMinMax;
	vec3_t                  toneMinAvgMaxLevel;
	world_t					*world;

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*flareImage;
	image_t					*whiteImage;			// full of 0xff
	image_t					*identityLightImage;	// full of tr.identityLightByte

	image_t                 *shadowCubemaps[MAX_DLIGHTS];
	

	image_t					*renderImage;
	image_t					*godRaysImage;
	image_t					*renderDepthImage;
	image_t					*pshadowMaps[MAX_DRAWN_PSHADOWS];
	image_t					*textureScratchImage[2];
	image_t					*screenScratchImage;
	image_t                 *quarterImage[2];
	image_t					*calcLevelsImage;
	image_t					*targetLevelsImage;
	image_t					*fixedLevelsImage;
	image_t					*sunShadowDepthImage[3];
	image_t                 *screenShadowImage;
	image_t                 *screenSsaoImage;
	image_t					*hdrDepthImage;
	
	image_t					*textureDepthImage;

	FBO_t					*renderFbo;
	FBO_t					*msaaResolveFbo;
	FBO_t					*godRaysFbo;
	FBO_t					*depthFbo;
	FBO_t					*pshadowFbos[MAX_DRAWN_PSHADOWS];
	FBO_t					*textureScratchFbo[2];
	FBO_t					*screenScratchFbo;
	FBO_t                   *quarterFbo[2];
	FBO_t					*calcLevelsFbo;
	FBO_t					*targetLevelsFbo;
	FBO_t					*sunShadowFbo[3];
	FBO_t					*screenShadowFbo;
	FBO_t					*screenSsaoFbo;
	FBO_t					*hdrDepthFbo;

	shader_t				*defaultShader;
	shader_t				*shadowShader;
	shader_t				*projectionShadowShader;

	shader_t				*flareShader;
	shader_t				*sunShader;

	int						numLightmaps;
	int						lightmapSize;
	image_t					**lightmaps;
	image_t					**deluxemaps;

	int                     fatLightmapSize;
	int		                fatLightmapStep;

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_REFENTITYNUM_SHIFT
	model_t					*currentModel;

	//
	// GPU shader programs
	//
	shaderProgram_t genericShader[GENERICDEF_COUNT];
	shaderProgram_t textureColorShader;
	shaderProgram_t fogShader;
	shaderProgram_t dlightallShader;
	shaderProgram_t lightallShader[LIGHTDEF_COUNT];
	shaderProgram_t shadowmapShader;
	shaderProgram_t pshadowShader;
	shaderProgram_t down4xShader;
	shaderProgram_t bokehShader;
	shaderProgram_t tonemapShader;
	shaderProgram_t calclevels4xShader[2];
	shaderProgram_t shadowmaskShader;
	shaderProgram_t ssaoShader;
	shaderProgram_t depthBlurShader[2];


	// -----------------------------------------

	viewParms_t				viewParms;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t			or;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	float                   mapLightScale;

	qboolean                sunShadows;
	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunAmbient;
	vec3_t					sunDirection;

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	int						numImages;
	image_t					*images[MAX_DRAWIMAGES];

	int						numFBOs;
	FBO_t					*fbos[MAX_FBOS];

	int						numVBOs;
	VBO_t					*vbos[MAX_VBOS];

	int						numIBOs;
	IBO_t					*ibos[MAX_IBOS];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

#ifdef REACTION
	GLuint					sunFlareQuery[2];
	int						sunFlareQueryIndex;
	qboolean				sunFlareQueryActive[2];
#endif

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];
} trGlobals_t;

extern backEndState_t	backEnd;
extern trGlobals_t	tr;
extern glconfig_t	glConfig;		// outside of TR since it shouldn't be cleared during ref re-init
extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init

// These three variables should live inside glConfig but can't because of compatibility issues to the original ID vms.
// If you release a stand-alone game and your mod uses tr_types.h from this build you can safely move them to
// the glconfig_t struct.
extern qboolean  textureFilterAnisotropic;
extern int       maxAnisotropy;
extern glRefConfig_t glRefConfig;
extern float     displayAspect;


//
// cvars
//
extern cvar_t	*r_flareSize;
extern cvar_t	*r_flareFade;
// coefficient for the flare intensity falloff function.
#define FLARE_STDCOEFF "150"
extern cvar_t	*r_flareCoeff;

extern cvar_t	*r_railWidth;
extern cvar_t	*r_railCoreWidth;
extern cvar_t	*r_railSegmentLength;

extern cvar_t	*r_ignore;				// used for debugging anything
extern cvar_t	*r_verbose;				// used for verbose debug spew

extern cvar_t	*r_znear;				// near Z clip plane
extern cvar_t	*r_zproj;				// z distance of projection plane
extern cvar_t	*r_stereoSeparation;			// separation of cameras for stereo rendering

extern cvar_t	*r_stencilbits;			// number of desired stencil bits
extern cvar_t	*r_depthbits;			// number of desired depth bits
extern cvar_t	*r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern cvar_t	*r_texturebits;			// number of desired texture bits
extern cvar_t	*r_ext_multisample;
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error

extern cvar_t	*r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern cvar_t	*r_lodbias;				// push/pull LOD transitions
extern cvar_t	*r_lodscale;

extern cvar_t	*r_inGameVideo;				// controls whether in game video should be draw
extern cvar_t	*r_fastsky;				// controls whether sky should be cleared or drawn
extern cvar_t	*r_drawSun;				// controls drawing of sun quad
extern cvar_t	*r_dynamiclight;		// dynamic lights enabled/disabled
extern cvar_t	*r_dlightBacks;			// dlight non-facing surfaces for continuity

extern	cvar_t	*r_norefresh;			// bypasses the ref rendering
extern	cvar_t	*r_drawentities;		// disable/enable entity rendering
extern	cvar_t	*r_drawworld;			// disable/enable world rendering
extern	cvar_t	*r_speeds;				// various levels of information display
extern  cvar_t	*r_detailTextures;		// enables/disables detail texturing stages
extern	cvar_t	*r_novis;				// disable/enable usage of PVS
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	cvar_t	*r_nocurves;
extern	cvar_t	*r_showcluster;

extern cvar_t	*r_mode;				// video mode
extern cvar_t	*r_fullscreen;
extern cvar_t	*r_noborder;
extern cvar_t	*r_gamma;
extern cvar_t	*r_ignorehwgamma;		// overrides hardware gamma capabilities

extern cvar_t	*r_allowExtensions;				// global enable/disable of OpenGL extensions
extern cvar_t	*r_ext_compressed_textures;		// these control use of specific extensions
extern cvar_t	*r_ext_multitexture;
extern cvar_t	*r_ext_compiled_vertex_array;
extern cvar_t	*r_ext_texture_env_add;

extern cvar_t	*r_ext_texture_filter_anisotropic;
extern cvar_t	*r_ext_max_anisotropy;

extern  cvar_t  *r_ext_draw_range_elements;
extern  cvar_t  *r_ext_multi_draw_arrays;
extern  cvar_t  *r_ext_framebuffer_object;
extern  cvar_t  *r_ext_texture_float;
extern  cvar_t  *r_arb_half_float_pixel;
extern  cvar_t  *r_ext_framebuffer_multisample;

extern	cvar_t	*r_nobind;						// turns off binding to appropriate textures
extern	cvar_t	*r_singleShader;				// make most world faces use default shader
extern	cvar_t	*r_roundImagesDown;
extern	cvar_t	*r_colorMipLevels;				// development aid to see texture mip usage
extern	cvar_t	*r_picmip;						// controls picmip values
extern	cvar_t	*r_finish;
extern	cvar_t	*r_drawBuffer;
extern	cvar_t	*r_swapInterval;
extern	cvar_t	*r_textureMode;
extern	cvar_t	*r_offsetFactor;
extern	cvar_t	*r_offsetUnits;

extern	cvar_t	*r_fullbright;					// avoid lightmap pass
extern	cvar_t	*r_lightmap;					// render lightmaps only
extern	cvar_t	*r_vertexLight;					// vertex lighting mode for better performance
extern	cvar_t	*r_uiFullScreen;				// ui is running fullscreen

extern	cvar_t	*r_logFile;						// number of frames to emit GL logs
extern	cvar_t	*r_showtris;					// enables wireframe rendering of the world
extern	cvar_t	*r_showsky;						// forces sky in front of all surfaces
extern	cvar_t	*r_shownormals;					// draws wireframe normals
extern	cvar_t	*r_clear;						// force screen clear every frame

extern	cvar_t	*r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern	cvar_t	*r_flares;						// light flares

extern	cvar_t	*r_intensity;

extern	cvar_t	*r_lockpvs;
extern	cvar_t	*r_noportals;
extern	cvar_t	*r_portalOnly;

extern	cvar_t	*r_subdivisions;
extern	cvar_t	*r_lodCurveError;
extern	cvar_t	*r_smp;
extern	cvar_t	*r_showSmp;
extern	cvar_t	*r_skipBackEnd;

extern	cvar_t	*r_stereoEnabled;
extern	cvar_t	*r_anaglyphMode;

extern  cvar_t  *r_mergeMultidraws;
extern  cvar_t  *r_mergeLeafSurfaces;

extern  cvar_t  *r_hdr;
extern  cvar_t  *r_postProcess;

extern  cvar_t  *r_toneMap;
extern  cvar_t  *r_forceToneMap;
extern  cvar_t  *r_forceToneMapMin;
extern  cvar_t  *r_forceToneMapAvg;
extern  cvar_t  *r_forceToneMapMax;

extern  cvar_t  *r_autoExposure;
extern  cvar_t  *r_forceAutoExposure;
extern  cvar_t  *r_forceAutoExposureMin;
extern  cvar_t  *r_forceAutoExposureMax;

extern  cvar_t  *r_cameraExposure;

extern  cvar_t  *r_srgb;

extern  cvar_t  *r_depthPrepass;
extern  cvar_t  *r_ssao;

extern  cvar_t  *r_normalMapping;
extern  cvar_t  *r_specularMapping;
extern  cvar_t  *r_deluxeMapping;
extern  cvar_t  *r_parallaxMapping;
extern  cvar_t  *r_normalAmbient;
extern  cvar_t  *r_dlightMode;
extern  cvar_t  *r_pshadowDist;
extern  cvar_t  *r_recalcMD3Normals;
extern  cvar_t  *r_mergeLightmaps;
extern  cvar_t  *r_imageUpsample;
extern  cvar_t  *r_imageUpsampleMaxSize;
extern  cvar_t  *r_imageUpsampleType;
extern  cvar_t  *r_genNormalMaps;
extern  cvar_t  *r_forceSun;
extern  cvar_t  *r_forceSunMapLightScale;
extern  cvar_t  *r_forceSunLightScale;
extern  cvar_t  *r_forceSunAmbientScale;
extern  cvar_t  *r_sunShadows;
extern  cvar_t  *r_shadowFilter;
extern  cvar_t  *r_shadowMapSize;
extern  cvar_t  *r_shadowCascadeZNear;
extern  cvar_t  *r_shadowCascadeZFar;
extern  cvar_t  *r_shadowCascadeZBias;

extern	cvar_t	*r_greyscale;

extern	cvar_t	*r_ignoreGLErrors;

extern	cvar_t	*r_overBrightBits;
extern	cvar_t	*r_mapOverBrightBits;

extern	cvar_t	*r_debugSurface;
extern	cvar_t	*r_simpleMipMaps;

extern	cvar_t	*r_showImages;
extern	cvar_t	*r_debugSort;

extern	cvar_t	*r_printShaders;
extern	cvar_t	*r_saveFontData;

extern cvar_t	*r_marksOnTriangleMeshes;

//====================================================================

float R_NoiseGet4f( float x, float y, float z, float t );
void  R_NoiseInit( void );

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );
void R_RenderDlightCubemaps(const refdef_t *fd);
void R_RenderPshadowMaps(const refdef_t *fd);
void R_RenderSunShadowMaps(const refdef_t *fd, int level);

void R_AddMD3Surfaces( trRefEntity_t *e );
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap, int *pshadowMap );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, 
				   int fogIndex, int dlightMap, int pshadowMap );

void R_CalcTangentSpace(vec3_t tangent, vec3_t bitangent, vec3_t normal,
                        const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec2_t t0, const vec2_t t1, const vec2_t t2);
qboolean R_CalcTangentVectors(srfVert_t * dv[3]);
void R_CalcSurfaceTriangleNeighbors(int numTriangles, srfTriangle_t * triangles);
void R_CalcSurfaceTrianglePlanes(int numTriangles, srfTriangle_t * triangles, srfVert_t * verts);

#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
void R_LocalNormalToWorld (const vec3_t local, vec3_t world);
void R_LocalPointToWorld (const vec3_t local, vec3_t world);
int R_CullBox (vec3_t bounds[2]);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadiusEx( const vec3_t origin, float radius, const cplane_t* frustum, int numPlanes );
int R_CullPointAndRadius( const vec3_t origin, float radius );
int R_CullLocalPointAndRadius( const vec3_t origin, float radius );

void R_SetupProjection(viewParms_t *dest, float zProj, float zFar, qboolean computeFrustum);
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *or );

/*
** GL wrapper/helper functions
*/
void	GL_Bind( image_t *image );
void	GL_BindCubemap( image_t *image );
void	GL_BindToTMU( image_t *image, int tmu );
void	GL_SetDefaultState (void);
void	GL_SelectTexture( int unit );
void	GL_TextureMode( const char *string );
void	GL_CheckErrs( char *file, int line );
#define GL_CheckErrors(...) GL_CheckErrs(__FILE__, __LINE__)
void	GL_State( unsigned long stateVector );
void    GL_SetProjectionMatrix(matrix_t matrix);
void    GL_SetModelviewMatrix(matrix_t matrix);
void	GL_TexEnv( int env );
void	GL_Cull( int cullType );

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define		GLS_SRCBLEND_BITS					0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define		GLS_DSTBLEND_BITS					0x000000f0

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000
#define GLS_DEPTHFUNC_GREATER                   0x00040000
#define GLS_DEPTHFUNC_BITS                      0x00060000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define		GLS_ATEST_BITS						0x70000000

#define GLS_DEFAULT			GLS_DEPTHMASK_TRUE

void	RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void	RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void		RE_BeginFrame( stereoFrame_t stereoFrame );
void		RE_BeginRegistration( glconfig_t *glconfig );
void		RE_LoadWorldMap( const char *mapname );
void		RE_SetWorldVisData( const byte *vis );
qhandle_t	RE_RegisterModel( const char *name );
qhandle_t	RE_RegisterSkin( const char *name );
void		RE_Shutdown( qboolean destroyWindow );

qboolean	R_GetEntityToken( char *buffer, int size );

model_t		*R_AllocModel( void );

void    	R_Init( void );
image_t     *R_FindImageFile( const char *name, imgType_t type, imgFlags_t flags );
image_t *R_CreateImage( const char *name, byte *pic, int width, int height, imgType_t type, imgFlags_t flags, int internalFormat );
void		R_UpdateSubImage( image_t *image, byte *pic, int x, int y, int width, int height );
qboolean	R_GetModeInfo( int *width, int *height, float *windowAspect, int mode );

void		R_SetColorMappings( void );
void		R_GammaCorrect( byte *buffer, int bufSize );

void	R_ImageList_f( void );
void	R_SkinList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
const void *RB_TakeScreenshotCmd( const void *data );
void	R_ScreenShot_f( void );

void	R_InitFogTable( void );
float	R_FogFactor( float s, float t );
void	R_InitImages( void );
void	R_DeleteTextures( void );
int		R_SumOfUsedImages( void );
void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );

int R_ComputeLOD( trRefEntity_t *ent );

const void *RB_TakeVideoFrameCmd( const void *data );

//
// tr_shader.c
//
qhandle_t		 RE_RegisterShaderLightMap( const char *name, int lightmapIndex );
qhandle_t		 RE_RegisterShader( const char *name );
qhandle_t		 RE_RegisterShaderNoMip( const char *name );
qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage);

shader_t	*R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
shader_t	*R_GetShaderByHandle( qhandle_t hShader );
shader_t	*R_GetShaderByState( int index, long *cycleTime );
shader_t *R_FindShaderByName( const char *name );
void		R_InitShaders( void );
void		R_ShaderList_f( void );
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_Init( void );
void		GLimp_Shutdown( void );
void		GLimp_EndFrame( void );

qboolean	GLimp_SpawnRenderThread( void (*function)( void ) );
void		*GLimp_RendererSleep( void );
void		GLimp_FrontEndSleep( void );
void		GLimp_WakeRenderer( void *data );

void		GLimp_LogComment( char *comment );
void		GLimp_Minimize(void);

// NOTE TTimo linux works with float gamma value, not the gamma table
//   the params won't be used, getting the r_gamma cvar directly
void		GLimp_SetGamma( unsigned char red[256], 
						    unsigned char green[256],
							unsigned char blue[256] );


void		GLimp_InitExtraExtensions( void );
/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/

typedef struct stageVars
{
	color4ub_t	colors[SHADER_MAX_VERTEXES];
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;

#define MAX_MULTIDRAW_PRIMITIVES	16384

typedef struct shaderCommands_s 
{
	glIndex_t	indexes[SHADER_MAX_INDEXES] QALIGN(16);
	vec4_t		xyz[SHADER_MAX_VERTEXES] QALIGN(16);
	vec4_t		normal[SHADER_MAX_VERTEXES] QALIGN(16);
#ifdef USE_VERT_TANGENT_SPACE
	vec4_t		tangent[SHADER_MAX_VERTEXES] QALIGN(16);
	vec4_t		bitangent[SHADER_MAX_VERTEXES] QALIGN(16);
#endif
	vec2_t		texCoords[SHADER_MAX_VERTEXES][2] QALIGN(16);
	vec4_t		vertexColors[SHADER_MAX_VERTEXES] QALIGN(16);
	vec4_t      lightdir[SHADER_MAX_VERTEXES] QALIGN(16);
	//int			vertexDlightBits[SHADER_MAX_VERTEXES] QALIGN(16);

	VBO_t       *vbo;
	IBO_t       *ibo;
	qboolean    useInternalVBO;

	stageVars_t	svars QALIGN(16);

	//color4ub_t	constantColor255[SHADER_MAX_VERTEXES] QALIGN(16);

	shader_t	*shader;
	float		shaderTime;
	int			fogNum;

	int			dlightBits;	// or together of all vertexDlightBits
	int         pshadowBits;

	int			firstIndex;
	int			numIndexes;
	int			numVertexes;

	int         multiDrawPrimitives;
	GLsizei     multiDrawNumIndexes[MAX_MULTIDRAW_PRIMITIVES];
	GLvoid *    multiDrawFirstIndex[MAX_MULTIDRAW_PRIMITIVES];
	GLvoid *    multiDrawLastIndex[MAX_MULTIDRAW_PRIMITIVES];

	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;
} shaderCommands_t;

extern	shaderCommands_t	tess;

void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void R_DrawElementsVBO( int numIndexes, int firstIndex );
void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 );
void RB_InstantQuad( vec4_t quadVerts[4] );
//void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4], vec4_t color, shaderProgram_t *sp, vec2_t invTexRes);
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4]);

void RB_ShowImages( void );


/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );
qboolean R_inPVS( const vec3_t p1, const vec3_t p2 );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare( void *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal );
void RB_AddDlightFlares( void );
void RB_RenderFlares (void);

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( bmodel_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *or );
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
int R_LightDirForPoint( vec3_t point, vec3_t lightDir, vec3_t normal, world_t *world );


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( void );

/*
============================================================

SKIES

============================================================
*/

void R_BuildCloudData( shaderCommands_t *shader );
void R_InitSkyTexCoords( float cloudLayerHeight );
void R_DrawSkyBox( shaderCommands_t *shader );
void RB_DrawSun( void );
void RB_ClipSkyPolygons( shaderCommands_t *shader );

/*
============================================================

CURVE TESSELATION

============================================================
*/

#define PATCH_STITCHING

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
								srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

VERTEX BUFFER OBJECTS

============================================================
*/
VBO_t          *R_CreateVBO(const char *name, byte * vertexes, int vertexesSize, vboUsage_t usage);
VBO_t          *R_CreateVBO2(const char *name, int numVertexes, srfVert_t * vertexes, uint32_t stateBits, vboUsage_t usage);

IBO_t          *R_CreateIBO(const char *name, byte * indexes, int indexesSize, vboUsage_t usage);
IBO_t          *R_CreateIBO2(const char *name, int numTriangles, srfTriangle_t * triangles, vboUsage_t usage);

void            R_BindVBO(VBO_t * vbo);
void            R_BindNullVBO(void);

void            R_BindIBO(IBO_t * ibo);
void            R_BindNullIBO(void);

void            R_InitVBOs(void);
void            R_ShutdownVBOs(void);
void            R_VBOList_f(void);

void            RB_UpdateVBOs(unsigned int attribBits);


/*
============================================================

GLSL

============================================================
*/

void GLSL_InitGPUShaders(void);
void GLSL_ShutdownGPUShaders(void);
void GLSL_VertexAttribsState(uint32_t stateBits);
void GLSL_VertexAttribPointers(uint32_t attribBits);
void GLSL_BindProgram(shaderProgram_t * program);
void GLSL_BindNullProgram(void);

void GLSL_SetNumUniforms(shaderProgram_t *program, int numUniforms);
void GLSL_SetUniformName(shaderProgram_t *program, int uniformNum, const char *name);
void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value);
void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value);
void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v);
void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v);
void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v);
void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v);
void GLSL_SetUniformMatrix16(shaderProgram_t *program, int uniformNum, const matrix_t matrix);

shaderProgram_t *GLSL_GetGenericShaderProgram(int stage);

/*
============================================================

SCENE GENERATION

============================================================
*/

void R_ToggleSmpFrame( void );

void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_RenderScene( const refdef_t *fd );

#ifdef RAVENMD4
/*
=============================================================

UNCOMPRESSING BONES

=============================================================
*/

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

void MC_UnCompress(float mat[3][4],const unsigned char * comp);
#endif

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

// void R_MakeAnimModel( model_t *model );      haven't seen this one really, so not needed I guess.
void R_AddAnimSurfaces( trRefEntity_t *ent );
void RB_SurfaceAnim( md4Surface_t *surfType );
#ifdef RAVENMD4
void R_MDRAddAnimSurfaces( trRefEntity_t *ent );
void RB_MDRSurfaceAnim( md4Surface_t *surface );
#endif
qboolean R_LoadIQM (model_t *mod, void *buffer, int filesize, const char *name );
void R_AddIQMSurfaces( trRefEntity_t *ent );
void RB_IQMSurfaceAnim( surfaceType_t *surface );
int R_IQMLerpTag( orientation_t *tag, iqmData_t *data,
                  int startFrame, int endFrame,
                  float frac, const char *tagName );

/*
=============================================================

IMAGE LOADERS

=============================================================
*/

void R_LoadBMP( const char *name, byte **pic, int *width, int *height );
void R_LoadJPG( const char *name, byte **pic, int *width, int *height );
void R_LoadPCX( const char *name, byte **pic, int *width, int *height );
void R_LoadPNG( const char *name, byte **pic, int *width, int *height );
void R_LoadTGA( const char *name, byte **pic, int *width, int *height );

/*
=============================================================
=============================================================
*/
void	R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst );
void	R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void	RB_DeformTessGeometry( void );

void	RB_CalcEnvironmentTexCoords( float *dstTexCoords );
void	RB_CalcFogTexCoords( float *dstTexCoords );
void	RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
void	RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
void	RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
void	RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
void	RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );

void	RB_CalcScaleTexMatrix( const float scale[2], float *matrix );
void	RB_CalcScrollTexMatrix( const float scrollSpeed[2], float *matrix );
void	RB_CalcRotateTexMatrix( float degsPerSecond, float *matrix );
void RB_CalcTurbulentTexMatrix( const waveForm_t *wf, matrix_t matrix );
void	RB_CalcTransformTexMatrix( const texModInfo_t *tmi, float *matrix  );
void	RB_CalcStretchTexMatrix( const waveForm_t *wf, float *matrix );

void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
void	RB_CalcModulateAlphasByFog( unsigned char *dstColors );
void	RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
void	RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
float	RB_CalcWaveAlphaSingle( const waveForm_t *wf );
void	RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors );
float	RB_CalcWaveColorSingle( const waveForm_t *wf );
void	RB_CalcAlphaFromEntity( unsigned char *dstColors );
void	RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
void	RB_CalcColorFromEntity( unsigned char *dstColors );
void	RB_CalcColorFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcSpecularAlpha( unsigned char *alphas );
void	RB_CalcDiffuseColor( unsigned char *colors );

/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/

void RB_RenderThread( void );
void RB_ExecuteRenderCommands( const void *data );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct {
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct {
	int		commandId;
	int		buffer;
} drawBufferCommand_t;

typedef struct {
	int		commandId;
	image_t	*image;
	int		width;
	int		height;
	void	*data;
} subImageCommand_t;

typedef struct {
	int		commandId;
} swapBuffersCommand_t;

typedef struct {
	int		commandId;
	int		buffer;
} endFrameCommand_t;

typedef struct {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;

typedef struct {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	qboolean jpeg;
} screenshotCommand_t;

typedef struct {
	int						commandId;
	int						width;
	int						height;
	byte					*captureBuffer;
	byte					*encodeBuffer;
	qboolean			motionJpeg;
} videoFrameCommand_t;

typedef struct
{
	int commandId;

	GLboolean rgba[4];
} colorMaskCommand_t;

typedef struct
{
	int commandId;
} clearDepthCommand_t;

typedef struct {
	int commandId;
	int map;
	int cubeSide;
} capShadowmapCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
} postProcessCommand_t;

typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT,
	RC_VIDEOFRAME,
	RC_COLORMASK,
	RC_CLEARDEPTH,
	RC_CAPSHADOWMAP,
	RC_POSTPROCESS
} renderCommand_t;


// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	MAX_POLYS		600
#define	MAX_POLYVERTS	3000

// all of the information needed by the back end must be
// contained in a backEndData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine
typedef struct {
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_REFENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	pshadow_t pshadows[MAX_CALC_PSHADOWS];
	renderCommandList_t	commands;
} backEndData_t;

extern	int		max_polys;
extern	int		max_polyverts;

extern	backEndData_t	*backEndData[SMP_FRAMES];	// the second one may not be allocated

extern	volatile renderCommandList_t	*renderCommandList;

extern	volatile qboolean	renderThreadActive;


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_InitCommandBuffers( void );
void R_ShutdownCommandBuffers( void );

void R_SyncRenderThread( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );
void R_AddCapShadowmapCmd( int dlight, int cubeSide );
void R_AddPostProcessCmd (void);

void RE_SetColor( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );
void RE_SaveJPG(char * filename, int quality, int image_width, int image_height,
                unsigned char *image_buffer, int padding);
size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
		          int image_width, int image_height, byte *image_buffer, int padding);
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg );

// font stuff
void R_InitFreeType( void );
void R_DoneFreeType( void );
void RE_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);


#endif //TR_LOCAL_H
