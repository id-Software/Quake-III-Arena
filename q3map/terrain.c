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
#include <assert.h>

#define SURF_WIDTH	2048
#define SURF_HEIGHT 2048

#define GROW_VERTS		512
#define GROW_INDICES	512
#define GROW_SURFACES	128

#define VectorSet(v, x, y, z)		v[0] = x;v[1] = y;v[2] = z;

void QuakeTextureVecs( 	plane_t *plane, vec_t shift[2], vec_t rotate, vec_t scale[2], vec_t mappingVecs[2][4] );

typedef struct {
	shaderInfo_t	*shader;
	int				x, y;

	int				maxVerts;
	int				numVerts;
	drawVert_t		*verts;

	int				maxIndexes;
	int				numIndexes;
	int				*indexes;
} terrainSurf_t;

static terrainSurf_t	*surfaces = NULL;
static terrainSurf_t	*lastSurface = NULL;
static int				numsurfaces = 0;
static int				maxsurfaces = 0;

/*
================
ShaderForLayer
================
*/
shaderInfo_t *ShaderForLayer( int minlayer, int maxlayer, const char *shadername ) {
	char	shader[ MAX_QPATH ];

	if ( minlayer == maxlayer ) {
		sprintf( shader, "textures/%s_%d", shadername, maxlayer );
	} else {
		sprintf( shader, "textures/%s_%dto%d", shadername, minlayer, maxlayer );
	}

	return ShaderInfoForShader( shader );
}

