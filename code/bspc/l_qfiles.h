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

#include "../qcommon/unzip.h"

#define QFILETYPE_UNKNOWN			0x8000
#define QFILETYPE_PAK				0x0001
#define QFILETYPE_PK3				0x0002
#define QFILETYPE_BSP				0x0004
#define QFILETYPE_MAP				0x0008
#define QFILETYPE_MDL				0x0010
#define QFILETYPE_MD2				0x0020
#define QFILETYPE_MD3				0x0040
#define QFILETYPE_WAL				0x0080
#define QFILETYPE_WAV				0x0100
#define QFILETYPE_AAS				0x4000

#define QFILEEXT_UNKNOWN			""
#define QFILEEXT_PAK				".PAK"
#define QFILEEXT_PK3				".PK3"
#define QFILEEXT_SIN				".SIN"
#define QFILEEXT_BSP				".BSP"
#define QFILEEXT_MAP				".MAP"
#define QFILEEXT_MDL				".MDL"
#define QFILEEXT_MD2				".MD2"
#define QFILEEXT_MD3				".MD3"
#define QFILEEXT_WAL				".WAL"
#define QFILEEXT_WAV				".WAV"
#define QFILEEXT_AAS				".AAS"

//maximum path length
#ifndef _MAX_PATH
	#define _MAX_PATH				1024
#endif

//for Sin packs
#define MAX_PAK_FILENAME_LENGTH 120
#define SINPAKHEADER		(('K'<<24)+('A'<<16)+('P'<<8)+'S')

typedef struct
{
	char	name[MAX_PAK_FILENAME_LENGTH];
	int	filepos, filelen;
} dsinpackfile_t;

typedef struct quakefile_s
{
	char pakfile[_MAX_PATH];
	char filename[_MAX_PATH];
	char origname[_MAX_PATH];
	int zipfile;
	int type;
	int offset;
	int length;
	unz_s zipinfo;
	struct quakefile_s *next;
} quakefile_t;

//returns the file extension for the given type
char *QuakeFileTypeExtension(int type);
//returns the file type for the given extension
int QuakeFileExtensionType(char *extension);
//return the Quake file type for the given file
int QuakeFileType(char *filename);
//returns true if the filename complies to the filter
int FileFilter(char *filter, char *filename, int casesensitive);
//find Quake files using the given filter
quakefile_t *FindQuakeFiles(char *filter);
//load the given Quake file, returns the length of the file
int LoadQuakeFile(quakefile_t *qf, void **bufferptr);
//read part of a Quake file into the buffer
int ReadQuakeFile(quakefile_t *qf, void *buffer, int offset, int length);
