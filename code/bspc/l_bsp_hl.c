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
#include "l_bsp_hl.h"
#include "l_bsp_ent.h"

//=============================================================================

int				hl_nummodels;
hl_dmodel_t		*hl_dmodels;//[HL_MAX_MAP_MODELS];
int				hl_dmodels_checksum;

int				hl_visdatasize;
byte				*hl_dvisdata;//[HL_MAX_MAP_VISIBILITY];
int				hl_dvisdata_checksum;

int				hl_lightdatasize;
byte				*hl_dlightdata;//[HL_MAX_MAP_LIGHTING];
int				hl_dlightdata_checksum;

int				hl_texdatasize;
byte				*hl_dtexdata;//[HL_MAX_MAP_MIPTEX]; // (dmiptexlump_t)
int				hl_dtexdata_checksum;

int				hl_entdatasize;
char				*hl_dentdata;//[HL_MAX_MAP_ENTSTRING];
int				hl_dentdata_checksum;

int				hl_numleafs;
hl_dleaf_t		*hl_dleafs;//[HL_MAX_MAP_LEAFS];
int				hl_dleafs_checksum;

int				hl_numplanes;
hl_dplane_t		*hl_dplanes;//[HL_MAX_MAP_PLANES];
int				hl_dplanes_checksum;

int				hl_numvertexes;
hl_dvertex_t	*hl_dvertexes;//[HL_MAX_MAP_VERTS];
int				hl_dvertexes_checksum;

int				hl_numnodes;
hl_dnode_t		*hl_dnodes;//[HL_MAX_MAP_NODES];
int				hl_dnodes_checksum;

int				hl_numtexinfo;
hl_texinfo_t	*hl_texinfo;//[HL_MAX_MAP_TEXINFO];
int				hl_texinfo_checksum;

int				hl_numfaces;
hl_dface_t		*hl_dfaces;//[HL_MAX_MAP_FACES];
int				hl_dfaces_checksum;

int				hl_numclipnodes;
hl_dclipnode_t	*hl_dclipnodes;//[HL_MAX_MAP_CLIPNODES];
int				hl_dclipnodes_checksum;

int				hl_numedges;
hl_dedge_t		*hl_dedges;//[HL_MAX_MAP_EDGES];
int				hl_dedges_checksum;

int				hl_nummarksurfaces;
unsigned short	*hl_dmarksurfaces;//[HL_MAX_MAP_MARKSURFACES];
int				hl_dmarksurfaces_checksum;

int				hl_numsurfedges;
int				*hl_dsurfedges;//[HL_MAX_MAP_SURFEDGES];
int				hl_dsurfedges_checksum;

//int				num_entities;
//entity_t			entities[HL_MAX_MAP_ENTITIES];


//#ifdef //ME

int hl_bspallocated = false;
int hl_allocatedbspmem = 0;

