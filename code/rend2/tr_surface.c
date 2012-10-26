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
// tr_surf.c
#include "tr_local.h"
#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/


//============================================================================


/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow( int verts, int indexes ) {
	if (tess.numVertexes + verts < SHADER_MAX_VERTEXES
		&& tess.numIndexes + indexes < SHADER_MAX_INDEXES) {
		return;
	}

	RB_EndSurface();

	if ( verts >= SHADER_MAX_VERTEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, SHADER_MAX_VERTEXES );
	}
	if ( indexes >= SHADER_MAX_INDEXES ) {
		ri.Error(ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, SHADER_MAX_INDEXES );
	}

	RB_BeginSurface(tess.shader, tess.fogNum );
}

void RB_CheckVBOandIBO(VBO_t *vbo, IBO_t *ibo)
{
	if (!(vbo == glState.currentVBO && ibo == glState.currentIBO) || tess.multiDrawPrimitives >= MAX_MULTIDRAW_PRIMITIVES)
	{
		RB_EndSurface();
		RB_BeginSurface(tess.shader, tess.fogNum);

		R_BindVBO(vbo);
		R_BindIBO(ibo);
	}

	if (vbo != tess.vbo && ibo != tess.ibo)
		tess.useInternalVBO = qfalse;
}


/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, float color[4], float s1, float t1, float s2, float t2 ) {
	vec3_t		normal;
	int			ndx;

	RB_CHECKOVERFLOW( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.xyz[ndx][0] = origin[0] + left[0] + up[0];
	tess.xyz[ndx][1] = origin[1] + left[1] + up[1];
	tess.xyz[ndx][2] = origin[2] + left[2] + up[2];

	tess.xyz[ndx+1][0] = origin[0] - left[0] + up[0];
	tess.xyz[ndx+1][1] = origin[1] - left[1] + up[1];
	tess.xyz[ndx+1][2] = origin[2] - left[2] + up[2];

	tess.xyz[ndx+2][0] = origin[0] - left[0] - up[0];
	tess.xyz[ndx+2][1] = origin[1] - left[1] - up[1];
	tess.xyz[ndx+2][2] = origin[2] - left[2] - up[2];

	tess.xyz[ndx+3][0] = origin[0] + left[0] - up[0];
	tess.xyz[ndx+3][1] = origin[1] + left[1] - up[1];
	tess.xyz[ndx+3][2] = origin[2] + left[2] - up[2];


	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.or.axis[0], normal );

	tess.normal[ndx][0] = tess.normal[ndx+1][0] = tess.normal[ndx+2][0] = tess.normal[ndx+3][0] = normal[0];
	tess.normal[ndx][1] = tess.normal[ndx+1][1] = tess.normal[ndx+2][1] = tess.normal[ndx+3][1] = normal[1];
	tess.normal[ndx][2] = tess.normal[ndx+1][2] = tess.normal[ndx+2][2] = tess.normal[ndx+3][2] = normal[2];
	
	// standard square texture coordinates
	tess.texCoords[ndx][0][0] = tess.texCoords[ndx][1][0] = s1;
	tess.texCoords[ndx][0][1] = tess.texCoords[ndx][1][1] = t1;

	tess.texCoords[ndx+1][0][0] = tess.texCoords[ndx+1][1][0] = s2;
	tess.texCoords[ndx+1][0][1] = tess.texCoords[ndx+1][1][1] = t1;

	tess.texCoords[ndx+2][0][0] = tess.texCoords[ndx+2][1][0] = s2;
	tess.texCoords[ndx+2][0][1] = tess.texCoords[ndx+2][1][1] = t2;

	tess.texCoords[ndx+3][0][0] = tess.texCoords[ndx+3][1][0] = s1;
	tess.texCoords[ndx+3][0][1] = tess.texCoords[ndx+3][1][1] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	tess.vertexColors[ndx][0] = color[0];
	tess.vertexColors[ndx][1] = color[1];
	tess.vertexColors[ndx][2] = color[2];
	tess.vertexColors[ndx][3] = color[3];

	tess.vertexColors[ndx+1][0] = color[0];
	tess.vertexColors[ndx+1][1] = color[1];
	tess.vertexColors[ndx+1][2] = color[2];
	tess.vertexColors[ndx+1][3] = color[3];

	tess.vertexColors[ndx+2][0] = color[0];
	tess.vertexColors[ndx+2][1] = color[1];
	tess.vertexColors[ndx+2][2] = color[2];
	tess.vertexColors[ndx+2][3] = color[3];

	tess.vertexColors[ndx+3][0] = color[0];
	tess.vertexColors[ndx+3][1] = color[1];
	tess.vertexColors[ndx+3][2] = color[2];
	tess.vertexColors[ndx+3][3] = color[3];

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, float color[4] ) {
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}


