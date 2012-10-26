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

/*
=============
R_CheckFBO
=============
*/
qboolean R_CheckFBO(const FBO_t * fbo)
{
	int             code;
	int             id;

	qglGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &id);
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->frameBuffer);

	code = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

	if(code == GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);
		return qtrue;
	}

	// an error occured
	switch (code)
	{
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			break;

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
			//ri.Error(ERR_FATAL, "R_CheckFBO: (%s) unknown error 0x%X", fbo->name, code);
			//assert(0);
			break;
	}

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, id);

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
		ri.Error(ERR_DROP, "FBO_Create: \"%s\" is too long\n", name);
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

	qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, *pRenderBuffer);
	if (multisample && glRefConfig.framebufferMultisample)
	{
		qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, multisample, format, fbo->width, fbo->height);
	}
	else
	{
		qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, format, fbo->width, fbo->height);
	}

	if(absent)
	{
		if (attachment == 0)
		{
			qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,   GL_RENDERBUFFER_EXT, *pRenderBuffer);
			qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, *pRenderBuffer);
		}
		else
			qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, GL_RENDERBUFFER_EXT, *pRenderBuffer);
	}
}


/*
=================
R_AttachFBOTexture1D
=================
*/
void R_AttachFBOTexture1D(int texId, int index)
{
	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture1D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture1DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + index, GL_TEXTURE_1D, texId, 0);
}

/*
=================
R_AttachFBOTexture2D
=================
*/
void R_AttachFBOTexture2D(int target, int texId, int index)
{
	if(target != GL_TEXTURE_2D && (target < GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB || target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB))
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture2D: invalid target %i\n", target);
		return;
	}

	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture2D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + index, target, texId, 0);
}

/*
=================
R_AttachFBOTexture3D
=================
*/
void R_AttachFBOTexture3D(int texId, int index, int zOffset)
{
	if(index < 0 || index >= glRefConfig.maxColorAttachments)
	{
		ri.Printf(PRINT_WARNING, "R_AttachFBOTexture3D: invalid attachment index %i\n", index);
		return;
	}

	qglFramebufferTexture3DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + index, GL_TEXTURE_3D_EXT, texId, 0, zOffset);
}

/*
=================
R_AttachFBOTextureDepth
=================
*/
void R_AttachFBOTextureDepth(int texId)
{
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, texId, 0);
}

/*
=================
R_AttachFBOTexturePackedDepthStencil
=================
*/
void R_AttachFBOTexturePackedDepthStencil(int texId)
{
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, texId, 0);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, texId, 0);
}

void FBO_AttachTextureImage(image_t *img, int index)
{
	if (!glState.currentFBO)
	{
		ri.Printf(PRINT_WARNING, "FBO: attempted to attach a texture image with no FBO bound!\n");
		return;
	}

	R_AttachFBOTexture2D(GL_TEXTURE_2D, img->texnum, index);
	glState.currentFBO->colorImage[index] = img;
}

/*
============
FBO_Bind
============
*/
void FBO_Bind(FBO_t * fbo)
{
	if (fbo && glState.currentFBO == fbo)
		return;
		
	if (r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		if (fbo)
			GLimp_LogComment(va("--- FBO_Bind( %s ) ---\n", fbo->name));
		else
			GLimp_LogComment("--- FBO_Bind ( NULL ) ---\n");
	}

	if (!fbo)
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		//qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		glState.currentFBO = NULL;
		
		return;
	}
		
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->frameBuffer);

	/*
	   if(fbo->colorBuffers[0])
	   {
	   qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo->colorBuffers[0]);
	   }
	 */

	/*
	   if(fbo->depthBuffer)
	   {
	   qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fbo->depthBuffer);
	   qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fbo->depthBuffer);
	   }
	 */

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
	// int             width, height, hdrFormat, multisample;
	int             hdrFormat, multisample;

	ri.Printf(PRINT_ALL, "------- FBO_Init -------\n");

	if(!glRefConfig.framebufferObject)
		return;

	tr.numFBOs = 0;

	GL_CheckErrors();

	// make sure the render thread is stopped
	R_SyncRenderThread();

