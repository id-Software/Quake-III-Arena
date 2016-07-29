/*
===========================================================================
Copyright (C) 2016 James Canete

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
===========================================================================
*/

#ifndef __TR_DSA_H__
#define __TR_DSA_H__

#include "../renderercommon/qgl.h"

void GL_BindNullTextures(void);
int GL_BindMultiTexture(GLenum texunit, GLenum target, GLuint texture);

GLvoid APIENTRY GLDSA_BindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture);
GLvoid APIENTRY GLDSA_TextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param);
GLvoid APIENTRY GLDSA_TextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param);
GLvoid APIENTRY GLDSA_TextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid APIENTRY GLDSA_TextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid APIENTRY GLDSA_CopyTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
GLvoid APIENTRY GLDSA_CompressedTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
GLvoid APIENTRY GLDSA_CompressedTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
	GLsizei imageSize, const GLvoid *data);

GLvoid APIENTRY GLDSA_GenerateTextureMipmapEXT(GLuint texture, GLenum target);

void GL_BindNullProgram(void);
int GL_UseProgram(GLuint program);

GLvoid APIENTRY GLDSA_ProgramUniform1iEXT(GLuint program, GLint location, GLint v0);
GLvoid APIENTRY GLDSA_ProgramUniform1fEXT(GLuint program, GLint location, GLfloat v0);
GLvoid APIENTRY GLDSA_ProgramUniform2fEXT(GLuint program, GLint location,
	GLfloat v0, GLfloat v1);
GLvoid APIENTRY GLDSA_ProgramUniform3fEXT(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid APIENTRY GLDSA_ProgramUniform4fEXT(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid APIENTRY GLDSA_ProgramUniform1fvEXT(GLuint program, GLint location,
	GLsizei count, const GLfloat *value);
GLvoid APIENTRY GLDSA_ProgramUniformMatrix4fvEXT(GLuint program, GLint location,
	GLsizei count, GLboolean transpose,
	const GLfloat *value);

void GL_BindNullFramebuffers(void);
void GL_BindFramebuffer(GLenum target, GLuint framebuffer);
void GL_BindRenderbuffer(GLuint renderbuffer);

GLvoid APIENTRY GLDSA_NamedRenderbufferStorageEXT(GLuint renderbuffer,
	GLenum internalformat, GLsizei width, GLsizei height);

GLvoid APIENTRY GLDSA_NamedRenderbufferStorageMultisampleEXT(GLuint renderbuffer,
	GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

GLenum APIENTRY GLDSA_CheckNamedFramebufferStatusEXT(GLuint framebuffer, GLenum target);
GLvoid APIENTRY GLDSA_NamedFramebufferTexture2DEXT(GLuint framebuffer,
	GLenum attachment, GLenum textarget, GLuint texture, GLint level);
GLvoid APIENTRY GLDSA_NamedFramebufferRenderbufferEXT(GLuint framebuffer,
	GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);


#endif
