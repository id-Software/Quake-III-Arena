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
#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// these are the definitions that have no dependance on
// central system services, and can be used by any part
// of the program without any state issues.

// A user mod should never modify this file

// incursion of DOOM code into the Q3A codebase
//#define	Q3_VERSION		"DOOM 0.01"

// alignment macros for SIMD
#define	ALIGN_ON
#define	ALIGN_OFF

#ifdef _WIN32

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4032)
#pragma warning(disable : 4051)
#pragma warning(disable : 4057)		// slightly different base types
#pragma warning(disable : 4100)		// unreferenced formal parameter
#pragma warning(disable : 4115)
#pragma warning(disable : 4125)		// decimal digit terminates octal escape sequence
#pragma warning(disable : 4127)		// conditional expression is constant
#pragma warning(disable : 4136)
#pragma warning(disable : 4201)
#pragma warning(disable : 4214)
#pragma warning(disable : 4244)
#pragma warning(disable : 4305)		// truncation from const double to float
#pragma warning(disable : 4310)		// cast truncates constant value
#pragma warning(disable : 4514)
#pragma warning(disable : 4711)		// selected for automatic inline expansion
#pragma warning(disable : 4220)		// varargs matches remaining parameters

#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#ifdef WIN32				// mac doesn't have malloc.h
#include <malloc.h>			// for _alloca()
#endif
#ifdef _WIN32

//#pragma intrinsic( memset, memcpy )

#endif


// this is the define for determining if we have an asm version of a C function
#if (defined _M_IX86 || defined __i386__) && !defined __sun__  && !defined __LCC__
#define id386	1
#else
#define id386	0
#endif

// for windows fastcall option

#define	QDECL

//======================= WIN32 DEFINES =================================

#ifdef WIN32

#define	MAC_STATIC

#undef QDECL
#define	QDECL	__cdecl

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define	CPUSTRING	"win-x86"
#elif defined _M_ALPHA
#define	CPUSTRING	"win-AXP"
#endif
#else
#ifdef _M_IX86
#define	CPUSTRING	"win-x86-debug"
#elif defined _M_ALPHA
#define	CPUSTRING	"win-AXP-debug"
#endif
#endif


#define	PATH_SEP '\\'

#endif

//======================= MAC OS X SERVER DEFINES =====================

#if defined(__MACH__) && defined(__APPLE__)

#define MAC_STATIC

#ifdef __ppc__
#define CPUSTRING	"MacOSXS-ppc"
#elif defined __i386__
#define CPUSTRING	"MacOSXS-i386"
#else
#define CPUSTRING	"MacOSXS-other"
#endif

#define	PATH_SEP	'/'

#define	GAME_HARD_LINKED
#define	CGAME_HARD_LINKED
#define	UI_HARD_LINKED
#define _alloca alloca

#undef ALIGN_ON
#undef ALIGN_OFF
#define	ALIGN_ON		#pragma align(16)
#define	ALIGN_OFF		#pragma align()

#ifdef __cplusplus
	extern "C" {
#endif

void *osxAllocateMemory(long size);
void osxFreeMemory(void *pointer);

#ifdef __cplusplus
        }
#endif

#endif

//======================= MAC DEFINES =================================

#ifdef __MACOS__

#define	MAC_STATIC static

#define	CPUSTRING	"MacOS-PPC"

#define	PATH_SEP ':'

void Sys_PumpEvents( void );

#endif

#ifdef __MRC__

#define	MAC_STATIC

#define	CPUSTRING	"MacOS-PPC"

#define	PATH_SEP ':'

void Sys_PumpEvents( void );

#undef QDECL
#define	QDECL	__cdecl

#define _alloca alloca
#endif

//======================= LINUX DEFINES =================================

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

// bk001205 - from Makefile
#define stricmp strcasecmp

#define	MAC_STATIC // bk: FIXME

#ifdef __i386__
#define	CPUSTRING	"linux-i386"
#elif defined __axp__
#define	CPUSTRING	"linux-alpha"
#else
#define	CPUSTRING	"linux-other"
#endif

#define	PATH_SEP '/'

// bk001205 - try
#ifdef Q3_STATIC
#define	GAME_HARD_LINKED
#define	CGAME_HARD_LINKED
#define	UI_HARD_LINKED
#define	BOTLIB_HARD_LINKED
#endif

#endif

