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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "l_cmd.h"
#include "l_math.h"
#include "l_mem.h"
#include "l_log.h"
#include "../botlib/l_script.h"
#include "l_bsp_q1.h"
#include "l_bsp_ent.h"

//=============================================================================

int				q1_nummodels;
q1_dmodel_t		*q1_dmodels;//[MAX_MAP_MODELS];

int				q1_visdatasize;
byte				*q1_dvisdata;//[MAX_MAP_VISIBILITY];

int				q1_lightdatasize;
byte				*q1_dlightdata;//[MAX_MAP_LIGHTING];

int				q1_texdatasize;
byte				*q1_dtexdata;//[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

int				q1_entdatasize;
char				*q1_dentdata;//[MAX_MAP_ENTSTRING];

int				q1_numleafs;
q1_dleaf_t		*q1_dleafs;//[MAX_MAP_LEAFS];

int				q1_numplanes;
q1_dplane_t		*q1_dplanes;//[MAX_MAP_PLANES];

int				q1_numvertexes;
q1_dvertex_t	*q1_dvertexes;//[MAX_MAP_VERTS];

int				q1_numnodes;
q1_dnode_t		*q1_dnodes;//[MAX_MAP_NODES];

int				q1_numtexinfo;
q1_texinfo_t	*q1_texinfo;//[MAX_MAP_TEXINFO];

int				q1_numfaces;
q1_dface_t		*q1_dfaces;//[MAX_MAP_FACES];

int				q1_numclipnodes;
q1_dclipnode_t	*q1_dclipnodes;//[MAX_MAP_CLIPNODES];

int				q1_numedges;
q1_dedge_t		*q1_dedges;//[MAX_MAP_EDGES];

int				q1_nummarksurfaces;
unsigned short	*q1_dmarksurfaces;//[MAX_MAP_MARKSURFACES];

int				q1_numsurfedges;
int				*q1_dsurfedges;//[MAX_MAP_SURFEDGES];

//=============================================================================

int q1_bspallocated = false;
int q1_allocatedbspmem = 0;

