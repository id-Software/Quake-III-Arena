/*
===========================================================================
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_glsl.c
#include "tr_local.h"

#include "tr_dsa.h"

extern const char *fallbackShader_bokeh_vp;
extern const char *fallbackShader_bokeh_fp;
extern const char *fallbackShader_calclevels4x_vp;
extern const char *fallbackShader_calclevels4x_fp;
extern const char *fallbackShader_depthblur_vp;
extern const char *fallbackShader_depthblur_fp;
extern const char *fallbackShader_dlight_vp;
extern const char *fallbackShader_dlight_fp;
extern const char *fallbackShader_down4x_vp;
extern const char *fallbackShader_down4x_fp;
extern const char *fallbackShader_fogpass_vp;
extern const char *fallbackShader_fogpass_fp;
extern const char *fallbackShader_generic_vp;
extern const char *fallbackShader_generic_fp;
extern const char *fallbackShader_lightall_vp;
extern const char *fallbackShader_lightall_fp;
extern const char *fallbackShader_pshadow_vp;
extern const char *fallbackShader_pshadow_fp;
extern const char *fallbackShader_shadowfill_vp;
extern const char *fallbackShader_shadowfill_fp;
extern const char *fallbackShader_shadowmask_vp;
extern const char *fallbackShader_shadowmask_fp;
extern const char *fallbackShader_ssao_vp;
extern const char *fallbackShader_ssao_fp;
extern const char *fallbackShader_texturecolor_vp;
extern const char *fallbackShader_texturecolor_fp;
extern const char *fallbackShader_tonemap_vp;
extern const char *fallbackShader_tonemap_fp;

typedef struct uniformInfo_s
{
	char *name;
	int type;
}
uniformInfo_t;

// These must be in the same order as in uniform_t in tr_local.h.
static uniformInfo_t uniformsInfo[] =
{
	{ "u_DiffuseMap",  GLSL_INT },
	{ "u_LightMap",    GLSL_INT },
	{ "u_NormalMap",   GLSL_INT },
	{ "u_DeluxeMap",   GLSL_INT },
	{ "u_SpecularMap", GLSL_INT },

	{ "u_TextureMap", GLSL_INT },
	{ "u_LevelsMap",  GLSL_INT },
	{ "u_CubeMap",    GLSL_INT },

	{ "u_ScreenImageMap", GLSL_INT },
	{ "u_ScreenDepthMap", GLSL_INT },

	{ "u_ShadowMap",  GLSL_INT },
	{ "u_ShadowMap2", GLSL_INT },
	{ "u_ShadowMap3", GLSL_INT },
	{ "u_ShadowMap4", GLSL_INT },

	{ "u_ShadowMvp",  GLSL_MAT16 },
	{ "u_ShadowMvp2", GLSL_MAT16 },
	{ "u_ShadowMvp3", GLSL_MAT16 },
	{ "u_ShadowMvp4", GLSL_MAT16 },

	{ "u_EnableTextures", GLSL_VEC4 },

	{ "u_DiffuseTexMatrix",  GLSL_VEC4 },
	{ "u_DiffuseTexOffTurb", GLSL_VEC4 },

	{ "u_TCGen0",        GLSL_INT },
	{ "u_TCGen0Vector0", GLSL_VEC3 },
	{ "u_TCGen0Vector1", GLSL_VEC3 },

	{ "u_DeformGen",    GLSL_INT },
	{ "u_DeformParams", GLSL_FLOAT5 },

	{ "u_ColorGen",  GLSL_INT },
	{ "u_AlphaGen",  GLSL_INT },
	{ "u_Color",     GLSL_VEC4 },
	{ "u_BaseColor", GLSL_VEC4 },
	{ "u_VertColor", GLSL_VEC4 },

	{ "u_DlightInfo",    GLSL_VEC4 },
	{ "u_LightForward",  GLSL_VEC3 },
	{ "u_LightUp",       GLSL_VEC3 },
	{ "u_LightRight",    GLSL_VEC3 },
	{ "u_LightOrigin",   GLSL_VEC4 },
	{ "u_ModelLightDir", GLSL_VEC3 },
	{ "u_LightRadius",   GLSL_FLOAT },
	{ "u_AmbientLight",  GLSL_VEC3 },
	{ "u_DirectedLight", GLSL_VEC3 },

	{ "u_PortalRange", GLSL_FLOAT },

	{ "u_FogDistance",  GLSL_VEC4 },
	{ "u_FogDepth",     GLSL_VEC4 },
	{ "u_FogEyeT",      GLSL_FLOAT },
	{ "u_FogColorMask", GLSL_VEC4 },

	{ "u_ModelMatrix",               GLSL_MAT16 },
	{ "u_ModelViewProjectionMatrix", GLSL_MAT16 },

	{ "u_Time",          GLSL_FLOAT },
	{ "u_VertexLerp" ,   GLSL_FLOAT },
	{ "u_NormalScale",   GLSL_VEC4 },
	{ "u_SpecularScale", GLSL_VEC4 },

	{ "u_ViewInfo",        GLSL_VEC4 },
	{ "u_ViewOrigin",      GLSL_VEC3 },
	{ "u_LocalViewOrigin", GLSL_VEC3 },
	{ "u_ViewForward",     GLSL_VEC3 },
	{ "u_ViewLeft",        GLSL_VEC3 },
	{ "u_ViewUp",          GLSL_VEC3 },

	{ "u_InvTexRes",           GLSL_VEC2 },
	{ "u_AutoExposureMinMax",  GLSL_VEC2 },
	{ "u_ToneMinAvgMaxLinear", GLSL_VEC3 },

	{ "u_PrimaryLightOrigin",  GLSL_VEC4  },
	{ "u_PrimaryLightColor",   GLSL_VEC3  },
	{ "u_PrimaryLightAmbient", GLSL_VEC3  },
	{ "u_PrimaryLightRadius",  GLSL_FLOAT },

	{ "u_CubeMapInfo", GLSL_VEC4 },

	{ "u_AlphaTest", GLSL_INT },

	{ "u_BoneMatrix", GLSL_MAT16_BONEMATRIX },
};

typedef enum
{
	GLSL_PRINTLOG_PROGRAM_INFO,
	GLSL_PRINTLOG_SHADER_INFO,
	GLSL_PRINTLOG_SHADER_SOURCE
}
glslPrintLog_t;

static void GLSL_PrintLog(GLuint programOrShader, glslPrintLog_t type, qboolean developerOnly)
{
	char           *msg;
	static char     msgPart[1024];
	int             maxLength = 0;
	int             i;
	int             printLevel = developerOnly ? PRINT_DEVELOPER : PRINT_ALL;

	switch (type)
	{
		case GLSL_PRINTLOG_PROGRAM_INFO:
			ri.Printf(printLevel, "Program info log:\n");
			qglGetProgramiv(programOrShader, GL_INFO_LOG_LENGTH, &maxLength);
			break;

		case GLSL_PRINTLOG_SHADER_INFO:
			ri.Printf(printLevel, "Shader info log:\n");
			qglGetShaderiv(programOrShader, GL_INFO_LOG_LENGTH, &maxLength);
			break;

		case GLSL_PRINTLOG_SHADER_SOURCE:
			ri.Printf(printLevel, "Shader source:\n");
			qglGetShaderiv(programOrShader, GL_SHADER_SOURCE_LENGTH, &maxLength);
			break;
	}

	if (maxLength <= 0)
	{
		ri.Printf(printLevel, "None.\n");
		return;
	}

	if (maxLength < 1023)
		msg = msgPart;
	else
		msg = ri.Malloc(maxLength);

	switch (type)
	{
		case GLSL_PRINTLOG_PROGRAM_INFO:
			qglGetProgramInfoLog(programOrShader, maxLength, &maxLength, msg);
			break;

		case GLSL_PRINTLOG_SHADER_INFO:
			qglGetShaderInfoLog(programOrShader, maxLength, &maxLength, msg);
			break;

		case GLSL_PRINTLOG_SHADER_SOURCE:
			qglGetShaderSource(programOrShader, maxLength, &maxLength, msg);
			break;
	}

	if (maxLength < 1023)
	{
		msgPart[maxLength + 1] = '\0';

		ri.Printf(printLevel, "%s\n", msgPart);
	}
	else
	{
		for(i = 0; i < maxLength; i += 1023)
		{
			Q_strncpyz(msgPart, msg + i, sizeof(msgPart));

			ri.Printf(printLevel, "%s", msgPart);
		}

		ri.Printf(printLevel, "\n");

		ri.Free(msg);
	}

}

static void GLSL_GetShaderHeader( GLenum shaderType, const GLchar *extra, char *dest, int size )
{
	float fbufWidthScale, fbufHeightScale;

	dest[0] = '\0';

	// HACK: abuse the GLSL preprocessor to turn GLSL 1.20 shaders into 1.30 ones
	if(glRefConfig.glslMajorVersion > 1 || (glRefConfig.glslMajorVersion == 1 && glRefConfig.glslMinorVersion >= 30))
	{
		if (glRefConfig.glslMajorVersion > 1 || (glRefConfig.glslMajorVersion == 1 && glRefConfig.glslMinorVersion >= 50))
			Q_strcat(dest, size, "#version 150\n");
		else
			Q_strcat(dest, size, "#version 130\n");

		if(shaderType == GL_VERTEX_SHADER)
		{
			Q_strcat(dest, size, "#define attribute in\n");
			Q_strcat(dest, size, "#define varying out\n");
		}
		else
		{
			Q_strcat(dest, size, "#define varying in\n");

			Q_strcat(dest, size, "out vec4 out_Color;\n");
			Q_strcat(dest, size, "#define gl_FragColor out_Color\n");
			Q_strcat(dest, size, "#define texture2D texture\n");
			Q_strcat(dest, size, "#define textureCubeLod textureLod\n");
			Q_strcat(dest, size, "#define shadow2D texture\n");
		}
	}
	else
	{
		Q_strcat(dest, size, "#version 120\n");
		Q_strcat(dest, size, "#define shadow2D(a,b) shadow2D(a,b).r \n");
	}

	// HACK: add some macros to avoid extra uniforms and save speed and code maintenance
	//Q_strcat(dest, size,
	//		 va("#ifndef r_SpecularExponent\n#define r_SpecularExponent %f\n#endif\n", r_specularExponent->value));
	//Q_strcat(dest, size,
	//		 va("#ifndef r_SpecularScale\n#define r_SpecularScale %f\n#endif\n", r_specularScale->value));
	//Q_strcat(dest, size,
	//       va("#ifndef r_NormalScale\n#define r_NormalScale %f\n#endif\n", r_normalScale->value));


	Q_strcat(dest, size, "#ifndef M_PI\n#define M_PI 3.14159265358979323846\n#endif\n");

	//Q_strcat(dest, size, va("#ifndef MAX_SHADOWMAPS\n#define MAX_SHADOWMAPS %i\n#endif\n", MAX_SHADOWMAPS));

	Q_strcat(dest, size,
					 va("#ifndef deformGen_t\n"
						"#define deformGen_t\n"
						"#define DGEN_WAVE_SIN %i\n"
						"#define DGEN_WAVE_SQUARE %i\n"
						"#define DGEN_WAVE_TRIANGLE %i\n"
						"#define DGEN_WAVE_SAWTOOTH %i\n"
						"#define DGEN_WAVE_INVERSE_SAWTOOTH %i\n"
						"#define DGEN_BULGE %i\n"
						"#define DGEN_MOVE %i\n"
						"#endif\n",
						DGEN_WAVE_SIN,
						DGEN_WAVE_SQUARE,
						DGEN_WAVE_TRIANGLE,
						DGEN_WAVE_SAWTOOTH,
						DGEN_WAVE_INVERSE_SAWTOOTH,
						DGEN_BULGE,
						DGEN_MOVE));

	Q_strcat(dest, size,
					 va("#ifndef tcGen_t\n"
						"#define tcGen_t\n"
						"#define TCGEN_LIGHTMAP %i\n"
						"#define TCGEN_TEXTURE %i\n"
						"#define TCGEN_ENVIRONMENT_MAPPED %i\n"
						"#define TCGEN_FOG %i\n"
						"#define TCGEN_VECTOR %i\n"
						"#endif\n",
						TCGEN_LIGHTMAP,
						TCGEN_TEXTURE,
						TCGEN_ENVIRONMENT_MAPPED,
						TCGEN_FOG,
						TCGEN_VECTOR));

	Q_strcat(dest, size,
					 va("#ifndef colorGen_t\n"
						"#define colorGen_t\n"
						"#define CGEN_LIGHTING_DIFFUSE %i\n"
						"#endif\n",
						CGEN_LIGHTING_DIFFUSE));

	Q_strcat(dest, size,
							 va("#ifndef alphaGen_t\n"
								"#define alphaGen_t\n"
								"#define AGEN_LIGHTING_SPECULAR %i\n"
								"#define AGEN_PORTAL %i\n"
								"#endif\n",
								AGEN_LIGHTING_SPECULAR,
								AGEN_PORTAL));

	fbufWidthScale = 1.0f / ((float)glConfig.vidWidth);
	fbufHeightScale = 1.0f / ((float)glConfig.vidHeight);
	Q_strcat(dest, size,
			 va("#ifndef r_FBufScale\n#define r_FBufScale vec2(%f, %f)\n#endif\n", fbufWidthScale, fbufHeightScale));

	if (r_pbr->integer)
		Q_strcat(dest, size, "#define USE_PBR\n");

	if (r_cubeMapping->integer)
	{
		int cubeMipSize = r_cubemapSize->integer;
		int numRoughnessMips = 0;

		while (cubeMipSize)
		{
			cubeMipSize >>= 1;
			numRoughnessMips++;
		}
		numRoughnessMips = MAX(1, numRoughnessMips - 2);
		Q_strcat(dest, size, va("#define ROUGHNESS_MIPS float(%d)\n", numRoughnessMips));
	}

	if (extra)
	{
		Q_strcat(dest, size, extra);
	}

	// OK we added a lot of stuff but if we do something bad in the GLSL shaders then we want the proper line
	// so we have to reset the line counting
	Q_strcat(dest, size, "#line 0\n");
}

static int GLSL_CompileGPUShader(GLuint program, GLuint *prevShader, const GLchar *buffer, int size, GLenum shaderType)
{
	GLint           compiled;
	GLuint          shader;

	shader = qglCreateShader(shaderType);

	qglShaderSource(shader, 1, (const GLchar **)&buffer, &size);

	// compile shader
	qglCompileShader(shader);

	// check if shader compiled
	qglGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(!compiled)
	{
		GLSL_PrintLog(shader, GLSL_PRINTLOG_SHADER_SOURCE, qfalse);
		GLSL_PrintLog(shader, GLSL_PRINTLOG_SHADER_INFO, qfalse);
		ri.Error(ERR_DROP, "Couldn't compile shader");
		return 0;
	}

	if (*prevShader)
	{
		qglDetachShader(program, *prevShader);
		qglDeleteShader(*prevShader);
	}

	// attach shader to program
	qglAttachShader(program, shader);

	*prevShader = shader;

	return 1;
}

static int GLSL_LoadGPUShaderText(const char *name, const char *fallback,
	GLenum shaderType, char *dest, int destSize)
{
	char            filename[MAX_QPATH];
	GLchar      *buffer = NULL;
	const GLchar *shaderText = NULL;
	int             size;
	int             result;

	if(shaderType == GL_VERTEX_SHADER)
	{
		Com_sprintf(filename, sizeof(filename), "glsl/%s_vp.glsl", name);
	}
	else
	{
		Com_sprintf(filename, sizeof(filename), "glsl/%s_fp.glsl", name);
	}

	if ( r_externalGLSL->integer ) {
		size = ri.FS_ReadFile(filename, (void **)&buffer);
	} else {
		size = 0;
		buffer = NULL;
	}

	if(!buffer)
	{
		if (fallback)
		{
			ri.Printf(PRINT_DEVELOPER, "...loading built-in '%s'\n", filename);
			shaderText = fallback;
			size = strlen(shaderText);
		}
		else
		{
			ri.Printf(PRINT_DEVELOPER, "couldn't load '%s'\n", filename);
			return 0;
		}
	}
	else
	{
		ri.Printf(PRINT_DEVELOPER, "...loading '%s'\n", filename);
		shaderText = buffer;
	}

	if (size > destSize)
	{
		result = 0;
	}
	else
	{
		Q_strncpyz(dest, shaderText, size + 1);
		result = 1;
	}

	if (buffer)
	{
		ri.FS_FreeFile(buffer);
	}
	
	return result;
}

static void GLSL_LinkProgram(GLuint program)
{
	GLint           linked;

	qglLinkProgram(program);

	qglGetProgramiv(program, GL_LINK_STATUS, &linked);
	if(!linked)
	{
		GLSL_PrintLog(program, GLSL_PRINTLOG_PROGRAM_INFO, qfalse);
		ri.Error(ERR_DROP, "shaders failed to link");
	}
}

static void GLSL_ShowProgramUniforms(GLuint program)
{
	int             i, count, size;
	GLenum			type;
	char            uniformName[1000];

	// query the number of active uniforms
	qglGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

	// Loop over each of the active uniforms, and set their value
	for(i = 0; i < count; i++)
	{
		qglGetActiveUniform(program, i, sizeof(uniformName), NULL, &size, &type, uniformName);

		ri.Printf(PRINT_DEVELOPER, "active uniform: '%s'\n", uniformName);
	}
}

static int GLSL_InitGPUShader2(shaderProgram_t * program, const char *name, int attribs, const char *vpCode, const char *fpCode)
{
	ri.Printf(PRINT_DEVELOPER, "------- GPU shader -------\n");

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "GLSL_InitGPUShader2: \"%s\" is too long", name);
	}

	Q_strncpyz(program->name, name, sizeof(program->name));

	program->program = qglCreateProgram();
	program->attribs = attribs;

	if (!(GLSL_CompileGPUShader(program->program, &program->vertexShader, vpCode, strlen(vpCode), GL_VERTEX_SHADER)))
	{
		ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_VERTEX_SHADER\n", name);
		qglDeleteProgram(program->program);
		return 0;
	}

	if(fpCode)
	{
		if(!(GLSL_CompileGPUShader(program->program, &program->fragmentShader, fpCode, strlen(fpCode), GL_FRAGMENT_SHADER)))
		{
			ri.Printf(PRINT_ALL, "GLSL_InitGPUShader2: Unable to load \"%s\" as GL_FRAGMENT_SHADER\n", name);
			qglDeleteProgram(program->program);
			return 0;
		}
	}

	if(attribs & ATTR_POSITION)
		qglBindAttribLocation(program->program, ATTR_INDEX_POSITION, "attr_Position");

	if(attribs & ATTR_TEXCOORD)
		qglBindAttribLocation(program->program, ATTR_INDEX_TEXCOORD, "attr_TexCoord0");

	if(attribs & ATTR_LIGHTCOORD)
		qglBindAttribLocation(program->program, ATTR_INDEX_LIGHTCOORD, "attr_TexCoord1");

//  if(attribs & ATTR_TEXCOORD2)
//      qglBindAttribLocation(program->program, ATTR_INDEX_TEXCOORD2, "attr_TexCoord2");

//  if(attribs & ATTR_TEXCOORD3)
//      qglBindAttribLocation(program->program, ATTR_INDEX_TEXCOORD3, "attr_TexCoord3");

	if(attribs & ATTR_TANGENT)
		qglBindAttribLocation(program->program, ATTR_INDEX_TANGENT, "attr_Tangent");

	if(attribs & ATTR_NORMAL)
		qglBindAttribLocation(program->program, ATTR_INDEX_NORMAL, "attr_Normal");

	if(attribs & ATTR_COLOR)
		qglBindAttribLocation(program->program, ATTR_INDEX_COLOR, "attr_Color");

	if(attribs & ATTR_PAINTCOLOR)
		qglBindAttribLocation(program->program, ATTR_INDEX_PAINTCOLOR, "attr_PaintColor");

	if(attribs & ATTR_LIGHTDIRECTION)
		qglBindAttribLocation(program->program, ATTR_INDEX_LIGHTDIRECTION, "attr_LightDirection");

	if(attribs & ATTR_BONE_INDEXES)
		qglBindAttribLocation(program->program, ATTR_INDEX_BONE_INDEXES, "attr_BoneIndexes");

	if(attribs & ATTR_BONE_WEIGHTS)
		qglBindAttribLocation(program->program, ATTR_INDEX_BONE_WEIGHTS, "attr_BoneWeights");

	if(attribs & ATTR_POSITION2)
		qglBindAttribLocation(program->program, ATTR_INDEX_POSITION2, "attr_Position2");

	if(attribs & ATTR_NORMAL2)
		qglBindAttribLocation(program->program, ATTR_INDEX_NORMAL2, "attr_Normal2");

	if(attribs & ATTR_TANGENT2)
		qglBindAttribLocation(program->program, ATTR_INDEX_TANGENT2, "attr_Tangent2");

	GLSL_LinkProgram(program->program);

	return 1;
}

static int GLSL_InitGPUShader(shaderProgram_t * program, const char *name,
	int attribs, qboolean fragmentShader, const GLchar *extra, qboolean addHeader,
	const char *fallback_vp, const char *fallback_fp)
{
	char vpCode[32000];
	char fpCode[32000];
	char *postHeader;
	int size;
	int result;

	size = sizeof(vpCode);
	if (addHeader)
	{
		GLSL_GetShaderHeader(GL_VERTEX_SHADER, extra, vpCode, size);
		postHeader = &vpCode[strlen(vpCode)];
		size -= strlen(vpCode);
	}
	else
	{
		postHeader = &vpCode[0];
	}

	if (!GLSL_LoadGPUShaderText(name, fallback_vp, GL_VERTEX_SHADER, postHeader, size))
	{
		return 0;
	}

	if (fragmentShader)
	{
		size = sizeof(fpCode);
		if (addHeader)
		{
			GLSL_GetShaderHeader(GL_FRAGMENT_SHADER, extra, fpCode, size);
			postHeader = &fpCode[strlen(fpCode)];
			size -= strlen(fpCode);
		}
		else
		{
			postHeader = &fpCode[0];
		}

		if (!GLSL_LoadGPUShaderText(name, fallback_fp, GL_FRAGMENT_SHADER, postHeader, size))
		{
			return 0;
		}
	}

	result = GLSL_InitGPUShader2(program, name, attribs, vpCode, fragmentShader ? fpCode : NULL);

	return result;
}

void GLSL_InitUniforms(shaderProgram_t *program)
{
	int i, size;

	GLint *uniforms = program->uniforms;

	size = 0;
	for (i = 0; i < UNIFORM_COUNT; i++)
	{
		uniforms[i] = qglGetUniformLocation(program->program, uniformsInfo[i].name);

		if (uniforms[i] == -1)
			continue;
		 
		program->uniformBufferOffsets[i] = size;

		switch(uniformsInfo[i].type)
		{
			case GLSL_INT:
				size += sizeof(GLint);
				break;
			case GLSL_FLOAT:
				size += sizeof(GLfloat);
				break;
			case GLSL_FLOAT5:
				size += sizeof(vec_t) * 5;
				break;
			case GLSL_VEC2:
				size += sizeof(vec_t) * 2;
				break;
			case GLSL_VEC3:
				size += sizeof(vec_t) * 3;
				break;
			case GLSL_VEC4:
				size += sizeof(vec_t) * 4;
				break;
			case GLSL_MAT16:
				size += sizeof(vec_t) * 16;
				break;
			case GLSL_MAT16_BONEMATRIX:
				size += sizeof(vec_t) * 16 * glRefConfig.glslMaxAnimatedBones;
				break;
			default:
				break;
		}
	}

	program->uniformBuffer = ri.Malloc(size);
}

void GLSL_FinishGPUShader(shaderProgram_t *program)
{
	GLSL_ShowProgramUniforms(program->program);
	GL_CheckErrors();
}

void GLSL_SetUniformInt(shaderProgram_t *program, int uniformNum, GLint value)
{
	GLint *uniforms = program->uniforms;
	GLint *compare = (GLint *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_INT)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformInt: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;

	qglProgramUniform1iEXT(program->program, uniforms[uniformNum], value);
}

void GLSL_SetUniformFloat(shaderProgram_t *program, int uniformNum, GLfloat value)
{
	GLint *uniforms = program->uniforms;
	GLfloat *compare = (GLfloat *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformFloat: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (value == *compare)
	{
		return;
	}

	*compare = value;
	
	qglProgramUniform1fEXT(program->program, uniforms[uniformNum], value);
}

void GLSL_SetUniformVec2(shaderProgram_t *program, int uniformNum, const vec2_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_VEC2)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformVec2: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (v[0] == compare[0] && v[1] == compare[1])
	{
		return;
	}

	compare[0] = v[0];
	compare[1] = v[1];

	qglProgramUniform2fEXT(program->program, uniforms[uniformNum], v[0], v[1]);
}

void GLSL_SetUniformVec3(shaderProgram_t *program, int uniformNum, const vec3_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_VEC3)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformVec3: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare(v, compare))
	{
		return;
	}

	VectorCopy(v, compare);

	qglProgramUniform3fEXT(program->program, uniforms[uniformNum], v[0], v[1], v[2]);
}

void GLSL_SetUniformVec4(shaderProgram_t *program, int uniformNum, const vec4_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_VEC4)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformVec4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare4(v, compare))
	{
		return;
	}

	VectorCopy4(v, compare);

	qglProgramUniform4fEXT(program->program, uniforms[uniformNum], v[0], v[1], v[2], v[3]);
}

void GLSL_SetUniformFloat5(shaderProgram_t *program, int uniformNum, const vec5_t v)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_FLOAT5)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformFloat5: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (VectorCompare5(v, compare))
	{
		return;
	}

	VectorCopy5(v, compare);

	qglProgramUniform1fvEXT(program->program, uniforms[uniformNum], 5, v);
}

void GLSL_SetUniformMat4(shaderProgram_t *program, int uniformNum, const mat4_t matrix)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1)
		return;

	if (uniformsInfo[uniformNum].type != GLSL_MAT16)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformMat4: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (Mat4Compare(matrix, compare))
	{
		return;
	}

	Mat4Copy(matrix, compare);

	qglProgramUniformMatrix4fvEXT(program->program, uniforms[uniformNum], 1, GL_FALSE, matrix);
}

void GLSL_SetUniformMat4BoneMatrix(shaderProgram_t *program, int uniformNum, /*const*/ mat4_t *matrix, int numMatricies)
{
	GLint *uniforms = program->uniforms;
	vec_t *compare = (float *)(program->uniformBuffer + program->uniformBufferOffsets[uniformNum]);

	if (uniforms[uniformNum] == -1) {
		return;
	}

	if (uniformsInfo[uniformNum].type != GLSL_MAT16_BONEMATRIX)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformMat4BoneMatrix: wrong type for uniform %i in program %s\n", uniformNum, program->name);
		return;
	}

	if (numMatricies > glRefConfig.glslMaxAnimatedBones)
	{
		ri.Printf( PRINT_WARNING, "GLSL_SetUniformMat4BoneMatrix: too many matricies (%d/%d) for uniform %i in program %s\n",
				numMatricies, glRefConfig.glslMaxAnimatedBones, uniformNum, program->name);
		return;
	}

	if (!memcmp(matrix, compare, numMatricies * sizeof(mat4_t)))
	{
		return;
	}

	Com_Memcpy(compare, matrix, numMatricies * sizeof(mat4_t));

	qglProgramUniformMatrix4fvEXT(program->program, uniforms[uniformNum], numMatricies, GL_FALSE, &matrix[0][0]);
}