//=============================================================


  
typedef enum {qfalse, qtrue}	qboolean;

typedef unsigned char 		byte;

#define	EQUAL_EPSILON	0.001

typedef int		qhandle_t;
typedef int		sfxHandle_t;
typedef int		fileHandle_t;
typedef int		clipHandle_t;

typedef enum {
	INVALID_JOINT = -1
} jointHandle_t;

#ifndef NULL
#define NULL ((void *)0)
#endif

#define	MAX_QINT			0x7fffffff
#define	MIN_QINT			(-MAX_QINT-1)

#ifndef max
#define max( x, y ) ( ( ( x ) > ( y ) ) ? ( x ) : ( y ) )
#define min( x, y ) ( ( ( x ) < ( y ) ) ? ( x ) : ( y ) )
#endif

#ifndef sign
#define sign( f )	( ( f > 0 ) ? 1 : ( ( f < 0 ) ? -1 : 0 ) )
#endif

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	256		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024	// max length of an individual token

#define	MAX_INFO_STRING		1024
#define	MAX_INFO_KEY		1024
#define	MAX_INFO_VALUE		1024


#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname

#define	MAX_NAME_LENGTH		32		// max length of a client name

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW,			// don't return until completed, a VM should NEVER use this,
						// because some commands might cause the VM to be unloaded...
	EXEC_INSERT,		// insert at current position, but don't run yet
	EXEC_APPEND			// add to end of the command buffer (normal case)
} cbufExec_t;


//
// these aren't needed by any of the VMs.  put in another header?
//
#define	MAX_MAP_AREA_BYTES		32		// bit vector of area visibility

#undef ERR_FATAL						// malloc.h on unix

// parameters to the main Error routine
typedef enum {
	ERR_NONE,
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_DISCONNECT,				// don't kill server
	ERR_NEED_CD					// pop up the need-cd dialog
} errorParm_t;


// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH			3
#define PROP_SPACE_WIDTH		8
#define PROP_HEIGHT				27
#define PROP_SMALL_SIZE_SCALE	0.75

#define BLINK_DIVISOR			200
#define PULSE_DIVISOR			75

#define UI_LEFT			0x00000000	// default
#define UI_CENTER		0x00000001
#define UI_RIGHT		0x00000002
#define UI_FORMATMASK	0x00000007
#define UI_SMALLFONT	0x00000010
#define UI_BIGFONT		0x00000020	// default
#define UI_GIANTFONT	0x00000040
#define UI_DROPSHADOW	0x00000800
#define UI_BLINK		0x00001000
#define UI_INVERSE		0x00002000
#define UI_PULSE		0x00004000


/*
==============================================================

MATHLIB

==============================================================
*/
#ifdef __cplusplus			// so we can include this in C code
#define	SIDE_FRONT		0
#define	SIDE_BACK		1
#define	SIDE_ON			2
#define	SIDE_CROSS		3

#define	Q_PI	3.14159265358979323846
#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#include "math_vector.h"
#include "math_angles.h"
#include "math_matrix.h"
#include "math_quaternion.h"

class idVec3_t;						// for defining vectors
typedef idVec3_t &vec3_p;				// for passing vectors as function arguments
typedef const idVec3_t &vec3_c;		// for passing vectors as const function arguments
									
class angles_t;						// for defining angle vectors
typedef angles_t &angles_p;			// for passing angles as function arguments
typedef const angles_t &angles_c;	// for passing angles as const function arguments

class mat3_t;						// for defining matrices
typedef mat3_t &mat3_p;				// for passing matrices as function arguments
typedef const mat3_t &mat3_c;		// for passing matrices as const function arguments



#define NUMVERTEXNORMALS	162
extern	idVec3_t	bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define	SCREEN_WIDTH		640
#define	SCREEN_HEIGHT		480

#define TINYCHAR_WIDTH		(SMALLCHAR_WIDTH)
#define TINYCHAR_HEIGHT		(SMALLCHAR_HEIGHT/2)

#define SMALLCHAR_WIDTH		8
#define SMALLCHAR_HEIGHT	16

#define BIGCHAR_WIDTH		16
#define BIGCHAR_HEIGHT		16

#define	GIANTCHAR_WIDTH		32
#define	GIANTCHAR_HEIGHT	48

