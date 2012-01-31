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
#include "aselib.h"
#ifdef _WIN32
#ifdef _TTIMOBUILD
#include "pakstuff.h"
#else
#include "../libs/pakstuff.h"
#endif
#endif


typedef struct {
	char		modelName[1024];
	md3Header_t	*header;
} loadedModel_t;

int		c_triangleModels;
int		c_triangleSurfaces;
int		c_triangleVertexes;
int		c_triangleIndexes;


#define	MAX_LOADED_MODELS	1024
loadedModel_t		loadedModels[MAX_LOADED_MODELS];
int					numLoadedModels;

/*
=================
R_LoadMD3
=================
*/
#define	LL(x) x=LittleLong(x)
md3Header_t *R_LoadMD3( const char *mod_name ) {
	int					i, j;
	md3Header_t			*md3;
    md3Frame_t			*frame;
	md3Surface_t		*surf;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	int					version;
	char				filename[1024];
	int					len;

	sprintf( filename, "%s%s", gamedir, mod_name );
	len = TryLoadFile( filename, (void **)&md3 );
#ifdef _WIN32
  if ( len <= 0 ) {
    len = PakLoadAnyFile(filename, (void **)&md3);
  }
#endif
	if ( len <= 0 ) {
	  return NULL;
	}

  version = LittleLong (md3->version);
	if (version != MD3_VERSION) {
		_printf( "R_LoadMD3: %s has wrong version (%i should be %i)\n",
				 mod_name, version, MD3_VERSION);
		return NULL;
	}

    LL(md3->ident);
    LL(md3->version);
    LL(md3->numFrames);
    LL(md3->numTags);
    LL(md3->numSurfaces);
    LL(md3->numSkins);
    LL(md3->ofsFrames);
    LL(md3->ofsTags);
    LL(md3->ofsSurfaces);
    LL(md3->ofsEnd);

	if ( md3->numFrames < 1 ) {
		_printf( "R_LoadMD3: %s has no frames\n", mod_name );
		return NULL;
	}

    // we don't need to swap tags in the renderer, they aren't used
    
	// swap all the frames
    frame = (md3Frame_t *) ( (byte *)md3 + md3->ofsFrames );
    for ( i = 0 ; i < md3->numFrames ; i++, frame++) {
    	frame->radius = LittleFloat( frame->radius );
        for ( j = 0 ; j < 3 ; j++ ) {
            frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
            frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    	frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
        }
	}

	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)md3 + md3->ofsSurfaces );
	for ( i = 0 ; i < md3->numSurfaces ; i++) {

        LL(surf->ident);
        LL(surf->flags);
        LL(surf->numFrames);
        LL(surf->numShaders);
        LL(surf->numTriangles);
        LL(surf->ofsTriangles);
        LL(surf->numVerts);
        LL(surf->ofsShaders);
        LL(surf->ofsSt);
        LL(surf->ofsXyzNormals);
        LL(surf->ofsEnd);
		
		if ( surf->numVerts > SHADER_MAX_VERTEXES ) {
			Error ("R_LoadMD3: %s has more than %i verts on a surface (%i)",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
		}
		if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) {
			Error ("R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
		}

		// swap all the triangles
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		// swap all the ST
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        for ( j = 0 ; j < surf->numVerts ; j++, st++ ) {
            st->st[0] = LittleFloat( st->st[0] );
            st->st[1] = LittleFloat( st->st[1] );
        }

		// swap all the XyzNormals
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );
        for ( j = 0 ; j < surf->numVerts * surf->numFrames ; j++, xyz++ ) 
		{
            xyz->xyz[0] = LittleShort( xyz->xyz[0] );
            xyz->xyz[1] = LittleShort( xyz->xyz[1] );
            xyz->xyz[2] = LittleShort( xyz->xyz[2] );

            xyz->normal = LittleShort( xyz->normal );
        }


		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}

	return md3;
}


/*
================
LoadModel
================
*/
md3Header_t *LoadModel( const char *modelName ) {
	int				i;
	loadedModel_t	*lm;

	// see if we already have it loaded
	for ( i = 0, lm = loadedModels ; i < numLoadedModels ; i++, lm++ ) {
		if ( !strcmp( modelName, lm->modelName ) ) {
			return lm->header;
		}
	}

	// load it
	if ( numLoadedModels == MAX_LOADED_MODELS ) {
		Error( "MAX_LOADED_MODELS" );
	}
	numLoadedModels++;

	strcpy( lm->modelName, modelName );

	lm->header = R_LoadMD3( modelName );

	return lm->header;
}

