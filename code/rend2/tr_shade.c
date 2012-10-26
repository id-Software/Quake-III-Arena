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
// tr_shade.c

#include "tr_local.h" 
#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif

/*

  THIS ENTIRE FILE IS BACK END

  This file deals with applying shaders to surface data in the tess struct.
*/


/*
==================
R_DrawElements

==================
*/

void R_DrawElementsVBO( int numIndexes, int firstIndex )
{
	if (glRefConfig.drawRangeElements)
		qglDrawRangeElementsEXT(GL_TRIANGLES, 0, numIndexes, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(firstIndex * sizeof(GL_INDEX_TYPE)));
	else
		qglDrawElements(GL_TRIANGLES, numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(firstIndex * sizeof(GL_INDEX_TYPE)));
	
}


static void R_DrawMultiElementsVBO( int multiDrawPrimitives, const GLvoid **multiDrawFirstIndex, GLsizei *multiDrawNumIndexes )
{
	if (glRefConfig.multiDrawArrays)
	{
		qglMultiDrawElementsEXT(GL_TRIANGLES, multiDrawNumIndexes, GL_INDEX_TYPE, multiDrawFirstIndex, multiDrawPrimitives);
	}
	else
	{
		int i;

		if (glRefConfig.drawRangeElements)
		{
			for (i = 0; i < multiDrawPrimitives; i++)
			{
				qglDrawRangeElementsEXT(GL_TRIANGLES, 0, multiDrawNumIndexes[i],  multiDrawNumIndexes[i], GL_INDEX_TYPE, multiDrawFirstIndex[i]);
			}
		}
		else
		{
			for (i = 0; i < multiDrawPrimitives; i++)
			{
				qglDrawElements(GL_TRIANGLES, multiDrawNumIndexes[i], GL_INDEX_TYPE, multiDrawFirstIndex[i]);
			}
		}
	}
}


/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t	tess;


/*
=================
R_BindAnimatedImageToTMU

=================
*/
static void R_BindAnimatedImageToTMU( textureBundle_t *bundle, int tmu ) {
	int		index;

	if ( bundle->isVideoMap ) {
		int oldtmu = glState.currenttmu;
		GL_SelectTexture(tmu);
		ri.CIN_RunCinematic(bundle->videoMapHandle);
		ri.CIN_UploadCinematic(bundle->videoMapHandle);
		GL_SelectTexture(oldtmu);
		return;
	}

	if ( bundle->numImageAnimations <= 1 ) {
		GL_BindToTMU( bundle->image[0], tmu);
		return;
	}

	// it is necessary to do this messy calc to make sure animations line up
	// exactly with waveforms of the same frequency
	index = ri.ftol(tess.shaderTime * bundle->imageAnimationSpeed * FUNCTABLE_SIZE);
	index >>= FUNCTABLE_SIZE2;

	if ( index < 0 ) {
		index = 0;	// may happen with shader time offsets
	}
	index %= bundle->numImageAnimations;

	GL_BindToTMU( bundle->image[ index ], tmu );
}


/*
================
DrawTris

Draws triangle outlines for debugging
================
*/
static void DrawTris (shaderCommands_t *input) {
	GL_Bind( tr.whiteImage );

	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglDepthRange( 0, 0 );

	{
		shaderProgram_t *sp = &tr.textureColorShader;
		vec4_t color;

		GLSL_VertexAttribsState(ATTR_POSITION);
		GLSL_BindProgram(sp);
		
		GLSL_SetUniformMatrix16(sp, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		VectorSet4(color, 1, 1, 1, 1);
		GLSL_SetUniformVec4(sp, TEXTURECOLOR_UNIFORM_COLOR, color);

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, (const GLvoid **)input->multiDrawFirstIndex, input->multiDrawNumIndexes);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex);
		}
	}

	qglDepthRange( 0, 1 );
}


/*
================
DrawNormals

Draws vertex normals for debugging
================
*/
static void DrawNormals (shaderCommands_t *input) {
	//FIXME: implement this
}

/*
==============
RB_BeginSurface

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a RB_End due
to overflow.
==============
*/
void RB_BeginSurface( shader_t *shader, int fogNum ) {

	shader_t *state = (shader->remappedShader) ? shader->remappedShader : shader;

	tess.numIndexes = 0;
	tess.firstIndex = 0;
	tess.numVertexes = 0;
	tess.multiDrawPrimitives = 0;
	tess.shader = state;
	tess.fogNum = fogNum;
	tess.dlightBits = 0;		// will be OR'd in by surface functions
	tess.pshadowBits = 0;       // will be OR'd in by surface functions
	tess.xstages = state->stages;
	tess.numPasses = state->numUnfoggedPasses;
	tess.currentStageIteratorFunc = state->optimalStageIteratorFunc;
	tess.useInternalVBO = qtrue;

	tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
	if (tess.shader->clampTime && tess.shaderTime >= tess.shader->clampTime) {
		tess.shaderTime = tess.shader->clampTime;
	}

	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		tess.currentStageIteratorFunc = RB_StageIteratorGeneric;
	}
}



extern float EvalWaveForm( const waveForm_t *wf );
extern float EvalWaveFormClamped( const waveForm_t *wf );