/*	if(glRefConfig.textureNonPowerOfTwo)
	{
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
	}
	else
	{
		width = NextPowerOfTwo(glConfig.vidWidth);
		height = NextPowerOfTwo(glConfig.vidHeight);
	} */

	hdrFormat = GL_RGBA8;
	if (r_hdr->integer && glRefConfig.framebufferObject && glRefConfig.textureFloat)
	{
		hdrFormat = GL_RGB16F_ARB;
	}

	qglGetIntegerv(GL_MAX_SAMPLES_EXT, &multisample);

	if (r_ext_framebuffer_multisample->integer < multisample)
	{
		multisample = r_ext_framebuffer_multisample->integer;
	}

	if (multisample < 2 || !glRefConfig.framebufferBlit)
		multisample = 0;

	if (multisample != r_ext_framebuffer_multisample->integer)
	{
		ri.Cvar_SetValue("r_ext_framebuffer_multisample", (float)multisample);
	}
	
	if (multisample && glRefConfig.framebufferMultisample)
	{
		tr.renderFbo = FBO_Create("_render", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderFbo);

		FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, multisample);
		FBO_CreateBuffer(tr.renderFbo, GL_DEPTH_COMPONENT24_ARB, 0, multisample);

		R_CheckFBO(tr.renderFbo);


		tr.msaaResolveFbo = FBO_Create("_msaaResolve", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.msaaResolveFbo);

		//FBO_CreateBuffer(tr.msaaResolveFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.renderImage, 0);

		//FBO_CreateBuffer(tr.msaaResolveFbo, GL_DEPTH_COMPONENT24_ARB, 0, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);

		R_CheckFBO(tr.msaaResolveFbo);
	}
	else
	{
		tr.renderFbo = FBO_Create("_render", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.renderFbo);

		//FBO_CreateBuffer(tr.renderFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.renderImage, 0);

		//FBO_CreateBuffer(tr.renderFbo, GL_DEPTH_COMPONENT24_ARB, 0, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);

		R_CheckFBO(tr.renderFbo);
	}

	// clear render buffer
	// this fixes the corrupt screen bug with r_hdr 1 on older hardware
	FBO_Bind(tr.renderFbo);
	qglClearColor( 1, 0, 0.5, 1 );
	qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	FBO_Bind(NULL);

#ifdef REACTION
	{
		tr.godRaysFbo = FBO_Create("_godRays", tr.renderDepthImage->width, tr.renderDepthImage->height);
		FBO_Bind(tr.godRaysFbo);

		//FBO_CreateBuffer(tr.godRaysFbo, GL_RGBA8, 0, multisample);
		FBO_AttachTextureImage(tr.godRaysImage, 0);

		//FBO_CreateBuffer(tr.godRaysFbo, GL_DEPTH_COMPONENT24_ARB, 0, multisample);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);

		R_CheckFBO(tr.godRaysFbo);
	}
