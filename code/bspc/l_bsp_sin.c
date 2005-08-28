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
#include "l_poly.h"
#include "../botlib/l_script.h"
#include "l_bsp_ent.h"
#include "l_bsp_sin.h"

void GetLeafNums (void);

//=============================================================================

int					sin_nummodels;
sin_dmodel_t		*sin_dmodels;//[SIN_MAX_MAP_MODELS];

int					sin_visdatasize;
byte				*sin_dvisdata;//[SIN_MAX_MAP_VISIBILITY];
sin_dvis_t			*sin_dvis;// = (sin_dvis_t *)sin_sin_dvisdata;

int					sin_lightdatasize;
byte				*sin_dlightdata;//[SIN_MAX_MAP_LIGHTING];

int					sin_entdatasize;
char				*sin_dentdata;//[SIN_MAX_MAP_ENTSTRING];

int					sin_numleafs;
sin_dleaf_t			*sin_dleafs;//[SIN_MAX_MAP_LEAFS];

int					sin_numplanes;
sin_dplane_t		*sin_dplanes;//[SIN_MAX_MAP_PLANES];

int					sin_numvertexes;
sin_dvertex_t		*sin_dvertexes;//[SIN_MAX_MAP_VERTS];

int					sin_numnodes;
sin_dnode_t			*sin_dnodes;//[SIN_MAX_MAP_NODES];

int					sin_numtexinfo;
sin_texinfo_t		*sin_texinfo;//[SIN_MAX_MAP_sin_texinfo];

int					sin_numfaces;
sin_dface_t			*sin_dfaces;//[SIN_MAX_MAP_FACES];

int					sin_numedges;
sin_dedge_t			*sin_dedges;//[SIN_MAX_MAP_EDGES];

int					sin_numleaffaces;
unsigned short		*sin_dleaffaces;//[SIN_MAX_MAP_LEAFFACES];

int					sin_numleafbrushes;
unsigned short		*sin_dleafbrushes;//[SIN_MAX_MAP_LEAFBRUSHES];

int					sin_numsurfedges;
int					*sin_dsurfedges;//[SIN_MAX_MAP_SURFEDGES];

int					sin_numbrushes;
sin_dbrush_t		*sin_dbrushes;//[SIN_MAX_MAP_BRUSHES];

int					sin_numbrushsides;
sin_dbrushside_t	*sin_dbrushsides;//[SIN_MAX_MAP_BRUSHSIDES];

int					sin_numareas;
sin_darea_t			*sin_dareas;//[SIN_MAX_MAP_AREAS];

int					sin_numareaportals;
sin_dareaportal_t	*sin_dareaportals;//[SIN_MAX_MAP_AREAPORTALS];

int					sin_numlightinfo;
sin_lightvalue_t	*sin_lightinfo;//[SIN_MAX_MAP_LIGHTINFO];

byte				sin_dpop[256];

char				sin_dbrushsidetextured[SIN_MAX_MAP_BRUSHSIDES];

int sin_bspallocated = false;
int sin_allocatedbspmem = 0;

void Sin_AllocMaxBSP(void)
{
	//models
	sin_nummodels = 0;
	sin_dmodels = (sin_dmodel_t *) GetClearedMemory(SIN_MAX_MAP_MODELS * sizeof(sin_dmodel_t));
	sin_allocatedbspmem += SIN_MAX_MAP_MODELS * sizeof(sin_dmodel_t);
	//vis data
	sin_visdatasize = 0;
	sin_dvisdata = (byte *) GetClearedMemory(SIN_MAX_MAP_VISIBILITY * sizeof(byte));
	sin_dvis = (sin_dvis_t *) sin_dvisdata;
	sin_allocatedbspmem += SIN_MAX_MAP_VISIBILITY * sizeof(byte);
	//light data
	sin_lightdatasize = 0;
	sin_dlightdata = (byte *) GetClearedMemory(SIN_MAX_MAP_LIGHTING * sizeof(byte));
	sin_allocatedbspmem += SIN_MAX_MAP_LIGHTING * sizeof(byte);
	//entity data
	sin_entdatasize = 0;
	sin_dentdata = (char *) GetClearedMemory(SIN_MAX_MAP_ENTSTRING * sizeof(char));
	sin_allocatedbspmem += SIN_MAX_MAP_ENTSTRING * sizeof(char);
	//leafs
	sin_numleafs = 0;
	sin_dleafs = (sin_dleaf_t *) GetClearedMemory(SIN_MAX_MAP_LEAFS * sizeof(sin_dleaf_t));
	sin_allocatedbspmem += SIN_MAX_MAP_LEAFS * sizeof(sin_dleaf_t);
	//planes
	sin_numplanes = 0;
	sin_dplanes = (sin_dplane_t *) GetClearedMemory(SIN_MAX_MAP_PLANES * sizeof(sin_dplane_t));
	sin_allocatedbspmem += SIN_MAX_MAP_PLANES * sizeof(sin_dplane_t);
	//vertexes
	sin_numvertexes = 0;
	sin_dvertexes = (sin_dvertex_t *) GetClearedMemory(SIN_MAX_MAP_VERTS * sizeof(sin_dvertex_t));
	sin_allocatedbspmem += SIN_MAX_MAP_VERTS * sizeof(sin_dvertex_t);
	//nodes
	sin_numnodes = 0;
	sin_dnodes = (sin_dnode_t *) GetClearedMemory(SIN_MAX_MAP_NODES * sizeof(sin_dnode_t));
	sin_allocatedbspmem += SIN_MAX_MAP_NODES * sizeof(sin_dnode_t);
	//texture info
	sin_numtexinfo = 0;
	sin_texinfo = (sin_texinfo_t *) GetClearedMemory(SIN_MAX_MAP_TEXINFO * sizeof(sin_texinfo_t));
	sin_allocatedbspmem += SIN_MAX_MAP_TEXINFO * sizeof(sin_texinfo_t);
	//faces
	sin_numfaces = 0;
	sin_dfaces = (sin_dface_t *) GetClearedMemory(SIN_MAX_MAP_FACES * sizeof(sin_dface_t));
	sin_allocatedbspmem += SIN_MAX_MAP_FACES * sizeof(sin_dface_t);
	//edges
	sin_numedges = 0;
	sin_dedges = (sin_dedge_t *) GetClearedMemory(SIN_MAX_MAP_EDGES * sizeof(sin_dedge_t));
	sin_allocatedbspmem += SIN_MAX_MAP_EDGES * sizeof(sin_dedge_t);
	//leaf faces
	sin_numleaffaces = 0;
	sin_dleaffaces = (unsigned short *) GetClearedMemory(SIN_MAX_MAP_LEAFFACES * sizeof(unsigned short));
	sin_allocatedbspmem += SIN_MAX_MAP_LEAFFACES * sizeof(unsigned short);
	//leaf brushes
	sin_numleafbrushes = 0;
	sin_dleafbrushes = (unsigned short *) GetClearedMemory(SIN_MAX_MAP_LEAFBRUSHES * sizeof(unsigned short));
	sin_allocatedbspmem += SIN_MAX_MAP_LEAFBRUSHES * sizeof(unsigned short);
	//surface edges
	sin_numsurfedges = 0;
	sin_dsurfedges = (int *) GetClearedMemory(SIN_MAX_MAP_SURFEDGES * sizeof(int));
	sin_allocatedbspmem += SIN_MAX_MAP_SURFEDGES * sizeof(int);
	//brushes
	sin_numbrushes = 0;
	sin_dbrushes = (sin_dbrush_t *) GetClearedMemory(SIN_MAX_MAP_BRUSHES * sizeof(sin_dbrush_t));
	sin_allocatedbspmem += SIN_MAX_MAP_BRUSHES * sizeof(sin_dbrush_t);
	//brushsides
	sin_numbrushsides = 0;
	sin_dbrushsides = (sin_dbrushside_t *) GetClearedMemory(SIN_MAX_MAP_BRUSHSIDES * sizeof(sin_dbrushside_t));
	sin_allocatedbspmem += SIN_MAX_MAP_BRUSHSIDES * sizeof(sin_dbrushside_t);
	//areas
	sin_numareas = 0;
	sin_dareas = (sin_darea_t *) GetClearedMemory(SIN_MAX_MAP_AREAS * sizeof(sin_darea_t));
	sin_allocatedbspmem += SIN_MAX_MAP_AREAS * sizeof(sin_darea_t);
	//area portals
	sin_numareaportals = 0;
	sin_dareaportals = (sin_dareaportal_t *) GetClearedMemory(SIN_MAX_MAP_AREAPORTALS * sizeof(sin_dareaportal_t));
	sin_allocatedbspmem += SIN_MAX_MAP_AREAPORTALS * sizeof(sin_dareaportal_t);
	//light info
	sin_numlightinfo = 0;
	sin_lightinfo = (sin_lightvalue_t *) GetClearedMemory(SIN_MAX_MAP_LIGHTINFO * sizeof(sin_lightvalue_t));
	sin_allocatedbspmem += SIN_MAX_MAP_LIGHTINFO * sizeof(sin_lightvalue_t);
	//print allocated memory
	Log_Print("allocated ");
	PrintMemorySize(sin_allocatedbspmem);
	Log_Print(" of BSP memory\n");
} //end of the function Sin_AllocMaxBSP