void HL_AllocMaxBSP(void)
{
	//models
	hl_nummodels = 0;
	hl_dmodels = (hl_dmodel_t *) GetMemory(HL_MAX_MAP_MODELS * sizeof(hl_dmodel_t));
	hl_allocatedbspmem = HL_MAX_MAP_MODELS * sizeof(hl_dmodel_t);
	//visibility
	hl_visdatasize = 0;
	hl_dvisdata = (byte *) GetMemory(HL_MAX_MAP_VISIBILITY * sizeof(byte));
	hl_allocatedbspmem += HL_MAX_MAP_VISIBILITY * sizeof(byte);
	//light data
	hl_lightdatasize = 0;
	hl_dlightdata = (byte *) GetMemory(HL_MAX_MAP_LIGHTING * sizeof(byte));
	hl_allocatedbspmem += HL_MAX_MAP_LIGHTING * sizeof(byte);
	//texture data
	hl_texdatasize = 0;
	hl_dtexdata = (byte *) GetMemory(HL_MAX_MAP_MIPTEX * sizeof(byte)); // (dmiptexlump_t)
	hl_allocatedbspmem += HL_MAX_MAP_MIPTEX * sizeof(byte);
	//entities
	hl_entdatasize = 0;
	hl_dentdata = (char *) GetMemory(HL_MAX_MAP_ENTSTRING * sizeof(char));
	hl_allocatedbspmem += HL_MAX_MAP_ENTSTRING * sizeof(char);
	//leaves
	hl_numleafs = 0;
	hl_dleafs = (hl_dleaf_t *) GetMemory(HL_MAX_MAP_LEAFS * sizeof(hl_dleaf_t));
	hl_allocatedbspmem += HL_MAX_MAP_LEAFS * sizeof(hl_dleaf_t);
	//planes
	hl_numplanes = 0;
	hl_dplanes = (hl_dplane_t *) GetMemory(HL_MAX_MAP_PLANES * sizeof(hl_dplane_t));
	hl_allocatedbspmem += HL_MAX_MAP_PLANES * sizeof(hl_dplane_t);
	//vertexes
	hl_numvertexes = 0;
	hl_dvertexes = (hl_dvertex_t *) GetMemory(HL_MAX_MAP_VERTS * sizeof(hl_dvertex_t));
	hl_allocatedbspmem += HL_MAX_MAP_VERTS * sizeof(hl_dvertex_t);
	//nodes
	hl_numnodes = 0;
	hl_dnodes = (hl_dnode_t *) GetMemory(HL_MAX_MAP_NODES * sizeof(hl_dnode_t));
	hl_allocatedbspmem += HL_MAX_MAP_NODES * sizeof(hl_dnode_t);
	//texture info
	hl_numtexinfo = 0;
	hl_texinfo = (hl_texinfo_t *) GetMemory(HL_MAX_MAP_TEXINFO * sizeof(hl_texinfo_t));
	hl_allocatedbspmem += HL_MAX_MAP_TEXINFO * sizeof(hl_texinfo_t);
	//faces
	hl_numfaces = 0;
	hl_dfaces = (hl_dface_t *) GetMemory(HL_MAX_MAP_FACES * sizeof(hl_dface_t));
	hl_allocatedbspmem += HL_MAX_MAP_FACES * sizeof(hl_dface_t);
	//clip nodes
	hl_numclipnodes = 0;
	hl_dclipnodes = (hl_dclipnode_t *) GetMemory(HL_MAX_MAP_CLIPNODES * sizeof(hl_dclipnode_t));
	hl_allocatedbspmem += HL_MAX_MAP_CLIPNODES * sizeof(hl_dclipnode_t);
	//edges
	hl_numedges = 0;
	hl_dedges = (hl_dedge_t *) GetMemory(HL_MAX_MAP_EDGES * sizeof(hl_dedge_t));
	hl_allocatedbspmem += HL_MAX_MAP_EDGES, sizeof(hl_dedge_t);
	//mark surfaces
	hl_nummarksurfaces = 0;
	hl_dmarksurfaces = (unsigned short *) GetMemory(HL_MAX_MAP_MARKSURFACES * sizeof(unsigned short));
	hl_allocatedbspmem += HL_MAX_MAP_MARKSURFACES * sizeof(unsigned short);
	//surface edges
	hl_numsurfedges = 0;
	hl_dsurfedges = (int *) GetMemory(HL_MAX_MAP_SURFEDGES * sizeof(int));
	hl_allocatedbspmem += HL_MAX_MAP_SURFEDGES * sizeof(int);
	//print allocated memory
	Log_Print("allocated ");
	PrintMemorySize(hl_allocatedbspmem);
	Log_Print(" of BSP memory\n");
} //end of the function HL_AllocMaxBSP

