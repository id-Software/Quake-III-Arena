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


#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "imagelib.h"
#include "threads.h"
#include "mutex.h"
#include "scriplib.h"

#include "shaders.h"
#include "mesh.h"

#ifdef _WIN32
//Improve floating-point consistency.
#pragma optimize( "p", on )
#endif

#ifdef _WIN32
#include "../libs/pakstuff.h"
#endif

#define MAX_CLUSTERS		16384
#define	MAX_PORTALS			32768
#define MAX_FACETS			65536
#define MAX_LIGHTS			16384

#define LIGHTMAP_SIZE		128

#define LIGHTMAP_PIXELSHIFT		0.5

//#define LIGHTMAP_PATCHSHIFT

#define	PORTALFILE	"PRT1"

#define	ON_EPSILON	0.1

#define VectorSet(v, x, y, z)		v[0] = x;v[1] = y;v[2] = z;

typedef struct
{
	vec3_t		normal;
	float		dist;
} plane_t;

#define MAX_POINTS_ON_WINDING	64
//NOTE: whenever this is overflowed parts of lightmaps might end up not being lit
#define	MAX_POINTS_ON_FIXED_WINDING	48

typedef struct
{
	int		numpoints;
	vec3_t	points[MAX_POINTS_ON_FIXED_WINDING];			// variable sized
} winding_t;

typedef struct
{
	plane_t		plane;	// normal pointing into neighbor
	int			leaf;	// neighbor
	winding_t	*winding;
	vec3_t		origin;	// for fast clip testing
	float		radius;
} lportal_t;

#define	MAX_PORTALS_ON_LEAF		128
typedef struct lleaf_s
{
	int			numportals;
	lportal_t	*portals[MAX_PORTALS_ON_LEAF];
	//
	int			numSurfaces;
	int			firstSurface;
} lleaf_t;

typedef struct lFacet_s
{
	int		num;
	plane_t	plane;
	vec3_t	points[4];				//
	int		numpoints;
	float	lightmapCoords[4][2];
	plane_t boundaries[4];			// negative is outside the bounds
	float	textureMatrix[2][4];	// texture coordinates for translucency
	float	lightmapMatrix[2][4];	// lightmap texture coordinates
	vec3_t	mins;
	int		x, y, width, height;
} lFacet_t;

typedef struct lsurfaceTest_s
{
	vec3_t			mins, maxs;
	vec3_t			origin;
	float			radius;
	qboolean		patch;			// true if this is a patch
	qboolean		trisoup;		// true if this is a triangle soup
	int				numFacets;
	lFacet_t		*facets;
	mesh_t			*detailMesh;	// detailed mesh with points for each lmp
	shaderInfo_t	*shader;		// for translucency
	mutex_t			*mutex;
	int				numvolumes;		// number of volumes casted at this surface
	//
	int				always_tracelight;
	int				always_vlight;
} lsurfaceTest_t;

//volume types
#define VOLUME_NORMAL			0
#define VOLUME_DIRECTED			1

#define MAX_TRANSLUCENTFACETS	32

typedef struct lightvolume_s
{
	int num;
	int cluster;							//cluster this light volume started in
	plane_t endplane;						//end plane
	plane_t farplane;						//original end plane
	vec3_t points[MAX_POINTS_ON_WINDING];	//end winding points
	plane_t planes[MAX_POINTS_ON_WINDING];	//volume bounding planes
	int numplanes;							//number of volume bounding planes
	int type;								//light volume type
	//list with translucent surfaces the volume went through
	int transFacets[MAX_TRANSLUCENTFACETS];
	int transSurfaces[MAX_TRANSLUCENTFACETS];
	int numtransFacets;
	//clusters already tested
	byte clusterTested[MAX_CLUSTERS/8];
	//facets already tested
	byte facetTested[MAX_FACETS/8];
	int facetNum;			//number of the facet blocking the light in this volume
	int surfaceNum;			//number of the surface blocking the light in this volume
} lightvolume_t;

//light types
#define LIGHT_POINTRADIAL			1
#define LIGHT_POINTSPOT				2
#define LIGHT_POINTFAKESURFACE		3
#define LIGHT_SURFACEDIRECTED		4
#define LIGHT_SURFACERADIAL			5
#define LIGHT_SURFACESPOT			6

//light distance attenuation types
#define LDAT_QUADRATIC				0
#define LDAT_LINEAR					1
#define LDAT_NOSCALE				2

//light angle attenuation types
#define LAAT_NORMAL					0
#define LAAT_QUADRATIC				1
#define LAAT_DOUBLEQUADRATIC		2

typedef struct vlight_s
{
	vec3_t origin;				//light origin, for point lights
	winding_t w;				//light winding, for area lights
	vec4_t plane;				//light winding plane
	vec3_t normal;				//direction of the light
	int type;					//light type
	vec3_t color;				//light color
	qboolean twosided;			//radiates light at both sides of the winding
	int style;					//light style (not used)
	int atten_disttype;			//light distance attenuation type
	int atten_angletype;		//light angle attenuation type
	float atten_distscale;		//distance attenuation scale
	float atten_anglescale;		//angle attenuation scale
	float radiusByDist;			//radius by distance for spot lights
	float photons;				//emitted photons
	float intensity;			//intensity
	vec3_t emitColor;			//full out-of-gamut value (not used)
	struct shaderInfo_s	*si;	//shader info
	int insolid;				//set when light is in solid
} vlight_t;

float	lightLinearScale			= 1.0 / 8000;
float	lightPointScale				= 7500;
float	lightAreaScale				= 0.25;
float	lightFormFactorValueScale	= 3;
int		lightDefaultSubdivide		= 999;		// vary by surface size?
vec3_t	lightAmbientColor;

int			portalclusters, numportals, numfaces;
lleaf_t		*leafs;
lportal_t	*portals;
int			numvlights = 0;
vlight_t	*vlights[MAX_LIGHTS];
int			nostitching = 0;
int			noalphashading = 0;
int			nocolorshading = 0;
int			nobackfaceculling = 0;
int			defaulttracelight = 0;
int			radiosity = 0;
int			radiosity_scale;

int				clustersurfaces[MAX_MAP_LEAFFACES];
int				numclustersurfaces = 0;
lsurfaceTest_t	*lsurfaceTest[MAX_MAP_DRAW_SURFS];
int				numfacets;
float			lightmappixelarea[MAX_MAP_LIGHTING/3];
float			*lightFloats;//[MAX_MAP_LIGHTING];

// from polylib.c
winding_t	*AllocWinding (int points);
void		FreeWinding (winding_t *w);
void		WindingCenter (winding_t *w, vec3_t center);
void		WindingBounds (winding_t *w, vec3_t mins, vec3_t maxs);
vec_t		WindingArea (winding_t *w);
winding_t	*BaseWindingForPlane (vec3_t normal, vec_t dist);
void		ClipWindingEpsilon (winding_t *in, vec3_t normal, vec_t dist, 
				vec_t epsilon, winding_t **front, winding_t **back);
winding_t	*ReverseWinding (winding_t *w);

// from light.c
extern char		source[1024];
extern vec3_t	surfaceOrigin[ MAX_MAP_DRAW_SURFS ];
extern int		entitySurface[ MAX_MAP_DRAW_SURFS ];
extern int		samplesize;
extern int		novertexlighting;
extern int		nogridlighting;
extern qboolean	patchshadows;
extern vec3_t	gridSize;

float PointToPolygonFormFactor( const vec3_t point, const vec3_t normal, const winding_t *w );
void ColorToBytes( const float *color, byte *colorBytes );
void CountLightmaps( void );
void GridAndVertexLighting( void );
void SetEntityOrigins( void );


//#define DEBUGNET

#ifdef DEBUGNET

#include "l_net.h"

socket_t *debug_socket;

/*
=====================
DebugNet_Setup
=====================
*/
void DebugNet_Setup(void)
{
	address_t address;
	int i;

	Net_Setup();
	Net_StringToAddress("127.0.0.1:28000", &address);
	for (i = 0; i < 10; i++)
	{
		debug_socket = Net_Connect(&address, 28005 + i);
		if (debug_socket)
			break;
	}
}

/*
=====================
DebugNet_Shutdown
=====================
*/
void DebugNet_Shutdown(void)
{
	netmessage_t msg;

	if (debug_socket)
	{
		NMSG_Clear(&msg);
		NMSG_WriteByte(&msg, 1);
		Net_Send(debug_socket, &msg);
		Net_Disconnect(debug_socket);
	}
	debug_socket = NULL;
	Net_Shutdown();
}

/*
=====================
DebugNet_RemoveAllPolys
=====================
*/
void DebugNet_RemoveAllPolys(void)
{
	netmessage_t msg;

	if (!debug_socket)
		return;
	NMSG_Clear(&msg);
	NMSG_WriteByte(&msg, 2);		//remove all debug polys
	Net_Send(debug_socket, &msg);
}

/*
====================
DebugNet_DrawWinding
=====================
*/
void DebugNet_DrawWinding(winding_t *w, int color)
{
	netmessage_t msg;
	int i;

	if (!debug_socket)
		return;
	NMSG_Clear(&msg);
	NMSG_WriteByte(&msg, 0);				//draw a winding
	NMSG_WriteByte(&msg, w->numpoints);		//number of points
	NMSG_WriteLong(&msg, color);			//color
	for (i = 0; i < w->numpoints; i++)
	{
		NMSG_WriteFloat(&msg, w->points[i][0]);
		NMSG_WriteFloat(&msg, w->points[i][1]);
		NMSG_WriteFloat(&msg, w->points[i][2]);
	}
	Net_Send(debug_socket, &msg);
}

/*
=====================
DebugNet_DrawLine
=====================
*/
void DebugNet_DrawLine(vec3_t p1, vec3_t p2, int color)
{
	netmessage_t msg;

	if (!debug_socket)
		return;
	NMSG_Clear(&msg);
	NMSG_WriteByte(&msg, 1);				//draw a line
	NMSG_WriteLong(&msg, color);			//color
	NMSG_WriteFloat(&msg, p1[0]);
	NMSG_WriteFloat(&msg, p1[1]);
	NMSG_WriteFloat(&msg, p1[2]);
	NMSG_WriteFloat(&msg, p2[0]);
	NMSG_WriteFloat(&msg, p2[1]);
	NMSG_WriteFloat(&msg, p2[2]);
	Net_Send(debug_socket, &msg);
}

/*
=====================
DebugNet_DrawMesh
=====================
*/
void DebugNet_DrawMesh(mesh_t *mesh)
{
	int i, j;
	float dot;
	drawVert_t	*v1, *v2, *v3, *v4;
	winding_t winding;
	plane_t plane;
	vec3_t d1, d2;

	for ( i = 0 ; i < mesh->width - 1 ; i++ ) {
		for ( j = 0 ; j < mesh->height - 1 ; j++ ) {

			v1 = mesh->verts + j * mesh->width + i;
			v2 = v1 + 1;
			v3 = v1 + mesh->width + 1;
			v4 = v1 + mesh->width;

			VectorSubtract( v4->xyz, v1->xyz, d1 );
			VectorSubtract( v3->xyz, v1->xyz, d2 );
			CrossProduct( d2, d1, plane.normal );
			if ( VectorNormalize( plane.normal, plane.normal ) != 0 )
			{
				plane.dist = DotProduct( v1->xyz, plane.normal );
				dot = DotProduct(plane.normal, v2->xyz) - plane.dist;
				if (fabs(dot) < 0.1)
				{
					VectorCopy(v1->xyz, winding.points[0]);
					VectorCopy(v4->xyz, winding.points[1]);
					VectorCopy(v3->xyz, winding.points[2]);
					VectorCopy(v2->xyz, winding.points[3]);
					winding.numpoints = 4;
					DebugNet_DrawWinding(&winding, 2);
					continue;
				}
			}

			winding.numpoints = 3;
			VectorCopy(v1->xyz, winding.points[0]);
			VectorCopy(v4->xyz, winding.points[1]);
			VectorCopy(v3->xyz, winding.points[2]);
			DebugNet_DrawWinding(&winding, 2);

			VectorCopy(v1->xyz, winding.points[0]);
			VectorCopy(v3->xyz, winding.points[1]);
			VectorCopy(v2->xyz, winding.points[2]);
			DebugNet_DrawWinding(&winding, 2);
		}
	}
}

/*
=====================
VL_DrawLightVolume
=====================
*/
int VL_ChopWinding (winding_t *in, plane_t *split, float epsilon);

void VL_DrawLightVolume(vlight_t *light, lightvolume_t *volume)
{
	winding_t w;
	int i;
	vec3_t p2, invlight;

	memcpy(w.points, volume->points, volume->numplanes * sizeof(vec3_t));
	w.numpoints = volume->numplanes;
	DebugNet_DrawWinding(&w, 2);

	if (volume->type == VOLUME_DIRECTED)
	{
		VectorCopy(light->normal, invlight);
		VectorInverse(invlight);
		for (i = 0; i < volume->numplanes; i++)
		{
			VectorCopy(volume->points[i], w.points[0]);
			VectorCopy(volume->points[(i+1) % volume->numplanes], w.points[1]);
			VectorMA(w.points[1], MAX_WORLD_COORD, invlight, w.points[2]);
			VectorMA(w.points[0], MAX_WORLD_COORD, invlight, w.points[3]);
			w.numpoints = 4;
			DebugNet_DrawWinding(&w, 2);
			VectorMA(volume->points[i], 8, volume->planes[i].normal, p2);
			DebugNet_DrawLine(volume->points[i], p2, 3);
		}
	}
	else
	{
		//
		VectorCopy(light->origin, w.points[0]);
		w.numpoints = 3;
		for (i = 0; i < volume->numplanes; i++)
		{
			VectorCopy(volume->points[i], w.points[1]);
			VectorCopy(volume->points[(i+1) % volume->numplanes], w.points[2]);
			VL_ChopWinding(&w, &volume->endplane, 0);
			DebugNet_DrawWinding(&w, 2);
			VectorMA(volume->points[i], 8, volume->planes[i].normal, p2);
			DebugNet_DrawLine(volume->points[i], p2, 3);
		}
	}
}

/*
=============
VL_DrawLightmapPixel
=============
*/
void VL_DrawLightmapPixel(int surfaceNum, int x, int y, int color)
{
	winding_t w;
	dsurface_t *ds;
	mesh_t *mesh;

	ds = &drawSurfaces[surfaceNum];

	if (ds->surfaceType == MST_PATCH)
	{
		mesh = lsurfaceTest[surfaceNum]->detailMesh;
		VectorCopy( mesh->verts[(y-ds->lightmapY)*mesh->width+x-ds->lightmapX].xyz, w.points[0]);
		VectorCopy( mesh->verts[(y+1-ds->lightmapY)*mesh->width+x-ds->lightmapX].xyz, w.points[1]);
		VectorCopy( mesh->verts[(y+1-ds->lightmapY)*mesh->width+x+1-ds->lightmapX].xyz, w.points[2]);
		VectorCopy( mesh->verts[(y-ds->lightmapY)*mesh->width+x+1-ds->lightmapX].xyz, w.points[3]);
		w.numpoints = 4;
	}
	else
	{
		VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT - ds->lightmapX, ds->lightmapVecs[0], w.points[0]);
		VectorMA(w.points[0], (float) y - LIGHTMAP_PIXELSHIFT - ds->lightmapY, ds->lightmapVecs[1], w.points[0]);
		VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT - ds->lightmapX, ds->lightmapVecs[0], w.points[1]);
		VectorMA(w.points[1], (float) y - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapY, ds->lightmapVecs[1], w.points[1]);
		VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapX, ds->lightmapVecs[0], w.points[2]);
		VectorMA(w.points[2], (float) y - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapY, ds->lightmapVecs[1], w.points[2]);
		VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapX, ds->lightmapVecs[0], w.points[3]);
		VectorMA(w.points[3], (float) y - LIGHTMAP_PIXELSHIFT - ds->lightmapY, ds->lightmapVecs[1], w.points[3]);
		w.numpoints = 4;
	}
	DebugNet_DrawWinding(&w, color);
}

/*
============
VL_DrawPortals
============
*/
void VL_DrawPortals(void)
{
	int j;
	lportal_t *p;

	for (j = 0; j < numportals * 2; j++)
	{
		p = portals + j;
		DebugNet_DrawWinding(p->winding, 1);
	}
}

/*
============
VL_DrawLeaf
============
*/
void VL_DrawLeaf(int cluster)
{
	int i;
	lleaf_t *leaf;
	lportal_t *p;

	leaf = &leafs[cluster];
	for (i = 0; i < leaf->numportals; i++)
	{
		p = leaf->portals[i];
		DebugNet_DrawWinding(p->winding, 1);
	}
}

#endif //DEBUGNET

/*
=============
VL_SplitWinding
=============
*/
int VL_SplitWinding (winding_t *in, winding_t *back, plane_t *split, float epsilon)
{
	vec_t	dists[128];
	int		sides[128];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t out;
	winding_t	*neww;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if (!counts[SIDE_BACK])
	{
		if (!counts[SIDE_FRONT])
			return SIDE_ON;
		else
			return SIDE_FRONT;
	}
	
	if (!counts[SIDE_FRONT])
	{
		return SIDE_BACK;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	neww = &out;

	neww->numpoints = 0;
	back->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (neww->numpoints >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitWinding -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return SIDE_FRONT;		// can't chop -- fall back to original
		}
		if (back->numpoints >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitWinding -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return SIDE_FRONT;
		}

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			VectorCopy (p1, back->points[back->numpoints]);
			back->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, back->points[back->numpoints]);
			back->numpoints++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		if (neww->numpoints >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitWinding -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return SIDE_FRONT;		// can't chop -- fall back to original
		}

		if (back->numpoints >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitWinding -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return SIDE_FRONT;		// can't chop -- fall back to original
		}

		// generate a split point
		p2 = in->points[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
		VectorCopy (mid, back->points[back->numpoints]);
		back->numpoints++;
	}
	memcpy(in, &out, sizeof(winding_t));
	
	return SIDE_CROSS;
}

/*
=====================
VL_LinkSurfaceIntoCluster
=====================
*/
void VL_LinkSurfaceIntoCluster(int cluster, int surfaceNum)
{
	lleaf_t *leaf;
	int i;

	leaf = &leafs[cluster];

	for (i = 0; i < leaf->numSurfaces; i++)
	{
		if (clustersurfaces[leaf->firstSurface + i] == surfaceNum)
			return;
	}
	for (i = numclustersurfaces; i > leaf->firstSurface + leaf->numSurfaces; i--)
		clustersurfaces[i] = clustersurfaces[i-1];
	for (i = 0; i < portalclusters; i++)
	{
		if (i == cluster)
			continue;
		if (leafs[i].firstSurface >= leaf->firstSurface + leaf->numSurfaces)
			leafs[i].firstSurface++;
	}
	clustersurfaces[leaf->firstSurface + leaf->numSurfaces] = surfaceNum;
	leaf->numSurfaces++;
	numclustersurfaces++;
	if (numclustersurfaces >= MAX_MAP_LEAFFACES)
		Error("MAX_MAP_LEAFFACES");
}

/*
=====================
VL_R_LinkSurface
=====================
*/
void VL_R_LinkSurface(int nodenum, int surfaceNum, winding_t *w)
{
	int leafnum, cluster, res;
	dnode_t *node;
	dplane_t *plane;
	winding_t back;
	plane_t split;

	while(nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];

		VectorCopy(plane->normal, split.normal);
		split.dist = plane->dist;
		res = VL_SplitWinding (w, &back, &split, 0.1);

		if (res == SIDE_FRONT)
		{
			nodenum = node->children[0];
		}
		else if (res == SIDE_BACK)
		{
			nodenum = node->children[1];
		}
		else if (res == SIDE_ON)
		{
			memcpy(&back, w, sizeof(winding_t));
			VL_R_LinkSurface(node->children[1], surfaceNum, &back);
			nodenum = node->children[0];
		}
		else
		{
			VL_R_LinkSurface(node->children[1], surfaceNum, &back);
			nodenum = node->children[0];
		}
	}
	leafnum = -nodenum - 1;
	cluster = dleafs[leafnum].cluster;
	if (cluster != -1)
	{
		VL_LinkSurfaceIntoCluster(cluster, surfaceNum);
	}
}

/*
=====================
VL_LinkSurfaces

maybe link each facet seperately instead of the test surfaces?
=====================
*/
void VL_LinkSurfaces(void)
{
	int i, j;
	lsurfaceTest_t *test;
	lFacet_t *facet;
	winding_t winding;

	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		for (j = 0; j < test->numFacets; j++)
		{
			facet = &test->facets[j];
			memcpy(winding.points, facet->points, facet->numpoints * sizeof(vec3_t));
			winding.numpoints = facet->numpoints;
			VL_R_LinkSurface(0, i, &winding);
		}
	}
}