static void ComputeTexMatrix( shaderStage_t *pStage, int bundleNum, float *outmatrix)
{
	int tm;
	float matrix[16], currentmatrix[16];
	textureBundle_t *bundle = &pStage->bundle[bundleNum];

	Matrix16Identity(outmatrix);
	Matrix16Identity(currentmatrix);

	for ( tm = 0; tm < bundle->numTexMods ; tm++ ) {
		switch ( bundle->texMods[tm].type )
		{
			
		case TMOD_NONE:
			tm = TR_MAX_TEXMODS;		// break out of for loop
			break;

		case TMOD_TURBULENT:
			RB_CalcTurbulentTexMatrix( &bundle->texMods[tm].wave, 
									 matrix );
			outmatrix[12] = matrix[12];
			outmatrix[13] = matrix[13];
			Matrix16Copy(outmatrix, currentmatrix);
			break;

		case TMOD_ENTITY_TRANSLATE:
			RB_CalcScrollTexMatrix( backEnd.currentEntity->e.shaderTexCoord,
								 matrix );
			Matrix16Multiply(matrix, currentmatrix, outmatrix);
			Matrix16Copy(outmatrix, currentmatrix);
			break;

		case TMOD_SCROLL:
			RB_CalcScrollTexMatrix( bundle->texMods[tm].scroll,
									 matrix );
			Matrix16Multiply(matrix, currentmatrix, outmatrix);
			Matrix16Copy(outmatrix, currentmatrix);
			break;

		case TMOD_SCALE:
			RB_CalcScaleTexMatrix( bundle->texMods[tm].scale,
								  matrix );
			Matrix16Multiply(matrix, currentmatrix, outmatrix);
			Matrix16Copy(outmatrix, currentmatrix);
			break;
		
		case TMOD_STRETCH:
			RB_CalcStretchTexMatrix( &bundle->texMods[tm].wave, 
								   matrix );
			Matrix16Multiply(matrix, currentmatrix, outmatrix);
			Matrix16Copy(outmatrix, currentmatrix);
			break;

		case TMOD_TRANSFORM:
			RB_CalcTransformTexMatrix( &bundle->texMods[tm],
									 matrix );
			Matrix16Multiply(matrix, currentmatrix, outmatrix);
			Matrix16Copy(outmatrix, currentmatrix);
			break;

		case TMOD_ROTATE:
			RB_CalcRotateTexMatrix( bundle->texMods[tm].rotateSpeed,
									matrix );
			Matrix16Multiply(matrix, currentmatrix, outmatrix);
			Matrix16Copy(outmatrix, currentmatrix);
			break;

		default:
			ri.Error( ERR_DROP, "ERROR: unknown texmod '%d' in shader '%s'\n", bundle->texMods[tm].type, tess.shader->name );
			break;
		}
	}
}


static void ComputeDeformValues(int *deformGen, vec5_t deformParams)
{
	// u_DeformGen
	*deformGen = DGEN_NONE;
	if(!ShaderRequiresCPUDeforms(tess.shader))
	{
		deformStage_t  *ds;

		// only support the first one
		ds = &tess.shader->deforms[0];

		switch (ds->deformation)
		{
			case DEFORM_WAVE:
				*deformGen = ds->deformationWave.func;

				deformParams[0] = ds->deformationWave.base;
				deformParams[1] = ds->deformationWave.amplitude;
				deformParams[2] = ds->deformationWave.phase;
				deformParams[3] = ds->deformationWave.frequency;
				deformParams[4] = ds->deformationSpread;
				break;

			case DEFORM_BULGE:
				*deformGen = DGEN_BULGE;

				deformParams[0] = 0;
				deformParams[1] = ds->bulgeHeight; // amplitude
				deformParams[2] = ds->bulgeWidth;  // phase
				deformParams[3] = ds->bulgeSpeed;  // frequency
				deformParams[4] = 0;
				break;

			default:
				break;
		}
	}
}


static void ProjectDlightTexture( void ) {
	int		l;
	vec3_t	origin;
	float	scale;
	float	radius;
	int deformGen;
	vec5_t deformParams;

	if ( !backEnd.refdef.num_dlights ) {
		return;
	}

	ComputeDeformValues(&deformGen, deformParams);

	for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
		dlight_t	*dl;
		shaderProgram_t *sp;
		vec4_t vector;

		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}

		dl = &backEnd.refdef.dlights[l];
		VectorCopy( dl->transformed, origin );
		radius = dl->radius;
		scale = 1.0f / radius;

		sp = &tr.dlightallShader;

		backEnd.pc.c_dlightDraws++;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, DLIGHT_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		GLSL_SetUniformFloat(sp, DLIGHT_UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);
		
		GLSL_SetUniformInt(sp, DLIGHT_UNIFORM_DEFORMGEN, deformGen);
		if (deformGen != DGEN_NONE)
		{
			GLSL_SetUniformFloat5(sp, DLIGHT_UNIFORM_DEFORMPARAMS, deformParams);
			GLSL_SetUniformFloat(sp, DLIGHT_UNIFORM_TIME, tess.shaderTime);
		}

		vector[0] = dl->color[0];
		vector[1] = dl->color[1];
		vector[2] = dl->color[2];
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, DLIGHT_UNIFORM_COLOR, vector);

		vector[0] = origin[0];
		vector[1] = origin[1];
		vector[2] = origin[2];
		vector[3] = scale;
		GLSL_SetUniformVec4(sp, DLIGHT_UNIFORM_DLIGHTINFO, vector);
	  
		GL_Bind( tr.dlightImage );

		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		if ( dl->additive ) {
			GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}
		else {
			GL_State( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );
		}

		if (tess.multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(tess.multiDrawPrimitives, (const GLvoid **)tess.multiDrawFirstIndex, tess.multiDrawNumIndexes);
		}
		else
		{
			R_DrawElementsVBO(tess.numIndexes, tess.firstIndex);
		}

		backEnd.pc.c_totalIndexes += tess.numIndexes;
		backEnd.pc.c_dlightIndexes += tess.numIndexes;
	}
}


