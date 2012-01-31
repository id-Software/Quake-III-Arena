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

#include "qbsp.h"
#include "../botlib/aasfile.h"
#include "aas_file.h"
#include "aas_store.h"
#include "aas_create.h"
#include "aas_cfg.h"


//#define NOTHREEVERTEXFACES

#define STOREPLANESDOUBLE

#define VERTEX_EPSILON			0.1			//NOTE: changed from 0.5
#define DIST_EPSILON			0.05		//NOTE: changed from 0.9
#define NORMAL_EPSILON			0.0001		//NOTE: changed from 0.005
#define INTEGRAL_EPSILON		0.01

#define VERTEX_HASHING
#define VERTEX_HASH_SHIFT		7
#define VERTEX_HASH_SIZE		((MAX_MAP_BOUNDS>>(VERTEX_HASH_SHIFT-1))+1)	//was 64
//
#define PLANE_HASHING
#define PLANE_HASH_SIZE			1024		//must be power of 2
//
#define EDGE_HASHING
#define EDGE_HASH_SIZE			1024		//must be power of 2

aas_t aasworld;

//vertex hash
int *aas_vertexchain;						// the next vertex in a hash chain
int aas_hashverts[VERTEX_HASH_SIZE*VERTEX_HASH_SIZE];	// a vertex number, or 0 for no verts
//plane hash
int *aas_planechain;
int aas_hashplanes[PLANE_HASH_SIZE];
//edge hash
int *aas_edgechain;
int aas_hashedges[EDGE_HASH_SIZE];

int allocatedaasmem = 0;