/*
=====================
VL_TextureMatrixFromPoints
=====================
*/
void VL_TextureMatrixFromPoints( lFacet_t *f, drawVert_t *a, drawVert_t *b, drawVert_t *c ) {
	int			i, j;
	float		t;
	float		m[3][4];
	float		s;

	// This is an incredibly stupid way of solving a three variable equation
	for ( i = 0 ; i < 2 ; i++ ) {

		m[0][0] = a->xyz[0];
		m[0][1] = a->xyz[1];
		m[0][2] = a->xyz[2];
		m[0][3] = a->st[i];

		m[1][0] = b->xyz[0];
		m[1][1] = b->xyz[1];
		m[1][2] = b->xyz[2];
		m[1][3] = b->st[i];

		m[2][0] = c->xyz[0];
		m[2][1] = c->xyz[1];
		m[2][2] = c->xyz[2];
		m[2][3] = c->st[i];

		if ( fabs(m[1][0]) > fabs(m[0][0]) && fabs(m[1][0]) > fabs(m[2][0]) ) {
			for ( j = 0 ; j < 4 ; j ++ ) {
				t = m[0][j];
				m[0][j] = m[1][j];
				m[1][j] = t;
			}
		} else if ( fabs(m[2][0]) > fabs(m[0][0]) && fabs(m[2][0]) > fabs(m[1][0]) ) {
			for ( j = 0 ; j < 4 ; j ++ ) {
				t = m[0][j];
				m[0][j] = m[2][j];
				m[2][j] = t;
			}
		}

		s = 1.0 / m[0][0];
		m[0][0] *= s;
		m[0][1] *= s;
		m[0][2] *= s;
		m[0][3] *= s;

		s = m[1][0];
		m[1][0] -= m[0][0] * s;
		m[1][1] -= m[0][1] * s;
		m[1][2] -= m[0][2] * s;
		m[1][3] -= m[0][3] * s;

		s = m[2][0];
		m[2][0] -= m[0][0] * s;
		m[2][1] -= m[0][1] * s;
		m[2][2] -= m[0][2] * s;
		m[2][3] -= m[0][3] * s;

		if ( fabs(m[2][1]) > fabs(m[1][1]) ) {
			for ( j = 0 ; j < 4 ; j ++ ) {
				t = m[1][j];
				m[1][j] = m[2][j];
				m[2][j] = t;
			}
		}

		s = 1.0 / m[1][1];
		m[1][0] *= s;
		m[1][1] *= s;
		m[1][2] *= s;
		m[1][3] *= s;

		s = m[2][1];// / m[1][1];
		m[2][0] -= m[1][0] * s;
		m[2][1] -= m[1][1] * s;
		m[2][2] -= m[1][2] * s;
		m[2][3] -= m[1][3] * s;

		s = 1.0 / m[2][2];
		m[2][0] *= s;
		m[2][1] *= s;
		m[2][2] *= s;
		m[2][3] *= s;

		f->textureMatrix[i][2] = m[2][3];
		f->textureMatrix[i][1] = m[1][3] - f->textureMatrix[i][2] * m[1][2];
		f->textureMatrix[i][0] = m[0][3] - f->textureMatrix[i][2] * m[0][2] - f->textureMatrix[i][1] * m[0][1];

		f->textureMatrix[i][3] = 0;
/*
		s = fabs( DotProduct( a->xyz, f->textureMatrix[i] ) - a->st[i] );
		if ( s > 0.01 ) {
			Error( "Bad textureMatrix" );
		}
		s = fabs( DotProduct( b->xyz, f->textureMatrix[i] ) - b->st[i] );
		if ( s > 0.01 ) {
			Error( "Bad textureMatrix" );
		}
		s = fabs( DotProduct( c->xyz, f->textureMatrix[i] ) - c->st[i] );
		if ( s > 0.01 ) {
			Error( "Bad textureMatrix" );
		}
*/
	}
}

/*
=====================
VL_LightmapMatrixFromPoints
=====================
*/
void VL_LightmapMatrixFromPoints( dsurface_t *dsurf, shaderInfo_t *si, lFacet_t *f, drawVert_t *a, drawVert_t *b, drawVert_t *c ) {
	int			i, j;
	float		t;
	float		m[3][4], al, bl, cl;
	float		s;
	int			h, w, ssize;
	vec3_t		mins, maxs, delta, size, planeNormal;
	drawVert_t	*verts;
	static int	message;

	// vertex-lit triangle model
	if ( dsurf->surfaceType == MST_TRIANGLE_SOUP ) {
		return;
	}
	
	if ( dsurf->lightmapNum < 0 ) {
		return;		// doesn't need lighting
	}

	VectorClear(f->mins);
	if (dsurf->surfaceType != MST_PATCH)
	{
		ssize = samplesize;
		if (si->lightmapSampleSize)
			ssize = si->lightmapSampleSize;
		ClearBounds( mins, maxs );
		verts = &drawVerts[dsurf->firstVert];
		for ( i = 0 ; i < dsurf->numVerts ; i++ ) {
			AddPointToBounds( verts[i].xyz, mins, maxs );
		}
		// round to the lightmap resolution
		for ( i = 0 ; i < 3 ; i++ ) {
			mins[i] = ssize * floor( mins[i] / ssize );
			maxs[i] = ssize * ceil( maxs[i] / ssize );
			f->mins[i] = mins[i];
			size[i] = (maxs[i] - mins[i]) / ssize + 1;
		}
		// the two largest axis will be the lightmap size
		VectorClear(f->lightmapMatrix[0]);
		f->lightmapMatrix[0][3] = 0;
		VectorClear(f->lightmapMatrix[1]);
		f->lightmapMatrix[1][3] = 0;

		planeNormal[0] = fabs( dsurf->lightmapVecs[2][0] );
		planeNormal[1] = fabs( dsurf->lightmapVecs[2][1] );
		planeNormal[2] = fabs( dsurf->lightmapVecs[2][2] );

		if ( planeNormal[0] >= planeNormal[1] && planeNormal[0] >= planeNormal[2] ) {
			w = size[1];
			h = size[2];
			f->lightmapMatrix[0][1] = 1.0 / ssize;
			f->lightmapMatrix[1][2] = 1.0 / ssize;
		} else if ( planeNormal[1] >= planeNormal[0] && planeNormal[1] >= planeNormal[2] ) {
			w = size[0];
			h = size[2];
			f->lightmapMatrix[0][0] = 1.0 / ssize;
			f->lightmapMatrix[1][2] = 1.0 / ssize;
		} else {
			w = size[0];
			h = size[1];
			f->lightmapMatrix[0][0] = 1.0 / ssize;
			f->lightmapMatrix[1][1] = 1.0 / ssize;
		}
		if ( w > LIGHTMAP_WIDTH ) {
			VectorScale ( f->lightmapMatrix[0], (float)LIGHTMAP_SIZE/w, f->lightmapMatrix[0] );
		}
		
		if ( h > LIGHTMAP_HEIGHT ) {
			VectorScale ( f->lightmapMatrix[1], (float)LIGHTMAP_SIZE/h, f->lightmapMatrix[1] );
		}
		VectorSubtract(a->xyz, f->mins, delta);
		s = (DotProduct( delta, f->lightmapMatrix[0] ) + dsurf->lightmapX + 0.5) / LIGHTMAP_SIZE;
		if ( fabs(s - a->lightmap[0]) > 0.01 ) {
			_printf( "Bad lightmapMatrix" );
		}
		t = (DotProduct( delta, f->lightmapMatrix[1] ) + dsurf->lightmapY + 0.5) / LIGHTMAP_SIZE;
		if ( fabs(t - a->lightmap[1]) > 0.01 ) {
			_printf( "Bad lightmapMatrix" );
		}
		VectorSubtract(b->xyz, f->mins, delta);
		s = (DotProduct( delta, f->lightmapMatrix[0] ) + dsurf->lightmapX + 0.5) / LIGHTMAP_SIZE;
		if ( fabs(s - b->lightmap[0]) > 0.01 ) {
			_printf( "Bad lightmapMatrix" );
		}
		t = (DotProduct( delta, f->lightmapMatrix[1] ) + dsurf->lightmapY + 0.5) / LIGHTMAP_SIZE;
		if ( fabs(t - b->lightmap[1]) > 0.01 ) {
			_printf( "Bad lightmapMatrix" );
		}
		VectorSubtract(c->xyz, f->mins, delta);
		s = (DotProduct( delta, f->lightmapMatrix[0] ) + dsurf->lightmapX + 0.5) / LIGHTMAP_SIZE;
		if ( fabs(s - c->lightmap[0]) > 0.01 ) {
			_printf( "Bad lightmapMatrix" );
		}
		t = (DotProduct( delta, f->lightmapMatrix[1] ) + dsurf->lightmapY + 0.5) / LIGHTMAP_SIZE;
		if ( fabs(t - c->lightmap[1]) > 0.01 ) {
			_printf( "Bad lightmapMatrix" );
		}
		VectorAdd(f->mins, surfaceOrigin[dsurf - drawSurfaces], f->mins);
		return;
	}
	// This is an incredibly stupid way of solving a three variable equation
	for ( i = 0 ; i < 2 ; i++ ) {

		if (i)
			al = a->lightmap[i] - ((float) dsurf->lightmapY + 0.5) / LIGHTMAP_SIZE;
		else
			al = a->lightmap[i] - ((float) dsurf->lightmapX + 0.5) / LIGHTMAP_SIZE;

		m[0][0] = a->xyz[0] - f->mins[0];
		m[0][1] = a->xyz[1] - f->mins[1];
		m[0][2] = a->xyz[2] - f->mins[2];
		m[0][3] = al;

		if (i)
			bl = b->lightmap[i] - ((float) dsurf->lightmapY + 0.5) / LIGHTMAP_SIZE;
		else
			bl = b->lightmap[i] - ((float) dsurf->lightmapX + 0.5) / LIGHTMAP_SIZE;

		m[1][0] = b->xyz[0] - f->mins[0];
		m[1][1] = b->xyz[1] - f->mins[1];
		m[1][2] = b->xyz[2] - f->mins[2];
		m[1][3] = bl;

		if (i)
			cl = c->lightmap[i] - ((float) dsurf->lightmapY + 0.5) / LIGHTMAP_SIZE;
		else
			cl = c->lightmap[i] - ((float) dsurf->lightmapX + 0.5) / LIGHTMAP_SIZE;

		m[2][0] = c->xyz[0] - f->mins[0];
		m[2][1] = c->xyz[1] - f->mins[1];
		m[2][2] = c->xyz[2] - f->mins[2];
		m[2][3] = cl;

		if ( fabs(m[1][0]) > fabs(m[0][0]) && fabs(m[1][0]) >= fabs(m[2][0]) ) {
			for ( j = 0 ; j < 4 ; j ++ ) {
				t = m[0][j];
				m[0][j] = m[1][j];
				m[1][j] = t;
			}
		} else if ( fabs(m[2][0]) > fabs(m[0][0]) && fabs(m[2][0]) >= fabs(m[1][0]) ) {
			for ( j = 0 ; j < 4 ; j ++ ) {
				t = m[0][j];
				m[0][j] = m[2][j];
				m[2][j] = t;
			}
		}

		if (m[0][0])
		{
			s = 1.0 / m[0][0];
			m[0][0] *= s;
			m[0][1] *= s;
			m[0][2] *= s;
			m[0][3] *= s;

			s = m[1][0];
			m[1][0] -= m[0][0] * s;
			m[1][1] -= m[0][1] * s;
			m[1][2] -= m[0][2] * s;
			m[1][3] -= m[0][3] * s;

			s = m[2][0];
			m[2][0] -= m[0][0] * s;
			m[2][1] -= m[0][1] * s;
			m[2][2] -= m[0][2] * s;
			m[2][3] -= m[0][3] * s;
		}

		if ( fabs(m[2][1]) > fabs(m[1][1]) ) {
			for ( j = 0 ; j < 4 ; j ++ ) {
				t = m[1][j];
				m[1][j] = m[2][j];
				m[2][j] = t;
			}
		}

		if (m[1][1])
		{
			s = 1.0 / m[1][1];
			m[1][0] *= s;
			m[1][1] *= s;
			m[1][2] *= s;
			m[1][3] *= s;

			s = m[2][1];
			m[2][0] -= m[1][0] * s;
			m[2][1] -= m[1][1] * s;
			m[2][2] -= m[1][2] * s;
			m[2][3] -= m[1][3] * s;
		}

		if (m[2][2])
		{
			s = 1.0 / m[2][2];
			m[2][0] *= s;
			m[2][1] *= s;
			m[2][2] *= s;
			m[2][3] *= s;
		}

		f->lightmapMatrix[i][2] = m[2][3];
		f->lightmapMatrix[i][1] = m[1][3] - f->lightmapMatrix[i][2] * m[1][2];
		f->lightmapMatrix[i][0] = m[0][3] - f->lightmapMatrix[i][2] * m[0][2] - f->lightmapMatrix[i][1] * m[0][1];

		f->lightmapMatrix[i][3] = 0;

		VectorSubtract(a->xyz, f->mins, delta);
		s = fabs( DotProduct( delta, f->lightmapMatrix[i] ) - al );
		if ( s > 0.01 ) {
			if (!message)
				_printf( "Bad lightmapMatrix\n" );
			message = qtrue;
		}
		VectorSubtract(b->xyz, f->mins, delta);
		s = fabs( DotProduct( delta, f->lightmapMatrix[i] ) - bl );
		if ( s > 0.01 ) {
			if (!message)
				_printf( "Bad lightmapMatrix\n" );
			message = qtrue;
		}
		VectorSubtract(c->xyz, f->mins, delta);
		s = fabs( DotProduct( delta, f->lightmapMatrix[i] ) - cl );
		if ( s > 0.01 ) {
			if (!message)
				_printf( "Bad lightmapMatrix\n" );
			message = qtrue;
		}
		VectorAdd(f->mins, surfaceOrigin[dsurf - drawSurfaces], f->mins);
	}
}

/*
=============
Plane_Equal
=============
*/
#define	NORMAL_EPSILON	0.0001
#define	DIST_EPSILON	0.02

int Plane_Equal(plane_t *a, plane_t *b, int flip)
{
	vec3_t normal;
	float dist;

	if (flip) {
		normal[0] = - b->normal[0];
		normal[1] = - b->normal[1];
		normal[2] = - b->normal[2];
		dist = - b->dist;
	}
	else {
		normal[0] = b->normal[0];
		normal[1] = b->normal[1];
		normal[2] = b->normal[2];
		dist = b->dist;
	}
	if (
	   fabs(a->normal[0] - normal[0]) < NORMAL_EPSILON
	&& fabs(a->normal[1] - normal[1]) < NORMAL_EPSILON
	&& fabs(a->normal[2] - normal[2]) < NORMAL_EPSILON
	&& fabs(a->dist - dist) < DIST_EPSILON )
		return qtrue;
	return qfalse;
}

/*
=============
VL_PlaneFromPoints
=============
*/
qboolean VL_PlaneFromPoints( plane_t *plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane->normal );
	if ( VectorNormalize( plane->normal, plane->normal ) == 0 ) {
		return qfalse;
	}

	plane->dist = DotProduct( a, plane->normal );
	return qtrue;
}

/*
=====================
VL_GenerateBoundaryForPoints
=====================
*/
void VL_GenerateBoundaryForPoints( plane_t *boundary, plane_t *plane, vec3_t a, vec3_t b ) {
	vec3_t	d1;

	// make a perpendicular vector to the edge and the surface
	VectorSubtract( a, b, d1 );
	CrossProduct( plane->normal, d1, boundary->normal );
	VectorNormalize( boundary->normal, boundary->normal );
	boundary->dist = DotProduct( a, boundary->normal );
}

/*
=====================
VL_GenerateFacetFor3Points
=====================
*/
qboolean VL_GenerateFacetFor3Points( dsurface_t *dsurf, shaderInfo_t *si, lFacet_t *f, drawVert_t *a, drawVert_t *b, drawVert_t *c ) {
	//
	vec3_t dir;
	int i;

	// if we can't generate a valid plane for the points, ignore the facet
	if ( !VL_PlaneFromPoints( &f->plane, a->xyz, b->xyz, c->xyz ) ) {
		f->numpoints = 0;
		return qfalse;
	}

	f->num = numfacets++;

	VectorAdd( a->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[0] );
	VectorAdd( b->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[1] );
	VectorAdd( c->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[2] );

	f->lightmapCoords[0][0] = a->lightmap[0];
	f->lightmapCoords[0][1] = a->lightmap[1];
	f->lightmapCoords[1][0] = b->lightmap[0];
	f->lightmapCoords[1][1] = b->lightmap[1];
	f->lightmapCoords[2][0] = c->lightmap[0];
	f->lightmapCoords[2][1] = c->lightmap[1];

	VL_GenerateBoundaryForPoints( &f->boundaries[0], &f->plane, f->points[0], f->points[1] );
	VL_GenerateBoundaryForPoints( &f->boundaries[1], &f->plane, f->points[1], f->points[2] );
	VL_GenerateBoundaryForPoints( &f->boundaries[2], &f->plane, f->points[2], f->points[0] );

	for (i = 0; i < 3; i++)
	{
		VectorSubtract(f->points[(i+1)%3], f->points[i], dir);
		if (VectorLength(dir) < 0.1)
			return qfalse;
	}

	VL_TextureMatrixFromPoints( f, a, b, c );
	VL_LightmapMatrixFromPoints( dsurf, si, f, a, b, c );

	f->numpoints = 3;

	return qtrue;
}

/*
=====================
VL_GenerateFacetFor4Points

Attempts to use four points as a planar quad
=====================
*/
#define	PLANAR_EPSILON	0.1
qboolean VL_GenerateFacetFor4Points( dsurface_t *dsurf, shaderInfo_t *si, lFacet_t *f, drawVert_t *a, drawVert_t *b, drawVert_t *c, drawVert_t *d ) {
	float	dist;
	vec3_t dir;
	int i;
	plane_t plane;

	// if we can't generate a valid plane for the points, ignore the facet
	if ( !VL_PlaneFromPoints( &f->plane, a->xyz, b->xyz, c->xyz ) ) {
		f->numpoints = 0;
		return qfalse;
	}

	// if the fourth point is also on the plane, we can make a quad facet
	dist = DotProduct( d->xyz, f->plane.normal ) - f->plane.dist;
	if ( fabs( dist ) > PLANAR_EPSILON ) {
		f->numpoints = 0;
		return qfalse;
	}

	VectorAdd( a->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[0] );
	VectorAdd( b->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[1] );
	VectorAdd( c->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[2] );
	VectorAdd( d->xyz, surfaceOrigin[dsurf - drawSurfaces], f->points[3] );

	for (i = 1; i < 4; i++)
	{
		if ( !VL_PlaneFromPoints( &plane, f->points[i], f->points[(i+1) % 4], f->points[(i+2) % 4]) ) {
			f->numpoints = 0;
			return qfalse;
		}

		if (!Plane_Equal(&f->plane, &plane, qfalse)) {
			f->numpoints = 0;
			return qfalse;
		}
	}

	f->lightmapCoords[0][0] = a->lightmap[0];
	f->lightmapCoords[0][1] = a->lightmap[1];
	f->lightmapCoords[1][0] = b->lightmap[0];
	f->lightmapCoords[1][1] = b->lightmap[1];
	f->lightmapCoords[2][0] = c->lightmap[0];
	f->lightmapCoords[2][1] = c->lightmap[1];
	f->lightmapCoords[3][0] = d->lightmap[0];
	f->lightmapCoords[3][1] = d->lightmap[1];

	VL_GenerateBoundaryForPoints( &f->boundaries[0], &f->plane, f->points[0], f->points[1] );
	VL_GenerateBoundaryForPoints( &f->boundaries[1], &f->plane, f->points[1], f->points[2] );
	VL_GenerateBoundaryForPoints( &f->boundaries[2], &f->plane, f->points[2], f->points[3] );
	VL_GenerateBoundaryForPoints( &f->boundaries[3], &f->plane, f->points[3], f->points[0] );

	for (i = 0; i < 4; i++)
	{
		VectorSubtract(f->points[(i+1)%4], f->points[i], dir);
		if (VectorLength(dir) < 0.1)
			return qfalse;
	}

	VL_TextureMatrixFromPoints( f, a, b, c );
	VL_LightmapMatrixFromPoints( dsurf, si, f, a, b, c );

	f->num = numfacets++;
	f->numpoints = 4;

	return qtrue;
}

/*
===============
VL_SphereFromBounds
===============
*/
void VL_SphereFromBounds( vec3_t mins, vec3_t maxs, vec3_t origin, float *radius ) {
	vec3_t		temp;

	VectorAdd( mins, maxs, origin );
	VectorScale( origin, 0.5, origin );
	VectorSubtract( maxs, origin, temp );
	*radius = VectorLength( temp );
}

/*
====================
VL_FacetsForTriangleSurface
====================
*/
void VL_FacetsForTriangleSurface( dsurface_t *dsurf, shaderInfo_t *si, lsurfaceTest_t *test ) {
	int			i;
	drawVert_t	*v1, *v2, *v3, *v4;
	int			count;
	int			i1, i2, i3, i4, i5, i6;

	test->patch = qfalse;
	if (dsurf->surfaceType == MST_TRIANGLE_SOUP)
		test->trisoup = qtrue;
	else
		test->trisoup = qfalse;
	test->numFacets = dsurf->numIndexes / 3;
	test->facets = malloc( sizeof( test->facets[0] ) * test->numFacets );
	test->shader = si;

	count = 0;
	for ( i = 0 ; i < test->numFacets ; i++ ) {
		i1 = drawIndexes[ dsurf->firstIndex + i*3 ];
		i2 = drawIndexes[ dsurf->firstIndex + i*3 + 1 ];
		i3 = drawIndexes[ dsurf->firstIndex + i*3 + 2 ];

		v1 = &drawVerts[ dsurf->firstVert + i1 ];
		v2 = &drawVerts[ dsurf->firstVert + i2 ];
		v3 = &drawVerts[ dsurf->firstVert + i3 ];

		// try and make a quad out of two triangles
		if ( i != test->numFacets - 1 ) {
			i4 = drawIndexes[ dsurf->firstIndex + i*3 + 3 ];
			i5 = drawIndexes[ dsurf->firstIndex + i*3 + 4 ];
			i6 = drawIndexes[ dsurf->firstIndex + i*3 + 5 ];
			if ( i4 == i3 && i5 == i2 ) {
				v4 = &drawVerts[ dsurf->firstVert + i6 ];
				if ( VL_GenerateFacetFor4Points( dsurf, si, &test->facets[count], v1, v2, v4, v3 ) ) {
					count++;
					i++;		// skip next tri
					continue;
				}
			}
		}

		if (VL_GenerateFacetFor3Points( dsurf, si, &test->facets[count], v1, v2, v3 )) {
			count++;
		}
	}		

	// we may have turned some pairs into quads
	test->numFacets = count;
}