void Q1_AllocMaxBSP(void)
{
	//models
	q1_nummodels = 0;
	q1_dmodels = (q1_dmodel_t *) GetMemory(Q1_MAX_MAP_MODELS * sizeof(q1_dmodel_t));
	q1_allocatedbspmem = Q1_MAX_MAP_MODELS * sizeof(q1_dmodel_t);
	//visibility
	q1_visdatasize = 0;
	q1_dvisdata = (byte *) GetMemory(Q1_MAX_MAP_VISIBILITY * sizeof(byte));
	q1_allocatedbspmem += Q1_MAX_MAP_VISIBILITY * sizeof(byte);
	//light data
	q1_lightdatasize = 0;
	q1_dlightdata = (byte *) GetMemory(Q1_MAX_MAP_LIGHTING * sizeof(byte));
	q1_allocatedbspmem += Q1_MAX_MAP_LIGHTING * sizeof(byte);
	//texture data
	q1_texdatasize = 0;
	q1_dtexdata = (byte *) GetMemory(Q1_MAX_MAP_MIPTEX * sizeof(byte)); // (dmiptexlump_t)
	q1_allocatedbspmem += Q1_MAX_MAP_MIPTEX * sizeof(byte);
	//entities
	q1_entdatasize = 0;
	q1_dentdata = (char *) GetMemory(Q1_MAX_MAP_ENTSTRING * sizeof(char));
	q1_allocatedbspmem += Q1_MAX_MAP_ENTSTRING * sizeof(char);
	//leaves
	q1_numleafs = 0;
	q1_dleafs = (q1_dleaf_t *) GetMemory(Q1_MAX_MAP_LEAFS * sizeof(q1_dleaf_t));
	q1_allocatedbspmem += Q1_MAX_MAP_LEAFS * sizeof(q1_dleaf_t);
	//planes
	q1_numplanes = 0;
	q1_dplanes = (q1_dplane_t *) GetMemory(Q1_MAX_MAP_PLANES * sizeof(q1_dplane_t));
	q1_allocatedbspmem += Q1_MAX_MAP_PLANES * sizeof(q1_dplane_t);
	//vertexes
	q1_numvertexes = 0;
	q1_dvertexes = (q1_dvertex_t *) GetMemory(Q1_MAX_MAP_VERTS * sizeof(q1_dvertex_t));
	q1_allocatedbspmem += Q1_MAX_MAP_VERTS * sizeof(q1_dvertex_t);
	//nodes
	q1_numnodes = 0;
	q1_dnodes = (q1_dnode_t *) GetMemory(Q1_MAX_MAP_NODES * sizeof(q1_dnode_t));
	q1_allocatedbspmem += Q1_MAX_MAP_NODES * sizeof(q1_dnode_t);
	//texture info
	q1_numtexinfo = 0;
	q1_texinfo = (q1_texinfo_t *) GetMemory(Q1_MAX_MAP_TEXINFO * sizeof(q1_texinfo_t));
	q1_allocatedbspmem += Q1_MAX_MAP_TEXINFO * sizeof(q1_texinfo_t);
	//faces
	q1_numfaces = 0;
	q1_dfaces = (q1_dface_t *) GetMemory(Q1_MAX_MAP_FACES * sizeof(q1_dface_t));
	q1_allocatedbspmem += Q1_MAX_MAP_FACES * sizeof(q1_dface_t);
	//clip nodes
	q1_numclipnodes = 0;
	q1_dclipnodes = (q1_dclipnode_t *) GetMemory(Q1_MAX_MAP_CLIPNODES * sizeof(q1_dclipnode_t));
	q1_allocatedbspmem += Q1_MAX_MAP_CLIPNODES * sizeof(q1_dclipnode_t);
	//edges
	q1_numedges = 0;
	q1_dedges = (q1_dedge_t *) GetMemory(Q1_MAX_MAP_EDGES * sizeof(q1_dedge_t));
	q1_allocatedbspmem += Q1_MAX_MAP_EDGES, sizeof(q1_dedge_t);
	//mark surfaces
	q1_nummarksurfaces = 0;
	q1_dmarksurfaces = (unsigned short *) GetMemory(Q1_MAX_MAP_MARKSURFACES * sizeof(unsigned short));
	q1_allocatedbspmem += Q1_MAX_MAP_MARKSURFACES * sizeof(unsigned short);
	//surface edges
	q1_numsurfedges = 0;
	q1_dsurfedges = (int *) GetMemory(Q1_MAX_MAP_SURFEDGES * sizeof(int));
	q1_allocatedbspmem += Q1_MAX_MAP_SURFEDGES * sizeof(int);
	//print allocated memory
	Log_Print("allocated ");
	PrintMemorySize(q1_allocatedbspmem);
	Log_Print(" of BSP memory\n");
} //end of the function Q1_AllocMaxBSP

void Q1_FreeMaxBSP(void)
{
	//models
	q1_nummodels = 0;
	FreeMemory(q1_dmodels);
	q1_dmodels = NULL;
	//visibility
	q1_visdatasize = 0;
	FreeMemory(q1_dvisdata);
	q1_dvisdata = NULL;
	//light data
	q1_lightdatasize = 0;
	FreeMemory(q1_dlightdata);
	q1_dlightdata = NULL;
	//texture data
	q1_texdatasize = 0;
	FreeMemory(q1_dtexdata);
	q1_dtexdata = NULL;
	//entities
	q1_entdatasize = 0;
	FreeMemory(q1_dentdata);
	q1_dentdata = NULL;
	//leaves
	q1_numleafs = 0;
	FreeMemory(q1_dleafs);
	q1_dleafs = NULL;
	//planes
	q1_numplanes = 0;
	FreeMemory(q1_dplanes);
	q1_dplanes = NULL;
	//vertexes
	q1_numvertexes = 0;
	FreeMemory(q1_dvertexes);
	q1_dvertexes = NULL;
	//nodes
	q1_numnodes = 0;
	FreeMemory(q1_dnodes);
	q1_dnodes = NULL;
	//texture info
	q1_numtexinfo = 0;
	FreeMemory(q1_texinfo);
	q1_texinfo = NULL;
	//faces
	q1_numfaces = 0;
	FreeMemory(q1_dfaces);
	q1_dfaces = NULL;
	//clip nodes
	q1_numclipnodes = 0;
	FreeMemory(q1_dclipnodes);
	q1_dclipnodes = NULL;
	//edges
	q1_numedges = 0;
	FreeMemory(q1_dedges);
	q1_dedges = NULL;
	//mark surfaces
	q1_nummarksurfaces = 0;
	FreeMemory(q1_dmarksurfaces);
	q1_dmarksurfaces = NULL;
	//surface edges
	q1_numsurfedges = 0;
	FreeMemory(q1_dsurfedges);
	q1_dsurfedges = NULL;
	//
	Log_Print("freed ");
	PrintMemorySize(q1_allocatedbspmem);
	Log_Print(" of BSP memory\n");
	q1_allocatedbspmem = 0;
} //end of the function Q1_FreeMaxBSP
//#endif //ME