void GLSL_DeleteGPUShader(shaderProgram_t *program)
{
	if(program->program)
	{
		if (program->vertexShader)
		{
			qglDetachShader(program->program, program->vertexShader);
			qglDeleteShader(program->vertexShader);
		}

		if (program->fragmentShader)
		{
			qglDetachShader(program->program, program->fragmentShader);
			qglDeleteShader(program->fragmentShader);
		}

		qglDeleteProgram(program->program);

		if (program->uniformBuffer)
		{
			ri.Free(program->uniformBuffer);
		}

		Com_Memset(program, 0, sizeof(*program));
	}
}

void GLSL_InitGPUShaders(void)
{
	int             startTime, endTime;
	int i;
	char extradefines[1024];
	int attribs;
	int numGenShaders = 0, numLightShaders = 0, numEtcShaders = 0;

	ri.Printf(PRINT_ALL, "------- GLSL_InitGPUShaders -------\n");

	R_IssuePendingRenderCommands();

	startTime = ri.Milliseconds();

	for (i = 0; i < GENERICDEF_COUNT; i++)
	{	
		if ((i & GENERICDEF_USE_VERTEX_ANIMATION) && (i & GENERICDEF_USE_BONE_ANIMATION))
			continue;

		if ((i & GENERICDEF_USE_BONE_ANIMATION) && !glRefConfig.glslMaxAnimatedBones)
			continue;

		attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_LIGHTCOORD | ATTR_NORMAL | ATTR_COLOR;
		extradefines[0] = '\0';

		if (i & GENERICDEF_USE_DEFORM_VERTEXES)
			Q_strcat(extradefines, 1024, "#define USE_DEFORM_VERTEXES\n");

		if (i & GENERICDEF_USE_TCGEN_AND_TCMOD)
		{
			Q_strcat(extradefines, 1024, "#define USE_TCGEN\n");
			Q_strcat(extradefines, 1024, "#define USE_TCMOD\n");
		}

		if (i & GENERICDEF_USE_VERTEX_ANIMATION)
		{
			Q_strcat(extradefines, 1024, "#define USE_VERTEX_ANIMATION\n");
			attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
		}
		else if (i & GENERICDEF_USE_BONE_ANIMATION)
		{
			Q_strcat(extradefines, 1024, va("#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones));
			attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
		}

		if (i & GENERICDEF_USE_FOG)
			Q_strcat(extradefines, 1024, "#define USE_FOG\n");

		if (i & GENERICDEF_USE_RGBAGEN)
			Q_strcat(extradefines, 1024, "#define USE_RGBAGEN\n");

		if (!GLSL_InitGPUShader(&tr.genericShader[i], "generic", attribs, qtrue, extradefines, qtrue, fallbackShader_generic_vp, fallbackShader_generic_fp))
		{
			ri.Error(ERR_FATAL, "Could not load generic shader!");
		}

		GLSL_InitUniforms(&tr.genericShader[i]);

		GLSL_SetUniformInt(&tr.genericShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.genericShader[i], UNIFORM_LIGHTMAP,   TB_LIGHTMAP);

		GLSL_FinishGPUShader(&tr.genericShader[i]);

		numGenShaders++;
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD;

	if (!GLSL_InitGPUShader(&tr.textureColorShader, "texturecolor", attribs, qtrue, extradefines, qtrue, fallbackShader_texturecolor_vp, fallbackShader_texturecolor_fp))
	{
		ri.Error(ERR_FATAL, "Could not load texturecolor shader!");
	}
	
	GLSL_InitUniforms(&tr.textureColorShader);

	GLSL_SetUniformInt(&tr.textureColorShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.textureColorShader);

	numEtcShaders++;

	for (i = 0; i < FOGDEF_COUNT; i++)
	{
		if ((i & FOGDEF_USE_VERTEX_ANIMATION) && (i & FOGDEF_USE_BONE_ANIMATION))
			continue;

		if ((i & FOGDEF_USE_BONE_ANIMATION) && !glRefConfig.glslMaxAnimatedBones)
			continue;

		attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;
		extradefines[0] = '\0';

		if (i & FOGDEF_USE_DEFORM_VERTEXES)
			Q_strcat(extradefines, 1024, "#define USE_DEFORM_VERTEXES\n");

		if (i & FOGDEF_USE_VERTEX_ANIMATION)
		{
			Q_strcat(extradefines, 1024, "#define USE_VERTEX_ANIMATION\n");
			attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
		}
		else if (i & FOGDEF_USE_BONE_ANIMATION)
		{
			Q_strcat(extradefines, 1024, va("#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones));
			attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
		}

		if (!GLSL_InitGPUShader(&tr.fogShader[i], "fogpass", attribs, qtrue, extradefines, qtrue, fallbackShader_fogpass_vp, fallbackShader_fogpass_fp))
		{
			ri.Error(ERR_FATAL, "Could not load fogpass shader!");
		}

		GLSL_InitUniforms(&tr.fogShader[i]);
		GLSL_FinishGPUShader(&tr.fogShader[i]);

		numEtcShaders++;
	}


	for (i = 0; i < DLIGHTDEF_COUNT; i++)
	{
		attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;
		extradefines[0] = '\0';

		if (i & DLIGHTDEF_USE_DEFORM_VERTEXES)
		{
			Q_strcat(extradefines, 1024, "#define USE_DEFORM_VERTEXES\n");
		}

		if (!GLSL_InitGPUShader(&tr.dlightShader[i], "dlight", attribs, qtrue, extradefines, qtrue, fallbackShader_dlight_vp, fallbackShader_dlight_fp))
		{
			ri.Error(ERR_FATAL, "Could not load dlight shader!");
		}

		GLSL_InitUniforms(&tr.dlightShader[i]);
		
		GLSL_SetUniformInt(&tr.dlightShader[i], UNIFORM_DIFFUSEMAP, TB_DIFFUSEMAP);

		GLSL_FinishGPUShader(&tr.dlightShader[i]);

		numEtcShaders++;
	}


	for (i = 0; i < LIGHTDEF_COUNT; i++)
	{
		int lightType = i & LIGHTDEF_LIGHTTYPE_MASK;
		qboolean fastLight = !(r_normalMapping->integer || r_specularMapping->integer);

		// skip impossible combos
		if ((i & LIGHTDEF_USE_PARALLAXMAP) && !r_parallaxMapping->integer)
			continue;

		if ((i & LIGHTDEF_USE_SHADOWMAP) && (!lightType || !r_sunlightMode->integer))
			continue;

		if ((i & LIGHTDEF_ENTITY_VERTEX_ANIMATION) && (i & LIGHTDEF_ENTITY_BONE_ANIMATION))
			continue;

		if ((i & LIGHTDEF_ENTITY_BONE_ANIMATION) && !glRefConfig.glslMaxAnimatedBones)
			continue;

		attribs = ATTR_POSITION | ATTR_TEXCOORD | ATTR_COLOR | ATTR_NORMAL;

		extradefines[0] = '\0';

		if (r_dlightMode->integer >= 2)
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

		if (glRefConfig.swizzleNormalmap)
			Q_strcat(extradefines, 1024, "#define SWIZZLE_NORMALMAP\n");

		if (lightType)
		{
			Q_strcat(extradefines, 1024, "#define USE_LIGHT\n");

			if (fastLight)
				Q_strcat(extradefines, 1024, "#define USE_FAST_LIGHT\n");

			switch (lightType)
			{
				case LIGHTDEF_USE_LIGHTMAP:
					Q_strcat(extradefines, 1024, "#define USE_LIGHTMAP\n");
					if (r_deluxeMapping->integer && !fastLight)
						Q_strcat(extradefines, 1024, "#define USE_DELUXEMAP\n");
					attribs |= ATTR_LIGHTCOORD | ATTR_LIGHTDIRECTION;
					break;
				case LIGHTDEF_USE_LIGHT_VECTOR:
					Q_strcat(extradefines, 1024, "#define USE_LIGHT_VECTOR\n");
					break;
				case LIGHTDEF_USE_LIGHT_VERTEX:
					Q_strcat(extradefines, 1024, "#define USE_LIGHT_VERTEX\n");
					attribs |= ATTR_LIGHTDIRECTION;
					break;
				default:
					break;
			}

			if (r_normalMapping->integer)
			{
				Q_strcat(extradefines, 1024, "#define USE_NORMALMAP\n");

				attribs |= ATTR_TANGENT;

				if ((i & LIGHTDEF_USE_PARALLAXMAP) && !(i & LIGHTDEF_ENTITY_VERTEX_ANIMATION) && !(i & LIGHTDEF_ENTITY_BONE_ANIMATION) && r_parallaxMapping->integer)
				{
					Q_strcat(extradefines, 1024, "#define USE_PARALLAXMAP\n");
					if (r_parallaxMapping->integer > 1)
						Q_strcat(extradefines, 1024, "#define USE_RELIEFMAP\n");
				}
			}

			if (r_specularMapping->integer)
				Q_strcat(extradefines, 1024, "#define USE_SPECULARMAP\n");

			if (r_cubeMapping->integer)
				Q_strcat(extradefines, 1024, "#define USE_CUBEMAP\n");
			else if (r_deluxeSpecular->value > 0.000001f)
				Q_strcat(extradefines, 1024, va("#define r_deluxeSpecular %f\n", r_deluxeSpecular->value));

			switch (r_glossType->integer)
			{
				case 0:
				default:
					Q_strcat(extradefines, 1024, "#define GLOSS_IS_GLOSS\n");
					break;
				case 1:
					Q_strcat(extradefines, 1024, "#define GLOSS_IS_SMOOTHNESS\n");
					break;
				case 2:
					Q_strcat(extradefines, 1024, "#define GLOSS_IS_ROUGHNESS\n");
					break;
				case 3:
					Q_strcat(extradefines, 1024, "#define GLOSS_IS_SHININESS\n");
					break;
			}
		}

		if (i & LIGHTDEF_USE_SHADOWMAP)
		{
			Q_strcat(extradefines, 1024, "#define USE_SHADOWMAP\n");

			if (r_sunlightMode->integer == 1)
				Q_strcat(extradefines, 1024, "#define SHADOWMAP_MODULATE\n");
			else if (r_sunlightMode->integer == 2)
				Q_strcat(extradefines, 1024, "#define USE_PRIMARY_LIGHT\n");
		}

		if (i & LIGHTDEF_USE_TCGEN_AND_TCMOD)
		{
			Q_strcat(extradefines, 1024, "#define USE_TCGEN\n");
			Q_strcat(extradefines, 1024, "#define USE_TCMOD\n");
		}

		if (i & LIGHTDEF_ENTITY_VERTEX_ANIMATION)
		{
			Q_strcat(extradefines, 1024, "#define USE_VERTEX_ANIMATION\n#define USE_MODELMATRIX\n");
			attribs |= ATTR_POSITION2 | ATTR_NORMAL2;

			if (r_normalMapping->integer)
			{
				attribs |= ATTR_TANGENT2;
			}
		}
		else if (i & LIGHTDEF_ENTITY_BONE_ANIMATION)
		{
			Q_strcat(extradefines, 1024, "#define USE_MODELMATRIX\n");
			Q_strcat(extradefines, 1024, va("#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones));
			attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
		}

		if (!GLSL_InitGPUShader(&tr.lightallShader[i], "lightall", attribs, qtrue, extradefines, qtrue, fallbackShader_lightall_vp, fallbackShader_lightall_fp))
		{
			ri.Error(ERR_FATAL, "Could not load lightall shader!");
		}

		GLSL_InitUniforms(&tr.lightallShader[i]);

		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_DIFFUSEMAP,  TB_DIFFUSEMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_LIGHTMAP,    TB_LIGHTMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_NORMALMAP,   TB_NORMALMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_DELUXEMAP,   TB_DELUXEMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_SPECULARMAP, TB_SPECULARMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_SHADOWMAP,   TB_SHADOWMAP);
		GLSL_SetUniformInt(&tr.lightallShader[i], UNIFORM_CUBEMAP,     TB_CUBEMAP);

		GLSL_FinishGPUShader(&tr.lightallShader[i]);

		numLightShaders++;
	}

	for (i = 0; i < SHADOWMAPDEF_COUNT; i++)
	{
		if ((i & SHADOWMAPDEF_USE_VERTEX_ANIMATION) && (i & SHADOWMAPDEF_USE_BONE_ANIMATION))
			continue;

		if ((i & SHADOWMAPDEF_USE_BONE_ANIMATION) && !glRefConfig.glslMaxAnimatedBones)
			continue;

		attribs = ATTR_POSITION | ATTR_NORMAL | ATTR_TEXCOORD;

		extradefines[0] = '\0';

		if (i & SHADOWMAPDEF_USE_VERTEX_ANIMATION)
		{
			Q_strcat(extradefines, 1024, "#define USE_VERTEX_ANIMATION\n");
			attribs |= ATTR_POSITION2 | ATTR_NORMAL2;
		}

		if (i & SHADOWMAPDEF_USE_BONE_ANIMATION)
		{
			Q_strcat(extradefines, 1024, va("#define USE_BONE_ANIMATION\n#define MAX_GLSL_BONES %d\n", glRefConfig.glslMaxAnimatedBones));
			attribs |= ATTR_BONE_INDEXES | ATTR_BONE_WEIGHTS;
		}

		if (!GLSL_InitGPUShader(&tr.shadowmapShader[i], "shadowfill", attribs, qtrue, extradefines, qtrue, fallbackShader_shadowfill_vp, fallbackShader_shadowfill_fp))
		{
			ri.Error(ERR_FATAL, "Could not load shadowfill shader!");
		}

		GLSL_InitUniforms(&tr.shadowmapShader[i]);
		GLSL_FinishGPUShader(&tr.shadowmapShader[i]);

		numEtcShaders++;
	}

	attribs = ATTR_POSITION | ATTR_NORMAL;
	extradefines[0] = '\0';

	Q_strcat(extradefines, 1024, "#define USE_PCF\n#define USE_DISCARD\n");

	if (!GLSL_InitGPUShader(&tr.pshadowShader, "pshadow", attribs, qtrue, extradefines, qtrue, fallbackShader_pshadow_vp, fallbackShader_pshadow_fp))
	{
		ri.Error(ERR_FATAL, "Could not load pshadow shader!");
	}
	
	GLSL_InitUniforms(&tr.pshadowShader);

	GLSL_SetUniformInt(&tr.pshadowShader, UNIFORM_SHADOWMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.pshadowShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.down4xShader, "down4x", attribs, qtrue, extradefines, qtrue, fallbackShader_down4x_vp, fallbackShader_down4x_fp))
	{
		ri.Error(ERR_FATAL, "Could not load down4x shader!");
	}
	
	GLSL_InitUniforms(&tr.down4xShader);

	GLSL_SetUniformInt(&tr.down4xShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.down4xShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.bokehShader, "bokeh", attribs, qtrue, extradefines, qtrue, fallbackShader_bokeh_vp, fallbackShader_bokeh_fp))
	{
		ri.Error(ERR_FATAL, "Could not load bokeh shader!");
	}

	GLSL_InitUniforms(&tr.bokehShader);

	GLSL_SetUniformInt(&tr.bokehShader, UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

	GLSL_FinishGPUShader(&tr.bokehShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.tonemapShader, "tonemap", attribs, qtrue, extradefines, qtrue, fallbackShader_tonemap_vp, fallbackShader_tonemap_fp))
	{
		ri.Error(ERR_FATAL, "Could not load tonemap shader!");
	}

	GLSL_InitUniforms(&tr.tonemapShader);

	GLSL_SetUniformInt(&tr.tonemapShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.tonemapShader, UNIFORM_LEVELSMAP,  TB_LEVELSMAP);

	GLSL_FinishGPUShader(&tr.tonemapShader);

	numEtcShaders++;


	for (i = 0; i < 2; i++)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD;
		extradefines[0] = '\0';

		if (!i)
			Q_strcat(extradefines, 1024, "#define FIRST_PASS\n");

		if (!GLSL_InitGPUShader(&tr.calclevels4xShader[i], "calclevels4x", attribs, qtrue, extradefines, qtrue, fallbackShader_calclevels4x_vp, fallbackShader_calclevels4x_fp))
		{
			ri.Error(ERR_FATAL, "Could not load calclevels4x shader!");
		}

		GLSL_InitUniforms(&tr.calclevels4xShader[i]);

		GLSL_SetUniformInt(&tr.calclevels4xShader[i], UNIFORM_TEXTUREMAP, TB_DIFFUSEMAP);

		GLSL_FinishGPUShader(&tr.calclevels4xShader[i]);

		numEtcShaders++;		
	}


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (r_shadowFilter->integer >= 1)
		Q_strcat(extradefines, 1024, "#define USE_SHADOW_FILTER\n");

	if (r_shadowFilter->integer >= 2)
		Q_strcat(extradefines, 1024, "#define USE_SHADOW_FILTER2\n");

	if (r_shadowCascadeZFar->integer != 0)
		Q_strcat(extradefines, 1024, "#define USE_SHADOW_CASCADE\n");

	Q_strcat(extradefines, 1024, va("#define r_shadowMapSize %f\n", r_shadowMapSize->value));
	Q_strcat(extradefines, 1024, va("#define r_shadowCascadeZFar %f\n", r_shadowCascadeZFar->value));


	if (!GLSL_InitGPUShader(&tr.shadowmaskShader, "shadowmask", attribs, qtrue, extradefines, qtrue, fallbackShader_shadowmask_vp, fallbackShader_shadowmask_fp))
	{
		ri.Error(ERR_FATAL, "Could not load shadowmask shader!");
	}
	
	GLSL_InitUniforms(&tr.shadowmaskShader);

	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP,  TB_SHADOWMAP);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP2, TB_SHADOWMAP2);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP3, TB_SHADOWMAP3);
	GLSL_SetUniformInt(&tr.shadowmaskShader, UNIFORM_SHADOWMAP4, TB_SHADOWMAP4);

	GLSL_FinishGPUShader(&tr.shadowmaskShader);

	numEtcShaders++;


	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.ssaoShader, "ssao", attribs, qtrue, extradefines, qtrue, fallbackShader_ssao_vp, fallbackShader_ssao_fp))
	{
		ri.Error(ERR_FATAL, "Could not load ssao shader!");
	}

	GLSL_InitUniforms(&tr.ssaoShader);

	GLSL_SetUniformInt(&tr.ssaoShader, UNIFORM_SCREENDEPTHMAP, TB_COLORMAP);

	GLSL_FinishGPUShader(&tr.ssaoShader);

	numEtcShaders++;


	for (i = 0; i < 4; i++)
	{
		attribs = ATTR_POSITION | ATTR_TEXCOORD;
		extradefines[0] = '\0';

		if (i & 1)
			Q_strcat(extradefines, 1024, "#define USE_VERTICAL_BLUR\n");
		else
			Q_strcat(extradefines, 1024, "#define USE_HORIZONTAL_BLUR\n");

		if (!(i & 2))
			Q_strcat(extradefines, 1024, "#define USE_DEPTH\n");


		if (!GLSL_InitGPUShader(&tr.depthBlurShader[i], "depthBlur", attribs, qtrue, extradefines, qtrue, fallbackShader_depthblur_vp, fallbackShader_depthblur_fp))
		{
			ri.Error(ERR_FATAL, "Could not load depthBlur shader!");
		}
		
		GLSL_InitUniforms(&tr.depthBlurShader[i]);

		GLSL_SetUniformInt(&tr.depthBlurShader[i], UNIFORM_SCREENIMAGEMAP, TB_COLORMAP);
		GLSL_SetUniformInt(&tr.depthBlurShader[i], UNIFORM_SCREENDEPTHMAP, TB_LIGHTMAP);

		GLSL_FinishGPUShader(&tr.depthBlurShader[i]);

		numEtcShaders++;
	}

