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
/*

Todo:

immediate:
Texture placement
New map format
q3map


later:
Smoothing brush
Stitching terrains together
Cross terrain selection





Terrain_ApplyMatrix
UpdateTerrainInspector

*/

#include "stdafx.h"
#include "qe3.h"
#include "DialogInfo.h"
#include "assert.h"

//random in the range [0, 1]
#define random()			((rand () & 0x7fff) / ((float)0x7fff))

//random in the range [-1, 1]
#define crandom()			(2.0 * (random() - 0.5))

typedef struct {
	int		index;
	vec3_t	xyz;
	vec4_t	rgba;
	vec2_t	tc;
} terravert_t;

/*
==============
Terrain_SetEpair
sets an epair for the given patch
==============
*/
void Terrain_SetEpair( terrainMesh_t *p, const char *pKey, const char *pValue ) {
	if ( g_qeglobals.m_bBrushPrimitMode ) {
		SetKeyValue( p->epairs, pKey, pValue );
	}
}

/* 
=================
Terrain_GetKeyValue
=================
*/
const char *Terrain_GetKeyValue( terrainMesh_t *p, const char *pKey ) {
	if ( g_qeglobals.m_bBrushPrimitMode ) {
		return ValueForKey( p->epairs, pKey );
	}
	return "";
}

/*
==================
Terrain_MemorySize
==================
*/
int Terrain_MemorySize( terrainMesh_t *p ) {
	return _msize( p );
}

void Terrain_GetVert( terrainMesh_t *pm, int x, int y, float s, float t, terravert_t *v, qtexture_t *texture = NULL ) {
	terrainVert_t *cell;

	v->index = x + y * pm->width;

	cell = &pm->heightmap[ v->index ];

	v->xyz[ 0 ] = pm->origin[ 0 ] + x * pm->scale_x;
	v->xyz[ 1 ] = pm->origin[ 1 ] + y * pm->scale_y;
	v->xyz[ 2 ] = pm->origin[ 2 ] + cell->height;

	VectorCopy( cell->rgba, v->rgba );

	if ( !texture || ( texture == cell->tri.texture ) ) {
		v->rgba[ 3 ] = 1.0f;
	} else {
		v->rgba[ 3 ] = 0.0f;
	}

	v->tc[ 0 ] = s;
	v->tc[ 1 ] = t;
}

void Terrain_GetTriangles( terrainMesh_t *pm, int x, int y, terravert_t *a0, terravert_t *a1, terravert_t *a2, terravert_t *b0, terravert_t *b1, terravert_t *b2, qtexture_t *texture ) {
	if ( ( x + y ) & 1 ) {
		// first tri
		Terrain_GetVert( pm, x, y, 1.0f, 1.0f, a0, texture );
		Terrain_GetVert( pm, x, y + 1, 1.0f, 0.0f, a1, texture );
		Terrain_GetVert( pm, x + 1, y + 1, 0.0f, 0.0f, a2, texture );

		// second tri
		*b0 = *a2;
		Terrain_GetVert( pm, x + 1, y, 0.0f, 1.0f, b1, texture );
		*b2 = *a0;
	} else {
		// first tri
		Terrain_GetVert( pm, x, y, 1.0f, 1.0f, a0, texture );
		Terrain_GetVert( pm, x, y + 1, 1.0f, 0.0f, a1, texture );
		Terrain_GetVert( pm, x + 1, y, 0.0f, 1.0f, a2, texture );

		// second tri
		*b0 = *a2;
		*b1 = *a1;
		Terrain_GetVert( pm, x + 1, y + 1, 0.0f, 0.0f, b2, texture );
	}
}

void Terrain_GetTriangle( terrainMesh_t *pm, int index, terravert_t *a0, terravert_t *a1, terravert_t *a2 ) {
	int x;
	int y;
	int which;

	which = index & 1;
	index >>= 1;
	y = index / pm->width;
	x = index % pm->width;

	if ( ( x + y ) & 1 ) {
		if ( !which ) {
			// first tri
			Terrain_GetVert( pm, x, y, 1.0f, 1.0f, a0 );
			Terrain_GetVert( pm, x, y + 1, 1.0f, 0.0f, a1 );
			Terrain_GetVert( pm, x + 1, y + 1, 0.0f, 0.0f, a2 );
		} else {
			Terrain_GetVert( pm, x + 1, y + 1, 0.0f, 0.0f, a0 );
			Terrain_GetVert( pm, x + 1, y, 0.0f, 1.0f, a1 );
			Terrain_GetVert( pm, x, y, 1.0f, 1.0f, a2 );
		}
	} else {
		if ( !which ) {
			// first tri
			Terrain_GetVert( pm, x, y, 1.0f, 1.0f, a0 );
			Terrain_GetVert( pm, x, y + 1, 1.0f, 0.0f, a1 );
			Terrain_GetVert( pm, x + 1, y, 0.0f, 1.0f, a2 );
		} else {
			Terrain_GetVert( pm, x + 1, y, 0.0f, 1.0f, a0 );
			Terrain_GetVert( pm, x, y + 1, 1.0f, 0.0f, a1 );
			Terrain_GetVert( pm, x + 1, y + 1, 0.0f, 0.0f, a2 );
		}
	}
}

void Terrain_Delete( terrainMesh_t *p ) {
	if ( p->pSymbiot ) {
		p->pSymbiot->pTerrain = NULL;
		p->pSymbiot->terrainBrush = false;
	}

	free( p );

	p = NULL;
   
	UpdateTerrainInspector();
}

void Terrain_AddTexture( terrainMesh_t *pm, qtexture_t *texture ) {
	int i;

	if ( !texture ) {
		return;
	}

	for( i = 0; i < pm->numtextures; i++ ) {
		if ( pm->textures[ i ] == texture ) {
			return;
		}
	}

	if ( pm->numtextures >= MAX_TERRAIN_TEXTURES ) {
		Warning( "Too many textures on terrain" );
		return;
	}

	pm->textures[ pm->numtextures++ ] = texture;
}

void Terrain_RemoveTexture( terrainMesh_t *p, qtexture_t *texture ) {
	int i;

	for( i = 0; i < p->numtextures; i++ ) {
		if ( p->textures[ i ] == texture ) {
			break;
		}
	}

	if ( i < p->numtextures ) {
		// shift all textures down to remove the texture from the list
		p->numtextures--;
		for( ; i < p->numtextures; i++ ) {
			p->textures[ i ] = p->textures[ i + 1 ];
		}
	}
}