void HL_FreeMaxBSP(void)
{
	//models
	hl_nummodels = 0;
	FreeMemory(hl_dmodels);
	hl_dmodels = NULL;
	//visibility
	hl_visdatasize = 0;
	FreeMemory(hl_dvisdata);
	hl_dvisdata = NULL;
	//light data
	hl_lightdatasize = 0;
	FreeMemory(hl_dlightdata);
	hl_dlightdata = NULL;
	//texture data
	hl_texdatasize = 0;
	FreeMemory(hl_dtexdata);
	hl_dtexdata = NULL;
	//entities
	hl_entdatasize = 0;
	FreeMemory(hl_dentdata);
	hl_dentdata = NULL;
	//leaves
	hl_numleafs = 0;
	FreeMemory(hl_dleafs);
	hl_dleafs = NULL;
	//planes
	hl_numplanes = 0;
	FreeMemory(hl_dplanes);
	hl_dplanes = NULL;
	//vertexes
	hl_numvertexes = 0;
	FreeMemory(hl_dvertexes);
	hl_dvertexes = NULL;
	//nodes
	hl_numnodes = 0;
	FreeMemory(hl_dnodes);
	hl_dnodes = NULL;
	//texture info
	hl_numtexinfo = 0;
	FreeMemory(hl_texinfo);
	hl_texinfo = NULL;
	//faces
	hl_numfaces = 0;
	FreeMemory(hl_dfaces);
	hl_dfaces = NULL;
	//clip nodes
	hl_numclipnodes = 0;
	FreeMemory(hl_dclipnodes);
	hl_dclipnodes = NULL;
	//edges
	hl_numedges = 0;
	FreeMemory(hl_dedges);
	hl_dedges = NULL;
	//mark surfaces
	hl_nummarksurfaces = 0;
	FreeMemory(hl_dmarksurfaces);
	hl_dmarksurfaces = NULL;
	//surface edges
	hl_numsurfedges = 0;
	FreeMemory(hl_dsurfedges);
	hl_dsurfedges = NULL;
	//
	Log_Print("freed ");
	PrintMemorySize(hl_allocatedbspmem);
	Log_Print(" of BSP memory\n");
	hl_allocatedbspmem = 0;
} //end of the function HL_FreeMaxBSP
//#endif //ME

/*
===============
FastChecksum
===============
*/

int FastChecksum(void *buffer, int bytes)
{
	int	checksum = 0;

	while( bytes-- )  
		checksum = (checksum << 4) ^ *((char *)buffer)++;

	return checksum;
}

/*
===============
HL_CompressVis
===============
*/
int HL_CompressVis(byte *vis, byte *dest)
{
	int		j;
	int		rep;
	int		visrow;
	byte	*dest_p;
	
	dest_p = dest;
	visrow = (hl_numleafs + 7)>>3;
	
	for (j=0 ; j<visrow ; j++)
	{
		*dest_p++ = vis[j];
		if (vis[j])
			continue;

		rep = 1;
		for ( j++; j<visrow ; j++)
			if (vis[j] || rep == 255)
				break;
			else
				rep++;
		*dest_p++ = rep;
		j--;
	}
	
	return dest_p - dest;
}


/*
===================
HL_DecompressVis
===================
*/
void HL_DecompressVis (byte *in, byte *decompressed)
{
	int		c;
	byte	*out;
	int		row;

	row = (hl_numleafs+7)>>3;	
	out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
}

//=============================================================================