/*
==============
RB_InstantQuad

based on Tess_InstantQuad from xreal
==============
*/
void RB_InstantQuad2(vec4_t quadVerts[4], vec2_t texCoords[4])
{
	GLimp_LogComment("--- RB_InstantQuad2 ---\n");

	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	VectorCopy4(quadVerts[0], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[0], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[1], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[1], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[2], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[2], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	VectorCopy4(quadVerts[3], tess.xyz[tess.numVertexes]);
	VectorCopy2(texCoords[3], tess.texCoords[tess.numVertexes][0]);
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	RB_UpdateVBOs(ATTR_POSITION | ATTR_TEXCOORD);

	GLSL_VertexAttribsState(ATTR_POSITION | ATTR_TEXCOORD);

	R_DrawElementsVBO(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}


void RB_InstantQuad(vec4_t quadVerts[4])
{
	vec4_t color;
	vec2_t texCoords[4];
	vec2_t invTexRes;

	VectorSet4(color, 1, 1, 1, 1);

	texCoords[0][0] = 0;
	texCoords[0][1] = 0;

	texCoords[1][0] = 1;
	texCoords[1][1] = 0;

	texCoords[2][0] = 1;
	texCoords[2][1] = 1;

	texCoords[3][0] = 0;
	texCoords[3][1] = 1;

	invTexRes[0] = 1.0f / 256.0f;
	invTexRes[1] = 1.0f / 256.0f;

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMatrix16(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_COLOR, color);
	GLSL_SetUniformVec2(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_INVTEXRES, invTexRes);
	GLSL_SetUniformVec2(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax);
	GLSL_SetUniformVec3(&tr.textureColorShader, TEXTURECOLOR_UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear);

	RB_InstantQuad2(quadVerts, texCoords); //, color, &tr.textureColorShader, invTexRes);
}


/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( void ) {
	vec3_t		left, up;
	float		radius;
	float			colors[4];
	trRefEntity_t	*ent = backEnd.currentEntity;

	// calculate the xyz locations for the four corners
	radius = ent->e.radius;
	if ( ent->e.rotation == 0 ) {
		VectorScale( backEnd.viewParms.or.axis[1], radius, left );
		VectorScale( backEnd.viewParms.or.axis[2], radius, up );
	} else {
		float	s, c;
		float	ang;
		
		ang = M_PI * ent->e.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.viewParms.or.axis[1], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.or.axis[2], left );

		VectorScale( backEnd.viewParms.or.axis[2], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.or.axis[1], up );
	}
	if ( backEnd.viewParms.isMirror ) {
		VectorSubtract( vec3_origin, left, left );
	}

#ifdef REACTION
	if (ent->e.renderfx & RF_SUNFLARE)
	{
		if (backEnd.viewHasSunFlare)
		{
			ri.Printf(PRINT_WARNING, "Multiple sun flares not supported\n");
			return;
		}
		if (R_CullPointAndRadiusEx(ent->e.origin, ent->e.radius, backEnd.viewParms.frustum, ARRAY_LEN(backEnd.viewParms.frustum)) == CULL_OUT)
			return;
		colors[0] = colors[1] = colors[2] = colors[3] = ent->e.shaderRGBA[glRefConfig.framebufferObject] / 255.0f;
		if (colors[0] == 0)
			return;
		backEnd.viewHasSunFlare = qtrue;
		backEnd.frameHasSunFlare = qtrue;
	}
	else
#endif
	{
		VectorScale4(ent->e.shaderRGBA, 1.0f / 255.0f, colors);
	}

	RB_AddQuadStamp( ent->e.origin, left, up, colors );
}


/*
=============
RB_SurfacePolychain
=============
*/
static void RB_SurfacePolychain( srfPoly_t *p ) {
	int		i;
	int		numv;

	RB_CHECKOVERFLOW( p->numVerts, 3*(p->numVerts - 2) );

	// fan triangles into the tess array
	numv = tess.numVertexes;
	for ( i = 0; i < p->numVerts; i++ ) {
		VectorCopy( p->verts[i].xyz, tess.xyz[numv] );
		tess.texCoords[numv][0][0] = p->verts[i].st[0];
		tess.texCoords[numv][0][1] = p->verts[i].st[1];
		tess.vertexColors[numv][0] = p->verts[ i ].modulate[0] / 255.0f;
		tess.vertexColors[numv][1] = p->verts[ i ].modulate[1] / 255.0f;
		tess.vertexColors[numv][2] = p->verts[ i ].modulate[2] / 255.0f;
		tess.vertexColors[numv][3] = p->verts[ i ].modulate[3] / 255.0f;

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts-2; i++ ) {
		tess.indexes[tess.numIndexes + 0] = tess.numVertexes;
		tess.indexes[tess.numIndexes + 1] = tess.numVertexes + i + 1;
		tess.indexes[tess.numIndexes + 2] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

static void RB_SurfaceHelper( int numVerts, srfVert_t *verts, int numTriangles, srfTriangle_t *triangles, int dlightBits, int pshadowBits)
{
	int			i;
	srfTriangle_t  *tri;
	srfVert_t      *dv;
	float          *xyz, *normal, *texCoords, *lightCoords, *lightdir;
#ifdef USE_VERT_TANGENT_SPACE
	float          *tangent, *bitangent;
#endif
	glIndex_t      *index;
	float		*color;

	RB_CheckVBOandIBO(tess.vbo, tess.ibo);

	RB_CHECKOVERFLOW( numVerts, numTriangles * 3 );

	tri = triangles;
	index = &tess.indexes[ tess.numIndexes ];
	for ( i = 0 ; i < numTriangles ; i++, tri++ ) {
		*index++ = tess.numVertexes + tri->indexes[0];
		*index++ = tess.numVertexes + tri->indexes[1];
		*index++ = tess.numVertexes + tri->indexes[2];
	}
	tess.numIndexes += numTriangles * 3;

	if ( tess.shader->vertexAttribs & ATTR_POSITION )
	{
		dv = verts;
		xyz = tess.xyz[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, xyz+=4 )
			VectorCopy(dv->xyz, xyz);
	}

	if ( tess.shader->vertexAttribs & ATTR_NORMAL )
	{
		dv = verts;
		normal = tess.normal[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, normal+=4 )
			VectorCopy(dv->normal, normal);
	}

#ifdef USE_VERT_TANGENT_SPACE
	if ( tess.shader->vertexAttribs & ATTR_TANGENT )
	{
		dv = verts;
		tangent = tess.tangent[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, tangent+=4 )
			VectorCopy(dv->tangent, tangent);
	}

	if ( tess.shader->vertexAttribs & ATTR_BITANGENT )
	{
		dv = verts;
		bitangent = tess.bitangent[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, bitangent+=4 )
			VectorCopy(dv->bitangent, bitangent);
	}
#endif

	if ( tess.shader->vertexAttribs & ATTR_TEXCOORD )
	{
		dv = verts;
		texCoords = tess.texCoords[ tess.numVertexes ][0];
		for ( i = 0 ; i < numVerts ; i++, dv++, texCoords+=4 )
			VectorCopy2(dv->st, texCoords);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTCOORD )
	{
		dv = verts;
		lightCoords = tess.texCoords[ tess.numVertexes ][1];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightCoords+=4 )
			VectorCopy2(dv->lightmap, lightCoords);
	}

	if ( tess.shader->vertexAttribs & ATTR_COLOR )
	{
		dv = verts;
		color = tess.vertexColors[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, color+=4 )
			VectorCopy4(dv->vertexColors, color);
	}

	if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
	{
		dv = verts;
		lightdir = tess.lightdir[ tess.numVertexes ];
		for ( i = 0 ; i < numVerts ; i++, dv++, lightdir+=4 )
			VectorCopy(dv->lightdir, lightdir);
	}

#if 0  // nothing even uses vertex dlightbits
	for ( i = 0 ; i < numVerts ; i++ ) {
		tess.vertexDlightBits[ tess.numVertexes + i ] = dlightBits;
	}
#endif

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	tess.numVertexes += numVerts;
}

static qboolean RB_SurfaceHelperVBO(VBO_t *vbo, IBO_t *ibo, int numVerts, int numIndexes, int firstIndex, int dlightBits, int pshadowBits, qboolean shaderCheck)
{
	int i, mergeForward, mergeBack;
	GLvoid *firstIndexOffset, *lastIndexOffset;

	if (!vbo || !ibo)
	{
		return qfalse;
	}

	if (shaderCheck && !(!ShaderRequiresCPUDeforms(tess.shader) && !tess.shader->isSky && !tess.shader->isPortal))
	{
		return qfalse;
	}

	RB_CheckVBOandIBO(vbo, ibo);

	tess.dlightBits |= dlightBits;
	tess.pshadowBits |= pshadowBits;

	// merge this into any existing multidraw primitives
	mergeForward = -1;
	mergeBack = -1;
	firstIndexOffset = BUFFER_OFFSET(firstIndex * sizeof(GL_INDEX_TYPE));
	lastIndexOffset  = BUFFER_OFFSET((firstIndex + numIndexes) * sizeof(GL_INDEX_TYPE));

	if (r_mergeMultidraws->integer)
	{
		i = 0;

		if (r_mergeMultidraws->integer == 1)
		{
			// lazy merge, only check the last primitive
			if (tess.multiDrawPrimitives)
			{
				i = tess.multiDrawPrimitives - 1;
			}
		}

		for (; i < tess.multiDrawPrimitives; i++)
		{
			if (tess.multiDrawLastIndex[i] == firstIndexOffset)
			{
				mergeBack = i;
			}

			if (lastIndexOffset == tess.multiDrawFirstIndex[i])
			{
				mergeForward = i;
			}
		}
	}

	if (mergeBack != -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += numIndexes;
		tess.multiDrawLastIndex[mergeBack]   = (byte *)tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack] * sizeof(GL_INDEX_TYPE);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack == -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeForward] += numIndexes;
		tess.multiDrawFirstIndex[mergeForward]  = firstIndexOffset;
		tess.multiDrawLastIndex[mergeForward]   = (byte *)tess.multiDrawFirstIndex[mergeForward] + tess.multiDrawNumIndexes[mergeForward] * sizeof(GL_INDEX_TYPE);
		backEnd.pc.c_multidrawsMerged++;
	}
	else if (mergeBack != -1 && mergeForward != -1)
	{
		tess.multiDrawNumIndexes[mergeBack] += numIndexes + tess.multiDrawNumIndexes[mergeForward];
		tess.multiDrawLastIndex[mergeBack]   = (byte *)tess.multiDrawFirstIndex[mergeBack] + tess.multiDrawNumIndexes[mergeBack] * sizeof(GL_INDEX_TYPE);
		tess.multiDrawPrimitives--;

		if (mergeForward != tess.multiDrawPrimitives)
		{
			tess.multiDrawNumIndexes[mergeForward] = tess.multiDrawNumIndexes[tess.multiDrawPrimitives];
			tess.multiDrawFirstIndex[mergeForward] = tess.multiDrawFirstIndex[tess.multiDrawPrimitives];
		}
		backEnd.pc.c_multidrawsMerged += 2;
	}
	else if (mergeBack == -1 && mergeForward == -1)
	{
		tess.multiDrawNumIndexes[tess.multiDrawPrimitives] = numIndexes;
		tess.multiDrawFirstIndex[tess.multiDrawPrimitives] = firstIndexOffset;
		tess.multiDrawLastIndex[tess.multiDrawPrimitives] = lastIndexOffset;
		tess.multiDrawPrimitives++;
	}

	backEnd.pc.c_multidraws++;

	tess.numIndexes  += numIndexes;
	tess.numVertexes += numVerts;

	return qtrue;
}