terrainMesh_t *MakeNewTerrain( int width, int height, qtexture_t *texture ) {
	int				h;
	int				w;
	terrainMesh_t	*pm;
	size_t			size;
	size_t			heightmapsize;
	terrainVert_t	*vert;
	int				index;

	heightmapsize  = sizeof( terrainVert_t ) * width * height;
	size           = sizeof( terrainMesh_t ) + heightmapsize;
   
	pm = reinterpret_cast< terrainMesh_t * >( qmalloc( size ) );
   
	memset( pm, 0x00, size );

	pm->numtextures = 0;
	pm->width      = width;
	pm->height     = height;
	pm->heightmap  = reinterpret_cast< terrainVert_t * >( pm + 1 );

	if ( texture ) {
		Terrain_AddTexture( pm, texture );
	}

	index = 0;
	vert = pm->heightmap;
	for( h = 0; h < pm->height; h++ ) {
		for( w = 0; w < pm->width; w++, vert++ ) {
			vert->tri.index = index++;
			vert->tri.texture = texture;
			if ( texture ) {
				vert->tri.texdef.SetName( texture->name );
			}

			vert->height = 0;

			VectorClear( vert->normal );
			VectorSet( vert->rgba, 1.0f, 1.0f, 1.0f );
			vert->rgba[ 3 ] = 1.0f;
		}
	}
   
	return pm;
}

brush_t *AddBrushForTerrain( terrainMesh_t *pm, bool bLinkToWorld ) {
	int		j;
	vec3_t	vMin;
	vec3_t	vMax;
	brush_t	*b;
	face_t	*f;

	// calculate the face normals
	Terrain_CalcNormals( pm );
   
	// find the farthest points in x,y,z
	Terrain_CalcBounds( pm, vMin, vMax );

	for( j = 0; j < 3; j++ ) {
		if ( vMin[ j ] == vMax[ j ] ) {
			vMin[ j ] -= 4;
			vMax[ j ] += 4;
		}
	}

	b = Brush_Create( vMin, vMax, &pm->heightmap->tri.texdef );

	for( f = b->brush_faces; f != NULL; f = f->next ) {
		// copy the texdef to the brush faces texdef
		f->texdef = pm->heightmap->tri.texdef;
	}

	// FIXME: this entire type of linkage needs to be fixed
	b->pTerrain		= pm;
	b->terrainBrush = true;
	pm->pSymbiot    = b;
	pm->bSelected   = false;
	pm->bDirty      = true;
	pm->nListID		= -1;

	if ( bLinkToWorld ) {
		Brush_AddToList( b, &active_brushes );
		Entity_LinkBrush( world_entity, b );
		Brush_Build( b, true );
	}
  
	return b;
}

terrainMesh_t *Terrain_Duplicate( terrainMesh_t *pFrom ) {
	terrainMesh_t *p;
	int w;
	int h;
	int index;

	p = MakeNewTerrain( pFrom->width, pFrom->height );
   
	VectorCopy( pFrom->origin, p->origin );
	VectorCopy( pFrom->mins, p->mins );
	VectorCopy( pFrom->maxs, p->maxs );

	p->scale_x  = pFrom->scale_x;
	p->scale_y  = pFrom->scale_y;
	p->pSymbiot = pFrom->pSymbiot;

	for( index = 0; index < pFrom->numtextures; index++ ) {
		Terrain_AddTexture( p, pFrom->textures[ index ] );
	}

	index = 0;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, index++ ) {
			p->heightmap[ index ] = pFrom->heightmap[ index ];
		}
	}
   
	p->bSelected   = false;
	p->bDirty      = true;
	p->nListID     = -1;

	AddBrushForTerrain( p );

	return p;
}

void Terrain_BrushToMesh( void ) {
	brush_t			*b;
	terrainMesh_t	*p;

	if ( !QE_SingleBrush() ) {
		return;
	}

	b = selected_brushes.next;

	if ( g_qeglobals.d_terrainWidth	< 1 ) {
		g_qeglobals.d_terrainWidth = 1;
	}

	if ( g_qeglobals.d_terrainHeight < 1 ) {
		g_qeglobals.d_terrainHeight = 1;
	}

	p = MakeNewTerrain( g_qeglobals.d_terrainWidth + 1, g_qeglobals.d_terrainHeight + 1, b->brush_faces->d_texture );
	p->scale_x = ( b->maxs[ 0 ] - b->mins[ 0 ] ) / float( p->width - 1 );
	p->scale_y = ( b->maxs[ 1 ] - b->mins[ 1 ] ) / float( p->height - 1 );

	VectorCopy( b->mins, p->origin );

	b = AddBrushForTerrain( p );
	Select_Delete();
	Select_Brush( b );
}

terrainFace_t *Terrain_ParseFace( terrainFace_t *f ) {
	// read the texturename
	GetToken( false );
	f->texdef.SetName( token );

	// Load the texture, and set the face to that texture's defaults
	f->texture = Texture_ForName( f->texdef.Name() );

	// read the texturedef
	GetToken( false );
	f->texdef.shift[ 0 ] = ( float )atoi( token );
	GetToken( false );
	f->texdef.shift[ 1 ] = ( float )atoi( token );
	GetToken( false );
	f->texdef.rotate = atof( token );
	GetToken( false );
	f->texdef.scale[ 0 ] = atof( token );
	GetToken( false );
	f->texdef.scale[ 1 ] = atof( token );

	// the flags and value field aren't necessarily present
	//f->texture = Texture_ForName( f->texdef.Name() );
	f->texdef.flags = f->texture->flags;
	f->texdef.value = f->texture->value;
	f->texdef.contents = f->texture->contents;
	
	if ( TokenAvailable () ) {
		GetToken (false);
		f->texdef.contents = atoi(token);
		GetToken (false);
		f->texdef.flags = atoi(token);
		GetToken (false);
		f->texdef.value = atoi(token);
	}

	return f;
}

brush_t *Terrain_Parse( void ) {
	terrainMesh_t *pm;
	terrainVert_t *vert;
	int w;
	int h;

	GetToken( true );
    if ( strcmp( token, "{" ) ) {
		return NULL;
	}

	// get width
	GetToken( false );
	w = atoi( token );

	// get height
	GetToken( false );
	h = atoi( token );

	pm = MakeNewTerrain( w, h );

	// get scale_x
	GetToken( false );
	pm->scale_x = atoi( token );

	// get scale_y
	GetToken( false );
	pm->scale_y = atoi( token );

	// get origin
	GetToken( true );
	pm->origin[ 0 ] = atoi( token );
	GetToken( false );
	pm->origin[ 1 ] = atoi( token );
	GetToken( false );
	pm->origin[ 2 ] = atoi( token );

	// get the height map
	vert = pm->heightmap;
	for( h = 0; h < pm->height; h++ ) {
		for( w = 0; w < pm->width; w++, vert++ ) {
			GetToken( true );
			vert->height = atoi( token );

			if ( !Terrain_ParseFace( &vert->tri ) ) {
				Terrain_Delete( pm );
				return NULL;
			}

			Terrain_AddTexture( pm, vert->tri.texture );
		}
	}

	GetToken( true );
	if ( strcmp( token, "}" ) ) {
		Terrain_Delete( pm );
		return NULL;
	}

	return AddBrushForTerrain( pm, false );
}