static void ComputeShaderColors( shaderStage_t *pStage, vec4_t baseColor, vec4_t vertColor )
{
	//
	// rgbGen
	//
	switch ( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 1.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_IDENTITY_LIGHTING:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] = tr.identityLight;
			baseColor[3] = 1.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_EXACT_VERTEX:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 1.0f;
			break;
		case CGEN_EXACT_VERTEX_LIT:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] = 1.0f;
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = 0.0f;
			vertColor[3] = 1.0f;
			break;
		case CGEN_CONST:
			baseColor[0] = pStage->constantColor[0] / 255.0f;
			baseColor[1] = pStage->constantColor[1] / 255.0f;
			baseColor[2] = pStage->constantColor[2] / 255.0f;
			baseColor[3] = pStage->constantColor[3] / 255.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_VERTEX:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = tr.identityLight;
			vertColor[3] = 1.0f;
			break;
		case CGEN_VERTEX_LIT:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] = tr.identityLight;
			baseColor[3] = 0.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = 0.0f;
			vertColor[3] = 1.0f;
			break;
		case CGEN_ONE_MINUS_VERTEX:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] = tr.identityLight;
			baseColor[3] = 1.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] = -tr.identityLight;
			vertColor[3] = 0.0f;
			break;
		case CGEN_FOG:
			{
				fog_t		*fog;

				fog = tr.world->fogs + tess.fogNum;

				baseColor[0] = ((unsigned char *)(&fog->colorInt))[0] / 255.0f;
				baseColor[1] = ((unsigned char *)(&fog->colorInt))[1] / 255.0f;
				baseColor[2] = ((unsigned char *)(&fog->colorInt))[2] / 255.0f;
				baseColor[3] = ((unsigned char *)(&fog->colorInt))[3] / 255.0f;
			}

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_WAVEFORM:
			baseColor[0] = 
			baseColor[1] = 
			baseColor[2] = RB_CalcWaveColorSingle( &pStage->rgbWave );
			baseColor[3] = 1.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			
			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[0] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[0] / 255.0f;
				baseColor[1] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[1] / 255.0f;
				baseColor[2] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[2] / 255.0f;
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
		case CGEN_LIGHTING_DIFFUSE:
		case CGEN_BAD:
			baseColor[0] = 
			baseColor[1] =
			baseColor[2] =
			baseColor[3] = 1.0f;

			vertColor[0] =
			vertColor[1] =
			vertColor[2] =
			vertColor[3] = 0.0f;
			break;
	}

	//
	// alphaGen
	//
	switch ( pStage->alphaGen )
	{
		case AGEN_SKIP:
			break;
		case AGEN_IDENTITY:
			baseColor[3] = 1.0f;
			vertColor[3] = 0.0f;
			break;
		case AGEN_CONST:
			baseColor[3] = pStage->constantColor[3] / 255.0f;
			vertColor[3] = 0.0f;
			break;
		case AGEN_WAVEFORM:
			baseColor[3] = RB_CalcWaveAlphaSingle( &pStage->alphaWave );
			vertColor[3] = 0.0f;
			break;
		case AGEN_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_ONE_MINUS_ENTITY:
			if (backEnd.currentEntity)
			{
				baseColor[3] = 1.0f - ((unsigned char *)backEnd.currentEntity->e.shaderRGBA)[3] / 255.0f;
			}
			vertColor[3] = 0.0f;
			break;
		case AGEN_VERTEX:
			baseColor[3] = 0.0f;
			vertColor[3] = 1.0f;
			break;
		case AGEN_ONE_MINUS_VERTEX:
			baseColor[3] = 1.0f;
			vertColor[3] = -1.0f;
			break;
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
		case AGEN_FRESNEL:
			// Done entirely in vertex program
			baseColor[3] = 1.0f;
			vertColor[3] = 0.0f;
			break;
	}

	// FIXME: find some way to implement this.
#if 0
	// if in greyscale rendering mode turn all color values into greyscale.
	if(r_greyscale->integer)
	{
		int scale;
		
		for(i = 0; i < tess.numVertexes; i++)
		{
			scale = (tess.svars.colors[i][0] + tess.svars.colors[i][1] + tess.svars.colors[i][2]) / 3;
			tess.svars.colors[i][0] = tess.svars.colors[i][1] = tess.svars.colors[i][2] = scale;
		}
	}
#endif
}


static void ComputeFogValues(vec4_t fogDistanceVector, vec4_t fogDepthVector, float *eyeT)
{
	// from RB_CalcFogTexCoords()
	fog_t  *fog;
	vec3_t  local;

	if (!tess.fogNum)
		return;

	fog = tr.world->fogs + tess.fogNum;

	VectorSubtract( backEnd.or.origin, backEnd.viewParms.or.origin, local );
	fogDistanceVector[0] = -backEnd.or.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.or.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.or.modelMatrix[10];
	fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.or.axis[0] );

	// scale the fog vectors based on the fog's thickness
	VectorScale4(fogDistanceVector, fog->tcScale, fogDistanceVector);

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector[0] = fog->surface[0] * backEnd.or.axis[0][0] + 
			fog->surface[1] * backEnd.or.axis[0][1] + fog->surface[2] * backEnd.or.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.or.axis[1][0] + 
			fog->surface[1] * backEnd.or.axis[1][1] + fog->surface[2] * backEnd.or.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.or.axis[2][0] + 
			fog->surface[1] * backEnd.or.axis[2][1] + fog->surface[2] * backEnd.or.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.or.origin, fog->surface );

		*eyeT = DotProduct( backEnd.or.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	} else {
		*eyeT = 1;	// non-surface fog always has eye inside
	}
}


static void ComputeFogColorMask( shaderStage_t *pStage, vec4_t fogColorMask )
{
	switch(pStage->adjustColorsForFog)
	{
		case ACFF_MODULATE_RGB:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] = 1.0f;
			fogColorMask[3] = 0.0f;
			break;
		case ACFF_MODULATE_ALPHA:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] = 0.0f;
			fogColorMask[3] = 1.0f;
			break;
		case ACFF_MODULATE_RGBA:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] =
			fogColorMask[3] = 1.0f;
			break;
		default:
			fogColorMask[0] =
			fogColorMask[1] =
			fogColorMask[2] =
			fogColorMask[3] = 0.0f;
			break;
	}
}