extern	vec4_t		colorBlack;
extern	vec4_t		colorRed;
extern	vec4_t		colorGreen;
extern	vec4_t		colorBlue;
extern	vec4_t		colorYellow;
extern	vec4_t		colorMagenta;
extern	vec4_t		colorCyan;
extern	vec4_t		colorWhite;
extern	vec4_t		colorLtGrey;
extern	vec4_t		colorMdGrey;
extern	vec4_t		colorDkGrey;

#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE )

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define ColorIndex(c)	( ( (c) - '0' ) & 7 )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"

extern vec4_t	g_color_table[8];

#define	MAKERGB( v, r, g, b ) v[0]=r;v[1]=g;v[2]=b
#define	MAKERGBA( v, r, g, b, a ) v[0]=r;v[1]=g;v[2]=b;v[3]=a

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )

struct cplane_s;

extern	idVec3_t	vec3_origin;
extern	vec4_t	vec4_origin;
extern	mat3_t	axisDefault;

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

float Q_fabs( float f );
float Q_rsqrt( float f );		// reciprocal square root

#define SQRTFAST( x ) ( 1.0f / Q_rsqrt( x ) )

signed char ClampChar( int i );
signed short ClampShort( int i );

// this isn't a real cheap function to call!
int DirToByte( const idVec3_t &dir );
void ByteToDir( int b, vec3_p dir );

#define DotProduct(a,b)			((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
//#define VectorCopy(a,b)			((b).x=(a).x,(b).y=(a).y,(b).z=(a).z])

#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define CrossProduct(a,b,c)		((c)[0]=(a)[1]*(b)[2]-(a)[2]*(b)[1],(c)[1]=(a)[2]*(b)[0]-(a)[0]*(b)[2],(c)[2]=(a)[0]*(b)[1]-(a)[1]*(b)[0])

#define DotProduct4(x,y)		((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2]+(x)[3]*(y)[3])
#define VectorSubtract4(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2],(c)[3]=(a)[3]-(b)[3])
#define VectorAdd4(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2],(c)[3]=(a)[3]+(b)[3])
#define VectorCopy4(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define	VectorScale4(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s),(o)[3]=(v)[3]*(s))
#define	VectorMA4(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s),(o)[3]=(v)[3]+(b)[3]*(s))


#define VectorClear(a)			((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(a,b)		((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define VectorSet(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define Vector4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])

#define	SnapVector(v) {v[0]=(int)v[0];v[1]=(int)v[1];v[2]=(int)v[2];}

float NormalizeColor( vec3_c in, vec3_p out );

int VectorCompare( vec3_c v1, vec3_c v2 );
float VectorLength( vec3_c v );
float Distance( vec3_c p1, vec3_c p2 );
float DistanceSquared( vec3_c p1, vec3_c p2 );
float VectorNormalize (vec3_p v);		// returns vector length
void VectorNormalizeFast(vec3_p v);		// does NOT return vector length, uses rsqrt approximation
float VectorNormalize2( vec3_c v, vec3_p out );
void VectorInverse (vec3_p v);
void VectorRotate( vec3_c in, mat3_c matrix, vec3_p out );
void VectorPolar(vec3_p v, float radius, float theta, float phi);
void VectorSnap(vec3_p v);
void Vector53Copy( const idVec5_t &in, vec3_p out);
void Vector5Scale( const idVec5_t &v, float scale, idVec5_t &out);
void Vector5Add( const idVec5_t &va, const idVec5_t &vb, idVec5_t &out);
void VectorRotate3( vec3_c vIn, vec3_c vRotation, vec3_p out);
void VectorRotate3Origin(vec3_c vIn, vec3_c vRotation, vec3_c vOrigin, vec3_p out);


int Q_log2(int val);

int		Q_rand( int *seed );
float	Q_random( int *seed );
float	Q_crandom( int *seed );

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

float Q_rint( float in );

void vectoangles( vec3_c value1, angles_p angles);
void AnglesToAxis( angles_c angles, mat3_p axis );

void AxisCopy( mat3_c in, mat3_p out );
qboolean AxisRotated( mat3_c in );			// assumes a non-degenerate axis

int SignbitsForNormal( vec3_c normal );
int BoxOnPlaneSide( const Bounds &b, struct cplane_s *p );

float	AngleMod(float a);
float	LerpAngle (float from, float to, float frac);
float	AngleSubtract( float a1, float a2 );
void	AnglesSubtract( angles_c v1, angles_c v2, angles_p v3 );

