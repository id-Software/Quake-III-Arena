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


void R_VaoPackTangent(int16_t *out, vec4_t v)
{
	out[0] = v[0] * 32767.0f + (v[0] > 0.0f ? 0.5f : -0.5f);
	out[1] = v[1] * 32767.0f + (v[1] > 0.0f ? 0.5f : -0.5f);
	out[2] = v[2] * 32767.0f + (v[2] > 0.0f ? 0.5f : -0.5f);
	out[3] = v[3] * 32767.0f + (v[3] > 0.0f ? 0.5f : -0.5f);
}

void R_VaoPackNormal(int16_t *out, vec3_t v)
{
	out[0] = v[0] * 32767.0f + (v[0] > 0.0f ? 0.5f : -0.5f);
	out[1] = v[1] * 32767.0f + (v[1] > 0.0f ? 0.5f : -0.5f);
	out[2] = v[2] * 32767.0f + (v[2] > 0.0f ? 0.5f : -0.5f);
	out[3] = 0;
}

void R_VaoPackColor(uint16_t *out, vec4_t c)
{
	out[0] = c[0] * 65535.0f + 0.5f;
	out[1] = c[1] * 65535.0f + 0.5f;
	out[2] = c[2] * 65535.0f + 0.5f;
	out[3] = c[3] * 65535.0f + 0.5f;
}

void R_VaoUnpackTangent(vec4_t v, int16_t *pack)
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
	v[3] = pack[3] / 32767.0f;
}