CString Terrain_SurfaceString( terrainFace_t *face ) {
	char		temp[ 1024 ];
	CString		text;
	const char	*pname;

	pname = face->texdef.Name();
	if ( pname[ 0 ] == 0 ) {
		pname = "unnamed";
	}

	sprintf( temp, "%s %i %i %.2f ", pname, ( int )face->texdef.shift[ 0 ], ( int )face->texdef.shift[ 1 ], face->texdef.rotate );
	text += temp;

	if ( face->texdef.scale[ 0 ] == ( int )face->texdef.scale[ 0 ] ) {
		sprintf( temp, "%i ", ( int )face->texdef.scale[ 0 ] );
	} else {
		sprintf( temp, "%f ", ( float )face->texdef.scale[ 0 ] );
	}
	text += temp;

	if ( face->texdef.scale[ 1 ] == (int)face->texdef.scale[ 1 ] ) {
		sprintf( temp, "%i", ( int )face->texdef.scale[ 1 ] );
	} else {
		sprintf( temp, "%f", ( float )face->texdef.scale[ 1 ] );
	}
	text += temp;

	// only output flags and value if not default
	sprintf( temp, " %i %i %i ", face->texdef.contents, face->texdef.flags, face->texdef.value );
	text += temp;
   
	return text;
}

void Terrain_Write( terrainMesh_t *p, CMemFile *file ) {
	int w;
	int h;
	terrainVert_t *vert;

	MemFile_fprintf( file, " {\n  terrainDef\n  {\n" );
	MemFile_fprintf( file, "   %d %d %f %f\n", p->width, p->height, p->scale_x, p->scale_y );
	MemFile_fprintf( file, "   %f %f %f\n", p->origin[ 0 ], p->origin[ 1 ], p->origin[ 2 ] );

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			MemFile_fprintf( file, "   %f %s\n", vert->height, ( const char * )Terrain_SurfaceString( &vert->tri ) );
		}
	}
   
	MemFile_fprintf( file, "  }\n }\n" );
}

void Terrain_Write( terrainMesh_t *p, FILE *file ) {
	int w;
	int h;
	terrainVert_t *vert;

	fprintf( file, " {\n  terrainDef\n  {\n" );
	fprintf( file, "   %d %d %f %f\n", p->width, p->height, p->scale_x, p->scale_y );
	fprintf( file, "   %f %f %f\n", p->origin[ 0 ], p->origin[ 1 ], p->origin[ 2 ] );

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			fprintf( file, "   %f %s\n", vert->height, ( const char * )Terrain_SurfaceString( &vert->tri ) );
		}
	}
   
	fprintf( file, "  }\n }\n" );
}

void Terrain_Select( terrainMesh_t *p ) {
	p->bSelected = true;
}

void Terrain_Deselect( terrainMesh_t *p ) {
	p->bSelected = false;
}

void Terrain_Move( terrainMesh_t *pm, const vec3_t vMove, bool bRebuild ) {
	pm->bDirty = true;

	VectorAdd( pm->origin, vMove, pm->origin );

	if ( bRebuild ) {
		vec3_t vMin; 
		vec3_t vMax;

		Terrain_CalcBounds( pm, vMin, vMax );
	}
  
	UpdateTerrainInspector();
}

void UpdateTerrainInspector( void ) {
	// not written yet
}

void Terrain_CalcBounds( terrainMesh_t *p, vec3_t &vMin, vec3_t &vMax ) {
	int w;
	int h;
	float f;
	terrainVert_t *vert;

	vMin[ 0 ] = p->origin[ 0 ];
	vMin[ 1 ] = p->origin[ 1 ];
	vMin[ 2 ] = MAX_WORLD_COORD;

	vMax[ 0 ] = p->origin[ 0 ] + ( p->width - 1 ) * p->scale_x;
	vMax[ 1 ] = p->origin[ 1 ] + ( p->height - 1 ) * p->scale_y;
	vMax[ 2 ] = MIN_WORLD_COORD;

	p->bDirty = true;
	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			f = p->origin[ 2 ] + vert->height;
			if ( f < vMin[ 2 ] ) {
				vMin[ 2 ] = f;
			}

			if ( f > vMax[ 2 ] ) {
				vMax[ 2 ] = f;
			}
		}
	}
}

void CalcTriNormal( const vec3_t a, const vec3_t b, const vec3_t c, vec3_t o ) {
	vec3_t a1;
	vec3_t b1;

	VectorSubtract( b, a, a1 );
	VectorNormalize( a1 );

	VectorSubtract( c, a, b1 );
	VectorNormalize( b1 );

	CrossProduct( a1, b1, o );
	VectorNormalize( o );
}

inline void Terrain_CalcVertPos( terrainMesh_t *p, int x, int y, vec3_t vert ) {
	int index;

	index = x + y * p->width;
	vert[ 0 ] = p->origin[ 0 ] + x * p->scale_x;
	vert[ 1 ] = p->origin[ 1 ] + y * p->scale_y;
	vert[ 2 ] = p->origin[ 2 ] + p->heightmap[ index ].height;
	
	VectorCopy( vert, p->heightmap[ index ].xyz );
}
   
void Terrain_CalcNormals( terrainMesh_t *p ) {
	int				x;
	int				y;
	int				width;
	int				num;
	terrainVert_t	*vert;
	vec3_t			norm;
	terravert_t		a0;
	terravert_t		a1;
	terravert_t		a2;
	terravert_t		b0;
	terravert_t		b1;
	terravert_t		b2;

	p->bDirty = true;

	num = p->height * p->width;
	vert = p->heightmap;
	//for( x = 0; x < num; x++, vert++ ) {
	for( y = 0; y < p->height - 1; y++ ) {
		for( x = 0; x < p->width - 1; x++, vert++ ) {
			VectorClear( vert->normal );
			Terrain_CalcVertPos( p, x, y, norm );
		}
	}

	width = p->width;
	vert = p->heightmap;
   
	for( y = 0; y < p->height - 1; y++ ) {
		for( x = 0; x < width - 1; x++ ) {
			Terrain_GetTriangles( p, x, y, &a0, &a1, &a2, &b0, &b1, &b2, NULL );

			CalcTriNormal( a0.xyz, a2.xyz, a1.xyz, norm );

			VectorAdd( vert[ a0.index ].normal, norm, vert[ a0.index ].normal );
			VectorAdd( vert[ a1.index ].normal, norm, vert[ a1.index ].normal );
			VectorAdd( vert[ a2.index ].normal, norm, vert[ a2.index ].normal );

			CalcTriNormal( b0.xyz, b2.xyz, b1.xyz, norm );

			VectorAdd( vert[ b0.index ].normal, norm, vert[ b0.index ].normal );
			VectorAdd( vert[ b1.index ].normal, norm, vert[ b1.index ].normal );
			VectorAdd( vert[ b2.index ].normal, norm, vert[ b2.index ].normal );
		}
	}
   
	for( x = 0; x < num; x++, vert++ ) {
		VectorNormalize( vert->normal );
		//FIXME
		vert->normal[ 2 ] += 0.5;
		VectorNormalize( vert->normal );
		assert( vert->normal[ 2 ] > 0 );
		VectorSet( vert->rgba, vert->normal[ 2 ], vert->normal[ 2 ], vert->normal[ 2 ] );
		vert->rgba[ 3 ] = 1.0f;
	}
}