/*
================
CompareVert
================
*/
qboolean CompareVert( drawVert_t *v1, drawVert_t *v2, qboolean checkst ) {
	int i;

	for( i = 0; i < 3; i++ ) {
		if ( floor( v1->xyz[ i ] + 0.1 ) != floor( v2->xyz[ i ] + 0.1 ) ) {
			return qfalse;
		}
		if ( checkst && ( ( v1->st[ 0 ] != v2->st[ 0 ] ) || ( v1->st[ 1 ] != v2->st[ 1 ] ) ) ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
================
LoadAlphaMap
================
*/
byte *LoadAlphaMap( int *num_layers, int *alphawidth, int *alphaheight ) {
	int			*alphamap32;
	byte		*alphamap;
	const char	*alphamapname;
	char		ext[ 128 ];
	int			width;
	int			height;
	int			layers;
	int			size;
	int			i;

	assert( alphawidth );
	assert( alphaheight );
	assert( num_layers );

	layers = atoi( ValueForKey( mapent, "layers" ) );
	if ( layers < 1 ) {
		Error ("SetTerrainTextures: invalid value for 'layers' (%d)", layers );
	}

	alphamapname = ValueForKey( mapent, "alphamap" );
	if ( !alphamapname[ 0 ] ) {
		Error ("LoadAlphaMap: No alphamap specified on terrain" );
	}

	ExtractFileExtension( alphamapname, ext);
	if ( !Q_stricmp( ext, "tga" ) ) {
		Load32BitImage( ExpandGamePath( alphamapname ), &alphamap32, &width, &height );

		size = width * height;
		alphamap = malloc( size );
		for( i = 0; i < size; i++ ) {
			alphamap[ i ] = ( ( alphamap32[ i ] & 0xff ) * layers ) / 256;
			if ( alphamap[ i ] >= layers ) {
				alphamap[ i ] = layers - 1;
			}
		}
	} else {
		Load256Image( ExpandGamePath( alphamapname ), &alphamap, NULL, &width, &height );
		size = width * height;
		for( i = 0; i < size; i++ ) {
			if ( alphamap[ i ] >= layers ) {
				alphamap[ i ] = layers - 1;
			}
		}
	}

	if ( ( width < 2 ) || ( height < 2 ) ) {
		Error ("LoadAlphaMap: alphamap width/height must be at least 2x2." );
	}

	*num_layers		= layers;
	*alphawidth		= width;
	*alphaheight	= height;

	return alphamap;
}

/*
================
CalcTerrainSize
================
*/
void CalcTerrainSize( vec3_t mins, vec3_t maxs, vec3_t size ) {
	bspbrush_t	*brush;
	int			i;
	const char  *key;

	// calculate the size of the terrain
	ClearBounds( mins, maxs );
	for( brush = mapent->brushes; brush != NULL; brush = brush->next ) {
		AddPointToBounds( brush->mins, mins, maxs );
		AddPointToBounds( brush->maxs, mins, maxs );
	}

	key = ValueForKey( mapent, "min" ); 
	if ( key[ 0 ] ) {
		GetVectorForKey( mapent, "min", mins );
	}

	key = ValueForKey( mapent, "max" ); 
	if ( key[ 0 ] ) {
		GetVectorForKey( mapent, "max", maxs );
	}

	for( i = 0; i < 3; i++ ) {
		mins[ i ] =  floor( mins[ i ] + 0.1 );
		maxs[ i ] =  floor( maxs[ i ] + 0.1 );
	}

	VectorSubtract( maxs, mins, size );

	if ( ( size[ 0 ] <= 0 ) || ( size[ 1 ] <= 0 ) ) {
		Error ("CalcTerrainSize: Invalid terrain size: %fx%f", size[ 0 ], size[ 1 ] );
	}
}

/*
==================
IsTriangleDegenerate

Returns qtrue if all three points are collinear or backwards
===================
*/
#define	COLINEAR_AREA	10
static qboolean	IsTriangleDegenerate( drawVert_t *points, int a, int b, int c ) {
	vec3_t		v1, v2, v3;
	float		d;

	VectorSubtract( points[b].xyz, points[a].xyz, v1 );
	VectorSubtract( points[c].xyz, points[a].xyz, v2 );
	CrossProduct( v1, v2, v3 );
	d = VectorLength( v3 );

	// assume all very small or backwards triangles will cause problems
	if ( d < COLINEAR_AREA ) {
		return qtrue;
	}

	return qfalse;
}

/*
===============
SideAsTriFan

The surface can't be represented as a single tristrip without
leaving a degenerate triangle (and therefore a crack), so add
a point in the middle and create (points-1) triangles in fan order
===============
*/
static void SideAsTriFan( terrainSurf_t *surf, int *index, int num ) {
	int					i;
	int					colorSum[4];
	drawVert_t			*mid, *v;

	// make sure we have enough space for a new vert
	if ( surf->numVerts >= surf->maxVerts ) {
		surf->maxVerts += GROW_VERTS;
		surf->verts = realloc( surf->verts, surf->maxVerts * sizeof( *surf->verts ) );
	}

	// create a new point in the center of the face
	mid = &surf->verts[ surf->numVerts ];
	surf->numVerts++;

	colorSum[0] = colorSum[1] = colorSum[2] = colorSum[3] = 0;

	for (i = 0 ; i < num; i++ ) {
		v = &surf->verts[ index[ i ] ];
		VectorAdd( mid->xyz, v->xyz, mid->xyz );
		mid->st[0] += v->st[0];
		mid->st[1] += v->st[1];
		mid->lightmap[0] += v->lightmap[0];
		mid->lightmap[1] += v->lightmap[1];

		colorSum[0] += v->color[0];
		colorSum[1] += v->color[1];
		colorSum[2] += v->color[2];
		colorSum[3] += v->color[3];
	}

	mid->xyz[0] /= num;
	mid->xyz[1] /= num;
	mid->xyz[2] /= num;

	mid->st[0] /= num;
	mid->st[1] /= num;

	mid->lightmap[0] /= num;
	mid->lightmap[1] /= num;

	mid->color[0] = colorSum[0] / num;
	mid->color[1] = colorSum[1] / num;
	mid->color[2] = colorSum[2] / num;
	mid->color[3] = colorSum[3] / num;

	// fill in indices in trifan order
	if ( surf->numIndexes + num * 3 > surf->maxIndexes ) {
		surf->maxIndexes = surf->numIndexes + num * 3;
		surf->indexes = realloc( surf->indexes, surf->maxIndexes * sizeof( *surf->indexes ) );
	}


	for ( i = 0 ; i < num; i++ ) {
		surf->indexes[ surf->numIndexes++ ] = surf->numVerts - 1;
		surf->indexes[ surf->numIndexes++ ] = index[ i ];
		surf->indexes[ surf->numIndexes++ ] = index[ (i+1) % ( surf->numVerts - 1 ) ];
	}
}
/*
================
SideAsTristrip

Try to create indices that make (points-2) triangles in tristrip order
================
*/
#define	MAX_INDICES	1024
static void SideAsTristrip( terrainSurf_t *surf, int *index, int num ) {
	int					i;
	int					rotate;
	int					numIndices;
	int					ni;
	int					a, b, c;
	int					indices[ MAX_INDICES ];

	// determine the triangle strip order
	numIndices = ( num - 2 ) * 3;
	if ( numIndices > MAX_INDICES ) {
		Error( "MAX_INDICES exceeded for surface" );
	}

	// try all possible orderings of the points looking
	// for a strip order that isn't degenerate
	for ( rotate = 0 ; rotate < num; rotate++ ) {
		for ( ni = 0, i = 0 ; i < num - 2 - i ; i++ ) {
			a = index[ ( num - 1 - i + rotate ) % num ];
			b = index[ ( i + rotate ) % num ];
			c = index[ ( num - 2 - i + rotate ) % num ];

			if ( IsTriangleDegenerate( surf->verts, a, b, c ) ) {
				break;
			}
			indices[ni++] = a;
			indices[ni++] = b;
			indices[ni++] = c;

			if ( i + 1 != num - 1 - i ) {
				a = index[ ( num - 2 - i + rotate ) % num ];
				b = index[ ( i + rotate ) % num ];
				c = index[ ( i + 1 + rotate ) % num ];

				if ( IsTriangleDegenerate( surf->verts, a, b, c ) ) {
					break;
				}
				indices[ni++] = a;
				indices[ni++] = b;
				indices[ni++] = c;
			}
		}
		if ( ni == numIndices ) {
			break;		// got it done without degenerate triangles
		}
	}

	// if any triangle in the strip is degenerate,
	// render from a centered fan point instead
	if ( ni < numIndices ) {
		SideAsTriFan( surf, index, num );
		return;
	}

	// a normal tristrip
	if ( surf->numIndexes + ni > surf->maxIndexes ) {
		surf->maxIndexes = surf->numIndexes + ni;
		surf->indexes = realloc( surf->indexes, surf->maxIndexes * sizeof( *surf->indexes ) );
	}

	memcpy( surf->indexes + surf->numIndexes, indices, ni * sizeof( *surf->indexes ) );
	surf->numIndexes += ni;
}

/*
================
CreateTerrainSurface
================
*/
void CreateTerrainSurface( terrainSurf_t *surf, shaderInfo_t *shader ) {
	int					i, j, k;
	drawVert_t			*out;
	drawVert_t			*in;
	mapDrawSurface_t	*newsurf;

	newsurf = AllocDrawSurf();

	newsurf->miscModel		= qtrue;
	newsurf->shaderInfo		= shader;
	newsurf->lightmapNum	= -1;
	newsurf->fogNum			= -1;
	newsurf->numIndexes		= surf->numIndexes;
	newsurf->numVerts		= surf->numVerts;

	// copy the indices
	newsurf->indexes = malloc( surf->numIndexes * sizeof( *newsurf->indexes ) );
	memcpy( newsurf->indexes, surf->indexes, surf->numIndexes * sizeof( *newsurf->indexes ) );

	// allocate the vertices
	newsurf->verts = malloc( surf->numVerts * sizeof( *newsurf->verts ) );
	memset( newsurf->verts, 0, surf->numVerts * sizeof( *newsurf->verts ) );

	// calculate the surface verts
	out = newsurf->verts;
	for( i = 0; i < newsurf->numVerts; i++, out++ ) {
		VectorCopy( surf->verts[ i ].xyz, out->xyz );

		// set the texture coordinates
		out->st[ 0 ] = surf->verts[ i ].st[ 0 ];
		out->st[ 1 ] = surf->verts[ i ].st[ 1 ];

		// the colors will be set by the lighting pass
		out->color[0] = 255;
		out->color[1] = 255;
		out->color[2] = 255;
		out->color[3] = surf->verts[ i ].color[ 3 ];

		// calculate the vertex normal
		VectorClear( out->normal );
		for( j = 0; j < numsurfaces; j++ ) {
			in = surfaces[ j ].verts;
			for( k = 0; k < surfaces[ j ].numVerts; k++, in++ ) {
				if ( CompareVert( out, in, qfalse ) ) {
					VectorAdd( out->normal, in->normal, out->normal );
				}
			}
		}

		VectorNormalize( out->normal, out->normal );
	}
}

/*
================
EmitTerrainVerts
================
*/
void EmitTerrainVerts( side_t *side, terrainSurf_t *surf, int maxlayer, int alpha[ MAX_POINTS_ON_WINDING ], qboolean projecttexture ) {
	int			i;
	int			j;
	drawVert_t	*vert;
	int			*indices;
	int			numindices;
	int			maxindices;
	int			xyplane;
	vec3_t		xynorm = { 0, 0, 1 };
	vec_t		shift[ 2 ] = { 0, 0 };
	vec_t		scale[ 2 ] = { 0.5, 0.5 };
	float		vecs[ 2 ][ 4 ];
	static int numtimes = 0;

	numtimes++;

	if ( !surf->verts ) {
		surf->numVerts		= 0;
		surf->maxVerts		= GROW_VERTS;
		surf->verts			= malloc( surf->maxVerts * sizeof( *surf->verts ) );

		surf->numIndexes	= 0;
		surf->maxIndexes	= GROW_INDICES;
		surf->indexes		= malloc( surf->maxIndexes * sizeof( *surf->indexes ) );
	}

	// calculate the texture coordinate vectors
	xyplane = FindFloatPlane( xynorm, 0 );
	QuakeTextureVecs( &mapplanes[ xyplane ], shift, 0, scale, vecs );

	// emit the vertexes
	numindices = 0;
	maxindices = surf->maxIndexes;
	indices = malloc ( maxindices * sizeof( *indices ) );

	for ( i = 0; i < side->winding->numpoints; i++ ) {
		vert = &surf->verts[ surf->numVerts ];

		// set the final alpha value--0 for texture 1, 255 for texture 2
		if ( alpha[ i ] < maxlayer ) {
			vert->color[3] = 0;
		} else {
			vert->color[3] = 255;
		}

		vert->xyz[ 0 ] = floor( side->winding->p[ i ][ 0 ] + 0.1f );
		vert->xyz[ 1 ] = floor( side->winding->p[ i ][ 1 ] + 0.1f );
		vert->xyz[ 2 ] = floor( side->winding->p[ i ][ 2 ] + 0.1f );

		// set the texture coordinates
		if ( projecttexture ) {
			vert->st[0] = ( vecs[0][3] + DotProduct( vecs[ 0 ], vert->xyz ) ) / surf->shader->width;
			vert->st[1] = ( vecs[1][3] + DotProduct( vecs[ 1 ], vert->xyz ) ) / surf->shader->height;
		} else {
			vert->st[0] = ( side->vecs[0][3] + DotProduct( side->vecs[ 0 ], vert->xyz ) ) / surf->shader->width;
			vert->st[1] = ( side->vecs[1][3] + DotProduct( side->vecs[ 1 ], vert->xyz ) ) / surf->shader->height;
		}

		VectorCopy( mapplanes[ side->planenum ].normal, vert->normal );

		for( j = 0; j < surf->numVerts; j++ ) {
			if ( CompareVert( vert, &surf->verts[ j ], qtrue ) ) {
				break;
			}
		}
		
		if ( numindices >= maxindices ) {
			maxindices += GROW_INDICES;
			indices = realloc( indices, maxindices * sizeof( *indices ) );
		}

		if ( j != surf->numVerts ) {
			indices[ numindices++ ] = j;
		} else {
			indices[ numindices++ ] = surf->numVerts;
			surf->numVerts++;
			if ( surf->numVerts >= surf->maxVerts ) {
				surf->maxVerts += GROW_VERTS;
				surf->verts = realloc( surf->verts, surf->maxVerts * sizeof( *surf->verts ) );
			}
		}
	}

	SideAsTristrip( surf, indices, numindices );

	free( indices );
}

/*
================
SurfaceForShader
================
*/
terrainSurf_t *SurfaceForShader( shaderInfo_t *shader, int x, int y ) {
	int i;

	if ( lastSurface && ( lastSurface->shader == shader ) && ( lastSurface->x == x ) && ( lastSurface->y == y ) ) {
		return lastSurface;
	}

	lastSurface = surfaces;
	for( i = 0; i < numsurfaces; i++, lastSurface++ ) {
		if ( ( lastSurface->shader == shader ) && ( lastSurface->x == x ) && ( lastSurface->y == y ) ) {
			return lastSurface;
		}
	}

	if ( numsurfaces >= maxsurfaces ) {
		maxsurfaces += GROW_SURFACES;
		surfaces = realloc( surfaces, maxsurfaces * sizeof( *surfaces ) );
		memset( surfaces + numsurfaces + 1, 0, ( maxsurfaces - numsurfaces - 1 ) * sizeof( *surfaces ) );
	}

	lastSurface= &surfaces[ numsurfaces++ ];
	lastSurface->shader = shader;
	lastSurface->x = x;
	lastSurface->y = y;

	return lastSurface;
}

/*
================
SetTerrainTextures
================
*/
void SetTerrainTextures( void ) {
	int				i;
	int				x, y;
	int				layer;
	int				minlayer, maxlayer;
	float			s, t;
	float			min_s, min_t;
	int				alpha[ MAX_POINTS_ON_WINDING ];
	shaderInfo_t	*si, *terrainShader;
	bspbrush_t		*brush;
	side_t			*side;
	const char		*shadername;
	vec3_t			mins, maxs;
	vec3_t			size;
	int				surfwidth, surfheight, surfsize;
	terrainSurf_t	*surf;
	byte			*alphamap;
	int				alphawidth, alphaheight;
	int				num_layers;
	extern qboolean	onlyents;

	if ( onlyents ) {
		return;
	}

	shadername = ValueForKey( mapent, "shader" );
	if ( !shadername[ 0 ] ) {
		Error ("SetTerrainTextures: shader not specified" );
	}

	alphamap = LoadAlphaMap( &num_layers, &alphawidth, &alphaheight );

	mapent->firstDrawSurf = numMapDrawSurfs;

	// calculate the size of the terrain
	CalcTerrainSize( mins, maxs, size );

	surfwidth	= ( size[ 0 ] + SURF_WIDTH - 1 ) / SURF_WIDTH;
	surfheight	= ( size[ 1 ] + SURF_HEIGHT - 1 ) / SURF_HEIGHT;
	surfsize = surfwidth * surfheight;

	lastSurface = NULL;
	numsurfaces = 0;
	maxsurfaces = 0;
	for( i = num_layers; i > 0; i-- ) {
		maxsurfaces += i * surfsize;
	}
	surfaces = malloc( maxsurfaces * sizeof( *surfaces ) );
	memset( surfaces, 0, maxsurfaces * sizeof( *surfaces ) );

	terrainShader = ShaderInfoForShader( "textures/common/terrain" );

	for( brush = mapent->brushes; brush != NULL; brush = brush->next ) {
		// only create surfaces for sides marked as terrain
		for( side = brush->sides; side < &brush->sides[ brush->numsides ]; side++ ) {
			if ( !side->shaderInfo ) {
				continue;
			}

			if ( ( ( side->surfaceFlags | side->shaderInfo->surfaceFlags ) & SURF_NODRAW ) && !strstr( side->shaderInfo->shader, "terrain" ) ) {
				continue;
			}

			minlayer = num_layers;
			maxlayer = 0;

			// project each point of the winding onto the alphamap to determine which
			// textures to blend
			min_s = 1.0;
			min_t = 1.0;
			for( i = 0; i < side->winding->numpoints; i++ ) {
				s = floor( side->winding->p[ i ][ 0 ] + 0.1f - mins[ 0 ] ) / size[ 0 ];
				t = floor( side->winding->p[ i ][ 1 ] + 0.1f - mins[ 0 ] ) / size[ 1 ];

				if ( s < 0 ) {
					s = 0;
				}
				
				if ( t < 0 ) {
					t = 0;
				}

				if ( s >= 1.0 ) {
					s = 1.0;
				}

				if ( t >= 1.0 ) {
					t = 1.0;
				}

				if ( s < min_s ) {
					min_s = s;
				}

				if ( t < min_t ) {
					min_t = t;
				}

				x = ( alphawidth - 1 ) * s;
				y = ( alphaheight - 1 ) * t;

				layer = alphamap[ x + y * alphawidth ];
				if ( layer < minlayer ) {
					minlayer = layer;
				}

				if ( layer > maxlayer ) {
					maxlayer = layer;
				}

				alpha[ i ] = layer;
			}

			x = min_s * surfwidth;
			if ( x >= surfwidth ) {
				x = surfwidth - 1;
			}

			y = min_t * surfheight;
			if ( y >= surfheight ) {
				y = surfheight - 1;
			}

			if ( strstr( side->shaderInfo->shader, "terrain" ) ) {
				si = ShaderForLayer( minlayer, maxlayer, shadername );
				if ( showseams ) {
					for( i = 0; i < side->winding->numpoints; i++ ) {
						if ( ( alpha[ i ] != minlayer ) && ( alpha[ i ] != maxlayer ) ) {
							si = ShaderInfoForShader( "textures/common/white" );
							break;
						}
					}
				}
				surf = SurfaceForShader( si, x, y );
				EmitTerrainVerts( side, surf, maxlayer, alpha, qtrue );
			} else {
				si = side->shaderInfo;
				side->shaderInfo = terrainShader;
				surf = SurfaceForShader( si, x, y );
				EmitTerrainVerts( side, surf, maxlayer, alpha, qfalse );
			}
		}
	}

	// create the final surfaces
	for( surf = surfaces, i = 0; i < numsurfaces; i++, surf++ ) {
		if ( surf->numVerts ) {
			CreateTerrainSurface( surf, surf->shader );
		}
	}

	//
	// clean up any allocated memory
	//
	for( surf = surfaces, i = 0; i < numsurfaces; i++, surf++ ) {
		if ( surf->verts ) {
			free( surf->verts );
			free( surf->indexes );
		}
	}
	free( alphamap );
	free( surfaces );

	surfaces = NULL;
	lastSurface = NULL;
	numsurfaces = 0;
	maxsurfaces = 0;
}

/*****************************************************************************

	New terrain code

******************************************************************************/

typedef struct terrainFace_s {
	shaderInfo_t			*shaderInfo;
	//texdef_t				texdef;

	float					vecs[ 2 ][ 4 ]; // texture coordinate mapping
} terrainFace_t;

typedef struct terrainVert_s {
	vec3_t					xyz;
	terrainFace_t			tri;
} terrainVert_t;

typedef struct terrainMesh_s {
	float					scale_x;
	float					scale_y;
	vec3_t					origin;

	int						width, height;
	terrainVert_t			*map;
} terrainMesh_t;

terrainVert_t *Terrain_GetVert( terrainMesh_t *pm, int x, int y ) {
	return &pm->map[ x + y * pm->width ];
}

void Terrain_GetTriangles( terrainMesh_t *pm, int x, int y, terrainVert_t **verts ) {
	if ( ( x + y ) & 1 ) {
		// first tri
		verts[ 0 ] = Terrain_GetVert( pm, x, y );
		verts[ 1 ] = Terrain_GetVert( pm, x, y + 1 );
		verts[ 2 ] = Terrain_GetVert( pm, x + 1, y + 1 );

		// second tri
		verts[ 3 ] = verts[ 2 ];
		verts[ 4 ] = Terrain_GetVert( pm, x + 1, y );
		verts[ 5 ] = verts[ 0 ];
	} else {
		// first tri
		verts[ 0 ] = Terrain_GetVert( pm, x, y );
		verts[ 1 ] = Terrain_GetVert( pm, x, y + 1 );
		verts[ 2 ] = Terrain_GetVert( pm, x + 1, y );

		// second tri
		verts[ 3 ] = verts[ 2 ];
		verts[ 4 ] = verts[ 1 ];
		verts[ 5 ] = Terrain_GetVert( pm, x + 1, y + 1 );
	}
}

/*
================
EmitTerrainVerts2
================
*/
void EmitTerrainVerts2( terrainSurf_t *surf, terrainVert_t **verts, int alpha[ 3 ] ) {
	int			i;
	int			j;
	drawVert_t	*vert;
	int			*indices;
	int			numindices;
	int			maxindices;
	int			xyplane;
	vec3_t		xynorm = { 0, 0, 1 };
	vec_t		shift[ 2 ] = { 0, 0 };
	vec_t		scale[ 2 ] = { 0.5, 0.5 };
	float		vecs[ 2 ][ 4 ];
	vec4_t		plane;

	if ( !surf->verts ) {
		surf->numVerts		= 0;
		surf->maxVerts		= GROW_VERTS;
		surf->verts			= malloc( surf->maxVerts * sizeof( *surf->verts ) );

		surf->numIndexes	= 0;
		surf->maxIndexes	= GROW_INDICES;
		surf->indexes		= malloc( surf->maxIndexes * sizeof( *surf->indexes ) );
	}

	// calculate the texture coordinate vectors
	xyplane = FindFloatPlane( xynorm, 0 );
	QuakeTextureVecs( &mapplanes[ xyplane ], shift, 0, scale, vecs );

	// emit the vertexes
	numindices = 0;
	maxindices = surf->maxIndexes;
	assert( maxindices >= 0 );
	indices = malloc ( maxindices * sizeof( *indices ) );

	PlaneFromPoints( plane, verts[ 0 ]->xyz, verts[ 1 ]->xyz, verts[ 2 ]->xyz );

	for ( i = 0; i < 3; i++ ) {
		vert = &surf->verts[ surf->numVerts ];

		if ( alpha[ i ] ) {
			vert->color[3] = 255;
		} else {
			vert->color[3] = 0;
		}

		vert->xyz[ 0 ] = floor( verts[ i ]->xyz[ 0 ] + 0.1f );
		vert->xyz[ 1 ] = floor( verts[ i ]->xyz[ 1 ] + 0.1f );
		vert->xyz[ 2 ] = floor( verts[ i ]->xyz[ 2 ] + 0.1f );

		// set the texture coordinates
		vert->st[0] = ( vecs[0][3] + DotProduct( vecs[ 0 ], vert->xyz ) ) / surf->shader->width;
		vert->st[1] = ( vecs[1][3] + DotProduct( vecs[ 1 ], vert->xyz ) ) / surf->shader->height;

		VectorCopy( plane, vert->normal );

		for( j = 0; j < surf->numVerts; j++ ) {
			if ( CompareVert( vert, &surf->verts[ j ], qtrue ) ) {
				break;
			}
		}
		
		if ( numindices >= maxindices ) {
			maxindices += GROW_INDICES;
			indices = realloc( indices, maxindices * sizeof( *indices ) );
		}

		if ( j != surf->numVerts ) {
			indices[ numindices++ ] = j;
		} else {
			indices[ numindices++ ] = surf->numVerts;
			surf->numVerts++;
			if ( surf->numVerts >= surf->maxVerts ) {
				surf->maxVerts += GROW_VERTS;
				surf->verts = realloc( surf->verts, surf->maxVerts * sizeof( *surf->verts ) );
			}
		}
	}

	SideAsTristrip( surf, indices, numindices );

	free( indices );
}

int      MapPlaneFromPoints( vec3_t p0, vec3_t p1, vec3_t p2 );
void     QuakeTextureVecs( plane_t *plane, vec_t shift[2], vec_t rotate, vec_t scale[2], vec_t mappingVecs[2][4] );
qboolean RemoveDuplicateBrushPlanes( bspbrush_t *b );
void     SetBrushContents( bspbrush_t *b );

void AddBrushSide( vec3_t v1, vec3_t v2, vec3_t v3, shaderInfo_t *terrainShader ) {
	side_t	*side;
	int		planenum;

	side = &buildBrush->sides[ buildBrush->numsides ];
	memset( side, 0, sizeof( *side ) );
	buildBrush->numsides++;

	side->shaderInfo = terrainShader;

	// find the plane number
	planenum = MapPlaneFromPoints( v1, v2, v3 );
	side->planenum = planenum;
}

void MakeBrushFromTriangle( vec3_t v1, vec3_t v2, vec3_t v3, shaderInfo_t *terrainShader ) {
	bspbrush_t	*b;
	vec3_t		d1;
	vec3_t		d2;
	vec3_t		d3;

	VectorSet( d1, v1[ 0 ], v1[ 1 ], MIN_WORLD_COORD + 10 );	//FIXME
	VectorSet( d2, v2[ 0 ], v2[ 1 ], MIN_WORLD_COORD + 10 );
	VectorSet( d3, v3[ 0 ], v3[ 1 ], MIN_WORLD_COORD + 10 );

	buildBrush->numsides = 0;
	buildBrush->detail = qfalse;

	AddBrushSide( v1, v2, v3, terrainShader );
	AddBrushSide( v1, d1, v2, terrainShader );
	AddBrushSide( v2, d2, v3, terrainShader );
	AddBrushSide( v3, d3, v1, terrainShader );
	AddBrushSide( d3, d2, d1, terrainShader );

	buildBrush->portalareas[0] = -1;
	buildBrush->portalareas[1] = -1;
	buildBrush->entitynum = num_entities-1;
	buildBrush->brushnum = entitySourceBrushes;

	// if there are mirrored planes, the entire brush is invalid
	if ( !RemoveDuplicateBrushPlanes( buildBrush ) ) {
		return;
	}

	// get the content for the entire brush
	SetBrushContents( buildBrush );
	buildBrush->contents |= CONTENTS_DETAIL;

	b = FinishBrush();
	if ( !b ) {
		return;
	}
}

void MakeTerrainIntoBrushes( terrainMesh_t *tm ) {
	int				index[ 6 ];
	int				y;
	int				x;
	terrainVert_t	*verts;
	shaderInfo_t	*terrainShader;

	terrainShader = ShaderInfoForShader( "textures/common/terrain" );

	verts = tm->map;
	for( y = 0; y < tm->height - 1; y++ ) {
		for( x = 0; x < tm->width - 1; x++ ) {
			if ( ( x + y ) & 1 ) {
				// first tri
				index[ 0 ] = x + y * tm->width;
				index[ 1 ] = x + ( y + 1 ) * tm->width;
				index[ 2 ] = ( x + 1 ) + ( y + 1 ) * tm->width;
				index[ 3 ] = ( x + 1 ) + ( y + 1 ) * tm->width;
				index[ 4 ] = ( x + 1 ) + y * tm->width;
				index[ 5 ] = x + y * tm->width;
			} else {
				// first tri
				index[ 0 ] = x + y * tm->width;
				index[ 1 ] = x + ( y + 1 ) * tm->width;
				index[ 2 ] = ( x + 1 ) + y * tm->width;
				index[ 3 ] = ( x + 1 ) + y * tm->width;
				index[ 4 ] = x + ( y + 1 ) * tm->width;
				index[ 5 ] = ( x + 1 ) + ( y + 1 ) * tm->width;
			}

			MakeBrushFromTriangle( verts[ index[ 0 ] ].xyz, verts[ index[ 1 ] ].xyz, verts[ index[ 2 ] ].xyz, terrainShader );
			MakeBrushFromTriangle( verts[ index[ 3 ] ].xyz, verts[ index[ 4 ] ].xyz, verts[ index[ 5 ] ].xyz, terrainShader );
		}
	}
}

void Terrain_ParseFace( terrainFace_t *face ) {
	shaderInfo_t	*si;
	vec_t			shift[ 2 ];
	vec_t			rotate;
	vec_t			scale[ 2 ];
	char			name[ MAX_QPATH ];
	char			shader[ MAX_QPATH ];
	plane_t			p;

	// read the texturedef
	GetToken( qfalse );
	strcpy( name, token );

	GetToken( qfalse );
	shift[ 0 ] = atof(token);
	GetToken( qfalse );
	shift[ 1 ] = atof( token ); 
	GetToken( qfalse );
	rotate = atof( token );
	GetToken( qfalse );
	scale[ 0 ] = atof( token );
	GetToken( qfalse );
	scale[ 1 ] = atof( token );

	// find default flags and values
	sprintf( shader, "textures/%s", name );
	si = ShaderInfoForShader( shader );
	face->shaderInfo = si;
	//face->texdef = si->texdef;

	// skip over old contents
	GetToken( qfalse );

	// skip over old flags
	GetToken( qfalse );

	// skip over old value
	GetToken( qfalse );

	//Surface_Parse( &face->texdef );
	//Surface_BuildTexdef( &face->texdef );

	// make a fake horizontal plane
	VectorSet( p.normal, 0, 0, 1 );
	p.dist = 0;
	p.type = PlaneTypeForNormal( p.normal );

	QuakeTextureVecs( &p, shift, rotate, scale, face->vecs );
}

#define MAX_TERRAIN_TEXTURES 128
static int			numtextures = 0;;
static shaderInfo_t	*textures[ MAX_TERRAIN_TEXTURES ];

void Terrain_AddTexture( shaderInfo_t *texture ) {
	int i;

	if ( !texture ) {
		return;
	}

	for( i = 0; i < numtextures; i++ ) {
		if ( textures[ i ] == texture ) {
			return;
		}
	}

	if ( numtextures >= MAX_TERRAIN_TEXTURES ) {
		Error( "Too many textures on terrain" );
		return;
	}

	textures[ numtextures++ ] = texture;
}

int LayerForShader( shaderInfo_t *shader ) {
	int i;
	int l;

	l = strlen( shader->shader );
	for( i = l - 1; i >= 0; i-- ) {
		if ( shader->shader[ i ] == '_' ) {
			return atoi( &shader->shader[ i + 1 ] );
			break;
		}
	}

	return 0;
}

/*
=================
ParseTerrain

Creates a mapDrawSurface_t from the terrain text
=================
*/

void ParseTerrain( void ) {
	int				i, j;
	int				x, y;
	int				x1, y1;
	terrainMesh_t	t;
	int				index;
	terrainVert_t	*verts[ 6 ];
	int				num_layers;
	int				layer, minlayer, maxlayer;
	int				alpha[ 6 ];
	shaderInfo_t	*si, *terrainShader;
	int				surfwidth, surfheight, surfsize;
	terrainSurf_t	*surf;
	char			shadername[ MAX_QPATH ];

	mapent->firstDrawSurf = numMapDrawSurfs;

	memset( &t, 0, sizeof( t ) );

	MatchToken( "{" );

	// get width
	GetToken( qtrue );
	t.width = atoi( token );

	// get height
	GetToken( qfalse );
	t.height = atoi( token );

	// get scale_x
	GetToken( qfalse );
	t.scale_x = atof( token );

	// get scale_y
	GetToken( qfalse );
	t.scale_y = atof( token );

	// get origin
	GetToken( qtrue );
	t.origin[ 0 ] = atof( token );
	GetToken( qfalse );
	t.origin[ 1 ] = atof( token );
	GetToken( qfalse );
	t.origin[ 2 ] = atof( token );

	t.map = malloc( t.width * t.height * sizeof( t.map[ 0 ] ) );

	if ( t.width <= 0 || t.height <= 0 ) {
		Error( "ParseTerrain: bad size" );
	}

	numtextures = 0;
	index = 0;
	for ( i = 0; i < t.height; i++ ) {
		for( j = 0; j < t.width; j++, index++ ) {
			// get height
			GetToken( qtrue );
			t.map[ index ].xyz[ 0 ] = t.origin[ 0 ] + t.scale_x * ( float )j;
			t.map[ index ].xyz[ 1 ] = t.origin[ 1 ] + t.scale_y * ( float )i;
			t.map[ index ].xyz[ 2 ] = t.origin[ 2 ] + atof( token );

			Terrain_ParseFace( &t.map[ index ].tri );
			Terrain_AddTexture( t.map[ index ].tri.shaderInfo );
		}
	}

	MatchToken( "}" );
	MatchToken( "}" );

	MakeTerrainIntoBrushes( &t );

	surfwidth	= ( ( t.scale_x * t.width ) + SURF_WIDTH - 1 ) / SURF_WIDTH;
	surfheight	= ( ( t.scale_y * t.height ) + SURF_HEIGHT - 1 ) / SURF_HEIGHT;
	surfsize = surfwidth * surfheight;

	//FIXME
	num_layers = 0;
	for( i = 0; i < numtextures; i++ ) {
		layer = LayerForShader( textures[ i ] ) + 1;
		if ( layer > num_layers ) {
			num_layers = layer;
		}
	}
	num_layers = 4;

	memset( alpha, 0, sizeof( alpha ) );

	lastSurface = NULL;
	numsurfaces = 0;
	maxsurfaces = 0;
	for( i = num_layers; i > 0; i-- ) {
		maxsurfaces += i * surfsize;
	}

	surfaces = malloc( maxsurfaces * sizeof( *surfaces ) );
	memset( surfaces, 0, maxsurfaces * sizeof( *surfaces ) );

	terrainShader = ShaderInfoForShader( "textures/common/terrain" );

	// get the shadername
	if ( Q_strncasecmp( textures[ 0 ]->shader, "textures/", 9 ) == 0 ) {
		strcpy( shadername, &textures[ 0 ]->shader[ 9 ] );
	} else {
		strcpy( shadername, textures[ 0 ]->shader );
	}
	j = strlen( shadername );
	for( i = j - 1; i >= 0; i-- ) {
		if ( shadername[ i ] == '_' ) {
			shadername[ i ] = 0;
			break;
		}
	}
	
	for( y = 0; y < t.height - 1; y++ ) {
		for( x = 0; x < t.width - 1; x++ ) {
			Terrain_GetTriangles( &t, x, y, verts );

			x1 = ( ( float )x / ( float )( t.width - 1 ) ) * surfwidth;
			if ( x1 >= surfwidth ) {
				x1 = surfwidth - 1;
			}

			y1 = ( ( float )y / ( float )( t.height - 1 ) ) * surfheight;
			if ( y1 >= surfheight ) {
				y1 = surfheight - 1;
			}

			maxlayer = minlayer = LayerForShader( verts[ 0 ]->tri.shaderInfo );
			for( i = 0; i < 3; i++ ) {
				layer = LayerForShader( verts[ i ]->tri.shaderInfo );
				if ( layer < minlayer ) {
					minlayer = layer;
				}
				if ( layer > maxlayer ) {
					maxlayer = layer;
				}
			}

			for( i = 0; i < 3; i++ ) {
				layer = LayerForShader( verts[ i ]->tri.shaderInfo );
				if ( layer > minlayer ) {
					alpha[ i ] = 1.0f;
				} else {
					alpha[ i ] = 0.0f;
				}
			}

			si = ShaderForLayer( minlayer, maxlayer, shadername );
			surf = SurfaceForShader( si, x1, y1 );
			EmitTerrainVerts2( surf, &verts[ 0 ], &alpha[ 0 ] );

			// second triangle
			maxlayer = minlayer = LayerForShader( verts[ 3 ]->tri.shaderInfo );
			for( i = 3; i < 6; i++ ) {
				layer = LayerForShader( verts[ i ]->tri.shaderInfo );
				if ( layer < minlayer ) {
					minlayer = layer;
				}
				if ( layer > maxlayer ) {
					maxlayer = layer;
				}
			}

			for( i = 3; i < 6; i++ ) {
				layer = LayerForShader( verts[ i ]->tri.shaderInfo );
				if ( layer > minlayer ) {
					alpha[ i ] = 1.0f;
				} else {
					alpha[ i ] = 0.0f;
				}
			}

			si = ShaderForLayer( minlayer, maxlayer, shadername );
			surf = SurfaceForShader( si, x1, y1 );
			EmitTerrainVerts2( surf, &verts[ 3 ], &alpha[ 3 ] );
		}
	}

	// create the final surfaces
	for( surf = surfaces, i = 0; i < numsurfaces; i++, surf++ ) {
		if ( surf->numVerts ) {
			CreateTerrainSurface( surf, surf->shader );
		}
	}

	//
	// clean up any allocated memory
	//
	for( surf = surfaces, i = 0; i < numsurfaces; i++, surf++ ) {
		if ( surf->verts ) {
			free( surf->verts );
			free( surf->indexes );
		}
	}
	free( surfaces );

	surfaces = NULL;
	lastSurface = NULL;
	numsurfaces = 0;
	maxsurfaces = 0;

    free( t.map );
}