/*
============
InsertMD3Model

Convert a model entity to raw geometry surfaces and insert it in the tree
============
*/
void InsertMD3Model( const char *modelName, vec3_t origin, float angle, tree_t *tree ) {
	int					i, j;
	md3Header_t			*md3;
	md3Surface_t		*surf;
	md3Shader_t			*shader;
	md3Triangle_t		*tri;
	md3St_t				*st;
	md3XyzNormal_t		*xyz;
	drawVert_t			*outv;
	float				lat, lng;
	float				angleCos, angleSin;
	mapDrawSurface_t	*out;
	vec3_t				temp;

	angle = angle / 180 * Q_PI;
	angleCos = cos( angle );
	angleSin = sin( angle );

	// load the model
	md3 = LoadModel( modelName );
	if ( !md3 ) {
		return;
	}

	// each md3 surface will become a new bsp surface

	c_triangleModels++;
	c_triangleSurfaces += md3->numSurfaces;

	// expand, translate, and rotate the vertexes
	// swap all the surfaces
	surf = (md3Surface_t *) ( (byte *)md3 + md3->ofsSurfaces );
	for ( i = 0 ; i < md3->numSurfaces ; i++) {
		// allocate a surface
		out = AllocDrawSurf();
		out->miscModel = qtrue;

        shader = (md3Shader_t *) ( (byte *)surf + surf->ofsShaders );

		out->shaderInfo = ShaderInfoForShader( shader->name );

		out->numVerts = surf->numVerts;
		out->verts = malloc( out->numVerts * sizeof( out->verts[0] ) );

		out->numIndexes = surf->numTriangles * 3;
		out->indexes = malloc( out->numIndexes * sizeof( out->indexes[0] ) );

		out->lightmapNum = -1;
		out->fogNum = -1;

		// emit the indexes
		c_triangleIndexes += surf->numTriangles * 3;
		tri = (md3Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
		for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
			out->indexes[j*3+0] = tri->indexes[0];
			out->indexes[j*3+1] = tri->indexes[1];
			out->indexes[j*3+2] = tri->indexes[2];
		}

		// emit the vertexes
        st = (md3St_t *) ( (byte *)surf + surf->ofsSt );
        xyz = (md3XyzNormal_t *) ( (byte *)surf + surf->ofsXyzNormals );

		c_triangleVertexes += surf->numVerts;
		for ( j = 0 ; j < surf->numVerts ; j++, st++, xyz++ ) {
			outv = &out->verts[ j ];

			outv->st[0] = st->st[0];
			outv->st[1] = st->st[1];

			outv->lightmap[0] = 0;
			outv->lightmap[1] = 0;

			// the colors will be set by the lighting pass
			outv->color[0] = 255;
			outv->color[1] = 255;
			outv->color[2] = 255;
			outv->color[3] = 255;

			outv->xyz[0] = origin[0] + MD3_XYZ_SCALE * ( xyz->xyz[0] * angleCos - xyz->xyz[1] * angleSin );
			outv->xyz[1] = origin[1] + MD3_XYZ_SCALE * ( xyz->xyz[0] * angleSin +  xyz->xyz[1] * angleCos );
			outv->xyz[2] = origin[2] + MD3_XYZ_SCALE * ( xyz->xyz[2] );

			// decode the lat/lng normal to a 3 float normal
			lat = ( xyz->normal >> 8 ) & 0xff;
			lng = ( xyz->normal & 0xff );
			lat *= Q_PI/128;
			lng *= Q_PI/128;

			temp[0] = cos(lat) * sin(lng);
			temp[1] = sin(lat) * sin(lng);
			temp[2] = cos(lng);

			// rotate the normal
			outv->normal[0] = temp[0] * angleCos - temp[1] * angleSin;
			outv->normal[1] = temp[0] * angleSin +  temp[1] * angleCos;
			outv->normal[2] = temp[2];
		}

		// find the next surface
		surf = (md3Surface_t *)( (byte *)surf + surf->ofsEnd );
	}

}

//==============================================================================


