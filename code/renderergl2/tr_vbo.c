/*
===========================================================================
Copyright (C) 2007-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_vbo.c
#include "tr_local.h"


union pack10_u {
	struct {
		signed int x:10;
		signed int y:10;
		signed int z:10;
		signed int w:2;
	} pack;
	uint32_t i;
};

union pack8_u {
	struct {
		signed int x:8;
		signed int y:8;
		signed int z:8;
		signed int w:8;
	} pack;
	uint32_t i;
};


uint32_t R_VaoPackTangent(vec4_t v)
{
	if (glRefConfig.packedNormalDataType == GL_INT_2_10_10_10_REV)
	{
		union pack10_u num;

		num.pack.x = v[0] * 511.0f;
		num.pack.y = v[1] * 511.0f;
		num.pack.z = v[2] * 511.0f;
		num.pack.w = v[3];

		return num.i;
	}
	else
	{
		union pack8_u num;

		num.pack.x = v[0] * 127.0f;
		num.pack.y = v[1] * 127.0f;
		num.pack.z = v[2] * 127.0f;
		num.pack.w = v[3] * 127.0f;

		return num.i;
	}
}

uint32_t R_VaoPackNormal(vec3_t v)
{
	if (glRefConfig.packedNormalDataType == GL_INT_2_10_10_10_REV)
	{
		union pack10_u num;

		num.pack.x = v[0] * 511.0f;
		num.pack.y = v[1] * 511.0f;
		num.pack.z = v[2] * 511.0f;
		num.pack.w = 0;

		return num.i;
	}
	else
	{
		union pack8_u num;

		num.pack.x = v[0] * 127.0f;
		num.pack.y = v[1] * 127.0f;
		num.pack.z = v[2] * 127.0f;
		num.pack.w = 0;

		return num.i;
	}
}

void R_VaoUnpackTangent(vec4_t v, uint32_t b)
{
	if (glRefConfig.packedNormalDataType == GL_INT_2_10_10_10_REV)
	{
		union pack10_u num;

		num.i = b;

		v[0] = num.pack.x / 511.0f;
		v[1] = num.pack.y / 511.0f;
		v[2] = num.pack.z / 511.0f;
		v[3] = num.pack.w; 
	}
	else
	{
		union pack8_u num;

		num.i = b;

		v[0] = num.pack.x / 127.0f;
		v[1] = num.pack.y / 127.0f;
		v[2] = num.pack.z / 127.0f;
		v[3] = num.pack.w / 127.0f; 
	}
}

void R_VaoUnpackNormal(vec3_t v, uint32_t b)
{
	if (glRefConfig.packedNormalDataType == GL_INT_2_10_10_10_REV)
	{
		union pack10_u num;

		num.i = b;

		v[0] = num.pack.x / 511.0f;
		v[1] = num.pack.y / 511.0f;
		v[2] = num.pack.z / 511.0f;
	}
	else
	{
		union pack8_u num;

		num.i = b;

		v[0] = num.pack.x / 127.0f;
		v[1] = num.pack.y / 127.0f;
		v[2] = num.pack.z / 127.0f;
	}
}


void Vao_SetVertexPointers(vao_t *vao)
{
	int attribIndex;

	// set vertex pointers
	for (attribIndex = 0; attribIndex < ATTR_INDEX_COUNT; attribIndex++)
	{
		uint32_t attribBit = 1 << attribIndex;
		vaoAttrib_t *vAtb = &vao->attribs[attribIndex];

		if (vAtb->enabled)
		{
			qglVertexAttribPointerARB(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));
			if (glRefConfig.vertexArrayObject || !(glState.vertexAttribsEnabled & attribBit))
				qglEnableVertexAttribArrayARB(attribIndex);

			if (!glRefConfig.vertexArrayObject || vao == tess.vao)
				glState.vertexAttribsEnabled |= attribBit;
		}
		else
		{
			// don't disable vertex attribs when using vertex array objects
			// Vao_SetVertexPointers is only called during init when using VAOs, and vertex attribs start disabled anyway
			if (!glRefConfig.vertexArrayObject && (glState.vertexAttribsEnabled & attribBit))
				qglDisableVertexAttribArrayARB(attribIndex);

			if (!glRefConfig.vertexArrayObject || vao == tess.vao)
				glState.vertexAttribsEnabled &= ~attribBit;
		}
	}
}

/*
============
R_CreateVao
============
*/
vao_t *R_CreateVao(const char *name, byte *vertexes, int vertexesSize, byte *indexes, int indexesSize, vaoUsage_t usage)
{
	vao_t          *vao;
	int				glUsage;

	switch (usage)
	{
		case VAO_USAGE_STATIC:
			glUsage = GL_STATIC_DRAW_ARB;
			break;

		case VAO_USAGE_DYNAMIC:
			glUsage = GL_DYNAMIC_DRAW_ARB;
			break;

		default:
			Com_Error(ERR_FATAL, "bad vaoUsage_t given: %i", usage);
			return NULL;
	}

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "R_CreateVao: \"%s\" is too long", name);
	}

	if ( tr.numVaos == MAX_VAOS ) {
		ri.Error( ERR_DROP, "R_CreateVao: MAX_VAOS hit");
	}

	R_IssuePendingRenderCommands();

	vao = tr.vaos[tr.numVaos] = ri.Hunk_Alloc(sizeof(*vao), h_low);
	tr.numVaos++;

	memset(vao, 0, sizeof(*vao));

	Q_strncpyz(vao->name, name, sizeof(vao->name));


	if (glRefConfig.vertexArrayObject)
	{
		qglGenVertexArraysARB(1, &vao->vao);
		qglBindVertexArrayARB(vao->vao);
	}


	vao->vertexesSize = vertexesSize;

	qglGenBuffersARB(1, &vao->vertexesVBO);

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->vertexesVBO);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vertexesSize, vertexes, glUsage);


	vao->indexesSize = indexesSize;

	qglGenBuffersARB(1, &vao->indexesIBO);

	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vao->indexesIBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, indexesSize, indexes, glUsage);


	glState.currentVao = vao;

	GL_CheckErrors();

	return vao;
}

