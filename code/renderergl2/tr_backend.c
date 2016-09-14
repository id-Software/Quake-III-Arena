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
#include "tr_local.h"
#include "tr_fbo.h"
#include "tr_dsa.h"

backEndData_t	*backEndData;
backEndState_t	backEnd;


static float	s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


/*
** GL_BindToTMU
*/
void GL_BindToTMU( image_t *image, int tmu )
{
	GLuint texture = (tmu == TB_COLORMAP) ? tr.defaultImage->texnum : 0;
	GLenum target = GL_TEXTURE_2D;

	if (image)
	{
		if (image->flags & IMGFLAG_CUBEMAP)
			target = GL_TEXTURE_CUBE_MAP;

		image->frameUsed = tr.frameCount;
		texture = image->texnum;
	}
	else
	{
		ri.Printf(PRINT_WARNING, "GL_BindToTMU: NULL image\n");
	}

	GL_BindMultiTexture(GL_TEXTURE0_ARB + tmu, target, texture);
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	if ( cullType == CT_TWO_SIDED ) 
	{
		qglDisable( GL_CULL_FACE );
	} 
	else 
	{
		qboolean cullFront = (cullType == CT_FRONT_SIDED);

		if ( glState.faceCulling == CT_TWO_SIDED )
			qglEnable( GL_CULL_FACE );

		if ( glState.faceCullFront != cullFront )
			qglCullFace( cullFront ? GL_FRONT : GL_BACK );

		glState.faceCullFront = cullFront;
	}

	glState.faceCulling = cullType;
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( unsigned long stateBits )
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if ( !diff )
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_BITS )
	{
		if ( stateBits & GLS_DEPTHFUNC_EQUAL )
		{
			qglDepthFunc( GL_EQUAL );
		}
		else if ( stateBits & GLS_DEPTHFUNC_GREATER)
		{
			qglDepthFunc( GL_GREATER );
		}
		else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) )
	{
		uint32_t oldState = glState.glStateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
		uint32_t newState = stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
		uint32_t storedState = glState.storedGlState & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );

		if (oldState == 0)
		{
			qglEnable( GL_BLEND );
		}
		else if (newState == 0)
		{
			qglDisable( GL_BLEND );
		}

		if (newState != 0 && storedState != newState)
		{
			GLenum srcFactor = GL_ONE, dstFactor = GL_ONE;

			glState.storedGlState &= ~( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS );
			glState.storedGlState |= newState;

			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid src blend state bits" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits" );
				break;
			}

			qglBlendFunc( srcFactor, dstFactor );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE )
	{
		if ( stateBits & GLS_DEPTHMASK_TRUE )
		{
			qglDepthMask( GL_TRUE );
		}
		else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE )
	{
		if ( stateBits & GLS_POLYMODE_LINE )
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE )
	{
		if ( stateBits & GLS_DEPTHTEST_DISABLE )
		{
			qglDisable( GL_DEPTH_TEST );
		}
		else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS )
	{
		uint32_t oldState = glState.glStateBits & GLS_ATEST_BITS;
		uint32_t newState = stateBits & GLS_ATEST_BITS;
		uint32_t storedState = glState.storedGlState & GLS_ATEST_BITS;

		if (oldState == 0)
		{
			qglEnable(GL_ALPHA_TEST);
		}
		else if (newState == 0)
		{
			qglDisable(GL_ALPHA_TEST);
		}

		if (newState != 0 && storedState != newState)
		{
			glState.storedGlState &= ~GLS_ATEST_BITS;
			glState.storedGlState |= newState;

			switch ( newState )
			{
			case GLS_ATEST_GT_0:
				qglAlphaFunc( GL_GREATER, 0.0f );
				break;
			case GLS_ATEST_LT_80:
				qglAlphaFunc( GL_LESS, 0.5f );
				break;
			case GLS_ATEST_GE_80:
				qglAlphaFunc( GL_GEQUAL, 0.5f );
				break;
			}
		}
	}

	glState.glStateBits = stateBits;
}


void GL_SetProjectionMatrix(mat4_t matrix)
{
	Mat4Copy(matrix, glState.projection);
	Mat4Multiply(glState.projection, glState.modelview, glState.modelviewProjection);	
}


void GL_SetModelviewMatrix(mat4_t matrix)
{
	Mat4Copy(matrix, glState.modelview);
	Mat4Multiply(glState.projection, glState.modelview, glState.modelviewProjection);	
}


/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float		c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );
	qglClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	backEnd.isHyperspace = qtrue;
}