float AngleNormalize360 ( float angle );
float AngleNormalize180 ( float angle );
float AngleDelta ( float angle1, float angle2 );

qboolean PlaneFromPoints( vec4_t &plane, vec3_c a, vec3_c b, vec3_c c );
void ProjectPointOnPlane( vec3_p dst, vec3_c p, vec3_c normal );
void RotatePointAroundVector( vec3_p dst, vec3_c dir, vec3_c point, float degrees );
void RotateAroundDirection( mat3_p axis, float yaw );
void MakeNormalVectors( vec3_c forward, vec3_p right, vec3_p up );
// perpendicular vector could be replaced by this

int	PlaneTypeForNormal( vec3_c normal );

void MatrixMultiply( mat3_c in1, mat3_c in2, mat3_p out );
void MatrixInverseMultiply( mat3_c in1, mat3_c in2, mat3_p out );	// in2 is transposed during multiply
void MatrixTransformVector( vec3_c in, mat3_c matrix, vec3_p out );
void MatrixProjectVector( vec3_c in, mat3_c matrix, vec3_p out ); // Places the vector into a new coordinate system.
void AngleVectors( angles_c angles, vec3_p forward, vec3_p right, vec3_p up);
void PerpendicularVector( vec3_p dst, vec3_c src );

float TriangleArea( vec3_c a, vec3_c b, vec3_c c );
#endif										// __cplusplus

//=============================================

float Com_Clamp( float min, float max, float value );

#define FILE_HASH_SIZE		1024
int Com_HashString( const char *fname );

char	*Com_SkipPath( char *pathname );

// it is ok for out == in
void	Com_StripExtension( const char *in, char *out );

// "extension" should include the dot: ".map"
void	Com_DefaultExtension( char *path, int maxSize, const char *extension );

int		Com_ParseInfos( const char *buf, int max, char infos[][MAX_INFO_STRING] );

/*
=====================================================================================

SCRIPT PARSING

=====================================================================================
*/

// this just controls the comment printing, it doesn't actually load a file
void Com_BeginParseSession( const char *filename );
void Com_EndParseSession( void );

int Com_GetCurrentParseLine( void );

// Will never return NULL, just empty strings.
// An empty string will only be returned at end of file.
// ParseOnLine will return empty if there isn't another token on this line

// this funny typedef just means a moving pointer into a const char * buffer
const char *Com_Parse( const char *(*data_p) );
const char *Com_ParseOnLine( const char *(*data_p) );
const char *Com_ParseRestOfLine( const char *(*data_p) );

void Com_UngetToken( void );

#ifdef __cplusplus
void Com_MatchToken( const char *(*buf_p), const char *match, qboolean warning = qfalse );
#else
void Com_MatchToken( const char *(*buf_p), const char *match, qboolean warning );
#endif

void Com_ScriptError( const char *msg, ... );
void Com_ScriptWarning( const char *msg, ... );

void Com_SkipBracedSection( const char *(*program) );
void Com_SkipRestOfLine( const char *(*data) );

float Com_ParseFloat( const char *(*buf_p) );
int	Com_ParseInt( const char *(*buf_p) );

void Com_Parse1DMatrix( const char *(*buf_p), int x, float *m );
void Com_Parse2DMatrix( const char *(*buf_p), int y, int x, float *m );
void Com_Parse3DMatrix( const char *(*buf_p), int z, int y, int x, float *m );

//=====================================================================================
#ifdef __cplusplus
	extern "C" {
#endif

void	QDECL Com_sprintf (char *dest, int size, const char *fmt, ...);


// mode parm for FS_FOpenFile
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

//=============================================

int Q_isprint( int c );
int Q_islower( int c );
int Q_isupper( int c );
int Q_isalpha( int c );

// portable case insensitive compare
int		Q_stricmp (const char *s1, const char *s2);
int		Q_strncmp (const char *s1, const char *s2, int n);
int		Q_stricmpn (const char *s1, const char *s2, int n);
char	*Q_strlwr( char *s1 );
char	*Q_strupr( char *s1 );
char	*Q_strrchr( const char* string, int c );

// buffer size safe library replacements
void	Q_strncpyz( char *dest, const char *src, int destsize );
void	Q_strcat( char *dest, int size, const char *src );

// strlen that discounts Quake color sequences
int Q_PrintStrlen( const char *string );
// removes color sequences from string
char *Q_CleanStr( char *string );

int			Com_Filter( const char *filter, const char *name, int casesensitive );
const char *Com_StringContains( const char *str1, const char *str2, int casesensitive );


//=============================================

short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);

