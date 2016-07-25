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

#define qglAccum glAccum
#define qglAlphaFunc glAlphaFunc
#define qglAreTexturesResident glAreTexturesResident
#define qglArrayElement glArrayElement
#define qglBegin glBegin
#define qglBindTexture glBindTexture
#define qglBitmap glBitmap
#define qglBlendFunc glBlendFunc
#define qglCallList glCallList
#define qglCallLists glCallLists
#define qglClear glClear
#define qglClearAccum glClearAccum
#define qglClearColor glClearColor
#define qglClearDepth glClearDepth
#define qglClearIndex glClearIndex
#define qglClearStencil glClearStencil
#define qglClipPlane glClipPlane
#define qglColor3b glColor3b
#define qglColor3bv glColor3bv
#define qglColor3d glColor3d
#define qglColor3dv glColor3dv
#define qglColor3f glColor3f
#define qglColor3fv glColor3fv
#define qglColor3i glColor3i
#define qglColor3iv glColor3iv
#define qglColor3s glColor3s
#define qglColor3sv glColor3sv
#define qglColor3ub glColor3ub
#define qglColor3ubv glColor3ubv
#define qglColor3ui glColor3ui
#define qglColor3uiv glColor3uiv
#define qglColor3us glColor3us
#define qglColor3usv glColor3usv
#define qglColor4b glColor4b
#define qglColor4bv glColor4bv
#define qglColor4d glColor4d
#define qglColor4dv glColor4dv
#define qglColor4f glColor4f
#define qglColor4fv glColor4fv
#define qglColor4i glColor4i
#define qglColor4iv glColor4iv
#define qglColor4s glColor4s
#define qglColor4sv glColor4sv
#define qglColor4ub glColor4ub
#define qglColor4ubv glColor4ubv
#define qglColor4ui glColor4ui
#define qglColor4uiv glColor4uiv
#define qglColor4us glColor4us
#define qglColor4usv glColor4usv
#define qglColorMask glColorMask
#define qglColorMaterial glColorMaterial
#define qglColorPointer glColorPointer
#define qglCopyPixels glCopyPixels
#define qglCopyTexImage1D glCopyTexImage1D
#define qglCopyTexImage2D glCopyTexImage2D
#define qglCopyTexSubImage1D glCopyTexSubImage1D
#define qglCopyTexSubImage2D glCopyTexSubImage2D
#define qglCullFace glCullFace
#define qglDeleteLists glDeleteLists
#define qglDeleteTextures glDeleteTextures
#define qglDepthFunc glDepthFunc
#define qglDepthMask glDepthMask
#define qglDepthRange glDepthRange
#define qglDisable glDisable
#define qglDisableClientState glDisableClientState
#define qglDrawArrays glDrawArrays
#define qglDrawBuffer glDrawBuffer
#define qglDrawElements glDrawElements
#define qglDrawPixels glDrawPixels
#define qglEdgeFlag glEdgeFlag
#define qglEdgeFlagPointer glEdgeFlagPointer
#define qglEdgeFlagv glEdgeFlagv
#define qglEnable glEnable
#define qglEnableClientState glEnableClientState
#define qglEnd glEnd
#define qglEndList glEndList
#define qglEvalCoord1d glEvalCoord1d
#define qglEvalCoord1dv glEvalCoord1dv
#define qglEvalCoord1f glEvalCoord1f
#define qglEvalCoord1fv glEvalCoord1fv
#define qglEvalCoord2d glEvalCoord2d
#define qglEvalCoord2dv glEvalCoord2dv
#define qglEvalCoord2f glEvalCoord2f
#define qglEvalCoord2fv glEvalCoord2fv
#define qglEvalMesh1 glEvalMesh1
#define qglEvalMesh2 glEvalMesh2
#define qglEvalPoint1 glEvalPoint1
#define qglEvalPoint2 glEvalPoint2
#define qglFeedbackBuffer glFeedbackBuffer
#define qglFinish glFinish
#define qglFlush glFlush
#define qglFogf glFogf
#define qglFogfv glFogfv
#define qglFogi glFogi
#define qglFogiv glFogiv
#define qglFrontFace glFrontFace
#define qglFrustum glFrustum
#define qglGenLists glGenLists
#define qglGenTextures glGenTextures
#define qglGetBooleanv glGetBooleanv
#define qglGetClipPlane glGetClipPlane
#define qglGetDoublev glGetDoublev
#define qglGetError glGetError
#define qglGetFloatv glGetFloatv
#define qglGetIntegerv glGetIntegerv
#define qglGetLightfv glGetLightfv
#define qglGetLightiv glGetLightiv
#define qglGetMapdv glGetMapdv
#define qglGetMapfv glGetMapfv
#define qglGetMapiv glGetMapiv
#define qglGetMaterialfv glGetMaterialfv
#define qglGetMaterialiv glGetMaterialiv
#define qglGetPixelMapfv glGetPixelMapfv
#define qglGetPixelMapuiv glGetPixelMapuiv
#define qglGetPixelMapusv glGetPixelMapusv
#define qglGetPointerv glGetPointerv
#define qglGetPolygonStipple glGetPolygonStipple
#define qglGetString glGetString
#define qglGetTexGendv glGetTexGendv
#define qglGetTexGenfv glGetTexGenfv
#define qglGetTexGeniv glGetTexGeniv
#define qglGetTexImage glGetTexImage
#define qglGetTexLevelParameterfv glGetTexLevelParameterfv
#define qglGetTexLevelParameteriv glGetTexLevelParameteriv
#define qglGetTexParameterfv glGetTexParameterfv
#define qglGetTexParameteriv glGetTexParameteriv
#define qglHint glHint
#define qglIndexMask glIndexMask
#define qglIndexPointer glIndexPointer
#define qglIndexd glIndexd
#define qglIndexdv glIndexdv
#define qglIndexf glIndexf
#define qglIndexfv glIndexfv
#define qglIndexi glIndexi
#define qglIndexiv glIndexiv
#define qglIndexs glIndexs
#define qglIndexsv glIndexsv
#define qglIndexub glIndexub
#define qglIndexubv glIndexubv
#define qglInitNames glInitNames
#define qglInterleavedArrays glInterleavedArrays
#define qglIsEnabled glIsEnabled
#define qglIsList glIsList
#define qglIsTexture glIsTexture
#define qglLightModelf glLightModelf
#define qglLightModelfv glLightModelfv
#define qglLightModeli glLightModeli
#define qglLightModeliv glLightModeliv
#define qglLightf glLightf
#define qglLightfv glLightfv
#define qglLighti glLighti
#define qglLightiv glLightiv
#define qglLineStipple glLineStipple
#define qglLineWidth glLineWidth
#define qglListBase glListBase
#define qglLoadIdentity glLoadIdentity
#define qglLoadMatrixd glLoadMatrixd
#define qglLoadMatrixf glLoadMatrixf
#define qglLoadName glLoadName
#define qglLogicOp glLogicOp
#define qglMap1d glMap1d
#define qglMap1f glMap1f
#define qglMap2d glMap2d
#define qglMap2f glMap2f
#define qglMapGrid1d glMapGrid1d
#define qglMapGrid1f glMapGrid1f
#define qglMapGrid2d glMapGrid2d
#define qglMapGrid2f glMapGrid2f
#define qglMaterialf glMaterialf
#define qglMaterialfv glMaterialfv
#define qglMateriali glMateriali
#define qglMaterialiv glMaterialiv
#define qglMatrixMode glMatrixMode
#define qglMultMatrixd glMultMatrixd
#define qglMultMatrixf glMultMatrixf
#define qglNewList glNewList
#define qglNormal3b glNormal3b
#define qglNormal3bv glNormal3bv
#define qglNormal3d glNormal3d
#define qglNormal3dv glNormal3dv
#define qglNormal3f glNormal3f
#define qglNormal3fv glNormal3fv
#define qglNormal3i glNormal3i
#define qglNormal3iv glNormal3iv
#define qglNormal3s glNormal3s
#define qglNormal3sv glNormal3sv
#define qglNormalPointer glNormalPointer
#define qglOrtho glOrtho
#define qglPassThrough glPassThrough
#define qglPixelMapfv glPixelMapfv
#define qglPixelMapuiv glPixelMapuiv
#define qglPixelMapusv glPixelMapusv
#define qglPixelStoref glPixelStoref
#define qglPixelStorei glPixelStorei
#define qglPixelTransferf glPixelTransferf
#define qglPixelTransferi glPixelTransferi
#define qglPixelZoom glPixelZoom
#define qglPointSize glPointSize
#define qglPolygonMode glPolygonMode
#define qglPolygonOffset glPolygonOffset
#define qglPolygonStipple glPolygonStipple
#define qglPopAttrib glPopAttrib
#define qglPopClientAttrib glPopClientAttrib
#define qglPopMatrix glPopMatrix
#define qglPopName glPopName
#define qglPrioritizeTextures glPrioritizeTextures
#define qglPushAttrib glPushAttrib
#define qglPushClientAttrib glPushClientAttrib
#define qglPushMatrix glPushMatrix
#define qglPushName glPushName
#define qglRasterPos2d glRasterPos2d
#define qglRasterPos2dv glRasterPos2dv
#define qglRasterPos2f glRasterPos2f
#define qglRasterPos2fv glRasterPos2fv
#define qglRasterPos2i glRasterPos2i
#define qglRasterPos2iv glRasterPos2iv
#define qglRasterPos2s glRasterPos2s
#define qglRasterPos2sv glRasterPos2sv
#define qglRasterPos3d glRasterPos3d
#define qglRasterPos3dv glRasterPos3dv
#define qglRasterPos3f glRasterPos3f
#define qglRasterPos3fv glRasterPos3fv
#define qglRasterPos3i glRasterPos3i
#define qglRasterPos3iv glRasterPos3iv
#define qglRasterPos3s glRasterPos3s
#define qglRasterPos3sv glRasterPos3sv
#define qglRasterPos4d glRasterPos4d
#define qglRasterPos4dv glRasterPos4dv
#define qglRasterPos4f glRasterPos4f
#define qglRasterPos4fv glRasterPos4fv
#define qglRasterPos4i glRasterPos4i
#define qglRasterPos4iv glRasterPos4iv
#define qglRasterPos4s glRasterPos4s
#define qglRasterPos4sv glRasterPos4sv
#define qglReadBuffer glReadBuffer
#define qglReadPixels glReadPixels
#define qglRectd glRectd
#define qglRectdv glRectdv
#define qglRectf glRectf
#define qglRectfv glRectfv
#define qglRecti glRecti
#define qglRectiv glRectiv
#define qglRects glRects
#define qglRectsv glRectsv
#define qglRenderMode glRenderMode
#define qglRotated glRotated
#define qglRotatef glRotatef
#define qglScaled glScaled
#define qglScalef glScalef
#define qglScissor glScissor
#define qglSelectBuffer glSelectBuffer
#define qglShadeModel glShadeModel
#define qglStencilFunc glStencilFunc
#define qglStencilMask glStencilMask
#define qglStencilOp glStencilOp
#define qglTexCoord1d glTexCoord1d
#define qglTexCoord1dv glTexCoord1dv
#define qglTexCoord1f glTexCoord1f
#define qglTexCoord1fv glTexCoord1fv
#define qglTexCoord1i glTexCoord1i
#define qglTexCoord1iv glTexCoord1iv
#define qglTexCoord1s glTexCoord1s
#define qglTexCoord1sv glTexCoord1sv
#define qglTexCoord2d glTexCoord2d
#define qglTexCoord2dv glTexCoord2dv
#define qglTexCoord2f glTexCoord2f
#define qglTexCoord2fv glTexCoord2fv
#define qglTexCoord2i glTexCoord2i
#define qglTexCoord2iv glTexCoord2iv
#define qglTexCoord2s glTexCoord2s
#define qglTexCoord2sv glTexCoord2sv
#define qglTexCoord3d glTexCoord3d
#define qglTexCoord3dv glTexCoord3dv
#define qglTexCoord3f glTexCoord3f
#define qglTexCoord3fv glTexCoord3fv
#define qglTexCoord3i glTexCoord3i
#define qglTexCoord3iv glTexCoord3iv
#define qglTexCoord3s glTexCoord3s
#define qglTexCoord3sv glTexCoord3sv
#define qglTexCoord4d glTexCoord4d
#define qglTexCoord4dv glTexCoord4dv
#define qglTexCoord4f glTexCoord4f
#define qglTexCoord4fv glTexCoord4fv
#define qglTexCoord4i glTexCoord4i
#define qglTexCoord4iv glTexCoord4iv
#define qglTexCoord4s glTexCoord4s
#define qglTexCoord4sv glTexCoord4sv
#define qglTexCoordPointer glTexCoordPointer
#define qglTexEnvf glTexEnvf
#define qglTexEnvfv glTexEnvfv
#define qglTexEnvi glTexEnvi
#define qglTexEnviv glTexEnviv
#define qglTexGend glTexGend
#define qglTexGendv glTexGendv
#define qglTexGenf glTexGenf
#define qglTexGenfv glTexGenfv
#define qglTexGeni glTexGeni
#define qglTexGeniv glTexGeniv
#define qglTexImage1D glTexImage1D
#define qglTexImage2D glTexImage2D
#define qglTexParameterf glTexParameterf
#define qglTexParameterfv glTexParameterfv
#define qglTexParameteri glTexParameteri
#define qglTexParameteriv glTexParameteriv
#define qglTexSubImage1D glTexSubImage1D
#define qglTexSubImage2D glTexSubImage2D
#define qglTranslated glTranslated
#define qglTranslatef glTranslatef
#define qglVertex2d glVertex2d
#define qglVertex2dv glVertex2dv
#define qglVertex2f glVertex2f
#define qglVertex2fv glVertex2fv
#define qglVertex2i glVertex2i
#define qglVertex2iv glVertex2iv
#define qglVertex2s glVertex2s
#define qglVertex2sv glVertex2sv
#define qglVertex3d glVertex3d
#define qglVertex3dv glVertex3dv
#define qglVertex3f glVertex3f
#define qglVertex3fv glVertex3fv
#define qglVertex3i glVertex3i
#define qglVertex3iv glVertex3iv
#define qglVertex3s glVertex3s
#define qglVertex3sv glVertex3sv
#define qglVertex4d glVertex4d
#define qglVertex4dv glVertex4dv
#define qglVertex4f glVertex4f
#define qglVertex4fv glVertex4fv
#define qglVertex4i glVertex4i
#define qglVertex4iv glVertex4iv
#define qglVertex4s glVertex4s
#define qglVertex4sv glVertex4sv
#define qglVertexPointer glVertexPointer
#define qglViewport glViewport