static void SetViewportAndScissor( void ) {
	GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );

	// set the window clipping
	qglViewport( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
	qglScissor( backEnd.viewParms.viewportX, backEnd.viewParms.viewportY, 
		backEnd.viewParms.viewportWidth, backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {
	int clearBits = 0;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish ();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	if (glRefConfig.framebufferObject)
	{
		FBO_t *fbo = backEnd.viewParms.targetFbo;

		// FIXME: HUGE HACK: render to the screen fbo if we've already postprocessed the frame and aren't drawing more world
		// drawing more world check is in case of double renders, such as skyportals
		if (fbo == NULL && !(backEnd.framePostProcessed && (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)))
			fbo = tr.renderFbo;

		if (tr.renderCubeFbo && fbo == tr.renderCubeFbo)
		{
			cubemap_t *cubemap = &tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex];
			FBO_AttachImage(fbo, cubemap->image, GL_COLOR_ATTACHMENT0_EXT, backEnd.viewParms.targetFboLayer);
		}

		FBO_Bind(fbo);
	}

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );
	// clear relevant buffers
	clearBits = GL_DEPTH_BUFFER_BIT;

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 )
	{
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}
	if ( r_fastsky->integer && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		clearBits |= GL_COLOR_BUFFER_BIT;	// FIXME: only if sky shaders have been used
	}

	// clear to black for cube maps
	if (tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		clearBits |= GL_COLOR_BUFFER_BIT;
	}

	qglClear( clearBits );

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) )
	{
		RB_Hyperspace();
		return;
	}
	else
	{
		backEnd.isHyperspace = qfalse;
	}

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
#if 0
		float	plane[4];
		GLdouble	plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct (backEnd.viewParms.or.axis[0], plane);
		plane2[1] = DotProduct (backEnd.viewParms.or.axis[1], plane);
		plane2[2] = DotProduct (backEnd.viewParms.or.axis[2], plane);
		plane2[3] = DotProduct (plane, backEnd.viewParms.or.origin) - plane[3];
#endif
		GL_SetModelviewMatrix( s_flipMatrix );
	}
}


/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t		*shader, *oldShader;
	int				fogNum, oldFogNum;
	int				entityNum, oldEntityNum;
	int				dlighted, oldDlighted;
	int				pshadowed, oldPshadowed;
	int             cubemapIndex, oldCubemapIndex;
	qboolean		depthRange, oldDepthRange, isCrosshair, wasCrosshair;
	int				i;
	drawSurf_t		*drawSurf;
	int				oldSort;
	float			originalTime;
	FBO_t*			fbo = NULL;
	qboolean		inQuery = qfalse;

	float			depth[2];


	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	fbo = glState.currentFBO;

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	wasCrosshair = qfalse;
	oldDlighted = qfalse;
	oldPshadowed = qfalse;
	oldCubemapIndex = -1;
	oldSort = -1;

	depth[0] = 0.f;
	depth[1] = 1.f;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for (i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++) {
		if ( drawSurf->sort == oldSort && drawSurf->cubemapIndex == oldCubemapIndex) {
			if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE)
				continue;

			// fast path, same as previous sort
			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
			continue;
		}
		oldSort = drawSurf->sort;
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &pshadowed );
		cubemapIndex = drawSurf->cubemapIndex;

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != NULL && ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted || pshadowed != oldPshadowed || cubemapIndex != oldCubemapIndex
			|| ( entityNum != oldEntityNum && !shader->entityMergable ) ) ) {
			if (oldShader != NULL) {
				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum, cubemapIndex );
			backEnd.pc.c_surfBatches++;
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
			oldPshadowed = pshadowed;
			oldCubemapIndex = cubemapIndex;
		}

		if (backEnd.depthFill && shader && shader->sort != SS_OPAQUE)
			continue;

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			qboolean sunflare = qfalse;
			depthRange = isCrosshair = qfalse;

			if ( entityNum != REFENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if(backEnd.currentEntity->e.renderfx & RF_DEPTHHACK)
				{
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
					
					if(backEnd.currentEntity->e.renderfx & RF_CROSSHAIR)
						isCrosshair = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;
				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;
				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}

			GL_SetModelviewMatrix( backEnd.or.modelMatrix );

			//
			// change depthrange. Also change projection matrix so first person weapon does not look like coming
			// out of the screen.
			//
			if (oldDepthRange != depthRange || wasCrosshair != isCrosshair)
			{
				if (depthRange)
				{
					if(backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						if(isCrosshair)
						{
							if(oldDepthRange)
							{
								// was not a crosshair but now is, change back proj matrix
								GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
							}
						}
						else
						{
							viewParms_t temp = backEnd.viewParms;

							R_SetupProjection(&temp, r_znear->value, 0, qfalse);

							GL_SetProjectionMatrix( temp.projectionMatrix );
						}
					}

 					if(!oldDepthRange)
					{
						depth[0] = 0;
						depth[1] = 0.3f;
 						qglDepthRange (depth[0], depth[1]);
	 				}
				}
				else
				{
					if(!wasCrosshair && backEnd.viewParms.stereoFrame != STEREO_CENTER)
					{
						GL_SetProjectionMatrix( backEnd.viewParms.projectionMatrix );
					}

					if (!sunflare)
						qglDepthRange (0, 1);

					depth[0] = 0;
					depth[1] = 1;
				}

				oldDepthRange = depthRange;
				wasCrosshair = isCrosshair;
			}

			oldEntityNum = entityNum;
		}

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
	}

	backEnd.refdef.floatTime = originalTime;

	// draw the contents of the last shader batch
	if (oldShader != NULL) {
		RB_EndSurface();
	}

	if (inQuery) {
		qglEndQuery(GL_SAMPLES_PASSED);
	}

	if (glRefConfig.framebufferObject)
		FBO_Bind(fbo);

	// go back to the world modelview matrix

	GL_SetModelviewMatrix( backEnd.viewParms.world.modelMatrix );

	qglDepthRange (0, 1);
}


