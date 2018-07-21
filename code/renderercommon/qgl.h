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
/*
** QGL.H
*/

#ifndef __QGL_H__
#define __QGL_H__

#ifdef USE_LOCAL_HEADERS
#	include "SDL_opengl.h"
#else
#	include <SDL_opengl.h>
#endif

extern void (APIENTRYP qglActiveTextureARB) (GLenum texture);
extern void (APIENTRYP qglClientActiveTextureARB) (GLenum texture);
extern void (APIENTRYP qglMultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);

extern void (APIENTRYP qglLockArraysEXT) (GLint first, GLsizei count);
extern void (APIENTRYP qglUnlockArraysEXT) (void);


//===========================================================================

// GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a
// get missing functions from code/SDL2/include/SDL_opengl.h

// OpenGL 1.0/1.1, OpenGL ES 1.0, and OpenGL 3.2 core profile
#define QGL_1_1_PROCS \
	GLE(void, BindTexture, GLenum target, GLuint texture) \
	GLE(void, BlendFunc, GLenum sfactor, GLenum dfactor) \
	GLE(void, ClearColor, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) \
	GLE(void, Clear, GLbitfield mask) \
	GLE(void, ClearStencil, GLint s) \
	GLE(void, ColorMask, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) \
	GLE(void, CopyTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) \
	GLE(void, CullFace, GLenum mode) \
	GLE(void, DeleteTextures, GLsizei n, const GLuint *textures) \
	GLE(void, DepthFunc, GLenum func) \
	GLE(void, DepthMask, GLboolean flag) \
	GLE(void, Disable, GLenum cap) \
	GLE(void, DrawArrays, GLenum mode, GLint first, GLsizei count) \
	GLE(void, DrawElements, GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) \
	GLE(void, Enable, GLenum cap) \
	GLE(void, Finish, void) \
	GLE(void, Flush, void) \
	GLE(void, GenTextures, GLsizei n, GLuint *textures ) \
	GLE(void, GetBooleanv, GLenum pname, GLboolean *params) \
	GLE(GLenum, GetError, void) \
	GLE(void, GetIntegerv, GLenum pname, GLint *params) \
	GLE(const GLubyte *, GetString, GLenum name) \
	GLE(void, LineWidth, GLfloat width) \
	GLE(void, PolygonOffset, GLfloat factor, GLfloat units) \
	GLE(void, ReadPixels, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) \
	GLE(void, Scissor, GLint x, GLint y, GLsizei width, GLsizei height) \
	GLE(void, StencilFunc, GLenum func, GLint ref, GLuint mask) \
	GLE(void, StencilMask, GLuint mask) \
	GLE(void, StencilOp, GLenum fail, GLenum zfail, GLenum zpass) \
	GLE(void, TexImage2D, GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) \
	GLE(void, TexParameterf, GLenum target, GLenum pname, GLfloat param) \
	GLE(void, TexParameteri, GLenum target, GLenum pname, GLint param) \
	GLE(void, TexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) \
	GLE(void, Translatef, GLfloat x, GLfloat y, GLfloat z) \
	GLE(void, Viewport, GLint x, GLint y, GLsizei width, GLsizei height) \

// OpenGL 1.0/1.1 and OpenGL ES 1.x but not OpenGL 3.2 core profile
#define QGL_1_1_FIXED_FUNCTION_PROCS \
	GLE(void, AlphaFunc, GLenum func, GLclampf ref) \
	GLE(void, Color4f, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) \
	GLE(void, ColorPointer, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) \
	GLE(void, DisableClientState, GLenum cap) \
	GLE(void, EnableClientState, GLenum cap) \
	GLE(void, LoadIdentity, void) \
	GLE(void, LoadMatrixf, const GLfloat *m) \
	GLE(void, MatrixMode, GLenum mode) \
	GLE(void, PopMatrix, void) \
	GLE(void, PushMatrix, void) \
	GLE(void, ShadeModel, GLenum mode) \
	GLE(void, TexCoordPointer, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) \
	GLE(void, TexEnvf, GLenum target, GLenum pname, GLfloat param) \
	GLE(void, VertexPointer, GLint size, GLenum type, GLsizei stride, const GLvoid *ptr) \

