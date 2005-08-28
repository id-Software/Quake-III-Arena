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

/*****************************************************************************
 * name:		l_struct.h
 *
 * desc:		structure reading/writing
 *
 * $Archive: /source/code/botlib/l_struct.h $
 *
 *****************************************************************************/


#define MAX_STRINGFIELD				80
//field types
#define FT_CHAR						1			// char
#define FT_INT							2			// int
#define FT_FLOAT						3			// float
#define FT_STRING						4			// char [MAX_STRINGFIELD]
#define FT_STRUCT						6			// struct (sub structure)
//type only mask
#define FT_TYPE						0x00FF	// only type, clear subtype
//sub types
#define FT_ARRAY						0x0100	// array of type
#define FT_BOUNDED					0x0200	// bounded value
#define FT_UNSIGNED					0x0400

//structure field definition
typedef struct fielddef_s
{
	char *name;										//name of the field
	int offset;										//offset in the structure
	int type;										//type of the field
	//type specific fields
	int maxarray;									//maximum array size
	float floatmin, floatmax;					//float min and max
	struct structdef_s *substruct;			//sub structure
} fielddef_t;

//structure definition
typedef struct structdef_s
{
	int size;
	fielddef_t *fields;
} structdef_t;

//read a structure from a script
int ReadStructure(source_t *source, structdef_t *def, char *structure);
//write a structure to a file
int WriteStructure(FILE *fp, structdef_t *def, char *structure);
//writes indents
int WriteIndent(FILE *fp, int indent);
//writes a float without traling zeros
int WriteFloat(FILE *fp, float value);