/*
============
R_CreateVao2
============
*/
vao_t *R_CreateVao2(const char *name, int numVertexes, srfVert_t *verts, int numIndexes, glIndex_t *indexes)
{
	vao_t          *vao;
	int             i;

	byte           *data;
	int             dataSize;
	int             dataOfs;

	int				glUsage = GL_STATIC_DRAW_ARB;

	if(!numVertexes || !numIndexes)
		return NULL;

	if(strlen(name) >= MAX_QPATH)
	{
		ri.Error(ERR_DROP, "R_CreateVao2: \"%s\" is too long", name);
	}

	if ( tr.numVaos == MAX_VAOS ) {
		ri.Error( ERR_DROP, "R_CreateVao2: MAX_VAOS hit");
	}

	R_IssuePendingRenderCommands();

	vao = tr.vaos[tr.numVaos] = ri.Hunk_Alloc(sizeof(*vao), h_low);
	tr.numVaos++;

	memset(vao, 0, sizeof(*vao));

	Q_strncpyz(vao->name, name, sizeof(vao->name));

	// since these vertex attributes are never altered, interleave them
	vao->attribs[ATTR_INDEX_POSITION      ].enabled = 1;
	vao->attribs[ATTR_INDEX_NORMAL        ].enabled = 1;
#ifdef USE_VERT_TANGENT_SPACE
	vao->attribs[ATTR_INDEX_TANGENT       ].enabled = 1;
#endif
	vao->attribs[ATTR_INDEX_TEXCOORD      ].enabled = 1;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].enabled = 1;
	vao->attribs[ATTR_INDEX_COLOR         ].enabled = 1;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].enabled = 1;

	vao->attribs[ATTR_INDEX_POSITION      ].count = 3;
	vao->attribs[ATTR_INDEX_NORMAL        ].count = 4;
	vao->attribs[ATTR_INDEX_TANGENT       ].count = 4;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].count = 2;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].count = 2;
	vao->attribs[ATTR_INDEX_COLOR         ].count = 4;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].count = 4;

	vao->attribs[ATTR_INDEX_POSITION      ].type = GL_FLOAT;
	vao->attribs[ATTR_INDEX_NORMAL        ].type = glRefConfig.packedNormalDataType;
	vao->attribs[ATTR_INDEX_TANGENT       ].type = glRefConfig.packedNormalDataType;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].type = GL_FLOAT;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].type = GL_FLOAT;
	vao->attribs[ATTR_INDEX_COLOR         ].type = GL_FLOAT;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].type = glRefConfig.packedNormalDataType;

	vao->attribs[ATTR_INDEX_POSITION      ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_NORMAL        ].normalized = GL_TRUE;
	vao->attribs[ATTR_INDEX_TANGENT       ].normalized = GL_TRUE;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_COLOR         ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].normalized = GL_TRUE;

	vao->attribs[ATTR_INDEX_POSITION      ].offset = 0;        dataSize  = sizeof(verts[0].xyz);
	vao->attribs[ATTR_INDEX_NORMAL        ].offset = dataSize; dataSize += sizeof(uint32_t);