#if 0
	attribs = ATTR_POSITION | ATTR_TEXCOORD;
	extradefines[0] = '\0';

	if (!GLSL_InitGPUShader(&tr.testcubeShader, "testcube", attribs, qtrue, extradefines, qtrue, NULL, NULL))
	{
		ri.Error(ERR_FATAL, "Could not load testcube shader!");
	}

	GLSL_InitUniforms(&tr.testcubeShader);

	GLSL_SetUniformInt(&tr.testcubeShader, UNIFORM_TEXTUREMAP, TB_COLORMAP);

	GLSL_FinishGPUShader(&tr.testcubeShader);

	numEtcShaders++;
#endif


	endTime = ri.Milliseconds();

	ri.Printf(PRINT_ALL, "loaded %i GLSL shaders (%i gen %i light %i etc) in %5.2f seconds\n", 
		numGenShaders + numLightShaders + numEtcShaders, numGenShaders, numLightShaders, 
		numEtcShaders, (endTime - startTime) / 1000.0);
}

void GLSL_ShutdownGPUShaders(void)
{
	int i;

	ri.Printf(PRINT_ALL, "------- GLSL_ShutdownGPUShaders -------\n");

	for (i = 0; i < ATTR_INDEX_COUNT; i++)
		qglDisableVertexAttribArray(i);

	GL_BindNullProgram();

	for ( i = 0; i < GENERICDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.genericShader[i]);

	GLSL_DeleteGPUShader(&tr.textureColorShader);

	for ( i = 0; i < FOGDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.fogShader[i]);

	for ( i = 0; i < DLIGHTDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.dlightShader[i]);

	for ( i = 0; i < LIGHTDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.lightallShader[i]);

	for ( i = 0; i < SHADOWMAPDEF_COUNT; i++)
		GLSL_DeleteGPUShader(&tr.shadowmapShader[i]);

	GLSL_DeleteGPUShader(&tr.pshadowShader);
	GLSL_DeleteGPUShader(&tr.down4xShader);
	GLSL_DeleteGPUShader(&tr.bokehShader);
	GLSL_DeleteGPUShader(&tr.tonemapShader);

	for ( i = 0; i < 2; i++)
		GLSL_DeleteGPUShader(&tr.calclevels4xShader[i]);

	GLSL_DeleteGPUShader(&tr.shadowmaskShader);
	GLSL_DeleteGPUShader(&tr.ssaoShader);

	for ( i = 0; i < 4; i++)
		GLSL_DeleteGPUShader(&tr.depthBlurShader[i]);
}