/*
=============
RB_SurfaceTriangles
=============
*/
static void RB_SurfaceTriangles( srfTriangles_t *srf ) {
	if( RB_SurfaceHelperVBO (srf->vbo, srf->ibo, srf->numVerts, srf->numTriangles * 3, srf->firstIndex, srf->dlightBits[backEnd.smpFrame], srf->pshadowBits[backEnd.smpFrame], qtrue ) )
	{
		return;
	}

	RB_SurfaceHelper(srf->numVerts, srf->verts, srf->numTriangles, srf->triangles, srf->dlightBits[backEnd.smpFrame], srf->pshadowBits[backEnd.smpFrame]);
}



/*
==============
RB_SurfaceBeam
==============
*/
static void RB_SurfaceBeam( void )
{
#define NUM_BEAM_SEGS 6
	refEntity_t *e;
	int	i;
	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	e = &backEnd.currentEntity->e;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS ; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
//		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	GL_Bind( tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	// FIXME: Quake3 doesn't use this, so I never tested it
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	for ( i = 0; i <= NUM_BEAM_SEGS; i++ ) {
		VectorCopy(start_points[ i % NUM_BEAM_SEGS ], tess.xyz[tess.numVertexes++]);
		VectorCopy(end_points  [ i % NUM_BEAM_SEGS ], tess.xyz[tess.numVertexes++]);
	}

	for ( i = 0; i < NUM_BEAM_SEGS; i++ ) {
		tess.indexes[tess.numIndexes++] =       i      * 2;
		tess.indexes[tess.numIndexes++] =      (i + 1) * 2;
		tess.indexes[tess.numIndexes++] = 1  +  i      * 2;

		tess.indexes[tess.numIndexes++] = 1  +  i      * 2;
		tess.indexes[tess.numIndexes++] =      (i + 1) * 2;
		tess.indexes[tess.numIndexes++] = 1  + (i + 1) * 2;
	}

	// FIXME: A lot of this can probably be removed for speed, and refactored into a more convenient function
	RB_UpdateVBOs(ATTR_POSITION);
	
	{
		shaderProgram_t *sp = &tr.textureColorShader;
		vec4_t color;

		GLSL_VertexAttribsState(ATTR_POSITION);
		GLSL_BindProgram(sp);
		
		GLSL_SetUniformMatrix16(sp, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
					
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 1.0f;
		GLSL_SetUniformVec4(sp, TEXTURECOLOR_UNIFORM_COLOR, color);
	}

	R_DrawElementsVBO(tess.numIndexes, tess.firstIndex);

	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
}

//================================================================================

static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth )
{
	float		spanWidth2;
	int			vbase;
	float		t = len / 256.0f;

	vbase = tess.numVertexes;

	spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] * 0.25 / 255.0f;
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] * 0.25 / 255.0f;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] * 0.25 / 255.0f;
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] / 255.0f;
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] / 255.0f;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] / 255.0f;
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[tess.numVertexes] );

	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] / 255.0f;
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] / 255.0f;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] / 255.0f;
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[tess.numVertexes] );
	tess.texCoords[tess.numVertexes][0][0] = t;
	tess.texCoords[tess.numVertexes][0][1] = 1;
	tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] / 255.0f;
	tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] / 255.0f;
	tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] / 255.0f;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = vbase;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 2;

	tess.indexes[tess.numIndexes++] = vbase + 2;
	tess.indexes[tess.numIndexes++] = vbase + 1;
	tess.indexes[tess.numIndexes++] = vbase + 3;
}