void R_VaoUnpackNormal(vec3_t v, int16_t *pack)
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
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
			qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));
			if (glRefConfig.vertexArrayObject || !(glState.vertexAttribsEnabled & attribBit))
				qglEnableVertexAttribArray(attribIndex);

			if (!glRefConfig.vertexArrayObject || vao == tess.vao)
				glState.vertexAttribsEnabled |= attribBit;
		}
		else
		{
			// don't disable vertex attribs when using vertex array objects
			// Vao_SetVertexPointers is only called during init when using VAOs, and vertex attribs start disabled anyway
			if (!glRefConfig.vertexArrayObject && (glState.vertexAttribsEnabled & attribBit))
				qglDisableVertexAttribArray(attribIndex);

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
			glUsage = GL_STATIC_DRAW;
			break;

		case VAO_USAGE_DYNAMIC:
			glUsage = GL_DYNAMIC_DRAW;
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
		qglGenVertexArrays(1, &vao->vao);
		qglBindVertexArray(vao->vao);
	}


	vao->vertexesSize = vertexesSize;

	qglGenBuffers(1, &vao->vertexesVBO);

	qglBindBuffer(GL_ARRAY_BUFFER, vao->vertexesVBO);
	qglBufferData(GL_ARRAY_BUFFER, vertexesSize, vertexes, glUsage);


	vao->indexesSize = indexesSize;

	qglGenBuffers(1, &vao->indexesIBO);

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesSize, indexes, glUsage);


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

	int				glUsage = GL_STATIC_DRAW;

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
	vao->attribs[ATTR_INDEX_TANGENT       ].enabled = 1;
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
	vao->attribs[ATTR_INDEX_NORMAL        ].type = GL_SHORT;
	vao->attribs[ATTR_INDEX_TANGENT       ].type = GL_SHORT;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].type = GL_FLOAT;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].type = GL_FLOAT;
	vao->attribs[ATTR_INDEX_COLOR         ].type = GL_UNSIGNED_SHORT;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].type = GL_SHORT;

	vao->attribs[ATTR_INDEX_POSITION      ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_NORMAL        ].normalized = GL_TRUE;
	vao->attribs[ATTR_INDEX_TANGENT       ].normalized = GL_TRUE;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].normalized = GL_FALSE;
	vao->attribs[ATTR_INDEX_COLOR         ].normalized = GL_TRUE;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].normalized = GL_TRUE;

	vao->attribs[ATTR_INDEX_POSITION      ].offset = 0;        dataSize  = sizeof(verts[0].xyz);
	vao->attribs[ATTR_INDEX_NORMAL        ].offset = dataSize; dataSize += sizeof(verts[0].normal);
	vao->attribs[ATTR_INDEX_TANGENT       ].offset = dataSize; dataSize += sizeof(verts[0].tangent);
	vao->attribs[ATTR_INDEX_TEXCOORD      ].offset = dataSize; dataSize += sizeof(verts[0].st);
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].offset = dataSize; dataSize += sizeof(verts[0].lightmap);
	vao->attribs[ATTR_INDEX_COLOR         ].offset = dataSize; dataSize += sizeof(verts[0].color);
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].offset = dataSize; dataSize += sizeof(verts[0].lightdir);

	vao->attribs[ATTR_INDEX_POSITION      ].stride = dataSize;
	vao->attribs[ATTR_INDEX_NORMAL        ].stride = dataSize;
	vao->attribs[ATTR_INDEX_TANGENT       ].stride = dataSize;
	vao->attribs[ATTR_INDEX_TEXCOORD      ].stride = dataSize;
	vao->attribs[ATTR_INDEX_LIGHTCOORD    ].stride = dataSize;
	vao->attribs[ATTR_INDEX_COLOR         ].stride = dataSize;
	vao->attribs[ATTR_INDEX_LIGHTDIRECTION].stride = dataSize;


	if (glRefConfig.vertexArrayObject)
	{
		qglGenVertexArrays(1, &vao->vao);
		qglBindVertexArray(vao->vao);
	}


	// create VBO
	dataSize *= numVertexes;
	data = ri.Hunk_AllocateTempMemory(dataSize);
	dataOfs = 0;

	for (i = 0; i < numVertexes; i++)
	{
		// xyz
		memcpy(data + dataOfs, &verts[i].xyz, sizeof(verts[i].xyz));
		dataOfs += sizeof(verts[i].xyz);

		// normal
		memcpy(data + dataOfs, &verts[i].normal, sizeof(verts[i].normal));
		dataOfs += sizeof(verts[i].normal);

		// tangent
		memcpy(data + dataOfs, &verts[i].tangent, sizeof(verts[i].tangent));
		dataOfs += sizeof(verts[i].tangent);

		// texcoords
		memcpy(data + dataOfs, &verts[i].st, sizeof(verts[i].st));
		dataOfs += sizeof(verts[i].st);

		// lightmap texcoords
		memcpy(data + dataOfs, &verts[i].lightmap, sizeof(verts[i].lightmap));
		dataOfs += sizeof(verts[i].lightmap);

		// colors
		memcpy(data + dataOfs, &verts[i].color, sizeof(verts[i].color));
		dataOfs += sizeof(verts[i].color);

		// light directions
		memcpy(data + dataOfs, &verts[i].lightdir, sizeof(verts[i].lightdir));
		dataOfs += sizeof(verts[i].lightdir);
	}

	vao->vertexesSize = dataSize;

	qglGenBuffers(1, &vao->vertexesVBO);

	qglBindBuffer(GL_ARRAY_BUFFER, vao->vertexesVBO);
	qglBufferData(GL_ARRAY_BUFFER, vao->vertexesSize, data, glUsage);


	// create IBO
	vao->indexesSize = numIndexes * sizeof(glIndex_t);

	qglGenBuffers(1, &vao->indexesIBO);

	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, vao->indexesSize, indexes, glUsage);


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
			qglBindVertexArray(vao->vao);

			// Intel Graphics doesn't save GL_ELEMENT_ARRAY_BUFFER binding with VAO binding.
			if (glRefConfig.intelGraphics || vao == tess.vao)
				qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO);

			// tess VAO always has buffers bound
			if (vao == tess.vao)
				qglBindBuffer(GL_ARRAY_BUFFER, vao->vertexesVBO);
		}
		else
		{
			qglBindBuffer(GL_ARRAY_BUFFER, vao->vertexesVBO);
			qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->indexesIBO);

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
			qglBindVertexArray(0);

			// why you no save GL_ELEMENT_ARRAY_BUFFER binding, Intel?
			if (1) qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		else
		{
			qglBindBuffer(GL_ARRAY_BUFFER, 0);
			qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
	vertexesSize += sizeof(tess.tangent[0]);
	vertexesSize += sizeof(tess.color[0]);
	vertexesSize += sizeof(tess.texCoords[0]);
	vertexesSize += sizeof(tess.lightCoords[0]);
	vertexesSize += sizeof(tess.lightdir[0]);
	vertexesSize *= SHADER_MAX_VERTEXES;

	indexesSize = sizeof(tess.indexes[0]) * SHADER_MAX_INDEXES;

	tess.vao = R_CreateVao("tessVertexArray_VAO", NULL, vertexesSize, NULL, indexesSize, VAO_USAGE_DYNAMIC);

	offset = 0;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].enabled = 1;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].enabled = 1;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].enabled = 1;
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
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].type = GL_SHORT;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].type = GL_SHORT;
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].type = GL_FLOAT;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].type = GL_FLOAT;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].type = GL_UNSIGNED_SHORT;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].type = GL_SHORT;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].normalized = GL_TRUE;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].normalized = GL_TRUE;
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].normalized = GL_FALSE;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].normalized = GL_TRUE;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].normalized = GL_TRUE;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].offset = offset; offset += sizeof(tess.xyz[0])         * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].offset = offset; offset += sizeof(tess.normal[0])      * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].offset = offset; offset += sizeof(tess.tangent[0])     * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].offset = offset; offset += sizeof(tess.texCoords[0])   * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].offset = offset; offset += sizeof(tess.lightCoords[0]) * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_COLOR         ].offset = offset; offset += sizeof(tess.color[0])       * SHADER_MAX_VERTEXES;
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].offset = offset;

	tess.vao->attribs[ATTR_INDEX_POSITION      ].stride = sizeof(tess.xyz[0]);
	tess.vao->attribs[ATTR_INDEX_NORMAL        ].stride = sizeof(tess.normal[0]);
	tess.vao->attribs[ATTR_INDEX_TANGENT       ].stride = sizeof(tess.tangent[0]);
	tess.vao->attribs[ATTR_INDEX_TEXCOORD      ].stride = sizeof(tess.texCoords[0]);
	tess.vao->attribs[ATTR_INDEX_LIGHTCOORD    ].stride = sizeof(tess.lightCoords[0]);
	tess.vao->attribs[ATTR_INDEX_COLOR         ].stride = sizeof(tess.color[0]);
	tess.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].stride = sizeof(tess.lightdir[0]);

	tess.attribPointers[ATTR_INDEX_POSITION]       = tess.xyz;
	tess.attribPointers[ATTR_INDEX_NORMAL]         = tess.normal;
	tess.attribPointers[ATTR_INDEX_TANGENT]        = tess.tangent;
	tess.attribPointers[ATTR_INDEX_TEXCOORD]       = tess.texCoords;
	tess.attribPointers[ATTR_INDEX_LIGHTCOORD]     = tess.lightCoords;
	tess.attribPointers[ATTR_INDEX_COLOR]          = tess.color;
	tess.attribPointers[ATTR_INDEX_LIGHTDIRECTION] = tess.lightdir;

	Vao_SetVertexPointers(tess.vao);

	R_BindNullVao();

	VaoCache_Init();

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
			qglDeleteVertexArrays(1, &vao->vao);

		if(vao->vertexesVBO)
		{
			qglDeleteBuffers(1, &vao->vertexesVBO);
		}

		if(vao->indexesIBO)
		{
			qglDeleteBuffers(1, &vao->indexesIBO);
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
		qglBufferData(GL_ARRAY_BUFFER, tess.vao->vertexesSize, NULL, GL_DYNAMIC_DRAW);

		// if nothing to set, set everything
		if(!(attribBits & ATTR_BITS))
			attribBits = ATTR_BITS;

		attribUpload = attribBits;

		for (attribIndex = 0; attribIndex < ATTR_INDEX_COUNT; attribIndex++)
		{
			uint32_t attribBit = 1 << attribIndex;
			vaoAttrib_t *vAtb = &tess.vao->attribs[attribIndex];

			if (attribUpload & attribBit)
			{
				// note: tess has a VBO where stride == size
				qglBufferSubData(GL_ARRAY_BUFFER, vAtb->offset, tess.numVertexes * vAtb->stride, tess.attribPointers[attribIndex]);
			}

			if (attribBits & attribBit)
			{
				if (!glRefConfig.vertexArrayObject)
					qglVertexAttribPointer(attribIndex, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, BUFFER_OFFSET(vAtb->offset));

				if (!(glState.vertexAttribsEnabled & attribBit))
				{
					qglEnableVertexAttribArray(attribIndex);
					glState.vertexAttribsEnabled |= attribBit;
				}
			}
			else
			{
				if ((glState.vertexAttribsEnabled & attribBit))
				{
					qglDisableVertexAttribArray(attribIndex);
					glState.vertexAttribsEnabled &= ~attribBit;
				}
			}
		}

		// orphan old index buffer so we don't stall on it
		qglBufferData(GL_ELEMENT_ARRAY_BUFFER, tess.vao->indexesSize, NULL, GL_DYNAMIC_DRAW);

		qglBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, tess.numIndexes * sizeof(tess.indexes[0]), tess.indexes);
	}
}

