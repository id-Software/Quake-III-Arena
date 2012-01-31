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
#include "qbsp.h"


void PrintCtrl( vec3_t ctrl[9] ) {
	int		i, j;

	for ( i = 0 ; i < 3 ; i++ ) {
		for ( j = 0 ; j < 3 ; j++ ) {
			_printf("(%5.2f %5.2f %5.2f) ", ctrl[i*3+j][0], ctrl[i*3+j][1], ctrl[i*3+j][2] );
		}
		_printf("\n");
	}
}

/*
================
DrawSurfaceForMesh
================
*/
mapDrawSurface_t	*DrawSurfaceForMesh( mesh_t *m ) {
	mapDrawSurface_t	*ds;
	int				i, j;
	mesh_t			*copy;

	// to make valid normals for patches with degenerate edges,
	// we need to make a copy of the mesh and put the aproximating
	// points onto the curve
	copy = CopyMesh( m );
	PutMeshOnCurve( *copy );
	MakeMeshNormals( *copy );
	for ( j = 0 ; j < m->width ; j++ ) {
		for ( i = 0 ; i < m->height ; i++ ) {
			VectorCopy( copy->verts[i*m->width+j].normal, m->verts[i*m->width+j].normal );
		}
	}
	FreeMesh( copy );

	ds = AllocDrawSurf();
	ds->mapBrush = NULL;
	ds->side = NULL;

	ds->patch = qtrue;
	ds->patchWidth = m->width;
	ds->patchHeight = m->height;
	ds->numVerts = ds->patchWidth * ds->patchHeight;
	ds->verts = malloc( ds->numVerts * sizeof( *ds->verts ) );
	memcpy( ds->verts, m->verts, ds->numVerts * sizeof( *ds->verts ) );

	ds->lightmapNum = -1;
	ds->fogNum = -1;

	return ds;
}

/*
=================
ParsePatch

Creates a mapDrawSurface_t from the patch text
=================
*/
void ParsePatch( void ) {
	vec_t		info[5];
	int			i, j;
	parseMesh_t	*pm;
	char		texture[MAX_QPATH];
	char		shader[MAX_QPATH];
	mesh_t		m;
	drawVert_t	*verts;
  epair_t *ep;

	MatchToken( "{" );

	// get texture
	GetToken (qtrue);
	strcpy( texture, token );

	// save the shader name for retexturing
	if ( numMapIndexedShaders == MAX_MAP_BRUSHSIDES ) {
		Error( "MAX_MAP_BRUSHSIDES" );
	}
	strcpy( mapIndexedShaders[numMapIndexedShaders], texture );
	numMapIndexedShaders++;


	Parse1DMatrix( 5, info );
	m.width = info[0];
	m.height = info[1];
	m.verts = verts = malloc( m.width * m.height * sizeof( m.verts[0] ) );

	if ( m.width < 0 || m.width > MAX_PATCH_SIZE
		|| m.height < 0 || m.height > MAX_PATCH_SIZE ) {
		Error("ParsePatch: bad size");
	}

	MatchToken( "(" );
	for ( j = 0 ; j < m.width ; j++ ) {
		MatchToken( "(" );
		for ( i = 0 ; i < m.height ; i++ ) {
			Parse1DMatrix( 5, verts[i*m.width+j].xyz );
		}
		MatchToken( ")" );
	}
	MatchToken( ")" );

  // if brush primitives format, we may have some epairs to ignore here
  GetToken(qtrue);
  if (g_bBrushPrimit!=BPRIMIT_OLDBRUSHES && strcmp(token,"}"))
  {
    // NOTE: we leak that!
    ep = ParseEpair();
  }
  else
    UnGetToken();

	MatchToken( "}" );
	MatchToken( "}" );

	if ( noCurveBrushes ) {
		return;
	}

	// find default flags and values
	pm = malloc( sizeof( *pm ) );
	memset( pm, 0, sizeof( *pm ) );

	sprintf( shader, "textures/%s", texture );
	pm->shaderInfo = ShaderInfoForShader( shader ); 
	pm->mesh = m;

	// link to the entity
	pm->next = mapent->patches;
	mapent->patches = pm;
}


