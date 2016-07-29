/*
===========================================================================
Copyright (C) 2006 Kirk Barnes
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_fbo.c
#include "tr_local.h"

#include "tr_dsa.h"

/*
=============
R_CheckFBO
=============
*/
qboolean R_CheckFBO(const FBO_t * fbo)
{
	GLenum code = qglCheckNamedFramebufferStatusEXT(fbo->frameBuffer, GL_FRAMEBUFFER_EXT);

	if(code == GL_FRAMEBUFFER_COMPLETE_EXT)
		return qtrue;

	// an error occured
	switch (code)
	{
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Unsupported framebuffer format\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete attachment\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing attachment\n", fbo->name);
			break;

			//case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
			//  ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, duplicate attachment\n", fbo->name);
			//  break;

		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, attached images must have same dimensions\n",
					  fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, attached images must have same format\n",
					  fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing draw buffer\n", fbo->name);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) Framebuffer incomplete, missing read buffer\n", fbo->name);
			break;

		default:
			ri.Printf(PRINT_WARNING, "R_CheckFBO: (%s) unknown error 0x%X\n", fbo->name, code);
			break;
	}

	return qfalse;
}

/*
============
FBO_Create
============
*/
FBO_t          *FBO_Create(const char *name, int width, int height)
{
	FBO_t          *fbo;

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "FBO_Create: \"%s\" is too long", name);
	}

	if(width <= 0 || width > glRefConfig.maxRenderbufferSize)
	{
		ri.Error(ERR_DROP, "FBO_Create: bad width %i", width);
	}

	if(height <= 0 || height > glRefConfig.maxRenderbufferSize)
	{
		ri.Error(ERR_DROP, "FBO_Create: bad height %i", height);
	}

	if(tr.numFBOs == MAX_FBOS)
	{
		ri.Error(ERR_DROP, "FBO_Create: MAX_FBOS hit");
	}

	fbo = tr.fbos[tr.numFBOs] = ri.Hunk_Alloc(sizeof(*fbo), h_low);
	Q_strncpyz(fbo->name, name, sizeof(fbo->name));
	fbo->index = tr.numFBOs++;
	fbo->width = width;
	fbo->height = height;

	qglGenFramebuffersEXT(1, &fbo->frameBuffer);

	return fbo;
}

/*
=================
FBO_CreateBuffer
=================
*/
void FBO_CreateBuffer(FBO_t *fbo, int format, int index, int multisample)
{
	uint32_t *pRenderBuffer;
	GLenum attachment;
	qboolean absent;

	switch(format)
	{
		case GL_RGB:
		case GL_RGBA:
		case GL_RGB8:
		case GL_RGBA8:
		case GL_RGB16F_ARB:
		case GL_RGBA16F_ARB:
		case GL_RGB32F_ARB:
		case GL_RGBA32F_ARB:
			fbo->colorFormat = format;
			pRenderBuffer = &fbo->colorBuffers[index];
			attachment = GL_COLOR_ATTACHMENT0_EXT + index;
			break;

		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16_ARB:
		case GL_DEPTH_COMPONENT24_ARB:
		case GL_DEPTH_COMPONENT32_ARB:
			fbo->depthFormat = format;
			pRenderBuffer = &fbo->depthBuffer;
			attachment = GL_DEPTH_ATTACHMENT_EXT;
			break;

		case GL_STENCIL_INDEX:
		case GL_STENCIL_INDEX1_EXT:
		case GL_STENCIL_INDEX4_EXT:
		case GL_STENCIL_INDEX8_EXT:
		case GL_STENCIL_INDEX16_EXT:
			fbo->stencilFormat = format;
			pRenderBuffer = &fbo->stencilBuffer;
			attachment = GL_STENCIL_ATTACHMENT_EXT;
			break;

		case GL_DEPTH_STENCIL_EXT:
		case GL_DEPTH24_STENCIL8_EXT:
			fbo->packedDepthStencilFormat = format;
			pRenderBuffer = &fbo->packedDepthStencilBuffer;
			attachment = 0; // special for stencil and depth
			break;

		default:
			ri.Printf(PRINT_WARNING, "FBO_CreateBuffer: invalid format %d\n", format);
			return;
	}

	absent = *pRenderBuffer == 0;
	if (absent)
		qglGenRenderbuffersEXT(1, pRenderBuffer);

	if (multisample && glRefConfig.framebufferMultisample)
		qglNamedRenderbufferStorageMultisampleEXT(*pRenderBuffer, multisample, format, fbo->width, fbo->height);
	else
		qglNamedRenderbufferStorageEXT(*pRenderBuffer, format, fbo->width, fbo->height);

	if(absent)
	{
		if (attachment == 0)
		{
			qglNamedFramebufferRenderbufferEXT(fbo->frameBuffer, GL_DEPTH_ATTACHMENT_EXT,   GL_RENDERBUFFER_EXT, *pRenderBuffer);
			qglNamedFramebufferRenderbufferEXT(fbo->frameBuffer, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, *pRenderBuffer);
		}
		else
		{
			qglNamedFramebufferRenderbufferEXT(fbo->frameBuffer, attachment, GL_RENDERBUFFER_EXT, *pRenderBuffer);
		}
	}
}