/*
=============
Q1_SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void Q1_SwapBSPFile (qboolean todisk)
{
	int i, j, c;
	q1_dmodel_t *d;
	q1_dmiptexlump_t *mtl;

	
// models	
	for (i=0 ; i<q1_nummodels ; i++)
	{
		d = &q1_dmodels[i];

		for (j=0 ; j<Q1_MAX_MAP_HULLS ; j++)
			d->headnode[j] = LittleLong (d->headnode[j]);

		d->visleafs = LittleLong (d->visleafs);
		d->firstface = LittleLong (d->firstface);
		d->numfaces = LittleLong (d->numfaces);
		
		for (j=0 ; j<3 ; j++)
		{
			d->mins[j] = LittleFloat(d->mins[j]);
			d->maxs[j] = LittleFloat(d->maxs[j]);
			d->origin[j] = LittleFloat(d->origin[j]);
		}
	}

//
// vertexes
//
	for (i=0 ; i<q1_numvertexes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			q1_dvertexes[i].point[j] = LittleFloat(q1_dvertexes[i].point[j]);
	}
		
//
// planes
//	
	for (i=0 ; i<q1_numplanes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			q1_dplanes[i].normal[j] = LittleFloat(q1_dplanes[i].normal[j]);
		q1_dplanes[i].dist = LittleFloat(q1_dplanes[i].dist);
		q1_dplanes[i].type = LittleLong(q1_dplanes[i].type);
	}
	
//
// texinfos
//	
	for (i=0 ; i<q1_numtexinfo ; i++)
	{
		for (j=0 ; j<8 ; j++)
			q1_texinfo[i].vecs[0][j] = LittleFloat(q1_texinfo[i].vecs[0][j]);
		q1_texinfo[i].miptex = LittleLong(q1_texinfo[i].miptex);
		q1_texinfo[i].flags = LittleLong(q1_texinfo[i].flags);
	}
	
//
// faces
//
	for (i=0 ; i<q1_numfaces ; i++)
	{
		q1_dfaces[i].texinfo = LittleShort(q1_dfaces[i].texinfo);
		q1_dfaces[i].planenum = LittleShort(q1_dfaces[i].planenum);
		q1_dfaces[i].side = LittleShort(q1_dfaces[i].side);
		q1_dfaces[i].lightofs = LittleLong(q1_dfaces[i].lightofs);
		q1_dfaces[i].firstedge = LittleLong(q1_dfaces[i].firstedge);
		q1_dfaces[i].numedges = LittleShort(q1_dfaces[i].numedges);
	}

//
// nodes
//
	for (i=0 ; i<q1_numnodes ; i++)
	{
		q1_dnodes[i].planenum = LittleLong(q1_dnodes[i].planenum);
		for (j=0 ; j<3 ; j++)
		{
			q1_dnodes[i].mins[j] = LittleShort(q1_dnodes[i].mins[j]);
			q1_dnodes[i].maxs[j] = LittleShort(q1_dnodes[i].maxs[j]);
		}
		q1_dnodes[i].children[0] = LittleShort(q1_dnodes[i].children[0]);
		q1_dnodes[i].children[1] = LittleShort(q1_dnodes[i].children[1]);
		q1_dnodes[i].firstface = LittleShort(q1_dnodes[i].firstface);
		q1_dnodes[i].numfaces = LittleShort(q1_dnodes[i].numfaces);
	}

//
// leafs
//
	for (i=0 ; i<q1_numleafs ; i++)
	{
		q1_dleafs[i].contents = LittleLong(q1_dleafs[i].contents);
		for (j=0 ; j<3 ; j++)
		{
			q1_dleafs[i].mins[j] = LittleShort(q1_dleafs[i].mins[j]);
			q1_dleafs[i].maxs[j] = LittleShort(q1_dleafs[i].maxs[j]);
		}

		q1_dleafs[i].firstmarksurface = LittleShort(q1_dleafs[i].firstmarksurface);
		q1_dleafs[i].nummarksurfaces = LittleShort(q1_dleafs[i].nummarksurfaces);
		q1_dleafs[i].visofs = LittleLong(q1_dleafs[i].visofs);
	}

//
// clipnodes
//
	for (i=0 ; i<q1_numclipnodes ; i++)
	{
		q1_dclipnodes[i].planenum = LittleLong(q1_dclipnodes[i].planenum);
		q1_dclipnodes[i].children[0] = LittleShort(q1_dclipnodes[i].children[0]);
		q1_dclipnodes[i].children[1] = LittleShort(q1_dclipnodes[i].children[1]);
	}

//
// miptex
//
	if (q1_texdatasize)
	{
		mtl = (q1_dmiptexlump_t *)q1_dtexdata;
		if (todisk)
			c = mtl->nummiptex;
		else
			c = LittleLong(mtl->nummiptex);
		mtl->nummiptex = LittleLong (mtl->nummiptex);
		for (i=0 ; i<c ; i++)
			mtl->dataofs[i] = LittleLong(mtl->dataofs[i]);
	}
	
//
// marksurfaces
//
	for (i=0 ; i<q1_nummarksurfaces ; i++)
		q1_dmarksurfaces[i] = LittleShort(q1_dmarksurfaces[i]);

//
// surfedges
//
	for (i=0 ; i<q1_numsurfedges ; i++)
		q1_dsurfedges[i] = LittleLong(q1_dsurfedges[i]);

//
// edges
//
	for (i=0 ; i<q1_numedges ; i++)
	{
		q1_dedges[i].v[0] = LittleShort(q1_dedges[i].v[0]);
		q1_dedges[i].v[1] = LittleShort(q1_dedges[i].v[1]);
	}
}


q1_dheader_t *q1_header;
int			q1_fileLength;

int Q1_CopyLump (int lump, void *dest, int size, int maxsize)
{
	int		length, ofs;

	length = q1_header->lumps[lump].filelen;
	ofs = q1_header->lumps[lump].fileofs;
	
	if (length % size) {
		Error ("LoadBSPFile: odd lump size");
	}
	// somehow things got out of range
	if ((length/size) > maxsize) {
		printf("WARNING: exceeded max size for lump %d size %d > maxsize %d\n", lump, (length/size), maxsize);
		length = maxsize * size;
	}
	if ( ofs + length > q1_fileLength ) {
		printf("WARNING: exceeded file length for lump %d\n", lump);
		length = q1_fileLength - ofs;
		if ( length <= 0 ) {
			return 0;
		}
	}

	memcpy (dest, (byte *)q1_header + ofs, length);

	return length / size;
}

/*
=============
Q1_LoadBSPFile
=============
*/
void	Q1_LoadBSPFile(char *filename, int offset, int length)
{
	int			i;
	
//
// load the file header
//
	q1_fileLength = LoadFile(filename, (void **)&q1_header, offset, length);

// swap the header
	for (i=0 ; i< sizeof(q1_dheader_t)/4 ; i++)
		((int *)q1_header)[i] = LittleLong ( ((int *)q1_header)[i]);

	if (q1_header->version != Q1_BSPVERSION)
		Error ("%s is version %i, not %i", filename, i, Q1_BSPVERSION);

	q1_nummodels = Q1_CopyLump (Q1_LUMP_MODELS, q1_dmodels, sizeof(q1_dmodel_t), Q1_MAX_MAP_MODELS );
	q1_numvertexes = Q1_CopyLump (Q1_LUMP_VERTEXES, q1_dvertexes, sizeof(q1_dvertex_t), Q1_MAX_MAP_VERTS );
	q1_numplanes = Q1_CopyLump (Q1_LUMP_PLANES, q1_dplanes, sizeof(q1_dplane_t), Q1_MAX_MAP_PLANES );
	q1_numleafs = Q1_CopyLump (Q1_LUMP_LEAFS, q1_dleafs, sizeof(q1_dleaf_t), Q1_MAX_MAP_LEAFS );
	q1_numnodes = Q1_CopyLump (Q1_LUMP_NODES, q1_dnodes, sizeof(q1_dnode_t), Q1_MAX_MAP_NODES );
	q1_numtexinfo = Q1_CopyLump (Q1_LUMP_TEXINFO, q1_texinfo, sizeof(q1_texinfo_t), Q1_MAX_MAP_TEXINFO );
	q1_numclipnodes = Q1_CopyLump (Q1_LUMP_CLIPNODES, q1_dclipnodes, sizeof(q1_dclipnode_t), Q1_MAX_MAP_CLIPNODES );
	q1_numfaces = Q1_CopyLump (Q1_LUMP_FACES, q1_dfaces, sizeof(q1_dface_t), Q1_MAX_MAP_FACES );
	q1_nummarksurfaces = Q1_CopyLump (Q1_LUMP_MARKSURFACES, q1_dmarksurfaces, sizeof(q1_dmarksurfaces[0]), Q1_MAX_MAP_MARKSURFACES );
	q1_numsurfedges = Q1_CopyLump (Q1_LUMP_SURFEDGES, q1_dsurfedges, sizeof(q1_dsurfedges[0]), Q1_MAX_MAP_SURFEDGES );
	q1_numedges = Q1_CopyLump (Q1_LUMP_EDGES, q1_dedges, sizeof(q1_dedge_t), Q1_MAX_MAP_EDGES );

	q1_texdatasize = Q1_CopyLump (Q1_LUMP_TEXTURES, q1_dtexdata, 1, Q1_MAX_MAP_MIPTEX );
	q1_visdatasize = Q1_CopyLump (Q1_LUMP_VISIBILITY, q1_dvisdata, 1, Q1_MAX_MAP_VISIBILITY );
	q1_lightdatasize = Q1_CopyLump (Q1_LUMP_LIGHTING, q1_dlightdata, 1, Q1_MAX_MAP_LIGHTING );
	q1_entdatasize = Q1_CopyLump (Q1_LUMP_ENTITIES, q1_dentdata, 1, Q1_MAX_MAP_ENTSTRING );

	FreeMemory(q1_header);		// everything has been copied out
		
//
// swap everything
//	
	Q1_SwapBSPFile (false);
}

