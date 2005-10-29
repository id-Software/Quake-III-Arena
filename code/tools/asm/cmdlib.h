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
// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#ifdef _MSC_VER
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)     // truncate from double to float

#pragma check_stack(off)

#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>

#ifdef _MSC_VER

#pragma intrinsic( memset, memcpy )

#endif

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
typedef enum { qfalse, qtrue } qboolean;
typedef unsigned char byte;
#endif

#define	MAX_OS_PATH		1024
#define MEM_BLOCKSIZE 4096

// the dec offsetof macro doesnt work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)


// set these before calling CheckParm
extern int myargc;
extern char **myargv;

char *strupr (char *in);
char *strlower (char *in);
int Q_strncasecmp( const char *s1, const char *s2, int n );
int Q_stricmp( const char *s1, const char *s2 );
void Q_getwd( char *out );

int Q_filelength (FILE *f);
int	FileTime( const char *path );

void	Q_mkdir( const char *path );

extern	char		qdir[1024];
extern	char		gamedir[1024];
extern  char		writedir[1024];
void SetQdirFromPath( const char *path );
char *ExpandArg( const char *path );	// from cmd line
char *ExpandPath( const char *path );	// from scripts
char *ExpandGamePath (const char *path);
char *ExpandPathAndArchive( const char *path );


double I_FloatTime( void );

void	Error( const char *error, ... );
int		CheckParm( const char *check );

FILE	*SafeOpenWrite( const char *filename );
FILE	*SafeOpenRead( const char *filename );
void	SafeRead (FILE *f, void *buffer, int count);
void	SafeWrite (FILE *f, const void *buffer, int count);

int		LoadFile( const char *filename, void **bufferptr );
int   LoadFileBlock( const char *filename, void **bufferptr );
int		TryLoadFile( const char *filename, void **bufferptr );
void	SaveFile( const char *filename, const void *buffer, int count );
qboolean	FileExists( const char *filename );

void 	DefaultExtension( char *path, const char *extension );
void 	DefaultPath( char *path, const char *basepath );
void 	StripFilename( char *path );
void 	StripExtension( char *path );

void 	ExtractFilePath( const char *path, char *dest );
void 	ExtractFileBase( const char *path, char *dest );
void	ExtractFileExtension( const char *path, char *dest );

int 	ParseNum (const char *str);

short	BigShort (short l);
short	LittleShort (short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);


char *COM_Parse (char *data);

extern	char		com_token[1024];
extern	qboolean	com_eof;

char *copystring(const char *s);


void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);

void	CreatePath( const char *path );
void	QCopyFile( const char *from, const char *to );

extern	qboolean		archive;
extern	char			archivedir[1024];


extern	qboolean verbose;
void qprintf( const char *format, ... );
void _printf( const char *format, ... );

void ExpandWildcards( int *argc, char ***argv );


// for compression routines
typedef struct
{
	void	*data;
	int		count, width, height;
} cblock_t;


#endif