// FIXME: This sets a limit of 65536 verts/262144 indexes per static surface
// This is higher than the old vq3 limits but is worth noting
#define VAOCACHE_QUEUE_MAX_SURFACES (1 << 10)
#define VAOCACHE_QUEUE_MAX_VERTEXES (1 << 16)
#define VAOCACHE_QUEUE_MAX_INDEXES (VAOCACHE_QUEUE_MAX_VERTEXES * 4)

typedef struct queuedSurface_s
{
	srfVert_t *vertexes;
	int numVerts;
	glIndex_t *indexes;
	int numIndexes;
}
queuedSurface_t;

static struct
{
	queuedSurface_t surfaces[VAOCACHE_QUEUE_MAX_SURFACES];
	int numSurfaces;

	srfVert_t vertexes[VAOCACHE_QUEUE_MAX_VERTEXES];
	int vertexCommitSize;

	glIndex_t indexes[VAOCACHE_QUEUE_MAX_INDEXES];
	int indexCommitSize;
}
vcq;

#define VAOCACHE_MAX_SURFACES (1 << 16)
#define VAOCACHE_MAX_BATCHES (1 << 10)

// srfVert_t is 60 bytes
// assuming each vert is referenced 4 times, need 16 bytes (4 glIndex_t) per vert
// -> need about 4/15ths the space for indexes as vertexes
#define VAOCACHE_VERTEX_BUFFER_SIZE (16 * 1024 * 1024)
#define VAOCACHE_INDEX_BUFFER_SIZE (5 * 1024 * 1024)