// GL function loader, based on https://gist.github.com/rygorous/16796a0c876cf8a5f542caddb55bce8a

// OpenGL 1.2, was GL_EXT_draw_range_elements
#define QGL_1_2_PROCS \
	GLE(void, DrawRangeElements, GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) \

// OpenGL 1.3, was GL_ARB_texture_compression
#define QGL_1_3_PROCS \
	GLE(void, CompressedTexImage2D, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) \
	GLE(void, CompressedTexSubImage2D, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) \

// OpenGL 1.4, was GL_EXT_multi_draw_arrays
#define QGL_1_4_PROCS \
	GLE(void, MultiDrawElements, GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount) \

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

#define GLE(ret, name, ...) typedef ret APIENTRY name##proc(__VA_ARGS__); extern name##proc * qgl##name;
QGL_1_2_PROCS
QGL_1_3_PROCS
QGL_1_4_PROCS
QGL_1_5_PROCS
QGL_2_0_PROCS
#undef GLE

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

// GL_EXT_framebuffer_object
extern GLboolean (APIENTRY * qglIsRenderbufferEXT)(GLuint renderbuffer);
extern void (APIENTRY * qglBindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
extern void (APIENTRY * qglDeleteRenderbuffersEXT)(GLsizei n, const GLuint *renderbuffers);
extern void (APIENTRY * qglGenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers);
extern void (APIENTRY * qglRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRY * qglGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern GLboolean (APIENTRY * qglIsFramebufferEXT)(GLuint framebuffer);
extern void (APIENTRY * qglBindFramebufferEXT)(GLenum target, GLuint framebuffer);
extern void (APIENTRY * qglDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);
extern void (APIENTRY * qglGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);
extern GLenum (APIENTRY * qglCheckFramebufferStatusEXT)(GLenum target);
extern void (APIENTRY * qglFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level);
extern void (APIENTRY * qglFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level);
extern void (APIENTRY * qglFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
	GLint level, GLint zoffset);
extern void (APIENTRY * qglFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget,
	GLuint renderbuffer);
extern void (APIENTRY * qglGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRY * qglGenerateMipmapEXT)(GLenum target);

#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object
#define GL_FRAMEBUFFER_EXT                     0x8D40
#define GL_RENDERBUFFER_EXT                    0x8D41
#define GL_STENCIL_INDEX1_EXT                  0x8D46
#define GL_STENCIL_INDEX4_EXT                  0x8D47
#define GL_STENCIL_INDEX8_EXT                  0x8D48
#define GL_STENCIL_INDEX16_EXT                 0x8D49
#define GL_RENDERBUFFER_WIDTH_EXT              0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT             0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT    0x8D44
#define GL_RENDERBUFFER_RED_SIZE_EXT           0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT         0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT          0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT         0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT         0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT       0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT            0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT            0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT          0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT  0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT     0x8CD4
#define GL_COLOR_ATTACHMENT0_EXT                0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT                0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT                0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT                0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT                0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT                0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT                0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT                0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT                0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT                0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT               0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT               0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT               0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT               0x8CED
#define GL_COLOR_ATTACHMENT14_EXT               0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT               0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT                 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT               0x8D20
#define GL_FRAMEBUFFER_COMPLETE_EXT                          0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT             0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT     0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT             0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT                0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT            0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT            0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT                       0x8CDD
#define GL_FRAMEBUFFER_BINDING_EXT             0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT            0x8CA7
#define GL_MAX_COLOR_ATTACHMENTS_EXT           0x8CDF
#define GL_MAX_RENDERBUFFER_SIZE_EXT           0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT   0x0506
#endif

// GL_EXT_packed_depth_stencil
#ifndef GL_EXT_packed_depth_stencil
#define GL_EXT_packed_depth_stencil
#define GL_DEPTH_STENCIL_EXT                              0x84F9
#define GL_UNSIGNED_INT_24_8_EXT                          0x84FA
#define GL_DEPTH24_STENCIL8_EXT                           0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT                       0x88F1
#endif

// GL_EXT_framebuffer_blit
extern void (APIENTRY * qglBlitFramebufferEXT)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                            GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                            GLbitfield mask, GLenum filter);

#ifndef GL_EXT_framebuffer_blit
#define GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT                0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT                0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT        0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_EXT        0x8CAA
#endif

// GL_EXT_framebuffer_multisample
extern void (APIENTRY * qglRenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples,
	GLenum internalformat, GLsizei width, GLsizei height);

#ifndef GL_EXT_framebuffer_multisample
#define GL_EXT_framebuffer_multisample
#define GL_RENDERBUFFER_SAMPLES_EXT                0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT  0x8D56
#define GL_MAX_SAMPLES_EXT                         0x8D57
#endif

#ifndef GL_EXT_texture_sRGB
#define GL_EXT_texture_sRGB
#define GL_SRGB_EXT                                       0x8C40
#define GL_SRGB8_EXT                                      0x8C41
#define GL_SRGB_ALPHA_EXT                                 0x8C42
#define GL_SRGB8_ALPHA8_EXT                               0x8C43
#define GL_SLUMINANCE_ALPHA_EXT                           0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT                         0x8C45
#define GL_SLUMINANCE_EXT                                 0x8C46
#define GL_SLUMINANCE8_EXT                                0x8C47
#define GL_COMPRESSED_SRGB_EXT                            0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT                      0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT                      0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT                0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT                  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT            0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT            0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT            0x8C4F
#endif

#ifndef GL_EXT_framebuffer_sRGB
#define GL_EXT_framebuffer_sRGB
#define GL_FRAMEBUFFER_SRGB_EXT                         0x8DB9
#endif

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

// GL_ARB_vertex_array_object
extern void (APIENTRY * qglBindVertexArrayARB)(GLuint array);
extern void (APIENTRY * qglDeleteVertexArraysARB)(GLsizei n, const GLuint *arrays);
extern void (APIENTRY * qglGenVertexArraysARB)(GLsizei n, GLuint *arrays);
extern GLboolean (APIENTRY * qglIsVertexArrayARB)(GLuint array);
#ifndef GL_ARB_vertex_array_object
#define GL_ARB_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING_ARB                0x85B5
#endif

// GL_EXT_direct_state_access
extern GLvoid(APIENTRY * qglBindMultiTexture)(GLenum texunit, GLenum target, GLuint texture);
extern GLvoid(APIENTRY * qglTextureParameterf)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
extern GLvoid(APIENTRY * qglTextureParameteri)(GLuint texture, GLenum target, GLenum pname, GLint param);
extern GLvoid(APIENTRY * qglTextureImage2D)(GLuint texture, GLenum target, GLint level, GLint internalformat,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid(APIENTRY * qglTextureSubImage2D)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern GLvoid(APIENTRY * qglCopyTextureImage2D)(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern GLvoid(APIENTRY * qglCompressedTextureImage2D)(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
extern GLvoid(APIENTRY * qglCompressedTextureSubImage2D)(GLuint texture, GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
	GLsizei imageSize, const GLvoid *data);
extern GLvoid(APIENTRY * qglGenerateTextureMipmap)(GLuint texture, GLenum target);

extern GLvoid(APIENTRY * qglProgramUniform1i)(GLuint program, GLint location, GLint v0);
extern GLvoid(APIENTRY * qglProgramUniform1f)(GLuint program, GLint location, GLfloat v0);
extern GLvoid(APIENTRY * qglProgramUniform2f)(GLuint program, GLint location,
	GLfloat v0, GLfloat v1);
extern GLvoid(APIENTRY * qglProgramUniform3f)(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2);
extern GLvoid(APIENTRY * qglProgramUniform4f)(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern GLvoid(APIENTRY * qglProgramUniform1fv)(GLuint program, GLint location,
	GLsizei count, const GLfloat *value);
extern GLvoid(APIENTRY * qglProgramUniformMatrix4fv)(GLuint program, GLint location,
	GLsizei count, GLboolean transpose,
	const GLfloat *value);

extern GLvoid(APIENTRY * qglNamedRenderbufferStorage)(GLuint renderbuffer,
	GLenum internalformat, GLsizei width, GLsizei height);

extern GLvoid(APIENTRY * qglNamedRenderbufferStorageMultisample)(GLuint renderbuffer,
	GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

extern GLenum(APIENTRY * qglCheckNamedFramebufferStatus)(GLuint framebuffer, GLenum target);
extern GLvoid(APIENTRY * qglNamedFramebufferTexture2D)(GLuint framebuffer,
	GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern GLvoid(APIENTRY * qglNamedFramebufferRenderbuffer)(GLuint framebuffer,
	GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);




#if defined(WIN32)
// WGL_ARB_create_context
#ifndef WGL_ARB_create_context
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB               0x2093
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x0002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_VERSION_ARB                 0x2095
#define ERROR_INVALID_PROFILE_ARB                 0x2096
#endif

extern          HGLRC(APIENTRY * qwglCreateContextAttribsARB) (HDC hdC, HGLRC hShareContext, const int *attribList);
#endif

#if 0 //defined(__linux__)
// GLX_ARB_create_context
#ifndef GLX_ARB_create_context
#define GLX_CONTEXT_DEBUG_BIT_ARB          0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB      0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB      0x2092
#define GLX_CONTEXT_FLAGS_ARB              0x2094
#endif

extern GLXContext	(APIENTRY * qglXCreateContextAttribsARB) (Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
#endif

#endif