void	Swap_Init (void);
char	* QDECL va(char *format, ...);

#ifdef __cplusplus
    }
#endif


//=============================================
#ifdef __cplusplus
//
// mapfile parsing
//
typedef struct ePair_s {
	char	*key;
	char	*value;
} ePair_t;

typedef struct mapSide_s {
	char		material[MAX_QPATH];
	vec4_t		plane;
	vec4_t		textureVectors[2];
} mapSide_t;

typedef struct {
	int			numSides;
	mapSide_t	**sides;
} mapBrush_t;

typedef struct {
	idVec3_t		xyz;
	float		st[2];
} patchVertex_t;

typedef struct {
	char		material[MAX_QPATH];
	int			width, height;
	patchVertex_t	*patchVerts;
} mapPatch_t;

typedef struct {
	char		modelName[MAX_QPATH];
	float		matrix[16];
} mapModel_t;

typedef struct mapPrimitive_s {
	int				numEpairs;
	ePair_t			**ePairs;

	// only one of these will be non-NULL
	mapBrush_t		*brush;
	mapPatch_t		*patch;
	mapModel_t		*model;
} mapPrimitive_t;

typedef struct mapEntity_s {
	int				numPrimitives;
	mapPrimitive_t	**primitives;

	int				numEpairs;
	ePair_t			**ePairs;
} mapEntity_t;

typedef struct {
	int				numEntities;
	mapEntity_t		**entities;
} mapFile_t;


// the order of entities, brushes, and sides will be maintained, the
// lists won't be swapped on each load or save
mapFile_t *ParseMapFile( const char *text );
void FreeMapFile( mapFile_t *mapFile );
void WriteMapFile( const mapFile_t *mapFile, FILE *f );

// key names are case-insensitive
const char 	*ValueForMapEntityKey( const mapEntity_t *ent, const char *key );
float	FloatForMapEntityKey( const mapEntity_t *ent, const char *key );
qboolean 	GetVectorForMapEntityKey( const mapEntity_t *ent, const char *key, idVec3_t &vec );

typedef struct {
	idVec3_t		xyz;
	idVec2_t		st;
	idVec3_t		normal;
	idVec3_t		tangents[2];
	byte		smoothing[4];		// colors for silhouette smoothing
} drawVert_t;

typedef struct {
	int			width, height;
	drawVert_t	*verts;
} drawVertMesh_t;

// Tesselate a map patch into smoothed, drawable vertexes
// MaxError of around 4 is reasonable
drawVertMesh_t *SubdivideMapPatch( const mapPatch_t *patch, float maxError );
#endif			// __cplusplus

//=========================================

#ifdef __cplusplus
	extern "C" {
#endif

void	QDECL Com_Error( int level, const char *error, ... );
void	QDECL Com_Printf( const char *msg, ... );
void	QDECL Com_DPrintf( const char *msg, ... );

#ifdef __cplusplus
	}
#endif


typedef struct {
	qboolean	frameMemory;
	int		currentElements;
	int		maxElements;		// will reallocate and move when exceeded
	void	**elements;
} growList_t;

// you don't need to init the growlist if you don't mind it growing and moving
// the list as it expands
void		Com_InitGrowList( growList_t *list, int maxElements );
int			Com_AddToGrowList( growList_t *list, void *data );
void		*Com_GrowListElement( const growList_t *list, int index );
int			Com_IndexForGrowListElement( const growList_t *list, const void *element );


//
// key / value info strings
//
char *Info_ValueForKey( const char *s, const char *key );
void Info_RemoveKey( char *s, const char *key );
void Info_SetValueForKey( char *s, const char *key, const char *value );
qboolean Info_Validate( const char *s );
void Info_NextPair( const char *(*s), char key[MAX_INFO_KEY], char value[MAX_INFO_VALUE] );

// get cvar defs, collision defs, etc
//#include "../shared/interface.h"

// get key code numbers for events
//#include "../shared/keycodes.h"

#ifdef __cplusplus
// get the polygon winding functions
//#include "../shared/windings.h"

// get the flags class
//#include "../shared/idflags.h"
#endif	// __cplusplus

#endif	// __Q_SHARED_H