static void ForwardDlight( void ) {
	int		l;
	//vec3_t	origin;
	//float	scale;
	float	radius;

	int deformGen;
	vec5_t deformParams;
	
	vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float eyeT = 0;

	shaderCommands_t *input = &tess;
	shaderStage_t *pStage = tess.xstages[0];

	if ( !backEnd.refdef.num_dlights ) {
		return;
	}
	
	ComputeDeformValues(&deformGen, deformParams);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

	for ( l = 0 ; l < backEnd.refdef.num_dlights ; l++ ) {
		dlight_t	*dl;
		shaderProgram_t *sp;
		vec4_t vector;
		matrix_t matrix;

		if ( !( tess.dlightBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this light
		}

		dl = &backEnd.refdef.dlights[l];
		//VectorCopy( dl->transformed, origin );
		radius = dl->radius;
		//scale = 1.0f / radius;

		//if (pStage->glslShaderGroup == tr.lightallShader)
		{
			int index = pStage->glslShaderIndex;

			index &= ~(LIGHTDEF_LIGHTTYPE_MASK | LIGHTDEF_USE_DELUXEMAP);
			index |= LIGHTDEF_USE_LIGHT_VECTOR;

			sp = &tr.lightallShader[index];
		}

		backEnd.pc.c_lightallDraws++;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_VIEWORIGIN, backEnd.viewParms.or.origin);

		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_DEFORMGEN, deformGen);
		if (deformGen != DGEN_NONE)
		{
			GLSL_SetUniformFloat5(sp, GENERIC_UNIFORM_DEFORMPARAMS, deformParams);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_TIME, tess.shaderTime);
		}

		if ( input->fogNum ) {
			vec4_t fogColorMask;

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGDISTANCE, fogDistanceVector);
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGDEPTH, fogDepthVector);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_FOGEYET, eyeT);

			ComputeFogColorMask(pStage, fogColorMask);

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGCOLORMASK, fogColorMask);
		}

		{
			vec4_t baseColor;
			vec4_t vertColor;

			ComputeShaderColors(pStage, baseColor, vertColor);

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_BASECOLOR, baseColor);
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_VERTCOLOR, vertColor);
		}

		if (pStage->alphaGen == AGEN_PORTAL)
		{
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_PORTALRANGE, tess.shader->portalRange);
		}

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_COLORGEN, pStage->rgbGen);
		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_ALPHAGEN, pStage->alphaGen);

		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_DIRECTEDLIGHT, dl->color);

		VectorSet(vector, 0, 0, 0);
		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_AMBIENTLIGHT, vector);

		VectorCopy(dl->origin, vector);
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_LIGHTORIGIN, vector);

		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_LIGHTRADIUS, radius);

		GLSL_SetUniformVec2(sp, GENERIC_UNIFORM_MATERIALINFO, pStage->materialInfo);
		
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL );

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELMATRIX, backEnd.or.transformMatrix);

		if (pStage->bundle[TB_DIFFUSEMAP].image[0])
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);

		if (pStage->bundle[TB_NORMALMAP].image[0])
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);

		if (pStage->bundle[TB_SPECULARMAP].image[0])
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP);

		if (r_dlightMode->integer >= 2)
		{
			GL_SelectTexture(TB_SHADOWMAP);
			GL_BindCubemap(tr.shadowCubemaps[l]);
			GL_SelectTexture(0);
		}

		ComputeTexMatrix( pStage, TB_DIFFUSEMAP, matrix );
		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_DIFFUSETEXMATRIX, matrix);

		//
		// draw
		//

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, (const GLvoid **)input->multiDrawFirstIndex, input->multiDrawNumIndexes);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex);
		}

		backEnd.pc.c_totalIndexes += tess.numIndexes;
		backEnd.pc.c_dlightIndexes += tess.numIndexes;
	}
}