typedef struct buffered_s
{
	void *data;
	int size;
	int bufferOffset;
}
buffered_t;

static struct
{
	vao_t *vao;
	buffered_t surfaceIndexSets[VAOCACHE_MAX_SURFACES];
	int numSurfaces;

	int batchLengths[VAOCACHE_MAX_BATCHES];
	int numBatches;

	int vertexOffset;
	int indexOffset;
}
vc;

void VaoCache_Commit(void)
{
	buffered_t *indexSet;
	int *batchLength;
	queuedSurface_t *surf, *end = vcq.surfaces + vcq.numSurfaces;

	R_BindVao(vc.vao);

	// Search for a matching batch
	// FIXME: Use faster search
	indexSet = vc.surfaceIndexSets;
	batchLength = vc.batchLengths;
	for (; batchLength < vc.batchLengths + vc.numBatches; batchLength++)
	{
		if (*batchLength == vcq.numSurfaces)
		{
			buffered_t *indexSet2 = indexSet;
			for (surf = vcq.surfaces; surf < end; surf++, indexSet2++)
			{
				if (surf->indexes != indexSet2->data || (surf->numIndexes * sizeof(glIndex_t)) != indexSet2->size)
					break;
			}

			if (surf == end)
				break;
		}

		indexSet += *batchLength;
	}

	// If found, use it
	if (indexSet < vc.surfaceIndexSets + vc.numSurfaces)
	{
		tess.firstIndex = indexSet->bufferOffset / sizeof(glIndex_t);
		//ri.Printf(PRINT_ALL, "firstIndex %d numIndexes %d as %d\n", tess.firstIndex, tess.numIndexes, batchLength - vc.batchLengths);
		//ri.Printf(PRINT_ALL, "vc.numSurfaces %d vc.numBatches %d\n", vc.numSurfaces, vc.numBatches);
	}
	// If not, rebuffer the batch
	// FIXME: keep track of the vertexes so we don't have to reupload them every time
	else
	{
		srfVert_t *dstVertex = vcq.vertexes;
		glIndex_t *dstIndex = vcq.indexes;

		batchLength = vc.batchLengths + vc.numBatches;
		*batchLength = vcq.numSurfaces;
		vc.numBatches++;

		tess.firstIndex = vc.indexOffset / sizeof(glIndex_t);
		vcq.vertexCommitSize = 0;
		vcq.indexCommitSize = 0;
		for (surf = vcq.surfaces; surf < end; surf++)
		{
			glIndex_t *srcIndex = surf->indexes;
			int vertexesSize = surf->numVerts * sizeof(srfVert_t);
			int indexesSize = surf->numIndexes * sizeof(glIndex_t);
			int i, indexOffset = (vc.vertexOffset + vcq.vertexCommitSize) / sizeof(srfVert_t);

			Com_Memcpy(dstVertex, surf->vertexes, vertexesSize);
			dstVertex += surf->numVerts;

			vcq.vertexCommitSize += vertexesSize;

			indexSet = vc.surfaceIndexSets + vc.numSurfaces;
			indexSet->data = surf->indexes;
			indexSet->size = indexesSize;
			indexSet->bufferOffset = vc.indexOffset + vcq.indexCommitSize;
			vc.numSurfaces++;

			for (i = 0; i < surf->numIndexes; i++)
				*dstIndex++ = *srcIndex++ + indexOffset;

			vcq.indexCommitSize += indexesSize;
		}

		//ri.Printf(PRINT_ALL, "committing %d to %d, %d to %d as %d\n", vcq.vertexCommitSize, vc.vertexOffset, vcq.indexCommitSize, vc.indexOffset, batchLength - vc.batchLengths);

		if (vcq.vertexCommitSize)
		{
			qglBindBuffer(GL_ARRAY_BUFFER, vc.vao->vertexesVBO);
			qglBufferSubData(GL_ARRAY_BUFFER, vc.vertexOffset, vcq.vertexCommitSize, vcq.vertexes);
			vc.vertexOffset += vcq.vertexCommitSize;
		}

		if (vcq.indexCommitSize)
		{
			qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vc.vao->indexesIBO);
			qglBufferSubData(GL_ELEMENT_ARRAY_BUFFER, vc.indexOffset, vcq.indexCommitSize, vcq.indexes);
			vc.indexOffset += vcq.indexCommitSize;
		}
	}
}