static void DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up )
{
	int i;
	vec3_t	pos[4];
	vec3_t	v;
	int		spanWidth = r_railWidth->integer;
	float c, s;
	float		scale;

	if ( numSegs > 1 )
		numSegs--;
	if ( !numSegs )
		return;

	scale = 0.25;

	for ( i = 0; i < 4; i++ )
	{
		c = cos( DEG2RAD( 45 + i * 90 ) );
		s = sin( DEG2RAD( 45 + i * 90 ) );
		v[0] = ( right[0] * c + up[0] * s ) * scale * spanWidth;
		v[1] = ( right[1] * c + up[1] * s ) * scale * spanWidth;
		v[2] = ( right[2] * c + up[2] * s ) * scale * spanWidth;
		VectorAdd( start, v, pos[i] );

		if ( numSegs > 1 )
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[i], dir, pos[i] );
		}
	}

	for ( i = 0; i < numSegs; i++ )
	{
		int j;

		RB_CHECKOVERFLOW( 4, 6 );

		for ( j = 0; j < 4; j++ )
		{
			VectorCopy( pos[j], tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = ( j < 2 );
			tess.texCoords[tess.numVertexes][0][1] = ( j && j != 3 );
			tess.vertexColors[tess.numVertexes][0] = backEnd.currentEntity->e.shaderRGBA[0] / 255.0f;
			tess.vertexColors[tess.numVertexes][1] = backEnd.currentEntity->e.shaderRGBA[1] / 255.0f;
			tess.vertexColors[tess.numVertexes][2] = backEnd.currentEntity->e.shaderRGBA[2] / 255.0f;
			tess.numVertexes++;

			VectorAdd( pos[j], dir, pos[j] );
		}

		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 0;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 3;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 1;
		tess.indexes[tess.numIndexes++] = tess.numVertexes - 4 + 2;
	}
}

/*
** RB_SurfaceRailRinges
*/
static void RB_SurfaceRailRings( void ) {
	refEntity_t *e;
	int			numSegs;
	int			len;
	vec3_t		vec;
	vec3_t		right, up;
	vec3_t		start, end;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );
	MakeNormalVectors( vec, right, up );
	numSegs = ( len ) / r_railSegmentLength->value;
	if ( numSegs <= 0 ) {
		numSegs = 1;
	}

	VectorScale( vec, r_railSegmentLength->value, vec );

	DoRailDiscs( numSegs, start, vec, right, up );
}