static void ForwardSunlight( void ) {
//	int		l;
	//vec3_t	origin;
	//float	scale;
	int stage;
	int stageGlState[2];
	qboolean alphaOverride = qfalse;

	int deformGen;
	vec5_t deformParams;
	
	vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float eyeT = 0;

	shaderCommands_t *input = &tess;
	
	ComputeDeformValues(&deformGen, deformParams);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

	// deal with vertex alpha blended surfaces
	if (input->xstages[0] && input->xstages[1] && 
		(input->xstages[1]->alphaGen == AGEN_VERTEX || input->xstages[1]->alphaGen == AGEN_ONE_MINUS_VERTEX))
	{
		stageGlState[0] = input->xstages[0]->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);

		if (stageGlState[0] == 0 || stageGlState[0] == (GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO))
		{
			stageGlState[1] = input->xstages[1]->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS);

			if (stageGlState[1] == (GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				alphaOverride = qtrue;
				stageGlState[0] = GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
				stageGlState[1] = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
			}
			else if (stageGlState[1] == (GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_SRC_ALPHA))
			{
				alphaOverride = qtrue;
				stageGlState[0] = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
				stageGlState[1] = GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
			}
		}
	}

	if (!alphaOverride)
	{
		stageGlState[0] =
		stageGlState[1] = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
	}

	for ( stage = 0; stage < 2 /*MAX_SHADER_STAGES */; stage++ )
	{
		shaderStage_t *pStage = input->xstages[stage];
		shaderProgram_t *sp;
		//vec4_t vector;
		matrix_t matrix;

		if ( !pStage )
		{
			break;
		}

		//VectorCopy( dl->transformed, origin );

		//if (pStage->glslShaderGroup == tr.lightallShader)
		{
			int index = pStage->glslShaderIndex;

			index &= ~(LIGHTDEF_LIGHTTYPE_MASK | LIGHTDEF_USE_DELUXEMAP);
			index |= LIGHTDEF_USE_LIGHT_VECTOR | LIGHTDEF_USE_SHADOWMAP;

			if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
			{
				index |= LIGHTDEF_ENTITY;
			}

			sp = &tr.lightallShader[index];
		}

		backEnd.pc.c_lightallDraws++;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_VIEWORIGIN, backEnd.viewParms.or.origin);

		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_DEFORMGEN, deformGen);
		if (deformGen != DGEN_NONE)
		{
			GLSL_SetUniformFloat5(sp, GENERIC_UNIFORM_DEFORMPARAMS, deformParams);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_TIME, tess.shaderTime);
		}

		if ( input->fogNum ) {
			vec4_t fogColorMask;

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGDISTANCE, fogDistanceVector);
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGDEPTH, fogDepthVector);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_FOGEYET, eyeT);

			ComputeFogColorMask(pStage, fogColorMask);

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGCOLORMASK, fogColorMask);
		}

		{
			vec4_t baseColor;
			vec4_t vertColor;

			ComputeShaderColors(pStage, baseColor, vertColor);

			if (alphaOverride)
			{
				if (input->xstages[1]->alphaGen == AGEN_VERTEX)
				{
					baseColor[3] = 0.0f;
					vertColor[3] = 1.0f;
				}
				else if (input->xstages[1]->alphaGen == AGEN_ONE_MINUS_VERTEX)
				{
					baseColor[3] = 1.0f;
					vertColor[3] = -1.0f;
				}
			}

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_BASECOLOR, baseColor);
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_VERTCOLOR, vertColor);
		}

		if (pStage->alphaGen == AGEN_PORTAL)
		{
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_PORTALRANGE, tess.shader->portalRange);
		}

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_COLORGEN, pStage->rgbGen);
		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_ALPHAGEN, pStage->alphaGen);

		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_DIRECTEDLIGHT, backEnd.refdef.sunCol);
		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_AMBIENTLIGHT,  backEnd.refdef.sunAmbCol);

		GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_LIGHTORIGIN, backEnd.refdef.sunDir);

		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_LIGHTRADIUS, 9999999999.9f);

		GLSL_SetUniformVec2(sp, GENERIC_UNIFORM_MATERIALINFO, pStage->materialInfo);
		
		GL_State( stageGlState[stage] );

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELMATRIX, backEnd.or.transformMatrix);

		if (pStage->bundle[TB_DIFFUSEMAP].image[0])
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_DIFFUSEMAP], TB_DIFFUSEMAP);

		if (pStage->bundle[TB_NORMALMAP].image[0])
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_NORMALMAP], TB_NORMALMAP);

		if (pStage->bundle[TB_SPECULARMAP].image[0])
			R_BindAnimatedImageToTMU( &pStage->bundle[TB_SPECULARMAP], TB_SPECULARMAP);

		/*
		{
			GL_BindToTMU(tr.sunShadowDepthImage[0], TB_SHADOWMAP);
			GL_BindToTMU(tr.sunShadowDepthImage[1], TB_SHADOWMAP2);
			GL_BindToTMU(tr.sunShadowDepthImage[2], TB_SHADOWMAP3);
			GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[0]);
			GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1]);
			GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2]);
		}
		*/
		GL_BindToTMU(tr.screenShadowImage, TB_SHADOWMAP);

		ComputeTexMatrix( pStage, TB_DIFFUSEMAP, matrix );
		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_DIFFUSETEXMATRIX, matrix);

		//
		// draw
		//

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, (const GLvoid **)input->multiDrawFirstIndex, input->multiDrawNumIndexes);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex);
		}

		backEnd.pc.c_totalIndexes += tess.numIndexes;
		backEnd.pc.c_dlightIndexes += tess.numIndexes;
	}
}


static void ProjectPshadowVBOGLSL( void ) {
	int		l;
	vec3_t	origin;
	float	radius;

	int deformGen;
	vec5_t deformParams;

	shaderCommands_t *input = &tess;

	if ( !backEnd.refdef.num_pshadows ) {
		return;
	}
	
	ComputeDeformValues(&deformGen, deformParams);

	for ( l = 0 ; l < backEnd.refdef.num_pshadows ; l++ ) {
		pshadow_t	*ps;
		shaderProgram_t *sp;
		vec4_t vector;

		if ( !( tess.pshadowBits & ( 1 << l ) ) ) {
			continue;	// this surface definately doesn't have any of this shadow
		}

		ps = &backEnd.refdef.pshadows[l];
		VectorCopy( ps->lightOrigin, origin );
		radius = ps->lightRadius;

		sp = &tr.pshadowShader;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, PSHADOW_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		VectorCopy(origin, vector);
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, PSHADOW_UNIFORM_LIGHTORIGIN, vector);

		VectorScale(ps->lightViewAxis[0], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, PSHADOW_UNIFORM_LIGHTFORWARD, vector);

		VectorScale(ps->lightViewAxis[1], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, PSHADOW_UNIFORM_LIGHTRIGHT, vector);

		VectorScale(ps->lightViewAxis[2], 1.0f / ps->viewRadius, vector);
		GLSL_SetUniformVec3(sp, PSHADOW_UNIFORM_LIGHTUP, vector);

		GLSL_SetUniformFloat(sp, PSHADOW_UNIFORM_LIGHTRADIUS, radius);
	  
		// include GLS_DEPTHFUNC_EQUAL so alpha tested surfaces don't add light
		// where they aren't rendered
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );

		GL_BindToTMU( tr.pshadowMaps[l], TB_DIFFUSEMAP );

		//
		// draw
		//

		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, (const GLvoid **)input->multiDrawFirstIndex, input->multiDrawNumIndexes);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex);
		}

		backEnd.pc.c_totalIndexes += tess.numIndexes;
		//backEnd.pc.c_dlightIndexes += tess.numIndexes;
	}
}