void Terrain_FindReplaceTexture( terrainMesh_t *p, const char *pFind, const char *pReplace, bool bForce ) {
	int				w;
	int				h;
	terrainVert_t	*vert;
	qtexture_t		*texture;

	texture = Texture_ForName( pReplace );

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			if ( bForce || strcmpi( vert->tri.texture->name, pFind ) == 0 ) {
				vert->tri.texture = texture;
				vert->tri.texdef.SetName( texture->name );
			}
		}
	}

	if ( bForce ) {
		p->numtextures = 0;
		Terrain_AddTexture( p, Texture_ForName( pReplace ) );
	} else {
		Terrain_RemoveTexture( p, Texture_ForName( pFind ) );
		Terrain_AddTexture( p, texture );
	}
}

bool Terrain_HasTexture( terrainMesh_t *p, const char *name ) {
	int w;
	int h;
	terrainVert_t *vert;

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			if ( strcmpi( vert->tri.texture->name, name ) == 0 ) {
				return true;
			}
		}
	}

	return false;
}

void Terrain_ReplaceQTexture( terrainMesh_t *p, qtexture_t *pOld, qtexture_t *pNew ) {
	int w;
	int h;
	terrainVert_t *vert;

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			if ( vert->tri.texture == pOld ) {
				vert->tri.texture = pNew;
				vert->tri.texdef.SetName( pNew->name );
			}
		}
	}

	Terrain_RemoveTexture( p, pOld );
	Terrain_AddTexture( p, pNew );
}

void Terrain_SetTexture( terrainMesh_t *p, texdef_t *tex_def ) {
	int w;
	int h;
	qtexture_t *newtex;
	terrainVert_t *vert;

	p->bDirty = 1;
  
	newtex = Texture_ForName( tex_def->name );

	p->numtextures = 0;
	Terrain_AddTexture( p, newtex );

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		for( w = 0; w < p->width; w++, vert++ ) {
			vert->tri.texture = newtex;
			vert->tri.texdef.SetName( newtex->name );
		}
	}
   
	UpdateTerrainInspector();
}

void Terrain_Scale( terrainMesh_t *p, const vec3_t vOrigin, const vec3_t vAmt, bool bRebuild ) {
	int    w;
	int    h;
	vec3_t pos;
	terrainVert_t *vert;
	vec3_t vMin;
	vec3_t vMax;

	vert = p->heightmap;
	for( h = 0; h < p->height; h++ ) {
		pos[ 1 ] = p->origin[ 1 ] + h * p->scale_y;
		for( w = 0; w < p->width; w++, vert++ ) {
			pos[ 0 ] = p->origin[ 0 ] + w * p->scale_x;
			pos[ 2 ] = vert->height;

			if ( ( g_qeglobals.d_select_mode == sel_terrainpoint ) && ( Terrain_PointInMoveList( vert ) == -1 ) ) {
				continue;
			}

			vert->height -= vOrigin[ 2 ] - p->origin[ 2 ];
			vert->height *= vAmt[ 2 ];
			vert->height += vOrigin[ 2 ] - p->origin[ 2 ];
		}
	}

	if ( g_qeglobals.d_select_mode != sel_terrainpoint ) {
		p->scale_x *= vAmt[ 0 ];
		p->scale_y *= vAmt[ 1 ];
      
		p->origin[ 0 ] -= vOrigin[ 0 ];
		p->origin[ 0 ] *= vAmt[ 0 ];
		p->origin[ 0 ] += vOrigin[ 0 ];

		p->origin[ 1 ] -= vOrigin[ 1 ];
		p->origin[ 1 ] *= vAmt[ 1 ];
		p->origin[ 1 ] += vOrigin[ 1 ];
	}

	if ( bRebuild ) {
		Terrain_CalcBounds( p, vMin, vMax );
		Terrain_CalcNormals( p );
		Brush_RebuildBrush( p->pSymbiot, vMin, vMax );
	}

	UpdateTerrainInspector();
}

bool Terrain_DragScale( terrainMesh_t *p, vec3_t vAmt, vec3_t vMove ) {
	vec3_t	vMin;
	vec3_t	vMax;
	vec3_t	vScale;
	vec3_t	vTemp;
	vec3_t	vMid;
	int		i;

	Terrain_CalcBounds( p, vMin, vMax );

	VectorSubtract( vMax, vMin, vTemp );

	// if we are scaling in the same dimension the terrain has no depth
	for( i = 0; i < 3; i++ ) {
		if ( ( vTemp[ i ] == 0 ) && ( vMove[ i ] != 0 ) ) {
			return false;
		}
	}
  
	for( i = 0; i < 3; i++ ) {
		vMid[ i ] = ( vMin[ i ] + vMax[ i ] ) / 2;
	}

	for( i = 0; i < 3; i++ ) {
		if ( vAmt[ i ] != 0 ) {
			vScale[i] = 1.0 + vAmt[i] / vTemp[i];
		} else {
			vScale[i] = 1.0;
		}
	}

	Terrain_Scale( p, vMid, vScale, false );
	VectorSubtract( vMax, vMin, vTemp );
	Terrain_CalcBounds( p, vMin, vMax );
  	VectorSubtract( vMax, vMin, vMid );
	VectorSubtract( vMid, vTemp, vTemp );
	VectorScale( vTemp, 0.5f, vTemp );

	// abs of both should always be equal
	if ( !VectorCompare( vMove, vAmt ) ) {
		for( i = 0; i < 3; i++ ) {
			if ( vMove[ i ] != vAmt[ i ] ) {
				vTemp[ i ] = -vTemp[ i ];
			}
		}
	}

	Terrain_CalcNormals( p );
	Terrain_Move( p, vTemp );

	return true;
}

void Terrain_ApplyMatrix( terrainMesh_t *p, const vec3_t vOrigin, const vec3_t vMatrix[ 3 ], bool bSnap ) {
}

void Terrain_DrawFace( brush_t *brush, terrainFace_t *terraface ) {
	terrainMesh_t	*pm;
	terravert_t		a0;
	terravert_t		a1;
	terravert_t		a2;

	pm = brush->pTerrain;
   
	Terrain_GetTriangle( pm, terraface->index, &a0, &a1, &a2 );

	qglBindTexture( GL_TEXTURE_2D, terraface->texture->texture_number );
	qglBegin( GL_TRIANGLES );

	// first tri
	qglColor4fv( a0.rgba );
	qglTexCoord2fv( a0.tc );
	qglVertex3fv( a0.xyz );

	qglColor4fv( a1.rgba );
	qglTexCoord2fv( a1.tc );
	qglVertex3fv( a1.xyz );

	qglColor4fv( a2.rgba );
	qglTexCoord2fv( a2.tc );
	qglVertex3fv( a2.xyz );

	qglEnd ();
}