#ifdef USE_VERT_TANGENT_SPACE
	vao->attribs[ATTR_INDEX_TANGENT       ].offset = dataSize; dataSize += sizeof(uint32_t);
#endif
	vao->attribs[ATTR_INDEX_TEXCOORD      ].offset = dataSize; dataSize += sizeof(verts[0].st);
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].offset = dataSize; dataSize += sizeof(verts[0].lightmap);
	vao->attribs[ATTR_INDEX_COLOR         ].offset = dataSize; dataSize += sizeof(verts[0].vertexColors);
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].offset = dataSize; dataSize += sizeof(uint32_t);

	vao->attribs[ATTR_INDEX_POSITION      ].stride = dataSize;
	vao->attribs[ATTR_INDEX_NORMAL        ].stride = dataSize;
	vao->attribs[ATTR_INDEX_TANGENT       ].stride = dataSize;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].stride = dataSize;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].stride = dataSize;
	vao->attribs[ATTR_INDEX_COLOR         ].stride = dataSize;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].stride = dataSize;


	if (glRefConfig.vertexArrayObject)
	{
		qglGenVertexArraysARB(1, &vao->vao);
		qglBindVertexArrayARB(vao->vao);
	}


	// create VBO
	dataSize *= numVertexes;
	data = ri.Hunk_AllocateTempMemory(dataSize);
	dataOfs = 0;

	for (i = 0; i < numVertexes; i++)
	{
		uint32_t *p;

		// xyz
		memcpy(data + dataOfs, &verts[i].xyz, sizeof(verts[i].xyz));
		dataOfs += sizeof(verts[i].xyz);

		// normal
		p = (uint32_t *)(data + dataOfs);
		*p = R_VaoPackNormal(verts[i].normal);
		dataOfs += sizeof(uint32_t);

#ifdef USE_VERT_TANGENT_SPACE
		// tangent
		p = (uint32_t *)(data + dataOfs);
		*p = R_VaoPackTangent(verts[i].tangent);
		dataOfs += sizeof(uint32_t);
#endif

		// vertex texcoords
		memcpy(data + dataOfs, &verts[i].st, sizeof(verts[i].st));
		dataOfs += sizeof(verts[i].st);

		// feed vertex lightmap texcoords
		memcpy(data + dataOfs, &verts[i].lightmap, sizeof(verts[i].lightmap));
		dataOfs += sizeof(verts[i].lightmap);

		// feed vertex colors
		memcpy(data + dataOfs, &verts[i].vertexColors, sizeof(verts[i].vertexColors));
		dataOfs += sizeof(verts[i].vertexColors);

		// feed vertex light directions
		p = (uint32_t *)(data + dataOfs);
		*p = R_VaoPackNormal(verts[i].lightdir);
		dataOfs += sizeof(uint32_t);
	}

	vao->vertexesSize = dataSize;

	qglGenBuffersARB(1, &vao->vertexesVBO);

	qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->vertexesVBO);
	qglBufferDataARB(GL_ARRAY_BUFFER_ARB, vao->vertexesSize, data, glUsage);


	// create IBO
	vao->indexesSize = numIndexes * sizeof(glIndex_t);

	qglGenBuffersARB(1, &vao->indexesIBO);

	qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vao->indexesIBO);
	qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vao->indexesSize, indexes, glUsage);


	Vao_SetVertexPointers(vao);


	glState.currentVao = vao;

	GL_CheckErrors();

	ri.Hunk_FreeTempMemory(data);

	return vao;
}