#endif

	// FIXME: Don't use separate color/depth buffers for a shadow buffer
	for( i = 0; i < MAX_DRAWN_PSHADOWS; i++)
	{
		tr.pshadowFbos[i] = FBO_Create(va("_shadowmap%d", i), tr.pshadowMaps[i]->width, tr.pshadowMaps[i]->height);
		FBO_Bind(tr.pshadowFbos[i]);

		//FBO_CreateBuffer(tr.pshadowFbos[i], GL_RGBA8, 0, 0);
		FBO_AttachTextureImage(tr.pshadowMaps[i], 0);

		FBO_CreateBuffer(tr.pshadowFbos[i], GL_DEPTH_COMPONENT24_ARB, 0, 0);
		//R_AttachFBOTextureDepth(tr.textureDepthImage->texnum);

		R_CheckFBO(tr.pshadowFbos[i]);
	}

	for ( i = 0; i < 3; i++)
	{
		tr.sunShadowFbo[i] = FBO_Create("_sunshadowmap", tr.sunShadowDepthImage[i]->width, tr.sunShadowDepthImage[i]->height);
		FBO_Bind(tr.sunShadowFbo[i]);

		//FBO_CreateBuffer(tr.sunShadowFbo[i], GL_RGBA8, 0, 0);
		//FBO_AttachTextureImage(tr.sunShadowImage, 0);
		qglDrawBuffer(GL_NONE);
		qglReadBuffer(GL_NONE);

		//FBO_CreateBuffer(tr.sunShadowFbo, GL_DEPTH_COMPONENT24_ARB, 0, 0);
		R_AttachFBOTextureDepth(tr.sunShadowDepthImage[i]->texnum);

		R_CheckFBO(tr.sunShadowFbo[i]);
	}

	for (i = 0; i < 2; i++)
	{
		tr.textureScratchFbo[i] = FBO_Create(va("_texturescratch%d", i), tr.textureScratchImage[i]->width, tr.textureScratchImage[i]->height);
		FBO_Bind(tr.textureScratchFbo[i]);

		//FBO_CreateBuffer(tr.textureScratchFbo[i], GL_RGBA8, 0, 0);
		FBO_AttachTextureImage(tr.textureScratchImage[i], 0);

		R_CheckFBO(tr.textureScratchFbo[i]);
	}

	{
		tr.calcLevelsFbo = FBO_Create("_calclevels", tr.calcLevelsImage->width, tr.calcLevelsImage->height);
		FBO_Bind(tr.calcLevelsFbo);

		//FBO_CreateBuffer(tr.calcLevelsFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.calcLevelsImage, 0);

		R_CheckFBO(tr.calcLevelsFbo);
	}

	{
		tr.targetLevelsFbo = FBO_Create("_targetlevels", tr.targetLevelsImage->width, tr.targetLevelsImage->height);
		FBO_Bind(tr.targetLevelsFbo);

		//FBO_CreateBuffer(tr.targetLevelsFbo, hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.targetLevelsImage, 0);

		R_CheckFBO(tr.targetLevelsFbo);
	}

	{
		//tr.screenScratchFbo = FBO_Create("_screenscratch", width, height);
		tr.screenScratchFbo = FBO_Create("_screenscratch", tr.screenScratchImage->width, tr.screenScratchImage->height);
		FBO_Bind(tr.screenScratchFbo);
		
		//FBO_CreateBuffer(tr.screenScratchFbo, format, 0, 0);
		FBO_AttachTextureImage(tr.screenScratchImage, 0);

		// FIXME: hack: share zbuffer between render fbo and pre-screen fbo
		//FBO_CreateBuffer(tr.screenScratchFbo, GL_DEPTH_COMPONENT24_ARB, 0, 0);
		R_AttachFBOTextureDepth(tr.renderDepthImage->texnum);

		R_CheckFBO(tr.screenScratchFbo);
	}

	for (i = 0; i < 2; i++)
	{
		tr.quarterFbo[i] = FBO_Create(va("_quarter%d", i), tr.quarterImage[i]->width, tr.quarterImage[i]->height);
		FBO_Bind(tr.quarterFbo[i]);

		//FBO_CreateBuffer(tr.quarterFbo[i], hdrFormat, 0, 0);
		FBO_AttachTextureImage(tr.quarterImage[i], 0);

		R_CheckFBO(tr.quarterFbo[i]);
	}

	{
		tr.screenShadowFbo = FBO_Create("_screenshadow", tr.screenShadowImage->width, tr.screenShadowImage->height);
		FBO_Bind(tr.screenShadowFbo);
		
		FBO_AttachTextureImage(tr.screenShadowImage, 0);

		R_CheckFBO(tr.screenShadowFbo);
	}

	if (r_ssao->integer)
	{
		tr.hdrDepthFbo = FBO_Create("_hdrDepth", tr.hdrDepthImage->width, tr.hdrDepthImage->height);
		FBO_Bind(tr.hdrDepthFbo);

		FBO_AttachTextureImage(tr.hdrDepthImage, 0);

		R_CheckFBO(tr.hdrDepthFbo);

		tr.screenSsaoFbo = FBO_Create("_screenssao", tr.screenSsaoImage->width, tr.screenSsaoImage->height);
		FBO_Bind(tr.screenSsaoFbo);
		
		FBO_AttachTextureImage(tr.screenSsaoImage, 0);

		R_CheckFBO(tr.screenSsaoFbo);
	}

	GL_CheckErrors();

	FBO_Bind(NULL);
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