void DrawTerrain( terrainMesh_t *pm, bool bPoints, bool bShade ) {
	int				i;
	int				w;
	int				h;
	int				x;
	int				y;
	//int			n;
	//float			x1;
	//float			y1;
	float			scale_x;
	float			scale_y;
	//vec3_t		pSelectedPoints[ MAX_TERRA_POINTS ];
	//int			nIndex;
	terravert_t		a0;
	terravert_t		a1;
	terravert_t		a2;
	terravert_t		b0;
	terravert_t		b1;
	terravert_t		b2;
	terrainVert_t	*vert;
	qtexture_t		*texture;

	h = pm->height - 1;
	w = pm->width - 1;
   
	scale_x = pm->scale_x;
	scale_y = pm->scale_y;

	qglShadeModel (GL_SMOOTH);

	if ( bShade ) {
		for( i = 0; i < pm->numtextures; i++ ) {
			texture = pm->textures[ i ];

			qglBindTexture( GL_TEXTURE_2D, texture->texture_number );

			vert = pm->heightmap;
			for( y = 0; y < h; y++ ) {
				qglBegin( GL_TRIANGLES );

				for( x = 0; x < w; x++, vert++ ) {
					Terrain_GetTriangles( pm, x, y, &a0, &a1, &a2, &b0, &b1, &b2, texture );

					// first tri
					if ( a0.rgba[ 3 ] || a1.rgba[ 3 ] || a2.rgba[ 3 ] ) {
						qglColor4fv( a0.rgba );
						qglTexCoord2fv( a0.tc );
						qglVertex3fv( a0.xyz );

						qglColor4fv( a1.rgba );
						qglTexCoord2fv( a1.tc );
						qglVertex3fv( a1.xyz );

						qglColor4fv( a2.rgba );
						qglTexCoord2fv( a2.tc );
						qglVertex3fv( a2.xyz );
					}

					// second tri
					if ( b0.rgba[ 3 ] || b1.rgba[ 3 ] || b2.rgba[ 3 ] ) {
						qglColor4fv( b0.rgba );
						qglTexCoord2fv( b0.tc );
						qglVertex3fv( b0.xyz );

						qglColor4fv( b1.rgba );
						qglTexCoord2fv( b1.tc );
						qglVertex3fv( b1.xyz );

						qglColor4fv( b2.rgba );
						qglTexCoord2fv( b2.tc );
						qglVertex3fv( b2.xyz );
					}
				}

			qglEnd ();
			}
		}
	} else {
		for( i = 0; i < pm->numtextures; i++ ) {
			texture = pm->textures[ i ];

			qglBindTexture( GL_TEXTURE_2D, texture->texture_number );

			vert = pm->heightmap;
			for( y = 0; y < h; y++ ) {
				qglBegin( GL_TRIANGLES );

				for( x = 0; x < w; x++, vert++ ) {
					Terrain_GetTriangles( pm, x, y, &a0, &a1, &a2, &b0, &b1, &b2, texture );

					// first tri
					if ( a0.rgba[ 3 ] || a1.rgba[ 3 ] || a2.rgba[ 3 ] ) {
						qglColor4fv( a0.rgba );
						qglTexCoord2fv( a0.tc );
						qglVertex3fv( a0.xyz );

						qglColor4fv( a1.rgba );
						qglTexCoord2fv( a1.tc );
						qglVertex3fv( a1.xyz );

						qglColor4fv( a2.rgba );
						qglTexCoord2fv( a2.tc );
						qglVertex3fv( a2.xyz );
					}

					// second tri
					if ( b0.rgba[ 3 ] || b1.rgba[ 3 ] || b2.rgba[ 3 ] ) {
						qglColor4fv( b0.rgba );
						qglTexCoord2fv( b0.tc );
						qglVertex3fv( b0.xyz );

						qglColor4fv( b1.rgba );
						qglTexCoord2fv( b1.tc );
						qglVertex3fv( b1.xyz );

						qglColor4fv( b2.rgba );
						qglTexCoord2fv( b2.tc );
						qglVertex3fv( b2.xyz );
					}
				}
				qglEnd ();
			}
		}
	}

	qglPushAttrib( GL_CURRENT_BIT );

	bool bDisabledLighting = qglIsEnabled( GL_LIGHTING );
	if ( bDisabledLighting ) {
		qglDisable( GL_LIGHTING );
	}

#if 0
	terrainVert_t	*currentrow;
	terrainVert_t	*nextrow;
	float			x2;
	float			y2;

	// Draw normals
	qglDisable( GL_TEXTURE_2D );
	qglDisable( GL_BLEND );
	qglColor3f( 1, 1, 1 );
	qglBegin( GL_LINES );

	y2 = pm->origin[ 1 ];
	nextrow = pm->heightmap;
	for( y = 0; y < h; y++ ) {
		y1 = y2;
		y2 += scale_y;

		x2 = pm->origin[ 0 ];
		currentrow = nextrow;
		nextrow = currentrow + pm->width;
		for( x = 0; x < w; x++ ) {
			x1 = x2;
			x2 += scale_x;

			// normals
			qglVertex3f( x1, y1, pm->origin[ 2 ] + currentrow[ x ].height );
			qglVertex3f( x1 + currentrow[ x ].normal[ 0 ] * 16.0f, y1 + currentrow[ x ].normal[ 1 ] * 16.0f, pm->origin[ 2 ] + currentrow[ x ].height + currentrow[ x ].normal[ 2 ] * 16.0f );

			qglVertex3f( x2, y1, pm->origin[ 2 ] + currentrow[ x + 1 ].height );
			qglVertex3f( x2 + currentrow[ x + 1 ].normal[ 0 ] * 16.0f, y1 + currentrow[ x + 1 ].normal[ 1 ] * 16.0f, pm->origin[ 2 ] + currentrow[ x + 1 ].height + currentrow[ x + 1 ].normal[ 2 ] * 16.0f );

			qglVertex3f( x1, y2, pm->origin[ 2 ] + nextrow[ x ].height );
			qglVertex3f( x1 + nextrow[ x ].normal[ 0 ] * 16.0f, y2 + nextrow[ x ].normal[ 1 ] * 16.0f, pm->origin[ 2 ] + nextrow[ x ].height + nextrow[ x ].normal[ 2 ] * 16.0f );

			qglVertex3f( x2, y2, pm->origin[ 2 ] + nextrow[ x + 1 ].height );
			qglVertex3f( x2 + nextrow[ x + 1 ].normal[ 0 ] * 16.0f, y2 + nextrow[ x + 1 ].normal[ 1 ] * 16.0f, pm->origin[ 2 ] + nextrow[ x + 1 ].height + nextrow[ x + 1 ].normal[ 2 ] * 16.0f );
		}
	}

	qglEnd ();
	qglEnable( GL_TEXTURE_2D );
#endif

#if 0
	if ( bPoints && ( g_qeglobals.d_select_mode == sel_terrainpoint || g_qeglobals.d_select_mode == sel_area ) ) {
		qglPointSize( 6 );
		qglDisable( GL_TEXTURE_2D );
		qglDisable( GL_BLEND );

		qglBegin( GL_POINTS );

		nIndex = 0;

		qglColor4f( 1, 0, 1, 1 );

		y1 = pm->origin[ 1 ];
		for ( y = 0; y < pm->height; y++, y1 += pm->scale_y ) {
			x1 = pm->origin[ 0 ];
			for( x = 0; x < pm->width; x++, x1 += pm->scale_x ) {
				// FIXME: need to not do loop lookups inside here
				n = Terrain_PointInMoveList( &pm->heightmap[ x + y * pm->width ] );
				if ( n >= 0 ) {
					VectorSet( pSelectedPoints[ nIndex ], x1, y1, pm->heightmap[ x + y * pm->width ].height + pm->origin[ 2 ] );
					nIndex++;
				} else {
					qglVertex3f( x1, y1, pm->origin[ 2 ] + pm->heightmap[ x + y * pm->width ].height );
				}
			}
		}

		qglEnd();
		
		qglEnable( GL_TEXTURE_2D );

		if ( nIndex > 0 ) {
			qglBegin( GL_POINTS );
			qglColor4f( 0, 0, 1, 1 );
			while( nIndex-- > 0 ) {
				qglVertex3fv( pSelectedPoints[ nIndex ] );
			}
		
			qglEnd();
		}
	}
#endif

	if ( g_qeglobals.d_numterrapoints && ( ( g_qeglobals.d_select_mode == sel_terrainpoint ) || ( g_qeglobals.d_select_mode == sel_terraintexture ) ) ) {
#if 0 
		qglPointSize( 6 );
		qglDisable( GL_TEXTURE_2D );
		qglDisable( GL_BLEND );

		qglBegin( GL_POINTS );

		qglColor4f( 1, 0, 1, 1 );

		for( i = 0; i < g_qeglobals.d_numterrapoints; i++ ) {
			qglVertex3fv( g_qeglobals.d_terrapoints[ i ]->xyz );
		}

		qglEnd();
			
		qglEnable( GL_TEXTURE_2D );
#endif

		brush_t			*pb;
		terrainMesh_t	*pm;

		pm = NULL;
		for( pb = active_brushes .next; pb != &active_brushes; pb = pb->next ) {
			if ( pb->terrainBrush ) {
				pm = pb->pTerrain;
				break;
			}
		}

		if ( pm ) {
			qglDisable( GL_TEXTURE_2D );
			qglBegin( GL_TRIANGLES );
			qglEnable( GL_BLEND );

			qglColor4f( 0.25, 0.5, 1, 0.35 );

			for( i = 0; i < g_qeglobals.d_numterrapoints; i++ ) {
				terravert_t		a0;
				terravert_t		a1;
				terravert_t		a2;

				qglColor4f( 0.25, 0.5, 1, g_qeglobals.d_terrapoints[ i ]->scale * 0.75 + 0.25 );
				Terrain_GetTriangle( pm, g_qeglobals.d_terrapoints[ i ]->tri.index * 2, &a0, &a1, &a2 );

				qglVertex3fv( a0.xyz );
				qglVertex3fv( a1.xyz );
				qglVertex3fv( a2.xyz );

				Terrain_GetTriangle( pm, g_qeglobals.d_terrapoints[ i ]->tri.index * 2 + 1, &a0, &a1, &a2 );

				qglVertex3fv( a0.xyz );
				qglVertex3fv( a1.xyz );
				qglVertex3fv( a2.xyz );
			}
			qglEnd();
			
			qglDisable( GL_BLEND );
			qglEnable( GL_TEXTURE_2D );
		}
	}
}