/*
============
R_BindVao
============
*/
void R_BindVao(vao_t * vao)
{
	if(!vao)
	{
		//R_BindNullVao();
		ri.Error(ERR_DROP, "R_BindVao: NULL vao");
		return;
	}

	if(r_logFile->integer)
	{
		// don't just call LogComment, or we will get a call to va() every frame!
		GLimp_LogComment(va("--- R_BindVao( %s ) ---\n", vao->name));
	}

	if(glState.currentVao != vao)
	{
		glState.currentVao = vao;

		glState.vertexAttribsInterpolation = 0;
		glState.vertexAnimation = qfalse;
		backEnd.pc.c_vaoBinds++;

		if (glRefConfig.vertexArrayObject)
		{
			qglBindVertexArrayARB(vao->vao);

			// why you no save GL_ELEMENT_ARRAY_BUFFER binding, Intel?
			if (1)
				qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO);

			// tess VAO always has buffers bound
			if (vao == tess.vao)
				qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->vertexesVBO);
		}
		else
		{
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vao->vertexesVBO);
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, vao->indexesIBO);

			// tess VAO doesn't have vertex pointers set until data is uploaded
			if (vao != tess.vao)
				Vao_SetVertexPointers(vao);
		}
	}
}

/*
============
R_BindNullVao
============
*/
void R_BindNullVao(void)
{
	GLimp_LogComment("--- R_BindNullVao ---\n");

	if(glState.currentVao)
	{
		if (glRefConfig.vertexArrayObject)
		{
			qglBindVertexArrayARB(0);

			// why you no save GL_ELEMENT_ARRAY_BUFFER binding, Intel?
			if (1) qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		}
		else
		{
			qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			qglBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		}
		glState.currentVao = NULL;
	}

	GL_CheckErrors();
}


/*
============
R_InitVaos
============
*/
void R_InitVaos(void)
{
	int             vertexesSize, indexesSize;
	int             offset;

	ri.Printf(PRINT_ALL, "------- R_InitVaos -------\n");

	tr.numVaos = 0;

	vertexesSize  = sizeof(tess.xyz[0]);
	vertexesSize += sizeof(tess.normal[0]);
#ifdef USE_VERT_TANGENT_SPACE
	vertexesSize += sizeof(tess.tangent[0]);
#endif
	vertexesSize += sizeof(tess.vertexColors[0]);
	vertexesSize += sizeof(tess.texCoords[0][0]) * 2;
	vertexesSize += sizeof(tess.lightdir[0]);
	vertexesSize *= SHADER_MAX_VERTEXES;

	indexesSize = sizeof(tess.indexes[0]) * SHADER_MAX_INDEXES;

	tess.vao = R_CreateVao("tessVertexArray_VAO", NULL, vertexesSize, NULL, indexesSize, VAO_USAGE_DYNAMIC);

	offset = 0;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].enabled = 1;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].enabled = 1;