/*
=============
HL_SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void HL_SwapBSPFile (qboolean todisk)
{
	int i, j, c;
	hl_dmodel_t *d;
	hl_dmiptexlump_t *mtl;

	
// models	
	for (i = 0; i < hl_nummodels; i++)
	{
		d = &hl_dmodels[i];

		for (j = 0; j < HL_MAX_MAP_HULLS; j++)
			d->headnode[j] = LittleLong(d->headnode[j]);

		d->visleafs = LittleLong(d->visleafs);
		d->firstface = LittleLong(d->firstface);
		d->numfaces = LittleLong(d->numfaces);
		
		for (j = 0; j < 3; j++)
		{
			d->mins[j] = LittleFloat(d->mins[j]);
			d->maxs[j] = LittleFloat(d->maxs[j]);
			d->origin[j] = LittleFloat(d->origin[j]);
		}
	}

//
// vertexes
//
	for (i = 0; i < hl_numvertexes; i++)
	{
		for (j = 0; j < 3; j++)
			hl_dvertexes[i].point[j] = LittleFloat (hl_dvertexes[i].point[j]);
	}
		
//
// planes
//	
	for (i=0 ; i<hl_numplanes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			hl_dplanes[i].normal[j] = LittleFloat (hl_dplanes[i].normal[j]);
		hl_dplanes[i].dist = LittleFloat (hl_dplanes[i].dist);
		hl_dplanes[i].type = LittleLong (hl_dplanes[i].type);
	}
	
//
// texinfos
//	
	for (i=0 ; i<hl_numtexinfo ; i++)
	{
		for (j=0 ; j<8 ; j++)
			hl_texinfo[i].vecs[0][j] = LittleFloat (hl_texinfo[i].vecs[0][j]);
		hl_texinfo[i].miptex = LittleLong (hl_texinfo[i].miptex);
		hl_texinfo[i].flags = LittleLong (hl_texinfo[i].flags);
	}
	
//
// faces
//
	for (i=0 ; i<hl_numfaces ; i++)
	{
		hl_dfaces[i].texinfo = LittleShort (hl_dfaces[i].texinfo);
		hl_dfaces[i].planenum = LittleShort (hl_dfaces[i].planenum);
		hl_dfaces[i].side = LittleShort (hl_dfaces[i].side);
		hl_dfaces[i].lightofs = LittleLong (hl_dfaces[i].lightofs);
		hl_dfaces[i].firstedge = LittleLong (hl_dfaces[i].firstedge);
		hl_dfaces[i].numedges = LittleShort (hl_dfaces[i].numedges);
	}

//
// nodes
//
	for (i=0 ; i<hl_numnodes ; i++)
	{
		hl_dnodes[i].planenum = LittleLong (hl_dnodes[i].planenum);
		for (j=0 ; j<3 ; j++)
		{
			hl_dnodes[i].mins[j] = LittleShort (hl_dnodes[i].mins[j]);
			hl_dnodes[i].maxs[j] = LittleShort (hl_dnodes[i].maxs[j]);
		}
		hl_dnodes[i].children[0] = LittleShort (hl_dnodes[i].children[0]);
		hl_dnodes[i].children[1] = LittleShort (hl_dnodes[i].children[1]);
		hl_dnodes[i].firstface = LittleShort (hl_dnodes[i].firstface);
		hl_dnodes[i].numfaces = LittleShort (hl_dnodes[i].numfaces);
	}

//
// leafs
//
	for (i=0 ; i<hl_numleafs ; i++)
	{
		hl_dleafs[i].contents = LittleLong (hl_dleafs[i].contents);
		for (j=0 ; j<3 ; j++)
		{
			hl_dleafs[i].mins[j] = LittleShort (hl_dleafs[i].mins[j]);
			hl_dleafs[i].maxs[j] = LittleShort (hl_dleafs[i].maxs[j]);
		}

		hl_dleafs[i].firstmarksurface = LittleShort (hl_dleafs[i].firstmarksurface);
		hl_dleafs[i].nummarksurfaces = LittleShort (hl_dleafs[i].nummarksurfaces);
		hl_dleafs[i].visofs = LittleLong (hl_dleafs[i].visofs);
	}

//
// clipnodes
//
	for (i=0 ; i<hl_numclipnodes ; i++)
	{
		hl_dclipnodes[i].planenum = LittleLong (hl_dclipnodes[i].planenum);
		hl_dclipnodes[i].children[0] = LittleShort (hl_dclipnodes[i].children[0]);
		hl_dclipnodes[i].children[1] = LittleShort (hl_dclipnodes[i].children[1]);
	}

//
// miptex
//
	if (hl_texdatasize)
	{
		mtl = (hl_dmiptexlump_t *)hl_dtexdata;
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
	for (i=0 ; i<hl_nummarksurfaces ; i++)
		hl_dmarksurfaces[i] = LittleShort (hl_dmarksurfaces[i]);

//
// surfedges
//
	for (i=0 ; i<hl_numsurfedges ; i++)
		hl_dsurfedges[i] = LittleLong (hl_dsurfedges[i]);

//
// edges
//
	for (i=0 ; i<hl_numedges ; i++)
	{
		hl_dedges[i].v[0] = LittleShort (hl_dedges[i].v[0]);
		hl_dedges[i].v[1] = LittleShort (hl_dedges[i].v[1]);
	}
} //end of the function HL_SwapBSPFile


hl_dheader_t	*hl_header;
int				hl_fileLength;

int HL_CopyLump (int lump, void *dest, int size, int maxsize)
{
	int		length, ofs;

	length = hl_header->lumps[lump].filelen;
	ofs = hl_header->lumps[lump].fileofs;
	
	if (length % size) {
		Error ("LoadBSPFile: odd lump size");
	}
	// somehow things got out of range
	if ((length/size) > maxsize) {
		printf("WARNING: exceeded max size for lump %d size %d > maxsize %d\n", lump, (length/size), maxsize);
		length = maxsize * size;
	}
	if ( ofs + length > hl_fileLength ) {
		printf("WARNING: exceeded file length for lump %d\n", lump);
		length = hl_fileLength - ofs;
		if ( length <= 0 ) {
			return 0;
		}
	}

	memcpy (dest, (byte *)hl_header + ofs, length);

	return length / size;
}

/*
=============
HL_LoadBSPFile
=============
*/
void	HL_LoadBSPFile (char *filename, int offset, int length)
{
	int			i;
	
//
// load the file header
//
	hl_fileLength = LoadFile (filename, (void **)&hl_header, offset, length);

// swap the header
	for (i=0 ; i< sizeof(hl_dheader_t)/4 ; i++)
		((int *)hl_header)[i] = LittleLong ( ((int *)hl_header)[i]);

	if (hl_header->version != HL_BSPVERSION)
		Error ("%s is version %i, not %i", filename, hl_header->version, HL_BSPVERSION);

	hl_nummodels = HL_CopyLump (HL_LUMP_MODELS, hl_dmodels, sizeof(hl_dmodel_t), HL_MAX_MAP_MODELS );
	hl_numvertexes = HL_CopyLump (HL_LUMP_VERTEXES, hl_dvertexes, sizeof(hl_dvertex_t), HL_MAX_MAP_VERTS );
	hl_numplanes = HL_CopyLump (HL_LUMP_PLANES, hl_dplanes, sizeof(hl_dplane_t), HL_MAX_MAP_PLANES );
	hl_numleafs = HL_CopyLump (HL_LUMP_LEAFS, hl_dleafs, sizeof(hl_dleaf_t), HL_MAX_MAP_LEAFS );
	hl_numnodes = HL_CopyLump (HL_LUMP_NODES, hl_dnodes, sizeof(hl_dnode_t), HL_MAX_MAP_NODES );
	hl_numtexinfo = HL_CopyLump (HL_LUMP_TEXINFO, hl_texinfo, sizeof(hl_texinfo_t), HL_MAX_MAP_TEXINFO );
	hl_numclipnodes = HL_CopyLump (HL_LUMP_CLIPNODES, hl_dclipnodes, sizeof(hl_dclipnode_t), HL_MAX_MAP_CLIPNODES );
	hl_numfaces = HL_CopyLump (HL_LUMP_FACES, hl_dfaces, sizeof(hl_dface_t), HL_MAX_MAP_FACES );
	hl_nummarksurfaces = HL_CopyLump (HL_LUMP_MARKSURFACES, hl_dmarksurfaces, sizeof(hl_dmarksurfaces[0]), HL_MAX_MAP_MARKSURFACES );
	hl_numsurfedges = HL_CopyLump (HL_LUMP_SURFEDGES, hl_dsurfedges, sizeof(hl_dsurfedges[0]), HL_MAX_MAP_SURFEDGES );
	hl_numedges = HL_CopyLump (HL_LUMP_EDGES, hl_dedges, sizeof(hl_dedge_t), HL_MAX_MAP_EDGES );

	hl_texdatasize = HL_CopyLump (HL_LUMP_TEXTURES, hl_dtexdata, 1, HL_MAX_MAP_MIPTEX );
	hl_visdatasize = HL_CopyLump (HL_LUMP_VISIBILITY, hl_dvisdata, 1, HL_MAX_MAP_VISIBILITY );
	hl_lightdatasize = HL_CopyLump (HL_LUMP_LIGHTING, hl_dlightdata, 1, HL_MAX_MAP_LIGHTING );
	hl_entdatasize = HL_CopyLump (HL_LUMP_ENTITIES, hl_dentdata, 1, HL_MAX_MAP_ENTSTRING );

	FreeMemory(hl_header);		// everything has been copied out
		
//
// swap everything
//	
	HL_SwapBSPFile (false);

	hl_dmodels_checksum = FastChecksum( hl_dmodels, hl_nummodels*sizeof(hl_dmodels[0]) );
	hl_dvertexes_checksum = FastChecksum( hl_dvertexes, hl_numvertexes*sizeof(hl_dvertexes[0]) );
	hl_dplanes_checksum = FastChecksum( hl_dplanes, hl_numplanes*sizeof(hl_dplanes[0]) );
	hl_dleafs_checksum = FastChecksum( hl_dleafs, hl_numleafs*sizeof(hl_dleafs[0]) );
	hl_dnodes_checksum = FastChecksum( hl_dnodes, hl_numnodes*sizeof(hl_dnodes[0]) );
	hl_texinfo_checksum = FastChecksum( hl_texinfo, hl_numtexinfo*sizeof(hl_texinfo[0]) );
	hl_dclipnodes_checksum = FastChecksum( hl_dclipnodes, hl_numclipnodes*sizeof(hl_dclipnodes[0]) );
	hl_dfaces_checksum = FastChecksum( hl_dfaces, hl_numfaces*sizeof(hl_dfaces[0]) );
	hl_dmarksurfaces_checksum = FastChecksum( hl_dmarksurfaces, hl_nummarksurfaces*sizeof(hl_dmarksurfaces[0]) );
	hl_dsurfedges_checksum = FastChecksum( hl_dsurfedges, hl_numsurfedges*sizeof(hl_dsurfedges[0]) );
	hl_dedges_checksum = FastChecksum( hl_dedges, hl_numedges*sizeof(hl_dedges[0]) );
	hl_dtexdata_checksum = FastChecksum( hl_dtexdata, hl_numedges*sizeof(hl_dtexdata[0]) );
	hl_dvisdata_checksum = FastChecksum( hl_dvisdata, hl_visdatasize*sizeof(hl_dvisdata[0]) );
	hl_dlightdata_checksum = FastChecksum( hl_dlightdata, hl_lightdatasize*sizeof(hl_dlightdata[0]) );
	hl_dentdata_checksum = FastChecksum( hl_dentdata, hl_entdatasize*sizeof(hl_dentdata[0]) );

}