/*
===================
RB_FogPass

Blends a fog texture on top of everything else
===================
*/
static void RB_FogPass( void ) {
	fog_t		*fog;
	vec4_t  color;
	vec4_t	fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float	eyeT = 0;
	shaderProgram_t *sp = &tr.fogShader;

	int deformGen;
	vec5_t deformParams;

	ComputeDeformValues(&deformGen, deformParams);

	backEnd.pc.c_fogDraws++;

	GLSL_BindProgram(sp);

	fog = tr.world->fogs + tess.fogNum;

	GLSL_SetUniformMatrix16(sp, FOGPASS_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

	GLSL_SetUniformFloat(sp, FOGPASS_UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);
	
	GLSL_SetUniformInt(sp, FOGPASS_UNIFORM_DEFORMGEN, deformGen);
	if (deformGen != DGEN_NONE)
	{
		GLSL_SetUniformFloat5(sp, FOGPASS_UNIFORM_DEFORMPARAMS, deformParams);
		GLSL_SetUniformFloat(sp, FOGPASS_UNIFORM_TIME, tess.shaderTime);
	}

	color[0] = ((unsigned char *)(&fog->colorInt))[0] / 255.0f;
	color[1] = ((unsigned char *)(&fog->colorInt))[1] / 255.0f;
	color[2] = ((unsigned char *)(&fog->colorInt))[2] / 255.0f;
	color[3] = ((unsigned char *)(&fog->colorInt))[3] / 255.0f;
	GLSL_SetUniformVec4(sp, FOGPASS_UNIFORM_COLOR, color);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

	GLSL_SetUniformVec4(sp, FOGPASS_UNIFORM_FOGDISTANCE, fogDistanceVector);
	GLSL_SetUniformVec4(sp, FOGPASS_UNIFORM_FOGDEPTH, fogDepthVector);
	GLSL_SetUniformFloat(sp, FOGPASS_UNIFORM_FOGEYET, eyeT);

	if ( tess.shader->fogPass == FP_EQUAL ) {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL );
	} else {
		GL_State( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}

	if (tess.multiDrawPrimitives)
	{
		R_DrawMultiElementsVBO(tess.multiDrawPrimitives, (const GLvoid **)tess.multiDrawFirstIndex, tess.multiDrawNumIndexes);
	}
	else
	{
		R_DrawElementsVBO(tess.numIndexes, tess.firstIndex);
	}
}


static unsigned int RB_CalcShaderVertexAttribs( shaderCommands_t *input )
{
	unsigned int vertexAttribs = input->shader->vertexAttribs;

	if(glState.vertexAttribsInterpolation > 0.0f)
	{
		vertexAttribs |= ATTR_POSITION2;
		if (vertexAttribs & ATTR_NORMAL)
		{
			vertexAttribs |= ATTR_NORMAL2;
#ifdef USE_VERT_TANGENT_SPACE
			vertexAttribs |= ATTR_TANGENT2;
			vertexAttribs |= ATTR_BITANGENT2;
#endif
		}
	}

	return vertexAttribs;
}

static void RB_IterateStagesGeneric( shaderCommands_t *input )
{
	int stage;
	matrix_t matrix;
	
	vec4_t fogDistanceVector, fogDepthVector = {0, 0, 0, 0};
	float eyeT = 0;

	int deformGen;
	vec5_t deformParams;

	ComputeDeformValues(&deformGen, deformParams);

	ComputeFogValues(fogDistanceVector, fogDepthVector, &eyeT);

	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = input->xstages[stage];
		shaderProgram_t *sp;

		if ( !pStage )
		{
			break;
		}

		if (backEnd.depthFill)
		{
			if (pStage->glslShaderGroup)
			{
				int index = 0;

				if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
				{
					index |= LIGHTDEF_ENTITY;
				}

				sp = &pStage->glslShaderGroup[index];
			}
			else
			{
				int shaderAttribs = 0;

				if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
				{
					shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
				}

				if (glState.vertexAttribsInterpolation > 0.0f && backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
				{
					shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
				}

				sp = &tr.genericShader[shaderAttribs];
			}
		}
		else if (pStage->glslShaderGroup)
		{
			int index = pStage->glslShaderIndex;

			if (backEnd.currentEntity && backEnd.currentEntity != &tr.worldEntity)
			{
				index |= LIGHTDEF_ENTITY;
			}

			if (r_lightmap->integer && index & LIGHTDEF_USE_LIGHTMAP)
			{
				index = LIGHTDEF_USE_LIGHTMAP;
			}

			sp = &pStage->glslShaderGroup[index];

			if (pStage->glslShaderGroup == tr.lightallShader)
			{
				backEnd.pc.c_lightallDraws++;
			}
		}
		else
		{
			sp = GLSL_GetGenericShaderProgram(stage);

			backEnd.pc.c_genericDraws++;
		}

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
		GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_VIEWORIGIN, backEnd.viewParms.or.origin);

		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);
		
		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_DEFORMGEN, deformGen);
		if (deformGen != DGEN_NONE)
		{
			GLSL_SetUniformFloat5(sp, GENERIC_UNIFORM_DEFORMPARAMS, deformParams);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_TIME, tess.shaderTime);
		}

		if ( input->fogNum ) {
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGDISTANCE, fogDistanceVector);
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGDEPTH, fogDepthVector);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_FOGEYET, eyeT);
		}

		GL_State( pStage->stateBits );

		{
			vec4_t baseColor;
			vec4_t vertColor;
			qboolean tint = qtrue;
			int stage2;

			ComputeShaderColors(pStage, baseColor, vertColor);

			for ( stage2 = stage + 1; stage2 < MAX_SHADER_STAGES; stage2++ )
			{
				shaderStage_t *pStage2 = input->xstages[stage2];
				unsigned int srcBlendBits, dstBlendBits;

				if ( !pStage2 )
				{
					break;
				}

				srcBlendBits = pStage2->stateBits & GLS_SRCBLEND_BITS;
				dstBlendBits = pStage2->stateBits & GLS_DSTBLEND_BITS;

				if (srcBlendBits == GLS_SRCBLEND_DST_COLOR)
				{
					tint = qfalse;
					break;
				}
			}
			
			if (!((tr.sunShadows || r_forceSun->integer) && tess.shader->sort <= SS_OPAQUE 
				&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) && tess.xstages[0]->glslShaderGroup == tr.lightallShader))
			{
				tint = qfalse;
			}

			if (tint)
			{
				// use VectorScale to only scale first three values, not alpha
				VectorScale(baseColor, backEnd.refdef.colorScale, baseColor);
				VectorScale(vertColor, backEnd.refdef.colorScale, vertColor);
			}

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_BASECOLOR, baseColor);
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_VERTCOLOR, vertColor);
		}

		if (pStage->rgbGen == CGEN_LIGHTING_DIFFUSE)
		{
			vec4_t vec;

			VectorScale(backEnd.currentEntity->ambientLight, 1.0f / 255.0f, vec);
			GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_AMBIENTLIGHT, vec);

			VectorScale(backEnd.currentEntity->directedLight, 1.0f / 255.0f, vec);
			GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_DIRECTEDLIGHT, vec);
			
			VectorCopy(backEnd.currentEntity->lightDir, vec);
			vec[3] = 0.0f;
			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_LIGHTORIGIN, vec);

			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_LIGHTRADIUS, 999999.0f);
		}

		if (pStage->alphaGen == AGEN_PORTAL)
		{
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_PORTALRANGE, tess.shader->portalRange);
		}

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_COLORGEN, pStage->rgbGen);
		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_ALPHAGEN, pStage->alphaGen);

		if ( input->fogNum )
		{
			vec4_t fogColorMask;

			ComputeFogColorMask(pStage, fogColorMask);

			GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_FOGCOLORMASK, fogColorMask);
		}

		ComputeTexMatrix( pStage, TB_DIFFUSEMAP, matrix );
		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_DIFFUSETEXMATRIX, matrix);

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_TCGEN0, pStage->bundle[0].tcGen);
		if (pStage->bundle[0].tcGen == TCGEN_VECTOR)
		{
			vec3_t vec;

			VectorCopy(pStage->bundle[0].tcGenVectors[0], vec);
			GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_TCGEN0VECTOR0, vec);
			VectorCopy(pStage->bundle[0].tcGenVectors[1], vec);
			GLSL_SetUniformVec3(sp, GENERIC_UNIFORM_TCGEN0VECTOR1, vec);
		}

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELMATRIX, backEnd.or.transformMatrix);

		GLSL_SetUniformVec2(sp, GENERIC_UNIFORM_MATERIALINFO, pStage->materialInfo);

		//GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_MAPLIGHTSCALE, backEnd.refdef.mapLightScale);

		//
		// do multitexture
		//
		if ( backEnd.depthFill )
		{
			if (!(pStage->stateBits & GLS_ATEST_BITS))
				GL_BindToTMU( tr.whiteImage, 0 );
			else if ( pStage->bundle[TB_COLORMAP].image[0] != 0 )
				R_BindAnimatedImageToTMU( &pStage->bundle[TB_COLORMAP], TB_COLORMAP );
		}
		else if ( pStage->glslShaderGroup )
		{
			int i;

			if ((r_lightmap->integer == 1 || r_lightmap->integer == 2) && pStage->bundle[TB_LIGHTMAP].image[0])
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (i == TB_LIGHTMAP)
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[i], i);
					}
					else if (pStage->bundle[i].image[0])
					{
						GL_BindToTMU( tr.whiteImage, i);
					}
				}
			}
			else if (r_lightmap->integer == 3 && pStage->bundle[TB_DELUXEMAP].image[0])
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (i == TB_LIGHTMAP)
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[TB_DELUXEMAP], i);
					}
					else if (pStage->bundle[i].image[0])
					{
						GL_BindToTMU( tr.whiteImage, i);
					}
				}
			}
			else
			{
				for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
				{
					if (pStage->bundle[i].image[0])
					{
						R_BindAnimatedImageToTMU( &pStage->bundle[i], i);
					}
				}
			}
		}
		else if ( pStage->bundle[1].image[0] != 0 )
		{
			R_BindAnimatedImageToTMU( &pStage->bundle[0], 0 );

			//
			// lightmap/secondary pass
			//
			if ( r_lightmap->integer ) {
				GLSL_SetUniformInt(sp, GENERIC_UNIFORM_TEXTURE1ENV, GL_REPLACE);
			} else {
				GLSL_SetUniformInt(sp, GENERIC_UNIFORM_TEXTURE1ENV, tess.shader->multitextureEnv);
			}

			R_BindAnimatedImageToTMU( &pStage->bundle[1], 1 );
		}
		else 
		{
			//
			// set state
			//
			if ( pStage->bundle[0].vertexLightmap && ( (r_vertexLight->integer && !r_uiFullScreen->integer) || glConfig.hardwareType == GLHW_PERMEDIA2 ) && r_lightmap->integer )
			{
				GL_BindToTMU( tr.whiteImage, 0 );
			}
			else 
				R_BindAnimatedImageToTMU( &pStage->bundle[0], 0 );

			GLSL_SetUniformInt(sp, GENERIC_UNIFORM_TEXTURE1ENV, 0);
		}

		//
		// draw
		//
		if (input->multiDrawPrimitives)
		{
			R_DrawMultiElementsVBO(input->multiDrawPrimitives, (const GLvoid **)input->multiDrawFirstIndex, input->multiDrawNumIndexes);
		}
		else
		{
			R_DrawElementsVBO(input->numIndexes, input->firstIndex);
		}

		// allow skipping out to show just lightmaps during development
		if ( r_lightmap->integer && ( pStage->bundle[0].isLightmap || pStage->bundle[1].isLightmap || pStage->bundle[0].vertexLightmap ) )
		{
			break;
		}

		if (backEnd.depthFill)
			break;
	}
}