void Terrain_DrawCam( terrainMesh_t *pm ) {
	qglColor3f( 1,1,1 );
	qglPushAttrib( GL_ALL_ATTRIB_BITS );

	if ( g_bPatchWireFrame ) {
		if( pm->bSelected ) {
			qglLineWidth( 2 );
		} else {
			qglLineWidth( 1 );
		}

		qglDisable( GL_CULL_FACE );
		qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		qglDisable( GL_TEXTURE_2D );

		if ( g_PrefsDlg.m_bGLLighting ) {
			qglDisable( GL_LIGHTING );
		}

		DrawTerrain( pm, pm->bSelected, true );

		if ( g_PrefsDlg.m_bGLLighting ) {
			qglEnable( GL_LIGHTING );
		}

		qglEnable( GL_CULL_FACE );
		qglLineWidth( 1 );
	} else {
		qglEnable( GL_CULL_FACE );
		qglCullFace( GL_FRONT );

		// draw the textured polys
		DrawTerrain( pm, pm->bSelected, true );

		// if selected, draw the red tint on the polys
		if( pm->bSelected ) { // && ( g_qeglobals.d_savedinfo.include & INCLUDE_CAMERATINT ) ) {
			qglColor4f( 1.0, 0.0, 0.0, 0.3 );
			qglEnable( GL_BLEND );
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
			qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			DrawTerrain( pm, pm->bSelected );

			qglColor3f( 1, 1, 1 );
		}

		// draw the backside poly outlines
		qglCullFace( GL_BACK );
		qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		qglDisable( GL_BLEND );
		DrawTerrain( pm, pm->bSelected, true );
	}

	qglPopAttrib();
}

void Terrain_DrawXY( terrainMesh_t *pm, entity_t *owner ) {
	qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
   
	if ( pm->bSelected ) {
		qglColor3fv( g_qeglobals.d_savedinfo.colors[ COLOR_SELBRUSHES ] );
	} else if ( owner != world_entity && _stricmp( owner->eclass->name, "func_group" ) ) {
		qglColor3fv( owner->eclass->color );
	} else {
		//FIXME
		qglColor3fv( g_qeglobals.d_savedinfo.colors[ COLOR_BRUSHES ] );
	}
	
	qglLineWidth( 1 );

	DrawTerrain( pm, pm->bSelected );

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
}

bool OnlyTerrainSelected( void ) {
	brush_t *pb;

	//if ( numselfaces || selected_brushes.next == &selected_brushes )
	if ( selected_brushes.next == &selected_brushes ) {
		return false;
	}

	for( pb = selected_brushes.next; pb != &selected_brushes; pb = pb->next ) {
		if ( !pb->terrainBrush ) {
			return false;
		}
	}
   
	return true;
}

bool AnyTerrainSelected( void ) {
	brush_t *pb;

	//if ( numselfaces || selected_brushes.next == &selected_brushes )
	if ( selected_brushes.next == &selected_brushes ) {
		return false;
	}

	for( pb = selected_brushes.next; pb != &selected_brushes; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			return true;
		}
	}
  
	return false;
}