/*
============
InsertASEModel

Convert a model entity to raw geometry surfaces and insert it in the tree
============
*/
void InsertASEModel( const char *modelName, vec3_t origin, float angle, tree_t *tree ) {
	int					i, j;
	drawVert_t			*outv;
	float				angleCos, angleSin;
	mapDrawSurface_t	*out;
	int					numSurfaces;
	const char			*name;
	polyset_t			*pset;
	int					numFrames;
	char				filename[1024];

	sprintf( filename, "%s%s", gamedir, modelName );

	angle = angle / 180 * Q_PI;
	angleCos = cos( angle );
	angleSin = sin( angle );

	// load the model
	ASE_Load( filename, qfalse, qfalse );

	// each ase surface will become a new bsp surface
	numSurfaces = ASE_GetNumSurfaces();

	c_triangleModels++;
	c_triangleSurfaces += numSurfaces;

	// expand, translate, and rotate the vertexes
	// swap all the surfaces
	for ( i = 0 ; i < numSurfaces ; i++) {
		name = ASE_GetSurfaceName( i );

		pset = ASE_GetSurfaceAnimation( i, &numFrames, -1, -1, -1 );
		if ( !name || !pset ) {
			continue;
		}

		// allocate a surface
		out = AllocDrawSurf();
		out->miscModel = qtrue;

		out->shaderInfo = ShaderInfoForShader( pset->materialname );

		out->numVerts = 3 * pset->numtriangles;
		out->verts = malloc( out->numVerts * sizeof( out->verts[0] ) );

		out->numIndexes = 3 * pset->numtriangles;
		out->indexes = malloc( out->numIndexes * sizeof( out->indexes[0] ) );

		out->lightmapNum = -1;
		out->fogNum = -1;

		// emit the indexes
		c_triangleIndexes += out->numIndexes;
		for ( j = 0 ; j < out->numIndexes ; j++ ) {
			out->indexes[j] = j;
		}

		// emit the vertexes
		c_triangleVertexes += out->numVerts;
		for ( j = 0 ; j < out->numVerts ; j++ ) {
			int		index;
			triangle_t	*tri;

			index = j % 3;
			tri = &pset->triangles[ j / 3 ];

			outv = &out->verts[ j ];

			outv->st[0] = tri->texcoords[index][0];
			outv->st[1] = tri->texcoords[index][1];

			outv->lightmap[0] = 0;
			outv->lightmap[1] = 0;

			// the colors will be set by the lighting pass
			outv->color[0] = 255;
			outv->color[1] = 255;
			outv->color[2] = 255;
			outv->color[3] = 255;

			outv->xyz[0] = origin[0] + tri->verts[index][0];
			outv->xyz[1] = origin[1] + tri->verts[index][1];
			outv->xyz[2] = origin[2] + tri->verts[index][2];

			// rotate the normal
			outv->normal[0] = tri->normals[index][0];
			outv->normal[1] = tri->normals[index][1];
			outv->normal[2] = tri->normals[index][2];
		}
	}

}


//==============================================================================



/*
=====================
AddTriangleModels
=====================
*/
void AddTriangleModels( tree_t *tree ) {
	int			entity_num;
	entity_t	*entity;

	qprintf("----- AddTriangleModels -----\n");

	for ( entity_num=1 ; entity_num< num_entities ; entity_num++ ) {
		entity = &entities[entity_num];
	
		// convert misc_models into raw geometry
		if ( !Q_stricmp( "misc_model", ValueForKey( entity, "classname" ) ) ) {
			const char	*model;
			vec3_t	origin;
			float	angle;

			// get the angle for rotation  FIXME: support full matrix positioning
			angle = FloatForKey( entity, "angle" );

			GetVectorForKey( entity, "origin", origin );

			model = ValueForKey( entity, "model" );
			if ( !model[0] ) {
				_printf("WARNING: misc_model at %i %i %i without a model key\n", (int)origin[0],
					(int)origin[1], (int)origin[2] );
				continue;
			}
			if ( strstr( model, ".md3" ) || strstr( model, ".MD3" ) ) {
				InsertMD3Model( model, origin, angle, tree );
				continue;
			}
			if ( strstr( model, ".ase" ) || strstr( model, ".ASE" ) ) {
				InsertASEModel( model, origin, angle, tree );
				continue;
			}
			_printf( "Unknown misc_model type: %s\n", model );
			continue;
		}
	}

	qprintf( "%5i triangle models\n", c_triangleModels );
	qprintf( "%5i triangle surfaces\n", c_triangleSurfaces );
	qprintf( "%5i triangle vertexes\n", c_triangleVertexes );
	qprintf( "%5i triangle indexes\n", c_triangleIndexes );
}