void Sin_FreeMaxBSP(void)
{
	//models
	sin_nummodels = 0;
	FreeMemory(sin_dmodels);
	sin_dmodels = NULL;
	//vis data
	sin_visdatasize = 0;
	FreeMemory(sin_dvisdata);
	sin_dvisdata = NULL;
	sin_dvis = NULL;
	//light data
	sin_lightdatasize = 0;
	FreeMemory(sin_dlightdata);
	sin_dlightdata = NULL;
	//entity data
	sin_entdatasize = 0;
	FreeMemory(sin_dentdata);
	sin_dentdata = NULL;
	//leafs
	sin_numleafs = 0;
	FreeMemory(sin_dleafs);
	sin_dleafs = NULL;
	//planes
	sin_numplanes = 0;
	FreeMemory(sin_dplanes);
	sin_dplanes = NULL;
	//vertexes
	sin_numvertexes = 0;
	FreeMemory(sin_dvertexes);
	sin_dvertexes = NULL;
	//nodes
	sin_numnodes = 0;
	FreeMemory(sin_dnodes);
	sin_dnodes = NULL;
	//texture info
	sin_numtexinfo = 0;
	FreeMemory(sin_texinfo);
	sin_texinfo = NULL;
	//faces
	sin_numfaces = 0;
	FreeMemory(sin_dfaces);
	sin_dfaces = NULL;
	//edges
	sin_numedges = 0;
	FreeMemory(sin_dedges);
	sin_dedges = NULL;
	//leaf faces
	sin_numleaffaces = 0;
	FreeMemory(sin_dleaffaces);
	sin_dleaffaces = NULL;
	//leaf brushes
	sin_numleafbrushes = 0;
	FreeMemory(sin_dleafbrushes);
	sin_dleafbrushes = NULL;
	//surface edges
	sin_numsurfedges = 0;
	FreeMemory(sin_dsurfedges);
	sin_dsurfedges = NULL;
	//brushes
	sin_numbrushes = 0;
	FreeMemory(sin_dbrushes);
	sin_dbrushes = NULL;
	//brushsides
	sin_numbrushsides = 0;
	FreeMemory(sin_dbrushsides);
	sin_dbrushsides = NULL;
	//areas
	sin_numareas = 0;
	FreeMemory(sin_dareas);
	sin_dareas = NULL;
	//area portals
	sin_numareaportals = 0;
	FreeMemory(sin_dareaportals);
	sin_dareaportals = NULL;
	//light info
	sin_numlightinfo = 0;
	FreeMemory(sin_lightinfo);
	sin_lightinfo = NULL;
	//
	Log_Print("freed ");
	PrintMemorySize(sin_allocatedbspmem);
	Log_Print(" of BSP memory\n");
	sin_allocatedbspmem = 0;
} //end of the function Sin_FreeMaxBSP

#define WCONVEX_EPSILON		0.5