//============================================================================

FILE *q1_wadfile;
q1_dheader_t q1_outheader;

void Q1_AddLump (int lumpnum, void *data, int len)
{
	q1_lump_t *lump;

	lump = &q1_header->lumps[lumpnum];
	
	lump->fileofs = LittleLong(ftell(q1_wadfile));
	lump->filelen = LittleLong(len);
	SafeWrite(q1_wadfile, data, (len+3)&~3);
}

/*
=============
Q1_WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void	Q1_WriteBSPFile (char *filename)
{		
	q1_header = &q1_outheader;
	memset (q1_header, 0, sizeof(q1_dheader_t));
	
	Q1_SwapBSPFile (true);

	q1_header->version = LittleLong (Q1_BSPVERSION);
	
	q1_wadfile = SafeOpenWrite (filename);
	SafeWrite (q1_wadfile, q1_header, sizeof(q1_dheader_t));	// overwritten later

	Q1_AddLump (Q1_LUMP_PLANES, q1_dplanes, q1_numplanes*sizeof(q1_dplane_t));
	Q1_AddLump (Q1_LUMP_LEAFS, q1_dleafs, q1_numleafs*sizeof(q1_dleaf_t));
	Q1_AddLump (Q1_LUMP_VERTEXES, q1_dvertexes, q1_numvertexes*sizeof(q1_dvertex_t));
	Q1_AddLump (Q1_LUMP_NODES, q1_dnodes, q1_numnodes*sizeof(q1_dnode_t));
	Q1_AddLump (Q1_LUMP_TEXINFO, q1_texinfo, q1_numtexinfo*sizeof(q1_texinfo_t));
	Q1_AddLump (Q1_LUMP_FACES, q1_dfaces, q1_numfaces*sizeof(q1_dface_t));
	Q1_AddLump (Q1_LUMP_CLIPNODES, q1_dclipnodes, q1_numclipnodes*sizeof(q1_dclipnode_t));
	Q1_AddLump (Q1_LUMP_MARKSURFACES, q1_dmarksurfaces, q1_nummarksurfaces*sizeof(q1_dmarksurfaces[0]));
	Q1_AddLump (Q1_LUMP_SURFEDGES, q1_dsurfedges, q1_numsurfedges*sizeof(q1_dsurfedges[0]));
	Q1_AddLump (Q1_LUMP_EDGES, q1_dedges, q1_numedges*sizeof(q1_dedge_t));
	Q1_AddLump (Q1_LUMP_MODELS, q1_dmodels, q1_nummodels*sizeof(q1_dmodel_t));

	Q1_AddLump (Q1_LUMP_LIGHTING, q1_dlightdata, q1_lightdatasize);
	Q1_AddLump (Q1_LUMP_VISIBILITY, q1_dvisdata, q1_visdatasize);
	Q1_AddLump (Q1_LUMP_ENTITIES, q1_dentdata, q1_entdatasize);
	Q1_AddLump (Q1_LUMP_TEXTURES, q1_dtexdata, q1_texdatasize);
	
	fseek (q1_wadfile, 0, SEEK_SET);
	SafeWrite (q1_wadfile, q1_header, sizeof(q1_dheader_t));
	fclose (q1_wadfile);	
}

//============================================================================

/*
=============
Q1_PrintBSPFileSizes

Dumps info about current file
=============
*/
void Q1_PrintBSPFileSizes (void)
{
	printf ("%5i planes       %6i\n"
		,q1_numplanes, (int)(q1_numplanes*sizeof(q1_dplane_t)));
	printf ("%5i vertexes     %6i\n"
		,q1_numvertexes, (int)(q1_numvertexes*sizeof(q1_dvertex_t)));
	printf ("%5i nodes        %6i\n"
		,q1_numnodes, (int)(q1_numnodes*sizeof(q1_dnode_t)));
	printf ("%5i texinfo      %6i\n"
		,q1_numtexinfo, (int)(q1_numtexinfo*sizeof(q1_texinfo_t)));
	printf ("%5i faces        %6i\n"
		,q1_numfaces, (int)(q1_numfaces*sizeof(q1_dface_t)));
	printf ("%5i clipnodes    %6i\n"
		,q1_numclipnodes, (int)(q1_numclipnodes*sizeof(q1_dclipnode_t)));
	printf ("%5i leafs        %6i\n"
		,q1_numleafs, (int)(q1_numleafs*sizeof(q1_dleaf_t)));
	printf ("%5i marksurfaces %6i\n"
		,q1_nummarksurfaces, (int)(q1_nummarksurfaces*sizeof(q1_dmarksurfaces[0])));
	printf ("%5i surfedges    %6i\n"
		,q1_numsurfedges, (int)(q1_numsurfedges*sizeof(q1_dmarksurfaces[0])));
	printf ("%5i edges        %6i\n"
		,q1_numedges, (int)(q1_numedges*sizeof(q1_dedge_t)));
	if (!q1_texdatasize)
		printf ("    0 textures          0\n");
	else
		printf ("%5i textures     %6i\n",((q1_dmiptexlump_t*)q1_dtexdata)->nummiptex, q1_texdatasize);
	printf ("      lightdata    %6i\n", q1_lightdatasize);
	printf ("      visdata      %6i\n", q1_visdatasize);
	printf ("      entdata      %6i\n", q1_entdatasize);
} //end of the function Q1_PrintBSPFileSizes