/*
** RB_SurfaceRailCore
*/
static void RB_SurfaceRailCore( void ) {
	refEntity_t *e;
	int			len;
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.or.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.or.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	DoRailCore( start, end, right, len, r_railCoreWidth->integer );
}

/*
** RB_SurfaceLightningBolt
*/
static void RB_SurfaceLightningBolt( void ) {
	refEntity_t *e;
	int			len;
	vec3_t		right;
	vec3_t		vec;
	vec3_t		start, end;
	vec3_t		v1, v2;
	int			i;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, end );
	VectorCopy( e->origin, start );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.or.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.or.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	for ( i = 0 ; i < 4 ; i++ ) {
		vec3_t	temp;

		DoRailCore( start, end, right, len, 8 );
		RotatePointAroundVector( temp, vec, right, 45 );
		VectorCopy( temp, right );
	}
}

/*
** VectorArrayNormalize
*
* The inputs to this routing seem to always be close to length = 1.0 (about 0.6 to 2.0)
* This means that we don't have to worry about zero length or enormously long vectors.
*/
static void VectorArrayNormalize(vec4_t *normals, unsigned int count)
{
//    assert(count);
        
#if idppc
    {
        register float half = 0.5;
        register float one  = 1.0;
        float *components = (float *)normals;
        
        // Vanilla PPC code, but since PPC has a reciprocal square root estimate instruction,
        // runs *much* faster than calling sqrt().  We'll use a single Newton-Raphson
        // refinement step to get a little more precision.  This seems to yeild results
        // that are correct to 3 decimal places and usually correct to at least 4 (sometimes 5).
        // (That is, for the given input range of about 0.6 to 2.0).
        do {
            float x, y, z;
            float B, y0, y1;
            
            x = components[0];
            y = components[1];
            z = components[2];
            components += 4;
            B = x*x + y*y + z*z;

#ifdef __GNUC__            
            asm("frsqrte %0,%1" : "=f" (y0) : "f" (B));
#else
			y0 = __frsqrte(B);
#endif
            y1 = y0 + half*y0*(one - B*y0*y0);

            x = x * y1;
            y = y * y1;
            components[-4] = x;
            z = z * y1;
            components[-3] = y;
            components[-2] = z;
        } while(count--);
    }
#else // No assembly version for this architecture, or C_ONLY defined
	// given the input, it's safe to call VectorNormalizeFast
    while (count--) {
        VectorNormalizeFast(normals[0]);
        normals++;
    }
#endif

}



/*
** LerpMeshVertexes
*/
#if idppc_altivec
static void LerpMeshVertexes_altivec(md3Surface_t *surf, float backlerp)
{
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale QALIGN(16);
	float   newXyzScale QALIGN(16);
	float	oldNormalScale QALIGN(16);
	float newNormalScale QALIGN(16);
	int		vertNum;
	unsigned lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		vector signed short newNormalsVec0;
		vector signed short newNormalsVec1;
		vector signed int newNormalsIntVec;
		vector float newNormalsFloatVec;
		vector float newXyzScaleVec;
		vector unsigned char newNormalsLoadPermute;
		vector unsigned char newNormalsStorePermute;
		vector float zero;
		
		newNormalsStorePermute = vec_lvsl(0,(float *)&newXyzScaleVec);
		newXyzScaleVec = *(vector float *)&newXyzScale;
		newXyzScaleVec = vec_perm(newXyzScaleVec,newXyzScaleVec,newNormalsStorePermute);
		newXyzScaleVec = vec_splat(newXyzScaleVec,0);		
		newNormalsLoadPermute = vec_lvsl(0,newXyz);
		newNormalsStorePermute = vec_lvsr(0,outXyz);
		zero = (vector float)vec_splat_s8(0);
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			newNormalsLoadPermute = vec_lvsl(0,newXyz);
			newNormalsStorePermute = vec_lvsr(0,outXyz);
			newNormalsVec0 = vec_ld(0,newXyz);
			newNormalsVec1 = vec_ld(16,newXyz);
			newNormalsVec0 = vec_perm(newNormalsVec0,newNormalsVec1,newNormalsLoadPermute);
			newNormalsIntVec = vec_unpackh(newNormalsVec0);
			newNormalsFloatVec = vec_ctf(newNormalsIntVec,0);
			newNormalsFloatVec = vec_madd(newNormalsFloatVec,newXyzScaleVec,zero);
			newNormalsFloatVec = vec_perm(newNormalsFloatVec,newNormalsFloatVec,newNormalsStorePermute);
			//outXyz[0] = newXyz[0] * newXyzScale;
			//outXyz[1] = newXyz[1] * newXyzScale;
			//outXyz[2] = newXyz[2] * newXyzScale;

			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			vec_ste(newNormalsFloatVec,0,outXyz);
			vec_ste(newNormalsFloatVec,4,outXyz);
			vec_ste(newNormalsFloatVec,8,outXyz);
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;
			uncompressedNewNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			lat = ( oldNormals[0] >> 8 ) & 0xff;
			lng = ( oldNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;

			uncompressedOldNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

//			VectorNormalize (outNormal);
		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
}
#endif

static void LerpMeshVertexes_scalar(mdvSurface_t *surf, float backlerp)
{
#if 0
	short	*oldXyz, *newXyz, *oldNormals, *newNormals;
	float	*outXyz, *outNormal;
	float	oldXyzScale, newXyzScale;
	float	oldNormalScale, newNormalScale;
	int		vertNum;
	unsigned lat, lng;
	int		numVerts;

	outXyz = tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	newXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
		+ (backEnd.currentEntity->e.frame * surf->numVerts * 4);
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * (1.0 - backlerp);
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 ) {
		//
		// just copy the vertexes
		//
		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			newXyz += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{

			outXyz[0] = newXyz[0] * newXyzScale;
			outXyz[1] = newXyz[1] * newXyzScale;
			outXyz[2] = newXyz[2] * newXyzScale;

			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= (FUNCTABLE_SIZE/256);
			lng *= (FUNCTABLE_SIZE/256);

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			outNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			outNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}
	} else {
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = (short *)((byte *)surf + surf->ofsXyzNormals)
			+ (backEnd.currentEntity->e.oldframe * surf->numVerts * 4);
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for (vertNum=0 ; vertNum < numVerts ; vertNum++,
			oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4,
			outXyz += 4, outNormal += 4) 
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[0] = oldXyz[0] * oldXyzScale + newXyz[0] * newXyzScale;
			outXyz[1] = oldXyz[1] * oldXyzScale + newXyz[1] * newXyzScale;
			outXyz[2] = oldXyz[2] * oldXyzScale + newXyz[2] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			lat = ( newNormals[0] >> 8 ) & 0xff;
			lng = ( newNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;
			uncompressedNewNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedNewNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedNewNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			lat = ( oldNormals[0] >> 8 ) & 0xff;
			lng = ( oldNormals[0] & 0xff );
			lat *= 4;
			lng *= 4;

			uncompressedOldNormal[0] = tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			uncompressedOldNormal[1] = tr.sinTable[lat] * tr.sinTable[lng];
			uncompressedOldNormal[2] = tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];

			outNormal[0] = uncompressedOldNormal[0] * oldNormalScale + uncompressedNewNormal[0] * newNormalScale;
			outNormal[1] = uncompressedOldNormal[1] * oldNormalScale + uncompressedNewNormal[1] * newNormalScale;
			outNormal[2] = uncompressedOldNormal[2] * oldNormalScale + uncompressedNewNormal[2] * newNormalScale;

//			VectorNormalize (outNormal);
		}
    	VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], numVerts);
   	}