/*
============================================================================

RENDER BACK END FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void	RB_SetGL2D (void) {
	mat4_t matrix;
	int width, height;

	if (backEnd.projection2D && backEnd.last2DFBO == glState.currentFBO)
		return;

	backEnd.projection2D = qtrue;
	backEnd.last2DFBO = glState.currentFBO;

	if (glState.currentFBO)
	{
		width = glState.currentFBO->width;
		height = glState.currentFBO->height;
	}
	else
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}

	// set 2D virtual screen size
	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Mat4Ortho(0, width, height, 0, 0, 1, matrix);
	GL_SetProjectionMatrix(matrix);
	Mat4Identity(matrix);
	GL_SetModelviewMatrix(matrix);

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	int			i, j;
	int			start, end;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];

	if ( !tr.registered ) {
		return;
	}
	R_IssuePendingRenderCommands();

	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = 0;
	if ( r_speeds->integer ) {
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

	RE_UploadCinematic (w, h, cols, rows, data, client, dirty);
	GL_BindToTMU(tr.scratchImage[client], TB_COLORMAP);

	if ( r_speeds->integer ) {
		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	// FIXME: HUGE hack
	if (glRefConfig.framebufferObject)
	{
		FBO_Bind(backEnd.framePostProcessed ? NULL : tr.renderFbo);
	}

	RB_SetGL2D();

	VectorSet4(quadVerts[0], x,     y,     0.0f, 1.0f);
	VectorSet4(quadVerts[1], x + w, y,     0.0f, 1.0f);
	VectorSet4(quadVerts[2], x + w, y + h, 0.0f, 1.0f);
	VectorSet4(quadVerts[3], x,     y + h, 0.0f, 1.0f);

	VectorSet2(texCoords[0], 0.5f / cols,          0.5f / rows);
	VectorSet2(texCoords[1], (cols - 0.5f) / cols, 0.5f / rows);
	VectorSet2(texCoords[2], (cols - 0.5f) / cols, (rows - 0.5f) / rows);
	VectorSet2(texCoords[3], 0.5f / cols,          (rows - 0.5f) / rows);

	GLSL_BindProgram(&tr.textureColorShader);
	
	GLSL_SetUniformMat4(&tr.textureColorShader, UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(&tr.textureColorShader, UNIFORM_COLOR, colorWhite);

	RB_InstantQuad2(quadVerts, texCoords);
}

void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty) {
	GLuint texture;

	if (!tr.scratchImage[client])
	{
		ri.Printf(PRINT_WARNING, "RE_UploadCinematic: scratch images not initialized\n");
		return;
	}

	texture = tr.scratchImage[client]->texnum;

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTextureImage2DEXT(texture, GL_TEXTURE_2D, 0, GL_RGB8, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		qglTextureParameterfEXT(texture, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		if (dirty) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTextureSubImage2DEXT(texture, GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
	}
}


/*
=============
RB_SetColor

=============
*/
const void	*RB_SetColor( const void *data ) {
	const setColorCommand_t	*cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)(cmd + 1);
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic ( const void *data ) {
	const stretchPicCommand_t	*cmd;
	shader_t *shader;
	int		numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	// FIXME: HUGE hack
	if (glRefConfig.framebufferObject)
		FBO_Bind(backEnd.framePostProcessed ? NULL : tr.renderFbo);

	RB_SetGL2D();

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	{
		vec4_t color;

		VectorScale4(backEnd.color2D, 1.0f / 255.0f, color);

		VectorCopy4(color, tess.vertexColors[ numVerts ]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 1]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 2]);
		VectorCopy4(color, tess.vertexColors[ numVerts + 3 ]);
	}

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawSurfs

=============
*/
const void	*RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView ();

	if (glRefConfig.framebufferObject && (backEnd.viewParms.flags & VPF_DEPTHCLAMP) && glRefConfig.depthClamp)
	{
		qglEnable(GL_DEPTH_CLAMP);
	}

	if (glRefConfig.framebufferObject && !(backEnd.refdef.rdflags & RDF_NOWORLDMODEL) && (r_depthPrepass->integer || (backEnd.viewParms.flags & VPF_DEPTHSHADOW)))
	{
		FBO_t *oldFbo = glState.currentFBO;
		vec4_t viewInfo;

		VectorSet4(viewInfo, backEnd.viewParms.zFar / r_znear->value, backEnd.viewParms.zFar, 0.0, 0.0);

		backEnd.depthFill = qtrue;
		qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );
		qglColorMask(!backEnd.colorMask[0], !backEnd.colorMask[1], !backEnd.colorMask[2], !backEnd.colorMask[3]);
		backEnd.depthFill = qfalse;

		if (tr.msaaResolveFbo)
		{
			// If we're using multisampling, resolve the depth first
			FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		}
		else if (tr.renderFbo == NULL && tr.renderDepthImage)
		{
			// If we're rendering directly to the screen, copy the depth to a texture
			qglCopyTextureImage2DEXT(tr.renderDepthImage->texnum, GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, glConfig.vidWidth, glConfig.vidHeight, 0);
		}

		if (tr.hdrDepthFbo)
		{
			// need the depth in a texture we can do GL_LINEAR sampling on, so copy it to an HDR image
			vec4_t srcTexCoords;

			VectorSet4(srcTexCoords, 0.0f, 0.0f, 1.0f, 1.0f);

			FBO_BlitFromTexture(tr.renderDepthImage, srcTexCoords, NULL, tr.hdrDepthFbo, NULL, NULL, NULL, 0);
		}

		if (r_sunlightMode->integer && backEnd.viewParms.flags & VPF_USESUNLIGHT)
		{
			vec4_t quadVerts[4];
			vec2_t texCoords[4];
			vec4_t box;

			FBO_Bind(tr.screenShadowFbo);

			box[0] = backEnd.viewParms.viewportX      * tr.screenShadowFbo->width  / (float)glConfig.vidWidth;
			box[1] = backEnd.viewParms.viewportY      * tr.screenShadowFbo->height / (float)glConfig.vidHeight;
			box[2] = backEnd.viewParms.viewportWidth  * tr.screenShadowFbo->width  / (float)glConfig.vidWidth;
			box[3] = backEnd.viewParms.viewportHeight * tr.screenShadowFbo->height / (float)glConfig.vidHeight;

			qglViewport(box[0], box[1], box[2], box[3]);
			qglScissor(box[0], box[1], box[2], box[3]);

			box[0] = backEnd.viewParms.viewportX               / (float)glConfig.vidWidth;
			box[1] = backEnd.viewParms.viewportY               / (float)glConfig.vidHeight;
			box[2] = box[0] + backEnd.viewParms.viewportWidth  / (float)glConfig.vidWidth;
			box[3] = box[1] + backEnd.viewParms.viewportHeight / (float)glConfig.vidHeight;

			texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
			texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
			texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
			texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

			box[0] = -1.0f;
			box[1] = -1.0f;
			box[2] =  1.0f;
			box[3] =  1.0f;

			VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
			VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
			VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
			VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

			GL_State( GLS_DEPTHTEST_DISABLE );

			GLSL_BindProgram(&tr.shadowmaskShader);

			GL_BindToTMU(tr.renderDepthImage, TB_COLORMAP);
			
			if (r_shadowCascadeZFar->integer != 0)
			{
				GL_BindToTMU(tr.sunShadowDepthImage[0], TB_SHADOWMAP);
				GL_BindToTMU(tr.sunShadowDepthImage[1], TB_SHADOWMAP2);
				GL_BindToTMU(tr.sunShadowDepthImage[2], TB_SHADOWMAP3);
				GL_BindToTMU(tr.sunShadowDepthImage[3], TB_SHADOWMAP4);

				GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP,  backEnd.refdef.sunShadowMvp[0]);
				GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP2, backEnd.refdef.sunShadowMvp[1]);
				GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP3, backEnd.refdef.sunShadowMvp[2]);
				GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP4, backEnd.refdef.sunShadowMvp[3]);
			}
			else
			{
				GL_BindToTMU(tr.sunShadowDepthImage[3], TB_SHADOWMAP);
				GLSL_SetUniformMat4(&tr.shadowmaskShader, UNIFORM_SHADOWMVP, backEnd.refdef.sunShadowMvp[3]);
			}
			
			GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWORIGIN,  backEnd.refdef.vieworg);
			{
				vec3_t viewVector;

				float zmax = backEnd.viewParms.zFar;
				float ymax = zmax * tan(backEnd.viewParms.fovY * M_PI / 360.0f);
				float xmax = zmax * tan(backEnd.viewParms.fovX * M_PI / 360.0f);

				VectorScale(backEnd.refdef.viewaxis[0], zmax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWFORWARD, viewVector);
				VectorScale(backEnd.refdef.viewaxis[1], xmax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWLEFT,    viewVector);
				VectorScale(backEnd.refdef.viewaxis[2], ymax, viewVector);
				GLSL_SetUniformVec3(&tr.shadowmaskShader, UNIFORM_VIEWUP,      viewVector);

				GLSL_SetUniformVec4(&tr.shadowmaskShader, UNIFORM_VIEWINFO, viewInfo);
			}

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

			if (r_shadowBlur->integer)
			{
				viewInfo[2] = 1.0f / (float)(tr.screenScratchFbo->width);
				viewInfo[3] = 1.0f / (float)(tr.screenScratchFbo->height);

				FBO_Bind(tr.screenScratchFbo);

				GLSL_BindProgram(&tr.depthBlurShader[0]);

				GL_BindToTMU(tr.screenShadowImage, TB_COLORMAP);
				GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

				GLSL_SetUniformVec4(&tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo);

				RB_InstantQuad2(quadVerts, texCoords);

				FBO_Bind(tr.screenShadowFbo);

				GLSL_BindProgram(&tr.depthBlurShader[1]);

				GL_BindToTMU(tr.screenScratchImage, TB_COLORMAP);
				GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

				GLSL_SetUniformVec4(&tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo);

				RB_InstantQuad2(quadVerts, texCoords);
			}
		}

		if (r_ssao->integer)
		{
			vec4_t quadVerts[4];
			vec2_t texCoords[4];

			viewInfo[2] = 1.0f / ((float)(tr.quarterImage[0]->width)  * tan(backEnd.viewParms.fovX * M_PI / 360.0f) * 2.0f);
			viewInfo[3] = 1.0f / ((float)(tr.quarterImage[0]->height) * tan(backEnd.viewParms.fovY * M_PI / 360.0f) * 2.0f);
			viewInfo[3] *= (float)backEnd.viewParms.viewportHeight / (float)backEnd.viewParms.viewportWidth;

			FBO_Bind(tr.quarterFbo[0]);

			qglViewport(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);
			qglScissor(0, 0, tr.quarterFbo[0]->width, tr.quarterFbo[0]->height);

			VectorSet4(quadVerts[0], -1,  1, 0, 1);
			VectorSet4(quadVerts[1],  1,  1, 0, 1);
			VectorSet4(quadVerts[2],  1, -1, 0, 1);
			VectorSet4(quadVerts[3], -1, -1, 0, 1);

			texCoords[0][0] = 0; texCoords[0][1] = 1;
			texCoords[1][0] = 1; texCoords[1][1] = 1;
			texCoords[2][0] = 1; texCoords[2][1] = 0;
			texCoords[3][0] = 0; texCoords[3][1] = 0;

			GL_State( GLS_DEPTHTEST_DISABLE );

			GLSL_BindProgram(&tr.ssaoShader);

			GL_BindToTMU(tr.hdrDepthImage, TB_COLORMAP);

			GLSL_SetUniformVec4(&tr.ssaoShader, UNIFORM_VIEWINFO, viewInfo);

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


			viewInfo[2] = 1.0f / (float)(tr.quarterImage[0]->width);
			viewInfo[3] = 1.0f / (float)(tr.quarterImage[0]->height);

			FBO_Bind(tr.quarterFbo[1]);

			qglViewport(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);
			qglScissor(0, 0, tr.quarterFbo[1]->width, tr.quarterFbo[1]->height);

			GLSL_BindProgram(&tr.depthBlurShader[0]);

			GL_BindToTMU(tr.quarterImage[0],  TB_COLORMAP);
			GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

			GLSL_SetUniformVec4(&tr.depthBlurShader[0], UNIFORM_VIEWINFO, viewInfo);

			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);


			FBO_Bind(tr.screenSsaoFbo);

			qglViewport(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);
			qglScissor(0, 0, tr.screenSsaoFbo->width, tr.screenSsaoFbo->height);

			GLSL_BindProgram(&tr.depthBlurShader[1]);

			GL_BindToTMU(tr.quarterImage[1],  TB_COLORMAP);
			GL_BindToTMU(tr.hdrDepthImage, TB_LIGHTMAP);

			GLSL_SetUniformVec4(&tr.depthBlurShader[1], UNIFORM_VIEWINFO, viewInfo);


			RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);
		}

		// reset viewport and scissor
		FBO_Bind(oldFbo);
		SetViewportAndScissor();
	}

	if (glRefConfig.framebufferObject && (backEnd.viewParms.flags & VPF_DEPTHCLAMP) && glRefConfig.depthClamp)
	{
		qglDisable(GL_DEPTH_CLAMP);
	}

	if (!(backEnd.viewParms.flags & VPF_DEPTHSHADOW))
	{
		RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

		if (r_drawSun->integer)
		{
			RB_DrawSun(0.1, tr.sunShader);
		}

		if (glRefConfig.framebufferObject && r_drawSunRays->integer)
		{
			FBO_t *oldFbo = glState.currentFBO;
			FBO_Bind(tr.sunRaysFbo);
			
			qglClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
			qglClear( GL_COLOR_BUFFER_BIT );

			if (glRefConfig.occlusionQuery)
			{
				tr.sunFlareQueryActive[tr.sunFlareQueryIndex] = qtrue;
				qglBeginQuery(GL_SAMPLES_PASSED, tr.sunFlareQuery[tr.sunFlareQueryIndex]);
			}

			RB_DrawSun(0.3, tr.sunFlareShader);

			if (glRefConfig.occlusionQuery)
			{
				qglEndQuery(GL_SAMPLES_PASSED);
			}

			FBO_Bind(oldFbo);
		}

		// darken down any stencil shadows
		RB_ShadowFinish();		

		// add light flares on lights that aren't obscured
		RB_RenderFlares();
	}

	if (glRefConfig.framebufferObject && tr.renderCubeFbo && backEnd.viewParms.targetFbo == tr.renderCubeFbo)
	{
		cubemap_t *cubemap = &tr.cubemaps[backEnd.viewParms.targetFboCubemapIndex];

		FBO_Bind(NULL);
		if (cubemap && cubemap->image)
			qglGenerateTextureMipmapEXT(cubemap->image->texnum, GL_TEXTURE_CUBE_MAP);
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_DrawBuffer

=============
*/
const void	*RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t	*cmd;

	cmd = (const drawBufferCommand_t *)data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (glRefConfig.framebufferObject)
		FBO_Bind(NULL);

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)(cmd + 1);
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int		i;
	image_t	*image;
	float	x, y, w, h;
	int		start, end;

	RB_SetGL2D();

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();

	start = ri.Milliseconds();

	for ( i=0 ; i<tr.numImages ; i++ ) {
		image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		{
			vec4_t quadVerts[4];

			GL_BindToTMU(image, TB_COLORMAP);

			VectorSet4(quadVerts[0], x, y, 0, 1);
			VectorSet4(quadVerts[1], x + w, y, 0, 1);
			VectorSet4(quadVerts[2], x + w, y + h, 0, 1);
			VectorSet4(quadVerts[3], x, y + h, 0, 1);

			RB_InstantQuad(quadVerts);
		}
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}