void GrowGroup_r( int patchNum, int patchCount, const byte *bordering, byte *group ) {
	int		i;
	const byte *row;

	if ( group[patchNum] ) {
		return;
	}
	group[patchNum] = 1;
	row = bordering + patchNum * patchCount;
	for ( i = 0 ; i < patchCount ; i++ ) {
		if ( row[i] ) {
			GrowGroup_r( i, patchCount, bordering, group );
		}
	}
}


/*
=====================
PatchMapDrawSurfs

Any patches that share an edge need to choose their
level of detail as a unit, otherwise the edges would
pull apart.
=====================
*/
void PatchMapDrawSurfs( entity_t *e ) {
	parseMesh_t			*pm;
	parseMesh_t			*check, *scan;
	mapDrawSurface_t	*ds;
	int					patchCount, groupCount;
	int					i, j, k, l, c1, c2;
	drawVert_t			*v1, *v2;
	vec3_t				bounds[2];
	byte				*bordering;
	parseMesh_t			*meshes[MAX_MAP_DRAW_SURFS];
	qboolean			grouped[MAX_MAP_DRAW_SURFS];
	byte				group[MAX_MAP_DRAW_SURFS];

	qprintf( "----- PatchMapDrawSurfs -----\n" );

	patchCount = 0;
	for ( pm = e->patches ; pm ; pm = pm->next  ) {
		meshes[patchCount] = pm;
		patchCount++;
	}

	if ( !patchCount ) {
		return;
	}
	bordering = malloc( patchCount * patchCount );
	memset( bordering, 0, patchCount * patchCount );

	// build the bordering matrix
	for ( k = 0 ; k < patchCount ; k++ ) {
		bordering[k*patchCount+k] = 1;

		for ( l = k+1 ; l < patchCount ; l++ ) {
			check = meshes[k];
			scan = meshes[l];
			c1 = scan->mesh.width * scan->mesh.height;
			v1 = scan->mesh.verts;

			for ( i = 0 ; i < c1 ; i++, v1++ ) {
				c2 = check->mesh.width * check->mesh.height;
				v2 = check->mesh.verts;
				for ( j = 0 ; j < c2 ; j++, v2++ ) {
					if ( fabs( v1->xyz[0] - v2->xyz[0] ) < 1.0
						&& fabs( v1->xyz[1] - v2->xyz[1] ) < 1.0
						&& fabs( v1->xyz[2] - v2->xyz[2] ) < 1.0 ) {
						break;
					}
				}
				if ( j != c2 ) {
					break;
				}
			}
			if ( i != c1 ) {
				// we have a connection
				bordering[k*patchCount+l] =
				bordering[l*patchCount+k] = 1;
			} else {
				// no connection
				bordering[k*patchCount+l] =
				bordering[l*patchCount+k] = 0;
			}

		}
	}

	// build groups
	memset( grouped, 0, sizeof(grouped) );
	groupCount = 0;
	for ( i = 0 ; i < patchCount ; i++ ) {
		if ( !grouped[i] ) {
			groupCount++;
		}

		// recursively find all patches that belong in the same group
		memset( group, 0, patchCount );
		GrowGroup_r( i, patchCount, bordering, group );

		// bound them
		ClearBounds( bounds[0], bounds[1] );
		for ( j = 0 ; j < patchCount ; j++ ) {
			if ( group[j] ) {
				grouped[j] = qtrue;
				scan = meshes[j];
				c1 = scan->mesh.width * scan->mesh.height;
				v1 = scan->mesh.verts;
				for ( k = 0 ; k < c1 ; k++, v1++ ) {
					AddPointToBounds( v1->xyz, bounds[0], bounds[1] );
				}
			}
		}

		// create drawsurf
		scan = meshes[i];
		scan->grouped = qtrue;
		ds = DrawSurfaceForMesh( &scan->mesh );
		ds->shaderInfo = scan->shaderInfo;
		VectorCopy( bounds[0], ds->lightmapVecs[0] );
		VectorCopy( bounds[1], ds->lightmapVecs[1] );
	}

	qprintf( "%5i patches\n", patchCount );
	qprintf( "%5i patch LOD groups\n", groupCount );
}