//============================================================================

FILE		*wadfile;
hl_dheader_t	outheader;

void HL_AddLump (int lumpnum, void *data, int len)
{
	hl_lump_t *lump;

	lump = &hl_header->lumps[lumpnum];
	
	lump->fileofs = LittleLong( ftell(wadfile) );
	lump->filelen = LittleLong(len);
	SafeWrite (wadfile, data, (len+3)&~3);
}

/*
=============
HL_WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void HL_WriteBSPFile (char *filename)
{		
	hl_header = &outheader;
	memset (hl_header, 0, sizeof(hl_dheader_t));
	
	HL_SwapBSPFile (true);

	hl_header->version = LittleLong (HL_BSPVERSION);
	
	wadfile = SafeOpenWrite (filename);
	SafeWrite (wadfile, hl_header, sizeof(hl_dheader_t));	// overwritten later

	HL_AddLump (HL_LUMP_PLANES, hl_dplanes, hl_numplanes*sizeof(hl_dplane_t));
	HL_AddLump (HL_LUMP_LEAFS, hl_dleafs, hl_numleafs*sizeof(hl_dleaf_t));
	HL_AddLump (HL_LUMP_VERTEXES, hl_dvertexes, hl_numvertexes*sizeof(hl_dvertex_t));
	HL_AddLump (HL_LUMP_NODES, hl_dnodes, hl_numnodes*sizeof(hl_dnode_t));
	HL_AddLump (HL_LUMP_TEXINFO, hl_texinfo, hl_numtexinfo*sizeof(hl_texinfo_t));
	HL_AddLump (HL_LUMP_FACES, hl_dfaces, hl_numfaces*sizeof(hl_dface_t));
	HL_AddLump (HL_LUMP_CLIPNODES, hl_dclipnodes, hl_numclipnodes*sizeof(hl_dclipnode_t));
	HL_AddLump (HL_LUMP_MARKSURFACES, hl_dmarksurfaces, hl_nummarksurfaces*sizeof(hl_dmarksurfaces[0]));
	HL_AddLump (HL_LUMP_SURFEDGES, hl_dsurfedges, hl_numsurfedges*sizeof(hl_dsurfedges[0]));
	HL_AddLump (HL_LUMP_EDGES, hl_dedges, hl_numedges*sizeof(hl_dedge_t));
	HL_AddLump (HL_LUMP_MODELS, hl_dmodels, hl_nummodels*sizeof(hl_dmodel_t));

	HL_AddLump (HL_LUMP_LIGHTING, hl_dlightdata, hl_lightdatasize);
	HL_AddLump (HL_LUMP_VISIBILITY, hl_dvisdata, hl_visdatasize);
	HL_AddLump (HL_LUMP_ENTITIES, hl_dentdata, hl_entdatasize);
	HL_AddLump (HL_LUMP_TEXTURES, hl_dtexdata, hl_texdatasize);
	
	fseek (wadfile, 0, SEEK_SET);
	SafeWrite (wadfile, hl_header, sizeof(hl_dheader_t));
	fclose (wadfile);	
}

//============================================================================

#define ENTRIES(a)		(sizeof(a)/sizeof(*(a)))
#define ENTRYSIZE(a)	(sizeof(*(a)))

ArrayUsage( char *szItem, int items, int maxitems, int itemsize )
{
	float	percentage = maxitems ? items * 100.0 / maxitems : 0.0;

   qprintf("%-12s  %7i/%-7i  %7i/%-7i  (%4.1f%%)", 
				szItem, items, maxitems, items * itemsize, maxitems * itemsize, percentage );
	if ( percentage > 80.0 )
		qprintf( "VERY FULL!\n" );
	else if ( percentage > 95.0 )
		qprintf( "SIZE DANGER!\n" );
	else if ( percentage > 99.9 )
		qprintf( "SIZE OVERFLOW!!!\n" );
	else
		qprintf( "\n" );
	return items * itemsize;
}

GlobUsage( char *szItem, int itemstorage, int maxstorage )
{
	float	percentage = maxstorage ? itemstorage * 100.0 / maxstorage : 0.0;

	qprintf("%-12s     [variable]    %7i/%-7i  (%4.1f%%)", 
				szItem, itemstorage, maxstorage, percentage );
	if ( percentage > 80.0 )
		qprintf( "VERY FULL!\n" );
	else if ( percentage > 95.0 )
		qprintf( "SIZE DANGER!\n" );
	else if ( percentage > 99.9 )
		qprintf( "SIZE OVERFLOW!!!\n" );
	else
		qprintf( "\n" );
	return itemstorage;
}

/*
=============
HL_PrintBSPFileSizes

Dumps info about current file
=============
*/
void HL_PrintBSPFileSizes(void)
{
	int	numtextures = hl_texdatasize ? ((hl_dmiptexlump_t*)hl_dtexdata)->nummiptex : 0;
	int	totalmemory = 0;

	qprintf("\n");
	qprintf("Object names  Objects/Maxobjs  Memory / Maxmem  Fullness\n" );
	qprintf("------------  ---------------  ---------------  --------\n" );

	totalmemory += ArrayUsage( "models",		hl_nummodels,		ENTRIES(hl_dmodels),		ENTRYSIZE(hl_dmodels) );
	totalmemory += ArrayUsage( "planes",		hl_numplanes,		ENTRIES(hl_dplanes),		ENTRYSIZE(hl_dplanes) );
	totalmemory += ArrayUsage( "vertexes",		hl_numvertexes,	ENTRIES(hl_dvertexes),	ENTRYSIZE(hl_dvertexes) );
	totalmemory += ArrayUsage( "nodes",			hl_numnodes,		ENTRIES(hl_dnodes),		ENTRYSIZE(hl_dnodes) );
	totalmemory += ArrayUsage( "texinfos",		hl_numtexinfo,		ENTRIES(hl_texinfo),		ENTRYSIZE(hl_texinfo) );
	totalmemory += ArrayUsage( "faces",			hl_numfaces,		ENTRIES(hl_dfaces),		ENTRYSIZE(hl_dfaces) );
	totalmemory += ArrayUsage( "clipnodes",	hl_numclipnodes,	ENTRIES(hl_dclipnodes),	ENTRYSIZE(hl_dclipnodes) );
	totalmemory += ArrayUsage( "leaves",		hl_numleafs,		ENTRIES(hl_dleafs),		ENTRYSIZE(hl_dleafs) );
	totalmemory += ArrayUsage( "marksurfaces",hl_nummarksurfaces,ENTRIES(hl_dmarksurfaces),ENTRYSIZE(hl_dmarksurfaces) );
	totalmemory += ArrayUsage( "surfedges",	hl_numsurfedges,	ENTRIES(hl_dsurfedges),	ENTRYSIZE(hl_dsurfedges) );
	totalmemory += ArrayUsage( "edges",			hl_numedges,		ENTRIES(hl_dedges),		ENTRYSIZE(hl_dedges) );

	totalmemory += GlobUsage( "texdata",		hl_texdatasize,	sizeof(hl_dtexdata) );
	totalmemory += GlobUsage( "lightdata",		hl_lightdatasize,	sizeof(hl_dlightdata) );
	totalmemory += GlobUsage( "visdata",		hl_visdatasize,	sizeof(hl_dvisdata) );
	totalmemory += GlobUsage( "entdata",		hl_entdatasize,	sizeof(hl_dentdata) );

	qprintf( "=== Total BSP file data space used: %d bytes ===\n\n", totalmemory );
}