/*
=============
RB_ColorMask

=============
*/
const void *RB_ColorMask(const void *data)
{
	const colorMaskCommand_t *cmd = data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (glRefConfig.framebufferObject)
	{
		// reverse color mask, so 0 0 0 0 is the default
		backEnd.colorMask[0] = !cmd->rgba[0];
		backEnd.colorMask[1] = !cmd->rgba[1];
		backEnd.colorMask[2] = !cmd->rgba[2];
		backEnd.colorMask[3] = !cmd->rgba[3];
	}

	qglColorMask(cmd->rgba[0], cmd->rgba[1], cmd->rgba[2], cmd->rgba[3]);
	
	return (const void *)(cmd + 1);
}

/*
=============
RB_ClearDepth

=============
*/
const void *RB_ClearDepth(const void *data)
{
	const clearDepthCommand_t *cmd = data;
	
	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	// texture swapping test
	if (r_showImages->integer)
		RB_ShowImages();

	if (glRefConfig.framebufferObject)
	{
		if (!tr.renderFbo || backEnd.framePostProcessed)
		{
			FBO_Bind(NULL);
		}
		else
		{
			FBO_Bind(tr.renderFbo);
		}
	}

	qglClear(GL_DEPTH_BUFFER_BIT);

	// if we're doing MSAA, clear the depth texture for the resolve buffer
	if (tr.msaaResolveFbo)
	{
		FBO_Bind(tr.msaaResolveFbo);
		qglClear(GL_DEPTH_BUFFER_BIT);
	}

	
	return (const void *)(cmd + 1);
}