static void RB_RenderShadowmap( shaderCommands_t *input )
{
	int deformGen;
	vec5_t deformParams;

	ComputeDeformValues(&deformGen, deformParams);

	{
		shaderProgram_t *sp = &tr.shadowmapShader;

		vec4_t vector;

		GLSL_BindProgram(sp);

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);

		GLSL_SetUniformMatrix16(sp, GENERIC_UNIFORM_MODELMATRIX, backEnd.or.transformMatrix);

		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_VERTEXLERP, glState.vertexAttribsInterpolation);

		GLSL_SetUniformInt(sp, GENERIC_UNIFORM_DEFORMGEN, deformGen);
		if (deformGen != DGEN_NONE)
		{
			GLSL_SetUniformFloat5(sp, GENERIC_UNIFORM_DEFORMPARAMS, deformParams);
			GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_TIME, tess.shaderTime);
		}

		VectorCopy(backEnd.viewParms.or.origin, vector);
		vector[3] = 1.0f;
		GLSL_SetUniformVec4(sp, GENERIC_UNIFORM_LIGHTORIGIN, vector);
		GLSL_SetUniformFloat(sp, GENERIC_UNIFORM_LIGHTRADIUS, backEnd.viewParms.zFar);

		GL_State( 0 );

		//
		// do multitexture
		//
		//if ( pStage->glslShaderGroup )
		{
			//
			// draw
			//

			if (input->multiDrawPrimitives)
			{
				R_DrawMultiElementsVBO(input->multiDrawPrimitives, (const GLvoid **)input->multiDrawFirstIndex, input->multiDrawNumIndexes);
			}
			else
			{
				R_DrawElementsVBO(input->numIndexes, input->firstIndex);
			}
		}
	}
}