void VaoCache_Init(void)
{
	srfVert_t vert;
	int dataSize;

	vc.vao = R_CreateVao("VaoCache", NULL, VAOCACHE_VERTEX_BUFFER_SIZE, NULL, VAOCACHE_INDEX_BUFFER_SIZE, VAO_USAGE_DYNAMIC);

	vc.vao->attribs[ATTR_INDEX_POSITION].enabled       = 1;
	vc.vao->attribs[ATTR_INDEX_TEXCOORD].enabled       = 1;
	vc.vao->attribs[ATTR_INDEX_LIGHTCOORD].enabled     = 1;
	vc.vao->attribs[ATTR_INDEX_NORMAL].enabled         = 1;
	vc.vao->attribs[ATTR_INDEX_TANGENT].enabled        = 1;
	vc.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].enabled = 1;
	vc.vao->attribs[ATTR_INDEX_COLOR].enabled          = 1;

	vc.vao->attribs[ATTR_INDEX_POSITION].count       = 3;
	vc.vao->attribs[ATTR_INDEX_TEXCOORD].count       = 2;
	vc.vao->attribs[ATTR_INDEX_LIGHTCOORD].count     = 2;
	vc.vao->attribs[ATTR_INDEX_NORMAL].count         = 4;
	vc.vao->attribs[ATTR_INDEX_TANGENT].count        = 4;
	vc.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].count = 4;
	vc.vao->attribs[ATTR_INDEX_COLOR].count          = 4;

	vc.vao->attribs[ATTR_INDEX_POSITION].type             = GL_FLOAT;
	vc.vao->attribs[ATTR_INDEX_TEXCOORD].type             = GL_FLOAT;
	vc.vao->attribs[ATTR_INDEX_LIGHTCOORD].type           = GL_FLOAT;
	vc.vao->attribs[ATTR_INDEX_NORMAL].type               = GL_SHORT;
	vc.vao->attribs[ATTR_INDEX_TANGENT].type              = GL_SHORT;
	vc.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].type       = GL_SHORT;
	vc.vao->attribs[ATTR_INDEX_COLOR].type                = GL_UNSIGNED_SHORT;

	vc.vao->attribs[ATTR_INDEX_POSITION].normalized       = GL_FALSE;
	vc.vao->attribs[ATTR_INDEX_TEXCOORD].normalized       = GL_FALSE;
	vc.vao->attribs[ATTR_INDEX_LIGHTCOORD].normalized     = GL_FALSE;
	vc.vao->attribs[ATTR_INDEX_NORMAL].normalized         = GL_TRUE;
	vc.vao->attribs[ATTR_INDEX_TANGENT].normalized        = GL_TRUE;
	vc.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].normalized = GL_TRUE;
	vc.vao->attribs[ATTR_INDEX_COLOR].normalized          = GL_TRUE;

	vc.vao->attribs[ATTR_INDEX_POSITION].offset       = 0;        dataSize  = sizeof(vert.xyz);
	vc.vao->attribs[ATTR_INDEX_TEXCOORD].offset       = dataSize; dataSize += sizeof(vert.st);
	vc.vao->attribs[ATTR_INDEX_LIGHTCOORD].offset     = dataSize; dataSize += sizeof(vert.lightmap);
	vc.vao->attribs[ATTR_INDEX_NORMAL].offset         = dataSize; dataSize += sizeof(vert.normal);
	vc.vao->attribs[ATTR_INDEX_TANGENT].offset        = dataSize; dataSize += sizeof(vert.tangent);
	vc.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].offset = dataSize; dataSize += sizeof(vert.lightdir);
	vc.vao->attribs[ATTR_INDEX_COLOR].offset          = dataSize; dataSize += sizeof(vert.color);

	vc.vao->attribs[ATTR_INDEX_POSITION].stride       = dataSize;
	vc.vao->attribs[ATTR_INDEX_TEXCOORD].stride       = dataSize;
	vc.vao->attribs[ATTR_INDEX_LIGHTCOORD].stride     = dataSize;
	vc.vao->attribs[ATTR_INDEX_NORMAL].stride         = dataSize;
	vc.vao->attribs[ATTR_INDEX_TANGENT].stride        = dataSize;
	vc.vao->attribs[ATTR_INDEX_LIGHTDIRECTION].stride = dataSize;
	vc.vao->attribs[ATTR_INDEX_COLOR].stride          = dataSize;

	Vao_SetVertexPointers(vc.vao);

	vc.numSurfaces = 0;
	vc.numBatches = 0;
	vc.vertexOffset = 0;
	vc.indexOffset = 0;
	vcq.vertexCommitSize = 0;
	vcq.indexCommitSize = 0;
	vcq.numSurfaces = 0;
}