/*
=============
RB_SwapBuffers

=============
*/
const void	*RB_SwapBuffers( const void *data ) {
	const swapBuffersCommand_t	*cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}

	if (glRefConfig.framebufferObject)
	{
		if (!backEnd.framePostProcessed)
		{
			if (tr.msaaResolveFbo && r_hdr->integer)
			{
				// Resolving an RGB16F MSAA FBO to the screen messes with the brightness, so resolve to an RGB16F FBO first
				FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
				FBO_FastBlit(tr.msaaResolveFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
			else if (tr.renderFbo)
			{
				FBO_FastBlit(tr.renderFbo, NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_NEAREST);
			}
		}
	}

	if ( !glState.finishCalled ) {
		qglFinish();
	}

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	GLimp_EndFrame();

	backEnd.framePostProcessed = qfalse;
	backEnd.projection2D = qfalse;

	return (const void *)(cmd + 1);
}

/*
=============
RB_CapShadowMap

=============
*/
const void *RB_CapShadowMap(const void *data)
{
	const capShadowmapCommand_t *cmd = data;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (cmd->map != -1)
	{
		if (cmd->cubeSide != -1)
		{
			if (tr.shadowCubemaps[cmd->map])
			{
				qglCopyTextureImage2DEXT(tr.shadowCubemaps[cmd->map]->texnum, GL_TEXTURE_CUBE_MAP_POSITIVE_X + cmd->cubeSide, 0, GL_RGBA8, backEnd.refdef.x, glConfig.vidHeight - ( backEnd.refdef.y + PSHADOW_MAP_SIZE ), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, 0);
			}
		}
		else
		{
			if (tr.pshadowMaps[cmd->map])
			{
				qglCopyTextureImage2DEXT(tr.pshadowMaps[cmd->map]->texnum, GL_TEXTURE_2D, 0, GL_RGBA8, backEnd.refdef.x, glConfig.vidHeight - (backEnd.refdef.y + PSHADOW_MAP_SIZE), PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, 0);
			}
		}
	}

	return (const void *)(cmd + 1);
}


/*
=============
RB_PostProcess

=============
*/
const void *RB_PostProcess(const void *data)
{
	const postProcessCommand_t *cmd = data;
	FBO_t *srcFbo;
	ivec4_t srcBox, dstBox;
	qboolean autoExposure;

	// finish any 2D drawing if needed
	if(tess.numIndexes)
		RB_EndSurface();

	if (!glRefConfig.framebufferObject || !r_postProcess->integer)
	{
		// do nothing
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		backEnd.refdef = cmd->refdef;
		backEnd.viewParms = cmd->viewParms;
	}

	srcFbo = tr.renderFbo;
	if (tr.msaaResolveFbo)
	{
		// Resolve the MSAA before anything else
		// Can't resolve just part of the MSAA FBO, so multiple views will suffer a performance hit here
		FBO_FastBlit(tr.renderFbo, NULL, tr.msaaResolveFbo, NULL, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		srcFbo = tr.msaaResolveFbo;
	}

	dstBox[0] = backEnd.viewParms.viewportX;
	dstBox[1] = backEnd.viewParms.viewportY;
	dstBox[2] = backEnd.viewParms.viewportWidth;
	dstBox[3] = backEnd.viewParms.viewportHeight;

	if (r_ssao->integer)
	{
		srcBox[0] = backEnd.viewParms.viewportX      * tr.screenSsaoImage->width  / (float)glConfig.vidWidth;
		srcBox[1] = backEnd.viewParms.viewportY      * tr.screenSsaoImage->height / (float)glConfig.vidHeight;
		srcBox[2] = backEnd.viewParms.viewportWidth  * tr.screenSsaoImage->width  / (float)glConfig.vidWidth;
		srcBox[3] = backEnd.viewParms.viewportHeight * tr.screenSsaoImage->height / (float)glConfig.vidHeight;

		FBO_Blit(tr.screenSsaoFbo, srcBox, NULL, srcFbo, dstBox, NULL, NULL, GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO);
	}

	srcBox[0] = backEnd.viewParms.viewportX;
	srcBox[1] = backEnd.viewParms.viewportY;
	srcBox[2] = backEnd.viewParms.viewportWidth;
	srcBox[3] = backEnd.viewParms.viewportHeight;

	if (srcFbo)
	{
		if (r_hdr->integer && (r_toneMap->integer || r_forceToneMap->integer))
		{
			autoExposure = r_autoExposure->integer || r_forceAutoExposure->integer;
			RB_ToneMap(srcFbo, srcBox, NULL, dstBox, autoExposure);
		}
		else if (r_cameraExposure->value == 0.0f)
		{
			FBO_FastBlit(srcFbo, srcBox, NULL, dstBox, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		}
		else
		{
			vec4_t color;

			color[0] =
			color[1] =
			color[2] = pow(2, r_cameraExposure->value); //exp2(r_cameraExposure->value);
			color[3] = 1.0f;

			FBO_Blit(srcFbo, srcBox, NULL, NULL, dstBox, NULL, color, 0);
		}
	}

	if (r_drawSunRays->integer)
		RB_SunRays(NULL, srcBox, NULL, dstBox);

	if (1)
		RB_BokehBlur(NULL, srcBox, NULL, dstBox, backEnd.refdef.blurFactor);
	else
		RB_GaussianBlur(backEnd.refdef.blurFactor);

#if 0
	if (0)
	{
		vec4_t quadVerts[4];
		vec2_t texCoords[4];
		ivec4_t iQtrBox;
		vec4_t box;
		vec4_t viewInfo;
		static float scale = 5.0f;

		scale -= 0.005f;
		if (scale < 0.01f)
			scale = 5.0f;

		FBO_FastBlit(NULL, NULL, tr.quarterFbo[0], NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		iQtrBox[0] = backEnd.viewParms.viewportX      * tr.quarterImage[0]->width / (float)glConfig.vidWidth;
		iQtrBox[1] = backEnd.viewParms.viewportY      * tr.quarterImage[0]->height / (float)glConfig.vidHeight;
		iQtrBox[2] = backEnd.viewParms.viewportWidth  * tr.quarterImage[0]->width / (float)glConfig.vidWidth;
		iQtrBox[3] = backEnd.viewParms.viewportHeight * tr.quarterImage[0]->height / (float)glConfig.vidHeight;

		qglViewport(iQtrBox[0], iQtrBox[1], iQtrBox[2], iQtrBox[3]);
		qglScissor(iQtrBox[0], iQtrBox[1], iQtrBox[2], iQtrBox[3]);

		VectorSet4(box, 0.0f, 0.0f, 1.0f, 1.0f);

		texCoords[0][0] = box[0]; texCoords[0][1] = box[3];
		texCoords[1][0] = box[2]; texCoords[1][1] = box[3];
		texCoords[2][0] = box[2]; texCoords[2][1] = box[1];
		texCoords[3][0] = box[0]; texCoords[3][1] = box[1];

		VectorSet4(box, -1.0f, -1.0f, 1.0f, 1.0f);

		VectorSet4(quadVerts[0], box[0], box[3], 0, 1);
		VectorSet4(quadVerts[1], box[2], box[3], 0, 1);
		VectorSet4(quadVerts[2], box[2], box[1], 0, 1);
		VectorSet4(quadVerts[3], box[0], box[1], 0, 1);

		GL_State(GLS_DEPTHTEST_DISABLE);


		VectorSet4(viewInfo, backEnd.viewParms.zFar / r_znear->value, backEnd.viewParms.zFar, 0.0, 0.0);

		viewInfo[2] = scale / (float)(tr.quarterImage[0]->width);
		viewInfo[3] = scale / (float)(tr.quarterImage[0]->height);

		FBO_Bind(tr.quarterFbo[1]);
		GLSL_BindProgram(&tr.depthBlurShader[2]);
		GL_BindToTMU(tr.quarterImage[0], TB_COLORMAP);
		GLSL_SetUniformVec4(&tr.depthBlurShader[2], UNIFORM_VIEWINFO, viewInfo);
		RB_InstantQuad2(quadVerts, texCoords);

		FBO_Bind(tr.quarterFbo[0]);
		GLSL_BindProgram(&tr.depthBlurShader[3]);
		GL_BindToTMU(tr.quarterImage[1], TB_COLORMAP);
		GLSL_SetUniformVec4(&tr.depthBlurShader[3], UNIFORM_VIEWINFO, viewInfo);
		RB_InstantQuad2(quadVerts, texCoords);

		SetViewportAndScissor();

		FBO_FastBlit(tr.quarterFbo[1], NULL, NULL, NULL, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		FBO_Bind(NULL);
	}
#endif

	if (0 && r_sunlightMode->integer)
	{
		ivec4_t dstBox;
		VectorSet4(dstBox, 0, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[0], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 128, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[1], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 256, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[2], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 384, 0, 128, 128);
		FBO_BlitFromTexture(tr.sunShadowDepthImage[3], NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		ivec4_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.renderDepthImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
		VectorSet4(dstBox, 512, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.screenShadowImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

	if (0)
	{
		ivec4_t dstBox;
		VectorSet4(dstBox, 256, glConfig.vidHeight - 256, 256, 256);
		FBO_BlitFromTexture(tr.sunRaysImage, NULL, NULL, NULL, dstBox, NULL, NULL, 0);
	}

#if 0
	if (r_cubeMapping->integer && tr.numCubemaps)
	{
		ivec4_t dstBox;
		int cubemapIndex = R_CubemapForPoint( backEnd.viewParms.or.origin );

		if (cubemapIndex)
		{
			VectorSet4(dstBox, 0, glConfig.vidHeight - 256, 256, 256);
			//FBO_BlitFromTexture(tr.renderCubeImage, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
			FBO_BlitFromTexture(tr.cubemaps[cubemapIndex - 1].image, NULL, NULL, NULL, dstBox, &tr.testcubeShader, NULL, 0);
		}
	}
#endif

	backEnd.framePostProcessed = qtrue;

	return (const void *)(cmd + 1);
}

// FIXME: put this function declaration elsewhere
void R_SaveDDS(const char *filename, byte *pic, int width, int height, int depth);

/*
=============
RB_ExportCubemaps

=============
*/
const void *RB_ExportCubemaps(const void *data)
{
	const exportCubemapsCommand_t *cmd = data;

	// finish any 2D drawing if needed
	if (tess.numIndexes)
		RB_EndSurface();

	if (!glRefConfig.framebufferObject || !tr.world || tr.numCubemaps == 0)
	{
		// do nothing
		ri.Printf(PRINT_ALL, "Nothing to export!\n");
		return (const void *)(cmd + 1);
	}

	if (cmd)
	{
		FBO_t *oldFbo = glState.currentFBO;
		int sideSize = r_cubemapSize->integer * r_cubemapSize->integer * 4;
		byte *cubemapPixels = ri.Malloc(sideSize * 6);
		int i, j;

		FBO_Bind(tr.renderCubeFbo);

		for (i = 0; i < tr.numCubemaps; i++)
		{
			char filename[MAX_QPATH];
			cubemap_t *cubemap = &tr.cubemaps[i];
			byte *p = cubemapPixels;

			for (j = 0; j < 6; j++)
			{
				FBO_AttachImage(tr.renderCubeFbo, cubemap->image, GL_COLOR_ATTACHMENT0_EXT, j);
				qglReadPixels(0, 0, r_cubemapSize->integer, r_cubemapSize->integer, GL_RGBA, GL_UNSIGNED_BYTE, p);
				p += sideSize;
			}

			if (cubemap->name[0])
			{
				COM_StripExtension(cubemap->name, filename, MAX_QPATH);
				Q_strcat(filename, MAX_QPATH, ".dds");
			}
			else
			{
				Com_sprintf(filename, MAX_QPATH, "cubemaps/%s/%03d.dds", tr.world->baseName, i);
			}

			R_SaveDDS(filename, cubemapPixels, r_cubemapSize->integer, r_cubemapSize->integer, 6);
			ri.Printf(PRINT_ALL, "Saved cubemap %d as %s\n", i, filename);
		}

		FBO_Bind(oldFbo);

		ri.Free(cubemapPixels);
	}

	return (const void *)(cmd + 1);
}


/*
====================
RB_ExecuteRenderCommands
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int		t1, t2;

	t1 = ri.Milliseconds ();

	while ( 1 ) {
		data = PADP(data, sizeof(void *));

		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;
		case RC_SCREENSHOT:
			data = RB_TakeScreenshotCmd( data );
			break;
		case RC_VIDEOFRAME:
			data = RB_TakeVideoFrameCmd( data );
			break;
		case RC_COLORMASK:
			data = RB_ColorMask(data);
			break;
		case RC_CLEARDEPTH:
			data = RB_ClearDepth(data);
			break;
		case RC_CAPSHADOWMAP:
			data = RB_CapShadowMap(data);
			break;
		case RC_POSTPROCESS:
			data = RB_PostProcess(data);
			break;
		case RC_EXPORT_CUBEMAPS:
			data = RB_ExportCubemaps(data);
			break;
		case RC_END_OF_LIST:
		default:
			// finish any 2D drawing if needed
			if(tess.numIndexes)
				RB_EndSurface();

			// stop rendering
			t2 = ri.Milliseconds ();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

}