#ifdef USE_VERT_TANGENT_SPACE
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].enabled = 1;
#endif
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].enabled = 1;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].enabled = 1;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].enabled = 1;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].enabled = 1;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].count = 3;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].count = 4;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].count = 4;
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].count = 2;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].count = 2;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].count = 4;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].count = 4;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].type = GL_FLOAT;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].type = glRefConfig.packedNormalDataType;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].type = glRefConfig.packedNormalDataType;
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].type = GL_FLOAT;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].type = GL_FLOAT;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].type = GL_FLOAT;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].type = glRefConfig.packedNormalDataType;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].normalized = GL_TRUE;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].normalized = GL_TRUE;
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].normalized = GL_TRUE;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].offset = offset; offset += sizeof(tess.xyz[0])              * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].offset = offset; offset += sizeof(tess.normal[0])           * SHADER_MAX_VERTEXES;
#ifdef USE_VERT_TANGENT_SPACE
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].offset = offset; offset += sizeof(tess.tangent[0])          * SHADER_MAX_VERTEXES;
#endif
	// these next two are actually interleaved
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].offset = offset; 
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].offset = offset + sizeof(tess.texCoords[0][0]);
	                                                              offset += sizeof(tess.texCoords[0][0]) * 2 * SHADER_MAX_VERTEXES;

	tess.vao->attribs[ATTR_INDEX_COLOR         ].offset = offset; offset += sizeof(tess.vertexColors[0])     * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].offset = offset;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].stride = sizeof(tess.xyz[0]);
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].stride = sizeof(tess.normal[0]);
#ifdef USE_VERT_TANGENT_SPACE
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].stride = sizeof(tess.tangent[0]);
#endif
	tess.vao->attribs[ATTR_INDEX_COLOR         ].stride = sizeof(tess.vertexColors[0]);
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].stride = sizeof(tess.texCoords[0][0]) * 2;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].stride = sizeof(tess.texCoords[0][0]) * 2;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].stride = sizeof(tess.lightdir[0]);

	tess.attribPointers[ATTR_INDEX_POSITION]       = tess.xyz;
	tess.attribPointers[ATTR_INDEX_TEXCOORD]       = tess.texCoords;
	tess.attribPointers[ATTR_INDEX_NORMAL]         = tess.normal;
#ifdef USE_VERT_TANGENT_SPACE
	tess.attribPointers[ATTR_INDEX_TANGENT]        = tess.tangent;
#endif
	tess.attribPointers[ATTR_INDEX_COLOR]          = tess.vertexColors;
	tess.attribPointers[ATTR_INDEX_LIGHTDIRECTION] = tess.lightdir;

	Vao_SetVertexPointers(tess.vao);

	R_BindNullVao();

	GL_CheckErrors();
}

/*
============
R_ShutdownVaos
============
*/
void R_ShutdownVaos(void)
{
	int             i;
	vao_t          *vao;

	ri.Printf(PRINT_ALL, "------- R_ShutdownVaos -------\n");

	R_BindNullVao();

	for(i = 0; i < tr.numVaos; i++)
	{
		vao = tr.vaos[i];

		if(vao->vao)
			qglDeleteVertexArraysARB(1, &vao->vao);

		if(vao->vertexesVBO)
		{
			qglDeleteBuffersARB(1, &vao->vertexesVBO);
		}

		if(vao->indexesIBO)
		{
			qglDeleteBuffersARB(1, &vao->indexesIBO);
		}
	}

	tr.numVaos = 0;
}