/*
====================
VL_FacetsForPatch
====================
*/
void VL_FacetsForPatch( dsurface_t *dsurf, int surfaceNum, shaderInfo_t *si, lsurfaceTest_t *test ) {
	int			i, j, x, y;
	drawVert_t	*v1, *v2, *v3, *v4;
	int			count, ssize;
	mesh_t		mesh;
	mesh_t		*subdivided, *detailmesh, *newmesh;
	int widthtable[LIGHTMAP_SIZE], heighttable[LIGHTMAP_SIZE];

	mesh.width = dsurf->patchWidth;
	mesh.height = dsurf->patchHeight;
	mesh.verts = &drawVerts[ dsurf->firstVert ];

	newmesh = SubdivideMesh( mesh, 8, 999 );
	PutMeshOnCurve( *newmesh );
	MakeMeshNormals( *newmesh );

	subdivided = RemoveLinearMeshColumnsRows( newmesh );
	FreeMesh(newmesh);

	//	DebugNet_RemoveAllPolys();
	//	DebugNet_DrawMesh(subdivided);

	ssize = samplesize;
	if (si->lightmapSampleSize)
		ssize = si->lightmapSampleSize;

	if ( dsurf->lightmapNum >= 0 ) {

		detailmesh = SubdivideMeshQuads( subdivided, ssize, LIGHTMAP_SIZE, widthtable, heighttable);
		test->detailMesh = detailmesh;

		// DebugNet_RemoveAllPolys();
		// DebugNet_DrawMesh(detailmesh);

		if ( detailmesh->width != dsurf->lightmapWidth || detailmesh->height != dsurf->lightmapHeight ) {
			Error( "Mesh lightmap miscount");
		}
	}
	else {
		test->detailMesh = NULL;
		memset(widthtable, 0, sizeof(widthtable));
		memset(heighttable, 0, sizeof(heighttable));
	}

	test->patch = qtrue;
	test->trisoup = qfalse;
	test->numFacets = ( subdivided->width - 1 ) * ( subdivided->height - 1 ) * 2;
	test->facets = malloc( sizeof( test->facets[0] ) * test->numFacets );
	test->shader = si;

	count = 0;
	x = 0;
	for ( i = 0 ; i < subdivided->width - 1 ; i++ ) {
		y = 0;
		for ( j = 0 ; j < subdivided->height - 1 ; j++ ) {

			v1 = subdivided->verts + j * subdivided->width + i;
			v2 = v1 + 1;
			v3 = v1 + subdivided->width + 1;
			v4 = v1 + subdivided->width;

			if ( VL_GenerateFacetFor4Points( dsurf, si, &test->facets[count], v1, v4, v3, v2 ) ) {
				test->facets[count].x = x;
				test->facets[count].y = y;
				test->facets[count].width = widthtable[i];
				test->facets[count].height = heighttable[j];
				count++;
			} else {
				if (VL_GenerateFacetFor3Points( dsurf, si, &test->facets[count], v1, v4, v3 )) {
					test->facets[count].x = x;
					test->facets[count].y = y;
					test->facets[count].width = widthtable[i];
					test->facets[count].height = heighttable[j];
					count++;
				}
				if (VL_GenerateFacetFor3Points( dsurf, si, &test->facets[count], v1, v3, v2 )) {
					test->facets[count].x = x;
					test->facets[count].y = y;
					test->facets[count].width = widthtable[i];
					test->facets[count].height = heighttable[j];
					count++;
				}
			}
			y += heighttable[j];
		}
		x += widthtable[i];
	}
	test->numFacets = count;

	FreeMesh(subdivided);
}

/*
=====================
VL_InitSurfacesForTesting
=====================
*/
void VL_InitSurfacesForTesting( void ) {

	int				i, j, k;
	dsurface_t		*dsurf;
	lsurfaceTest_t	*test;
	shaderInfo_t	*si;
	lFacet_t		*facet;

	for ( i = 0 ; i < numDrawSurfaces ; i++ ) {
		// don't light the entity surfaces with vlight
		if ( entitySurface[i] )
			continue;
		//
		dsurf = &drawSurfaces[ i ];
		if ( !dsurf->numIndexes && !dsurf->patchWidth ) {
			continue;
		}

		si = ShaderInfoForShader( dshaders[ dsurf->shaderNum].shader );
		// if the surface is translucent and does not cast an alpha shadow
		if ( (si->contents & CONTENTS_TRANSLUCENT) && !(si->surfaceFlags & SURF_ALPHASHADOW) ) {
			// if the surface has no lightmap
			if ( dsurf->lightmapNum < 0 )
				continue;
		}

		test = malloc( sizeof( *test ) );
		memset(test, 0, sizeof( *test ));
		test->mutex = MutexAlloc();
		test->numvolumes = 0;
		if (si->forceTraceLight)
			test->always_tracelight = qtrue;
		else if (si->forceVLight)
			test->always_vlight = qtrue;
		lsurfaceTest[i] = test;

		if ( dsurf->surfaceType == MST_TRIANGLE_SOUP || dsurf->surfaceType == MST_PLANAR ) {
			VL_FacetsForTriangleSurface( dsurf, si, test );
		} else if ( dsurf->surfaceType == MST_PATCH ) {
			VL_FacetsForPatch( dsurf, i, si, test );
		}
		if (numfacets >= MAX_FACETS)
			Error("numfacets >= MAX_FACETS (%d)", MAX_FACETS);

		ClearBounds( test->mins, test->maxs );
		for (j = 0; j < test->numFacets; j++)
		{
			facet = &test->facets[j];
			for ( k = 0 ; k < facet->numpoints; k++) {
				AddPointToBounds( facet->points[k], test->mins, test->maxs );
			}
		}
		VL_SphereFromBounds( test->mins, test->maxs, test->origin, &test->radius );
	}
	_printf("%6d facets\n", numfacets);
	_printf("linking surfaces...\n");
	VL_LinkSurfaces();
}

/*
=============
VL_ChopWinding
=============
*/
int VL_ChopWinding (winding_t *in, plane_t *split, float epsilon)
{
	vec_t	dists[128];
	int		sides[128];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t out;
	winding_t	*neww;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if (!counts[SIDE_BACK])
	{
		if (!counts[SIDE_FRONT])
			return SIDE_ON;
		else
			return SIDE_FRONT;
	}
	
	if (!counts[SIDE_FRONT])
	{
		return SIDE_BACK;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	neww = &out;

	neww->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (neww->numpoints >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_ChopWinding -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return SIDE_FRONT;		// can't chop -- fall back to original
		}

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		if (neww->numpoints >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_ChopWinding -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return SIDE_FRONT;		// can't chop -- fall back to original
		}

		// generate a split point
		p2 = in->points[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}
	memcpy(in, &out, sizeof(winding_t));
	
	return SIDE_CROSS;
}

/*
=============
VL_ChopWindingWithBrush

  returns all winding fragments outside the brush
=============
*/
int VL_ChopWindingWithBrush(winding_t *w, dbrush_t *brush, winding_t *outwindings, int maxout)
{
	int i, res, numout;
	winding_t front, back;
	plane_t plane;

	numout = 0;
	memcpy(front.points, w->points, w->numpoints * sizeof(vec3_t));
	front.numpoints = w->numpoints;
	for (i = 0; i < brush->numSides; i++)
	{
		VectorCopy(dplanes[ dbrushsides[ brush->firstSide + i ].planeNum ].normal, plane.normal);
		VectorInverse(plane.normal);
		plane.dist = -dplanes[ dbrushsides[ brush->firstSide + i ].planeNum ].dist;
		res = VL_SplitWinding(&front, &back, &plane, 0.1);
		if (res == SIDE_BACK || res == SIDE_ON)
		{
			memcpy(outwindings[0].points, w->points, w->numpoints * sizeof(vec3_t));
			outwindings[0].numpoints = w->numpoints;
			return 1;	//did not intersect
		}
		if (res != SIDE_FRONT)
		{
			if (numout >= maxout)
			{
				_printf("WARNING: VL_ChopWindingWithBrush: more than %d windings\n", maxout);
				return 0;
			}
			memcpy(outwindings[numout].points, back.points, back.numpoints * sizeof(vec3_t));
			outwindings[numout].numpoints = back.numpoints;
			numout++;
		}
	}
	return numout;
}

/*
=============
VL_WindingAreaOutsideBrushes
=============
*/
float VL_WindingAreaOutsideBrushes(winding_t *w, int *brushnums, int numbrushes)
{
	int i, j, numwindings[2], n;
	winding_t windingsbuf[2][64];
	dbrush_t *brush;
	float area;

	memcpy(windingsbuf[0][0].points, w->points, w->numpoints * sizeof(vec3_t));
	windingsbuf[0][0].numpoints = w->numpoints;
	numwindings[0] = 1;
	for (i = 0; i < numbrushes; i++)
	{
		brush = &dbrushes[brushnums[i]];
		if (!(dshaders[brush->shaderNum].contentFlags & (
					CONTENTS_LAVA
					| CONTENTS_SLIME
					| CONTENTS_WATER
					| CONTENTS_FOG
					| CONTENTS_AREAPORTAL
					| CONTENTS_PLAYERCLIP
					| CONTENTS_MONSTERCLIP
					| CONTENTS_CLUSTERPORTAL
					| CONTENTS_DONOTENTER
					| CONTENTS_BODY
					| CONTENTS_CORPSE
					| CONTENTS_TRANSLUCENT
					| CONTENTS_TRIGGER
					| CONTENTS_NODROP) ) &&
			(dshaders[brush->shaderNum].contentFlags & CONTENTS_SOLID) )
		{
			numwindings[!(i & 1)] = 0;
			for (j = 0; j < numwindings[i&1]; j++)
			{
				n = VL_ChopWindingWithBrush(&windingsbuf[i&1][j], brush,
											&windingsbuf[!(i&1)][numwindings[!(i&1)]],
											64 - numwindings[!(i&1)]);
				numwindings[!(i&1)] += n;
			}
			if (!numwindings[!(i&1)])
				return 0;
		}
		else
		{
			for (j = 0; j < numwindings[i&1]; j++)
			{
				windingsbuf[!(i&1)][j] = windingsbuf[i&1][j];
			}
			numwindings[!(i&1)] = numwindings[i&1];
		}
	}
	area = 0;
	for (j = 0; j < numwindings[i&1]; j++)
	{
		area += WindingArea(&windingsbuf[i&1][j]);
	}
	return area;
}

/*
=============
VL_R_WindingAreaOutsideSolid
=============
*/
float VL_R_WindingAreaOutsideSolid(winding_t *w, vec3_t normal, int nodenum)
{
	int leafnum, res;
	float area;
	dnode_t *node;
	dleaf_t *leaf;
	dplane_t *plane;
	winding_t back;
	plane_t split;

	area = 0;
	while(nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];

		VectorCopy(plane->normal, split.normal);
		split.dist = plane->dist;
		res = VL_SplitWinding (w, &back, &split, 0.1);

		if (res == SIDE_FRONT)
		{
			nodenum = node->children[0];
		}
		else if (res == SIDE_BACK)
		{
			nodenum = node->children[1];
		}
		else if (res == SIDE_ON)
		{
			if (DotProduct(normal, plane->normal) > 0)
				nodenum = node->children[0];
			else
				nodenum = node->children[1];
		}
		else
		{
			area += VL_R_WindingAreaOutsideSolid(&back, normal, node->children[1]);
			nodenum = node->children[0];
		}
	}
	leafnum = -nodenum - 1;
	leaf = &dleafs[leafnum];
	if (leaf->cluster != -1)
	{
		area += VL_WindingAreaOutsideBrushes(w, &dleafbrushes[leaf->firstLeafBrush], leaf->numLeafBrushes);
	}
	return area;
}

/*
=============
VL_WindingAreaOutsideSolid
=============
*/
float VL_WindingAreaOutsideSolid(winding_t *w, vec3_t normal)
{
	return VL_R_WindingAreaOutsideSolid(w, normal, 0);
}

/*
=============
VL_ChopWindingWithFacet
=============
*/
float VL_ChopWindingWithFacet(winding_t *w, lFacet_t *facet)
{
	int i;

	for (i = 0; i < facet->numpoints; i++)
	{
		if (VL_ChopWinding(w, &facet->boundaries[i], 0) == SIDE_BACK)
			return 0;
	}
	if (nostitching)
		return WindingArea(w);
	else
		return VL_WindingAreaOutsideSolid(w, facet->plane.normal);
}

/*
=============
VL_CalcVisibleLightmapPixelArea

nice brute force ;)
=============
*/
void VL_CalcVisibleLightmapPixelArea(void)
{
	int				i, j, x, y, k;
	dsurface_t		*ds;
	lsurfaceTest_t	*test;
	mesh_t			*mesh;
	winding_t w, tmpw;
	float area;

	_printf("calculating visible lightmap pixel area...\n");
	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		ds = &drawSurfaces[ i ];

		if ( ds->lightmapNum < 0 )
			continue;

		for (y = 0; y < ds->lightmapHeight; y++)
		{
			for (x = 0; x < ds->lightmapWidth; x++)
			{
				if (ds->surfaceType == MST_PATCH)
				{
					if (y == ds->lightmapHeight-1)
						continue;
					if (x == ds->lightmapWidth-1)
						continue;
					mesh = lsurfaceTest[i]->detailMesh;
					VectorCopy( mesh->verts[y*mesh->width+x].xyz, w.points[0]);
					VectorCopy( mesh->verts[(y+1)*mesh->width+x].xyz, w.points[1]);
					VectorCopy( mesh->verts[(y+1)*mesh->width+x+1].xyz, w.points[2]);
					VectorCopy( mesh->verts[y*mesh->width+x+1].xyz, w.points[3]);
					w.numpoints = 4;
					if (nostitching)
						area = WindingArea(&w);
					else
						area = VL_WindingAreaOutsideSolid(&w, mesh->verts[y*mesh->width+x].normal);
				}
				else
				{
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT, ds->lightmapVecs[0], w.points[0]);
					VectorMA(w.points[0], (float) y - LIGHTMAP_PIXELSHIFT, ds->lightmapVecs[1], w.points[0]);
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT, ds->lightmapVecs[0], w.points[3]);
					VectorMA(w.points[3], (float) y - LIGHTMAP_PIXELSHIFT + 1, ds->lightmapVecs[1], w.points[3]);
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT + 1, ds->lightmapVecs[0], w.points[2]);
					VectorMA(w.points[2], (float) y - LIGHTMAP_PIXELSHIFT + 1, ds->lightmapVecs[1], w.points[2]);
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT + 1, ds->lightmapVecs[0], w.points[1]);
					VectorMA(w.points[1], (float) y - LIGHTMAP_PIXELSHIFT, ds->lightmapVecs[1], w.points[1]);
					w.numpoints = 4;
					area = 0;
					for (j = 0; j < test->numFacets; j++)
					{
						memcpy(&tmpw, &w, sizeof(winding_t));
						area += VL_ChopWindingWithFacet(&tmpw, &test->facets[j]);
					}
				}
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				lightmappixelarea[k] = area;
			}
		}
	}
}

/*
=============
VL_FindAdjacentSurface
=============
*/
int VL_FindAdjacentSurface(int surfaceNum, int facetNum, vec3_t p1, vec3_t p2, int *sNum, int *fNum, int *point)
{
	int i, j, k;
	lsurfaceTest_t *test;
	lFacet_t *facet;
	dsurface_t *ds;
	float *fp1, *fp2;
	vec3_t dir;
	plane_t *facetplane;
	//	winding_t w;

	facetplane = &lsurfaceTest[surfaceNum]->facets[facetNum].plane;
	//	DebugNet_RemoveAllPolys();
	//	memcpy(w.points, lsurfaceTest[surfaceNum]->facets[facetNum].points,
	//			lsurfaceTest[surfaceNum]->facets[facetNum].numpoints * sizeof(vec3_t));
	//	w.numpoints = lsurfaceTest[surfaceNum]->facets[facetNum].numpoints;
	//	DebugNet_DrawWinding(&w, 2);
	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		if (i == surfaceNum)
			continue;
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		if (test->trisoup)// || test->patch)
			continue;
		ds = &drawSurfaces[i];
		if ( ds->lightmapNum < 0 )
			continue;
		//if this surface is not even near the edge
		VectorSubtract(p1, test->origin, dir);
		if (fabs(dir[0]) > test->radius ||
			fabs(dir[1]) > test->radius ||
			fabs(dir[1]) > test->radius)
		{
			VectorSubtract(p2, test->origin, dir);
			if (fabs(dir[0]) > test->radius ||
				fabs(dir[1]) > test->radius ||
				fabs(dir[1]) > test->radius)
			{
				continue;
			}
		}
		//
		for (j = 0; j < test->numFacets; j++)
		{
			facet = &test->facets[j];
			//
			//if (!Plane_Equal(&facet->plane, facetplane, qfalse))
			if (DotProduct(facet->plane.normal, facetplane->normal) < 0.9)
			{
				if (!test->trisoup && !test->patch)
					break;
				continue;
			}
			//
			for (k = 0; k < facet->numpoints; k++)
			{
				fp1 = facet->points[k];
				if (fabs(p2[0] - fp1[0]) < 0.1 &&
					fabs(p2[1] - fp1[1]) < 0.1 &&
					fabs(p2[2] - fp1[2]) < 0.1)
				{
					fp2 = facet->points[(k+1) % facet->numpoints];
					if (fabs(p1[0] - fp2[0]) < 0.1 &&
						fabs(p1[1] - fp2[1]) < 0.1 &&
						fabs(p1[2] - fp2[2]) < 0.1)
					{
						//	memcpy(w.points, facet->points, facet->numpoints * sizeof(vec3_t));
						//	w.numpoints = facet->numpoints;
						//	DebugNet_DrawWinding(&w, 1);
						*sNum = i;
						*fNum = j;
						*point = k;
						return qtrue;
					}
				}
				/*
				else if (fabs(p1[0] - fp1[0]) < 0.1 &&
					fabs(p1[1] - fp1[1]) < 0.1 &&
					fabs(p1[2] - fp1[2]) < 0.1)
				{
					fp2 = facet->points[(k+1) % facet->numpoints];
					if (fabs(p2[0] - fp2[0]) < 0.1 &&
						fabs(p2[1] - fp2[1]) < 0.1 &&
						fabs(p2[2] - fp2[2]) < 0.1)
					{
						//	memcpy(w.points, facet->points, facet->numpoints * sizeof(vec3_t));
						//	w.numpoints = facet->numpoints;
						//	DebugNet_DrawWinding(&w, 1);
						*sNum = i;
						*fNum = j;
						*point = k;
						return qtrue;
					}
				}
				//*/
			}
		}
	}
	return qfalse;
}