void VaoCache_BindVao(void)
{
	R_BindVao(vc.vao);
}

void VaoCache_CheckAdd(qboolean *endSurface, qboolean *recycleVertexBuffer, qboolean *recycleIndexBuffer, int numVerts, int numIndexes)
{
	int vertexesSize = sizeof(srfVert_t) * numVerts;
	int indexesSize = sizeof(glIndex_t) * numIndexes;

	if (vc.vao->vertexesSize < vc.vertexOffset + vcq.vertexCommitSize + vertexesSize)
	{
		//ri.Printf(PRINT_ALL, "out of space in vertex cache: %d < %d + %d + %d\n", vc.vao->vertexesSize, vc.vertexOffset, vc.vertexCommitSize, vertexesSize);
		*recycleVertexBuffer = qtrue;
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vc.vao->indexesSize < vc.indexOffset + vcq.indexCommitSize + indexesSize)
	{
		//ri.Printf(PRINT_ALL, "out of space in index cache\n");
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vc.numSurfaces + vcq.numSurfaces >= VAOCACHE_MAX_SURFACES)
	{
		//ri.Printf(PRINT_ALL, "out of surfaces in index cache\n");
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vc.numBatches >= VAOCACHE_MAX_BATCHES)
	{
		//ri.Printf(PRINT_ALL, "out of batches in index cache\n");
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vcq.numSurfaces >= VAOCACHE_QUEUE_MAX_SURFACES)
	{
		//ri.Printf(PRINT_ALL, "out of queued surfaces\n");
		*endSurface = qtrue;
	}

	if (VAOCACHE_QUEUE_MAX_VERTEXES * sizeof(srfVert_t) < vcq.vertexCommitSize + vertexesSize)
	{
		//ri.Printf(PRINT_ALL, "out of queued vertexes\n");
		*endSurface = qtrue;
	}

	if (VAOCACHE_QUEUE_MAX_INDEXES * sizeof(glIndex_t) < vcq.indexCommitSize + indexesSize)
	{
		//ri.Printf(PRINT_ALL, "out of queued indexes\n");
		*endSurface = qtrue;
	}
}

