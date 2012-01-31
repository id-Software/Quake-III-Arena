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

#include "q3files.h"
//#include "surfaceflags.h"

extern	int				q3_nummodels;
extern	q3_dmodel_t		*q3_dmodels;//[MAX_MAP_MODELS];

extern	int				q3_numShaders;
extern	q3_dshader_t	*q3_dshaders;//[Q3_MAX_MAP_SHADERS];

extern	int				q3_entdatasize;
extern	char			*q3_dentdata;//[Q3_MAX_MAP_ENTSTRING];

extern	int				q3_numleafs;
extern	q3_dleaf_t		*q3_dleafs;//[Q3_MAX_MAP_LEAFS];

extern	int				q3_numplanes;
extern	q3_dplane_t		*q3_dplanes;//[Q3_MAX_MAP_PLANES];

extern	int				q3_numnodes;
extern	q3_dnode_t		*q3_dnodes;//[Q3_MAX_MAP_NODES];

extern	int				q3_numleafsurfaces;
extern	int				*q3_dleafsurfaces;//[Q3_MAX_MAP_LEAFFACES];

extern	int				q3_numleafbrushes;
extern	int				*q3_dleafbrushes;//[Q3_MAX_MAP_LEAFBRUSHES];

extern	int				q3_numbrushes;
extern	q3_dbrush_t		*q3_dbrushes;//[Q3_MAX_MAP_BRUSHES];

extern	int				q3_numbrushsides;
extern	q3_dbrushside_t	*q3_dbrushsides;//[Q3_MAX_MAP_BRUSHSIDES];

extern	int				q3_numLightBytes;
extern	byte			*q3_lightBytes;//[Q3_MAX_MAP_LIGHTING];

extern	int				q3_numGridPoints;
extern	byte			*q3_gridData;//[Q3_MAX_MAP_LIGHTGRID];

extern	int				q3_numVisBytes;
extern	byte			*q3_visBytes;//[Q3_MAX_MAP_VISIBILITY];

extern	int				q3_numDrawVerts;
extern	q3_drawVert_t	*q3_drawVerts;//[Q3_MAX_MAP_DRAW_VERTS];

extern	int				q3_numDrawIndexes;
extern	int				*q3_drawIndexes;//[Q3_MAX_MAP_DRAW_INDEXES];

extern	int				q3_numDrawSurfaces;
extern	q3_dsurface_t	*q3_drawSurfaces;//[Q3_MAX_MAP_DRAW_SURFS];

extern	int				q3_numFogs;
extern	q3_dfog_t		*q3_dfogs;//[Q3_MAX_MAP_FOGS];

extern	char			q3_dbrushsidetextured[Q3_MAX_MAP_BRUSHSIDES];

void Q3_LoadBSPFile(struct quakefile_s *qf);
void Q3_FreeMaxBSP(void);
void Q3_ParseEntities (void);