/*
=============
VL_SmoothenLightmapEdges

this code is used to smoothen lightmaps across surface edges
=============
*/
void VL_SmoothenLightmapEdges(void)
{
	int i, j, k, coords1[2][2];
	float coords2[2][2];
	int x1, y1, xinc1, yinc1, k1, k2;
	float x2, y2, xinc2, yinc2, length;
	int surfaceNum, facetNum, point;
	lsurfaceTest_t *test;
	lFacet_t *facet1, *facet2;
	dsurface_t *ds1, *ds2;
	float *p[2], s, t, *color1, *color2;
	vec3_t dir, cross;

	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		if (test->trisoup)// || test->patch)
			continue;
		ds1 = &drawSurfaces[i];
		if ( ds1->lightmapNum < 0 )
			continue;
		for (j = 0; j < test->numFacets; j++)
		{
			facet1 = &test->facets[j];
			//
			for (k = 0; k < facet1->numpoints; k++)
			{
				p[0] = facet1->points[k];
				p[1] = facet1->points[(k+1)%facet1->numpoints];
				//
				coords1[0][0] = facet1->lightmapCoords[k][0] * LIGHTMAP_SIZE;
				coords1[0][1] = facet1->lightmapCoords[k][1] * LIGHTMAP_SIZE;
				coords1[1][0] = facet1->lightmapCoords[(k+1)%facet1->numpoints][0] * LIGHTMAP_SIZE;
				coords1[1][1] = facet1->lightmapCoords[(k+1)%facet1->numpoints][1] * LIGHTMAP_SIZE;
				if (coords1[0][0] >= LIGHTMAP_SIZE)
					coords1[0][0] = LIGHTMAP_SIZE-1;
				if (coords1[0][1] >= LIGHTMAP_SIZE)
					coords1[0][1] = LIGHTMAP_SIZE-1;
				if (coords1[1][0] >= LIGHTMAP_SIZE)
					coords1[1][0] = LIGHTMAP_SIZE-1;
				if (coords1[1][1] >= LIGHTMAP_SIZE)
					coords1[1][1] = LIGHTMAP_SIZE-1;
				// try one row or column further because on flat faces the lightmap can
				// extend beyond the edge
				VectorSubtract(p[1], p[0], dir);
				VectorNormalize(dir, dir);
				CrossProduct(dir, facet1->plane.normal, cross);
				//
				if (coords1[0][0] - coords1[1][0] == 0)
				{
					s = DotProduct( cross, facet1->lightmapMatrix[0] );
					coords1[0][0] += s < 0 ? 1 : -1;
					coords1[1][0] += s < 0 ? 1 : -1;
					if (coords1[0][0] < ds1->lightmapX || coords1[0][0] >= ds1->lightmapX + ds1->lightmapWidth)
					{
						coords1[0][0] += s < 0 ? -1 : 1;
						coords1[1][0] += s < 0 ? -1 : 1;
					}
					length = fabs(coords1[1][1] - coords1[0][1]);
				}
				else if (coords1[0][1] - coords1[1][1] == 0)
				{
					t = DotProduct( cross, facet1->lightmapMatrix[1] );
					coords1[0][1] += t < 0 ? 1 : -1;
					coords1[1][1] += t < 0 ? 1 : -1;
					if (coords1[0][1] < ds1->lightmapY || coords1[0][1] >= ds1->lightmapY + ds1->lightmapHeight)
					{
						coords1[0][1] += t < 0 ? -1 : 1;
						coords1[1][1] += t < 0 ? -1 : 1;
					}
					length = fabs(coords1[1][0] - coords1[0][0]);
				}
				else
				{
					//the edge is not parallell to one of the lightmap axis
					continue;
				}
				//
				x1 = coords1[0][0];
				y1 = coords1[0][1];
				xinc1 = coords1[1][0] - coords1[0][0];
				if (xinc1 < 0) xinc1 = -1;
				if (xinc1 > 0) xinc1 = 1;
				yinc1 = coords1[1][1] - coords1[0][1];
				if (yinc1 < 0) yinc1 = -1;
				if (yinc1 > 0) yinc1 = 1;
				// the edge should be parallell to one of the lightmap axis
				if (xinc1 != 0 && yinc1 != 0)
					continue;
				//
				if (!VL_FindAdjacentSurface(i, j, p[0], p[1], &surfaceNum, &facetNum, &point))
					continue;
				//
				ds2 = &drawSurfaces[surfaceNum];
				facet2 = &lsurfaceTest[surfaceNum]->facets[facetNum];
				coords2[0][0] = facet2->lightmapCoords[(point+1)%facet2->numpoints][0] * LIGHTMAP_SIZE;
				coords2[0][1] = facet2->lightmapCoords[(point+1)%facet2->numpoints][1] * LIGHTMAP_SIZE;
				coords2[1][0] = facet2->lightmapCoords[point][0] * LIGHTMAP_SIZE;
				coords2[1][1] = facet2->lightmapCoords[point][1] * LIGHTMAP_SIZE;
				if (coords2[0][0] >= LIGHTMAP_SIZE)
					coords2[0][0] = LIGHTMAP_SIZE-1;
				if (coords2[0][1] >= LIGHTMAP_SIZE)
					coords2[0][1] = LIGHTMAP_SIZE-1;
				if (coords2[1][0] >= LIGHTMAP_SIZE)
					coords2[1][0] = LIGHTMAP_SIZE-1;
				if (coords2[1][1] >= LIGHTMAP_SIZE)
					coords2[1][1] = LIGHTMAP_SIZE-1;
				//
				x2 = coords2[0][0];
				y2 = coords2[0][1];
				xinc2 = coords2[1][0] - coords2[0][0];
				if (length)
					xinc2 = xinc2 / length;
				yinc2 = coords2[1][1] - coords2[0][1];
				if (length)
					yinc2 = yinc2 / length;
				// the edge should be parallell to one of the lightmap axis
				if ((int) xinc2 != 0 && (int) yinc2 != 0)
					continue;
				//
				while(1)
				{
					k1 = ( ds1->lightmapNum * LIGHTMAP_HEIGHT + y1) * LIGHTMAP_WIDTH + x1;
					k2 = ( ds2->lightmapNum * LIGHTMAP_HEIGHT + ((int) y2)) * LIGHTMAP_WIDTH + ((int) x2);
					color1 = lightFloats + k1*3;
					color2 = lightFloats + k2*3;
					if (lightmappixelarea[k1] < 0.01)
					{
						color1[0] = color2[0];
						color1[1] = color2[1];
						color1[2] = color2[2];
					}
					else
					{
						color1[0] = (float) color2[0] * 0.7 + (float) color1[0] * 0.3;
						color1[1] = (float) color2[1] * 0.7 + (float) color1[1] * 0.3;
						color1[2] = (float) color2[2] * 0.7 + (float) color1[2] * 0.3;
					}
					//
					if (x1 == coords1[1][0] &&
						y1 == coords1[1][1])
						break;
					x1 += xinc1;
					y1 += yinc1;
					x2 += xinc2;
					y2 += yinc2;
					if (x2 < ds2->lightmapX)
						x2 = ds2->lightmapX;
					if (x2 >= ds2->lightmapX + ds2->lightmapWidth)
						x2 = ds2->lightmapX + ds2->lightmapWidth-1;
					if (y2 < ds2->lightmapY)
						y2 = ds2->lightmapY;
					if (y2 >= ds2->lightmapY + ds2->lightmapHeight)
						y2 = ds2->lightmapY + ds2->lightmapHeight-1;
				}
			}
		}
	}
}

/*
=============
VL_FixLightmapEdges
=============
*/
void VL_FixLightmapEdges(void)
{
	int				i, j, x, y, k, foundvalue, height, width, index;
	int				pos, top, bottom;
	dsurface_t		*ds;
	lsurfaceTest_t	*test;
	float			color[3];
	float			*ptr;
	byte filled[(LIGHTMAP_SIZE+1) * (LIGHTMAP_SIZE+1) / 8];
	float lightmap_edge_epsilon;

	lightmap_edge_epsilon = 0.1 * samplesize;
	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		ds = &drawSurfaces[ i ];

		if ( ds->lightmapNum < 0 )
			continue;
		if (ds->surfaceType == MST_PATCH)
		{
			height = ds->lightmapHeight - 1;
			width = ds->lightmapWidth - 1;
		}
		else
		{
			height = ds->lightmapHeight;
			width = ds->lightmapWidth;
		}
		memset(filled, 0, sizeof(filled));
//		printf("\n");
		for (x = 0; x < width; x++)
		{
			for (y = 0; y < height; y++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (lightmappixelarea[k] > lightmap_edge_epsilon)
				{
					index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
					filled[index >> 3] |= 1 << (index & 7);
//					printf("*");
				}
//				else
//					printf("_");
			}
//			printf("\n");
		}
		for (y = 0; y < height; y++)
		{
			pos = -2;
			for (x = 0; x < width; x++)
			{
				index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (pos == -2)
				{
					if (filled[index >> 3] & (1 << (index & 7)))
						pos = -1;
				}
				else if (pos == -1)
				{
					if (!(filled[index >> 3] & (1 << (index & 7))))
						pos = x - 1;
				}
				else
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						bottom = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
							* LIGHTMAP_WIDTH + ds->lightmapX + pos;
						top = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
							* LIGHTMAP_WIDTH + ds->lightmapX + x;
						for (j = 0; j < (x - pos + 1) / 2; j++)
						{
							k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
								* LIGHTMAP_WIDTH + ds->lightmapX + pos + j + 1;
							index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + pos + j + 1;
							filled[index >> 3] |= 1 << (index & 7);
							(lightFloats + k*3)[0] = (lightFloats + top*3)[0];
							(lightFloats + k*3)[1] = (lightFloats + top*3)[1];
							(lightFloats + k*3)[2] = (lightFloats + top*3)[2];
							k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
								* LIGHTMAP_WIDTH + ds->lightmapX + x - j - 1;
							index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x - j - 1;
							filled[index >> 3] |= 1 << (index & 7);
							(lightFloats + k*3)[0] = (lightFloats + bottom*3)[0];
							(lightFloats + k*3)[1] = (lightFloats + bottom*3)[1];
							(lightFloats + k*3)[2] = (lightFloats + bottom*3)[2];
						}
						pos = -1;
					}
				}
			}
		}
		for (x = 0; x < width; x++)
		{
			pos = -2;
			for (y = 0; y < height; y++)
			{
				index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (pos == -2)
				{
					if (filled[index >> 3] & (1 << (index & 7)))
						pos = -1;
				}
				else if (pos == -1)
				{
					if (!(filled[index >> 3] & (1 << (index & 7))))
						pos = y - 1;
				}
				else
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						bottom = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + pos)
							* LIGHTMAP_WIDTH + ds->lightmapX + x;
						top = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
							* LIGHTMAP_WIDTH + ds->lightmapX + x;
						for (j = 0; j < (y - pos + 1) / 2; j++)
						{
							k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + pos + j + 1)
								* LIGHTMAP_WIDTH + ds->lightmapX + x;
							index = (ds->lightmapY + pos + j + 1) * LIGHTMAP_WIDTH + ds->lightmapX + x;
							filled[index >> 3] |= 1 << (index & 7);
							(lightFloats + k*3)[0] = (lightFloats + top*3)[0];
							(lightFloats + k*3)[1] = (lightFloats + top*3)[1];
							(lightFloats + k*3)[2] = (lightFloats + top*3)[2];
							k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y - j - 1)
								* LIGHTMAP_WIDTH + ds->lightmapX + x;
							index = (ds->lightmapY + y - j - 1) * LIGHTMAP_WIDTH + ds->lightmapX + x;
							filled[index >> 3] |= 1 << (index & 7);
							(lightFloats + k*3)[0] = (lightFloats + bottom*3)[0];
							(lightFloats + k*3)[1] = (lightFloats + bottom*3)[1];
							(lightFloats + k*3)[2] = (lightFloats + bottom*3)[2];
						}
						pos = -1;
					}
				}
			}
		}
		for (y = 0; y < height; y++)
		{
			foundvalue = qfalse;
			for (x = 0; x < width; x++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (foundvalue)
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
					}
					else
					{
						ptr = lightFloats + k*3;
						ptr[0] = color[0];
						ptr[1] = color[1];
						ptr[2] = color[2];
						filled[index >> 3] |= 1 << (index & 7);
					}
				}
				else
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
						foundvalue = qtrue;
					}
				}
			}
			foundvalue = qfalse;
			for (x = width-1; x >= 0; x--)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (foundvalue)
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
					}
					else
					{
						ptr = lightFloats + k*3;
						ptr[0] = color[0];
						ptr[1] = color[1];
						ptr[2] = color[2];
						filled[index >> 3] |= 1 << (index & 7);
					}
				}
				else
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
						foundvalue = qtrue;
					}
				}
			}
		}
		for (x = 0; x < width; x++)
		{
			foundvalue = qfalse;
			for (y = 0; y < height; y++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (foundvalue)
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
					}
					else
					{
						ptr = lightFloats + k*3;
						ptr[0] = color[0];
						ptr[1] = color[1];
						ptr[2] = color[2];
						filled[index >> 3] |= 1 << (index & 7);
					}
				}
				else
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
						foundvalue = qtrue;
					}
				}
			}
			foundvalue = qfalse;
			for (y = height-1; y >= 0; y--)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				index = (ds->lightmapY + y) * LIGHTMAP_WIDTH + ds->lightmapX + x;
				if (foundvalue)
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
					}
					else
					{
						ptr = lightFloats + k*3;
						ptr[0] = color[0];
						ptr[1] = color[1];
						ptr[2] = color[2];
						filled[index >> 3] |= 1 << (index & 7);
					}
				}
				else
				{
					if (filled[index >> 3] & (1 << (index & 7)))
					{
						ptr = lightFloats + k*3;
						color[0] = ptr[0];
						color[1] = ptr[1];
						color[2] = ptr[2];
						foundvalue = qtrue;
					}
				}
			}
		}
		if (ds->surfaceType == MST_PATCH)
		{
			x = ds->lightmapWidth-1;
			for (y = 0; y < ds->lightmapHeight; y++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				ptr = lightFloats + k*3;
				ptr[0] = (lightFloats + (k-1)*3)[0];
				ptr[1] = (lightFloats + (k-1)*3)[1];
				ptr[2] = (lightFloats + (k-1)*3)[2];
			}
			y = ds->lightmapHeight-1;
			for (x = 0; x < ds->lightmapWidth; x++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				ptr = lightFloats + k*3;
				ptr[0] = (lightFloats + (k-LIGHTMAP_WIDTH)*3)[0];
				ptr[1] = (lightFloats + (k-LIGHTMAP_WIDTH)*3)[1];
				ptr[2] = (lightFloats + (k-LIGHTMAP_WIDTH)*3)[2];
			}
		}
		/*
		//colored debug edges
		if (ds->surfaceType == MST_PATCH)
		{
			x = ds->lightmapWidth-1;
			for (y = 0; y < ds->lightmapHeight; y++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				ptr = lightFloats + k*3;
				ptr[0] = 255;
				ptr[1] = 0;
				ptr[2] = 0;
			}
			y = ds->lightmapHeight-1;
			for (x = 0; x < ds->lightmapWidth; x++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				ptr = lightFloats + k*3;
				ptr[0] = 0;
				ptr[1] = 255;
				ptr[2] = 0;
			}
		}
		//*/
	}
	//
	VL_SmoothenLightmapEdges();
}

/*
=============
VL_ShiftPatchLightmaps
=============
*/
void VL_ShiftPatchLightmaps(void)
{
	int				i, j, x, y, k;
	drawVert_t		*verts;
	dsurface_t		*ds;
	lsurfaceTest_t	*test;
	float			*ptr;

	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		ds = &drawSurfaces[ i ];

		if ( ds->lightmapNum < 0 )
			continue;
		if (ds->surfaceType != MST_PATCH)
			continue;
		for (x = ds->lightmapWidth; x > 0; x--)
		{
			for (y = 0; y <= ds->lightmapHeight; y++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				ptr = lightFloats + k*3;
				ptr[0] = (lightFloats + (k-1)*3)[0];
				ptr[1] = (lightFloats + (k-1)*3)[1];
				ptr[2] = (lightFloats + (k-1)*3)[2];
			}
		}
		for (y = ds->lightmapHeight; y > 0; y--)
		{
			for (x = 0; x <= ds->lightmapWidth; x++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				ptr = lightFloats + k*3;
				ptr[0] = (lightFloats + (k-LIGHTMAP_WIDTH)*3)[0];
				ptr[1] = (lightFloats + (k-LIGHTMAP_WIDTH)*3)[1];
				ptr[2] = (lightFloats + (k-LIGHTMAP_WIDTH)*3)[2];
			}
		}
		verts = &drawVerts[ ds->firstVert ];
		for ( j = 0 ; j < ds->patchHeight * ds->patchWidth; j++ )
		{
			verts[j].lightmap[0] += 0.5 / LIGHTMAP_WIDTH;
			verts[j].lightmap[1] += 0.5 / LIGHTMAP_HEIGHT;
		}
		ds->lightmapHeight++;
		ds->lightmapWidth++;
	}
}

/*
=============
VL_StoreLightmap
=============
*/
void VL_StoreLightmap(void)
{
	int				i, x, y, k;
	dsurface_t		*ds;
	lsurfaceTest_t	*test;
	float			*src;
	byte			*dst;

	_printf("storing lightmaps...\n");
	//fix lightmap edges before storing them
	VL_FixLightmapEdges();
	//
#ifdef LIGHTMAP_PATCHSHIFT
	VL_ShiftPatchLightmaps();
#endif
	//
	for ( i = 0 ; i < numDrawSurfaces ; i++ )
	{
		test = lsurfaceTest[ i ];
		if (!test)
			continue;
		ds = &drawSurfaces[ i ];

		if ( ds->lightmapNum < 0 )
			continue;

		for (y = 0; y < ds->lightmapHeight; y++)
		{
			for (x = 0; x < ds->lightmapWidth; x++)
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y)
						* LIGHTMAP_WIDTH + ds->lightmapX + x;
				VectorAdd((lightFloats + k*3), lightAmbientColor, (lightFloats + k*3));
				src = &lightFloats[k*3];
				dst = lightBytes + k*3;
				ColorToBytes(src, dst);
			}
		}
	}
}

/*
=============
PointInLeafnum
=============
*/
int	PointInLeafnum(vec3_t point)
{
	int		nodenum;
	vec_t	dist;
	dnode_t	*node;
	dplane_t	*plane;

	nodenum = 0;
	while (nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];
		dist = DotProduct (point, plane->normal) - plane->dist;
		if (dist > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	}

	return -nodenum - 1;
}

/*
=============
VL_PointInLeafnum_r
=============
*/
int	VL_PointInLeafnum_r(vec3_t point, int nodenum)
{
	int leafnum;
	vec_t	dist;
	dnode_t	*node;
	dplane_t	*plane;

	while (nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];
		dist = DotProduct (point, plane->normal) - plane->dist;
		if (dist > 0.1)
		{
			nodenum = node->children[0];
		}
		else if (dist < -0.1)
		{
			nodenum = node->children[1];
		}
		else
		{
			leafnum = VL_PointInLeafnum_r(point, node->children[0]);
			if (dleafs[leafnum].cluster != -1)
				return leafnum;
			nodenum = node->children[1];
		}
	}

	leafnum = -nodenum - 1;
	return leafnum;
}

/*
=============
VL_PointInLeafnum
=============
*/
int	VL_PointInLeafnum(vec3_t point)
{
	return VL_PointInLeafnum_r(point, 0);
}

/*
=============
VL_LightLeafnum
=============
*/
int VL_LightLeafnum(vec3_t point)
{
	/*
	int leafnum;
	dleaf_t *leaf;
	float x, y, z;
	vec3_t test;

	leafnum = VL_PointInLeafnum(point);
	leaf = &dleafs[leafnum];
	if (leaf->cluster != -1)
		return leafnum;
	for (z = 1; z >= -1; z -= 1)
	{
		for (x = 1; x >= -1; x -= 1)
		{
			for (y = 1; y >= -1; y -= 1)
			{
				VectorCopy(point, test);
				test[0] += x;
				test[1] += y;
				test[2] += z;
				leafnum = VL_PointInLeafnum(test);
				leaf = &dleafs[leafnum];
				if (leaf->cluster != -1)
				{
					VectorCopy(test, point);
					return leafnum;
				}
			}
		}
	}
	return leafnum;
	*/
	return VL_PointInLeafnum(point);
}

//#define LIGHTPOLYS

#ifdef LIGHTPOLYS

winding_t *lightwindings[MAX_MAP_DRAW_SURFS];
int numlightwindings;

/*
=============
VL_DrawLightWindings
=============
*/
void VL_DrawLightWindings(void)
{
	int i;
	for (i = 0; i < numlightwindings; i++)
	{
#ifdef DEBUGNET
		DebugNet_DrawWinding(lightwindings[i], 1);
#endif
	}
}

/*
=============
VL_LightSurfaceWithVolume
=============
*/
void VL_LightSurfaceWithVolume(int surfaceNum, int facetNum, vlight_t *light, lightvolume_t *volume)
{
	winding_t *w;
	lsurfaceTest_t *test;
	lFacet_t *facet;
	int i;

	test = lsurfaceTest[ surfaceNum ];
	facet = &test->facets[ facetNum ];

	//
	w = (winding_t *) malloc(sizeof(winding_t));
	memcpy(w->points, facet->points, sizeof(vec3_t) * facet->numpoints);
	w->numpoints = facet->numpoints;

	for (i = 0; i < volume->numplanes; i++)
	{
		//if totally on the back
		if (VL_ChopWinding(w, &volume->planes[i], 0.01) == SIDE_BACK)
			return;
	}
	lightwindings[numlightwindings] = w;
	numlightwindings++;
	if (numlightwindings >= MAX_MAP_DRAW_SURFS)
		Error("MAX_LIGHTWINDINGS");
}

#else

/*
=============
VL_LightSurfaceWithVolume
=============
*/
/*
int VL_PointInsideLightVolume(vec3_t point, lightvolume_t *volume)
{
	int i;
	float d;

	for (i = 0; i < volume->numplanes; i++)
	{
		d = DotProduct(volume->planes[i].normal, point) - volume->planes[i].dist;
		if (d < 0) return qfalse;
	}
	return qtrue;
}

void VL_LightSurfaceWithVolume( int surfaceNum, int facetNum, vlight_t *light, lightvolume_t *volume )
{
	dsurface_t	*ds;
	int			i, j, k;
	int			numPositions;
	vec3_t		base, normal, color;
	int			sampleWidth, sampleHeight;
	vec3_t		lightmapOrigin, lightmapVecs[2], dir;
	unsigned char *ptr;
	float add, dist, angle;
	mesh_t * mesh;

	ds = &drawSurfaces[surfaceNum];

	// vertex-lit triangle model
	if ( ds->surfaceType == MST_TRIANGLE_SOUP ) {
		return;
	}
	
	if ( ds->lightmapNum < 0 ) {
		return;		// doesn't need lighting
	}

	if ( ds->surfaceType == MST_PATCH ) {
		mesh = lsurfaceTest[surfaceNum]->detailMesh;
	} else {
		VectorCopy( ds->lightmapVecs[2], normal );

		VectorCopy( ds->lightmapOrigin, lightmapOrigin );
		VectorCopy( ds->lightmapVecs[0], lightmapVecs[0] );
		VectorCopy( ds->lightmapVecs[1], lightmapVecs[1] );
	}

	sampleWidth = ds->lightmapWidth;
	sampleHeight = ds->lightmapHeight;

	//calculate lightmap
	for ( i = 0 ; i < sampleWidth; i++ ) {
		for ( j = 0 ; j < sampleHeight; j++ ) {

			if ( ds->patchWidth ) {
				numPositions = 9;
				VectorCopy( mesh->verts[j*mesh->width+i].normal, normal );
				// VectorNormalize( normal, normal );
				// push off of the curve a bit
				VectorMA( mesh->verts[j*mesh->width+i].xyz, 1, normal, base );

//				MakeNormalVectors( normal, lightmapVecs[0], lightmapVecs[1] );
			} else {
				numPositions = 9;
				for ( k = 0 ; k < 3 ; k++ ) {
					base[k] = lightmapOrigin[k] + normal[k]
								+ ((float) i) * lightmapVecs[0][k]
								+ ((float) j) * lightmapVecs[1][k];
				}
			}
			VectorAdd( base, surfaceOrigin[ surfaceNum ], base );

			VectorSubtract(base, light->origin, dir);
			dist = VectorNormalize(dir, dir);
			if ( dist < 16 ) {
				dist = 16;
			}
			angle = 1;//DotProduct( normal, dir ); //1;
			if (angle > 1)
				angle = 1;
			if ( light->atten_disttype == LDAT_LINEAR ) {
				add = angle * light->photons * lightLinearScale - dist;
				if ( add < 0 ) {
					add = 0;
				}
			} else {
				add = light->photons / ( dist * dist ) * angle;
			}
			if (add <= 1.0)
				continue;

			if (VL_PointInsideLightVolume(base, volume))
			{
				k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + j) 
					* LIGHTMAP_WIDTH + ds->lightmapX + i;
				ptr = lightBytes + k*3;
				color[0] = (float) ptr[0] + add * light->color[0];
				color[1] = (float) ptr[1] + add * light->color[1];
				color[2] = (float) ptr[2] + add * light->color[2];
				ColorToBytes(color, ptr);
			}
		}
	}
}
*/