// OpenGL 1.0/1.1 and 3.2 core profile but not OpenGL ES 1.x
#define QGL_DESKTOP_1_1_PROCS \
	GLE(void, ClearDepth, GLclampd depth) \
	GLE(void, DepthRange, GLclampd near_val, GLclampd far_val) \
	GLE(void, DrawBuffer, GLenum mode) \
	GLE(void, PolygonMode, GLenum face, GLenum mode) \

// OpenGL 1.0/1.1 but not OpenGL 3.2 core profile or OpenGL ES 1.x
#define QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS \
	GLE(void, ArrayElement, GLint i) \
	GLE(void, Begin, GLenum mode) \
	GLE(void, ClipPlane, GLenum plane, const GLdouble *equation) \
	GLE(void, Color3f, GLfloat red, GLfloat green, GLfloat blue) \
	GLE(void, Color4ubv, const GLubyte *v) \
	GLE(void, End, void) \
	GLE(void, Frustum, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val) \
	GLE(void, Ortho, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val) \
	GLE(void, TexCoord2f, GLfloat s, GLfloat t) \
	GLE(void, TexCoord2fv, const GLfloat *v) \
	GLE(void, Vertex2f, GLfloat x, GLfloat y) \
	GLE(void, Vertex3f, GLfloat x, GLfloat y, GLfloat z) \
	GLE(void, Vertex3fv, const GLfloat *v) \

// OpenGL ES 1.1 but not desktop OpenGL 1.x
#define QGL_ES_1_1_PROCS \
	GLE(void, ClearDepthf, GLclampf depth) \
	GLE(void, ClipPlanef, GLenum plane, const GLfloat *equation) \
	GLE(void, DepthRangef, GLclampf near_val, GLclampf far_val) \
	GLE(void, Frustumf, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val) \
	GLE(void, Orthof, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val) \

// OpenGL 1.3, was GL_ARB_texture_compression
#define QGL_1_3_PROCS \
	GLE(void, ActiveTexture, GLenum texture) \
	GLE(void, CompressedTexImage2D, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) \
	GLE(void, CompressedTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) \

// OpenGL 1.5, was GL_ARB_vertex_buffer_object and GL_ARB_occlusion_query
#define QGL_1_5_PROCS \
	GLE(void, GenQueries, GLsizei n, GLuint *ids) \
	GLE(void, DeleteQueries, GLsizei n, const GLuint *ids) \
	GLE(void, BeginQuery, GLenum target, GLuint id) \
	GLE(void, EndQuery, GLenum target) \
	GLE(void, GetQueryObjectiv, GLuint id, GLenum pname, GLint *params) \
	GLE(void, GetQueryObjectuiv, GLuint id, GLenum pname, GLuint *params) \
	GLE(void, BindBuffer, GLenum target, GLuint buffer) \
	GLE(void, DeleteBuffers, GLsizei n, const GLuint *buffers) \
	GLE(void, GenBuffers, GLsizei n, GLuint *buffers) \
	GLE(void, BufferData, GLenum target, GLsizeiptr size, const void *data, GLenum usage) \
	GLE(void, BufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const void *data) \