//===========================================================================
// returns the amount the face and the winding overlap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Sin_FaceOnWinding(sin_dface_t *face, winding_t *winding)
{
	int i, edgenum, side;
	float dist, area;
	sin_dplane_t plane;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	winding_t *w;

	//
	w = CopyWinding(winding);
	memcpy(&plane, &sin_dplanes[face->planenum], sizeof(sin_dplane_t));
	//check on which side of the plane the face is
	if (face->side)
	{
		VectorNegate(plane.normal, plane.normal);
		plane.dist = -plane.dist;
	} //end if
	for (i = 0; i < face->numedges && w; i++)
	{
		//get the first and second vertex of the edge
		edgenum = sin_dsurfedges[face->firstedge + i];
		side = edgenum > 0;
		//if the face plane is flipped
		v1 = sin_dvertexes[sin_dedges[abs(edgenum)].v[side]].point;
		v2 = sin_dvertexes[sin_dedges[abs(edgenum)].v[!side]].point;
		//create a plane through the edge vector, orthogonal to the face plane
		//and with the normal vector pointing out of the face
		VectorSubtract(v1, v2, edgevec);
		CrossProduct(edgevec, plane.normal, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		ChopWindingInPlace(&w, normal, dist, 0.9); //CLIP_EPSILON
	} //end for
	if (w)
	{
		area = WindingArea(w);
		FreeWinding(w);
		return area;
	} //end if
	return 0;
} //end of the function Sin_FaceOnWinding
//===========================================================================
// creates a winding for the given brush side on the given brush
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
winding_t *Sin_BrushSideWinding(sin_dbrush_t *brush, sin_dbrushside_t *baseside)
{
	int i;
	sin_dplane_t *baseplane, *plane;
	sin_dbrushside_t *side;
	winding_t *w;
	
	//create a winding for the brush side with the given planenumber
	baseplane = &sin_dplanes[baseside->planenum];
	w = BaseWindingForPlane(baseplane->normal, baseplane->dist);
	for (i = 0; i < brush->numsides && w; i++)
	{
		side = &sin_dbrushsides[brush->firstside + i];
		//don't chop with the base plane
		if (side->planenum == baseside->planenum) continue;
		//also don't use planes that are almost equal
		plane = &sin_dplanes[side->planenum];
		if (DotProduct(baseplane->normal, plane->normal) > 0.999
				&& fabs(baseplane->dist - plane->dist) < 0.01) continue;
		//
		plane = &sin_dplanes[side->planenum^1];
		ChopWindingInPlace(&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
	} //end for
	return w;
} //end of the function Sin_BrushSideWinding
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Sin_HintSkipBrush(sin_dbrush_t *brush)
{
	int j;
	sin_dbrushside_t *brushside;

	for (j = 0; j < brush->numsides; j++)
	{
		brushside = &sin_dbrushsides[brush->firstside + j];
		if (brushside->texinfo > 0)
		{
			if (sin_texinfo[brushside->texinfo].flags & (SURF_SKIP|SURF_HINT))
			{
				return true;
			} //end if
		} //end if
	} //end for
	return false;
} //end of the function Sin_HintSkipBrush
//===========================================================================
// fix screwed brush texture references
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WindingIsTiny(winding_t *w);

void Sin_FixTextureReferences(void)
{
	int i, j, k, we;
	sin_dbrushside_t *brushside;
	sin_dbrush_t *brush;
	sin_dface_t *face;
	winding_t *w;

	memset(sin_dbrushsidetextured, false, SIN_MAX_MAP_BRUSHSIDES);
	//go over all the brushes
   for (i = 0; i < sin_numbrushes; i++)
   {
		brush = &sin_dbrushes[i];
		//hint brushes are not textured
		if (Sin_HintSkipBrush(brush)) continue;
		//go over all the sides of the brush
		for (j = 0; j < brush->numsides; j++)
		{
			brushside = &sin_dbrushsides[brush->firstside + j];
			//
			w = Sin_BrushSideWinding(brush, brushside);
			if (!w)
			{
				sin_dbrushsidetextured[brush->firstside + j] = true;
				continue;
			} //end if
			else
			{
				//RemoveEqualPoints(w, 0.2);
				if (WindingIsTiny(w))
				{
					FreeWinding(w);
					sin_dbrushsidetextured[brush->firstside + j] = true;
					continue;
				} //end if
				else
				{
					we = WindingError(w);
					if (we == WE_NOTENOUGHPOINTS
						|| we == WE_SMALLAREA
						|| we == WE_POINTBOGUSRANGE
//						|| we == WE_NONCONVEX
						)
					{
						FreeWinding(w);
						sin_dbrushsidetextured[brush->firstside + j] = true;
						continue;
					} //end if
				} //end else
			} //end else
			if (WindingArea(w) < 20)
			{
				sin_dbrushsidetextured[brush->firstside + j] = true;
			} //end if
			//find a face for texturing this brush
			for (k = 0; k < sin_numfaces; k++)
			{
				face = &sin_dfaces[k];
				//if the face is in the same plane as the brush side
				if ((face->planenum&~1) != (brushside->planenum&~1)) continue;
				//if the face is partly or totally on the brush side
				if (Sin_FaceOnWinding(face, w))
				{
					brushside->texinfo = face->texinfo;
					sin_dbrushsidetextured[brush->firstside + j] = true;
					break;
				} //end if
			} //end for
			FreeWinding(w);
		} //end for
	} //end for
} //end of the function Sin_FixTextureReferences*/

/*
===============
CompressVis

===============
*/
int Sin_CompressVis (byte *vis, byte *dest)
{
	int		j;
	int		rep;
	int		visrow;
	byte	*dest_p;
	
	dest_p = dest;
//	visrow = (r_numvisleafs + 7)>>3;
	visrow = (sin_dvis->numclusters + 7)>>3;
	
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
} //end of the function Sin_CompressVis


/*
===================
DecompressVis
===================
*/
void Sin_DecompressVis (byte *in, byte *decompressed)
{
	int		c;
	byte	*out;
	int		row;

//	row = (r_numvisleafs+7)>>3;	
	row = (sin_dvis->numclusters+7)>>3;	
	out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
	
		c = in[1];
		if (!c)
			Error ("DecompressVis: 0 repeat");
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
} //end of the function Sin_DecompressVis

//=============================================================================

/*
=============
Sin_SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void Sin_SwapBSPFile (qboolean todisk)
{
	int				i, j;
	sin_dmodel_t		*d;

	
// models	
	for (i=0 ; i<sin_nummodels ; i++)
	{
		d = &sin_dmodels[i];

		d->firstface = LittleLong (d->firstface);
		d->numfaces = LittleLong (d->numfaces);
		d->headnode = LittleLong (d->headnode);
		
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
	for (i=0 ; i<sin_numvertexes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			sin_dvertexes[i].point[j] = LittleFloat (sin_dvertexes[i].point[j]);
	}
		
//
// planes
//	
	for (i=0 ; i<sin_numplanes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			sin_dplanes[i].normal[j] = LittleFloat (sin_dplanes[i].normal[j]);
		sin_dplanes[i].dist = LittleFloat (sin_dplanes[i].dist);
		sin_dplanes[i].type = LittleLong (sin_dplanes[i].type);
	}
	
//
// sin_texinfos
//	
	for (i = 0; i < sin_numtexinfo; i++)
	{
		for (j=0 ; j<8 ; j++)
			sin_texinfo[i].vecs[0][j] = LittleFloat (sin_texinfo[i].vecs[0][j]);
#ifdef SIN
      sin_texinfo[i].trans_mag = LittleFloat( sin_texinfo[i].trans_mag );     
      sin_texinfo[i].trans_angle = LittleLong( sin_texinfo[i].trans_angle );     
      sin_texinfo[i].animtime = LittleFloat( sin_texinfo[i].animtime );     
      sin_texinfo[i].nonlit = LittleFloat( sin_texinfo[i].nonlit );     
      sin_texinfo[i].translucence = LittleFloat( sin_texinfo[i].translucence );     
      sin_texinfo[i].friction = LittleFloat( sin_texinfo[i].friction );     
      sin_texinfo[i].restitution = LittleFloat( sin_texinfo[i].restitution );     
		sin_texinfo[i].flags = LittleUnsigned (sin_texinfo[i].flags);
#else
		sin_texinfo[i].value = LittleLong (sin_texinfo[i].value);
		sin_texinfo[i].flags = LittleLong (sin_texinfo[i].flags);
#endif
		sin_texinfo[i].nexttexinfo = LittleLong (sin_texinfo[i].nexttexinfo);
	}

#ifdef SIN
//
// lightinfos
//	
	for (i = 0; i < sin_numlightinfo; i++)
	{
		for (j=0 ; j<3 ; j++)
         {
			sin_lightinfo[i].color[j] = LittleFloat (sin_lightinfo[i].color[j]);
         }
		sin_lightinfo[i].value = LittleLong (sin_lightinfo[i].value);
      sin_lightinfo[i].direct = LittleFloat( sin_lightinfo[i].direct );     
      sin_lightinfo[i].directangle = LittleFloat( sin_lightinfo[i].directangle );     
      sin_lightinfo[i].directstyle = LittleFloat( sin_lightinfo[i].directstyle );     
	}
#endif
	
//
// faces
//
	for (i=0 ; i<sin_numfaces ; i++)
	{
		sin_dfaces[i].texinfo = LittleShort (sin_dfaces[i].texinfo);
#ifdef SIN
		sin_dfaces[i].lightinfo = LittleLong (sin_dfaces[i].lightinfo);
		sin_dfaces[i].planenum = LittleUnsignedShort (sin_dfaces[i].planenum);
#else
		sin_dfaces[i].planenum = LittleShort (sin_dfaces[i].planenum);
#endif
		sin_dfaces[i].side = LittleShort (sin_dfaces[i].side);
		sin_dfaces[i].lightofs = LittleLong (sin_dfaces[i].lightofs);
		sin_dfaces[i].firstedge = LittleLong (sin_dfaces[i].firstedge);
		sin_dfaces[i].numedges = LittleShort (sin_dfaces[i].numedges);
	}

//
// nodes
//
	for (i=0 ; i<sin_numnodes ; i++)
	{
		sin_dnodes[i].planenum = LittleLong (sin_dnodes[i].planenum);
		for (j=0 ; j<3 ; j++)
		{
			sin_dnodes[i].mins[j] = LittleShort (sin_dnodes[i].mins[j]);
			sin_dnodes[i].maxs[j] = LittleShort (sin_dnodes[i].maxs[j]);
		}
		sin_dnodes[i].children[0] = LittleLong (sin_dnodes[i].children[0]);
		sin_dnodes[i].children[1] = LittleLong (sin_dnodes[i].children[1]);
#ifdef SIN
		sin_dnodes[i].firstface = LittleUnsignedShort (sin_dnodes[i].firstface);
		sin_dnodes[i].numfaces = LittleUnsignedShort (sin_dnodes[i].numfaces);
#else
		sin_dnodes[i].firstface = LittleShort (sin_dnodes[i].firstface);
		sin_dnodes[i].numfaces = LittleShort (sin_dnodes[i].numfaces);
#endif
	}

//
// leafs
//
	for (i=0 ; i<sin_numleafs ; i++)
	{
		sin_dleafs[i].contents = LittleLong (sin_dleafs[i].contents);
		sin_dleafs[i].cluster = LittleShort (sin_dleafs[i].cluster);
		sin_dleafs[i].area = LittleShort (sin_dleafs[i].area);
		for (j=0 ; j<3 ; j++)
		{
			sin_dleafs[i].mins[j] = LittleShort (sin_dleafs[i].mins[j]);
			sin_dleafs[i].maxs[j] = LittleShort (sin_dleafs[i].maxs[j]);
		}
#ifdef SIN
		sin_dleafs[i].firstleafface = LittleUnsignedShort (sin_dleafs[i].firstleafface);
		sin_dleafs[i].numleaffaces = LittleUnsignedShort (sin_dleafs[i].numleaffaces);
		sin_dleafs[i].firstleafbrush = LittleUnsignedShort (sin_dleafs[i].firstleafbrush);
		sin_dleafs[i].numleafbrushes = LittleUnsignedShort (sin_dleafs[i].numleafbrushes);
#else
		sin_dleafs[i].firstleafface = LittleShort (sin_dleafs[i].firstleafface);
		sin_dleafs[i].numleaffaces = LittleShort (sin_dleafs[i].numleaffaces);
		sin_dleafs[i].firstleafbrush = LittleShort (sin_dleafs[i].firstleafbrush);
		sin_dleafs[i].numleafbrushes = LittleShort (sin_dleafs[i].numleafbrushes);
#endif
	}

//
// leaffaces
//
	for (i=0 ; i<sin_numleaffaces ; i++)
		sin_dleaffaces[i] = LittleShort (sin_dleaffaces[i]);

//
// leafbrushes
//
	for (i=0 ; i<sin_numleafbrushes ; i++)
		sin_dleafbrushes[i] = LittleShort (sin_dleafbrushes[i]);

//
// surfedges
//
	for (i=0 ; i<sin_numsurfedges ; i++)
		sin_dsurfedges[i] = LittleLong (sin_dsurfedges[i]);

//
// edges
//
	for (i=0 ; i<sin_numedges ; i++)
	{
#ifdef SIN
		sin_dedges[i].v[0] = LittleUnsignedShort (sin_dedges[i].v[0]);
		sin_dedges[i].v[1] = LittleUnsignedShort (sin_dedges[i].v[1]);
#else
		sin_dedges[i].v[0] = LittleShort (sin_dedges[i].v[0]);
		sin_dedges[i].v[1] = LittleShort (sin_dedges[i].v[1]);
#endif
	}

//
// brushes
//
	for (i=0 ; i<sin_numbrushes ; i++)
	{
		sin_dbrushes[i].firstside = LittleLong (sin_dbrushes[i].firstside);
		sin_dbrushes[i].numsides = LittleLong (sin_dbrushes[i].numsides);
		sin_dbrushes[i].contents = LittleLong (sin_dbrushes[i].contents);
	}

//
// areas
//
	for (i=0 ; i<sin_numareas ; i++)
	{
		sin_dareas[i].numareaportals = LittleLong (sin_dareas[i].numareaportals);
		sin_dareas[i].firstareaportal = LittleLong (sin_dareas[i].firstareaportal);
	}

//
// areasportals
//
	for (i=0 ; i<sin_numareaportals ; i++)
	{
		sin_dareaportals[i].portalnum = LittleLong (sin_dareaportals[i].portalnum);
		sin_dareaportals[i].otherarea = LittleLong (sin_dareaportals[i].otherarea);
	}

//
// brushsides
//
	for (i=0 ; i<sin_numbrushsides ; i++)
	{
#ifdef SIN
		sin_dbrushsides[i].planenum = LittleUnsignedShort (sin_dbrushsides[i].planenum);
#else
		sin_dbrushsides[i].planenum = LittleShort (sin_dbrushsides[i].planenum);
#endif
		sin_dbrushsides[i].texinfo = LittleShort (sin_dbrushsides[i].texinfo);
#ifdef SIN
		sin_dbrushsides[i].lightinfo = LittleLong (sin_dbrushsides[i].lightinfo);
#endif
	}

//
// visibility
//
	if (todisk)
		j = sin_dvis->numclusters;
	else
		j = LittleLong(sin_dvis->numclusters);
	sin_dvis->numclusters = LittleLong (sin_dvis->numclusters);
	for (i=0 ; i<j ; i++)
	{
		sin_dvis->bitofs[i][0] = LittleLong (sin_dvis->bitofs[i][0]);
		sin_dvis->bitofs[i][1] = LittleLong (sin_dvis->bitofs[i][1]);
	}
} //end of the function Sin_SwapBSPFile


sin_dheader_t	*header;
#ifdef SIN
int Sin_CopyLump (int lump, void *dest, int size, int maxsize)
{
	int		length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % size)
		Error ("Sin_LoadBSPFile: odd lump size");

   if ((length/size) > maxsize)
      Error ("Sin_LoadBSPFile: exceeded max size for lump %d size %d > maxsize %d\n", lump, (length/size), maxsize );
	
	memcpy (dest, (byte *)header + ofs, length);

	return length / size;
}
#else
int Sin_CopyLump (int lump, void *dest, int size)
{
	int		length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % size)
		Error ("Sin_LoadBSPFile: odd lump size");
	
	memcpy (dest, (byte *)header + ofs, length);

	return length / size;
}
#endif

/*
=============
Sin_LoadBSPFile
=============
*/
void	Sin_LoadBSPFile(char *filename, int offset, int length)
{
	int			i;
	
//
// load the file header
//
	LoadFile (filename, (void **)&header, offset, length);

// swap the header
	for (i=0 ; i< sizeof(sin_dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != SIN_BSPHEADER && header->ident != SINGAME_BSPHEADER)
		Error ("%s is not a IBSP file", filename);
	if (header->version != SIN_BSPVERSION && header->version != SINGAME_BSPVERSION)
		Error ("%s is version %i, not %i", filename, header->version, SIN_BSPVERSION);

#ifdef SIN
	sin_nummodels = Sin_CopyLump (SIN_LUMP_MODELS, sin_dmodels, sizeof(sin_dmodel_t), SIN_MAX_MAP_MODELS);
	sin_numvertexes = Sin_CopyLump (SIN_LUMP_VERTEXES, sin_dvertexes, sizeof(sin_dvertex_t), SIN_MAX_MAP_VERTS);
	sin_numplanes = Sin_CopyLump (SIN_LUMP_PLANES, sin_dplanes, sizeof(sin_dplane_t), SIN_MAX_MAP_PLANES);
	sin_numleafs = Sin_CopyLump (SIN_LUMP_LEAFS, sin_dleafs, sizeof(sin_dleaf_t), SIN_MAX_MAP_LEAFS);
	sin_numnodes = Sin_CopyLump (SIN_LUMP_NODES, sin_dnodes, sizeof(sin_dnode_t), SIN_MAX_MAP_NODES);
	sin_numtexinfo = Sin_CopyLump (SIN_LUMP_TEXINFO, sin_texinfo, sizeof(sin_texinfo_t), SIN_MAX_MAP_TEXINFO);
	sin_numfaces = Sin_CopyLump (SIN_LUMP_FACES, sin_dfaces, sizeof(sin_dface_t), SIN_MAX_MAP_FACES);
	sin_numleaffaces = Sin_CopyLump (SIN_LUMP_LEAFFACES, sin_dleaffaces, sizeof(sin_dleaffaces[0]), SIN_MAX_MAP_LEAFFACES);
	sin_numleafbrushes = Sin_CopyLump (SIN_LUMP_LEAFBRUSHES, sin_dleafbrushes, sizeof(sin_dleafbrushes[0]), SIN_MAX_MAP_LEAFBRUSHES);
	sin_numsurfedges = Sin_CopyLump (SIN_LUMP_SURFEDGES, sin_dsurfedges, sizeof(sin_dsurfedges[0]), SIN_MAX_MAP_SURFEDGES);
	sin_numedges = Sin_CopyLump (SIN_LUMP_EDGES, sin_dedges, sizeof(sin_dedge_t), SIN_MAX_MAP_EDGES);
	sin_numbrushes = Sin_CopyLump (SIN_LUMP_BRUSHES, sin_dbrushes, sizeof(sin_dbrush_t), SIN_MAX_MAP_BRUSHES);
	sin_numbrushsides = Sin_CopyLump (SIN_LUMP_BRUSHSIDES, sin_dbrushsides, sizeof(sin_dbrushside_t), SIN_MAX_MAP_BRUSHSIDES);
	sin_numareas = Sin_CopyLump (SIN_LUMP_AREAS, sin_dareas, sizeof(sin_darea_t), SIN_MAX_MAP_AREAS);
	sin_numareaportals = Sin_CopyLump (SIN_LUMP_AREAPORTALS, sin_dareaportals, sizeof(sin_dareaportal_t), SIN_MAX_MAP_AREAPORTALS);
	sin_numlightinfo = Sin_CopyLump (SIN_LUMP_LIGHTINFO, sin_lightinfo, sizeof(sin_lightvalue_t), SIN_MAX_MAP_LIGHTINFO);

	sin_visdatasize = Sin_CopyLump (SIN_LUMP_VISIBILITY, sin_dvisdata, 1, SIN_MAX_MAP_VISIBILITY);
	sin_lightdatasize = Sin_CopyLump (SIN_LUMP_LIGHTING, sin_dlightdata, 1, SIN_MAX_MAP_LIGHTING);
	sin_entdatasize = Sin_CopyLump (SIN_LUMP_ENTITIES, sin_dentdata, 1, SIN_MAX_MAP_ENTSTRING);

	Sin_CopyLump (SIN_LUMP_POP, sin_dpop, 1, sizeof(sin_dpop));
#else
	sin_nummodels = Sin_CopyLump (SIN_LUMP_MODELS, sin_dmodels, sizeof(sin_dmodel_t));
	sin_numvertexes = Sin_CopyLump (SIN_LUMP_VERTEXES, sin_dvertexes, sizeof(sin_dvertex_t));
	sin_numplanes = Sin_CopyLump (SIN_LUMP_PLANES, sin_dplanes, sizeof(sin_dplane_t));
	sin_numleafs = Sin_CopyLump (SIN_LUMP_LEAFS, sin_dleafs, sizeof(sin_dleaf_t));
	sin_numnodes = Sin_CopyLump (SIN_LUMP_NODES, sin_dnodes, sizeof(sin_dnode_t));
	sin_numtexinfo = Sin_CopyLump (SIN_LUMP_TEXINFO, sin_texinfo, sizeof(sin_texinfo_t));
	sin_numfaces = Sin_CopyLump (SIN_LUMP_FACES, sin_dfaces, sizeof(sin_dface_t));
	sin_numleaffaces = Sin_CopyLump (SIN_LUMP_LEAFFACES, sin_dleaffaces, sizeof(sin_dleaffaces[0]));
	sin_numleafbrushes = Sin_CopyLump (SIN_LUMP_LEAFBRUSHES, sin_dleafbrushes, sizeof(sin_dleafbrushes[0]));
	sin_numsurfedges = Sin_CopyLump (SIN_LUMP_SURFEDGES, sin_dsurfedges, sizeof(sin_dsurfedges[0]));
	sin_numedges = Sin_CopyLump (SIN_LUMP_EDGES, sin_dedges, sizeof(sin_dedge_t));
	sin_numbrushes = Sin_CopyLump (SIN_LUMP_BRUSHES, sin_dbrushes, sizeof(sin_dbrush_t));
	sin_numbrushsides = Sin_CopyLump (SIN_LUMP_BRUSHSIDES, sin_dbrushsides, sizeof(sin_dbrushside_t));
	sin_numareas = Sin_CopyLump (SIN_LUMP_AREAS, sin_dareas, sizeof(sin_darea_t));
	sin_numareaportals = Sin_CopyLump (SIN_LUMP_AREAPORTALS, sin_dareaportals, sizeof(sin_dareaportal_t));

	sin_visdatasize = Sin_CopyLump (SIN_LUMP_VISIBILITY, sin_dvisdata, 1);
	sin_lightdatasize = Sin_CopyLump (SIN_LUMP_LIGHTING, sin_dlightdata, 1);
	sin_entdatasize = Sin_CopyLump (SIN_LUMP_ENTITIES, sin_dentdata, 1);

	Sin_CopyLump (SIN_LUMP_POP, sin_dpop, 1);
#endif

	FreeMemory(header);		// everything has been copied out
		
//
// swap everything
//	
	Sin_SwapBSPFile (false);
} //end of the function Sin_LoadBSPFile

/*
=============
Sin_LoadBSPFilesTexinfo

Only loads the sin_texinfo lump, so qdata can scan for textures
=============
*/
void	Sin_LoadBSPFileTexinfo (char *filename)
{
	int			i;
	FILE		*f;
	int		length, ofs;

	header = GetMemory(sizeof(sin_dheader_t));

	f = fopen (filename, "rb");
	fread (header, sizeof(sin_dheader_t), 1, f);

// swap the header
	for (i=0 ; i< sizeof(sin_dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != SIN_BSPHEADER && header->ident != SINGAME_BSPHEADER)
		Error ("%s is not a IBSP file", filename);
	if (header->version != SIN_BSPVERSION && header->version != SINGAME_BSPVERSION)
		Error ("%s is version %i, not %i", filename, header->version, SIN_BSPVERSION);


	length = header->lumps[SIN_LUMP_TEXINFO].filelen;
	ofs = header->lumps[SIN_LUMP_TEXINFO].fileofs;

	fseek (f, ofs, SEEK_SET);
	fread (sin_texinfo, length, 1, f);
	fclose (f);

	sin_numtexinfo = length / sizeof(sin_texinfo_t);

	FreeMemory(header);		// everything has been copied out
		
	Sin_SwapBSPFile (false);
} //end of the function Sin_LoadBSPFilesTexinfo


//============================================================================

FILE		*wadfile;
sin_dheader_t	outheader;

#ifdef SIN
void Sin_AddLump (int lumpnum, void *data, int len, int size, int maxsize)
{
	sin_lump_t *lump;
	int totallength;

	totallength = len*size;

	if (len > maxsize)
		Error ("Sin_WriteBSPFile: exceeded max size for lump %d size %d > maxsize %d\n", lumpnum, len, maxsize );

	lump = &header->lumps[lumpnum];
	
	lump->fileofs = LittleLong( ftell(wadfile) );
	lump->filelen = LittleLong(totallength);
	SafeWrite (wadfile, data, (totallength+3)&~3);
}
#else
void Sin_AddLump (int lumpnum, void *data, int len)
{
	sin_lump_t *lump;

	lump = &header->lumps[lumpnum];
	
	lump->fileofs = LittleLong( ftell(wadfile) );
	lump->filelen = LittleLong(len);
	SafeWrite (wadfile, data, (len+3)&~3);
}
#endif
/*
=============
Sin_WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void	Sin_WriteBSPFile (char *filename)
{		
	header = &outheader;
	memset (header, 0, sizeof(sin_dheader_t));
	
	Sin_SwapBSPFile (true);

	header->ident = LittleLong (SIN_BSPHEADER);
	header->version = LittleLong (SIN_BSPVERSION);
	
	wadfile = SafeOpenWrite (filename);
	SafeWrite (wadfile, header, sizeof(sin_dheader_t));	// overwritten later

#ifdef SIN
	Sin_AddLump (SIN_LUMP_PLANES, sin_dplanes, sin_numplanes, sizeof(sin_dplane_t), SIN_MAX_MAP_PLANES);
	Sin_AddLump (SIN_LUMP_LEAFS, sin_dleafs, sin_numleafs, sizeof(sin_dleaf_t), SIN_MAX_MAP_LEAFS);
	Sin_AddLump (SIN_LUMP_VERTEXES, sin_dvertexes, sin_numvertexes, sizeof(sin_dvertex_t), SIN_MAX_MAP_VERTS);
	Sin_AddLump (SIN_LUMP_NODES, sin_dnodes, sin_numnodes, sizeof(sin_dnode_t), SIN_MAX_MAP_NODES);
	Sin_AddLump (SIN_LUMP_TEXINFO, sin_texinfo, sin_numtexinfo, sizeof(sin_texinfo_t), SIN_MAX_MAP_TEXINFO);
	Sin_AddLump (SIN_LUMP_FACES, sin_dfaces, sin_numfaces, sizeof(sin_dface_t), SIN_MAX_MAP_FACES);
	Sin_AddLump (SIN_LUMP_BRUSHES, sin_dbrushes, sin_numbrushes, sizeof(sin_dbrush_t), SIN_MAX_MAP_BRUSHES);
	Sin_AddLump (SIN_LUMP_BRUSHSIDES, sin_dbrushsides, sin_numbrushsides, sizeof(sin_dbrushside_t), SIN_MAX_MAP_BRUSHSIDES);
	Sin_AddLump (SIN_LUMP_LEAFFACES, sin_dleaffaces, sin_numleaffaces, sizeof(sin_dleaffaces[0]), SIN_MAX_MAP_LEAFFACES);
	Sin_AddLump (SIN_LUMP_LEAFBRUSHES, sin_dleafbrushes, sin_numleafbrushes, sizeof(sin_dleafbrushes[0]), SIN_MAX_MAP_LEAFBRUSHES);
	Sin_AddLump (SIN_LUMP_SURFEDGES, sin_dsurfedges, sin_numsurfedges, sizeof(sin_dsurfedges[0]), SIN_MAX_MAP_SURFEDGES);
	Sin_AddLump (SIN_LUMP_EDGES, sin_dedges, sin_numedges, sizeof(sin_dedge_t), SIN_MAX_MAP_EDGES);
	Sin_AddLump (SIN_LUMP_MODELS, sin_dmodels, sin_nummodels, sizeof(sin_dmodel_t), SIN_MAX_MAP_MODELS);
	Sin_AddLump (SIN_LUMP_AREAS, sin_dareas, sin_numareas, sizeof(sin_darea_t), SIN_MAX_MAP_AREAS);
	Sin_AddLump (SIN_LUMP_AREAPORTALS, sin_dareaportals, sin_numareaportals, sizeof(sin_dareaportal_t), SIN_MAX_MAP_AREAPORTALS);
	Sin_AddLump (SIN_LUMP_LIGHTINFO, sin_lightinfo, sin_numlightinfo, sizeof(sin_lightvalue_t), SIN_MAX_MAP_LIGHTINFO);

	Sin_AddLump (SIN_LUMP_LIGHTING, sin_dlightdata, sin_lightdatasize, 1, SIN_MAX_MAP_LIGHTING);
	Sin_AddLump (SIN_LUMP_VISIBILITY, sin_dvisdata, sin_visdatasize, 1, SIN_MAX_MAP_VISIBILITY);
	Sin_AddLump (SIN_LUMP_ENTITIES, sin_dentdata, sin_entdatasize, 1, SIN_MAX_MAP_ENTSTRING);
	Sin_AddLump (SIN_LUMP_POP, sin_dpop, sizeof(sin_dpop), 1, sizeof(sin_dpop));
#else
	Sin_AddLump (SIN_LUMP_PLANES, sin_dplanes, sin_numplanes*sizeof(sin_dplane_t));
	Sin_AddLump (SIN_LUMP_LEAFS, sin_dleafs, sin_numleafs*sizeof(sin_dleaf_t));
	Sin_AddLump (SIN_LUMP_VERTEXES, sin_dvertexes, sin_numvertexes*sizeof(sin_dvertex_t));
	Sin_AddLump (SIN_LUMP_NODES, sin_dnodes, sin_numnodes*sizeof(sin_dnode_t));
	Sin_AddLump (SIN_LUMP_TEXINFO, sin_texinfo, sin_numtexinfo*sizeof(sin_texinfo_t));
	Sin_AddLump (SIN_LUMP_FACES, sin_dfaces, sin_numfaces*sizeof(sin_dface_t));
	Sin_AddLump (SIN_LUMP_BRUSHES, sin_dbrushes, sin_numbrushes*sizeof(sin_dbrush_t));
	Sin_AddLump (SIN_LUMP_BRUSHSIDES, sin_dbrushsides, sin_numbrushsides*sizeof(sin_dbrushside_t));
	Sin_AddLump (SIN_LUMP_LEAFFACES, sin_dleaffaces, sin_numleaffaces*sizeof(sin_dleaffaces[0]));
	Sin_AddLump (SIN_LUMP_LEAFBRUSHES, sin_dleafbrushes, sin_numleafbrushes*sizeof(sin_dleafbrushes[0]));
	Sin_AddLump (SIN_LUMP_SURFEDGES, sin_dsurfedges, sin_numsurfedges*sizeof(sin_dsurfedges[0]));
	Sin_AddLump (SIN_LUMP_EDGES, sin_dedges, sin_numedges*sizeof(sin_dedge_t));
	Sin_AddLump (SIN_LUMP_MODELS, sin_dmodels, sin_nummodels*sizeof(sin_dmodel_t));
	Sin_AddLump (SIN_LUMP_AREAS, sin_dareas, sin_numareas*sizeof(sin_darea_t));
	Sin_AddLump (SIN_LUMP_AREAPORTALS, sin_dareaportals, sin_numareaportals*sizeof(sin_dareaportal_t));

	Sin_AddLump (SIN_LUMP_LIGHTING, sin_dlightdata, sin_lightdatasize);
	Sin_AddLump (SIN_LUMP_VISIBILITY, sin_dvisdata, sin_visdatasize);
	Sin_AddLump (SIN_LUMP_ENTITIES, sin_dentdata, sin_entdatasize);
	Sin_AddLump (SIN_LUMP_POP, sin_dpop, sizeof(sin_dpop));
#endif
	
	fseek (wadfile, 0, SEEK_SET);
	SafeWrite (wadfile, header, sizeof(sin_dheader_t));
	fclose (wadfile);	
}

//============================================================================


//============================================

/*
================
ParseEntities

Parses the sin_dentdata string into entities
================
*/
void Sin_ParseEntities (void)
{
	script_t *script;

	num_entities = 0;
	script = LoadScriptMemory(sin_dentdata, sin_entdatasize, "*sin bsp file");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES |
									SCFL_NOSTRINGESCAPECHARS);

	while(ParseEntity(script))
	{
	} //end while

	FreeScript(script);
} //end of the function Sin_ParseEntities


/*
================
UnparseEntities

Generates the sin_dentdata string from all the entities
================
*/
void Sin_UnparseEntities (void)
{
	char	*buf, *end;
	epair_t	*ep;
	char	line[2048];
	int		i;
	char	key[1024], value[1024];

	buf = sin_dentdata;
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
			strcpy (key, ep->key);
			StripTrailing (key);
			strcpy (value, ep->value);
			StripTrailing (value);
				
			sprintf (line, "\"%s\" \"%s\"\n", key, value);
			strcat (end, line);
			end += strlen(line);
		}
		strcat (end,"}\n");
		end += 2;

		if (end > buf + SIN_MAX_MAP_ENTSTRING)
			Error ("Entity text too long");
	}
	sin_entdatasize = end - buf + 1;
} //end of the function Sin_UnparseEntities

#ifdef SIN
void  FreeValueKeys(entity_t *ent)
{
	epair_t	*ep,*next;

	for (ep=ent->epairs ; ep ; ep=next)
	{
		next = ep->next;
		FreeMemory(ep->value);
		FreeMemory(ep->key);
		FreeMemory(ep);
	}
	ent->epairs = NULL;
}
#endif

/*
=============
Sin_PrintBSPFileSizes

Dumps info about current file
=============
*/
void Sin_PrintBSPFileSizes (void)
{
	if (!num_entities)
		Sin_ParseEntities ();

	Log_Print("%6i models       %7i\n"
		,sin_nummodels, (int)(sin_nummodels*sizeof(sin_dmodel_t)));
	Log_Print("%6i brushes      %7i\n"
		,sin_numbrushes, (int)(sin_numbrushes*sizeof(sin_dbrush_t)));
	Log_Print("%6i brushsides   %7i\n"
		,sin_numbrushsides, (int)(sin_numbrushsides*sizeof(sin_dbrushside_t)));
	Log_Print("%6i planes       %7i\n"
		,sin_numplanes, (int)(sin_numplanes*sizeof(sin_dplane_t)));
	Log_Print("%6i texinfo      %7i\n"
		,sin_numtexinfo, (int)(sin_numtexinfo*sizeof(sin_texinfo_t)));
#ifdef SIN
	Log_Print("%6i lightinfo    %7i\n"
		,sin_numlightinfo, (int)(sin_numlightinfo*sizeof(sin_lightvalue_t)));
#endif
	Log_Print("%6i entdata      %7i\n", num_entities, sin_entdatasize);

	Log_Print("\n");

	Log_Print("%6i vertexes     %7i\n"
		,sin_numvertexes, (int)(sin_numvertexes*sizeof(sin_dvertex_t)));
	Log_Print("%6i nodes        %7i\n"
		,sin_numnodes, (int)(sin_numnodes*sizeof(sin_dnode_t)));
	Log_Print("%6i faces        %7i\n"
		,sin_numfaces, (int)(sin_numfaces*sizeof(sin_dface_t)));
	Log_Print("%6i leafs        %7i\n"
		,sin_numleafs, (int)(sin_numleafs*sizeof(sin_dleaf_t)));
	Log_Print("%6i leaffaces    %7i\n"
		,sin_numleaffaces, (int)(sin_numleaffaces*sizeof(sin_dleaffaces[0])));
	Log_Print("%6i leafbrushes  %7i\n"
		,sin_numleafbrushes, (int)(sin_numleafbrushes*sizeof(sin_dleafbrushes[0])));
	Log_Print("%6i surfedges    %7i\n"
		,sin_numsurfedges, (int)(sin_numsurfedges*sizeof(sin_dsurfedges[0])));
	Log_Print("%6i edges        %7i\n"
		,sin_numedges, (int)(sin_numedges*sizeof(sin_dedge_t)));
	Log_Print("       lightdata    %7i\n", sin_lightdatasize);
	Log_Print("       visdata      %7i\n", sin_visdatasize);
}