/*
=============
VL_GetFilter

FIXME:  don't use a lightmap pixel origin but use the four corner points
		to map part of a translucent surface onto the lightmap pixel
=============
*/
void VL_GetFilter(vlight_t *light, lightvolume_t *volume, vec3_t lmp, vec3_t filter)
{
	lFacet_t *facet;
	lsurfaceTest_t *test;
	float d, d1, d2, frac, s, t, ns;
	int i, j, is, it, b;
	int x, y, u, v, numsamples, radius, color[4], largest;
	byte *image;
	vec3_t point, origin, total;

	VectorSet(filter, 1, 1, 1);

	if (noalphashading)
		return;

	if (volume->numtransFacets <= 0)
		return;

	if (light->type == LIGHT_SURFACEDIRECTED)
	{
		// project the light map pixel origin onto the area light source plane
		d = DotProduct(lmp, light->normal) - DotProduct(light->normal, light->w.points[0]);
		VectorMA(lmp, -d, light->normal, origin);
	}
	else
	{
		VectorCopy(light->origin, origin);
	}
	for (i = 0; i < volume->numtransFacets; i++)
	{
		test = lsurfaceTest[ volume->transSurfaces[i] ];
		facet = &test->facets[ volume->transFacets[i] ];
		// if this surface does not cast an alpha shadow
		if ( !(test->shader->surfaceFlags & SURF_ALPHASHADOW) )
			continue;
		// if there are no texture pixel available
		if ( !test->shader->pixels ) {
			continue;
		}
		//
		d1 = DotProduct( origin, facet->plane.normal) - facet->plane.dist;
		d2 = DotProduct( lmp, facet->plane.normal ) - facet->plane.dist;
		// this should never happen because the light volume went through the facet
		if ( ( d1 < 0 ) == ( d2 < 0 ) ) {
			continue;
		}
		// calculate the crossing point
		frac = d1 / ( d1 - d2 );

		for ( j = 0 ; j < 3 ; j++ ) {
			point[j] = origin[j] + frac * ( lmp[j] - origin[j] );
		}

		s = DotProduct( point, facet->textureMatrix[0] ) + facet->textureMatrix[0][3];
		t = DotProduct( point, facet->textureMatrix[1] ) + facet->textureMatrix[1][3];
		if (s < 0)
			s = 0;
		if (t < 0)
			t = 0;

		s = s - floor( s );
		t = t - floor( t );

		is = s * test->shader->width;
		it = t * test->shader->height;

		//if old style alpha shading
		if (nocolorshading) {
			image = test->shader->pixels + 4 * ( it * test->shader->width + is );

			// alpha filter
			b = image[3];

			// alpha test makes this a binary option
			b = b < 128 ? 0 : 255;

			filter[0] = filter[0] * (255-b) / 255;
			filter[1] = filter[1] * (255-b) / 255;
			filter[2] = filter[2] * (255-b) / 255;
		}
		else {
			VectorClear(total);
			numsamples = 0;
			radius = 2;
			for ( u = -radius; u <= radius; u++ )
			{
				x = is + u;
				if ( x < 0 || x >= test->shader->width)
					continue;
				for ( v = -radius; v <= radius; v++ )
				{
					y = it + v;
					if ( y < 0 || y >= test->shader->height)
						continue;

					image = test->shader->pixels + 4 * ( y * test->shader->width + x );
					color[0] = image[0];
					color[1] = image[1];
					color[2] = image[2];
					largest = 0;
					for (j = 0; j < 3; j++)
						if (image[j] > largest)
							largest = image[j];
					if (largest <= 0 || image[3] == 0) {
						color[0] = 255;
						color[1] = 255;
						color[2] = 255;
						largest = 255;
					}
					total[0] += ((float) color[0]/largest) * (255-image[3]) / 255.0;
					total[1] += ((float) color[1]/largest) * (255-image[3]) / 255.0;
					total[2] += ((float) color[2]/largest) * (255-image[3]) / 255.0;
					numsamples++;
				}
			}
			ns = numsamples;
			//
			filter[0] *= total[0] / ns;
			filter[1] *= total[1] / ns;
			filter[2] *= total[2] / ns;
		}
	}
}

/*
=============
VL_LightSurfaceWithVolume
=============
*/
void VL_LightSurfaceWithVolume( int surfaceNum, int facetNum, vlight_t *light, lightvolume_t *volume )
{
	int i;
	dsurface_t	*ds;
	lFacet_t *facet;
	lsurfaceTest_t *test;
	winding_t w;
	vec3_t base, dir, delta, normal, filter, origin;
	int min_x[LIGHTMAP_SIZE+2], max_x[LIGHTMAP_SIZE+2];
	int min_y, max_y, k, x, y, n;
	float *color, distscale;
	float d, add, angle, dist, area, insidearea, coords[MAX_POINTS_ON_WINDING+1][2];
	mesh_t *mesh;
	byte polygonedges[(LIGHTMAP_SIZE+1) * (LIGHTMAP_SIZE+1) / 8];


	ds = &drawSurfaces[surfaceNum];

	// vertex-lit triangle model
	if ( ds->surfaceType == MST_TRIANGLE_SOUP ) {
		return;
	}
	
	if ( ds->lightmapNum < 0 ) {
		return;		// doesn't need lighting
	}

	test = lsurfaceTest[ surfaceNum ];
	facet = &test->facets[ facetNum ];

	if (defaulttracelight && !test->always_vlight)
		return;
	if (test->always_tracelight)
		return;

	memcpy(w.points, facet->points, sizeof(vec3_t) * facet->numpoints);
	w.numpoints = facet->numpoints;

	for (i = 0; i < volume->numplanes; i++)
	{
		//if totally on the back
		if (VL_ChopWinding(&w, &volume->planes[i], 0.01) == SIDE_BACK)
			return;
	}

	// only one thread at a time may write to the lightmap of this surface
	MutexLock(test->mutex);

	test->numvolumes++;

	if (ds->surfaceType == MST_PATCH)
	{
		// FIXME: reduce size and don't mark all as edge
		min_y = ds->lightmapY + facet->y;
		max_y = ds->lightmapY + facet->y + facet->height - 1;
		for (y = min_y; y <= max_y; y++)
		{
			min_x[y] = ds->lightmapX + facet->x;
			max_x[y] = ds->lightmapX + facet->x + facet->width - 1;
			for (x = min_x[y]; x <= max_x[y]; x++)
			{
				n = y * LIGHTMAP_SIZE + x;
				polygonedges[n >> 3] |= 1 << (n & 7);
			}
		}
	}
	else
	{
		for (i = 0; i < w.numpoints; i++)
		{
			float	s, t;

			if (i >= MAX_POINTS_ON_WINDING)
				_printf("coords overflow\n");
			if (ds->surfaceType != MST_PATCH)
			{
				VectorSubtract(w.points[i], facet->mins, delta);
				s = DotProduct( delta, facet->lightmapMatrix[0] ) + ds->lightmapX + 0.5;
				t = DotProduct( delta, facet->lightmapMatrix[1] ) + ds->lightmapY + 0.5;
				if (s >= LIGHTMAP_SIZE)
					s = LIGHTMAP_SIZE - 0.5;
				if (s < 0)
					s = 0;
				if (t >= LIGHTMAP_SIZE)
					t = LIGHTMAP_SIZE - 0.5;
				if (t < 0)
					t = 0;
				coords[i][0] = s;
				coords[i][1] = t;
			}
			else
			{
				s = DotProduct( w.points[i], facet->lightmapMatrix[0] ) + facet->lightmapMatrix[0][3];
				t = DotProduct( w.points[i], facet->lightmapMatrix[1] ) + facet->lightmapMatrix[1][3];

				s = s - floor( s );
				t = t - floor( t );

				coords[i][0] = ds->lightmapX + s * LIGHTMAP_SIZE;// + 0.5;
				coords[i][1] = ds->lightmapY + t * LIGHTMAP_SIZE;// + 0.5;

				if (coords[i][0] >= LIGHTMAP_SIZE)
					coords[i][0] -= LIGHTMAP_SIZE;
				if (coords[i][1] >= LIGHTMAP_SIZE)
					coords[i][1] -= LIGHTMAP_SIZE;
				if (coords[i][0] < ds->lightmapX)
					coords[i][0] = ds->lightmapX;
				if (coords[i][1] < ds->lightmapY)
					coords[i][1] = ds->lightmapY;
			}
			x = coords[i][0];
			y = coords[i][1];
			if (x < ds->lightmapX || x >= LIGHTMAP_SIZE)
				_printf("VL_LightSurfaceWithVolume: x outside lightmap\n");
			if (y < ds->lightmapY || y >= LIGHTMAP_SIZE)
				_printf("VL_LightSurfaceWithVolume: y outside lightmap\n");
		}
		coords[i][0] = coords[0][0];
		coords[i][1] = coords[0][1];

		//
		min_y = LIGHTMAP_SIZE;
		max_y = 0;
		for (i = 0; i < LIGHTMAP_SIZE; i++)
		{
			min_x[i] = LIGHTMAP_SIZE;
			max_x[i] = 0;
		}
		memset(polygonedges, 0, sizeof(polygonedges));
		// scan convert the polygon onto the lightmap
		// for each edge it marks *every* lightmap pixel the edge goes through
		// so no brasenham and no scan conversion used for texture mapping but
		// more something like ray casting
		// this is necesary because we need all lightmap pixels totally or partly
		// inside the light volume. these lightmap pixels are only lit for the part
		// that they are inside the light volume.
		for (i = 0; i < w.numpoints; i++)
		{
			float xf, yf, dx, dy, xstep, ystep, xfrac, yfrac;
			int xinc, yinc;

			xf = coords[i][0];
			yf = coords[i][1];
			dx = coords[i+1][0] - xf;
			dy = coords[i+1][1] - yf;
			//
			x = (int) xf;
			y = (int) yf;
			//
			if (y < min_y)
				min_y = y;
			if (y > max_y)
				max_y = y;
			//
			if (fabs(dx) > fabs(dy))
			{
				if (dx > 0)
				{
					// y fraction at integer x below fractional x
					yfrac = yf + (floor(xf) - xf) * dy / dx;
					xinc = 1;
				}
				else if (dx < 0)
				{
					// y fraction at integer x above fractional x
					yfrac = yf + (floor(xf) + 1 - xf) * dy / dx;
					xinc = -1;
				}
				else
				{
					yfrac = yf;
					xinc = 0;
				}
				// step in y direction per 1 unit in x direction
				if (dx)
					ystep = dy / fabs(dx);
				else
					ystep = 0;
				while(1)
				{
					if (x < ds->lightmapX || x >= LIGHTMAP_SIZE)
						_printf("VL_LightSurfaceWithVolume: x outside lightmap\n");
					if (y < ds->lightmapY || y >= LIGHTMAP_SIZE)
						_printf("VL_LightSurfaceWithVolume: y outside lightmap\n");
					//
					n = y * LIGHTMAP_SIZE + x;
					polygonedges[n >> 3] |= 1 << (n & 7);
					if (x < min_x[y])
						min_x[y] = x;
					if (x > max_x[y])
						max_x[y] = x;
					if (x == (int) coords[i+1][0])
						break;
					yfrac += ystep;
					if (dy > 0)
					{
						if (yfrac > (float) y + 1)
						{
							y += 1;
							//
							n = y * LIGHTMAP_SIZE + x;
							polygonedges[n >> 3] |= 1 << (n & 7);
							if (x < min_x[y])
								min_x[y] = x;
							if (x > max_x[y])
								max_x[y] = x;
						}
					}
					else
					{
						if (yfrac < (float) y)
						{
							y -= 1;
							//
							n = y * LIGHTMAP_SIZE + x;
							polygonedges[n >> 3] |= 1 << (n & 7);
							if (x < min_x[y])
								min_x[y] = x;
							if (x > max_x[y])
								max_x[y] = x;
						}
					}
					x += xinc;
				}
			}
			else
			{
				if (dy > 0)
				{
					//x fraction at integer y below fractional y
					xfrac = xf + (floor(yf) - yf) * dx / dy;
					yinc = 1;
				}
				else if (dy < 0)
				{
					//x fraction at integer y above fractional y
					xfrac = xf + (floor(yf) + 1 - yf) * dx / dy;
					yinc = -1;
				}
				else
				{
					xfrac = xf;
					yinc = 0;
				}
				// step in x direction per 1 unit in y direction
				if (dy)
					xstep = dx / fabs(dy);
				else
					xstep = 0;
				while(1)
				{
					if (x < ds->lightmapX || x >= LIGHTMAP_SIZE)
						_printf("VL_LightSurfaceWithVolume: x outside lightmap\n");
					if (y < ds->lightmapY || y >= LIGHTMAP_SIZE)
						_printf("VL_LightSurfaceWithVolume: y outside lightmap\n");
					//
					n = y * LIGHTMAP_SIZE + x;
					polygonedges[n >> 3] |= 1 << (n & 7);
					if (x < min_x[y])
						min_x[y] = x;
					if (x > max_x[y])
						max_x[y] = x;
					if (y == (int) coords[i+1][1])
						break;
					xfrac += xstep;
					if (dx > 0)
					{
						if (xfrac > (float) x + 1)
						{
							x += 1;
							//
							n = y * LIGHTMAP_SIZE + x;
							polygonedges[n >> 3] |= 1 << (n & 7);
							if (x < min_x[y])
								min_x[y] = x;
							if (x > max_x[y])
								max_x[y] = x;
						}
					}
					else
					{
						if (xfrac < (float) x)
						{
							x -= 1;
							//
							n = y * LIGHTMAP_SIZE + x;
							polygonedges[n >> 3] |= 1 << (n & 7);
							if (x < min_x[y])
								min_x[y] = x;
							if (x > max_x[y])
								max_x[y] = x;
						}
					}
					y += yinc;
				}
			}
		}
	}
	// map light onto the lightmap
	for (y = min_y; y <= max_y; y++)
	{
		for (x = min_x[y]; x <= max_x[y]; x++)
		{
			if (ds->surfaceType == MST_PATCH)
			{
				mesh = test->detailMesh;
				VectorCopy( mesh->verts[(y-ds->lightmapY)*mesh->width+x-ds->lightmapX].xyz, base);
				VectorCopy( mesh->verts[(y-ds->lightmapY)*mesh->width+x-ds->lightmapX].normal, normal);
				//VectorCopy(facet->plane.normal, normal);
			}
			else
			{
				VectorMA(ds->lightmapOrigin, (float) x - ds->lightmapX, ds->lightmapVecs[0], base);
				VectorMA(base, (float) y - ds->lightmapY, ds->lightmapVecs[1], base);
				VectorCopy(facet->plane.normal, normal);
			}
			if (light->type == LIGHT_POINTSPOT)
			{
				float	distByNormal;
				vec3_t	pointAtDist;
				float	radiusAtDist;
				float	sampleRadius;
				vec3_t	distToSample;
				float	coneScale;

				VectorSubtract( light->origin, base, dir );

				distByNormal = -DotProduct( dir, light->normal );
				if ( distByNormal < 0 ) {
					continue;
				}
				VectorMA( light->origin, distByNormal, light->normal, pointAtDist );
				radiusAtDist = light->radiusByDist * distByNormal;

				VectorSubtract( base, pointAtDist, distToSample );
				sampleRadius = VectorLength( distToSample );

				if ( sampleRadius >= radiusAtDist ) {
					continue;		// outside the cone
				}
				if ( sampleRadius <= radiusAtDist - 32 ) {
					coneScale = 1.0;	// fully inside
				} else {
					coneScale = ( radiusAtDist - sampleRadius ) / 32.0;
				}
				
				dist = VectorNormalize( dir, dir );
				// clamp the distance to prevent super hot spots
				if ( dist < 16 ) {
					dist = 16;
				}
				angle = DotProduct( normal, dir );
				if (angle > 1)
					angle = 1;
				if (angle > 0) {
					if ( light->atten_angletype == LAAT_QUADRATIC ) {
						angle = 1 - angle;
						angle *= angle;
						angle = 1 - angle;
					}
					else if ( light->atten_angletype == LAAT_DOUBLEQUADRATIC ) {
						angle = 1 - angle;
						angle *= angle * angle;
						angle = 1 - angle;
					}
				}
				if (light->atten_anglescale > 0) {
					angle /= light->atten_anglescale;
					if (angle > 1)
						angle = 1;
				}
				if (light->atten_distscale > 0) {
					distscale = light->atten_distscale;
				}
				else {
					distscale = 1;
				}
				//
				if ( light->atten_disttype == LDAT_NOSCALE ) {
					add = angle * coneScale;
				}
				else if ( light->atten_disttype == LDAT_LINEAR ) {
					add = angle * light->photons * lightLinearScale * coneScale - dist * distscale;
					if ( add < 0 ) {
						add = 0;
					}
				}
				else {
					add = light->photons / ( dist * dist * distscale) * angle * coneScale;
				}
				if (add <= 1.0)
					continue;
			}
			else if (light->type == LIGHT_POINTFAKESURFACE)
			{
				// calculate the contribution
				add = PointToPolygonFormFactor( base, normal, &light->w );
				if ( add <= 0 ) {
					if ( light->twosided ) {
						add = -add;
					} else {
						continue;
					}
				}
			}
			else if (light->type == LIGHT_SURFACEDIRECTED)
			{
				//VectorCopy(light->normal, dir);
				//VectorInverse(dir);
				// project the light map pixel origin onto the area light source plane
				d = DotProduct(base, light->normal) - DotProduct(light->normal, light->w.points[0]);
				VectorMA(base, -d, light->normal, origin);
				VectorSubtract(origin, base, dir);
				dist = VectorNormalize(dir, dir);
				if ( dist < 16 ) {
					dist = 16;
				}
				//
				angle = DotProduct( normal, dir );
				if (angle > 1)
					angle = 1;
				if (angle > 0) {
					if ( light->atten_angletype == LAAT_QUADRATIC ) {
						angle = 1 - angle;
						angle *= angle;
						angle = 1 - angle;
					}
					else if ( light->atten_angletype == LAAT_DOUBLEQUADRATIC ) {
						angle = 1 - angle;
						angle *= angle * angle;
						angle = 1 - angle;
					}
				}
				if (light->atten_anglescale > 0) {
					angle /= light->atten_anglescale;
					if (angle > 1)
						angle = 1;
				}
				if (light->atten_distscale > 0) {
					distscale = light->atten_distscale;
				}
				else {
					distscale = 1;
				}
				if ( light->atten_disttype == LDAT_NOSCALE ) {
					add = angle;
				}
				else if ( light->atten_disttype == LDAT_LINEAR ) {
					add = angle * light->photons * lightLinearScale - dist * distscale;
					if ( add < 0 ) {
						add = 0;
					}
				} else { //default quadratic
					add = light->photons / ( dist * dist * distscale) * angle;
				}
				if (add <= 0)
					continue;
			}
			else //normal radial point light
			{
				VectorSubtract(light->origin, base, dir);
				dist = VectorNormalize(dir, dir);
				if ( dist < 16 ) {
					dist = 16;
				}
				angle = DotProduct( normal, dir );
				if (angle > 1)
					angle = 1;
				if (angle > 0) {
					if ( light->atten_angletype == LAAT_QUADRATIC ) {
						angle = 1 - angle;
						angle *= angle;
						angle = 1 - angle;
					}
					else if ( light->atten_angletype == LAAT_DOUBLEQUADRATIC ) {
						angle = 1 - angle;
						angle *= angle * angle;
						angle = 1 - angle;
					}
				}
				if (light->atten_anglescale > 0) {
					angle /= light->atten_anglescale;
					if (angle > 1)
						angle = 1;
				}
				if (light->atten_distscale > 0) {
					distscale = light->atten_distscale;
				}
				else {
					distscale = 1;
				}
				if ( light->atten_disttype == LDAT_NOSCALE ) {
					add = angle;
				}
				else if ( light->atten_disttype == LDAT_LINEAR ) {
					add = angle * light->photons * lightLinearScale - dist * distscale;
					if ( add < 0 ) {
						add = 0;
					}
				} else {
					add = light->photons / ( dist * dist * distscale) * angle;
				}
				if (add <= 1.0)
					continue;
			}
			//
			k = (ds->lightmapNum * LIGHTMAP_HEIGHT + y) * LIGHTMAP_WIDTH + x;
			//if on one of the edges
			n = y * LIGHTMAP_SIZE + x;
			if ((polygonedges[n >> 3] & (1 << (n & 7)) ))
			{
				// multiply 'add' by the relative area being lit of the total visible lightmap pixel area
				//
				// first create a winding for the lightmap pixel
				if (ds->surfaceType == MST_PATCH)
				{
					mesh = test->detailMesh;
					if (y-ds->lightmapY >= mesh->height-1)
						_printf("y outside mesh\n");
					if (x-ds->lightmapX >= mesh->width-1)
						_printf("x outside mesh\n");
					VectorCopy( mesh->verts[(y-ds->lightmapY)*mesh->width+x-ds->lightmapX].xyz, w.points[0]);
					VectorCopy( mesh->verts[(y+1-ds->lightmapY)*mesh->width+x-ds->lightmapX].xyz, w.points[1]);
					VectorCopy( mesh->verts[(y+1-ds->lightmapY)*mesh->width+x+1-ds->lightmapX].xyz, w.points[2]);
					VectorCopy( mesh->verts[(y-ds->lightmapY)*mesh->width+x+1-ds->lightmapX].xyz, w.points[3]);
					w.numpoints = 4;
				}
				else
				{
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT - ds->lightmapX, ds->lightmapVecs[0], w.points[0]);
					VectorMA(w.points[0], (float) y - LIGHTMAP_PIXELSHIFT - ds->lightmapY, ds->lightmapVecs[1], w.points[0]);
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT - ds->lightmapX, ds->lightmapVecs[0], w.points[1]);
					VectorMA(w.points[1], (float) y - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapY, ds->lightmapVecs[1], w.points[1]);
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapX, ds->lightmapVecs[0], w.points[2]);
					VectorMA(w.points[2], (float) y - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapY, ds->lightmapVecs[1], w.points[2]);
					VectorMA(ds->lightmapOrigin, (float) x - LIGHTMAP_PIXELSHIFT + 1 - ds->lightmapX, ds->lightmapVecs[0], w.points[3]);
					VectorMA(w.points[3], (float) y - LIGHTMAP_PIXELSHIFT - ds->lightmapY, ds->lightmapVecs[1], w.points[3]);
					w.numpoints = 4;
				}
				//
				// take the visible area of the lightmap pixel into account
				//
				//area = WindingArea(&w);
				area = lightmappixelarea[k];
				if (area <= 0)
					continue;
				// chop the lightmap pixel winding with the light volume
				for (i = 0; i < volume->numplanes; i++)
				{
					//if totally on the back
					if (VL_ChopWinding(&w, &volume->planes[i], 0) == SIDE_BACK)
						break;
				}
				// if the lightmap pixel is partly inside the light volume
				if (i >= volume->numplanes)
				{
					insidearea = WindingArea(&w);
					if (insidearea <= 0)
						i = 0;
					add = add * insidearea / area;
				}
				else
				{
					//DebugNet_DrawWinding(&w, 2);
					continue;	// this shouldn't happen
				}
			}
			// get the light filter from all the translucent surfaces the light volume went through
			VL_GetFilter(light, volume, base, filter);
			//
			color = &lightFloats[k*3];
			color[0] += add * light->color[0] * filter[0];
			color[1] += add * light->color[1] * filter[1];
			color[2] += add * light->color[2] * filter[2];
		}
	}

	MutexUnlock(test->mutex);
}

