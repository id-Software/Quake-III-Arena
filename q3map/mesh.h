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

// mesh.h


typedef struct {
	int			width, height;
	drawVert_t	*verts;
} mesh_t;

#define	MAX_EXPANDED_AXIS		128

extern	int	originalWidths[MAX_EXPANDED_AXIS];
extern	int	originalHeights[MAX_EXPANDED_AXIS];

void FreeMesh( mesh_t *m );
mesh_t *CopyMesh( mesh_t *mesh );
void PrintMesh( mesh_t *m );
mesh_t *TransposeMesh( mesh_t *in );
void InvertMesh( mesh_t *m );
mesh_t *SubdivideMesh( mesh_t in, float maxError, float minLength );
mesh_t *SubdivideMeshQuads( mesh_t *in, float minLength, int maxsize, int widthtable[], int heighttable[]);
mesh_t *RemoveLinearMeshColumnsRows( mesh_t *in );
void MakeMeshNormals( mesh_t in );
void PutMeshOnCurve( mesh_t in );


void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up);
