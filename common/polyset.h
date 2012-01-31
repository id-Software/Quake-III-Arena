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
#ifndef __POLYSET_H__
#define __POLYSET_H__

#define POLYSET_MAXTRIANGLES	4096
#define POLYSET_MAXPOLYSETS		64

typedef float st_t[2];
typedef float rgb_t[3];

typedef struct {
	vec3_t	verts[3];
	vec3_t	normals[3];
	st_t	texcoords[3];
} triangle_t;

typedef struct
{
	char name[100];
	char materialname[100];
	triangle_t *triangles;
	int numtriangles;
} polyset_t;

polyset_t *Polyset_LoadSets( const char *file, int *numpolysets, int maxTrisPerSet );
polyset_t *Polyset_CollapseSets( polyset_t *psets, int numpolysets );
polyset_t *Polyset_SplitSets( polyset_t *psets, int numpolysets, int *pNumNewPolysets, int maxTris );
void Polyset_SnapSets( polyset_t *psets, int numpolysets );
void Polyset_ComputeNormals( polyset_t *psets, int numpolysets );

#endif