terrainMesh_t *SingleTerrainSelected( void ) {
	if ( selected_brushes.next->terrainBrush ) {
		return selected_brushes.next->pTerrain;
	}

	return NULL;
}

void Terrain_Edit( void ) {
	//brush_t *pb;
	//terrainMesh_t *p;
	//int i;
	//int j;

//	g_qeglobals.d_numpoints = 0;
	g_qeglobals.d_numterrapoints = 0;
#if 0	
	for( pb = selected_brushes.next; pb != &selected_brushes ; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			p = pb->pTerrain;

			if ( ( g_qeglobals.d_numpoints + p->width * p->height ) > MAX_POINTS ) {
				Warning( "Too many points on terrain\n" );
				continue;
			}
			for( i = 0; i < p->width; i++ ) {
				for( j = 0; j < p->height; j++ ) {
					Terrain_CalcVertPos( p, i, j, g_qeglobals.d_points[ g_qeglobals.d_numpoints ] );
					g_qeglobals.d_numpoints++;
				}
			}
		}
	}
#endif
   
	g_qeglobals.d_select_mode = sel_terrainpoint;
}

void Terrain_SelectPointByRay( vec3_t org, vec3_t dir, int buttons ) {
	float			bestd;
	terrainFace_t	*face;
	terrainFace_t	*bestface;
	brush_t			*pb;
	float			dist;
	vec3_t			vec;

	// find the point closest to the ray
	bestface = NULL;
	bestd = WORLD_SIZE * 4;
	for( pb = selected_brushes.next; pb != &selected_brushes; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			face = Terrain_Ray( org, dir, pb, &dist );
			if ( face && ( dist < bestd ) ) {
				bestface = face;
				bestd = dist;
			}
		}
	}

	for( pb = active_brushes .next; pb != &active_brushes; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			face = Terrain_Ray( org, dir, pb, &dist );
			if ( face && ( dist < bestd ) ) {
				bestface = face;
				bestd = dist;
			}
		}
	}

	if ( !bestface ) {
		return;
	}
	
	VectorMA( org, bestd, dir, vec );
	Terrain_AddMovePoint( vec, buttons & MK_CONTROL, buttons & MK_SHIFT, buttons );
}

void Terrain_AddMovePoint( vec3_t v, bool bMulti, bool bFull, int buttons  ) {
	brush_t			*pb;
	terrainMesh_t	*p;
	terrainVert_t	*vert;
	int				x;
	int				y;
	int				x1, y1;
	float			dx, dy;
	float			dist;
	float			pd;

	if ( !g_bSameView && !bMulti && !bFull ) {
		g_bSameView = true;
		return;
	}
	
	g_qeglobals.d_numterrapoints = 0;
	for( pb = active_brushes .next; pb != &active_brushes; pb = pb->next ) {
	//for( pb = selected_brushes.next; pb != &selected_brushes; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			p = pb->pTerrain;
	    
			x = ( v[ 0 ] - p->origin[ 0 ] ) / p->scale_x;
			y = ( v[ 1 ] - p->origin[ 1 ] ) / p->scale_x;
			if ( ( x < 0 ) || ( x >= p->width ) || ( y < 0 ) || ( y >= p->height ) ) {
				continue;
			}

			vert = p->heightmap;
			for( y1 = 0; y1 < p->height; y1++ ) {
				for( x1 = 0; x1 < p->width; x1++, vert++ ) {

					if ( g_qeglobals.d_terrainBrush == TERRAIN_BRUSH_CIRCLE ) {
						dx = x1 - x;
						dy = y1 - y;
						dist = sqrt( dx * dx + dy * dy );
					} else {
						dx = abs( x1 - x );
						dy = abs( y1 - y );
						if ( dx > dy ) {
							dist = dx;
						} else {
							dist = dy;
						}
					}

					pd = dist * 2.0f / g_qeglobals.d_terrainBrushSize;
					if ( fabs( pd ) <= 1.0f ) {
						Terrain_AddPoint( p, vert );

						if ( ( buttons & MK_LBUTTON ) && ( g_qeglobals.d_select_mode == sel_terraintexture ) ) {
							vert->tri.texture = Texture_ForName( g_qeglobals.d_texturewin.texdef.name );
							vert->tri.texdef.SetName( vert->tri.texture->name );
							Terrain_AddTexture( p, vert->tri.texture );
							continue;
						}

						if ( g_qeglobals.d_terrainFalloff == TERRAIN_FALLOFF_CURVED ) {
							if ( g_qeglobals.d_terrainBrush == TERRAIN_BRUSH_CIRCLE ) {
								vert->scale = ( 0.5f + cos( pd * M_PI ) * 0.5f );
							} else {
								vert->scale = ( 0.5f + cos( dx/ g_qeglobals.d_terrainBrushSize * M_PI ) * 0.5f ) * ( 0.5f + cos( dy/ g_qeglobals.d_terrainBrushSize * M_PI ) * 0.5f ) - 0.25;
							}
						} else {
							vert->scale = 1.0f - pd;
						}

						switch( g_qeglobals.d_terrainNoiseType ) {
						case NOISE_PLUS :
							vert->scale *= crandom();
							break;

						case NOISE_PLUSMINUS :
							vert->scale *= random();
							break;
						}
					}
				}
			}
		}
	}
}

void Terrain_UpdateSelected( vec3_t vMove ) {
	int				i;
	brush_t			*pb;
	terrainMesh_t	*p;
	vec3_t			vMin;
	vec3_t			vMax;

	if ( g_qeglobals.d_select_mode == sel_terrainpoint ) {
		for( i = 0; i < g_qeglobals.d_numterrapoints; i++ ) {
			g_qeglobals.d_terrapoints[ i ]->height += vMove[ 2 ] * g_qeglobals.d_terrapoints[ i ]->scale;
		}
	}

	for( pb = active_brushes .next; pb != &active_brushes; pb = pb->next ) {
//	for( pb = selected_brushes.next; pb != &selected_brushes; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			p = pb->pTerrain;

			Terrain_CalcBounds( p, vMin, vMax );
			Terrain_CalcNormals( p );
			Brush_RebuildBrush( p->pSymbiot, vMin, vMax );
		}
	}
}

int Terrain_PointInMoveList( terrainVert_t *pf ) {
	int i;

	for( i = 0; i < g_qeglobals.d_numterrapoints; i++ ) {
		if ( pf == g_qeglobals.d_terrapoints[ i ] ) {
			return i;
		}
	}

	return -1;
}

void Terrain_RemovePointFromMoveList( terrainVert_t *v ) {
	int n;
	int i;
   
	while( ( n = Terrain_PointInMoveList( v ) ) >= 0 ) {
		for( i = n; i < g_qeglobals.d_numterrapoints - 1; i++ ) {
			g_qeglobals.d_terrapoints[ i ] = g_qeglobals.d_terrapoints[ i + 1 ];
		}
    
		g_qeglobals.d_numterrapoints--;
	}
}