#endif

/*
=============
VL_SplitLightVolume
=============
*/
int VL_SplitLightVolume(lightvolume_t *volume, lightvolume_t *back, plane_t *split, float epsilon)
{
	lightvolume_t f, b;
	vec_t	dists[128];
	int		sides[128];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i = 0; i < volume->numplanes; i++)
	{
		dot = DotProduct (volume->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if (!counts[1])
		return 0;		// completely on front side
	
	if (!counts[0])
		return 1;		// completely on back side

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	f.numplanes = 0;
	b.numplanes = 0;

	for (i = 0; i < volume->numplanes; i++)
	{
		p1 = volume->points[i];

		if (f.numplanes >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitLightVolume -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return 0;		// can't chop -- fall back to original
		}
		if (b.numplanes >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitLightVolume -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return 0;		// can't chop -- fall back to original
		}

		if (sides[i] == SIDE_ON)
		{
			VectorCopy(p1, f.points[f.numplanes]);
			VectorCopy(p1, b.points[b.numplanes]);
			if (sides[i+1] == SIDE_BACK)
			{
				f.planes[f.numplanes] = *split;
				b.planes[b.numplanes] = volume->planes[i];
			}
			else if (sides[i+1] == SIDE_FRONT)
			{
				f.planes[f.numplanes] = volume->planes[i];
				b.planes[b.numplanes] = *split;
				VectorInverse(b.planes[b.numplanes].normal);
				b.planes[b.numplanes].dist = -b.planes[b.numplanes].dist;
			}
			else //this shouldn't happen
			{
				f.planes[f.numplanes] = *split;
				b.planes[b.numplanes] = *split;
				VectorInverse(b.planes[b.numplanes].normal);
				b.planes[b.numplanes].dist = -b.planes[b.numplanes].dist;
			}
			f.numplanes++;
			b.numplanes++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f.points[f.numplanes]);
			f.planes[f.numplanes] = volume->planes[i];
			f.numplanes++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, b.points[b.numplanes]);
			b.planes[b.numplanes] = volume->planes[i];
			b.numplanes++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		if (f.numplanes >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitLightVolume -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return 0;		// can't chop -- fall back to original
		}
		if (b.numplanes >= MAX_POINTS_ON_FIXED_WINDING)
		{
			_printf("WARNING: VL_SplitLightVolume -> MAX_POINTS_ON_FIXED_WINDING overflowed\n");
			return 0;		// can't chop -- fall back to original
		}

		// generate a split point
		p2 = volume->points[(i+1)%volume->numplanes];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}

		VectorCopy (mid, f.points[f.numplanes]);
		VectorCopy(mid, b.points[b.numplanes]);
		if (sides[i+1] == SIDE_BACK)
		{
			f.planes[f.numplanes] = *split;
			b.planes[b.numplanes] = volume->planes[i];
		}
		else
		{
			f.planes[f.numplanes] = volume->planes[i];
			b.planes[b.numplanes] = *split;
			VectorInverse(b.planes[b.numplanes].normal);
			b.planes[b.numplanes].dist = -b.planes[b.numplanes].dist;
		}
		f.numplanes++;
		b.numplanes++;
	}
	memcpy(volume->points, f.points, sizeof(vec3_t) * f.numplanes);
	memcpy(volume->planes, f.planes, sizeof(plane_t) * f.numplanes);
	volume->numplanes = f.numplanes;
	memcpy(back->points, b.points, sizeof(vec3_t) * b.numplanes);
	memcpy(back->planes, b.planes, sizeof(plane_t) * b.numplanes);
	back->numplanes = b.numplanes;

	return 2;
}

/*
=============
VL_PlaneForEdgeToWinding
=============
*/
void VL_PlaneForEdgeToWinding(vec3_t p1, vec3_t p2, winding_t *w, int windingonfront, plane_t *plane)
{
	int i, j;
	float length, d;
	vec3_t v1, v2;

	VectorSubtract(p2, p1, v1);
	for (i = 0; i < w->numpoints; i++)
	{
		VectorSubtract (w->points[i], p1, v2);

		plane->normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
		plane->normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
		plane->normal[2] = v1[0]*v2[1] - v1[1]*v2[0];
			
		// if points don't make a valid plane, skip it
		length = plane->normal[0] * plane->normal[0]
					+ plane->normal[1] * plane->normal[1]
					+ plane->normal[2] * plane->normal[2];
			
		if (length < ON_EPSILON)
			continue;

		length = 1/sqrt(length);
			
		plane->normal[0] *= length;
		plane->normal[1] *= length;
		plane->normal[2] *= length;

		plane->dist = DotProduct (w->points[i], plane->normal);
		//
		for (j = 0; j < w->numpoints; j++)
		{
			if (j == i)
				continue;
			d = DotProduct(w->points[j], plane->normal) - plane->dist;
			if (windingonfront)
			{
				if (d < -ON_EPSILON)
					break;
			}
			else
			{
				if (d > ON_EPSILON)
					break;
			}
		}
		if (j >= w->numpoints)
			return;
	}
}

/*
=============
VL_R_CastLightAtSurface
=============
*/
void VL_R_FloodLight(vlight_t *light, lightvolume_t *volume, int cluster, int firstportal);

void VL_R_CastLightAtSurface(vlight_t *light, lightvolume_t *volume)
{
	lsurfaceTest_t *test;
	int i, n;

	// light the surface with this volume
	VL_LightSurfaceWithVolume(volume->surfaceNum, volume->facetNum, light, volume);
	//
	test = lsurfaceTest[ volume->surfaceNum ];
	// if this is not a translucent surface
	if ( !(test->shader->surfaceFlags & SURF_ALPHASHADOW) && !(test->shader->contents & CONTENTS_TRANSLUCENT))
		return;
	//
	if (volume->numtransFacets >= MAX_TRANSLUCENTFACETS)
		Error("a light valume went through more than %d translucent facets", MAX_TRANSLUCENTFACETS);
	//add this translucent surface to the list
	volume->transSurfaces[volume->numtransFacets] = volume->surfaceNum;
	volume->transFacets[volume->numtransFacets] = volume->facetNum;
	volume->numtransFacets++;
	//clear the tested facets except the translucent ones
	memset(volume->facetTested, 0, sizeof(volume->facetTested));
	for (i = 0; i < volume->numtransFacets; i++)
	{
		test = lsurfaceTest[ volume->transSurfaces[i] ];
		n = test->facets[volume->transFacets[i]].num;
		volume->facetTested[n >> 3] |= 1 << (n & 7);
	}
	memset(volume->clusterTested, 0, sizeof(volume->clusterTested));
	volume->endplane = volume->farplane;
	volume->surfaceNum = -1;
	volume->facetNum = 0;
	VL_R_FloodLight(light, volume, volume->cluster, 0);
	if (volume->surfaceNum >= 0)
	{
		VL_R_CastLightAtSurface(light, volume);
	}
}

/*
=============
VL_R_SplitLightVolume
=============
*/
int numvolumes = 0;

int VL_R_SplitLightVolume(vlight_t *light, lightvolume_t *volume, plane_t *split, int cluster, int firstportal)
{
	lightvolume_t back;
	int res;

	//
	res = VL_SplitLightVolume(volume, &back, split, 0.1);
	// if the volume was split
	if (res == 2)
	{
		memcpy(back.clusterTested, volume->clusterTested, sizeof(back.clusterTested));
		memcpy(back.facetTested, volume->facetTested, sizeof(back.facetTested));
		back.num = numvolumes++;
		back.endplane = volume->endplane;
		back.surfaceNum = volume->surfaceNum;
		back.facetNum = volume->facetNum;
		back.type = volume->type;
		back.cluster = volume->cluster;
		back.farplane = volume->farplane;
		if (volume->numtransFacets > 0)
		{
			memcpy(back.transFacets, volume->transFacets, sizeof(back.transFacets));
			memcpy(back.transSurfaces, volume->transSurfaces, sizeof(back.transSurfaces));
		}
		back.numtransFacets = volume->numtransFacets;
		//
		// flood the volume at the back of the split plane
		VL_R_FloodLight(light, &back, cluster, firstportal);
		// if the back volume hit a surface
		if (back.surfaceNum >= 0)
		{
			VL_R_CastLightAtSurface(light, &back);
		}
	}
	return res;
}

/*
=============
VL_R_FloodLight
=============
*/
void VL_R_FloodLight(vlight_t *light, lightvolume_t *volume, int cluster, int firstportal)
{
	int i, j, k, res, surfaceNum, backfaceculled, testculled;
	float d;
	winding_t winding, tmpwinding;
	lleaf_t *leaf;
	lportal_t *p;
	lsurfaceTest_t *test;
	lFacet_t *facet;
	vec3_t dir1, dir2;
	plane_t plane;

	//	DebugNet_RemoveAllPolys();
	//	VL_DrawLightVolume(light, volume);

	// if the first portal is not zero then we've checked all occluders in this leaf already
	if (firstportal == 0)
	{
		// check all potential occluders in this leaf
		for (i = 0; i < leafs[cluster].numSurfaces; i++)
		{
			surfaceNum = clustersurfaces[leafs[cluster].firstSurface + i];
			//
			test = lsurfaceTest[ surfaceNum ];
			if ( !test )
				continue;
			//
			testculled = qfalse;
			// use surface as an occluder
			for (j = 0; j < test->numFacets; j++)
			{
				// use each facet as an occluder
				facet = &test->facets[j];
				//
				//	memcpy(winding.points, facet->points, sizeof(vec3_t) * facet->numpoints);
				//	winding.numpoints = facet->numpoints;
				//	DebugNet_DrawWinding(&winding, 5);
				//
				// if the facet was tested already
				if ( volume->facetTested[facet->num >> 3] & (1 << (facet->num & 7)) )
					continue;
				volume->facetTested[facet->num >> 3] |= 1 << (facet->num & 7);
				// backface culling for planar surfaces
				backfaceculled = qfalse;
				if (!test->patch && !test->trisoup)
				{
					if (volume->type == VOLUME_NORMAL)
					{
						// facet backface culling
						d = DotProduct(light->origin, facet->plane.normal) - facet->plane.dist;
						if (d < 0)
						{
							// NOTE: this doesn't work too great because of sometimes very bad tesselation
							//		of surfaces that are supposed to be flat
							// FIXME: to work around this problem we should make sure that all facets
							//		created from planar surfaces use the lightmapVecs normal vector
							/*
							if ( !test->shader->twoSided )
							{
								// skip all other facets of this surface as well because they are in the same plane
								for (k = 0; k < test->numFacets; k++)
								{
									facet = &test->facets[k];
									volume->facetTested[facet->num >> 3] |= 1 << (facet->num & 7);
								}
							}*/
							backfaceculled = qtrue;
						}
					}
					else
					{
						// FIXME: if all light source winding points are at the back of the facet
						//			plane then backfaceculled = qtrue
					}
				}
				else // backface culling per facet for patches and triangle soups
				{
					if (volume->type == VOLUME_NORMAL)
					{
						// facet backface culling
						d = DotProduct(light->origin, facet->plane.normal) - facet->plane.dist;
						if (d < 0)
							backfaceculled = qtrue;
					}
					else
					{
						// FIXME: if all light source winding points are at the back of the facet
						//			plane then backfaceculled = qtrue
					}
				}
				/* chopping does this already
				// check if this facet is totally or partly in front of the volume end plane
				for (k = 0; k < facet->numpoints; k++)
				{
					d = DotProduct(volume->endplane.normal, facet->points[k]) - volume->endplane.dist;
					if (d > ON_EPSILON)
						break;
				}
				// if this facet is outside the light volume
				if (k >= facet->numpoints)
					continue;
				*/
				//
				if (backfaceculled)
				{
					// if the facet is not two sided
					if ( !nobackfaceculling && !test->shader->twoSided )
						continue;
					// flip the winding
					for (k = 0; k < facet->numpoints; k++)
						VectorCopy(facet->points[k], winding.points[facet->numpoints - k - 1]);
					winding.numpoints = facet->numpoints;
				}
				else
				{
					memcpy(winding.points, facet->points, sizeof(vec3_t) * facet->numpoints);
					winding.numpoints = facet->numpoints;
				}
				//
				if (!testculled)
				{
					testculled = qtrue;
					// fast check if the surface sphere is totally behind the volume end plane
					d = DotProduct(volume->endplane.normal, test->origin) - volume->endplane.dist;
					if (d < -test->radius)
					{
						for (k = 0; k < test->numFacets; k++)
						{
							facet = &test->facets[k];
							volume->facetTested[facet->num >> 3] |= 1 << (facet->num & 7);
						}
						break;
					}
					for (k = 0; k < volume->numplanes; k++)
					{
						d = DotProduct(volume->planes[k].normal, test->origin) - volume->planes[k].dist;
						if (d < - test->radius)
						{
							for (k = 0; k < test->numFacets; k++)
							{
								facet = &test->facets[k];
								volume->facetTested[facet->num >> 3] |= 1 << (facet->num & 7);
							}
							break;
						}
					}
					if (k < volume->numplanes)
						break;
				}
				//NOTE: we have to chop the facet winding with the volume end plane because
				//		the faces in Q3 are not stitched together nicely
				res = VL_ChopWinding(&winding, &volume->endplane, 0.01);
				// if the facet is on or at the back of the volume end plane
				if (res == SIDE_BACK || res == SIDE_ON)
					continue;
				// check if the facet winding is totally or partly inside the light volume
				memcpy(&tmpwinding, &winding, sizeof(winding_t));
				for (k = 0; k < volume->numplanes; k++)
				{
					res = VL_ChopWinding(&tmpwinding, &volume->planes[k], 0.01);
					if (res == SIDE_BACK || res == SIDE_ON)
						break;
				}
				// if no part of the light volume is occluded by this facet
				if (k < volume->numplanes)
					continue;
				//
				for (k = 0; k < winding.numpoints; k++)
				{
					if (volume->type == VOLUME_DIRECTED)
					{
						VectorSubtract(winding.points[(k+1) % winding.numpoints], winding.points[k], dir1);
						CrossProduct(light->normal, dir1, plane.normal);
						VectorNormalize(plane.normal, plane.normal);
						plane.dist = DotProduct(plane.normal, winding.points[k]);
					}
					else
					{
						VectorSubtract(winding.points[(k+1) % winding.numpoints], winding.points[k], dir1);
						VectorSubtract(light->origin, winding.points[k], dir2);
						CrossProduct(dir1, dir2, plane.normal);
						VectorNormalize(plane.normal, plane.normal);
						plane.dist = DotProduct(plane.normal, winding.points[k]);
					}
					res = VL_R_SplitLightVolume(light, volume, &plane, cluster, 0);
					if (res == 1)
						break; //the facet wasn't really inside the volume
				}
				if (k >= winding.numpoints)
				{
					volume->endplane = facet->plane;
					if (backfaceculled)
					{
						VectorInverse(volume->endplane.normal);
						volume->endplane.dist = -volume->endplane.dist;
					}
					volume->surfaceNum = surfaceNum;
					volume->facetNum = j;
				}
			}
		}
	}
	// we've tested all occluders in this cluster
	volume->clusterTested[cluster >> 3] |= 1 << (cluster & 7);
	// flood light through the portals of the current leaf
	leaf = &leafs[cluster];
	for (i = firstportal; i < leaf->numportals; i++)
	{
		p = leaf->portals[i];
		//
		//	memcpy(&winding, p->winding, sizeof(winding_t));
		//	DebugNet_DrawWinding(&winding, 5);
		// if already flooded into the cluster this portal leads to
		if ( volume->clusterTested[p->leaf >> 3] & (1 << (p->leaf & 7)) )
			continue;
		//
		if (volume->type == VOLUME_NORMAL)
		{
			// portal backface culling
			d = DotProduct(light->origin, p->plane.normal) - p->plane.dist;
			if (d > 0) // portal plane normal points into neighbour cluster
				continue;
		}
		else
		{
			// FIXME: if all light source winding points are at the back of this portal
			//			plane then there's no need to flood through
		}
		// check if this portal is totally or partly in front of the volume end plane
		// fast check with portal sphere
		d = DotProduct(volume->endplane.normal, p->origin) - volume->endplane.dist;
		if (d < -p->radius)
			continue;
		for (j = 0; j < p->winding->numpoints; j++)
		{
			d = DotProduct(volume->endplane.normal, p->winding->points[j]) - volume->endplane.dist;
			if (d > -0.01)
				break;
		}
		// if this portal is totally behind the light volume end plane
		if (j >= p->winding->numpoints)
			continue;
		//distance from point light to portal
		d = DotProduct(p->plane.normal, light->origin) - p->plane.dist;
		// only check if a point light is Not *on* the portal
		if (volume->type != VOLUME_NORMAL || fabs(d) > 0.1)
		{
			// check if the portal is partly or totally inside the light volume
			memcpy(&winding, p->winding, sizeof(winding_t));
			for (j = 0; j < volume->numplanes; j++)
			{
				res = VL_ChopWinding(&winding, &volume->planes[j], 0.01);
				if (res == SIDE_BACK || res == SIDE_ON)
					break;
			}
			// if the light volume does not go through this portal at all
			if (j < volume->numplanes)
				continue;
		}
		// chop the light volume with the portal
		for (k = 0; k < p->winding->numpoints; k++)
		{
			if (volume->type == VOLUME_DIRECTED)
			{
				VectorSubtract(p->winding->points[(k+1) % p->winding->numpoints], p->winding->points[k], dir1);
				CrossProduct(light->normal, dir1, plane.normal);
				VectorNormalize(plane.normal, plane.normal);
				plane.dist = DotProduct(plane.normal, p->winding->points[k]);
			}
			else
			{
				VectorSubtract(p->winding->points[(k+1) % p->winding->numpoints], p->winding->points[k], dir1);
				VectorSubtract(light->origin, p->winding->points[k], dir2);
				CrossProduct(dir1, dir2, plane.normal);
				VectorNormalize(plane.normal, plane.normal);
				plane.dist = DotProduct(plane.normal, p->winding->points[k]);
			}
			res = VL_R_SplitLightVolume(light, volume, &plane, cluster, i+1);
			if (res == 1)
				break; //volume didn't really go through the portal
		}
		// if the light volume went through the portal
		if (k >= p->winding->numpoints)
		{
			// flood through the portal
			VL_R_FloodLight(light, volume, p->leaf, 0);
		}
	}
}

/*
=============
VL_R_FloodAreaSpotLight
=============
*/
void VL_FloodAreaSpotLight(vlight_t *light, winding_t *w, int leafnum)
{
}

/*
=============
VL_R_SubdivideAreaSpotLight
=============
*/
void VL_R_SubdivideAreaSpotLight(vlight_t *light, int nodenum, winding_t *w)
{
	int leafnum, res;
	dnode_t *node;
	dplane_t *plane;
	winding_t back;
	plane_t split;

	while(nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];

		VectorCopy(plane->normal, split.normal);
		split.dist = plane->dist;
		res = VL_SplitWinding (w, &back, &split, 0.1);

		if (res == SIDE_FRONT)
		{
			nodenum = node->children[0];
		}
		else if (res == SIDE_BACK)
		{
			nodenum = node->children[1];
		}
		else if (res == SIDE_ON)
		{
			memcpy(&back, w, sizeof(winding_t));
			VL_R_SubdivideAreaSpotLight(light, node->children[1], &back);
			nodenum = node->children[0];
		}
		else
		{
			VL_R_SubdivideAreaSpotLight(light, node->children[1], &back);
			nodenum = node->children[0];
		}
	}
	leafnum = -nodenum - 1;
	if (dleafs[leafnum].cluster != -1)
	{
		VL_FloodAreaSpotLight(light, w, leafnum);
	}
}

/*
=============
VL_R_FloodRadialAreaLight
=============
*/
void VL_FloodRadialAreaLight(vlight_t *light, winding_t *w, int leafnum)
{
}