// FIXME
extern void RB_SetGL2D (void);

void FBO_BlitFromTexture(struct image_s *src, vec4i_t inSrcBox, vec2_t inSrcTexScale, FBO_t *dst, vec4i_t inDstBox, struct shaderProgram_s *shaderProgram, vec4_t inColor, int blend)
{
	vec4i_t dstBox, srcBox;
	vec2_t srcTexScale;
	vec4_t color;
	vec4_t quadVerts[4];
	vec2_t texCoords[4];
	vec2_t invTexRes;
	FBO_t *oldFbo = glState.currentFBO;

	if (!src)
		return;

	if (inSrcBox)
	{
		VectorSet4(srcBox, inSrcBox[0], inSrcBox[1], inSrcBox[0] + inSrcBox[2],  inSrcBox[1] + inSrcBox[3]);
	}
	else
	{
		VectorSet4(srcBox, 0, 0, src->width, src->height);
	}

	// framebuffers are 0 bottom, Y up.
	if (inDstBox)
	{
		if (dst)
		{
			dstBox[0] = inDstBox[0];
			dstBox[1] = dst->height - inDstBox[1] - inDstBox[3];
			dstBox[2] = inDstBox[0] + inDstBox[2];
			dstBox[3] = dst->height - inDstBox[1];
		}
		else
		{
			dstBox[0] = inDstBox[0];
			dstBox[1] = glConfig.vidHeight - inDstBox[1] - inDstBox[3];
			dstBox[2] = inDstBox[0] + inDstBox[2];
			dstBox[3] = glConfig.vidHeight - inDstBox[1];
		}
	}
	else if (dst)
	{
		VectorSet4(dstBox, 0, dst->height, dst->width, 0);
	}
	else
	{
		VectorSet4(dstBox, 0, glConfig.vidHeight, glConfig.vidWidth, 0);
	}

	if (inSrcTexScale)
	{
		VectorCopy2(inSrcTexScale, srcTexScale);
	}
	else
	{
		srcTexScale[0] = srcTexScale[1] = 1.0f;
	}

	if (inColor)
	{
		VectorCopy4(inColor, color);
	}
	else
	{
		color[0] = color[1] = color[2] = color[3] = 1.0f;
	}

	if (!shaderProgram)
	{
		shaderProgram = &tr.textureColorShader;
	}

	FBO_Bind(dst);

	RB_SetGL2D();

	GL_SelectTexture(TB_COLORMAP);

	GL_Bind(src);

	VectorSet4(quadVerts[0], dstBox[0], dstBox[1], 0, 1);
	VectorSet4(quadVerts[1], dstBox[2], dstBox[1], 0, 1);
	VectorSet4(quadVerts[2], dstBox[2], dstBox[3], 0, 1);
	VectorSet4(quadVerts[3], dstBox[0], dstBox[3], 0, 1);

	texCoords[0][0] = srcBox[0] / (float)src->width; texCoords[0][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[1][0] = srcBox[2] / (float)src->width; texCoords[1][1] = 1.0f - srcBox[1] / (float)src->height;
	texCoords[2][0] = srcBox[2] / (float)src->width; texCoords[2][1] = 1.0f - srcBox[3] / (float)src->height;
	texCoords[3][0] = srcBox[0] / (float)src->width; texCoords[3][1] = 1.0f - srcBox[3] / (float)src->height;

	invTexRes[0] = 1.0f / src->width  * srcTexScale[0];
	invTexRes[1] = 1.0f / src->height * srcTexScale[1];

	GL_State( blend );

	GLSL_BindProgram(shaderProgram);
	
	GLSL_SetUniformMatrix16(shaderProgram, TEXTURECOLOR_UNIFORM_MODELVIEWPROJECTIONMATRIX, glState.modelviewProjection);
	GLSL_SetUniformVec4(shaderProgram, TEXTURECOLOR_UNIFORM_COLOR, color);
	GLSL_SetUniformVec2(shaderProgram, TEXTURECOLOR_UNIFORM_INVTEXRES, invTexRes);
	GLSL_SetUniformVec2(shaderProgram, TEXTURECOLOR_UNIFORM_AUTOEXPOSUREMINMAX, tr.refdef.autoExposureMinMax);
	GLSL_SetUniformVec3(shaderProgram, TEXTURECOLOR_UNIFORM_TONEMINAVGMAXLINEAR, tr.refdef.toneMinAvgMaxLinear);

	RB_InstantQuad2(quadVerts, texCoords); //, color, shaderProgram, invTexRes);

	FBO_Bind(oldFbo);
}

void FBO_Blit(FBO_t *src, vec4i_t inSrcBox, vec2_t srcTexScale, FBO_t *dst, vec4i_t dstBox, struct shaderProgram_s *shaderProgram, vec4_t color, int blend)
{
	vec4i_t srcBox;

	if (!src)
		return;

	// framebuffers are 0 bottom, Y up.
	if (inSrcBox)
	{
		srcBox[0] = inSrcBox[0];
		srcBox[1] = src->height - inSrcBox[1] - inSrcBox[3];
		srcBox[2] = inSrcBox[2];
		srcBox[3] = inSrcBox[3];
	}
	else
	{
		VectorSet4(srcBox, 0, src->height, src->width, -src->height);
	}

	FBO_BlitFromTexture(src->colorImage[0], srcBox, srcTexScale, dst, dstBox, shaderProgram, color, blend | GLS_DEPTHTEST_DISABLE);
}

void FBO_FastBlit(FBO_t *src, vec4i_t srcBox, FBO_t *dst, vec4i_t dstBox, int buffers, int filter)
{
	vec4i_t srcBoxFinal, dstBoxFinal;
	GLuint srcFb, dstFb;

	if (!glRefConfig.framebufferBlit)
	{
		FBO_Blit(src, srcBox, NULL, dst, dstBox, NULL, NULL, 0);
		return;
	}

	// get to a neutral state first
	FBO_Bind(NULL);

	srcFb = src ? src->frameBuffer : 0;
	dstFb = dst ? dst->frameBuffer : 0;

	if (!srcBox)
	{
		if (src)
		{
			VectorSet4(srcBoxFinal, 0, 0, src->width, src->height);
		}
		else
		{
			VectorSet4(srcBoxFinal, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
		}
	}
	else
	{
		VectorSet4(srcBoxFinal, srcBox[0], srcBox[1], srcBox[0] + srcBox[2], srcBox[1] + srcBox[3]);
	}

	if (!dstBox)
	{
		if (dst)
		{
			VectorSet4(dstBoxFinal, 0, 0, dst->width, dst->height);
		}
		else
		{
			VectorSet4(dstBoxFinal, 0, 0, glConfig.vidWidth, glConfig.vidHeight);
		}
	}
	else
	{
		VectorSet4(dstBoxFinal, dstBox[0], dstBox[1], dstBox[0] + dstBox[2], dstBox[1] + dstBox[3]);
	}

	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, srcFb);
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, dstFb);
	qglBlitFramebufferEXT(srcBoxFinal[0], srcBoxFinal[1], srcBoxFinal[2], srcBoxFinal[3],
	                      dstBoxFinal[0], dstBoxFinal[1], dstBoxFinal[2], dstBoxFinal[3],
						  buffers, filter);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glState.currentFBO = NULL;
}