/*
=================
ParseEpair
=================
* /
epair_t *ParseEpair (void)
{
	epair_t	*e;
	
	e = malloc (sizeof(epair_t));
	memset (e, 0, sizeof(epair_t));
	
	if (strlen(token) >= MAX_KEY-1)
		Error ("ParseEpar: token too long");
	e->key = copystring(token);
	GetToken (false);
	if (strlen(token) >= MAX_VALUE-1)
		Error ("ParseEpar: token too long");
	e->value = copystring(token);

	return e;
} //*/


/*
================
ParseEntity
================
* /
qboolean	ParseEntity (void)
{
	epair_t		*e;
	entity_t	*mapent;

	if (!GetToken (true))
		return false;

	if (strcmp (token, "{") )
		Error ("ParseEntity: { not found");
	
	if (num_entities == HL_MAX_MAP_ENTITIES)
		Error ("num_entities == HL_MAX_MAP_ENTITIES");

	mapent = &entities[num_entities];
	num_entities++;

	do
	{
		if (!GetToken (true))
			Error ("ParseEntity: EOF without closing brace");
		if (!strcmp (token, "}") )
			break;
		e = ParseEpair ();
		e->next = mapent->epairs;
		mapent->epairs = e;
	} while (1);
	
	return true;
} //*/

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void HL_ParseEntities (void)
{
	script_t *script;

	num_entities = 0;
	script = LoadScriptMemory(hl_dentdata, hl_entdatasize, "*Half-Life bsp file");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES |
									SCFL_NOSTRINGESCAPECHARS);

	while(ParseEntity(script))
	{
	} //end while

	FreeScript(script);
} //end of the function HL_ParseEntities