/*
=============
VL_R_SubdivideRadialAreaLight
=============
*/
void VL_R_SubdivideRadialAreaLight(vlight_t *light, int nodenum, winding_t *w)
{
	int leafnum, res;
	dnode_t *node;
	dplane_t *plane;
	winding_t back;
	plane_t split;

	while(nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];

		VectorCopy(plane->normal, split.normal);
		split.dist = plane->dist;
		res = VL_SplitWinding (w, &back, &split, 0.1);

		if (res == SIDE_FRONT)
		{
			nodenum = node->children[0];
		}
		else if (res == SIDE_BACK)
		{
			nodenum = node->children[1];
		}
		else if (res == SIDE_ON)
		{
			memcpy(&back, w, sizeof(winding_t));
			VL_R_SubdivideRadialAreaLight(light, node->children[1], &back);
			nodenum = node->children[0];
		}
		else
		{
			VL_R_SubdivideRadialAreaLight(light, node->children[1], &back);
			nodenum = node->children[0];
		}
	}
	leafnum = -nodenum - 1;
	if (dleafs[leafnum].cluster != -1)
	{
		VL_FloodRadialAreaLight(light, w, leafnum);
	}
}

/*
=============
VL_R_FloodDirectedLight
=============
*/
void VL_FloodDirectedLight(vlight_t *light, winding_t *w, int leafnum)
{
	int i;
	float dist;
	lightvolume_t volume;
	vec3_t dir;

	if (light->atten_disttype == LDAT_NOSCALE)
	{
		// light travels without decrease in intensity over distance
		dist = MAX_WORLD_COORD;
	}
	else
	{
		if ( light->atten_disttype == LDAT_LINEAR )
			dist = light->photons * lightLinearScale;
		else
			dist = sqrt(light->photons);
	}

	memset(&volume, 0, sizeof(lightvolume_t));
	for (i = 0; i < w->numpoints; i++)
	{
		VectorMA(w->points[i], dist, light->normal, volume.points[i]);
		VectorSubtract(w->points[(i+1)%w->numpoints], w->points[i], dir);
		CrossProduct(light->normal, dir, volume.planes[i].normal);
		VectorNormalize(volume.planes[i].normal, volume.planes[i].normal);
		volume.planes[i].dist = DotProduct(volume.planes[i].normal, w->points[i]);
	}
	volume.numplanes = w->numpoints;
	VectorCopy(light->normal, volume.endplane.normal);
	VectorInverse(volume.endplane.normal);
	volume.endplane.dist = DotProduct(volume.endplane.normal, volume.points[0]);
	volume.farplane = volume.endplane;
	volume.surfaceNum = -1;
	volume.type = VOLUME_DIRECTED;
	volume.cluster = dleafs[leafnum].cluster;
	VL_R_FloodLight(light, &volume, volume.cluster, 0);
	if (volume.surfaceNum >= 0)
	{
		VL_R_CastLightAtSurface(light, &volume);
	}
}

/*
=============
VL_R_SubdivideDirectedAreaLight
=============
*/
void VL_R_SubdivideDirectedAreaLight(vlight_t *light, int nodenum, winding_t *w)
{
	int leafnum, res;
	dnode_t *node;
	dplane_t *plane;
	winding_t back;
	plane_t split;

	while(nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planeNum];

		VectorCopy(plane->normal, split.normal);
		split.dist = plane->dist;
		res = VL_SplitWinding (w, &back, &split, 0.1);

		if (res == SIDE_FRONT)
		{
			nodenum = node->children[0];
		}
		else if (res == SIDE_BACK)
		{
			nodenum = node->children[1];
		}
		else if (res == SIDE_ON)
		{
			memcpy(&back, w, sizeof(winding_t));
			VL_R_SubdivideDirectedAreaLight(light, node->children[1], &back);
			nodenum = node->children[0];
		}
		else
		{
			VL_R_SubdivideDirectedAreaLight(light, node->children[1], &back);
			nodenum = node->children[0];
		}
	}
	leafnum = -nodenum - 1;
	if (dleafs[leafnum].cluster != -1)
	{
		VL_FloodDirectedLight(light, w, leafnum);
	}
}

/*
=============
VL_FloodLight
=============
*/
void VL_FloodLight(vlight_t *light)
{
	lightvolume_t volume;
	dleaf_t *leaf;
	int leafnum, i, j, k, dir[2][4] = {{1, 1, -1, -1}, {1, -1, -1, 1}};
	float a, step, dist, radius, windingdist;
	vec3_t vec, r, p, temp;
	winding_t winding;

	switch(light->type)
	{
		case LIGHT_POINTRADIAL:
		{
			// source is a point
			// light radiates in all directions
			// creates sharp shadows
			//
			// create 6 volumes shining in the axis directions
			// what about: 4 tetrahedrons instead?
			//
			if ( light->atten_disttype == LDAT_LINEAR )
				dist = light->photons * lightLinearScale;
			else
				dist = sqrt(light->photons);
			//always put the winding at a large distance to avoid epsilon issues
			windingdist = MAX_WORLD_COORD;
			if (dist > windingdist)
				windingdist = dist;
			//
			leafnum = VL_LightLeafnum(light->origin);
			leaf = &dleafs[leafnum];
			if (leaf->cluster == -1)
			{
				light->insolid = qtrue;
				break;
			}
			// for each axis
			for (i = 0; i < 3; i++)
			{
				// for both directions on the axis
				for (j = -1; j <= 1; j += 2)
				{
					memset(&volume, 0, sizeof(lightvolume_t));
					volume.numplanes = 0;
					for (k = 0; k < 4; k ++)
					{
						volume.points[volume.numplanes][i] = light->origin[i] + j * windingdist;
						volume.points[volume.numplanes][(i+1)%3] = light->origin[(i+1)%3] + dir[0][k] * windingdist;
						volume.points[volume.numplanes][(i+2)%3] = light->origin[(i+2)%3] + dir[1][k] * windingdist;
						volume.numplanes++;
					}
					if (j >= 0)
					{
						VectorCopy(volume.points[0], temp);
						VectorCopy(volume.points[2], volume.points[0]);
						VectorCopy(temp, volume.points[2]);
					}
					for (k = 0; k < volume.numplanes; k++)
					{
						VL_PlaneFromPoints(&volume.planes[k], light->origin, volume.points[(k+1)%volume.numplanes], volume.points[k]);
					}
					VectorCopy(light->origin, temp);
					temp[i] += (float) j * dist;
					VectorClear(volume.endplane.normal);
					volume.endplane.normal[i] = -j;
					volume.endplane.dist = DotProduct(volume.endplane.normal, temp); //DotProduct(volume.endplane.normal, volume.points[0]);
					volume.farplane = volume.endplane;
					volume.cluster = leaf->cluster;
					volume.surfaceNum = -1;
					volume.type = VOLUME_NORMAL;
					//
					memset(volume.facetTested, 0, sizeof(volume.facetTested));
					memset(volume.clusterTested, 0, sizeof(volume.clusterTested));
					VL_R_FloodLight(light, &volume, leaf->cluster, 0);
					if (volume.surfaceNum >= 0)
					{
						VL_R_CastLightAtSurface(light, &volume);
					}
				}
			}
			break;
		}
		case LIGHT_POINTSPOT:
		{
			// source is a point
			// light is targetted
			// creates sharp shadows
			//
			// what about using brushes to shape spot lights? that'd be pretty cool
			//
			if ( light->atten_disttype == LDAT_LINEAR )
				dist = light->photons * lightLinearScale;
			else
				dist = sqrt(light->photons);
			dist *= 2;
			//
			windingdist = 4096;
			if (dist > windingdist)
				windingdist = dist;
			//take 8 times the cone radius because the spotlight also lights outside the cone
			radius = 8 * windingdist * light->radiusByDist;
			//
			memset(&volume, 0, sizeof(lightvolume_t));
			leafnum = VL_LightLeafnum(light->origin);
			leaf = &dleafs[leafnum];
			if (leaf->cluster == -1)
			{
				light->insolid = qtrue;
				break;
			}
			//
			VectorClear(vec);
			for (i = 0; i < 3; i++)
			{
				if (light->normal[i] > -0.9 && light->normal[i] < 0.9)
				{
					vec[i] = 1;
					break;
				}
			}
			CrossProduct(light->normal, vec, r);
			VectorScale(r, radius, p);
			volume.numplanes = 0;
			step = 45;
			for (a = step / 2; a < 360 + step / 2; a += step)
			{
				RotatePointAroundVector(volume.points[volume.numplanes], light->normal, p, a);
				VectorAdd(light->origin, volume.points[volume.numplanes], volume.points[volume.numplanes]);
				VectorMA(volume.points[volume.numplanes], windingdist, light->normal, volume.points[volume.numplanes]);
				volume.numplanes++;
			}
			for (i = 0; i < volume.numplanes; i++)
			{
				VL_PlaneFromPoints(&volume.planes[i], light->origin, volume.points[(i+1)%volume.numplanes], volume.points[i]);
			}
			VectorMA(light->origin, dist, light->normal, temp);
			VectorCopy(light->normal, volume.endplane.normal);
			VectorInverse(volume.endplane.normal);
			volume.endplane.dist = DotProduct(volume.endplane.normal, temp);//DotProduct(volume.endplane.normal, volume.points[0]);
			volume.farplane = volume.endplane;
			volume.cluster = leaf->cluster;
			volume.surfaceNum = -1;
			volume.type = VOLUME_NORMAL;
			//
			memset(volume.facetTested, 0, sizeof(volume.facetTested));
			memset(volume.clusterTested, 0, sizeof(volume.clusterTested));
			VL_R_FloodLight(light, &volume, leaf->cluster, 0);
			if (volume.surfaceNum >= 0)
			{
				VL_R_CastLightAtSurface(light, &volume);
			}
			break;
		}
		case LIGHT_POINTFAKESURFACE:
		{
			float value;
			int n, axis;
			vec3_t v, vecs[2];

			if ( light->atten_disttype == LDAT_LINEAR )
				dist = light->photons * lightLinearScale;
			else
				dist = sqrt(light->photons);
			//always put the winding at a large distance to avoid epsilon issues
			windingdist = 4096;
			if (dist > windingdist)
				windingdist = dist;
			//
			VectorMA(light->origin, 0.1, light->normal, light->origin);
			//
			leafnum = VL_LightLeafnum(light->origin);
			leaf = &dleafs[leafnum];
			if (leaf->cluster == -1)
			{
				light->insolid = qtrue;
				break;
			}
			value = 0;
			for (i = 0; i < 3; i++)
			{
				if (fabs(light->normal[i]) > value)
				{
					value = fabs(light->normal[i]);
					axis = i;
				}
			}
			for (i = 0; i < 2; i++)
			{
				VectorClear(v);
				v[(axis + 1 + i) % 3] = 1;
				CrossProduct(light->normal, v, vecs[i]);
			}
			//cast 4 volumes at the front of the surface
			for (i = -1; i <= 1; i += 2)
			{
				for (j = -1; j <= 1; j += 2)
				{
					for (n = 0; n < 2; n++)
					{
						memset(&volume, 0, sizeof(lightvolume_t));
						volume.numplanes = 3;
						VectorMA(light->origin, i * windingdist, vecs[0], volume.points[(i == j) == n]);
						VectorMA(light->origin, j * windingdist, vecs[1], volume.points[(i != j) == n]);
						VectorMA(light->origin, windingdist, light->normal, volume.points[2]);
						for (k = 0; k < volume.numplanes; k++)
						{
							VL_PlaneFromPoints(&volume.planes[k], light->origin, volume.points[(k+1)%volume.numplanes], volume.points[k]);
						}
						VL_PlaneFromPoints(&volume.endplane, volume.points[0], volume.points[1], volume.points[2]);
						VectorMA(light->origin, dist, light->normal, temp);
						volume.endplane.dist = DotProduct(volume.endplane.normal, temp);
						if (DotProduct(light->origin, volume.endplane.normal) - volume.endplane.dist > 0)
							break;
					}
					volume.farplane = volume.endplane;
					volume.cluster = leaf->cluster;
					volume.surfaceNum = -1;
					volume.type = VOLUME_NORMAL;
					//
					memset(volume.facetTested, 0, sizeof(volume.facetTested));
					memset(volume.clusterTested, 0, sizeof(volume.clusterTested));

					VL_R_FloodLight(light, &volume, leaf->cluster, 0);
					if (volume.surfaceNum >= 0)
					{
						VL_R_CastLightAtSurface(light, &volume);
					}
				}
			}
			break;
		}
		case LIGHT_SURFACEDIRECTED:
		{
			// source is an area defined by a winding
			// the light is unidirectional
			// creates sharp shadows
			// for instance sun light or laser light
			//
			memcpy(&winding, &light->w, sizeof(winding_t));
			VL_R_SubdivideDirectedAreaLight(light, 0, &winding);
			break;
		}
		case LIGHT_SURFACERADIAL:
		{
			// source is an area defined by a winding
			// the light radiates in all directions at the front of the winding plane
			//
			memcpy(&winding, &light->w, sizeof(winding_t));
			VL_R_SubdivideRadialAreaLight(light, 0, &winding);
			break;
		}
		case LIGHT_SURFACESPOT:
		{
			// source is an area defined by a winding
			// light is targetted but not unidirectional
			//
			memcpy(&winding, &light->w, sizeof(winding_t));
			VL_R_SubdivideAreaSpotLight(light, 0, &winding);
			break;
		}
	}
}

/*
=============
VL_FloodLightThread
=============
*/
void VL_FloodLightThread(int num)
{
	VL_FloodLight(vlights[num]);
}

/*
=============
VL_TestLightLeafs
=============
*/
void VL_TestLightLeafs(void)
{
	int leafnum, i;
	vlight_t *light;
	dleaf_t *leaf;

	for (i = 0; i < numvlights; i++)
	{
		light = vlights[i];
		if (light->type != LIGHT_POINTRADIAL &&
			light->type != LIGHT_POINTSPOT)
			continue;
		leafnum = VL_LightLeafnum(light->origin);
		leaf = &dleafs[leafnum];
		if (leaf->cluster == -1)
			if (light->type == LIGHT_POINTRADIAL)
				qprintf("light in solid at %1.1f %1.1f %1.1f\n", light->origin[0], light->origin[1], light->origin[2]);
			else if (light->type == LIGHT_POINTSPOT)
				qprintf("spot light in solid at %1.1f %1.1f %1.1f\n", light->origin[0], light->origin[1], light->origin[2]);
	}
}


/*
=============
VL_DoForcedTraceLight
=============
*/
// from light.c
void TraceLtm( int num );

void VL_DoForcedTraceLight(int num)
{
	dsurface_t		*ds;
	shaderInfo_t	*si;

	ds = &drawSurfaces[num];

	if ( ds->surfaceType == MST_TRIANGLE_SOUP )
		return;

	if ( ds->lightmapNum < 0 )
		return;

	// always light entity surfaces with the old light algorithm
	if ( !entitySurface[num] )
	{
		si = ShaderInfoForShader( dshaders[ ds->shaderNum].shader );

		if (defaulttracelight)
		{
			if (si->forceVLight)
				return;
		}
		else
		{
			if (!si->forceTraceLight)
				return;
		}
	}

	TraceLtm(num);
}

/*
=============
VL_DoForcedTraceLightSurfaces
=============
*/
void VL_DoForcedTraceLightSurfaces(void)
{
	_printf( "forced trace light\n" );
	RunThreadsOnIndividual( numDrawSurfaces, qtrue, VL_DoForcedTraceLight );
}

float *oldLightFloats;

/*
=============
VL_SurfaceRadiosity
=============
*/
void VL_SurfaceRadiosity( int num ) {
	dsurface_t		*ds;
	mesh_t			*mesh;
	shaderInfo_t	*si;
	lsurfaceTest_t *test;
	int x, y, k;
	vec3_t base, normal;
	float *color, area;
	vlight_t vlight;

	ds = &drawSurfaces[num];

	if ( ds->lightmapNum < 0 ) {
		return;		// doesn't have a lightmap
	}

	// vertex-lit triangle model
	if ( ds->surfaceType == MST_TRIANGLE_SOUP ) {
		return;
	}

	si = ShaderInfoForShader( dshaders[ ds->shaderNum].shader );
	test = lsurfaceTest[ num ];

	if (!test) {
		return;
	}

	for (x = 0; x < ds->lightmapWidth; x++) {
		for (y = 0; y < ds->lightmapHeight; y++) {
			//
			k = ( ds->lightmapNum * LIGHTMAP_HEIGHT + ds->lightmapY + y) 
							* LIGHTMAP_WIDTH + ds->lightmapX + x;
			area = lightmappixelarea[k];
			if (area <= 0)
				continue;
			//
			if (ds->surfaceType == MST_PATCH)
			{
				mesh = test->detailMesh;
				VectorCopy( mesh->verts[y*mesh->width+x].xyz, base);
				VectorCopy( mesh->verts[y*mesh->width+x].normal, normal);
			}
			else
			{
				VectorMA(ds->lightmapOrigin, (float) x, ds->lightmapVecs[0], base);
				VectorMA(base, (float) y, ds->lightmapVecs[1], base);
				VectorCopy(test->facets[0].plane.normal, normal);
			}
			// create ligth from base
			memset(&vlight, 0, sizeof(vlight_t));
			color = &oldLightFloats[k*3];
			// a few units away from the surface
			VectorMA(base, 5, normal, vlight.origin);
			ColorNormalize(color, vlight.color);
			// ok this is crap
			vlight.photons = VectorLength(color) * 0.05 * lightPointScale / (area * radiosity_scale);
			// what about using a front facing light only ?
			vlight.type = LIGHT_POINTRADIAL;
			// flood the light from this lightmap pixel
			VL_FloodLight(&vlight);
			// only one thread at a time may write to the lightmap of this surface
			MutexLock(test->mutex);
			// don't light the lightmap pixel itself
			lightFloats[k*3] = oldLightFloats[k*3];
			lightFloats[k*3+1] = oldLightFloats[k*3+1];
			lightFloats[k*3+2] = oldLightFloats[k*3+2];
			//
			MutexUnlock(test->mutex);
		}
	}
}

/*
=============
VL_Radiosity

this aint working real well but it's fun to play with.
=============
*/
void VL_Radiosity(void) {

	oldLightFloats = lightFloats;
	lightFloats = (float *) malloc(numLightBytes * sizeof(float));
	memcpy(lightFloats, oldLightFloats, numLightBytes * sizeof(float));
	_printf("%7i surfaces\n", numDrawSurfaces);
	RunThreadsOnIndividual( numDrawSurfaces, qtrue, VL_SurfaceRadiosity );
	free(oldLightFloats);
}

/*
=============
VL_LightWorld
=============
*/
void VL_LightWorld(void)
{
	int i, numcastedvolumes, numvlightsinsolid;
	float f;

	// find the optional world ambient
	GetVectorForKey( &entities[0], "_color", lightAmbientColor );
	f = FloatForKey( &entities[0], "ambient" );
	VectorScale( lightAmbientColor, f, lightAmbientColor );
	/*
	_printf("\r%6d lights out of %d", 0, numvlights);
	for (i = 0; i < numvlights; i++)
	{
		_printf("\r%6d", i);
		VL_FloodLight(vlights[i]);
	}
	_printf("\r%6d lights out of %d\n", i, numvlights);
	*/
	_printf("%7i lights\n", numvlights);
	RunThreadsOnIndividual( numvlights, qtrue, VL_FloodLightThread );

	numcastedvolumes = 0;
	for ( i = 0 ; i < numDrawSurfaces ; i++ ) {
		if (lsurfaceTest[i])
			numcastedvolumes += lsurfaceTest[i]->numvolumes;
	}
	_printf("%7i light volumes casted\n", numcastedvolumes);
	numvlightsinsolid = 0;
	for (i = 0; i < numvlights; i++)
	{
		if (vlights[i]->insolid)
			numvlightsinsolid++;
	}
	_printf("%7i lights in solid\n", numvlightsinsolid);
	//
	radiosity_scale = 1;
	for (i = 0; i < radiosity; i++) {
		VL_Radiosity();
		radiosity_scale <<= 1;
	}
	//
	VL_StoreLightmap();
	// redo surfaces with the old light algorithm when needed
	VL_DoForcedTraceLightSurfaces();
}

/*
=============
VL_CreateEntityLights
=============
*/
entity_t *FindTargetEntity( const char *target );

void VL_CreateEntityLights (void)
{
	int		i, c_entityLights;
	vlight_t	*dl;
	entity_t	*e, *e2;
	const char	*name;
	const char	*target;
	vec3_t	dest;
	const char	*_color;
	float	intensity;
	int		spawnflags;

	//
	c_entityLights = 0;
	_printf("Creating entity lights...\n");
	//
	for ( i = 0 ; i < num_entities ; i++ ) {
		e = &entities[i];
		name = ValueForKey (e, "classname");
		if (strncmp (name, "light", 5))
			continue;

		dl = malloc(sizeof(*dl));
		memset (dl, 0, sizeof(*dl));

		spawnflags = FloatForKey (e, "spawnflags");
		if ( spawnflags & 1 ) {
			dl->atten_disttype = LDAT_LINEAR;
		}
		if ( spawnflags & 2 ) {
			dl->atten_disttype = LDAT_NOSCALE;
		}
		if ( spawnflags & 4 ) {
			dl->atten_angletype = LAAT_QUADRATIC;
		}
		if ( spawnflags & 8 ) {
			dl->atten_angletype = LAAT_DOUBLEQUADRATIC;
		}

		dl->atten_distscale = FloatForKey(e, "atten_distscale");
		dl->atten_anglescale = FloatForKey(e, "atten_anglescale");

		GetVectorForKey (e, "origin", dl->origin);
		dl->style = FloatForKey (e, "_style");
		if (!dl->style)
			dl->style = FloatForKey (e, "style");
		if (dl->style < 0)
			dl->style = 0;

		intensity = FloatForKey (e, "light");
		if (!intensity)
			intensity = FloatForKey (e, "_light");
		if (!intensity)
			intensity = 300;
		_color = ValueForKey (e, "_color");
		if (_color && _color[0])
		{
			sscanf (_color, "%f %f %f", &dl->color[0],&dl->color[1],&dl->color[2]);
			ColorNormalize (dl->color, dl->color);
		}
		else
			dl->color[0] = dl->color[1] = dl->color[2] = 1.0;

		intensity = intensity * lightPointScale;
		dl->photons = intensity;

		dl->type = LIGHT_POINTRADIAL;

		// lights with a target will be spotlights
		target = ValueForKey (e, "target");

		if ( target[0] ) {
			float	radius;
			float	dist;

			e2 = FindTargetEntity (target);
			if (!e2) {
				_printf ("WARNING: light at (%i %i %i) has missing target\n",
				(int)dl->origin[0], (int)dl->origin[1], (int)dl->origin[2]);
			} else {
				GetVectorForKey (e2, "origin", dest);
				VectorSubtract (dest, dl->origin, dl->normal);
				dist = VectorNormalize (dl->normal, dl->normal);
				radius = FloatForKey (e, "radius");
				if ( !radius ) {
					radius = 64;
				}
				if ( !dist ) {
					dist = 64;
				}
				dl->radiusByDist = (radius + 16) / dist;
				dl->type = LIGHT_POINTSPOT;
			}
		}
		vlights[numvlights++] = dl;
		c_entityLights++;
	}
	_printf("%7i entity lights\n", c_entityLights);
}

