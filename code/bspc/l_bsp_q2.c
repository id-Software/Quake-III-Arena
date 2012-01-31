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
#include "q2files.h"
#include "l_bsp_q2.h"
#include "l_bsp_ent.h"

#define q2_dmodel_t			dmodel_t
#define q2_lump_t				lump_t
#define q2_dheader_t			dheader_t
#define q2_dmodel_t			dmodel_t
#define q2_dvertex_t			dvertex_t
#define q2_dplane_t			dplane_t
#define q2_dnode_t			dnode_t
#define q2_texinfo_t			texinfo_t
#define q2_dedge_t			dedge_t
#define q2_dface_t			dface_t
#define q2_dleaf_t			dleaf_t
#define q2_dbrushside_t		dbrushside_t
#define q2_dbrush_t			dbrush_t
#define q2_dvis_t				dvis_t
#define q2_dareaportal_t	dareaportal_t
#define q2_darea_t			darea_t

#define q2_nummodels			nummodels
#define q2_dmodels			dmodels
#define q2_numleafs			numleafs
#define q2_dleafs				dleafs
#define q2_numplanes			numplanes
#define q2_dplanes			dplanes
#define q2_numvertexes		numvertexes
#define q2_dvertexes			dvertexes
#define q2_numnodes			numnodes
#define q2_dnodes				dnodes
#define q2_numtexinfo		numtexinfo
#define q2_texinfo			texinfo
#define q2_numfaces			numfaces
#define q2_dfaces				dfaces
#define q2_numedges			numedges
#define q2_dedges				dedges
#define q2_numleaffaces		numleaffaces
#define q2_dleaffaces		dleaffaces
#define q2_numleafbrushes	numleafbrushes
#define q2_dleafbrushes		dleafbrushes
#define q2_dsurfedges		dsurfedges
#define q2_numbrushes		numbrushes
#define q2_dbrushes			dbrushes
#define q2_numbrushsides	numbrushsides
#define q2_dbrushsides		dbrushsides
#define q2_numareas			numareas
#define q2_dareas				dareas
#define q2_numareaportals	numareaportals
#define q2_dareaportals		dareaportals

void GetLeafNums (void);

//=============================================================================

int				nummodels;
dmodel_t			*dmodels;//[MAX_MAP_MODELS];

int				visdatasize;
byte				*dvisdata;//[MAX_MAP_VISIBILITY];
dvis_t			*dvis;// = (dvis_t *)dvisdata;

int				lightdatasize;
byte				*dlightdata;//[MAX_MAP_LIGHTING];

int				entdatasize;
char				*dentdata;//[MAX_MAP_ENTSTRING];

int				numleafs;
dleaf_t			*dleafs;//[MAX_MAP_LEAFS];

int				numplanes;
dplane_t			*dplanes;//[MAX_MAP_PLANES];

int				numvertexes;
dvertex_t		*dvertexes;//[MAX_MAP_VERTS];

int				numnodes;
dnode_t			*dnodes;//[MAX_MAP_NODES];

//NOTE: must be static for q2 .map to q2 .bsp
int				numtexinfo;
texinfo_t		texinfo[MAX_MAP_TEXINFO];

int				numfaces;
dface_t			*dfaces;//[MAX_MAP_FACES];

int				numedges;
dedge_t			*dedges;//[MAX_MAP_EDGES];

int				numleaffaces;
unsigned short	*dleaffaces;//[MAX_MAP_LEAFFACES];

int				numleafbrushes;
unsigned short	*dleafbrushes;//[MAX_MAP_LEAFBRUSHES];

int				numsurfedges;
int				*dsurfedges;//[MAX_MAP_SURFEDGES];

int				numbrushes;
dbrush_t			*dbrushes;//[MAX_MAP_BRUSHES];

int				numbrushsides;
dbrushside_t	*dbrushsides;//[MAX_MAP_BRUSHSIDES];

int				numareas;
darea_t			*dareas;//[MAX_MAP_AREAS];

int				numareaportals;
dareaportal_t	*dareaportals;//[MAX_MAP_AREAPORTALS];

#define MAX_MAP_DPOP			256
byte				dpop[MAX_MAP_DPOP];

//
char brushsidetextured[MAX_MAP_BRUSHSIDES];

//#ifdef ME

int bspallocated = false;
int allocatedbspmem = 0;