#endif
	float *outXyz, *outNormal;
	mdvVertex_t *newVerts;
	int		vertNum;

	newVerts = surf->verts + backEnd.currentEntity->e.frame * surf->numVerts;

	outXyz =    tess.xyz[tess.numVertexes];
	outNormal = tess.normal[tess.numVertexes];

	if (backlerp == 0)
	{
		//
		// just copy the vertexes
		//

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			VectorCopy(newVerts->xyz,    outXyz);
			VectorCopy(newVerts->normal, outNormal);
			newVerts++;
			outXyz += 4;
			outNormal += 4;
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//

		mdvVertex_t *oldVerts;

		oldVerts = surf->verts + backEnd.currentEntity->e.oldframe * surf->numVerts;

		for (vertNum=0 ; vertNum < surf->numVerts ; vertNum++)
		{
			VectorLerp(newVerts->xyz,    oldVerts->xyz,    backlerp, outXyz);
			VectorLerp(newVerts->normal, oldVerts->normal, backlerp, outNormal);
			//VectorNormalize(outNormal);
			newVerts++;
			oldVerts++;
			outXyz += 4;
			outNormal += 4;
		}
		VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes], surf->numVerts);
	}

}

static void LerpMeshVertexes(mdvSurface_t *surf, float backlerp)
{
#if 0
#if idppc_altivec
	if (com_altivec->integer) {
		// must be in a seperate function or G3 systems will crash.
		LerpMeshVertexes_altivec( surf, backlerp );
		return;
	}
#endif // idppc_altivec
#endif
	LerpMeshVertexes_scalar( surf, backlerp );
}