void Terrain_AddPoint( terrainMesh_t *p, terrainVert_t *v ) {
	if ( g_qeglobals.d_numterrapoints < MAX_TERRA_POINTS ) {
		g_qeglobals.d_terrapoints[ g_qeglobals.d_numterrapoints++ ] = v;
	}
}

void Terrain_SelectAreaPoints( void ) {
	brush_t			*pb;
	terrainMesh_t	*p;
	int				x;
	int				y;
	vec3_t			vec;

	g_qeglobals.d_numterrapoints = 0;
	g_nPatchClickedView = -1;

	for( pb = selected_brushes.next; pb != &selected_brushes; pb = pb->next ) {
		if ( pb->terrainBrush ) {
			p = pb->pTerrain;
			for( x = 0; x < p->width; x++ ) {
				for( y = 0; y < p->height; y++ ) {
					Terrain_CalcVertPos( p, x, y, vec );
					if ( within( vec, g_qeglobals.d_vAreaTL, g_qeglobals.d_vAreaBR ) ) {
						if ( g_qeglobals.d_numterrapoints < MAX_TERRA_POINTS ) {
							g_qeglobals.d_terrapoints[ g_qeglobals.d_numterrapoints++ ] = &p->heightmap[ x + y * p->width ];
						}
					}
				}
			}
		}
	}
}

#define EPSILON 0.0001

bool RayTriangleIntersect( vec3_t orig, vec3_t dir, vec3_t vert1, vec3_t vert2, vec3_t vert3, float *t ) {
	float   u;
	float   v;
	vec3_t  edge1;
	vec3_t  edge2;
	vec3_t  tvec;
	vec3_t	pvec;
	vec3_t	qvec;
	float	det;

	VectorSubtract( vert2, vert1, edge1 );
	VectorSubtract( vert3, vert1, edge2 );

	// begin calculating determinant - also used to calculate U parameter
	CrossProduct( dir, edge2, pvec );

	// if determinant is near zero, ray lies in plane of triangle
	det = DotProduct( edge1, pvec );
	if ( det < EPSILON ) {
		return false;
	}

	// calculate distance from vert1 to ray origin
	VectorSubtract( orig, vert1, tvec );

	// calculate U parameter and test bounds
	u = DotProduct( tvec, pvec );
	if ( ( u < 0.0f ) || ( u > det ) ) {
		return false;
	}

	// prepare to test V parameter
	CrossProduct( tvec, edge1, qvec );

	// calculate V parameter and test bounds
	v = DotProduct( dir, qvec );
	if ( ( v < 0.0f ) || ( u + v > det ) ) {
		return false;
	}

	// calculate t, scale parameters, ray intersects triangle
	*t = DotProduct( edge2, qvec ) / det;

	return true;
}

/*
==============
Terrain_Ray

Itersects a ray with a terrain
Returns the face hit and the distance along the ray the intersection occured at
Returns NULL and 0 if not hit at all
==============
*/
terrainFace_t *Terrain_Ray( vec3_t origin, vec3_t dir, brush_t *b, float *dist ) {
	terrainMesh_t	*pm;
	int				h;
	int				w;
	int				x;
	int				y;
	float			best_t;
	float			t;
	terravert_t		a0;
	terravert_t		a1;
	terravert_t		a2;
	terravert_t		b0;
	terravert_t		b1;
	terravert_t		b2;
	terrainVert_t	*vert;
	terrainFace_t	*best;

	best = NULL;
	best_t = WORLD_SIZE * 2;

	pm = b->pTerrain;
	h = pm->height - 1;
	w = pm->width - 1;
   
	vert = pm->heightmap;
	for( y = 0; y < h; y++, vert++ ) {
		for( x = 0; x < w; x++, vert++ ) {
			Terrain_GetTriangles( pm, x, y, &a0, &a1, &a2, &b0, &b1, &b2, NULL );

			t = WORLD_SIZE * 2;
			if ( RayTriangleIntersect( origin, dir, a2.xyz, a1.xyz, a0.xyz, &t ) ) {
				if ( ( t >= 0 ) && ( t < best_t ) ) {
					best = &vert->tri;
					best_t = t;
				}
			}

			t = WORLD_SIZE * 2;
			if ( RayTriangleIntersect( origin, dir, b2.xyz, b1.xyz, b0.xyz, &t ) ) {
				if ( ( t >= 0 ) && ( t < best_t ) ) {
					best = &vert->tri;
					best_t = t;
				}
			}
		}
	}

	if ( !best ) {
		*dist = 0;
		return NULL;
	}

	*dist = best_t;

	return best;
}

/*
============
Select_TerrainFace

Select the face
============
*/
void Select_TerrainFace ( brush_t * brush, terrainFace_t *terraface ) {
#if 0
	UnSelect_Brush( brush );

	if( numselfaces < MAX_SEL_FACES ) {
		selfaces[numselfaces].face = NULL;
		selfaces[numselfaces].brush = brush;
		selfaces[numselfaces].terraface = terraface;
		numselfaces++;
	}
#endif
}

void Select_TerrainFacesFromBrush( brush_t *brush ) {
	terrainMesh_t	*pm;
	int				h;
	int				w;
	int				x;
	int				y;

	pm = brush->pTerrain;
	h = pm->height - 1;
	w = pm->width - 1;
   
	for( y = 0; y < h; y++ ) {
		for( x = 0; x < w; x++ ) {
			Select_TerrainFace( brush, &brush->pTerrain->heightmap[ x + y * pm->width ].tri );
		}
	}
}

void SetTerrainTexdef( brush_t *brush, terrainFace_t *face, texdef_t *texdef ) {
	int	oldFlags;
	int	oldContents;

	oldFlags = face->texdef.flags;
	oldContents = face->texdef.contents;

	face->texdef = *texdef;

	face->texdef.flags = ( face->texdef.flags & ~SURF_KEEP ) | ( oldFlags & SURF_KEEP );
	face->texdef.contents = ( face->texdef.contents & ~CONTENTS_KEEP ) | ( oldContents & CONTENTS_KEEP );
   
	face->texture = Texture_ForName( texdef->name );

	//Terrain_AddTexture( face->texture );
}

void RotateTerrainFaceTexture( terrainFace_t *vert, int nAxis, float fDeg ) {
}

void TerrainFace_FitTexture( terrainFace_t *vert ) {
}

void Terrain_Init( void ) {
	g_qeglobals.d_terrainWidth		= 64;
	g_qeglobals.d_terrainHeight		= 64;
	g_qeglobals.d_terrainBrushSize	= 12;
	g_qeglobals.d_terrainNoiseType	= NOISE_NONE;
	//g_qeglobals.d_terrainFalloff	= TERRAIN_FALLOFF_LINEAR;
	g_qeglobals.d_terrainFalloff	= TERRAIN_FALLOFF_CURVED;
	g_qeglobals.d_terrainBrush		= TERRAIN_BRUSH_CIRCLE;
	//g_qeglobals.d_terrainBrush		= TERRAIN_BRUSH_SQUARE;
}