void Q2_AllocMaxBSP(void)
{
	//models
	nummodels = 0;
	dmodels = (dmodel_t *) GetClearedMemory(MAX_MAP_MODELS * sizeof(dmodel_t));
	allocatedbspmem += MAX_MAP_MODELS * sizeof(dmodel_t);
	//vis data
	visdatasize = 0;
	dvisdata = (byte *) GetClearedMemory(MAX_MAP_VISIBILITY * sizeof(byte));
	dvis = (dvis_t *) dvisdata;
	allocatedbspmem += MAX_MAP_VISIBILITY * sizeof(byte);
	//light data
	lightdatasize = 0;
	dlightdata = (byte *) GetClearedMemory(MAX_MAP_LIGHTING * sizeof(byte));
	allocatedbspmem += MAX_MAP_LIGHTING * sizeof(byte);
	//entity data
	entdatasize = 0;
	dentdata = (char *) GetClearedMemory(MAX_MAP_ENTSTRING * sizeof(char));
	allocatedbspmem += MAX_MAP_ENTSTRING * sizeof(char);
	//leafs
	numleafs = 0;
	dleafs = (dleaf_t *) GetClearedMemory(MAX_MAP_LEAFS * sizeof(dleaf_t));
	allocatedbspmem += MAX_MAP_LEAFS * sizeof(dleaf_t);
	//planes
	numplanes = 0;
	dplanes = (dplane_t *) GetClearedMemory(MAX_MAP_PLANES * sizeof(dplane_t));
	allocatedbspmem += MAX_MAP_PLANES * sizeof(dplane_t);
	//vertexes
	numvertexes = 0;
	dvertexes = (dvertex_t *) GetClearedMemory(MAX_MAP_VERTS * sizeof(dvertex_t));
	allocatedbspmem += MAX_MAP_VERTS * sizeof(dvertex_t);
	//nodes
	numnodes = 0;
	dnodes = (dnode_t *) GetClearedMemory(MAX_MAP_NODES * sizeof(dnode_t));
	allocatedbspmem += MAX_MAP_NODES * sizeof(dnode_t);
	/*
	//texture info
	numtexinfo = 0;
	texinfo = (texinfo_t *) GetClearedMemory(MAX_MAP_TEXINFO * sizeof(texinfo_t));
	allocatedbspmem += MAX_MAP_TEXINFO * sizeof(texinfo_t);
	//*/
	//faces
	numfaces = 0;
	dfaces = (dface_t *) GetClearedMemory(MAX_MAP_FACES * sizeof(dface_t));
	allocatedbspmem += MAX_MAP_FACES * sizeof(dface_t);
	//edges
	numedges = 0;
	dedges = (dedge_t *) GetClearedMemory(MAX_MAP_EDGES * sizeof(dedge_t));
	allocatedbspmem += MAX_MAP_EDGES * sizeof(dedge_t);
	//leaf faces
	numleaffaces = 0;
	dleaffaces = (unsigned short *) GetClearedMemory(MAX_MAP_LEAFFACES * sizeof(unsigned short));
	allocatedbspmem += MAX_MAP_LEAFFACES * sizeof(unsigned short);
	//leaf brushes
	numleafbrushes = 0;
	dleafbrushes = (unsigned short *) GetClearedMemory(MAX_MAP_LEAFBRUSHES * sizeof(unsigned short));
	allocatedbspmem += MAX_MAP_LEAFBRUSHES * sizeof(unsigned short);
	//surface edges
	numsurfedges = 0;
	dsurfedges = (int *) GetClearedMemory(MAX_MAP_SURFEDGES * sizeof(int));
	allocatedbspmem += MAX_MAP_SURFEDGES * sizeof(int);
	//brushes
	numbrushes = 0;
	dbrushes = (dbrush_t *) GetClearedMemory(MAX_MAP_BRUSHES * sizeof(dbrush_t));
	allocatedbspmem += MAX_MAP_BRUSHES * sizeof(dbrush_t);
	//brushsides
	numbrushsides = 0;
	dbrushsides = (dbrushside_t *) GetClearedMemory(MAX_MAP_BRUSHSIDES * sizeof(dbrushside_t));
	allocatedbspmem += MAX_MAP_BRUSHSIDES * sizeof(dbrushside_t);
	//areas
	numareas = 0;
	dareas = (darea_t *) GetClearedMemory(MAX_MAP_AREAS * sizeof(darea_t));
	allocatedbspmem += MAX_MAP_AREAS * sizeof(darea_t);
	//area portals
	numareaportals = 0;
	dareaportals = (dareaportal_t *) GetClearedMemory(MAX_MAP_AREAPORTALS * sizeof(dareaportal_t));
	allocatedbspmem += MAX_MAP_AREAPORTALS * sizeof(dareaportal_t);
	//print allocated memory
	Log_Print("allocated ");
	PrintMemorySize(allocatedbspmem);
	Log_Print(" of BSP memory\n");
} //end of the function Q2_AllocMaxBSP