int groundfacesonly = false;//true;
//
typedef struct max_aas_s
{
	int max_bboxes;
	int max_vertexes;
	int max_planes;
	int max_edges;
	int max_edgeindexsize;
	int max_faces;
	int max_faceindexsize;
	int max_areas;
	int max_areasettings;
	int max_reachabilitysize;
	int max_nodes;
	int max_portals;
	int max_portalindexsize;
	int max_clusters;
} max_aas_t;
//maximums of everything
max_aas_t max_aas;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_CountTmpNodes(tmp_node_t *tmpnode)
{
	if (!tmpnode) return 0;
	return AAS_CountTmpNodes(tmpnode->children[0]) +
				AAS_CountTmpNodes(tmpnode->children[1]) + 1;
} //end of the function AAS_CountTmpNodes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitMaxAAS(void)
{
	int numfaces, numpoints, numareas;
	tmp_face_t *f;
	tmp_area_t *a;

	numpoints = 0;
	numfaces = 0;
	for (f = tmpaasworld.faces; f; f = f->l_next)
	{
		numfaces++;
		if (f->winding) numpoints += f->winding->numpoints;
	} //end for
	//
	numareas = 0;
	for (a = tmpaasworld.areas; a; a = a->l_next)
	{
		numareas++;
	} //end for
	max_aas.max_bboxes = AAS_MAX_BBOXES;
	max_aas.max_vertexes = numpoints + 1;
	max_aas.max_planes = nummapplanes;
	max_aas.max_edges = numpoints + 1;
	max_aas.max_edgeindexsize = (numpoints + 1) * 3;
	max_aas.max_faces = numfaces + 10;
	max_aas.max_faceindexsize = (numfaces + 10) * 2;
	max_aas.max_areas = numareas + 10;
	max_aas.max_areasettings = numareas + 10;
	max_aas.max_reachabilitysize = 0;
	max_aas.max_nodes = AAS_CountTmpNodes(tmpaasworld.nodes) + 10;
	max_aas.max_portals = 0;
	max_aas.max_portalindexsize = 0;
	max_aas.max_clusters = 0;
} //end of the function AAS_InitMaxAAS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AllocMaxAAS(void)
{
	int i;

	AAS_InitMaxAAS();
	//bounding boxes
	aasworld.numbboxes = 0;
	aasworld.bboxes = (aas_bbox_t *) GetClearedMemory(max_aas.max_bboxes * sizeof(aas_bbox_t));
	allocatedaasmem += max_aas.max_bboxes * sizeof(aas_bbox_t);
	//vertexes
	aasworld.numvertexes = 0;
	aasworld.vertexes = (aas_vertex_t *) GetClearedMemory(max_aas.max_vertexes * sizeof(aas_vertex_t));
	allocatedaasmem += max_aas.max_vertexes * sizeof(aas_vertex_t);
	//planes
	aasworld.numplanes = 0;
	aasworld.planes = (aas_plane_t *) GetClearedMemory(max_aas.max_planes * sizeof(aas_plane_t));
	allocatedaasmem += max_aas.max_planes * sizeof(aas_plane_t);
	//edges
	aasworld.numedges = 0;
	aasworld.edges = (aas_edge_t *) GetClearedMemory(max_aas.max_edges * sizeof(aas_edge_t));
	allocatedaasmem += max_aas.max_edges * sizeof(aas_edge_t);
	//edge index
	aasworld.edgeindexsize = 0;
	aasworld.edgeindex = (aas_edgeindex_t *) GetClearedMemory(max_aas.max_edgeindexsize * sizeof(aas_edgeindex_t));
	allocatedaasmem += max_aas.max_edgeindexsize * sizeof(aas_edgeindex_t);
	//faces
	aasworld.numfaces = 0;
	aasworld.faces = (aas_face_t *) GetClearedMemory(max_aas.max_faces * sizeof(aas_face_t));
	allocatedaasmem += max_aas.max_faces * sizeof(aas_face_t);
	//face index
	aasworld.faceindexsize = 0;
	aasworld.faceindex = (aas_faceindex_t *) GetClearedMemory(max_aas.max_faceindexsize * sizeof(aas_faceindex_t));
	allocatedaasmem += max_aas.max_faceindexsize * sizeof(aas_faceindex_t);
	//convex areas
	aasworld.numareas = 0;
	aasworld.areas = (aas_area_t *) GetClearedMemory(max_aas.max_areas * sizeof(aas_area_t));
	allocatedaasmem += max_aas.max_areas * sizeof(aas_area_t);
	//convex area settings
	aasworld.numareasettings = 0;
	aasworld.areasettings = (aas_areasettings_t *) GetClearedMemory(max_aas.max_areasettings * sizeof(aas_areasettings_t));
	allocatedaasmem += max_aas.max_areasettings * sizeof(aas_areasettings_t);
	//reachablity list
	aasworld.reachabilitysize = 0;
	aasworld.reachability = (aas_reachability_t *) GetClearedMemory(max_aas.max_reachabilitysize * sizeof(aas_reachability_t));
	allocatedaasmem += max_aas.max_reachabilitysize * sizeof(aas_reachability_t);
	//nodes of the bsp tree
	aasworld.numnodes = 0;
	aasworld.nodes = (aas_node_t *) GetClearedMemory(max_aas.max_nodes * sizeof(aas_node_t));
	allocatedaasmem += max_aas.max_nodes * sizeof(aas_node_t);
	//cluster portals
	aasworld.numportals = 0;
	aasworld.portals = (aas_portal_t *) GetClearedMemory(max_aas.max_portals * sizeof(aas_portal_t));
	allocatedaasmem += max_aas.max_portals * sizeof(aas_portal_t);
	//cluster portal index
	aasworld.portalindexsize = 0;
	aasworld.portalindex = (aas_portalindex_t *) GetClearedMemory(max_aas.max_portalindexsize * sizeof(aas_portalindex_t));
	allocatedaasmem += max_aas.max_portalindexsize * sizeof(aas_portalindex_t);
	//cluster
	aasworld.numclusters = 0;
	aasworld.clusters = (aas_cluster_t *) GetClearedMemory(max_aas.max_clusters * sizeof(aas_cluster_t));
	allocatedaasmem += max_aas.max_clusters * sizeof(aas_cluster_t);
	//
	Log_Print("allocated ");
	PrintMemorySize(allocatedaasmem);
	Log_Print(" of AAS memory\n");
	//reset the has stuff
	aas_vertexchain = (int *) GetClearedMemory(max_aas.max_vertexes * sizeof(int));
	aas_planechain = (int *) GetClearedMemory(max_aas.max_planes * sizeof(int));
	aas_edgechain = (int *) GetClearedMemory(max_aas.max_edges * sizeof(int));
	//
	for (i = 0; i < max_aas.max_vertexes; i++) aas_vertexchain[i] = -1;
	for (i = 0; i < VERTEX_HASH_SIZE * VERTEX_HASH_SIZE; i++) aas_hashverts[i] = -1;
	//
	for (i = 0; i < max_aas.max_planes; i++) aas_planechain[i] = -1;
	for (i = 0; i < PLANE_HASH_SIZE; i++) aas_hashplanes[i] = -1;
	//
	for (i = 0; i < max_aas.max_edges; i++) aas_edgechain[i] = -1;
	for (i = 0; i < EDGE_HASH_SIZE; i++) aas_hashedges[i] = -1;
} //end of the function AAS_AllocMaxAAS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeMaxAAS(void)
{
	//bounding boxes
	if (aasworld.bboxes) FreeMemory(aasworld.bboxes);
	aasworld.bboxes = NULL;
	aasworld.numbboxes = 0;
	//vertexes
	if (aasworld.vertexes) FreeMemory(aasworld.vertexes);
	aasworld.vertexes = NULL;
	aasworld.numvertexes = 0;
	//planes
	if (aasworld.planes) FreeMemory(aasworld.planes);
	aasworld.planes = NULL;
	aasworld.numplanes = 0;
	//edges
	if (aasworld.edges) FreeMemory(aasworld.edges);
	aasworld.edges = NULL;
	aasworld.numedges = 0;
	//edge index
	if (aasworld.edgeindex) FreeMemory(aasworld.edgeindex);
	aasworld.edgeindex = NULL;
	aasworld.edgeindexsize = 0;
	//faces
	if (aasworld.faces) FreeMemory(aasworld.faces);
	aasworld.faces = NULL;
	aasworld.numfaces = 0;
	//face index
	if (aasworld.faceindex) FreeMemory(aasworld.faceindex);
	aasworld.faceindex = NULL;
	aasworld.faceindexsize = 0;
	//convex areas
	if (aasworld.areas) FreeMemory(aasworld.areas);
	aasworld.areas = NULL;
	aasworld.numareas = 0;
	//convex area settings
	if (aasworld.areasettings) FreeMemory(aasworld.areasettings);
	aasworld.areasettings = NULL;
	aasworld.numareasettings = 0;
	//reachablity list
	if (aasworld.reachability) FreeMemory(aasworld.reachability);
	aasworld.reachability = NULL;
	aasworld.reachabilitysize = 0;
	//nodes of the bsp tree
	if (aasworld.nodes) FreeMemory(aasworld.nodes);
	aasworld.nodes = NULL;
	aasworld.numnodes = 0;
	//cluster portals
	if (aasworld.portals) FreeMemory(aasworld.portals);
	aasworld.portals = NULL;
	aasworld.numportals = 0;
	//cluster portal index
	if (aasworld.portalindex) FreeMemory(aasworld.portalindex);
	aasworld.portalindex = NULL;
	aasworld.portalindexsize = 0;
	//clusters
	if (aasworld.clusters) FreeMemory(aasworld.clusters);
	aasworld.clusters = NULL;
	aasworld.numclusters = 0;
	
	Log_Print("freed ");
	PrintMemorySize(allocatedaasmem);
	Log_Print(" of AAS memory\n");
	allocatedaasmem = 0;
	//
	if (aas_vertexchain) FreeMemory(aas_vertexchain);
	aas_vertexchain = NULL;
	if (aas_planechain) FreeMemory(aas_planechain);
	aas_planechain = NULL;
	if (aas_edgechain) FreeMemory(aas_edgechain);
	aas_edgechain = NULL;
} //end of the function AAS_FreeMaxAAS
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
unsigned AAS_HashVec(vec3_t vec)
{
	int x, y;

	x = (MAX_MAP_BOUNDS + (int)(vec[0]+0.5)) >> VERTEX_HASH_SHIFT;
	y = (MAX_MAP_BOUNDS + (int)(vec[1]+0.5)) >> VERTEX_HASH_SHIFT;

	if (x < 0 || x >= VERTEX_HASH_SIZE || y < 0 || y >= VERTEX_HASH_SIZE)
	{
		Log_Print("WARNING! HashVec: point %f %f %f outside valid range\n", vec[0], vec[1], vec[2]);
		Log_Print("This should never happen!\n");
		return -1;
	} //end if
	
	return y*VERTEX_HASH_SIZE + x;
} //end of the function AAS_HashVec
//===========================================================================
// returns true if the vertex was found in the list
// stores the vertex number in *vnum
// stores a new vertex if not stored already
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_GetVertex(vec3_t v, int *vnum)
{
	int i;
#ifndef VERTEX_HASHING
	float diff;
#endif //VERTEX_HASHING

#ifdef VERTEX_HASHING
	int h, vn;
	vec3_t vert;
	
	for (i = 0; i < 3; i++)
	{
		if ( fabs(v[i] - Q_rint(v[i])) < INTEGRAL_EPSILON)
			vert[i] = Q_rint(v[i]);
		else
			vert[i] = v[i];
	} //end for

	h = AAS_HashVec(vert);
	//if the vertex was outside the valid range
	if (h == -1)
	{
		*vnum = -1;
		return true;
	} //end if

	for (vn = aas_hashverts[h]; vn >= 0; vn = aas_vertexchain[vn])
	{
		if (fabs(aasworld.vertexes[vn][0] - vert[0]) < VERTEX_EPSILON
				&& fabs(aasworld.vertexes[vn][1] - vert[1]) < VERTEX_EPSILON
				&& fabs(aasworld.vertexes[vn][2] - vert[2]) < VERTEX_EPSILON)
		{
			*vnum = vn;
			return true;
		} //end if
	} //end for
#else //VERTEX_HASHING
	//check if the vertex is already stored
	//stupid linear search
	for (i = 0; i < aasworld.numvertexes; i++)
	{
		diff = vert[0] - aasworld.vertexes[i][0];
		if (diff < VERTEX_EPSILON && diff > -VERTEX_EPSILON)
		{
			diff = vert[1] - aasworld.vertexes[i][1];
			if (diff < VERTEX_EPSILON && diff > -VERTEX_EPSILON)
			{
				diff = vert[2] - aasworld.vertexes[i][2];
				if (diff < VERTEX_EPSILON && diff > -VERTEX_EPSILON)
				{
					*vnum = i;
					return true;
				} //end if
			} //end if
		} //end if
	} //end for
#endif //VERTEX_HASHING

	if (aasworld.numvertexes >= max_aas.max_vertexes)
	{
		Error("AAS_MAX_VERTEXES = %d", max_aas.max_vertexes);
	} //end if
	VectorCopy(vert, aasworld.vertexes[aasworld.numvertexes]);
	*vnum = aasworld.numvertexes;

#ifdef VERTEX_HASHING
	aas_vertexchain[aasworld.numvertexes] = aas_hashverts[h];
	aas_hashverts[h] = aasworld.numvertexes;
#endif //VERTEX_HASHING

	aasworld.numvertexes++;
	return false;
} //end of the function AAS_GetVertex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
unsigned AAS_HashEdge(int v1, int v2)
{
	int vnum1, vnum2;
	//
	if (v1 < v2)
	{
		vnum1 = v1;
		vnum2 = v2;
	} //end if
	else
	{
		vnum1 = v2;
		vnum2 = v1;
	} //end else
	return (vnum1 + vnum2) & (EDGE_HASH_SIZE-1);
} //end of the function AAS_HashVec
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AddEdgeToHash(int edgenum)
{
	int hash;
	aas_edge_t *edge;

	edge = &aasworld.edges[edgenum];

	hash = AAS_HashEdge(edge->v[0], edge->v[1]);

	aas_edgechain[edgenum] = aas_hashedges[hash];
	aas_hashedges[hash] = edgenum;
} //end of the function AAS_AddEdgeToHash
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_FindHashedEdge(int v1num, int v2num, int *edgenum)
{
	int e, hash;
	aas_edge_t *edge;

	hash = AAS_HashEdge(v1num, v2num);
	for (e = aas_hashedges[hash]; e >= 0; e = aas_edgechain[e])
	{
		edge = &aasworld.edges[e];
		if (edge->v[0] == v1num)
		{
			if (edge->v[1] == v2num)
			{
				*edgenum = e;
				return true;
			} //end if
		} //end if
		else if (edge->v[1] == v1num)
		{
			if (edge->v[0] == v2num)
			{
				//negative for a reversed edge
				*edgenum = -e;
				return true;
			} //end if
		} //end else
	} //end for
	return false;
} //end of the function AAS_FindHashedPlane
//===========================================================================
// returns true if the edge was found
// stores the edge number in *edgenum (negative if reversed edge)
// stores new edge if not stored already
// returns zero when the edge is degenerate
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_GetEdge(vec3_t v1, vec3_t v2, int *edgenum)
{
	int v1num, v2num;
	qboolean found;

	//the first edge is a dummy
	if (aasworld.numedges == 0) aasworld.numedges = 1;

	found = AAS_GetVertex(v1, &v1num);
	found &= AAS_GetVertex(v2, &v2num);
	//if one of the vertexes was outside the valid range
	if (v1num == -1 || v2num == -1)
	{
		*edgenum = 0;
		return true;
	} //end if
	//if both vertexes are the same or snapped onto each other
	if (v1num == v2num)
	{
		*edgenum = 0;
		return true;
	} //end if
	//if both vertexes where already stored
	if (found)
	{
#ifdef EDGE_HASHING
		if (AAS_FindHashedEdge(v1num, v2num, edgenum)) return true;
#else
		int i;
		for (i = 1; i < aasworld.numedges; i++)
		{
			if (aasworld.edges[i].v[0] == v1num)
			{
				if (aasworld.edges[i].v[1] == v2num)
				{
					*edgenum = i;
					return true;
				} //end if
			} //end if
			else if (aasworld.edges[i].v[1] == v1num)
			{
				if (aasworld.edges[i].v[0] == v2num)
				{
					//negative for a reversed edge
					*edgenum = -i;
					return true;
				} //end if
			} //end else
		} //end for
#endif //EDGE_HASHING
	} //end if
	if (aasworld.numedges >= max_aas.max_edges)
	{
		Error("AAS_MAX_EDGES = %d", max_aas.max_edges);
	} //end if
	aasworld.edges[aasworld.numedges].v[0] = v1num;
	aasworld.edges[aasworld.numedges].v[1] = v2num;
	*edgenum = aasworld.numedges;
#ifdef EDGE_HASHING
	AAS_AddEdgeToHash(*edgenum);
#endif //EDGE_HASHING
	aasworld.numedges++;
	return false;
} //end of the function AAS_GetEdge
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PlaneTypeForNormal(vec3_t normal)
{
	vec_t	ax, ay, az;
	
	//NOTE: epsilon used
	if (	(normal[0] >= 1.0 -NORMAL_EPSILON) ||
			(normal[0] <= -1.0 + NORMAL_EPSILON)) return PLANE_X;
	if (	(normal[1] >= 1.0 -NORMAL_EPSILON) ||
			(normal[1] <= -1.0 + NORMAL_EPSILON)) return PLANE_Y;
	if (	(normal[2] >= 1.0 -NORMAL_EPSILON) ||
			(normal[2] <= -1.0 + NORMAL_EPSILON)) return PLANE_Z;
		
	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);
	
	if (ax >= ay && ax >= az) return PLANE_ANYX;
	if (ay >= ax && ay >= az) return PLANE_ANYY;
	return PLANE_ANYZ;
} //end of the function AAS_PlaneTypeForNormal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AddPlaneToHash(int planenum)
{
	int hash;
	aas_plane_t *plane;

	plane = &aasworld.planes[planenum];

	hash = (int)fabs(plane->dist) / 8;
	hash &= (PLANE_HASH_SIZE-1);

	aas_planechain[planenum] = aas_hashplanes[hash];
	aas_hashplanes[hash] = planenum;
} //end of the function AAS_AddPlaneToHash
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PlaneEqual(vec3_t normal, float dist, int planenum)
{
	float diff;

	diff = dist - aasworld.planes[planenum].dist;
	if (diff > -DIST_EPSILON && diff < DIST_EPSILON)
	{
		diff = normal[0] - aasworld.planes[planenum].normal[0];
		if (diff > -NORMAL_EPSILON && diff < NORMAL_EPSILON)
		{
			diff = normal[1] - aasworld.planes[planenum].normal[1];
			if (diff > -NORMAL_EPSILON && diff < NORMAL_EPSILON)
			{
				diff = normal[2] - aasworld.planes[planenum].normal[2];
				if (diff > -NORMAL_EPSILON && diff < NORMAL_EPSILON)
				{
					return true;
				} //end if
			} //end if
		} //end if
	} //end if
	return false;
} //end of the function AAS_PlaneEqual
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_FindPlane(vec3_t normal, float dist, int *planenum)
{
	int i;

	for (i = 0; i < aasworld.numplanes; i++)
	{
		if (AAS_PlaneEqual(normal, dist, i))
		{
			*planenum = i;
			return true;
		} //end if
	} //end for
	return false;
} //end of the function AAS_FindPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_FindHashedPlane(vec3_t normal, float dist, int *planenum)
{
	int i, p;
	aas_plane_t *plane;
	int hash, h;

	hash = (int)fabs(dist) / 8;
	hash &= (PLANE_HASH_SIZE-1);

	//search the border bins as well
	for (i = -1; i <= 1; i++)
	{
		h = (hash+i)&(PLANE_HASH_SIZE-1);
		for (p = aas_hashplanes[h]; p >= 0; p = aas_planechain[p])
		{
			plane = &aasworld.planes[p];
			if (AAS_PlaneEqual(normal, dist, p))
			{
				*planenum = p;
				return true;
			} //end if
		} //end for
	} //end for
	return false;
} //end of the function AAS_FindHashedPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_GetPlane(vec3_t normal, vec_t dist, int *planenum)
{
	aas_plane_t *plane, temp;

	//if (AAS_FindPlane(normal, dist, planenum)) return true;
	if (AAS_FindHashedPlane(normal, dist, planenum)) return true;

	if (aasworld.numplanes >= max_aas.max_planes-1)
	{
		Error("AAS_MAX_PLANES = %d", max_aas.max_planes);
	} //end if

#ifdef STOREPLANESDOUBLE
	plane = &aasworld.planes[aasworld.numplanes];
	VectorCopy(normal, plane->normal);
	plane->dist = dist;
	plane->type = (plane+1)->type = PlaneTypeForNormal(plane->normal);

	VectorCopy(normal, (plane+1)->normal);
	VectorNegate((plane+1)->normal, (plane+1)->normal);
	(plane+1)->dist = -dist;

	aasworld.numplanes += 2;

	//allways put axial planes facing positive first
	if (plane->type < 3)
	{
		if (plane->normal[0] < 0 || plane->normal[1] < 0 || plane->normal[2] < 0)
		{
			// flip order
			temp = *plane;
			*plane = *(plane+1);
			*(plane+1) = temp;
			*planenum = aasworld.numplanes - 1;
			return false;
		} //end if
	} //end if
	*planenum = aasworld.numplanes - 2;
	//add the planes to the hash
	AAS_AddPlaneToHash(aasworld.numplanes - 1);
	AAS_AddPlaneToHash(aasworld.numplanes - 2);
	return false;
#else
	plane = &aasworld.planes[aasworld.numplanes];
	VectorCopy(normal, plane->normal);
	plane->dist = dist;
	plane->type = AAS_PlaneTypeForNormal(normal);

	*planenum = aasworld.numplanes;
	aasworld.numplanes++;
	//add the plane to the hash
	AAS_AddPlaneToHash(aasworld.numplanes - 1);
	return false;
#endif //STOREPLANESDOUBLE
} //end of the function AAS_GetPlane
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_GetFace(winding_t *w, plane_t *p, int side, int *facenum)
{
	int edgenum, i, j;
	aas_face_t *face;

	//face zero is a dummy, because of the face index with negative numbers
	if (aasworld.numfaces == 0) aasworld.numfaces = 1;

	if (aasworld.numfaces >= max_aas.max_faces)
	{
		Error("AAS_MAX_FACES = %d", max_aas.max_faces);
	} //end if
	face = &aasworld.faces[aasworld.numfaces];
	AAS_GetPlane(p->normal, p->dist, &face->planenum);
	face->faceflags = 0;
	face->firstedge = aasworld.edgeindexsize;
	face->frontarea = 0;
	face->backarea = 0;
	face->numedges = 0;
	for (i = 0; i < w->numpoints; i++)
	{
		if (aasworld.edgeindexsize >= max_aas.max_edgeindexsize)
		{
			Error("AAS_MAX_EDGEINDEXSIZE = %d", max_aas.max_edgeindexsize);
		} //end if
		j = (i+1) % w->numpoints;
		AAS_GetEdge(w->p[i], w->p[j], &edgenum);
		//if the edge wasn't degenerate
		if (edgenum)
		{
			aasworld.edgeindex[aasworld.edgeindexsize++] = edgenum;
			face->numedges++;
		} //end if
		else if (verbose)
		{
			Log_Write("AAS_GetFace: face %d had degenerate edge %d-%d\r\n",
														aasworld.numfaces, i, j);
		} //end else
	} //end for
	if (face->numedges < 1
#ifdef NOTHREEVERTEXFACES
		|| face->numedges < 3
#endif //NOTHREEVERTEXFACES
		)
	{
		memset(&aasworld.faces[aasworld.numfaces], 0, sizeof(aas_face_t));
		Log_Write("AAS_GetFace: face %d was tiny\r\n", aasworld.numfaces);
		return false;
	} //end if
	*facenum = aasworld.numfaces;
	aasworld.numfaces++;
	return true;
} //end of the function AAS_GetFace
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
/*
qboolean AAS_GetFace(winding_t *w, plane_t *p, int side, int *facenum)
{
	aas_edgeindex_t edges[1024];
	int planenum, numedges, i;
	int j, edgenum;
	qboolean foundplane, foundedges;
	aas_face_t *face;

	//face zero is a dummy, because of the face index with negative numbers
	if (aasworld.numfaces == 0) aasworld.numfaces = 1;

	foundplane = AAS_GetPlane(p->normal, p->dist, &planenum);

	foundedges = true;
	numedges = w->numpoints;
	for (i = 0; i < w->numpoints; i++)
	{
		if (i >= 1024) Error("AAS_GetFace: more than %d edges\n", 1024);
		foundedges &= AAS_GetEdge(w->p[i], w->p[(i+1 >= w->numpoints ? 0 : i+1)], &edges[i]);
	} //end for

	//FIXME: use portal number instead of a search
	//if the plane and all edges already existed
	if (foundplane && foundedges)
	{
		for (i = 0; i < aasworld.numfaces; i++)
		{
			face = &aasworld.faces[i];
			if (planenum == face->planenum)
			{
				if (numedges == face->numedges)
				{
					for (j = 0; j < numedges; j++)
					{
						edgenum = abs(aasworld.edgeindex[face->firstedge + j]);
						if (abs(edges[i]) != edgenum) break;
					} //end for
					if (j == numedges)
					{
						//jippy found the face
						*facenum = -i;
						return true;
					} //end if
				} //end if
			} //end if
		} //end for
	} //end if
	if (aasworld.numfaces >= max_aas.max_faces)
	{
		Error("AAS_MAX_FACES = %d", max_aas.max_faces);
	} //end if
	face = &aasworld.faces[aasworld.numfaces];
	face->planenum = planenum;
	face->faceflags = 0;
	face->numedges = numedges;
	face->firstedge = aasworld.edgeindexsize;
	face->frontarea = 0;
	face->backarea = 0;
	for (i = 0; i < numedges; i++)
	{
		if (aasworld.edgeindexsize >= max_aas.max_edgeindexsize)
		{
			Error("AAS_MAX_EDGEINDEXSIZE = %d", max_aas.max_edgeindexsize);
		} //end if
		aasworld.edgeindex[aasworld.edgeindexsize++] = edges[i];
	} //end for
	*facenum = aasworld.numfaces;
	aasworld.numfaces++;
	return false;
} //end of the function AAS_GetFace*/
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_StoreAreaSettings(tmp_areasettings_t *tmpareasettings)
{
	aas_areasettings_t *areasettings;

	if (aasworld.numareasettings == 0) aasworld.numareasettings = 1;
	areasettings = &aasworld.areasettings[aasworld.numareasettings++];
	areasettings->areaflags = tmpareasettings->areaflags;
	areasettings->presencetype = tmpareasettings->presencetype;
	areasettings->contents = tmpareasettings->contents;
	if (tmpareasettings->modelnum > AREACONTENTS_MAXMODELNUM)
		Log_Print("WARNING: more than %d mover models\n", AREACONTENTS_MAXMODELNUM);
	areasettings->contents |= (tmpareasettings->modelnum & AREACONTENTS_MAXMODELNUM) << AREACONTENTS_MODELNUMSHIFT;
} //end of the function AAS_StoreAreaSettings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_StoreArea(tmp_area_t *tmparea)
{
	int side, edgenum, i;
	plane_t *plane;
	tmp_face_t *tmpface;
	aas_area_t *aasarea;
	aas_edge_t *edge;
	aas_face_t *aasface;
	aas_faceindex_t aasfacenum;
	vec3_t facecenter;
	winding_t *w;

	//when the area is merged go to the merged area
	//FIXME: this isn't necessary anymore because the tree
	//			is refreshed after area merging
	while(tmparea->mergedarea) tmparea = tmparea->mergedarea;
	//
	if (tmparea->invalid) Error("AAS_StoreArea: tried to store invalid area");
	//if there is an aas area already stored for this tmp area
	if (tmparea->aasareanum) return -tmparea->aasareanum;
	//
	if (aasworld.numareas >= max_aas.max_areas)
	{
		Error("AAS_MAX_AREAS = %d", max_aas.max_areas);
	} //end if
	//area zero is a dummy
	if (aasworld.numareas == 0) aasworld.numareas = 1;
	//create an area from this leaf
	aasarea = &aasworld.areas[aasworld.numareas];
	aasarea->areanum = aasworld.numareas;
	aasarea->numfaces = 0;
	aasarea->firstface = aasworld.faceindexsize;
	ClearBounds(aasarea->mins, aasarea->maxs);
	VectorClear(aasarea->center);
	//
//	Log_Write("tmparea %d became aasarea %d\r\n", tmparea->areanum, aasarea->areanum);
	//store the aas area number at the tmp area
	tmparea->aasareanum = aasarea->areanum;
	//
	for (tmpface = tmparea->tmpfaces; tmpface; tmpface = tmpface->next[side])
	{
		side = tmpface->frontarea != tmparea;
		//if there's an aas face created for the tmp face already
		if (tmpface->aasfacenum)
		{
			//we're at the back of the face so use a negative index
			aasfacenum = -tmpface->aasfacenum;
#ifdef DEBUG
			if (tmpface->aasfacenum < 0 || tmpface->aasfacenum > max_aas.max_faces)
			{
				Error("AAS_CreateTree_r: face number out of range");
			} //end if
#endif //DEBUG
			aasface = &aasworld.faces[tmpface->aasfacenum];
			aasface->backarea = aasarea->areanum;
		} //end if
		else
		{
			plane = &mapplanes[tmpface->planenum ^ side];
			if (side)
			{
				w = tmpface->winding;
				tmpface->winding = ReverseWinding(tmpface->winding);
			} //end if
			if (!AAS_GetFace(tmpface->winding, plane, 0, &aasfacenum)) continue;
			if (side)
			{
				FreeWinding(tmpface->winding);
				tmpface->winding = w;
			} //end if
			aasface = &aasworld.faces[aasfacenum];
			aasface->frontarea = aasarea->areanum;
			aasface->backarea = 0;
			aasface->faceflags = tmpface->faceflags;
			//set the face number at the tmp face
			tmpface->aasfacenum = aasfacenum;
		} //end else
		//add face points to the area bounds and
		//calculate the face 'center'
		VectorClear(facecenter);
		for (edgenum = 0; edgenum < aasface->numedges; edgenum++)
		{
			edge = &aasworld.edges[abs(aasworld.edgeindex[aasface->firstedge + edgenum])];
			for (i = 0; i < 2; i++)
			{
				AddPointToBounds(aasworld.vertexes[edge->v[i]], aasarea->mins, aasarea->maxs);
				VectorAdd(aasworld.vertexes[edge->v[i]], facecenter, facecenter);
			} //end for
		} //end for
		VectorScale(facecenter, 1.0 / (aasface->numedges * 2.0), facecenter);
		//add the face 'center' to the area 'center'
		VectorAdd(aasarea->center, facecenter, aasarea->center);
		//
		if (aasworld.faceindexsize >= max_aas.max_faceindexsize)
		{
			Error("AAS_MAX_FACEINDEXSIZE = %d", max_aas.max_faceindexsize);
		} //end if
		aasworld.faceindex[aasworld.faceindexsize++] = aasfacenum;
		aasarea->numfaces++;
	} //end for
	//if the area has no faces at all (return 0, = solid leaf)
	if (!aasarea->numfaces) return 0;
	//
	VectorScale(aasarea->center, 1.0 / aasarea->numfaces, aasarea->center);
	//Log_Write("area %d center %f %f %f\r\n", aasworld.numareas,
	//				aasarea->center[0], aasarea->center[1], aasarea->center[2]);
	//store the area settings
	AAS_StoreAreaSettings(tmparea->settings);
	//
	//Log_Write("tmp area %d became aas area %d\r\n", tmpareanum, aasarea->areanum);
	qprintf("\r%6d", aasarea->areanum);
	//
	aasworld.numareas++;
	return -(aasworld.numareas - 1);
} //end of the function AAS_StoreArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_StoreTree_r(tmp_node_t *tmpnode)
{
	int aasnodenum;
	plane_t *plane;
	aas_node_t *aasnode;

	//if it is a solid leaf
	if (!tmpnode) return 0;
	//negative so it's an area
	if (tmpnode->tmparea) return AAS_StoreArea(tmpnode->tmparea);
	//it's another node
	//the first node is a dummy
	if (aasworld.numnodes == 0) aasworld.numnodes = 1;
	if (aasworld.numnodes >= max_aas.max_nodes)
	{
		Error("AAS_MAX_NODES = %d", max_aas.max_nodes);
	} //end if
	aasnodenum = aasworld.numnodes;
	aasnode = &aasworld.nodes[aasworld.numnodes++];
	plane = &mapplanes[tmpnode->planenum];
	AAS_GetPlane(plane->normal, plane->dist, &aasnode->planenum);
	aasnode->children[0] = AAS_StoreTree_r(tmpnode->children[0]);
	aasnode->children[1] = AAS_StoreTree_r(tmpnode->children[1]);
	return aasnodenum;
} //end of the function AAS_StoreTree_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_StoreBoundingBoxes(void)
{
	if (cfg.numbboxes > max_aas.max_bboxes)
	{
		Error("more than %d bounding boxes", max_aas.max_bboxes);
	} //end if
	aasworld.numbboxes = cfg.numbboxes;
	memcpy(aasworld.bboxes, cfg.bboxes, cfg.numbboxes * sizeof(aas_bbox_t));
} //end of the function AAS_StoreBoundingBoxes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_StoreFile(char *filename)
{
	AAS_AllocMaxAAS();

	Log_Write("AAS_StoreFile\r\n");
	//
	AAS_StoreBoundingBoxes();
	//
	qprintf("%6d areas stored", 0);
	//start with node 1 because node zero is a dummy
	AAS_StoreTree_r(tmpaasworld.nodes);
	qprintf("\n");
	Log_Write("%6d areas stored\r\n", aasworld.numareas);
	aasworld.loaded = true;
} //end of the function AAS_StoreFile
