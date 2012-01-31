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

#include "sinfiles.h"

#define SINGAME_BSPHEADER		(('P'<<24)+('S'<<16)+('B'<<8)+'R')	//RBSP
#define SINGAME_BSPVERSION		1

#define SIN_BSPHEADER			(('P'<<24)+('S'<<16)+('B'<<8)+'I')	//IBSP
#define SIN_BSPVERSION	41


extern	int					sin_nummodels;
extern	sin_dmodel_t		*sin_dmodels;//[MAX_MAP_MODELS];

extern	int					sin_visdatasize;
extern	byte				*sin_dvisdata;//[MAX_MAP_VISIBILITY];
extern	sin_dvis_t			*sin_dvis;// = (dvis_t *)sin_sin_dvisdata;

extern	int					sin_lightdatasize;
extern	byte				*sin_dlightdata;//[MAX_MAP_LIGHTING];

extern	int					sin_entdatasize;
extern	char				*sin_dentdata;//[MAX_MAP_ENTSTRING];

extern	int					sin_numleafs;
extern	sin_dleaf_t			*sin_dleafs;//[MAX_MAP_LEAFS];

extern	int					sin_numplanes;
extern	sin_dplane_t		*sin_dplanes;//[MAX_MAP_PLANES];

extern	int					sin_numvertexes;
extern	sin_dvertex_t		*sin_dvertexes;//[MAX_MAP_VERTS];

extern	int					sin_numnodes;
extern	sin_dnode_t			*sin_dnodes;//[MAX_MAP_NODES];

extern	int					sin_numtexinfo;
extern	sin_texinfo_t		*sin_texinfo;//[MAX_MAP_sin_texinfo];

extern	int					sin_numfaces;
extern	sin_dface_t			*sin_dfaces;//[MAX_MAP_FACES];

extern	int					sin_numedges;
extern	sin_dedge_t			*sin_dedges;//[MAX_MAP_EDGES];

extern	int					sin_numleaffaces;
extern	unsigned short		*sin_dleaffaces;//[MAX_MAP_LEAFFACES];

extern	int					sin_numleafbrushes;
extern	unsigned short		*sin_dleafbrushes;//[MAX_MAP_LEAFBRUSHES];

extern	int					sin_numsurfedges;
extern	int					*sin_dsurfedges;//[MAX_MAP_SURFEDGES];

extern	int					sin_numbrushes;
extern	sin_dbrush_t		*sin_dbrushes;//[MAX_MAP_BRUSHES];

extern	int					sin_numbrushsides;
extern	sin_dbrushside_t	*sin_dbrushsides;//[MAX_MAP_BRUSHSIDES];

extern	int					sin_numareas;
extern	sin_darea_t			*sin_dareas;//[MAX_MAP_AREAS];

extern	int					sin_numareaportals;
extern	sin_dareaportal_t	*sin_dareaportals;//[MAX_MAP_AREAPORTALS];

extern	int					sin_numlightinfo;
extern	sin_lightvalue_t	*sin_lightinfo;//[MAX_MAP_LIGHTINFO];

extern	byte				sin_dpop[256];

extern	char				sin_dbrushsidetextured[SIN_MAX_MAP_BRUSHSIDES];

void Sin_AllocMaxBSP(void);
void Sin_FreeMaxBSP(void);

void Sin_DecompressVis(byte *in, byte *decompressed);
int Sin_CompressVis(byte *vis, byte *dest);

void Sin_LoadBSPFile (char *filename, int offset, int length);
void Sin_LoadBSPFileTexinfo (char *filename);	// just for qdata
void Sin_WriteBSPFile (char *filename);
void Sin_PrintBSPFileSizes (void);
void Sin_ParseEntities(void);
void Sin_UnparseEntities(void);