/*
=============
RB_SurfaceMesh
=============
*/
static void RB_SurfaceMesh(mdvSurface_t *surface) {
	int				j;
	float			backlerp;
	srfTriangle_t 	*triangles;
	mdvSt_t			*texCoords;
	int				indexes;
	int				Bob, Doug;
	int				numVerts;

	if (  backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame ) {
		backlerp = 0;
	} else  {
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles*3 );

	LerpMeshVertexes (surface, backlerp);

	triangles = surface->triangles;
	indexes = surface->numTriangles * 3;
	Bob = tess.numIndexes;
	Doug = tess.numVertexes;
	for (j = 0 ; j < surface->numTriangles ; j++) {
		tess.indexes[Bob + j*3 + 0] = Doug + triangles[j].indexes[0];
		tess.indexes[Bob + j*3 + 1] = Doug + triangles[j].indexes[1];
		tess.indexes[Bob + j*3 + 2] = Doug + triangles[j].indexes[2];
	}
	tess.numIndexes += indexes;

	texCoords = surface->st;

	numVerts = surface->numVerts;
	for ( j = 0; j < numVerts; j++ ) {
		tess.texCoords[Doug + j][0][0] = texCoords[j].st[0];
		tess.texCoords[Doug + j][0][1] = texCoords[j].st[1];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;

}


/*
==============
RB_SurfaceFace
==============
*/
static void RB_SurfaceFace( srfSurfaceFace_t *srf ) {
	if( RB_SurfaceHelperVBO (srf->vbo, srf->ibo, srf->numVerts, srf->numTriangles * 3, srf->firstIndex, srf->dlightBits[backEnd.smpFrame], srf->pshadowBits[backEnd.smpFrame], qtrue ) )
	{
		return;
	}

	RB_SurfaceHelper(srf->numVerts, srf->verts, srf->numTriangles, srf->triangles, srf->dlightBits[backEnd.smpFrame], srf->pshadowBits[backEnd.smpFrame]);
}


static float	LodErrorForVolume( vec3_t local, float radius ) {
	vec3_t		world;
	float		d;

	// never let it go negative
	if ( r_lodCurveError->value < 0 ) {
		return 0;
	}

	world[0] = local[0] * backEnd.or.axis[0][0] + local[1] * backEnd.or.axis[1][0] + 
		local[2] * backEnd.or.axis[2][0] + backEnd.or.origin[0];
	world[1] = local[0] * backEnd.or.axis[0][1] + local[1] * backEnd.or.axis[1][1] + 
		local[2] * backEnd.or.axis[2][1] + backEnd.or.origin[1];
	world[2] = local[0] * backEnd.or.axis[0][2] + local[1] * backEnd.or.axis[1][2] + 
		local[2] * backEnd.or.axis[2][2] + backEnd.or.origin[2];

	VectorSubtract( world, backEnd.viewParms.or.origin, world );
	d = DotProduct( world, backEnd.viewParms.or.axis[0] );

	if ( d < 0 ) {
		d = -d;
	}
	d -= radius;
	if ( d < 1 ) {
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
static void RB_SurfaceGrid( srfGridMesh_t *srf ) {
	int		i, j;
	float	*xyz;
	float	*texCoords, *lightCoords;
	float	*normal;
#ifdef USE_VERT_TANGENT_SPACE
	float   *tangent, *bitangent;
#endif
	float   *color, *lightdir;
	srfVert_t	*dv;
	int		rows, irows, vrows;
	int		used;
	int		widthTable[MAX_GRID_SIZE];
	int		heightTable[MAX_GRID_SIZE];
	float	lodError;
	int		lodWidth, lodHeight;
	int		numVertexes;
	int		dlightBits;
	int     pshadowBits;
	//int		*vDlightBits;

	if( RB_SurfaceHelperVBO (srf->vbo, srf->ibo, srf->numVerts, srf->numTriangles * 3, srf->firstIndex, srf->dlightBits[backEnd.smpFrame], srf->pshadowBits[backEnd.smpFrame], qtrue ) )
	{
		return;
	}

	dlightBits = srf->dlightBits[backEnd.smpFrame];
	tess.dlightBits |= dlightBits;

	pshadowBits = srf->pshadowBits[backEnd.smpFrame];
	tess.pshadowBits |= pshadowBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( srf->lodOrigin, srf->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[0] = 0;
	lodWidth = 1;
	for ( i = 1 ; i < srf->width-1 ; i++ ) {
		if ( srf->widthLodError[i] <= lodError ) {
			widthTable[lodWidth] = i;
			lodWidth++;
		}
	}
	widthTable[lodWidth] = srf->width-1;
	lodWidth++;

	heightTable[0] = 0;
	lodHeight = 1;
	for ( i = 1 ; i < srf->height-1 ; i++ ) {
		if ( srf->heightLodError[i] <= lodError ) {
			heightTable[lodHeight] = i;
			lodHeight++;
		}
	}
	heightTable[lodHeight] = srf->height-1;
	lodHeight++;


	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	while ( used < lodHeight - 1 ) {
		// see how many rows of both verts and indexes we can add without overflowing
		do {
			vrows = ( SHADER_MAX_VERTEXES - tess.numVertexes ) / lodWidth;
			irows = ( SHADER_MAX_INDEXES - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 ) {
				RB_EndSurface();
				RB_BeginSurface(tess.shader, tess.fogNum );
			} else {
				break;
			}
		} while ( 1 );
		
		rows = irows;
		if ( vrows < irows + 1 ) {
			rows = vrows - 1;
		}
		if ( used + rows > lodHeight ) {
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz = tess.xyz[numVertexes];
		normal = tess.normal[numVertexes];
#ifdef USE_VERT_TANGENT_SPACE
		tangent = tess.tangent[numVertexes];
		bitangent = tess.bitangent[numVertexes];
#endif
		texCoords = tess.texCoords[numVertexes][0];
		lightCoords = tess.texCoords[numVertexes][1];
		color = tess.vertexColors[numVertexes];
		lightdir = tess.lightdir[numVertexes];
		//vDlightBits = &tess.vertexDlightBits[numVertexes];

		for ( i = 0 ; i < rows ; i++ ) {
			for ( j = 0 ; j < lodWidth ; j++ ) {
				dv = srf->verts + heightTable[ used + i ] * srf->width
					+ widthTable[ j ];

				if ( tess.shader->vertexAttribs & ATTR_POSITION )
				{
					VectorCopy(dv->xyz, xyz);
					xyz += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_NORMAL )
				{
					VectorCopy(dv->normal, normal);
					normal += 4;
				}

#ifdef USE_VERT_TANGENT_SPACE
				if ( tess.shader->vertexAttribs & ATTR_TANGENT )
				{
					VectorCopy(dv->tangent, tangent);
					tangent += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_BITANGENT )
				{
					VectorCopy(dv->bitangent, bitangent);
					bitangent += 4;
				}
#endif
				if ( tess.shader->vertexAttribs & ATTR_TEXCOORD )
				{
					VectorCopy2(dv->st, texCoords);
					texCoords += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTCOORD )
				{
					VectorCopy2(dv->lightmap, lightCoords);
					lightCoords += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_COLOR )
				{
					VectorCopy4(dv->vertexColors, color);
					color += 4;
				}

				if ( tess.shader->vertexAttribs & ATTR_LIGHTDIRECTION )
				{
					VectorCopy(dv->lightdir, lightdir);
					lightdir += 4;
				}

				//*vDlightBits++ = dlightBits;
			}
		}


		// add the indexes
		{
			int		numIndexes;
			int		w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;
			for (i = 0 ; i < h ; i++) {
				for (j = 0 ; j < w ; j++) {
					int		v1, v2, v3, v4;
			
					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i*lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[numIndexes] = v2;
					tess.indexes[numIndexes+1] = v3;
					tess.indexes[numIndexes+2] = v1;
					
					tess.indexes[numIndexes+3] = v1;
					tess.indexes[numIndexes+4] = v3;
					tess.indexes[numIndexes+5] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}


/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
static void RB_SurfaceAxis( void ) {
	// FIXME: implement this
#if 0
	GL_Bind( tr.whiteImage );
	qglLineWidth( 3 );
	qglBegin( GL_LINES );
	qglColor3f( 1,0,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 16,0,0 );
	qglColor3f( 0,1,0 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,16,0 );
	qglColor3f( 0,0,1 );
	qglVertex3f( 0,0,0 );
	qglVertex3f( 0,0,16 );
	qglEnd();
	qglLineWidth( 1 );
#endif
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
static void RB_SurfaceEntity( surfaceType_t *surfType ) {
	switch( backEnd.currentEntity->e.reType ) {
	case RT_SPRITE:
		RB_SurfaceSprite();
		break;
	case RT_BEAM:
		RB_SurfaceBeam();
		break;
	case RT_RAIL_CORE:
		RB_SurfaceRailCore();
		break;
	case RT_RAIL_RINGS:
		RB_SurfaceRailRings();
		break;
	case RT_LIGHTNING:
		RB_SurfaceLightningBolt();
		break;
	default:
		RB_SurfaceAxis();
		break;
	}
	return;
}

static void RB_SurfaceBad( surfaceType_t *surfType ) {
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

static void RB_SurfaceFlare(srfFlare_t *surf)
{
	if (r_flares->integer)
		RB_AddFlare(surf, tess.fogNum, surf->origin, surf->color, surf->normal);
}

static void RB_SurfaceVBOMesh(srfVBOMesh_t * srf)
{
	RB_SurfaceHelperVBO (srf->vbo, srf->ibo, srf->numVerts, srf->numIndexes, srf->firstIndex, srf->dlightBits[backEnd.smpFrame], srf->pshadowBits[backEnd.smpFrame], qfalse );
}

void RB_SurfaceVBOMDVMesh(srfVBOMDVMesh_t * surface)
{
	//mdvModel_t     *mdvModel;
	//mdvSurface_t   *mdvSurface;
	refEntity_t    *refEnt;

	GLimp_LogComment("--- RB_SurfaceVBOMDVMesh ---\n");

	if(!surface->vbo || !surface->ibo)
		return;

	//RB_CheckVBOandIBO(surface->vbo, surface->ibo);
	RB_EndSurface();
	RB_BeginSurface(tess.shader, tess.fogNum);

	R_BindVBO(surface->vbo);
	R_BindIBO(surface->ibo);

	tess.useInternalVBO = qfalse;

	tess.numIndexes += surface->numIndexes;
	tess.numVertexes += surface->numVerts;

	//mdvModel = surface->mdvModel;
	//mdvSurface = surface->mdvSurface;

	refEnt = &backEnd.currentEntity->e;

	if(refEnt->oldframe == refEnt->frame)
	{
		glState.vertexAttribsInterpolation = 0;
	}
	else
	{
		glState.vertexAttribsInterpolation = refEnt->backlerp;
	}

	glState.vertexAttribsOldFrame = refEnt->oldframe;
	glState.vertexAttribsNewFrame = refEnt->frame;

	RB_EndSurface();

	// So we don't lerp surfaces that shouldn't be lerped
	glState.vertexAttribsInterpolation = 0;
}

static void RB_SurfaceDisplayList( srfDisplayList_t *surf ) {
	// all apropriate state must be set in RB_BeginSurface
	// this isn't implemented yet...
	qglCallList( surf->listNum );
}

static void RB_SurfaceSkip( void *surf ) {
}


void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])( void *) = {
	(void(*)(void*))RB_SurfaceBad,			// SF_BAD, 
	(void(*)(void*))RB_SurfaceSkip,			// SF_SKIP, 
	(void(*)(void*))RB_SurfaceFace,			// SF_FACE,
	(void(*)(void*))RB_SurfaceGrid,			// SF_GRID,
	(void(*)(void*))RB_SurfaceTriangles,		// SF_TRIANGLES,
	(void(*)(void*))RB_SurfacePolychain,		// SF_POLY,
	(void(*)(void*))RB_SurfaceMesh,			// SF_MDV,
	(void(*)(void*))RB_SurfaceAnim,			// SF_MD4,
#ifdef RAVENMD4
	(void(*)(void*))RB_MDRSurfaceAnim,		// SF_MDR,
#endif
	(void(*)(void*))RB_IQMSurfaceAnim,		// SF_IQM,
	(void(*)(void*))RB_SurfaceFlare,		// SF_FLARE,
	(void(*)(void*))RB_SurfaceEntity,		// SF_ENTITY
	(void(*)(void*))RB_SurfaceDisplayList,		// SF_DISPLAY_LIST
	(void(*)(void*))RB_SurfaceVBOMesh,	    // SF_VBO_MESH,
	(void(*)(void*))RB_SurfaceVBOMDVMesh,   // SF_VBO_MDVMESH
};
