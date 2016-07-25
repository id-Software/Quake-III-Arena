/*
===========================================================================
Copyright (C) 2011 James Canete (use.less01@gmail.com)

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
// tr_extensions.c - extensions needed by the renderer not in sdl_glimp.c

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include "tr_local.h"
#include "tr_dsa.h"

#define GLE(ret, name, ...) name##proc * qgl##name;
QGL_1_2_PROCS;
QGL_1_3_PROCS;
QGL_1_4_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
#undef GLE

// GL_EXT_framebuffer_object
GLboolean (APIENTRY * qglIsRenderbufferEXT)(GLuint renderbuffer);
void (APIENTRY * qglBindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
void (APIENTRY * qglDeleteRenderbuffersEXT)(GLsizei n, const GLuint *renderbuffers);
void (APIENTRY * qglGenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers);

void (APIENTRY * qglRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);

void (APIENTRY * qglGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint *params);

GLboolean (APIENTRY * qglIsFramebufferEXT)(GLuint framebuffer);
void (APIENTRY * qglBindFramebufferEXT)(GLenum target, GLuint framebuffer);
void (APIENTRY * qglDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);
void (APIENTRY * qglGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);

GLenum (APIENTRY * qglCheckFramebufferStatusEXT)(GLenum target);

void (APIENTRY * qglFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level);
void (APIENTRY * qglFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level);
void (APIENTRY * qglFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level, GLint zoffset);

void (APIENTRY * qglFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget,
	GLuint renderbuffer);

void (APIENTRY * qglGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint *params);

void (APIENTRY * qglGenerateMipmapEXT)(GLenum target);

// GL_EXT_framebuffer_blit
void (APIENTRY * qglBlitFramebufferEXT)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter);

// GL_EXT_framebuffer_multisample
void (APIENTRY * qglRenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples,
	GLenum internalformat, GLsizei width, GLsizei height);

// GL_ARB_vertex_array_object
void (APIENTRY * qglBindVertexArrayARB)(GLuint array);
void (APIENTRY * qglDeleteVertexArraysARB)(GLsizei n, const GLuint *arrays);
void (APIENTRY * qglGenVertexArraysARB)(GLsizei n, GLuint *arrays);
GLboolean (APIENTRY * qglIsVertexArrayARB)(GLuint array);

// GL_EXT_direct_state_access
GLvoid (APIENTRY * qglBindMultiTexture)(GLenum texunit, GLenum target, GLuint texture);
GLvoid (APIENTRY * qglTextureParameterf)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
GLvoid (APIENTRY * qglTextureParameteri)(GLuint texture, GLenum target, GLenum pname, GLint param);
GLvoid (APIENTRY * qglTextureImage2D)(GLuint texture, GLenum target, GLint level, GLint internalformat,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid (APIENTRY * qglTextureSubImage2D)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid (APIENTRY * qglCopyTextureImage2D)(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLvoid (APIENTRY * qglCompressedTextureImage2D)(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
GLvoid (APIENTRY * qglCompressedTextureSubImage2D)(GLuint texture, GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
	GLsizei imageSize, const GLvoid *data);
GLvoid (APIENTRY * qglGenerateTextureMipmap)(GLuint texture, GLenum target);

GLvoid(APIENTRY * qglProgramUniform1i)(GLuint program, GLint location, GLint v0);
GLvoid(APIENTRY * qglProgramUniform1f)(GLuint program, GLint location, GLfloat v0);
GLvoid(APIENTRY * qglProgramUniform2f)(GLuint program, GLint location,
	GLfloat v0, GLfloat v1);
GLvoid(APIENTRY * qglProgramUniform3f)(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid(APIENTRY * qglProgramUniform4f)(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid(APIENTRY * qglProgramUniform1fv)(GLuint program, GLint location,
	GLsizei count, const GLfloat *value);
GLvoid(APIENTRY * qglProgramUniformMatrix4fv)(GLuint program, GLint location,
	GLsizei count, GLboolean transpose,
	const GLfloat *value);

GLvoid(APIENTRY * qglNamedRenderbufferStorage)(GLuint renderbuffer,
	GLenum internalformat, GLsizei width, GLsizei height);

GLvoid(APIENTRY * qglNamedRenderbufferStorageMultisample)(GLuint renderbuffer,
	GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

GLenum(APIENTRY * qglCheckNamedFramebufferStatus)(GLuint framebuffer, GLenum target);
GLvoid(APIENTRY * qglNamedFramebufferTexture2D)(GLuint framebuffer,
	GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLvoid(APIENTRY * qglNamedFramebufferRenderbuffer)(GLuint framebuffer,
	GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);


static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

void GLimp_InitExtraExtensions()
{
	char *extension;
	const char* result[3] = { "...ignoring %s\n", "...using %s\n", "...%s not found\n" };

	sscanf(glConfig.version_string, "%d.%d", &glRefConfig.openglMajorVersion, &glRefConfig.openglMinorVersion);
	if (glRefConfig.openglMajorVersion < 2)
		ri.Error(ERR_FATAL, "OpenGL 2.0 required!");
	ri.Printf(PRINT_ALL, "...using OpenGL %s\n", glConfig.version_string);

	// GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a

#define GLE(ret, name, ...) qgl##name = (name##proc *) SDL_GL_GetProcAddress("gl" #name);

	// OpenGL 1.2, was GL_EXT_draw_range_elements
	QGL_1_2_PROCS;
	glRefConfig.drawRangeElements = r_ext_draw_range_elements->integer ? qtrue : qfalse;
	ri.Printf(PRINT_ALL, result[glRefConfig.drawRangeElements], "glDrawRangeElements()");

	// OpenGL 1.3, was GL_ARB_texture_compression
	QGL_1_3_PROCS;

	// OpenGL 1.4, was GL_EXT_multi_draw_arrays
	QGL_1_4_PROCS;
	glRefConfig.drawRangeElements = r_ext_multi_draw_arrays->integer ? qtrue : qfalse;
	ri.Printf(PRINT_ALL, result[glRefConfig.drawRangeElements], "glMultiDrawElements()");

	// OpenGL 1.5, was GL_ARB_vertex_buffer_object and GL_ARB_occlusion_query
	QGL_1_5_PROCS;
	glRefConfig.occlusionQuery = qtrue;

	// OpenGL 2.0, was GL_ARB_shading_language_100, GL_ARB_vertex_program, GL_ARB_shader_objects, and GL_ARB_vertex_shader
	QGL_2_0_PROCS;

#undef GLE

	if (1)
	{
		char version[256];

		Q_strncpyz(version, (char *)qglGetString(GL_SHADING_LANGUAGE_VERSION), sizeof(version));

		sscanf(version, "%d.%d", &glRefConfig.glslMajorVersion, &glRefConfig.glslMinorVersion);

		ri.Printf(PRINT_ALL, "...using GLSL version %s\n", version);
	}

	glRefConfig.memInfo = MI_NONE;

	if( GLimp_HaveExtension( "GL_NVX_gpu_memory_info" ) )
	{
		glRefConfig.memInfo = MI_NVX;
	}
	else if( GLimp_HaveExtension( "GL_ATI_meminfo" ) )
	{
		glRefConfig.memInfo = MI_ATI;
	}

	extension = "GL_ARB_texture_non_power_of_two";
	glRefConfig.textureNonPowerOfTwo = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		if(1) //(r_ext_texture_non_power_of_two->integer)
		{
			glRefConfig.textureNonPowerOfTwo = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.textureNonPowerOfTwo], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_texture_float
	extension = "GL_ARB_texture_float";
	glRefConfig.textureFloat = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		if( r_ext_texture_float->integer )
		{
			glRefConfig.textureFloat = qtrue;
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.textureFloat], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_half_float_pixel
	extension = "GL_ARB_half_float_pixel";
	glRefConfig.halfFloatPixel = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		if( r_arb_half_float_pixel->integer )
			glRefConfig.halfFloatPixel = qtrue;

		ri.Printf(PRINT_ALL, result[glRefConfig.halfFloatPixel], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_framebuffer_object
	extension = "GL_EXT_framebuffer_object";
	glRefConfig.framebufferObject = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE_EXT, &glRefConfig.maxRenderbufferSize);
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &glRefConfig.maxColorAttachments);

		qglIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC) SDL_GL_GetProcAddress("glIsRenderbufferEXT");
		qglBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) SDL_GL_GetProcAddress("glBindRenderbufferEXT");
		qglDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) SDL_GL_GetProcAddress("glDeleteRenderbuffersEXT");
		qglGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) SDL_GL_GetProcAddress("glGenRenderbuffersEXT");
		qglRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) SDL_GL_GetProcAddress("glRenderbufferStorageEXT");
		qglGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) SDL_GL_GetProcAddress("glGetRenderbufferParameterivEXT");
		qglIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glIsFramebufferEXT");
		qglBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) SDL_GL_GetProcAddress("glBindFramebufferEXT");
		qglDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glDeleteFramebuffersEXT");
		qglGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) SDL_GL_GetProcAddress("glGenFramebuffersEXT");
		qglCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) SDL_GL_GetProcAddress("glCheckFramebufferStatusEXT");
		qglFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture1DEXT");
		qglFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture2DEXT");
		qglFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) SDL_GL_GetProcAddress("glFramebufferTexture3DEXT");
		qglFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) SDL_GL_GetProcAddress("glFramebufferRenderbufferEXT");
		qglGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) SDL_GL_GetProcAddress("glGetFramebufferAttachmentParameterivEXT");
		qglGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) SDL_GL_GetProcAddress("glGenerateMipmapEXT");

		if(r_ext_framebuffer_object->value)
			glRefConfig.framebufferObject = qtrue;

		ri.Printf(PRINT_ALL, result[glRefConfig.framebufferObject], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_packed_depth_stencil
	extension = "GL_EXT_packed_depth_stencil";
	glRefConfig.packedDepthStencil = qfalse;
	if( GLimp_HaveExtension(extension))
	{
		glRefConfig.packedDepthStencil = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.packedDepthStencil], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_framebuffer_blit
	extension = "GL_EXT_framebuffer_blit";
	glRefConfig.framebufferBlit = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		qglBlitFramebufferEXT = (void *)SDL_GL_GetProcAddress("glBlitFramebufferEXT");
		glRefConfig.framebufferBlit = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.framebufferBlit], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_framebuffer_multisample
	extension = "GL_EXT_framebuffer_multisample";
	glRefConfig.framebufferMultisample = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		qglRenderbufferStorageMultisampleEXT = (void *)SDL_GL_GetProcAddress("glRenderbufferStorageMultisampleEXT");
		glRefConfig.framebufferMultisample = qtrue;
		ri.Printf(PRINT_ALL, result[glRefConfig.framebufferMultisample], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	glRefConfig.textureCompression = TCR_NONE;

	// GL_ARB_texture_compression_rgtc
	extension = "GL_ARB_texture_compression_rgtc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer)
			glRefConfig.textureCompression |= TCR_RGTC;

		ri.Printf(PRINT_ALL, result[r_ext_compressed_textures->integer ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	glRefConfig.swizzleNormalmap = r_ext_compressed_textures->integer && !(glRefConfig.textureCompression & TCR_RGTC);

	// GL_ARB_texture_compression_bptc
	extension = "GL_ARB_texture_compression_bptc";
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_compressed_textures->integer >= 2)
			glRefConfig.textureCompression |= TCR_BPTC;

		ri.Printf(PRINT_ALL, result[(r_ext_compressed_textures->integer >= 2) ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_depth_clamp
	extension = "GL_ARB_depth_clamp";
	glRefConfig.depthClamp = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		glRefConfig.depthClamp = qtrue;
		ri.Printf(PRINT_ALL, result[1], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_seamless_cube_map
	extension = "GL_ARB_seamless_cube_map";
	glRefConfig.seamlessCubeMap = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		if (r_arb_seamless_cube_map->integer)
			glRefConfig.seamlessCubeMap = qtrue;

		ri.Printf(PRINT_ALL, result[glRefConfig.seamlessCubeMap], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_vertex_type_2_10_10_10_rev
	extension = "GL_ARB_vertex_type_2_10_10_10_rev";
	glRefConfig.packedNormalDataType = GL_BYTE;
	if( GLimp_HaveExtension( extension ) )
	{
		if (r_arb_vertex_type_2_10_10_10_rev->integer)
			glRefConfig.packedNormalDataType = GL_INT_2_10_10_10_REV;

		ri.Printf(PRINT_ALL, result[r_arb_vertex_type_2_10_10_10_rev->integer ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// use float lightmaps?
	glRefConfig.floatLightmap = (glRefConfig.textureFloat && glRefConfig.halfFloatPixel && r_floatLightmap->integer && r_hdr->integer);

	// GL_ARB_vertex_array_object
	extension = "GL_ARB_vertex_array_object";
	glRefConfig.vertexArrayObject = qfalse;
	if( GLimp_HaveExtension( extension ) )
	{
		qglBindVertexArrayARB = (void *) SDL_GL_GetProcAddress("glBindVertexArray");
		qglDeleteVertexArraysARB = (void *) SDL_GL_GetProcAddress("glDeleteVertexArrays");
		qglGenVertexArraysARB = (void *) SDL_GL_GetProcAddress("glGenVertexArrays");
		qglIsVertexArrayARB = (void *) SDL_GL_GetProcAddress("glIsVertexArray");

		if (r_arb_vertex_array_object->integer)
			glRefConfig.vertexArrayObject = qtrue;

		ri.Printf(PRINT_ALL, result[glRefConfig.vertexArrayObject ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_ARB_half_float_vertex
	extension = "GL_ARB_half_float_vertex";
	glRefConfig.packedTexcoordDataType = GL_FLOAT;
	glRefConfig.packedTexcoordDataSize = sizeof(float) * 2;
	glRefConfig.packedColorDataType    = GL_FLOAT;
	glRefConfig.packedColorDataSize    = sizeof(float) * 4;
	if( GLimp_HaveExtension( extension ) )
	{
		if (r_arb_half_float_vertex->integer)
		{
			glRefConfig.packedTexcoordDataType = GL_HALF_FLOAT;
			glRefConfig.packedTexcoordDataSize = sizeof(uint16_t) * 2;
			glRefConfig.packedColorDataType    = GL_HALF_FLOAT;
			glRefConfig.packedColorDataSize    = sizeof(uint16_t) * 4;
		}

		ri.Printf(PRINT_ALL, result[r_arb_half_float_vertex->integer ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}

	// GL_EXT_direct_state_access
	extension = "GL_EXT_direct_state_access";

	qglBindMultiTexture = GLDSA_BindMultiTexture;
	qglTextureParameterf = GLDSA_TextureParameterf;
	qglTextureParameteri = GLDSA_TextureParameteri;
	qglTextureImage2D = GLDSA_TextureImage2D;
	qglTextureSubImage2D = GLDSA_TextureSubImage2D;
	qglCopyTextureImage2D = GLDSA_CopyTextureImage2D;
	qglCompressedTextureImage2D = GLDSA_CompressedTextureImage2D;
	qglCompressedTextureSubImage2D = GLDSA_CompressedTextureSubImage2D;
	qglGenerateTextureMipmap = GLDSA_GenerateTextureMipmap;

	qglProgramUniform1i = GLDSA_ProgramUniform1i;
	qglProgramUniform1f = GLDSA_ProgramUniform1f;
	qglProgramUniform2f = GLDSA_ProgramUniform2f;
	qglProgramUniform3f = GLDSA_ProgramUniform3f;
	qglProgramUniform4f = GLDSA_ProgramUniform4f;
	qglProgramUniform1fv = GLDSA_ProgramUniform1fv;
	qglProgramUniformMatrix4fv = GLDSA_ProgramUniformMatrix4fv;

	qglNamedRenderbufferStorage = GLDSA_NamedRenderbufferStorage;
	qglNamedRenderbufferStorageMultisample = GLDSA_NamedRenderbufferStorageMultisample;
	qglCheckNamedFramebufferStatus = GLDSA_CheckNamedFramebufferStatus;
	qglNamedFramebufferTexture2D = GLDSA_NamedFramebufferTexture2D;
	qglNamedFramebufferRenderbuffer = GLDSA_NamedFramebufferRenderbuffer;

	glRefConfig.directStateAccess = qfalse;
	if (GLimp_HaveExtension(extension))
	{
		if (r_ext_direct_state_access->integer)
		{
			glRefConfig.directStateAccess = qtrue;
			qglBindMultiTexture = (void *)SDL_GL_GetProcAddress("glBindMultiTextureEXT");
			qglTextureParameterf = (void *)SDL_GL_GetProcAddress("glTextureParameterfEXT");
			qglTextureParameteri = (void *)SDL_GL_GetProcAddress("glTextureParameteriEXT");
			qglTextureImage2D = (void *)SDL_GL_GetProcAddress("glTextureImage2DEXT");
			qglTextureSubImage2D = (void *)SDL_GL_GetProcAddress("glTextureSubImage2DEXT");
			qglCopyTextureImage2D = (void *)SDL_GL_GetProcAddress("glCopyTextureImage2DEXT");
			qglCompressedTextureImage2D = (void *)SDL_GL_GetProcAddress("glCompressedTextureImage2DEXT");
			qglCompressedTextureSubImage2D = (void *)SDL_GL_GetProcAddress("glCompressedTextureSubImage2DEXT");
			qglGenerateTextureMipmap = (void *)SDL_GL_GetProcAddress("glGenerateTextureMipmapEXT");

			qglProgramUniform1i = (void *)SDL_GL_GetProcAddress("glProgramUniform1iEXT");
			qglProgramUniform1f = (void *)SDL_GL_GetProcAddress("glProgramUniform1fEXT");
			qglProgramUniform2f = (void *)SDL_GL_GetProcAddress("glProgramUniform2fEXT");
			qglProgramUniform3f = (void *)SDL_GL_GetProcAddress("glProgramUniform3fEXT");
			qglProgramUniform4f = (void *)SDL_GL_GetProcAddress("glProgramUniform4fEXT");
			qglProgramUniform1fv = (void *)SDL_GL_GetProcAddress("glProgramUniform1fvEXT");
			qglProgramUniformMatrix4fv = (void *)SDL_GL_GetProcAddress("glProgramUniformMatrix4fvEXT");

			qglNamedRenderbufferStorage = (void *)SDL_GL_GetProcAddress("glNamedRenderbufferStorageEXT");
			qglNamedRenderbufferStorageMultisample = (void *)SDL_GL_GetProcAddress("glNamedRenderbufferStorageMultisampleEXT");
			qglCheckNamedFramebufferStatus = (void *)SDL_GL_GetProcAddress("glCheckNamedFramebufferStatusEXT");
			qglNamedFramebufferTexture2D = (void *)SDL_GL_GetProcAddress("glNamedFramebufferTexture2DEXT");
			qglNamedFramebufferRenderbuffer = (void *)SDL_GL_GetProcAddress("glNamedFramebufferRenderbufferEXT");
		}

		ri.Printf(PRINT_ALL, result[glRefConfig.directStateAccess ? 1 : 0], extension);
	}
	else
	{
		ri.Printf(PRINT_ALL, result[2], extension);
	}
}