/*
** RB_StageIteratorGeneric
*/
void RB_StageIteratorGeneric( void )
{
	shaderCommands_t *input;
	unsigned int vertexAttribs = 0;

	input = &tess;
	
	if (!input->numVertexes || !input->numIndexes)
	{
		return;
	}

	if (tess.useInternalVBO)
	{
		RB_DeformTessGeometry();
	}

	vertexAttribs = RB_CalcShaderVertexAttribs( input );

	if (tess.useInternalVBO)
	{
		RB_UpdateVBOs(vertexAttribs);
	}
	else
	{
		backEnd.pc.c_staticVboDraws++;
	}

	//
	// log this call
	//
	if ( r_logFile->integer ) 
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va("--- RB_StageIteratorGeneric( %s ) ---\n", tess.shader->name) );
	}

	//
	// set face culling appropriately
	//
	if ((backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		//GL_Cull( CT_TWO_SIDED );
		
		if (input->shader->cullType == CT_TWO_SIDED)
			GL_Cull( CT_TWO_SIDED );
		else if (input->shader->cullType == CT_FRONT_SIDED)
			GL_Cull( CT_BACK_SIDED );
		else
			GL_Cull( CT_FRONT_SIDED );
		
	}
	else
		GL_Cull( input->shader->cullType );

	// set polygon offset if necessary
	if ( input->shader->polygonOffset )
	{
		qglEnable( GL_POLYGON_OFFSET_FILL );
		qglPolygonOffset( r_offsetFactor->value, r_offsetUnits->value );
	}

	//
	// Set vertex attribs and pointers
	//
	GLSL_VertexAttribsState(vertexAttribs);

	//
	// render depth if in depthfill mode
	//
	if (backEnd.depthFill)
	{
		RB_IterateStagesGeneric( input );

		//
		// reset polygon offset
		//
		if ( input->shader->polygonOffset )
		{
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}

		return;
	}

	//
	// render shadowmap if in shadowmap mode
	//
	if (backEnd.viewParms.flags & VPF_SHADOWMAP)
	{
		if ( input->shader->sort == SS_OPAQUE )
		{
			RB_RenderShadowmap( input );
		}
		//
		// reset polygon offset
		//
		if ( input->shader->polygonOffset )
		{
			qglDisable( GL_POLYGON_OFFSET_FILL );
		}

		return;
	}

	//
	//
	// call shader function
	//
	RB_IterateStagesGeneric( input );

	//
	// pshadows!
	//
	if ( tess.pshadowBits && tess.shader->sort <= SS_OPAQUE
		&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
		ProjectPshadowVBOGLSL();
	}


	// 
	// now do any dynamic lighting needed
	//
	if ( tess.dlightBits && tess.shader->sort <= SS_OPAQUE
		&& !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) ) {
		if (tess.shader->numUnfoggedPasses == 1 && tess.xstages[0]->glslShaderGroup == tr.lightallShader
			&& (tess.xstages[0]->glslShaderIndex & LIGHTDEF_LIGHTTYPE_MASK) && r_dlightMode->integer)
		{
			ForwardDlight();
		}
		else
		{
			ProjectDlightTexture();
		}
	}

	if ((backEnd.viewParms.flags & VPF_USESUNLIGHT) && tess.shader->sort <= SS_OPAQUE 
	//if ((tr.sunShadows || r_forceSunlight->value > 0.0f) && tess.shader->sort <= SS_OPAQUE 
	    && !(tess.shader->surfaceFlags & (SURF_NODLIGHT | SURF_SKY) ) && tess.xstages[0]->glslShaderGroup == tr.lightallShader) {
		ForwardSunlight();
	}

	//
	// now do fog
	//
	if ( tess.fogNum && tess.shader->fogPass ) {
		RB_FogPass();
	}

	//
	// reset polygon offset
	//
	if ( input->shader->polygonOffset )
	{
		qglDisable( GL_POLYGON_OFFSET_FILL );
	}
}


/*
** RB_EndSurface
*/
void RB_EndSurface( void ) {
	shaderCommands_t *input;

	input = &tess;

	if (input->numIndexes == 0 || input->numVertexes == 0) {
		return;
	}

	if (input->indexes[SHADER_MAX_INDEXES-1] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}	
	if (input->xyz[SHADER_MAX_VERTEXES-1][0] != 0) {
		ri.Error (ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}

	if ( tess.shader == tr.shadowShader ) {
		RB_ShadowTessEnd();
		return;
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if ( r_debugSort->integer && r_debugSort->integer < tess.shader->sort ) {
		return;
	}

	//
	// update performance counters
	//
	backEnd.pc.c_shaders++;
	backEnd.pc.c_vertexes += tess.numVertexes;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_totalIndexes += tess.numIndexes * tess.numPasses;

	//
	// call off to shader specific tess end function
	//
	tess.currentStageIteratorFunc();

	//
	// draw debugging stuff
	//
	if ( r_showtris->integer ) {
		DrawTris (input);
	}
	if ( r_shownormals->integer ) {
		DrawNormals (input);
	}
	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;
	tess.firstIndex = 0;
	tess.multiDrawPrimitives = 0;

	GLimp_LogComment( "----------\n" );
}
