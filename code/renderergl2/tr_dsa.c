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

#include "tr_local.h"

#include "tr_dsa.h"

static struct
{
	GLuint textures[NUM_TEXTURE_BUNDLES];
	GLenum texunit;

	GLuint program;

	GLuint drawFramebuffer;
	GLuint readFramebuffer;
	GLuint renderbuffer;
}
glDsaState;

void GL_BindNullTextures()
{
	int i;

	if (glRefConfig.directStateAccess)
	{
		for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
		{
			qglBindMultiTextureEXT(GL_TEXTURE0_ARB + i, GL_TEXTURE_2D, 0);
			glDsaState.textures[i] = 0;
		}
	}
	else
	{
		for (i = 0; i < NUM_TEXTURE_BUNDLES; i++)
		{
			qglActiveTextureARB(GL_TEXTURE0_ARB + i);
			qglBindTexture(GL_TEXTURE_2D, 0);
			glDsaState.textures[i] = 0;
		}

		qglActiveTextureARB(GL_TEXTURE0_ARB);
		glDsaState.texunit = GL_TEXTURE0_ARB;
	}
}

int GL_BindMultiTexture(GLenum texunit, GLenum target, GLuint texture)
{
	GLuint tmu = texunit - GL_TEXTURE0_ARB;

	if (glDsaState.textures[tmu] == texture)
		return 0;

	if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z)
		target = GL_TEXTURE_CUBE_MAP;

	qglBindMultiTextureEXT(texunit, target, texture);
	glDsaState.textures[tmu] = texture;
	return 1;
}

GLvoid APIENTRY GLDSA_BindMultiTextureEXT(GLenum texunit, GLenum target, GLuint texture)
{
	if (glDsaState.texunit != texunit)
	{
		qglActiveTextureARB(texunit);
		glDsaState.texunit = texunit;
	}

	qglBindTexture(target, texture);
}

GLvoid APIENTRY GLDSA_TextureParameterfEXT(GLuint texture, GLenum target, GLenum pname, GLfloat param)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglTexParameterf(target, pname, param);
}

GLvoid APIENTRY GLDSA_TextureParameteriEXT(GLuint texture, GLenum target, GLenum pname, GLint param)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglTexParameteri(target, pname, param);
}

GLvoid APIENTRY GLDSA_TextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLint internalformat,
	GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

GLvoid APIENTRY GLDSA_TextureSubImage2DEXT(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset,
	GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

GLvoid APIENTRY GLDSA_CopyTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

GLvoid APIENTRY  GLDSA_CompressedTextureImage2DEXT(GLuint texture, GLenum target, GLint level, GLenum internalformat,
	GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
}

GLvoid APIENTRY GLDSA_CompressedTextureSubImage2DEXT(GLuint texture, GLenum target, GLint level,
	GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
	GLsizei imageSize, const GLvoid *data)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
}

GLvoid APIENTRY GLDSA_GenerateTextureMipmapEXT(GLuint texture, GLenum target)
{
	GL_BindMultiTexture(glDsaState.texunit, target, texture);
	qglGenerateMipmapEXT(target);
}

void GL_BindNullProgram()
{
	qglUseProgram(0);
	glDsaState.program = 0;
}

int GL_UseProgram(GLuint program)
{
	if (glDsaState.program == program)
		return 0;

	qglUseProgram(program);
	glDsaState.program = program;
	return 1;
}

GLvoid APIENTRY GLDSA_ProgramUniform1iEXT(GLuint program, GLint location, GLint v0)
{
	GL_UseProgram(program);
	qglUniform1i(location, v0);
}

GLvoid APIENTRY GLDSA_ProgramUniform1fEXT(GLuint program, GLint location, GLfloat v0)
{
	GL_UseProgram(program);
	qglUniform1f(location, v0);
}

GLvoid APIENTRY GLDSA_ProgramUniform2fEXT(GLuint program, GLint location,
	GLfloat v0, GLfloat v1)
{
	GL_UseProgram(program);
	qglUniform2f(location, v0, v1);
}

GLvoid APIENTRY GLDSA_ProgramUniform3fEXT(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2)
{
	GL_UseProgram(program);
	qglUniform3f(location, v0, v1, v2);
}

GLvoid APIENTRY GLDSA_ProgramUniform4fEXT(GLuint program, GLint location,
	GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
	GL_UseProgram(program);
	qglUniform4f(location, v0, v1, v2, v3);
}

GLvoid APIENTRY GLDSA_ProgramUniform1fvEXT(GLuint program, GLint location,
	GLsizei count, const GLfloat *value)
{
	GL_UseProgram(program);
	qglUniform1fv(location, count, value);
}

GLvoid APIENTRY GLDSA_ProgramUniformMatrix4fvEXT(GLuint program, GLint location,
	GLsizei count, GLboolean transpose,
	const GLfloat *value)
{
	GL_UseProgram(program);
	qglUniformMatrix4fv(location, count, transpose, value);
}

void GL_BindNullFramebuffers()
{
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glDsaState.drawFramebuffer = glDsaState.readFramebuffer = 0;
	qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	glDsaState.renderbuffer = 0;
}

void GL_BindFramebuffer(GLenum target, GLuint framebuffer)
{
	switch (target)
	{
		case GL_FRAMEBUFFER_EXT:
			if (framebuffer != glDsaState.drawFramebuffer || framebuffer != glDsaState.readFramebuffer)
			{
				qglBindFramebufferEXT(target, framebuffer);
				glDsaState.drawFramebuffer = glDsaState.readFramebuffer = framebuffer;
			}
			break;

		case GL_DRAW_FRAMEBUFFER_EXT:
			if (framebuffer != glDsaState.drawFramebuffer)
			{
				qglBindFramebufferEXT(target, framebuffer);
				glDsaState.drawFramebuffer = framebuffer;
			}
			break;

		case GL_READ_FRAMEBUFFER_EXT:
			if (framebuffer != glDsaState.readFramebuffer)
			{
				qglBindFramebufferEXT(target, framebuffer);
				glDsaState.readFramebuffer = framebuffer;
			}
			break;
	}
}

void GL_BindRenderbuffer(GLuint renderbuffer)
{
	if (renderbuffer != glDsaState.renderbuffer)
	{
		qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer);
		glDsaState.renderbuffer = renderbuffer;
	}
}

GLvoid APIENTRY GLDSA_NamedRenderbufferStorageEXT(GLuint renderbuffer,
	GLenum internalformat, GLsizei width, GLsizei height)
{
	GL_BindRenderbuffer(renderbuffer);
	qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, internalformat, width, height);
}

GLvoid APIENTRY GLDSA_NamedRenderbufferStorageMultisampleEXT(GLuint renderbuffer,
	GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
	GL_BindRenderbuffer(renderbuffer);
	qglRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, samples, internalformat, width, height);
}

GLenum APIENTRY GLDSA_CheckNamedFramebufferStatusEXT(GLuint framebuffer, GLenum target)
{
	GL_BindFramebuffer(target, framebuffer);
	return qglCheckFramebufferStatusEXT(target);
}

GLvoid APIENTRY GLDSA_NamedFramebufferTexture2DEXT(GLuint framebuffer,
	GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
	GL_BindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer);
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, attachment, textarget, texture, level);
}

GLvoid APIENTRY GLDSA_NamedFramebufferRenderbufferEXT(GLuint framebuffer,
	GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
	GL_BindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer);
	qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attachment, renderbuffertarget, renderbuffer);
}