// OpenGL 2.0, was GL_ARB_shading_language_100, GL_ARB_vertex_program, GL_ARB_shader_objects, and GL_ARB_vertex_shader
#define QGL_2_0_PROCS \
	GLE(void, AttachShader, GLuint program, GLuint shader) \
	GLE(void, BindAttribLocation, GLuint program, GLuint index, const GLchar *name) \
	GLE(void, CompileShader, GLuint shader) \
	GLE(GLuint, CreateProgram, void) \
	GLE(GLuint, CreateShader, GLenum type) \
	GLE(void, DeleteProgram, GLuint program) \
	GLE(void, DeleteShader, GLuint shader) \
	GLE(void, DetachShader, GLuint program, GLuint shader) \
	GLE(void, DisableVertexAttribArray, GLuint index) \
	GLE(void, EnableVertexAttribArray, GLuint index) \
	GLE(void, GetActiveUniform, GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) \
	GLE(void, GetProgramiv, GLuint program, GLenum pname, GLint *params) \
	GLE(void, GetProgramInfoLog, GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) \
	GLE(void, GetShaderiv, GLuint shader, GLenum pname, GLint *params) \
	GLE(void, GetShaderInfoLog, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) \
	GLE(void, GetShaderSource, GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) \
	GLE(GLint, GetUniformLocation, GLuint program, const GLchar *name) \
	GLE(void, LinkProgram, GLuint program) \
	GLE(void, ShaderSource, GLuint shader, GLsizei count, const GLchar* *string, const GLint *length) \
	GLE(void, UseProgram, GLuint program) \
	GLE(void, Uniform1f, GLint location, GLfloat v0) \
	GLE(void, Uniform2f, GLint location, GLfloat v0, GLfloat v1) \
	GLE(void, Uniform3f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) \
	GLE(void, Uniform4f, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
	GLE(void, Uniform1i, GLint location, GLint v0) \
	GLE(void, Uniform1fv, GLint location, GLsizei count, const GLfloat *value) \
	GLE(void, UniformMatrix4fv, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	GLE(void, ValidateProgram, GLuint program) \
	GLE(void, VertexAttribPointer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) \

// GL_NVX_gpu_memory_info
#ifndef GL_NVX_gpu_memory_info
#define GL_NVX_gpu_memory_info
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B
#endif

// GL_ATI_meminfo
#ifndef GL_ATI_meminfo
#define GL_ATI_meminfo
#define GL_VBO_FREE_MEMORY_ATI                    0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI           0x87FD
#endif

// GL_ARB_texture_float
#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float
#define GL_TEXTURE_RED_TYPE_ARB             0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB           0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB            0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB           0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB       0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB       0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB           0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB          0x8C17
#define GL_RGBA32F_ARB                      0x8814
#define GL_RGB32F_ARB                       0x8815
#define GL_ALPHA32F_ARB                     0x8816
#define GL_INTENSITY32F_ARB                 0x8817
#define GL_LUMINANCE32F_ARB                 0x8818
#define GL_LUMINANCE_ALPHA32F_ARB           0x8819
#define GL_RGBA16F_ARB                      0x881A
#define GL_RGB16F_ARB                       0x881B
#define GL_ALPHA16F_ARB                     0x881C
#define GL_INTENSITY16F_ARB                 0x881D
#define GL_LUMINANCE16F_ARB                 0x881E
#define GL_LUMINANCE_ALPHA16F_ARB           0x881F
#endif

#ifndef GL_ARB_half_float_pixel
#define GL_ARB_half_float_pixel
#define GL_HALF_FLOAT_ARB                   0x140B
#endif

// OpenGL 3.0 specific
#define QGL_3_0_PROCS \
	GLE(const GLubyte *, GetStringi, GLenum name, GLuint index) \

// GL_ARB_framebuffer_object, built-in to OpenGL 3.0
#define QGL_ARB_framebuffer_object_PROCS \
	GLE(void, BindRenderbuffer, GLenum target, GLuint renderbuffer) \
	GLE(void, DeleteRenderbuffers, GLsizei n, const GLuint *renderbuffers) \
	GLE(void, GenRenderbuffers, GLsizei n, GLuint *renderbuffers) \
	GLE(void, RenderbufferStorage, GLenum target, GLenum internalformat, GLsizei width, GLsizei height) \
	GLE(void, BindFramebuffer, GLenum target, GLuint framebuffer) \
	GLE(void, DeleteFramebuffers, GLsizei n, const GLuint *framebuffers) \
	GLE(void, GenFramebuffers, GLsizei n, GLuint *framebuffers) \
	GLE(GLenum, CheckFramebufferStatus, GLenum target) \
	GLE(void, FramebufferTexture2D, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) \
	GLE(void, FramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) \
	GLE(void, GenerateMipmap, GLenum target) \
	GLE(void, BlitFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) \
	GLE(void, RenderbufferStorageMultisample, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) \

// GL_ARB_vertex_array_object, built-in to OpenGL 3.0
#define QGL_ARB_vertex_array_object_PROCS \
	GLE(void, BindVertexArray, GLuint array) \
	GLE(void, DeleteVertexArrays, GLsizei n, const GLuint *arrays) \
	GLE(void, GenVertexArrays, GLsizei n, GLuint *arrays) \

#ifndef GL_ARB_texture_compression_rgtc
#define GL_ARB_texture_compression_rgtc
#define GL_COMPRESSED_RED_RGTC1                       0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1                0x8DBC
#define GL_COMPRESSED_RG_RGTC2                        0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2                 0x8DBE
#endif

#ifndef GL_ARB_texture_compression_bptc
#define GL_ARB_texture_compression_bptc
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB                 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB           0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB           0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB         0x8E8F
#endif

#ifndef GL_ARB_depth_clamp
#define GL_ARB_depth_clamp
#define GL_DEPTH_CLAMP				      0x864F
#endif

#ifndef GL_ARB_seamless_cube_map
#define GL_ARB_seamless_cube_map
#define GL_TEXTURE_CUBE_MAP_SEAMLESS               0x884F
#endif

// GL_EXT_direct_state_access
#define QGL_EXT_direct_state_access_PROCS \
	GLE(GLvoid, BindMultiTextureEXT, GLenum texunit, GLenum target, GLuint texture) \
	GLE(GLvoid, TextureParameterfEXT, GLuint texture, GLenum target, GLenum pname, GLfloat param) \
	GLE(GLvoid, TextureParameteriEXT, GLuint texture, GLenum target, GLenum pname, GLint param) \
	GLE(GLvoid, TextureImage2DEXT, GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) \
	GLE(GLvoid, TextureSubImage2DEXT, GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) \
	GLE(GLvoid, CopyTextureSubImage2DEXT, GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) \
	GLE(GLvoid, CompressedTextureImage2DEXT, GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data) \
	GLE(GLvoid, CompressedTextureSubImage2DEXT, GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data) \
	GLE(GLvoid, GenerateTextureMipmapEXT, GLuint texture, GLenum target) \
	GLE(GLvoid, ProgramUniform1iEXT, GLuint program, GLint location, GLint v0) \
	GLE(GLvoid, ProgramUniform1fEXT, GLuint program, GLint location, GLfloat v0) \
	GLE(GLvoid, ProgramUniform2fEXT, GLuint program, GLint location, GLfloat v0, GLfloat v1) \
	GLE(GLvoid, ProgramUniform3fEXT, GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) \
	GLE(GLvoid, ProgramUniform4fEXT, GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) \
	GLE(GLvoid, ProgramUniform1fvEXT, GLuint program, GLint location, GLsizei count, const GLfloat *value) \
	GLE(GLvoid, ProgramUniformMatrix4fvEXT, GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) \
	GLE(GLvoid, NamedRenderbufferStorageEXT, GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height) \
	GLE(GLvoid, NamedRenderbufferStorageMultisampleEXT, GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) \
	GLE(GLenum, CheckNamedFramebufferStatusEXT, GLuint framebuffer, GLenum target) \
	GLE(GLvoid, NamedFramebufferTexture2DEXT, GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level) \
	GLE(GLvoid, NamedFramebufferRenderbufferEXT, GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) \

#define GLE(ret, name, ...) typedef ret APIENTRY name##proc(__VA_ARGS__);
QGL_1_1_PROCS;
QGL_1_1_FIXED_FUNCTION_PROCS;
QGL_DESKTOP_1_1_PROCS;
QGL_DESKTOP_1_1_FIXED_FUNCTION_PROCS;
QGL_ES_1_1_PROCS;
QGL_1_3_PROCS;
QGL_1_5_PROCS;
QGL_2_0_PROCS;
QGL_3_0_PROCS;
QGL_ARB_framebuffer_object_PROCS;
QGL_ARB_vertex_array_object_PROCS;
QGL_EXT_direct_state_access_PROCS;
#undef GLE

extern int qglMajorVersion, qglMinorVersion;
extern int qglesMajorVersion, qglesMinorVersion;
#define QGL_VERSION_ATLEAST( major, minor ) ( qglMajorVersion > major || ( qglMajorVersion == major && qglMinorVersion >= minor ) )
#define QGLES_VERSION_ATLEAST( major, minor ) ( qglesMajorVersion > major || ( qglesMajorVersion == major && qglesMinorVersion >= minor ) )

#endif