/*
================
UnparseEntities

Generates the dentdata string from all the entities
================
*/
void HL_UnparseEntities (void)
{
	char *buf, *end;
	epair_t *ep;
	char line[2048];
	int i;
	
	buf = hl_dentdata;
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

		if (end > buf + HL_MAX_MAP_ENTSTRING)
			Error ("Entity text too long");
	}
	hl_entdatasize = end - buf + 1;
} //end of the function HL_UnparseEntities


/*
void 	SetKeyValue (entity_t *ent, char *key, char *value)
{
	epair_t	*ep;
	
	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
		{
			free (ep->value);
			ep->value = copystring(value);
			return;
		}
	ep = malloc (sizeof(*ep));
	ep->next = ent->epairs;
	ent->epairs = ep;
	ep->key = copystring(key);
	ep->value = copystring(value);
}

char 	*ValueForKey (entity_t *ent, char *key)
{
	epair_t	*ep;
	
	for (ep=ent->epairs ; ep ; ep=ep->next)
		if (!strcmp (ep->key, key) )
			return ep->value;
	return "";
}

vec_t	FloatForKey (entity_t *ent, char *key)
{
	char	*k;
	
	k = ValueForKey (ent, key);
	return atof(k);
}

void 	GetVectorForKey (entity_t *ent, char *key, vec3_t vec)
{
	char	*k;
	double	v1, v2, v3;

	k = ValueForKey (ent, key);
// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	vec[0] = v1;
	vec[1] = v2;
	vec[2] = v3;
} //*/