/*
=================
FBO_AttachImage
=================
*/
void FBO_AttachImage(FBO_t *fbo, image_t *image, GLenum attachment, GLuint cubemapside)
{
	GLenum target = GL_TEXTURE_2D;
	int index;

	if (image->flags & IMGFLAG_CUBEMAP)
		target = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + cubemapside;

	qglNamedFramebufferTexture2DEXT(fbo->frameBuffer, attachment, target, image->texnum, 0);
	index = attachment - GL_COLOR_ATTACHMENT0_EXT;
	if (index >= 0 && index <= 15)
		fbo->colorImage[index] = image;
}


/*
============
FBO_Bind
============
*/
void FBO_Bind(FBO_t * fbo)
{
	if (glState.currentFBO == fbo)
		return;
		
	if (r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- FBO_Bind( %s ) ---\n", fbo ? fbo->name : "NULL"));
	}

	GL_BindFramebuffer(GL_FRAMEBUFFER_EXT, fbo ? fbo->frameBuffer : 0);
	glState.currentFBO = fbo;
}

/*
============
FBO_Init
============
*/
void FBO_Init(void)
{
	int             i;
	int             hdrFormat, multisample = 0;

	ri.Printf(PRINT_ALL, "------- FBO_Init -------\n");

	if(!glRefConfig.framebufferObject)
		return;

	tr.numFBOs = 0;

	GL_CheckErrors();

	R_IssuePendingRenderCommands();

	hdrFormat = GL_RGBA8;
	if (r_hdr->integer && glRefConfig.framebufferObject && glRefConfig.textureFloat)
		hdrFormat = GL_RGBA16F_ARB;

	if (glRefConfig.framebufferMultisample)
		qglGetIntegerv(GL_MAX_SAMPLES_EXT, &multisample);

	if (r_ext_framebuffer_multisample->integer < multisample)
		multisample = r_ext_framebuffer_multisample->integer;

	if (multisample < 2 || !glRefConfig.framebufferBlit)
		multisample = 0;

	if (multisample != r_ext_framebuffer_multisample->integer)
		ri.Cvar_SetValue("r_ext_framebuffer_multisample", (float)multisample);
	
	// only create a render FBO if we need to resolve MSAA or do HDR
	// otherwise just render straight to the screen (tr.renderFbo = NULL)
	if (multisample && glRefConfig.framebufferMultisample)
	{
		tr.renderFbo = FBO_Create("_render", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, multisample);
		FBO_CreateBuffer(tr.renderFbo, GL_DEPTH_COMPONENT24_ARB, 0, multisample);
		R_CheckFBO(tr.renderFbo);

		tr.msaaResolveFbo = FBO_Create("_msaaResolve", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_AttachImage(tr.msaaResolveFbo, tr.renderImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		FBO_AttachImage(tr.msaaResolveFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT_EXT, 0);
		R_CheckFBO(tr.msaaResolveFbo);
	}
	else if (r_hdr->integer)
	{
		tr.renderFbo = FBO_Create("_render", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_AttachImage(tr.renderFbo, tr.renderImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		FBO_AttachImage(tr.renderFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT_EXT, 0);
		R_CheckFBO(tr.renderFbo);
	}

	// clear render buffer
	// this fixes the corrupt screen bug with r_hdr 1 on older hardware
	if (tr.renderFbo)
	{
		GL_BindFramebuffer(GL_FRAMEBUFFER_EXT, tr.renderFbo->frameBuffer);
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	if (tr.screenScratchImage)
	{
		tr.screenScratchFbo = FBO_Create("screenScratch", tr.screenScratchImage->width, tr.screenScratchImage->height);
		FBO_AttachImage(tr.screenScratchFbo, tr.screenScratchImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		FBO_AttachImage(tr.screenScratchFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT_EXT, 0);
		R_CheckFBO(tr.screenScratchFbo);
	}

	if (tr.sunRaysImage)
	{
		tr.sunRaysFbo = FBO_Create("_sunRays", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_AttachImage(tr.sunRaysFbo, tr.sunRaysImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		FBO_AttachImage(tr.sunRaysFbo, tr.renderDepthImage, GL_DEPTH_ATTACHMENT_EXT, 0);
		R_CheckFBO(tr.sunRaysFbo);
	}

	// FIXME: Don't use separate color/depth buffers for a shadow buffer
	if (MAX_DRAWN_PSHADOWS && tr.pshadowMaps[0])
	{
		for( i = 0; i < MAX_DRAWN_PSHADOWS; i++)
		{
			tr.pshadowFbos[i] = FBO_Create(va("_shadowmap%d", i), tr.pshadowMaps[i]->width, tr.pshadowMaps[i]->height);
			FBO_AttachImage(tr.pshadowFbos[i], tr.pshadowMaps[i], GL_COLOR_ATTACHMENT0_EXT, 0);
			FBO_CreateBuffer(tr.pshadowFbos[i], GL_DEPTH_COMPONENT24_ARB, 0, 0);
			R_CheckFBO(tr.pshadowFbos[i]);
		}
	}

	if (tr.sunShadowDepthImage[0])
	{
		for (i = 0; i < 4; i++)
		{
			tr.sunShadowFbo[i] = FBO_Create("_sunshadowmap", tr.sunShadowDepthImage[i]->width, tr.sunShadowDepthImage[i]->height);
			// FIXME: this next line wastes 16mb with 4x1024x1024 sun shadow maps, skip if OpenGL 4.3+ or ARB_framebuffer_no_attachments
			// This at least gets sun shadows working on older GPUs (Intel)
			FBO_CreateBuffer(tr.sunShadowFbo[i], GL_RGBA8, 0, 0);
			FBO_AttachImage(tr.sunShadowFbo[i], tr.sunShadowDepthImage[i], GL_DEPTH_ATTACHMENT_EXT, 0);
			R_CheckFBO(tr.sunShadowFbo[i]);
		}
	}

	if (tr.screenShadowImage)
	{
		tr.screenShadowFbo = FBO_Create("_screenshadow", tr.screenShadowImage->width, tr.screenShadowImage->height);
		FBO_AttachImage(tr.screenShadowFbo, tr.screenShadowImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		R_CheckFBO(tr.screenShadowFbo);
	}

	if (tr.textureScratchImage[0])
	{
		for (i = 0; i < 2; i++)
		{
			tr.textureScratchFbo[i] = FBO_Create(va("_texturescratch%d", i), tr.textureScratchImage[i]->width, tr.textureScratchImage[i]->height);
			FBO_AttachImage(tr.textureScratchFbo[i], tr.textureScratchImage[i], GL_COLOR_ATTACHMENT0_EXT, 0);
			R_CheckFBO(tr.textureScratchFbo[i]);
		}
	}

	if (tr.calcLevelsImage)
	{
		tr.calcLevelsFbo = FBO_Create("_calclevels", tr.calcLevelsImage->width, tr.calcLevelsImage->height);
		FBO_AttachImage(tr.calcLevelsFbo, tr.calcLevelsImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		R_CheckFBO(tr.calcLevelsFbo);
	}

	if (tr.targetLevelsImage)
	{
		tr.targetLevelsFbo = FBO_Create("_targetlevels", tr.targetLevelsImage->width, tr.targetLevelsImage->height);
		FBO_AttachImage(tr.targetLevelsFbo, tr.targetLevelsImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		R_CheckFBO(tr.targetLevelsFbo);
	}

	if (tr.quarterImage[0])
	{
		for (i = 0; i < 2; i++)
		{
			tr.quarterFbo[i] = FBO_Create(va("_quarter%d", i), tr.quarterImage[i]->width, tr.quarterImage[i]->height);
			FBO_AttachImage(tr.quarterFbo[i], tr.quarterImage[i], GL_COLOR_ATTACHMENT0_EXT, 0);
			R_CheckFBO(tr.quarterFbo[i]);
		}
	}

	if (tr.hdrDepthImage)
	{
		tr.hdrDepthFbo = FBO_Create("_hdrDepth", tr.hdrDepthImage->width, tr.hdrDepthImage->height);
		FBO_AttachImage(tr.hdrDepthFbo, tr.hdrDepthImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		R_CheckFBO(tr.hdrDepthFbo);
	}

	if (tr.screenSsaoImage)
	{
		tr.screenSsaoFbo = FBO_Create("_screenssao", tr.screenSsaoImage->width, tr.screenSsaoImage->height);
		FBO_AttachImage(tr.screenSsaoFbo, tr.screenSsaoImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		R_CheckFBO(tr.screenSsaoFbo);
	}

	if (tr.renderCubeImage)
	{
		tr.renderCubeFbo = FBO_Create("_renderCubeFbo", tr.renderCubeImage->width, tr.renderCubeImage->height);
		FBO_AttachImage(tr.renderCubeFbo, tr.renderCubeImage, GL_COLOR_ATTACHMENT0_EXT, 0);
		FBO_CreateBuffer(tr.renderCubeFbo, GL_DEPTH_COMPONENT24_ARB, 0, 0);
		R_CheckFBO(tr.renderCubeFbo);
	}

	GL_CheckErrors();

	GL_BindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	glState.currentFBO = NULL;
}

/*
============
FBO_Shutdown
============
*/
void FBO_Shutdown(void)
{
	int             i, j;
	FBO_t          *fbo;

	ri.Printf(PRINT_ALL, "------- FBO_Shutdown -------\n");

	if(!glRefConfig.framebufferObject)
		return;

	FBO_Bind(NULL);

	for(i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		for(j = 0; j < glRefConfig.maxColorAttachments; j++)
		{
			if(fbo->colorBuffers[j])
				qglDeleteRenderbuffersEXT(1, &fbo->colorBuffers[j]);
		}

		if(fbo->depthBuffer)
			qglDeleteRenderbuffersEXT(1, &fbo->depthBuffer);

		if(fbo->stencilBuffer)
			qglDeleteRenderbuffersEXT(1, &fbo->stencilBuffer);

		if(fbo->frameBuffer)
			qglDeleteFramebuffersEXT(1, &fbo->frameBuffer);
	}
}

/*
============
R_FBOList_f
============
*/
void R_FBOList_f(void)
{
	int             i;
	FBO_t          *fbo;

	if(!glRefConfig.framebufferObject)
	{
		ri.Printf(PRINT_ALL, "GL_EXT_framebuffer_object is not available.\n");
		return;
	}

	ri.Printf(PRINT_ALL, "             size       name\n");
	ri.Printf(PRINT_ALL, "----------------------------------------------------------\n");

	for(i = 0; i < tr.numFBOs; i++)
	{
		fbo = tr.fbos[i];

		ri.Printf(PRINT_ALL, "  %4i: %4i %4i %s\n", i, fbo->width, fbo->height, fbo->name);
	}

	ri.Printf(PRINT_ALL, " %i FBOs\n", tr.numFBOs);
}

void FBO_BlitFromTexture(struct image_s *src, vec4_t inSrcTexCorners, vec2_t inSrcTexScale, FBO_t *dst, ivec4_t inDstBox, struct shaderProgram_s *shaderProgram, vec4_t inColor, int blend)
{
	ivec4_t dstBox;
	vec4_t color;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	vec2_t invTexRes;
	FBO_t *oldFbo = glState.currentFBO;
	mat4_t projection;
	int width, height;

	if (!src)
	{
		ri.Printf(PRINT_WARNING, "Tried to blit from a NULL texture!\n");
		return;
	}

	width  = dst ? dst->width  : glConfig.vidWidth;
	height = dst ? dst->height : glConfig.vidHeight;

	if (inSrcTexCorners)
	{
		VectorSet2(texCoords[0], inSrcTexCorners[0], inSrcTexCorners[1]);
		VectorSet2(texCoords[1], inSrcTexCorners[2], inSrcTexCorners[1]);
		VectorSet2(texCoords[2], inSrcTexCorners[2], inSrcTexCorners[3]);
		VectorSet2(texCoords[3], inSrcTexCorners[0], inSrcTexCorners[3]);
	}
	else
	{
		VectorSet2(texCoords[0], 0.0f, 1.0f);
		VectorSet2(texCoords[1], 1.0f, 1.0f);
		VectorSet2(texCoords[2], 1.0f, 0.0f);
		VectorSet2(texCoords[3], 0.0f, 0.0f);
	}

	// framebuffers are 0 bottom, Y up.
	if (inDstBox)
	{
		dstBox[0] = inDstBox[0];
		dstBox[1] = height - inDstBox[1] - inDstBox[3];
		dstBox[2] = inDstBox[0] + inDstBox[2];
		dstBox[3] = height - inDstBox[1];
	}
	else
	{
		VectorSet4(dstBox, 0, height, width, 0);
	}

	if (inSrcTexScale)
	{
		VectorCopy2(inSrcTexScale, invTexRes);
	}
	else
	{
		VectorSet2(invTexRes, 1.0f, 1.0f);
	}

	if (inColor)
	{
		VectorCopy4(inColor, color);
	}
	else
	{
		VectorCopy4(colorWhite, color);
	}

	if (!shaderProgram)
	{
		shaderProgram = &tr.textureColorShader;
	}

	FBO_Bind(dst);

	qglViewport( 0, 0, width, height );
	qglScissor( 0, 0, width, height );

	Mat4Ortho(0, width, height, 0, 0, 1, projection);

	GL_Cull( CT_TWO_SIDED );

	GL_BindToTMU(src, TB_COLORMAP);

	VectorSet4(quadVerts[0], dstBox[0], dstBox[1], 0.0f, 1.0f);
	VectorSet4(quadVerts[1], dstBox[2], dstBox[1], 0.0f, 1.0f);
	VectorSet4(quadVerts[2], dstBox[2], dstBox[3], 0.0f, 1.0f);
	VectorSet4(quadVerts[3], dstBox[0], dstBox[3], 0.0f, 1.0f);

	invTexRes[0] /= src->width;
	invTexRes[1] /= src->height;

	GL_State( blend );

	GLSL_BindProgram(shaderProgram);
	
	GLSL_SetUniformMat4(shaderProgram, UNIFORM_MODELVIEWPROJECTIONMATRIX, projection);
	GLSL_SetUniformVec4(shaderProgram, UNIFORM_COLOR, color);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_INVTEXRES, invTexRes);
	GLSL_SetUniformVec2(shaderProgram, UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax);
	GLSL_SetUniformVec3(shaderProgram, UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear);

	RB_InstantQuad2(quadVerts, texCoords);

	FBO_Bind(oldFbo);
}

void FBO_Blit(FBO_t *src, ivec4_t inSrcBox, vec2_t srcTexScale, FBO_t *dst, ivec4_t dstBox, struct shaderProgram_s *shaderProgram, vec4_t color, int blend)
{
	vec4_t srcTexCorners;

	if (!src)
	{
		ri.Printf(PRINT_WARNING, "Tried to blit from a NULL FBO!\n");
		return;
	}

	if (inSrcBox)
	{
		srcTexCorners[0] =  inSrcBox[0]                / (float)src->width;
		srcTexCorners[1] = (inSrcBox[1] + inSrcBox[3]) / (float)src->height;
		srcTexCorners[2] = (inSrcBox[0] + inSrcBox[2]) / (float)src->width;
		srcTexCorners[3] =  inSrcBox[1]                / (float)src->height;
	}
	else
	{
		VectorSet4(srcTexCorners, 0.0f, 0.0f, 1.0f, 1.0f);
	}

	FBO_BlitFromTexture(src->colorImage[0], srcTexCorners, srcTexScale, dst, dstBox, shaderProgram, color, blend | GLS_DEPTHTEST_DISABLE);
}

void FBO_FastBlit(FBO_t *src, ivec4_t srcBox, FBO_t *dst, ivec4_t dstBox, int buffers, int filter)
{
	ivec4_t srcBoxFinal, dstBoxFinal;
	GLuint srcFb, dstFb;

	if (!glRefConfig.framebufferBlit)
	{
		FBO_Blit(src, srcBox, NULL, dst, dstBox, NULL, NULL, 0);
		return;
	}

	srcFb = src ? src->frameBuffer : 0;
	dstFb = dst ? dst->frameBuffer : 0;

	if (!srcBox)
	{
		int width =  src ? src->width  : glConfig.vidWidth;
		int height = src ? src->height : glConfig.vidHeight;

		VectorSet4(srcBoxFinal, 0, 0, width, height);
	}
	else
	{
		VectorSet4(srcBoxFinal, srcBox[0], srcBox[1], srcBox[0] + srcBox[2], srcBox[1] + srcBox[3]);
	}

	if (!dstBox)
	{
		int width  = dst ? dst->width  : glConfig.vidWidth;
		int height = dst ? dst->height : glConfig.vidHeight;

		VectorSet4(dstBoxFinal, 0, 0, width, height);
	}
	else
	{
		VectorSet4(dstBoxFinal, dstBox[0], dstBox[1], dstBox[0] + dstBox[2], dstBox[1] + dstBox[3]);
	}

	GL_BindFramebuffer(GL_READ_FRAMEBUFFER_EXT, srcFb);
	GL_BindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, dstFb);
	qglBlitFramebufferEXT(srcBoxFinal[0], srcBoxFinal[1], srcBoxFinal[2], srcBoxFinal[3],
	                      dstBoxFinal[0], dstBoxFinal[1], dstBoxFinal[2], dstBoxFinal[3],
						  buffers, filter);

	GL_BindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
	glState.currentFBO = NULL;
}
