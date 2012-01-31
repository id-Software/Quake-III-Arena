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
// scriplib.h

#ifndef __CMDLIB__
#include "cmdlib.h"
#endif
#ifndef __MATHLIB__
#include "mathlib.h"
#endif

#define	MAXTOKEN	1024

extern	char	token[MAXTOKEN];
extern	char	*scriptbuffer,*script_p,*scriptend_p;
extern	int		grabbed;
extern	int		scriptline;
extern	qboolean	endofscript;


void LoadScriptFile( const char *filename );
void ParseFromMemory (char *buffer, int size);

qboolean GetToken (qboolean crossline);
void UnGetToken (void);
qboolean TokenAvailable (void);

void MatchToken( char *match );

void Parse1DMatrix (int x, vec_t *m);
void Parse2DMatrix (int y, int x, vec_t *m);
void Parse3DMatrix (int z, int y, int x, vec_t *m);

void Write1DMatrix (FILE *f, int x, vec_t *m);
void Write2DMatrix (FILE *f, int y, int x, vec_t *m);
void Write3DMatrix (FILE *f, int z, int y, int x, vec_t *m);