void Q2_FreeMaxBSP(void)
{
	//models
	nummodels = 0;
	FreeMemory(dmodels);
	dmodels = NULL;
	//vis data
	visdatasize = 0;
	FreeMemory(dvisdata);
	dvisdata = NULL;
	dvis = NULL;
	//light data
	lightdatasize = 0;
	FreeMemory(dlightdata);
	dlightdata = NULL;
	//entity data
	entdatasize = 0;
	FreeMemory(dentdata);
	dentdata = NULL;
	//leafs
	numleafs = 0;
	FreeMemory(dleafs);
	dleafs = NULL;
	//planes
	numplanes = 0;
	FreeMemory(dplanes);
	dplanes = NULL;
	//vertexes
	numvertexes = 0;
	FreeMemory(dvertexes);
	dvertexes = NULL;
	//nodes
	numnodes = 0;
	FreeMemory(dnodes);
	dnodes = NULL;
	/*
	//texture info
	numtexinfo = 0;
	FreeMemory(texinfo);
	texinfo = NULL;
	//*/
	//faces
	numfaces = 0;
	FreeMemory(dfaces);
	dfaces = NULL;
	//edges
	numedges = 0;
	FreeMemory(dedges);
	dedges = NULL;
	//leaf faces
	numleaffaces = 0;
	FreeMemory(dleaffaces);
	dleaffaces = NULL;
	//leaf brushes
	numleafbrushes = 0;
	FreeMemory(dleafbrushes);
	dleafbrushes = NULL;
	//surface edges
	numsurfedges = 0;
	FreeMemory(dsurfedges);
	dsurfedges = NULL;
	//brushes
	numbrushes = 0;
	FreeMemory(dbrushes);
	dbrushes = NULL;
	//brushsides
	numbrushsides = 0;
	FreeMemory(dbrushsides);
	dbrushsides = NULL;
	//areas
	numareas = 0;
	FreeMemory(dareas);
	dareas = NULL;
	//area portals
	numareaportals = 0;
	FreeMemory(dareaportals);
	dareaportals = NULL;
	//
	Log_Print("freed ");
	PrintMemorySize(allocatedbspmem);
	Log_Print(" of BSP memory\n");
	allocatedbspmem = 0;
} //end of the function Q2_FreeMaxBSP

#define WCONVEX_EPSILON		0.5