/*
================
Q1_ParseEntities

Parses the dentdata string into entities
================
*/
void Q1_ParseEntities (void)
{
	script_t *script;

	num_entities = 0;
	script = LoadScriptMemory(q1_dentdata, q1_entdatasize, "*Quake1 bsp file");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES |
									SCFL_NOSTRINGESCAPECHARS);

	while(ParseEntity(script))
	{
	} //end while

	FreeScript(script);
} //end of the function Q1_ParseEntities


/*
================
Q1_UnparseEntities

Generates the dentdata string from all the entities
================
*/
void Q1_UnparseEntities (void)
{
	char *buf, *end;
	epair_t *ep;
	char line[2048];
	int i;
	
	buf = q1_dentdata;
	end = buf;
	*end = 0;
	
	for (i=0 ; i<num_entities ; i++)
	{
		ep = entities[i].epairs;
		if (!ep)
			continue;	// ent got removed
		
		strcat (end,"{\n");
		end += 2;
				
		for (ep = entities[i].epairs ; ep ; ep=ep->next)
		{
			sprintf (line, "\"%s\" \"%s\"\n", ep->key, ep->value);
			strcat (end, line);
			end += strlen(line);
		}
		strcat (end,"}\n");
		end += 2;

		if (end > buf + Q1_MAX_MAP_ENTSTRING)
			Error ("Entity text too long");
	}
	q1_entdatasize = end - buf + 1;
} //end of the function Q1_UnparseEntities