void VaoCache_RecycleVertexBuffer(void)
{
	qglBindBuffer(GL_ARRAY_BUFFER, vc.vao->vertexesVBO);
	qglBufferData(GL_ARRAY_BUFFER, vc.vao->vertexesSize, NULL, GL_DYNAMIC_DRAW);
	vc.vertexOffset = 0;
}

void VaoCache_RecycleIndexBuffer(void)
{
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vc.vao->indexesIBO);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, vc.vao->indexesSize, NULL, GL_DYNAMIC_DRAW);
	vc.indexOffset = 0;
	vc.numSurfaces = 0;
	vc.numBatches = 0;
}

void VaoCache_InitQueue(void)
{
	vcq.vertexCommitSize = 0;
	vcq.indexCommitSize = 0;
	vcq.numSurfaces = 0;
}

void VaoCache_AddSurface(srfVert_t *verts, int numVerts, glIndex_t *indexes, int numIndexes)
{
	queuedSurface_t *queueEntry = vcq.surfaces + vcq.numSurfaces;
	queueEntry->vertexes = verts;
	queueEntry->numVerts = numVerts;
	queueEntry->indexes = indexes;
	queueEntry->numIndexes = numIndexes;
	vcq.numSurfaces++;

	vcq.vertexCommitSize += sizeof(srfVert_t) * numVerts;;
	vcq.indexCommitSize += sizeof(glIndex_t) * numIndexes;
}