int InsideWinding(winding_t *w, vec3_t point, int planenum)
{
	int i;
	float dist;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	dplane_t *plane;

	for (i = 1; i <= w->numpoints; i++)
	{
		v1 = w->p[i % w->numpoints];
		v2 = w->p[(i + 1) % w->numpoints];

		VectorSubtract(v2, v1, edgevec);
		plane = &dplanes[planenum];
		CrossProduct(plane->normal, edgevec, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		if (DotProduct(normal, point) - dist > WCONVEX_EPSILON) return false;
	} //end for
	return true;
} //end of the function InsideWinding

int InsideFace(dface_t *face, vec3_t point)
{
	int i, edgenum, side;
	float dist;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	dplane_t *plane;

	for (i = 0; i < face->numedges; i++)
	{
		//get the first and second vertex of the edge
		edgenum = dsurfedges[face->firstedge + i];
		side = edgenum < 0;
		v1 = dvertexes[dedges[abs(edgenum)].v[side]].point;
		v2 = dvertexes[dedges[abs(edgenum)].v[!side]].point;
		//create a plane through the edge vector, orthogonal to the face plane
		//and with the normal vector pointing out of the face
		VectorSubtract(v1, v2, edgevec);
		plane = &dplanes[face->planenum];
		CrossProduct(plane->normal, edgevec, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		if (DotProduct(normal, point) - dist > WCONVEX_EPSILON) return false;
	} //end for
	return true;
} //end of the function InsideFace
//===========================================================================
// returns the amount the face and the winding overlap
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float Q2_FaceOnWinding(q2_dface_t *face, winding_t *winding)
{
	int i, edgenum, side;
	float dist, area;
	q2_dplane_t plane;
	vec_t *v1, *v2;
	vec3_t normal, edgevec;
	winding_t *w;

	//
	w = CopyWinding(winding);
	memcpy(&plane, &q2_dplanes[face->planenum], sizeof(q2_dplane_t));
	//check on which side of the plane the face is
	if (face->side)
	{
		VectorNegate(plane.normal, plane.normal);
		plane.dist = -plane.dist;
	} //end if
	for (i = 0; i < face->numedges && w; i++)
	{
		//get the first and second vertex of the edge
		edgenum = q2_dsurfedges[face->firstedge + i];
		side = edgenum > 0;
		//if the face plane is flipped
		v1 = q2_dvertexes[q2_dedges[abs(edgenum)].v[side]].point;
		v2 = q2_dvertexes[q2_dedges[abs(edgenum)].v[!side]].point;
		//create a plane through the edge vector, orthogonal to the face plane
		//and with the normal vector pointing inward
		VectorSubtract(v1, v2, edgevec);
		CrossProduct(edgevec, plane.normal, normal);
		VectorNormalize(normal);
		dist = DotProduct(normal, v1);
		//
		ChopWindingInPlace(&w, normal, dist, -0.1); //CLIP_EPSILON
	} //end for
	if (w)
	{
		area = WindingArea(w);
		FreeWinding(w);
		return area;
	} //end if
	return 0;
} //end of the function Q2_FaceOnWinding
//===========================================================================
// creates a winding for the given brush side on the given brush
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
winding_t *Q2_BrushSideWinding(dbrush_t *brush, dbrushside_t *baseside)
{
	int i;
	dplane_t *baseplane, *plane;
	winding_t *w;
	dbrushside_t *side;
	
	//create a winding for the brush side with the given planenumber
	baseplane = &dplanes[baseside->planenum];
	w = BaseWindingForPlane(baseplane->normal, baseplane->dist);
	for (i = 0; i < brush->numsides && w; i++)
	{
		side = &dbrushsides[brush->firstside + i];
		//don't chop with the base plane
		if (side->planenum == baseside->planenum) continue;
		//also don't use planes that are almost equal
		plane = &dplanes[side->planenum];
		if (DotProduct(baseplane->normal, plane->normal) > 0.999
				&& fabs(baseplane->dist - plane->dist) < 0.01) continue;
		//
		plane = &dplanes[side->planenum^1];
		ChopWindingInPlace(&w, plane->normal, plane->dist, -0.1); //CLIP_EPSILON);
	} //end for
	return w;
} //end of the function Q2_BrushSideWinding
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Q2_HintSkipBrush(dbrush_t *brush)
{
	int j;
	dbrushside_t *brushside;

	for (j = 0; j < brush->numsides; j++)
	{
		brushside = &dbrushsides[brush->firstside + j];
		if (brushside->texinfo > 0)
		{
			if (texinfo[brushside->texinfo].flags & (SURF_SKIP|SURF_HINT))
			{
				return true;
			} //end if
		} //end if
	} //end for
	return false;
} //end of the function Q2_HintSkipBrush
//===========================================================================
// fix screwed brush texture references
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WindingIsTiny(winding_t *w);

void Q2_FixTextureReferences(void)
{
	int i, j, k, we;
	dbrushside_t *brushside;
	dbrush_t *brush;
	dface_t *face;
	winding_t *w;

	memset(brushsidetextured, false, MAX_MAP_BRUSHSIDES);
	//go over all the brushes
   for (i = 0; i < numbrushes; i++)
   {
		brush = &dbrushes[i];
		//hint brushes are not textured
		if (Q2_HintSkipBrush(brush)) continue;
		//go over all the sides of the brush
		for (j = 0; j < brush->numsides; j++)
		{
			brushside = &dbrushsides[brush->firstside + j];
			//
			w = Q2_BrushSideWinding(brush, brushside);
			if (!w)
			{
				brushsidetextured[brush->firstside + j] = true;
				continue;
			} //end if
			else
			{
				//RemoveEqualPoints(w, 0.2);
				if (WindingIsTiny(w))
				{
					FreeWinding(w);
					brushsidetextured[brush->firstside + j] = true;
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
						brushsidetextured[brush->firstside + j] = true;
						continue;
					} //end if
				} //end else
			} //end else
			if (WindingArea(w) < 20)
			{
				brushsidetextured[brush->firstside + j] = true;
			} //end if
			//find a face for texturing this brush
			for (k = 0; k < numfaces; k++)
			{
				face = &dfaces[k];
				//if the face is in the same plane as the brush side
				if ((face->planenum&~1) != (brushside->planenum&~1)) continue;
				//if the face is partly or totally on the brush side
				if (Q2_FaceOnWinding(face, w))
				{
					brushside->texinfo = face->texinfo;
					brushsidetextured[brush->firstside + j] = true;
					break;
				} //end if
			} //end for
			FreeWinding(w);
		} //end for
	} //end for
} //end of the function Q2_FixTextureReferences*/

//#endif //ME


/*
===============
CompressVis

===============
*/
int Q2_CompressVis (byte *vis, byte *dest)
{
	int		j;
	int		rep;
	int		visrow;
	byte	*dest_p;
	
	dest_p = dest;
//	visrow = (r_numvisleafs + 7)>>3;
	visrow = (dvis->numclusters + 7)>>3;
	
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
DecompressVis
===================
*/
void Q2_DecompressVis (byte *in, byte *decompressed)
{
	int		c;
	byte	*out;
	int		row;

//	row = (r_numvisleafs+7)>>3;	
	row = (dvis->numclusters+7)>>3;	
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
}

//=============================================================================

/*
=============
SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void Q2_SwapBSPFile (qboolean todisk)
{
	int				i, j;
	dmodel_t		*d;

	
// models	
	for (i=0 ; i<nummodels ; i++)
	{
		d = &dmodels[i];

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
	for (i=0 ; i<numvertexes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			dvertexes[i].point[j] = LittleFloat (dvertexes[i].point[j]);
	}
		
//
// planes
//	
	for (i=0 ; i<numplanes ; i++)
	{
		for (j=0 ; j<3 ; j++)
			dplanes[i].normal[j] = LittleFloat (dplanes[i].normal[j]);
		dplanes[i].dist = LittleFloat (dplanes[i].dist);
		dplanes[i].type = LittleLong (dplanes[i].type);
	}
	
//
// texinfos
//	
	for (i=0 ; i<numtexinfo ; i++)
	{
		for (j=0 ; j<8 ; j++)
			texinfo[i].vecs[0][j] = LittleFloat (texinfo[i].vecs[0][j]);
		texinfo[i].flags = LittleLong (texinfo[i].flags);
		texinfo[i].value = LittleLong (texinfo[i].value);
		texinfo[i].nexttexinfo = LittleLong (texinfo[i].nexttexinfo);
	}
	
//
// faces
//
	for (i=0 ; i<numfaces ; i++)
	{
		dfaces[i].texinfo = LittleShort (dfaces[i].texinfo);
		dfaces[i].planenum = LittleShort (dfaces[i].planenum);
		dfaces[i].side = LittleShort (dfaces[i].side);
		dfaces[i].lightofs = LittleLong (dfaces[i].lightofs);
		dfaces[i].firstedge = LittleLong (dfaces[i].firstedge);
		dfaces[i].numedges = LittleShort (dfaces[i].numedges);
	}

//
// nodes
//
	for (i=0 ; i<numnodes ; i++)
	{
		dnodes[i].planenum = LittleLong (dnodes[i].planenum);
		for (j=0 ; j<3 ; j++)
		{
			dnodes[i].mins[j] = LittleShort (dnodes[i].mins[j]);
			dnodes[i].maxs[j] = LittleShort (dnodes[i].maxs[j]);
		}
		dnodes[i].children[0] = LittleLong (dnodes[i].children[0]);
		dnodes[i].children[1] = LittleLong (dnodes[i].children[1]);
		dnodes[i].firstface = LittleShort (dnodes[i].firstface);
		dnodes[i].numfaces = LittleShort (dnodes[i].numfaces);
	}

//
// leafs
//
	for (i=0 ; i<numleafs ; i++)
	{
		dleafs[i].contents = LittleLong (dleafs[i].contents);
		dleafs[i].cluster = LittleShort (dleafs[i].cluster);
		dleafs[i].area = LittleShort (dleafs[i].area);
		for (j=0 ; j<3 ; j++)
		{
			dleafs[i].mins[j] = LittleShort (dleafs[i].mins[j]);
			dleafs[i].maxs[j] = LittleShort (dleafs[i].maxs[j]);
		}

		dleafs[i].firstleafface = LittleShort (dleafs[i].firstleafface);
		dleafs[i].numleaffaces = LittleShort (dleafs[i].numleaffaces);
		dleafs[i].firstleafbrush = LittleShort (dleafs[i].firstleafbrush);
		dleafs[i].numleafbrushes = LittleShort (dleafs[i].numleafbrushes);
	}

//
// leaffaces
//
	for (i=0 ; i<numleaffaces ; i++)
		dleaffaces[i] = LittleShort (dleaffaces[i]);

//
// leafbrushes
//
	for (i=0 ; i<numleafbrushes ; i++)
		dleafbrushes[i] = LittleShort (dleafbrushes[i]);

//
// surfedges
//
	for (i=0 ; i<numsurfedges ; i++)
		dsurfedges[i] = LittleLong (dsurfedges[i]);

//
// edges
//
	for (i=0 ; i<numedges ; i++)
	{
		dedges[i].v[0] = LittleShort (dedges[i].v[0]);
		dedges[i].v[1] = LittleShort (dedges[i].v[1]);
	}

//
// brushes
//
	for (i=0 ; i<numbrushes ; i++)
	{
		dbrushes[i].firstside = LittleLong (dbrushes[i].firstside);
		dbrushes[i].numsides = LittleLong (dbrushes[i].numsides);
		dbrushes[i].contents = LittleLong (dbrushes[i].contents);
	}

//
// areas
//
	for (i=0 ; i<numareas ; i++)
	{
		dareas[i].numareaportals = LittleLong (dareas[i].numareaportals);
		dareas[i].firstareaportal = LittleLong (dareas[i].firstareaportal);
	}

//
// areasportals
//
	for (i=0 ; i<numareaportals ; i++)
	{
		dareaportals[i].portalnum = LittleLong (dareaportals[i].portalnum);
		dareaportals[i].otherarea = LittleLong (dareaportals[i].otherarea);
	}

//
// brushsides
//
	for (i=0 ; i<numbrushsides ; i++)
	{
		dbrushsides[i].planenum = LittleShort (dbrushsides[i].planenum);
		dbrushsides[i].texinfo = LittleShort (dbrushsides[i].texinfo);
	}

//
// visibility
//
	if (todisk)
		j = dvis->numclusters;
	else
		j = LittleLong(dvis->numclusters);
	dvis->numclusters = LittleLong (dvis->numclusters);
	for (i=0 ; i<j ; i++)
	{
		dvis->bitofs[i][0] = LittleLong (dvis->bitofs[i][0]);
		dvis->bitofs[i][1] = LittleLong (dvis->bitofs[i][1]);
	}
} //end of the function Q2_SwapBSPFile


dheader_t	*header;

int Q2_CopyLump (int lump, void *dest, int size, int maxsize)
{
	int		length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;
	
	if (length % size)
		Error ("LoadBSPFile: odd lump size");

   if ((length/size) > maxsize)
      Error ("Q2_LoadBSPFile: exceeded max size for lump %d size %d > maxsize %d\n", lump, (length/size), maxsize);

	memcpy (dest, (byte *)header + ofs, length);

	return length / size;
} //end of the function Q2_CopyLump

/*
=============
LoadBSPFile
=============
*/
void Q2_LoadBSPFile(char *filename, int offset, int length)
{
	int			i;
	
//
// load the file header
//
	LoadFile (filename, (void **)&header, offset, length);

// swap the header
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != IDBSPHEADER)
		Error ("%s is not a IBSP file", filename);
	if (header->version != BSPVERSION)
		Error ("%s is version %i, not %i", filename, header->version, BSPVERSION);

	nummodels = Q2_CopyLump (LUMP_MODELS, dmodels, sizeof(dmodel_t), MAX_MAP_MODELS);
	numvertexes = Q2_CopyLump (LUMP_VERTEXES, dvertexes, sizeof(dvertex_t), MAX_MAP_VERTS);
	numplanes = Q2_CopyLump (LUMP_PLANES, dplanes, sizeof(dplane_t), MAX_MAP_PLANES);
	numleafs = Q2_CopyLump (LUMP_LEAFS, dleafs, sizeof(dleaf_t), MAX_MAP_LEAFS);
	numnodes = Q2_CopyLump (LUMP_NODES, dnodes, sizeof(dnode_t), MAX_MAP_NODES);
	numtexinfo = Q2_CopyLump (LUMP_TEXINFO, texinfo, sizeof(texinfo_t), MAX_MAP_TEXINFO);
	numfaces = Q2_CopyLump (LUMP_FACES, dfaces, sizeof(dface_t), MAX_MAP_FACES);
	numleaffaces = Q2_CopyLump (LUMP_LEAFFACES, dleaffaces, sizeof(dleaffaces[0]), MAX_MAP_LEAFFACES);
	numleafbrushes = Q2_CopyLump (LUMP_LEAFBRUSHES, dleafbrushes, sizeof(dleafbrushes[0]), MAX_MAP_LEAFBRUSHES);
	numsurfedges = Q2_CopyLump (LUMP_SURFEDGES, dsurfedges, sizeof(dsurfedges[0]), MAX_MAP_SURFEDGES);
	numedges = Q2_CopyLump (LUMP_EDGES, dedges, sizeof(dedge_t), MAX_MAP_EDGES);
	numbrushes = Q2_CopyLump (LUMP_BRUSHES, dbrushes, sizeof(dbrush_t), MAX_MAP_BRUSHES);
	numbrushsides = Q2_CopyLump (LUMP_BRUSHSIDES, dbrushsides, sizeof(dbrushside_t), MAX_MAP_BRUSHSIDES);
	numareas = Q2_CopyLump (LUMP_AREAS, dareas, sizeof(darea_t), MAX_MAP_AREAS);
	numareaportals = Q2_CopyLump (LUMP_AREAPORTALS, dareaportals, sizeof(dareaportal_t), MAX_MAP_AREAPORTALS);

	visdatasize = Q2_CopyLump (LUMP_VISIBILITY, dvisdata, 1, MAX_MAP_VISIBILITY);
	lightdatasize = Q2_CopyLump (LUMP_LIGHTING, dlightdata, 1, MAX_MAP_LIGHTING);
	entdatasize = Q2_CopyLump (LUMP_ENTITIES, dentdata, 1, MAX_MAP_ENTSTRING);

	Q2_CopyLump (LUMP_POP, dpop, 1, MAX_MAP_DPOP);

	FreeMemory(header);		// everything has been copied out
		
//
// swap everything
//	
	Q2_SwapBSPFile (false);

	Q2_FixTextureReferences();
} //end of the function Q2_LoadBSPFile


/*
=============
LoadBSPFileTexinfo

Only loads the texinfo lump, so qdata can scan for textures
=============
*/
void	Q2_LoadBSPFileTexinfo (char *filename)
{
	int			i;
	FILE		*f;
	int		length, ofs;

	header = GetMemory(sizeof(dheader_t));

	f = fopen (filename, "rb");
	fread (header, sizeof(dheader_t), 1, f);

// swap the header
	for (i=0 ; i< sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	if (header->ident != IDBSPHEADER)
		Error ("%s is not a IBSP file", filename);
	if (header->version != BSPVERSION)
		Error ("%s is version %i, not %i", filename, header->version, BSPVERSION);


	length = header->lumps[LUMP_TEXINFO].filelen;
	ofs = header->lumps[LUMP_TEXINFO].fileofs;

	fseek (f, ofs, SEEK_SET);
	fread (texinfo, length, 1, f);
	fclose (f);

	numtexinfo = length / sizeof(texinfo_t);

	FreeMemory(header);		// everything has been copied out
		
	Q2_SwapBSPFile (false);
} //end of the function Q2_LoadBSPFileTexinfo


//============================================================================

FILE		*wadfile;
dheader_t	outheader;

void Q2_AddLump (int lumpnum, void *data, int len)
{
	lump_t *lump;

	lump = &header->lumps[lumpnum];
	
	lump->fileofs = LittleLong( ftell(wadfile) );
	lump->filelen = LittleLong(len);
	SafeWrite (wadfile, data, (len+3)&~3);
} //end of the function Q2_AddLump

/*
=============
WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void	Q2_WriteBSPFile (char *filename)
{		
	header = &outheader;
	memset (header, 0, sizeof(dheader_t));
	
	Q2_SwapBSPFile (true);

	header->ident = LittleLong (IDBSPHEADER);
	header->version = LittleLong (BSPVERSION);
	
	wadfile = SafeOpenWrite (filename);
	SafeWrite (wadfile, header, sizeof(dheader_t));	// overwritten later

	Q2_AddLump (LUMP_PLANES, dplanes, numplanes*sizeof(dplane_t));
	Q2_AddLump (LUMP_LEAFS, dleafs, numleafs*sizeof(dleaf_t));
	Q2_AddLump (LUMP_VERTEXES, dvertexes, numvertexes*sizeof(dvertex_t));
	Q2_AddLump (LUMP_NODES, dnodes, numnodes*sizeof(dnode_t));
	Q2_AddLump (LUMP_TEXINFO, texinfo, numtexinfo*sizeof(texinfo_t));
	Q2_AddLump (LUMP_FACES, dfaces, numfaces*sizeof(dface_t));
	Q2_AddLump (LUMP_BRUSHES, dbrushes, numbrushes*sizeof(dbrush_t));
	Q2_AddLump (LUMP_BRUSHSIDES, dbrushsides, numbrushsides*sizeof(dbrushside_t));
	Q2_AddLump (LUMP_LEAFFACES, dleaffaces, numleaffaces*sizeof(dleaffaces[0]));
	Q2_AddLump (LUMP_LEAFBRUSHES, dleafbrushes, numleafbrushes*sizeof(dleafbrushes[0]));
	Q2_AddLump (LUMP_SURFEDGES, dsurfedges, numsurfedges*sizeof(dsurfedges[0]));
	Q2_AddLump (LUMP_EDGES, dedges, numedges*sizeof(dedge_t));
	Q2_AddLump (LUMP_MODELS, dmodels, nummodels*sizeof(dmodel_t));
	Q2_AddLump (LUMP_AREAS, dareas, numareas*sizeof(darea_t));
	Q2_AddLump (LUMP_AREAPORTALS, dareaportals, numareaportals*sizeof(dareaportal_t));

	Q2_AddLump (LUMP_LIGHTING, dlightdata, lightdatasize);
	Q2_AddLump (LUMP_VISIBILITY, dvisdata, visdatasize);
	Q2_AddLump (LUMP_ENTITIES, dentdata, entdatasize);
	Q2_AddLump (LUMP_POP, dpop, sizeof(dpop));
	
	fseek (wadfile, 0, SEEK_SET);
	SafeWrite (wadfile, header, sizeof(dheader_t));
	fclose (wadfile);	
} //end of the function Q2_WriteBSPFile

//============================================================================

/*
=============
PrintBSPFileSizes

Dumps info about current file
=============
*/
void Q2_PrintBSPFileSizes (void)
{
	if (!num_entities)
		Q2_ParseEntities();

	printf ("%6i models       %7i\n"
		,nummodels, (int)(nummodels*sizeof(dmodel_t)));
	printf ("%6i brushes      %7i\n"
		,numbrushes, (int)(numbrushes*sizeof(dbrush_t)));
	printf ("%6i brushsides   %7i\n"
		,numbrushsides, (int)(numbrushsides*sizeof(dbrushside_t)));
	printf ("%6i planes       %7i\n"
		,numplanes, (int)(numplanes*sizeof(dplane_t)));
	printf ("%6i texinfo      %7i\n"
		,numtexinfo, (int)(numtexinfo*sizeof(texinfo_t)));
	printf ("%6i entdata      %7i\n", num_entities, entdatasize);

	printf ("\n");

	printf ("%6i vertexes     %7i\n"
		,numvertexes, (int)(numvertexes*sizeof(dvertex_t)));
	printf ("%6i nodes        %7i\n"
		,numnodes, (int)(numnodes*sizeof(dnode_t)));
	printf ("%6i faces        %7i\n"
		,numfaces, (int)(numfaces*sizeof(dface_t)));
	printf ("%6i leafs        %7i\n"
		,numleafs, (int)(numleafs*sizeof(dleaf_t)));
	printf ("%6i leaffaces    %7i\n"
		,numleaffaces, (int)(numleaffaces*sizeof(dleaffaces[0])));
	printf ("%6i leafbrushes  %7i\n"
		,numleafbrushes, (int)(numleafbrushes*sizeof(dleafbrushes[0])));
	printf ("%6i surfedges    %7i\n"
		,numsurfedges, (int)(numsurfedges*sizeof(dsurfedges[0])));
	printf ("%6i edges        %7i\n"
		,numedges, (int)(numedges*sizeof(dedge_t)));
//NEW
	printf ("%6i areas        %7i\n"
		,numareas, (int)(numareas*sizeof(darea_t)));
	printf ("%6i areaportals  %7i\n"
		,numareaportals, (int)(numareaportals*sizeof(dareaportal_t)));
//ENDNEW
	printf ("      lightdata    %7i\n", lightdatasize);
	printf ("      visdata      %7i\n", visdatasize);
} //end of the function Q2_PrintBSPFileSizes

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void Q2_ParseEntities (void)
{
	script_t *script;

	num_entities = 0;
	script = LoadScriptMemory(dentdata, entdatasize, "*Quake2 bsp file");
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES |
									SCFL_NOSTRINGESCAPECHARS);

	while(ParseEntity(script))
	{
	} //end while

	FreeScript(script);
} //end of the function Q2_ParseEntities


/*
================
UnparseEntities

Generates the dentdata string from all the entities
================
*/
void Q2_UnparseEntities (void)
{
	char	*buf, *end;
	epair_t	*ep;
	char	line[2048];
	int		i;
	char	key[1024], value[1024];

	buf = dentdata;
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

		if (end > buf + MAX_MAP_ENTSTRING)
			Error ("Entity text too long");
	}
	entdatasize = end - buf + 1;
} //end of the function Q2_UnparseEntities