void GLSL_BindProgram(shaderProgram_t * program)
{
	GLuint programObject = program ? program->program : 0;
	char *name = program ? program->name : "NULL";

	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- GLSL_BindProgram( %s ) ---\n", name));
	}

	if (GL_UseProgram(programObject))
		backEnd.pc.c_glslShaderBinds++;
}


shaderProgram_t *GLSL_GetGenericShaderProgram(int stage)
{
	shaderStage_t *pStage = tess.xstages[stage];
	int shaderAttribs = 0;

	if (tess.fogNum && pStage->adjustColorsForFog)
	{
		shaderAttribs |= GENERICDEF_USE_FOG;
	}

	switch (pStage->rgbGen)
	{
		case CGEN_LIGHTING_DIFFUSE:
			shaderAttribs |= GENERICDEF_USE_RGBAGEN;
			break;
		default:
			break;
	}

	switch (pStage->alphaGen)
	{
		case AGEN_LIGHTING_SPECULAR:
		case AGEN_PORTAL:
			shaderAttribs |= GENERICDEF_USE_RGBAGEN;
			break;
		default:
			break;
	}

	if (pStage->bundle[0].tcGen != TCGEN_TEXTURE)
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
	}

	if (tess.shader->numDeforms && !ShaderRequiresCPUDeforms(tess.shader))
	{
		shaderAttribs |= GENERICDEF_USE_DEFORM_VERTEXES;
	}

	if (glState.vertexAnimation)
	{
		shaderAttribs |= GENERICDEF_USE_VERTEX_ANIMATION;
	}
	else if (glState.boneAnimation)
	{
		shaderAttribs |= GENERICDEF_USE_BONE_ANIMATION;
	}

	if (pStage->bundle[0].numTexMods)
	{
		shaderAttribs |= GENERICDEF_USE_TCGEN_AND_TCMOD;
	}

	return &tr.genericShader[shaderAttribs];
}