/*
==================
VL_SubdivideAreaLight
==================
*/
void VL_SubdivideAreaLight( shaderInfo_t *ls, winding_t *w, vec3_t normal, 
						float areaSubdivide, qboolean backsplash ) {
	float			area, value, intensity;
	vlight_t			*dl, *dl2;
	vec3_t			mins, maxs;
	int				axis;
	winding_t		*front, *back;
	vec3_t			planeNormal;
	float			planeDist;

	if ( !w ) {
		return;
	}

	WindingBounds( w, mins, maxs );

	// check for subdivision
	for ( axis = 0 ; axis < 3 ; axis++ ) {
		if ( maxs[axis] - mins[axis] > areaSubdivide ) {
			VectorClear( planeNormal );
			planeNormal[axis] = 1;
			planeDist = ( maxs[axis] + mins[axis] ) * 0.5;
			ClipWindingEpsilon ( w, planeNormal, planeDist, ON_EPSILON, &front, &back );
			VL_SubdivideAreaLight( ls, front, normal, areaSubdivide, qfalse );
			VL_SubdivideAreaLight( ls, back, normal, areaSubdivide, qfalse );
			FreeWinding( w );
			return;
		}
	}

	// create a light from this
	area = WindingArea (w);
	if ( area <= 0 || area > 20000000 ) {
		return;
	}

	dl = malloc(sizeof(*dl));
	memset (dl, 0, sizeof(*dl));
	dl->type = LIGHT_POINTFAKESURFACE;

	WindingCenter( w, dl->origin );
	memcpy(dl->w.points, w->points, sizeof(vec3_t) * w->numpoints);
	dl->w.numpoints = w->numpoints;
	VectorCopy ( normal, dl->normal);
	VectorCopy ( normal, dl->plane);
	dl->plane[3] = DotProduct( dl->origin, normal );

	value = ls->value;
	intensity = value * area * lightAreaScale;
	VectorAdd( dl->origin, dl->normal, dl->origin );

	VectorCopy( ls->color, dl->color );

	dl->photons = intensity;

	// emitColor is irrespective of the area
	VectorScale( ls->color, value*lightFormFactorValueScale*lightAreaScale, dl->emitColor );
	//
	VectorCopy(dl->emitColor, dl->color);

	dl->si = ls;

	if ( ls->contents & CONTENTS_FOG ) {
		dl->twosided = qtrue;
	}

	vlights[numvlights++] = dl;

	// optionally create a point backsplash light
	if ( backsplash && ls->backsplashFraction > 0 ) {

		dl2 = malloc(sizeof(*dl));
		memset (dl2, 0, sizeof(*dl2));
		dl2->type = LIGHT_POINTRADIAL;

		VectorMA( dl->origin, ls->backsplashDistance, normal, dl2->origin );

		VectorCopy( ls->color, dl2->color );

		dl2->photons = dl->photons * ls->backsplashFraction;
		dl2->si = ls;

		vlights[numvlights++] = dl2;
	}
}

/*
==================
VL_CreateFakeSurfaceLights
==================
*/
void VL_CreateFakeSurfaceLights( void ) {
	int				i, j, side;
	dsurface_t		*ds;
	shaderInfo_t	*ls;
	winding_t		*w;
	lFacet_t		*f;
	vlight_t			*dl;
	vec3_t			origin;
	drawVert_t		*dv;
	int				c_surfaceLights;
	float			lightSubdivide;
	vec3_t			normal;


	c_surfaceLights = 0;
	_printf ("Creating surface lights...\n");

	for ( i = 0 ; i < numDrawSurfaces ; i++ ) {
		// see if this surface is light emiting
		ds = &drawSurfaces[i];

		ls = ShaderInfoForShader( dshaders[ ds->shaderNum].shader );
		if ( ls->value == 0 ) {
			continue;
		}

		// determine how much we need to chop up the surface
		if ( ls->lightSubdivide ) {
			lightSubdivide = ls->lightSubdivide;
		} else {
			lightSubdivide = lightDefaultSubdivide;
		}

		c_surfaceLights++;

		// an autosprite shader will become
		// a point light instead of an area light
		if ( ls->autosprite ) {
			// autosprite geometry should only have four vertexes
			if ( lsurfaceTest[i] ) {
				// curve or misc_model
				f = lsurfaceTest[i]->facets;
				if ( lsurfaceTest[i]->numFacets != 1 || f->numpoints != 4 ) {
					_printf( "WARNING: surface at (%i %i %i) has autosprite shader but isn't a quad\n",
						(int)f->points[0], (int)f->points[1], (int)f->points[2] );
				}
				VectorAdd( f->points[0], f->points[1], origin );
				VectorAdd( f->points[2], origin, origin );
				VectorAdd( f->points[3], origin, origin );
				VectorScale( origin, 0.25, origin );
			} else {
				// normal polygon
				dv = &drawVerts[ ds->firstVert ];
				if ( ds->numVerts != 4 ) {
					_printf( "WARNING: surface at (%i %i %i) has autosprite shader but %i verts\n",
						(int)dv->xyz[0], (int)dv->xyz[1], (int)dv->xyz[2] );
					continue;
				}

				VectorAdd( dv[0].xyz, dv[1].xyz, origin );
				VectorAdd( dv[2].xyz, origin, origin );
				VectorAdd( dv[3].xyz, origin, origin );
				VectorScale( origin, 0.25, origin );
			}

			dl = malloc(sizeof(*dl));
			memset (dl, 0, sizeof(*dl));
			VectorCopy( origin, dl->origin );
			VectorCopy( ls->color, dl->color );
			dl->photons = ls->value * lightPointScale;
			dl->type = LIGHT_POINTRADIAL;
			vlights[numvlights++] = dl;
			continue;
		}

		// possibly create for both sides of the polygon
		for ( side = 0 ; side <= ls->twoSided ; side++ ) {
			// create area lights
			if ( lsurfaceTest[i] ) {
				// curve or misc_model
				for ( j = 0 ; j < lsurfaceTest[i]->numFacets ; j++ ) {
					f = lsurfaceTest[i]->facets + j;
					w = AllocWinding( f->numpoints );
					w->numpoints = f->numpoints;
					memcpy( w->points, f->points, f->numpoints * 12 );

					VectorCopy( f->plane.normal, normal );
					if ( side ) {
						winding_t	*t;

						t = w;
						w = ReverseWinding( t );
						FreeWinding( t );
						VectorSubtract( vec3_origin, normal, normal );
					}
					VL_SubdivideAreaLight( ls, w, normal, lightSubdivide, qtrue );
				}
			} else {
				// normal polygon

				w = AllocWinding( ds->numVerts );
				w->numpoints = ds->numVerts;
				for ( j = 0 ; j < ds->numVerts ; j++ ) {
					VectorCopy( drawVerts[ds->firstVert+j].xyz, w->points[j] );
				}
				VectorCopy( ds->lightmapVecs[2], normal );
				if ( side ) {
					winding_t	*t;

					t = w;
					w = ReverseWinding( t );
					FreeWinding( t );
					VectorSubtract( vec3_origin, normal, normal );
				}
				VL_SubdivideAreaLight( ls, w, normal, lightSubdivide, qtrue );
			}
		}
	}

	_printf( "%7i light emitting surfaces\n", c_surfaceLights );
}


/*
==================
VL_WindingForBrushSide
==================
*/
winding_t *VL_WindingForBrushSide(dbrush_t *brush, int side, winding_t *w)
{
	int i, res;
	winding_t *tmpw;
	plane_t plane;

	VectorCopy(dplanes[ dbrushsides[ brush->firstSide + side ].planeNum ].normal, plane.normal);
	VectorInverse(plane.normal);
	plane.dist = -dplanes[ dbrushsides[ brush->firstSide + side ].planeNum ].dist;
	tmpw = BaseWindingForPlane( plane.normal, plane.dist );
	memcpy(w->points, tmpw->points, sizeof(vec3_t) * tmpw->numpoints);
	w->numpoints = tmpw->numpoints;

	for (i = 0; i < brush->numSides; i++)
	{
		if (i == side)
			continue;
		VectorCopy(dplanes[ dbrushsides[ brush->firstSide + i ].planeNum ].normal, plane.normal);
		VectorInverse(plane.normal);
		plane.dist = -dplanes[ dbrushsides[ brush->firstSide + i ].planeNum ].dist;
		res = VL_ChopWinding(w, &plane, 0.1);
		if (res == SIDE_BACK)
			return NULL;
	}
	return w;
}

/*
==================
VL_CreateSkyLights
==================
*/
void VL_CreateSkyLights(void)
{
	int				i, j, c_skyLights;
	dbrush_t		*b;
	shaderInfo_t	*si;
	dbrushside_t	*s;
	vlight_t		*dl;
	vec3_t sunColor, sunDir = { 0.45, 0.3, 0.9 };
	float d;

	VectorNormalize(sunDir, sunDir);
	VectorInverse(sunDir);

	c_skyLights = 0;
	_printf("Creating sky lights...\n");
	// find the sky shader
	for ( i = 0 ; i < numDrawSurfaces ; i++ ) {
		si = ShaderInfoForShader( dshaders[ drawSurfaces[i].shaderNum ].shader );
		if ( si->surfaceFlags & SURF_SKY ) {
			VectorCopy( si->sunLight, sunColor );
			VectorCopy( si->sunDirection, sunDir );
			VectorInverse(sunDir);
			break;
		}
	}

	// find the brushes
	for ( i = 0 ; i < numbrushes ; i++ ) {
		b = &dbrushes[i];
		for ( j = 0 ; j < b->numSides ; j++ ) {
			s = &dbrushsides[ b->firstSide + j ];
			if ( dshaders[ s->shaderNum ].surfaceFlags & SURF_SKY ) {
				//if this surface doesn't face in the same direction as the sun
				d = DotProduct(dplanes[ s->planeNum ].normal, sunDir);
				if (d <= 0)
					continue;
				//
				dl = malloc(sizeof(*dl));
				memset (dl, 0, sizeof(*dl));
				VectorCopy(sunColor, dl->color);
				VectorCopy(sunDir, dl->normal);
				VectorCopy(dplanes[ s->planeNum ].normal, dl->plane);
				dl->plane[3] = dplanes[ s->planeNum ].dist;
				dl->type = LIGHT_SURFACEDIRECTED;
				dl->atten_disttype = LDAT_NOSCALE;
				VL_WindingForBrushSide(b, j, &dl->w);
//				DebugNet_DrawWinding(&dl->w, 2);
				//
				vlights[numvlights++] = dl;
				c_skyLights++;
			}
		}
	}
	_printf("%7i light emitting sky surfaces\n", c_skyLights);
}

/*
==================
VL_SetPortalSphere
==================
*/
void VL_SetPortalSphere (lportal_t *p)
{
	int		i;
	vec3_t	total, dist;
	winding_t	*w;
	float	r, bestr;

	w = p->winding;
	VectorCopy (vec3_origin, total);
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorAdd (total, w->points[i], total);
	}
	
	for (i=0 ; i<3 ; i++)
		total[i] /= w->numpoints;

	bestr = 0;		
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorSubtract (w->points[i], total, dist);
		r = VectorLength (dist);
		if (r > bestr)
			bestr = r;
	}
	VectorCopy (total, p->origin);
	p->radius = bestr;
}

/*
==================
VL_PlaneFromWinding
==================
*/
void VL_PlaneFromWinding (winding_t *w, plane_t *plane)
{
	vec3_t		v1, v2;

	//calc plane
	VectorSubtract (w->points[2], w->points[1], v1);
	VectorSubtract (w->points[0], w->points[1], v2);
	CrossProduct (v2, v1, plane->normal);
	VectorNormalize (plane->normal, plane->normal);
	plane->dist = DotProduct (w->points[0], plane->normal);
}

/*
==================
VL_AllocWinding
==================
*/
winding_t *VL_AllocWinding (int points)
{
	winding_t	*w;
	int			size;
	
	if (points > MAX_POINTS_ON_WINDING)
		Error ("NewWinding: %i points", points);
	
	size = (int)((winding_t *)0)->points[points];
	w = malloc (size);
	memset (w, 0, size);
	
	return w;
}

/*
============
VL_LoadPortals
============
*/
void VL_LoadPortals (char *name)
{
	int			i, j, hint;
	lportal_t	*p;
	lleaf_t		*l;
	char		magic[80];
	FILE		*f;
	int			numpoints;
	winding_t	*w;
	int			leafnums[2];
	plane_t		plane;
	//
	
	if (!strcmp(name,"-"))
		f = stdin;
	else
	{
		f = fopen(name, "r");
		if (!f)
			Error ("LoadPortals: couldn't read %s\n",name);
	}

	if (fscanf (f,"%79s\n%i\n%i\n%i\n",magic, &portalclusters, &numportals, &numfaces) != 4)
		Error ("LoadPortals: failed to read header");
	if (strcmp(magic, PORTALFILE))
		Error ("LoadPortals: not a portal file");

	_printf ("%6i portalclusters\n", portalclusters);
	_printf ("%6i numportals\n", numportals);
	_printf ("%6i numfaces\n", numfaces);

	if (portalclusters >= MAX_CLUSTERS)
		Error ("more than %d clusters in portal file\n", MAX_CLUSTERS);

	// each file portal is split into two memory portals
	portals = malloc(2*numportals*sizeof(lportal_t));
	memset (portals, 0, 2*numportals*sizeof(lportal_t));
	
	leafs = malloc(portalclusters*sizeof(lleaf_t));
	memset (leafs, 0, portalclusters*sizeof(lleaf_t));

	for (i=0, p=portals ; i<numportals ; i++)
	{
		if (fscanf (f, "%i %i %i ", &numpoints, &leafnums[0], &leafnums[1]) != 3)
			Error ("LoadPortals: reading portal %i", i);
		if (numpoints > MAX_POINTS_ON_WINDING)
			Error ("LoadPortals: portal %i has too many points", i);
		if ( (unsigned)leafnums[0] > portalclusters
		|| (unsigned)leafnums[1] > portalclusters)
			Error ("LoadPortals: reading portal %i", i);
		if (fscanf (f, "%i ", &hint) != 1)
			Error ("LoadPortals: reading hint state");
		
		w = p->winding = VL_AllocWinding (numpoints);
		w->numpoints = numpoints;
		
		for (j=0 ; j<numpoints ; j++)
		{
			double	v[3];
			int		k;

			// scanf into double, then assign to vec_t
			// so we don't care what size vec_t is
			if (fscanf (f, "(%lf %lf %lf ) "
			, &v[0], &v[1], &v[2]) != 3)
				Error ("LoadPortals: reading portal %i", i);
			for (k=0 ; k<3 ; k++)
				w->points[j][k] = v[k];
		}
		fscanf (f, "\n");
		
		// calc plane
		VL_PlaneFromWinding (w, &plane);

		// create forward portal
		l = &leafs[leafnums[0]];
		if (l->numportals == MAX_PORTALS_ON_LEAF)
			Error ("Leaf with too many portals");
		l->portals[l->numportals] = p;
		l->numportals++;
		
		p->winding = w;
		VectorSubtract (vec3_origin, plane.normal, p->plane.normal);
		p->plane.dist = -plane.dist;
		p->leaf = leafnums[1];
		VL_SetPortalSphere (p);
		p++;
		
		// create backwards portal
		l = &leafs[leafnums[1]];
		if (l->numportals == MAX_PORTALS_ON_LEAF)
			Error ("Leaf with too many portals");
		l->portals[l->numportals] = p;
		l->numportals++;
		
		p->winding = VL_AllocWinding(w->numpoints);
		p->winding->numpoints = w->numpoints;
		for (j=0 ; j<w->numpoints ; j++)
		{
			VectorCopy (w->points[w->numpoints-1-j], p->winding->points[j]);
		}

		p->plane = plane;
		p->leaf = leafnums[0];
		VL_SetPortalSphere (p);
		p++;

	}
	
	fclose (f);
}

/*
============
VLightMain
============
*/
int VLightMain (int argc, char **argv) {
	int			i;
	double		start, end;
	const char	*value;

	_printf ("----- VLighting ----\n");

	for (i=1 ; i<argc ; i++) {
		if (!strcmp(argv[i],"-v")) {
			verbose = qtrue;
		} else if (!strcmp(argv[i],"-threads")) {
			numthreads = atoi (argv[i+1]);
			_printf("num threads = %d\n", numthreads);
			i++;
		} else if (!strcmp(argv[i],"-area")) {
			lightAreaScale *= atof(argv[i+1]);
			_printf ("area light scaling at %f\n", lightAreaScale);
			i++;
		} else if (!strcmp(argv[i],"-point")) {
			lightPointScale *= atof(argv[i+1]);
			_printf ("point light scaling at %f\n", lightPointScale);
			i++;
		} else if (!strcmp(argv[i], "-samplesize")) {
			samplesize = atoi(argv[i+1]);
			if (samplesize < 1) samplesize = 1;
			i++;
			_printf("lightmap sample size is %dx%d units\n", samplesize, samplesize);
		} else if (!strcmp(argv[i], "-novertex")) {
			novertexlighting = qtrue;
			_printf("no vertex lighting = true\n");
		} else if (!strcmp(argv[i], "-nogrid")) {
			nogridlighting = qtrue;
			_printf("no grid lighting = true\n");
		} else if (!strcmp(argv[i], "-nostitching")) {
			nostitching = qtrue;
			_printf("no stitching = true\n");
		} else if (!strcmp(argv[i], "-noalphashading")) {
			noalphashading = qtrue;
			_printf("no alpha shading = true\n");
		} else if (!strcmp(argv[i], "-nocolorshading")) {
			nocolorshading = qtrue;
			_printf("old style alpha shading = true\n");
		} else if (!strcmp(argv[i], "-nobackfaceculling")) {
			nobackfaceculling = qtrue;
			_printf("no backface culling = true\n");
		} else if (!strcmp(argv[i], "-tracelight")) {
			defaulttracelight = qtrue;
			_printf("default trace light = true\n");
		} else if (!strcmp(argv[i], "-radiosity")) {
			radiosity = atoi(argv[i+1]);
			_printf("radiosity = %d\n", radiosity);
			i++;
		} else {
			break;
		}
	}

	ThreadSetDefault ();

	if (i != argc - 1) {
		_printf("usage: q3map -vlight [-<switch> [-<switch> ...]] <mapname>\n"
				"\n"
				"Switches:\n"
				"   v              = verbose output\n"
				"   threads <X>    = set number of threads to X\n"
				"   area <V>       = set the area light scale to V\n"
				"   point <W>      = set the point light scale to W\n"
				"   novertex       = don't calculate vertex lighting\n"
				"   nogrid         = don't calculate light grid for dynamic model lighting\n"
				"   nostitching    = no polygon stitching before lighting\n"
				"   noalphashading = don't use alpha shading\n"
				"   nocolorshading = don't use color alpha shading\n"
				"   tracelight     = use old light algorithm by default\n"
				"   samplesize <N> = set the lightmap pixel size to NxN units\n");
		exit(0);
	}

	SetQdirFromPath (argv[i]);	

#ifdef _WIN32
	InitPakFile(gamedir, NULL);
#endif

	strcpy (source, ExpandArg(argv[i]));
	StripExtension (source);
	DefaultExtension (source, ".bsp");

	LoadShaderInfo();

	_printf ("reading %s\n", source);

	LoadBSPFile (source);
	ParseEntities();

	value = ValueForKey( &entities[0], "gridsize" );
	if (strlen(value)) {
		sscanf( value, "%f %f %f", &gridSize[0], &gridSize[1], &gridSize[2] );
		_printf("grid size = {%1.1f, %1.1f, %1.1f}\n", gridSize[0], gridSize[1], gridSize[2]);
	}

	CountLightmaps();

	StripExtension (source);
	DefaultExtension (source, ".prt");

	VL_LoadPortals(source);

	// set surfaceOrigin
	SetEntityOrigins();

	// grid and vertex lighting
	GridAndVertexLighting();

#ifdef DEBUGNET
	DebugNet_Setup();
#endif

	start = clock();

	lightFloats = (float *) malloc(numLightBytes * sizeof(float));
	memset(lightFloats, 0, numLightBytes * sizeof(float));

	VL_InitSurfacesForTesting();

	VL_CalcVisibleLightmapPixelArea();

	numvlights = 0;
	VL_CreateEntityLights();
	VL_CreateFakeSurfaceLights();
	VL_CreateSkyLights();

	VL_TestLightLeafs();

	VL_LightWorld();

#ifndef LIGHTPOLYS
	StripExtension (source);
	DefaultExtension (source, ".bsp");
	_printf ("writing %s\n", source);
	WriteBSPFile (source);
#endif

	end = clock();

	_printf ("%5.2f seconds elapsed\n", (end-start) / CLK_TCK);

#ifdef LIGHTPOLYS
	VL_DrawLightWindings();
#endif

#ifdef DEBUGNET
	DebugNet_Shutdown();
#endif
	return 0;
}