/*
============
R_VaoList_f
============
*/
void R_VaoList_f(void)
{
	int             i;
	vao_t          *vao;
	int             vertexesSize = 0;
	int             indexesSize = 0;

	ri.Printf(PRINT_ALL, " size          name\n");
	ri.Printf(PRINT_ALL, "----------------------------------------------------------\n");

	for(i = 0; i < tr.numVaos; i++)
	{
		vao = tr.vaos[i];

		ri.Printf(PRINT_ALL, "%d.%02d MB %s\n", vao->vertexesSize / (1024 * 1024),
				  (vao->vertexesSize % (1024 * 1024)) * 100 / (1024 * 1024), vao->name);

		vertexesSize += vao->vertexesSize;
	}

	for(i = 0; i < tr.numVaos; i++)
	{
		vao = tr.vaos[i];

		ri.Printf(PRINT_ALL, "%d.%02d MB %s\n", vao->indexesSize / (1024 * 1024),
				  (vao->indexesSize % (1024 * 1024)) * 100 / (1024 * 1024), vao->name);

		indexesSize += vao->indexesSize;
	}

	ri.Printf(PRINT_ALL, " %i total VAOs\n", tr.numVaos);
	ri.Printf(PRINT_ALL, " %d.%02d MB total vertices memory\n", vertexesSize / (1024 * 1024),
			  (vertexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
	ri.Printf(PRINT_ALL, " %d.%02d MB total triangle indices memory\n", indexesSize / (1024 * 1024),
			  (indexesSize % (1024 * 1024)) * 100 / (1024 * 1024));
}


/*
==============
RB_UpdateTessVao

Adapted from Tess_UpdateVBOs from xreal

Update the default VAO to replace the client side vertex arrays
==============
*/
void RB_UpdateTessVao(unsigned int attribBits)
{
	GLimp_LogComment("--- RB_UpdateTessVao ---\n");

	backEnd.pc.c_dynamicVaoDraws++;

	// update the default VAO
	if(tess.numVertexes > 0 && tess.numVertexes <= SHADER_MAX_VERTEXES && tess.numIndexes > 0 && tess.numIndexes <= SHADER_MAX_INDEXES)
	{
		int attribIndex;
		int attribUpload;

		R_BindVao(tess.vao);

		// orphan old vertex buffer so we don't stall on it
		qglBufferDataARB(GL_ARRAY_BUFFER_ARB, tess.vao->vertexesSize, NULL, GL_DYNAMIC_DRAW_ARB);

		// if nothing to set, set everything
		if(!(attribBits & ATTR_BITS))
			attribBits = ATTR_BITS;

		attribUpload = attribBits;

		if((attribUpload & ATTR_TEXCOORD) || (attribUpload & ATTR_LIGHTCOORD))
		{
			// these are interleaved, so we update both if either need it
			// this translates to updating ATTR_TEXCOORD twice as large as it needs
			attribUpload &= ~ATTR_LIGHTCOORD;
			attribUpload |= ATTR_TEXCOORD;
		}

		for (attribIndex = 0; attribIndex < ATTR_INDEX_COUNT; attribIndex++)
		{
			uint32_t attribBit = 1 << attribIndex;
			vaoAttrib_t *vAtb = &tess.vao->attribs[attribIndex];

			if (attribUpload & attribBit)
			{
				// note: tess has a VBO where stride == size
				qglBufferSubDataARB(GL_ARRAY_BUFFER_ARB, vAtb->offset, tess.numVertexes * vAtb->stride, tess.attribPointers[attribIndex]);
			}

			if (attribBits & attribBit)
			{
				if (!glRefConfig.vertexArrayObject)
					qglVertexAttribPointerARB(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));

				if (!(glState.vertexAttribsEnabled & attribBit))
				{
					qglEnableVertexAttribArrayARB(attribIndex);
					glState.vertexAttribsEnabled |= attribBit;
				}
			}
			else
			{
				if ((glState.vertexAttribsEnabled & attribBit))
				{
					qglDisableVertexAttribArrayARB(attribIndex);
					glState.vertexAttribsEnabled &= ~attribBit;
				}
			}
		}

		// orphan old index buffer so we don't stall on it
		qglBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, tess.vao->indexesSize, NULL, GL_DYNAMIC_DRAW_ARB);

		qglBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, tess.numIndexes * sizeof(tess.indexes[0]), tess.indexes);
	}
}
