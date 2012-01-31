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
#include "l_mem.h"
#include "../botlib/aasfile.h"
#include "aas_store.h"
#include "aas_cfg.h"
#include "aas_file.h"

//
// creating tetrahedrons from a arbitrary world bounded by triangles
//
// a triangle has 3 corners and 3 edges
// a tetrahedron is build out of 4 triangles
// a tetrahedron has 6 edges
// we start with a world bounded by triangles, a side of a triangle facing
// towards the oudside of the world is marked as part of tetrahedron -1
//
// a tetrahedron is defined by two non-coplanar triangles with a shared edge
//
// a tetrahedron is defined by one triangle and a vertex not in the triangle plane
//
// if all triangles using a specific vertex have tetrahedrons
// at both sides then this vertex will never be part of a new tetrahedron
//
// if all triangles using a specific edge have tetrahedrons
// at both sides then this vertex will never be part of a new tetrahedron
//
// each triangle can only be shared by two tetrahedrons
// when all triangles have tetrahedrons at both sides then we're done
//
// if we cannot create any new tetrahedrons and there is at least one triangle
// which has a tetrahedron only at one side then the world leaks
//

#define Sign(x)		(x < 0 ? 1 : 0)

#define MAX_TH_VERTEXES			128000
#define MAX_TH_PLANES			128000
#define MAX_TH_EDGES			512000
#define MAX_TH_TRIANGLES		51200
#define MAX_TH_TETRAHEDRONS		12800

#define PLANEHASH_SIZE			1024
#define EDGEHASH_SIZE			1024
#define TRIANGLEHASH_SIZE		1024
#define VERTEXHASH_SHIFT		7
#define VERTEXHASH_SIZE			((MAX_MAP_BOUNDS>>(VERTEXHASH_SHIFT-1))+1)	//was 64

#define	NORMAL_EPSILON			0.0001
#define DIST_EPSILON			0.1
#define VERTEX_EPSILON			0.01
#define INTEGRAL_EPSILON		0.01


//plane
typedef struct th_plane_s
{
	vec3_t normal;
	float dist;
	int type;
	int signbits;
	struct th_plane_s *hashnext;		//next plane in hash
} th_plane_t;

//vertex
typedef struct th_vertex_s
{
	vec3_t v;
	int usercount;						//2x the number of to be processed
										//triangles using this vertex
	struct th_vertex_s *hashnext;		//next vertex in hash
} th_vertex_t;

//edge
typedef struct th_edge_s
{
	int v[2];							//vertex indexes
	int usercount;						//number of to be processed
										//triangles using this edge
	struct th_edge_s *hashnext;			//next edge in hash
} th_edge_t;

//triangle
typedef struct th_triangle_s
{
	int edges[3];						//negative if edge is flipped
	th_plane_t planes[3];				//triangle bounding planes
	int planenum;						//plane the triangle is in
	int front;							//tetrahedron at the front
	int back;							//tetrahedron at the back
	vec3_t mins, maxs;					//triangle bounding box
	struct th_triangle_s *prev, *next;	//links in linked triangle lists
	struct th_triangle_s *hashnext;		//next triangle in hash
} th_triangle_t;

//tetrahedron
typedef struct th_tetrahedron_s
{
	int triangles[4];					//negative if at backside of triangle
	float volume;						//tetrahedron volume
} th_tetrahedron_t;

typedef struct th_s
{
	//vertexes
	int numvertexes;
	th_vertex_t *vertexes;
	th_vertex_t *vertexhash[VERTEXHASH_SIZE * VERTEXHASH_SIZE];
	//planes
	int numplanes;
	th_plane_t *planes;
	th_plane_t *planehash[PLANEHASH_SIZE];
	//edges
	int numedges;
	th_edge_t *edges;
	th_edge_t *edgehash[EDGEHASH_SIZE];
	//triangles
	int numtriangles;
	th_triangle_t *triangles;
	th_triangle_t *trianglehash[TRIANGLEHASH_SIZE];
	//tetrahedrons
	int numtetrahedrons;
	th_tetrahedron_t *tetrahedrons;
} th_t;

th_t thworld;

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_InitMaxTH(void)
{
	//get memory for the tetrahedron data
	thworld.vertexes = (th_vertex_t *) GetClearedMemory(MAX_TH_VERTEXES * sizeof(th_vertex_t));
	thworld.planes = (th_plane_t *) GetClearedMemory(MAX_TH_PLANES * sizeof(th_plane_t));
	thworld.edges = (th_edge_t *) GetClearedMemory(MAX_TH_EDGES * sizeof(th_edge_t));
	thworld.triangles = (th_triangle_t *) GetClearedMemory(MAX_TH_TRIANGLES * sizeof(th_triangle_t));
	thworld.tetrahedrons = (th_tetrahedron_t *) GetClearedMemory(MAX_TH_TETRAHEDRONS * sizeof(th_tetrahedron_t));
	//reset the hash tables
	memset(thworld.vertexhash, 0, VERTEXHASH_SIZE * sizeof(th_vertex_t *));
	memset(thworld.planehash, 0, PLANEHASH_SIZE * sizeof(th_plane_t *));
	memset(thworld.edgehash, 0, EDGEHASH_SIZE * sizeof(th_edge_t *));
	memset(thworld.trianglehash, 0, TRIANGLEHASH_SIZE * sizeof(th_triangle_t *));
} //end of the function TH_InitMaxTH
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_FreeMaxTH(void)
{
	if (thworld.vertexes) FreeMemory(thworld.vertexes);
	thworld.vertexes = NULL;
	thworld.numvertexes = 0;
	if (thworld.planes) FreeMemory(thworld.planes);
	thworld.planes = NULL;
	thworld.numplanes = 0;
	if (thworld.edges) FreeMemory(thworld.edges);
	thworld.edges = NULL;
	thworld.numedges = 0;
	if (thworld.triangles) FreeMemory(thworld.triangles);
	thworld.triangles = NULL;
	thworld.numtriangles = 0;
	if (thworld.tetrahedrons) FreeMemory(thworld.tetrahedrons);
	thworld.tetrahedrons = NULL;
	thworld.numtetrahedrons = 0;
} //end of the function TH_FreeMaxTH
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float TH_TriangleArea(th_triangle_t *tri)
{
	return 0;
} //end of the function TH_TriangleArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float TH_TetrahedronVolume(th_tetrahedron_t *tetrahedron)
{
	int edgenum, verts[3], i, j, v2;
	float volume, d;
	th_triangle_t *tri, *tri2;
	th_plane_t *plane;

	tri = &thworld.triangles[abs(tetrahedron->triangles[0])];
	for (i = 0; i < 3; i++)
	{
		edgenum = tri->edges[i];
		if (edgenum < 0) verts[i] = thworld.edges[abs(edgenum)].v[1];
		else verts[i] = thworld.edges[edgenum].v[0];
	} //end for
	//
	tri2 = &thworld.triangles[abs(tetrahedron->triangles[1])];
	for (j = 0; j < 3; j++)
	{
		edgenum = tri2->edges[i];
		if (edgenum < 0) v2 = thworld.edges[abs(edgenum)].v[1];
		else v2 = thworld.edges[edgenum].v[0];
		if (v2 != verts[0] &&
			v2 != verts[1] &&
			v2 != verts[2]) break;
	} //end for

	plane = &thworld.planes[tri->planenum];
	d = -(DotProduct (thworld.vertexes[v2].v, plane->normal) - plane->dist);
	volume = TH_TriangleArea(tri) * d / 3;
	return volume;
} //end of the function TH_TetrahedronVolume
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_PlaneSignBits(vec3_t normal)
{
	int i, signbits;

	signbits = 0;
	for (i = 2; i >= 0; i--)
	{
		signbits = (signbits << 1) + Sign(normal[i]);
	} //end for
	return signbits;
} //end of the function TH_PlaneSignBits
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_PlaneTypeForNormal(vec3_t normal)
{
	vec_t	ax, ay, az;
	
// NOTE: should these have an epsilon around 1.0?		
	if (normal[0] == 1.0 || normal[0] == -1.0)
		return PLANE_X;
	if (normal[1] == 1.0 || normal[1] == -1.0)
		return PLANE_Y;
	if (normal[2] == 1.0 || normal[2] == -1.0)
		return PLANE_Z;
		
	ax = fabs(normal[0]);
	ay = fabs(normal[1]);
	az = fabs(normal[2]);
	
	if (ax >= ay && ax >= az)
		return PLANE_ANYX;
	if (ay >= ax && ay >= az)
		return PLANE_ANYY;
	return PLANE_ANYZ;
} //end of the function TH_PlaneTypeForNormal
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
qboolean TH_PlaneEqual(th_plane_t *p, vec3_t normal, vec_t dist)
{
	if (
	   fabs(p->normal[0] - normal[0]) < NORMAL_EPSILON
	&& fabs(p->normal[1] - normal[1]) < NORMAL_EPSILON
	&& fabs(p->normal[2] - normal[2]) < NORMAL_EPSILON
	&& fabs(p->dist - dist) < DIST_EPSILON )
		return true;
	return false;
} //end of the function TH_PlaneEqual
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AddPlaneToHash(th_plane_t *p)
{
	int hash;

	hash = (int)fabs(p->dist) / 8;
	hash &= (PLANEHASH_SIZE-1);

	p->hashnext = thworld.planehash[hash];
	thworld.planehash[hash] = p;
} //end of the function TH_AddPlaneToHash
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_CreateFloatPlane(vec3_t normal, vec_t dist)
{
	th_plane_t *p, temp;

	if (VectorLength(normal) < 0.5)
		Error ("FloatPlane: bad normal");
	// create a new plane
	if (thworld.numplanes+2 > MAX_TH_PLANES)
		Error ("MAX_TH_PLANES");

	p = &thworld.planes[thworld.numplanes];
	VectorCopy (normal, p->normal);
	p->dist = dist;
	p->type = (p+1)->type = TH_PlaneTypeForNormal (p->normal);
	p->signbits = TH_PlaneSignBits(p->normal);

	VectorSubtract (vec3_origin, normal, (p+1)->normal);
	(p+1)->dist = -dist;
	(p+1)->signbits = TH_PlaneSignBits((p+1)->normal);

	thworld.numplanes += 2;

	// allways put axial planes facing positive first
	if (p->type < 3)
	{
		if (p->normal[0] < 0 || p->normal[1] < 0 || p->normal[2] < 0)
		{
			// flip order
			temp = *p;
			*p = *(p+1);
			*(p+1) = temp;

			TH_AddPlaneToHash(p);
			TH_AddPlaneToHash(p+1);
			return thworld.numplanes - 1;
		} //end if
	} //end if

	TH_AddPlaneToHash(p);
	TH_AddPlaneToHash(p+1);
	return thworld.numplanes - 2;
} //end of the function TH_CreateFloatPlane
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_SnapVector(vec3_t normal)
{
	int		i;

	for (i = 0; i < 3; i++)
	{
		if ( fabs(normal[i] - 1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = 1;
			break;
		} //end if
		if ( fabs(normal[i] - -1) < NORMAL_EPSILON )
		{
			VectorClear (normal);
			normal[i] = -1;
			break;
		} //end if
	} //end for
} //end of the function TH_SnapVector
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_SnapPlane(vec3_t normal, vec_t *dist)
{
	TH_SnapVector(normal);

	if (fabs(*dist-Q_rint(*dist)) < DIST_EPSILON)
		*dist = Q_rint(*dist);
} //end of the function TH_SnapPlane
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindFloatPlane(vec3_t normal, vec_t dist)
{
	int i;
	th_plane_t *p;
	int hash, h;

	TH_SnapPlane (normal, &dist);
	hash = (int)fabs(dist) / 8;
	hash &= (PLANEHASH_SIZE-1);

	// search the border bins as well
	for (i = -1; i <= 1; i++)
	{
		h = (hash+i)&(PLANEHASH_SIZE-1);
		for (p = thworld.planehash[h]; p; p = p->hashnext)
		{
			if (TH_PlaneEqual(p, normal, dist))
			{
				return p - thworld.planes;
			} //end if
		} //end for
	} //end for
	return TH_CreateFloatPlane(normal, dist);
} //end of the function TH_FindFloatPlane
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_PlaneFromPoints(int v1, int v2, int v3)
{
	vec3_t t1, t2, normal;
	vec_t dist;
	float *p0, *p1, *p2;

	p0 = thworld.vertexes[v1].v;
	p1 = thworld.vertexes[v2].v;
	p2 = thworld.vertexes[v3].v;

	VectorSubtract(p0, p1, t1);
	VectorSubtract(p2, p1, t2);
	CrossProduct(t1, t2, normal);
	VectorNormalize(normal);

	dist = DotProduct(p0, normal);

	return TH_FindFloatPlane(normal, dist);
} //end of the function TH_PlaneFromPoints
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AddEdgeUser(int edgenum)
{
	th_edge_t *edge;

	edge = &thworld.edges[abs(edgenum)];
	//increase edge user count
	edge->usercount++;
	//increase vertex user count as well
	thworld.vertexes[edge->v[0]].usercount++;
	thworld.vertexes[edge->v[1]].usercount++;
} //end of the function TH_AddEdgeUser
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_RemoveEdgeUser(int edgenum)
{
	th_edge_t *edge;

	edge = &thworld.edges[abs(edgenum)];
	//decrease edge user count
	edge->usercount--;
	//decrease vertex user count as well
	thworld.vertexes[edge->v[0]].usercount--;
	thworld.vertexes[edge->v[1]].usercount--;
} //end of the function TH_RemoveEdgeUser
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_FreeTriangleEdges(th_triangle_t *tri)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		TH_RemoveEdgeUser(abs(tri->edges[i]));
	} //end for
} //end of the function TH_FreeTriangleEdges
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
unsigned TH_HashVec(vec3_t vec)
{
	int x, y;

	x = (MAX_MAP_BOUNDS + (int)(vec[0]+0.5)) >> VERTEXHASH_SHIFT;
	y = (MAX_MAP_BOUNDS + (int)(vec[1]+0.5)) >> VERTEXHASH_SHIFT;

	if (x < 0 || x >= VERTEXHASH_SIZE || y < 0 || y >= VERTEXHASH_SIZE)
		Error("HashVec: point %f %f %f outside valid range", vec[0], vec[1], vec[2]);
	
	return y*VERTEXHASH_SIZE + x;
} //end of the function TH_HashVec
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindVertex(vec3_t v)
{
	int i, h;
	th_vertex_t *vertex;
	vec3_t vert;
	
	for (i = 0; i < 3; i++)
	{
		if ( fabs(v[i] - Q_rint(v[i])) < INTEGRAL_EPSILON)
			vert[i] = Q_rint(v[i]);
		else
			vert[i] = v[i];
	} //end for

	h = TH_HashVec(vert);

	for (vertex = thworld.vertexhash[h]; vertex; vertex = vertex->hashnext)
	{
		if (fabs(vertex->v[0] - vert[0]) < VERTEX_EPSILON &&
			fabs(vertex->v[1] - vert[1]) < VERTEX_EPSILON &&
			fabs(vertex->v[2] - vert[2]) < VERTEX_EPSILON)
		{
			return vertex - thworld.vertexes;
		} //end if
	} //end for
	return 0;
} //end of the function TH_FindVertex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AddVertexToHash(th_vertex_t *vertex)
{
	int hashvalue;

	hashvalue = TH_HashVec(vertex->v);
	vertex->hashnext = thworld.vertexhash[hashvalue];
	thworld.vertexhash[hashvalue] = vertex;
} //end of the function TH_AddVertexToHash
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_CreateVertex(vec3_t v)
{
	if (thworld.numvertexes == 0) thworld.numvertexes = 1;
	if (thworld.numvertexes >= MAX_TH_VERTEXES)
		Error("MAX_TH_VERTEXES");
	VectorCopy(v, thworld.vertexes[thworld.numvertexes].v);
	thworld.vertexes[thworld.numvertexes].usercount = 0;
	TH_AddVertexToHash(&thworld.vertexes[thworld.numvertexes]);
	thworld.numvertexes++;
	return thworld.numvertexes-1;
} //end of the function TH_CreateVertex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindOrCreateVertex(vec3_t v)
{
	int vertexnum;

	vertexnum = TH_FindVertex(v);
	if (!vertexnum) vertexnum = TH_CreateVertex(v);
	return vertexnum;
} //end of the function TH_FindOrCreateVertex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindEdge(int v1, int v2)
{
	int hashvalue;
	th_edge_t *edge;

	hashvalue = (v1 + v2) & (EDGEHASH_SIZE-1);

	for (edge = thworld.edgehash[hashvalue]; edge; edge = edge->hashnext)
	{
		if (edge->v[0] == v1 && edge->v[1] == v2) return edge - thworld.edges;
		if (edge->v[1] == v1 && edge->v[0] == v2) return -(edge - thworld.edges);
	} //end for
	return 0;
} //end of the function TH_FindEdge
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AddEdgeToHash(th_edge_t *edge)
{
	int hashvalue;

	hashvalue = (edge->v[0] + edge->v[1]) & (EDGEHASH_SIZE-1);
	edge->hashnext = thworld.edgehash[hashvalue];
	thworld.edgehash[hashvalue] = edge;
} //end of the function TH_AddEdgeToHash
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_CreateEdge(int v1, int v2)
{
	th_edge_t *edge;

	if (thworld.numedges == 0) thworld.numedges = 1;
	if (thworld.numedges >= MAX_TH_EDGES)
		Error("MAX_TH_EDGES");
	edge = &thworld.edges[thworld.numedges++];
	edge->v[0] = v1;
	edge->v[1] = v2;
	TH_AddEdgeToHash(edge);
	return thworld.numedges-1;
} //end of the function TH_CreateEdge
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindOrCreateEdge(int v1, int v2)
{
	int edgenum;

	edgenum = TH_FindEdge(v1, v2);
	if (!edgenum) edgenum = TH_CreateEdge(v1, v2);
	return edgenum;
} //end of the function TH_FindOrCreateEdge
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindTriangle(int verts[3])
{
	int i, hashvalue, edges[3];
	th_triangle_t *tri;

	for (i = 0; i < 3; i++)
	{
		edges[i] = TH_FindEdge(verts[i], verts[(i+1)%3]);
		if (!edges[i]) return false;
	} //end for
	hashvalue = (abs(edges[0]) + abs(edges[1]) + abs(edges[2])) & (TRIANGLEHASH_SIZE-1);
	for (tri = thworld.trianglehash[hashvalue]; tri; tri = tri->next)
	{
		for (i = 0; i < 3; i++)
		{
			if (abs(tri->edges[i]) != abs(edges[0]) &&
				abs(tri->edges[i]) != abs(edges[1]) &&
				abs(tri->edges[i]) != abs(edges[2])) break;
		} //end for
		if (i >= 3) return tri - thworld.triangles;
	} //end for
	return 0;
} //end of the function TH_FindTriangle
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AddTriangleToHash(th_triangle_t *tri)
{
	int hashvalue;

	hashvalue = (abs(tri->edges[0]) + abs(tri->edges[1]) + abs(tri->edges[2])) & (TRIANGLEHASH_SIZE-1);
	tri->hashnext = thworld.trianglehash[hashvalue];
	thworld.trianglehash[hashvalue] = tri;
} //end of the function TH_AddTriangleToHash
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_CreateTrianglePlanes(int verts[3], th_plane_t *triplane, th_plane_t *planes)
{
	int i;
	vec3_t dir;

	for (i = 0; i < 3; i++)
	{
		VectorSubtract(thworld.vertexes[verts[(i+1)%3]].v, thworld.vertexes[verts[i]].v, dir);
		CrossProduct(dir, triplane->normal, planes[i].normal);
		VectorNormalize(planes[i].normal);
		planes[i].dist = DotProduct(thworld.vertexes[verts[i]].v, planes[i].normal);
	} //end for
} //end of the function TH_CreateTrianglePlanes
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_CreateTriangle(int verts[3])
{
	th_triangle_t *tri;
	int i;

	if (thworld.numtriangles == 0) thworld.numtriangles = 1;
	if (thworld.numtriangles >= MAX_TH_TRIANGLES)
		Error("MAX_TH_TRIANGLES");
	tri = &thworld.triangles[thworld.numtriangles++];
	for (i = 0; i < 3; i++)
	{
		tri->edges[i] = TH_FindOrCreateEdge(verts[i], verts[(i+1)%3]);
		TH_AddEdgeUser(abs(tri->edges[i]));
	} //end for
	tri->front = 0;
	tri->back = 0;
	tri->planenum = TH_PlaneFromPoints(verts[0], verts[1], verts[2]);
	tri->prev = NULL;
	tri->next = NULL;
	tri->hashnext = NULL;
	TH_CreateTrianglePlanes(verts, &thworld.planes[tri->planenum], tri->planes);
	TH_AddTriangleToHash(tri);
	ClearBounds(tri->mins, tri->maxs);
	for (i = 0; i < 3; i++)
	{
		AddPointToBounds(thworld.vertexes[verts[i]].v, tri->mins, tri->maxs);
	} //end for
	return thworld.numtriangles-1;
} //end of the function TH_CreateTriangle
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_CreateTetrahedron(int triangles[4])
{
	th_tetrahedron_t *tetrahedron;
	int i;

	if (thworld.numtetrahedrons == 0) thworld.numtetrahedrons = 1;
	if (thworld.numtetrahedrons >= MAX_TH_TETRAHEDRONS)
		Error("MAX_TH_TETRAHEDRONS");
	tetrahedron = &thworld.tetrahedrons[thworld.numtetrahedrons++];
	for (i = 0; i < 4; i++)
	{
		tetrahedron->triangles[i] = triangles[i];
		if (thworld.triangles[abs(triangles[i])].front)
		{
			thworld.triangles[abs(triangles[i])].back = thworld.numtetrahedrons-1;
		} //end if
		else
		{
			thworld.triangles[abs(triangles[i])].front = thworld.numtetrahedrons-1;
		} //end else
	} //end for
	tetrahedron->volume = 0;
	return thworld.numtetrahedrons-1;
} //end of the function TH_CreateTetrahedron
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_IntersectTrianglePlanes(int v1, int v2, th_plane_t *triplane, th_plane_t *planes)
{
	float *p1, *p2, front, back, frac, d;
	int i, side, lastside;
	vec3_t mid;

	p1 = thworld.vertexes[v1].v;
	p2 = thworld.vertexes[v2].v;

	front = DotProduct(p1, triplane->normal) - triplane->dist;
	back = DotProduct(p2, triplane->normal) - triplane->dist;
	//if both points at the same side of the plane
	if (front < 0.1 && back < 0.1) return false;
	if (front > -0.1 && back > -0.1) return false;
	//
	frac = front/(front-back);
	mid[0] = p1[0] + (p2[0] - p1[0]) * frac;
	mid[1] = p1[1] + (p2[1] - p1[1]) * frac;
	mid[2] = p1[2] + (p2[2] - p1[2]) * frac;
	//if the mid point is at the same side of all the tri bounding planes
	lastside = 0;
	for (i = 0; i < 3; i++)
	{
		d = DotProduct(mid, planes[i].normal) - planes[i].dist;
		side = d < 0;
		if (i && side != lastside) return false;
		lastside = side;
	} //end for
	return true;
} //end of the function TH_IntersectTrianglePlanes
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_OutsideBoundingBox(int v1, int v2, vec3_t mins, vec3_t maxs)
{
	float *p1, *p2;
	int i;

	p1 = thworld.vertexes[v1].v;
	p2 = thworld.vertexes[v2].v;
	//if both points are at the outer side of one of the bounding box planes
	for (i = 0; i < 3; i++)
	{
		if (p1[i] < mins[i] && p2[i] < mins[i]) return true;
		if (p1[i] > maxs[i] && p2[i] > maxs[i]) return true;
	} //end for
	return false;
} //end of the function TH_OutsideBoundingBox
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_TryEdge(int v1, int v2)
{
	int i, j, v;
	th_plane_t *plane;
	th_triangle_t *tri;

	//if the edge already exists it must be valid
	if (TH_FindEdge(v1, v2)) return true;
	//test the edge with all existing triangles
	for (i = 1; i < thworld.numtriangles; i++)
	{
		tri = &thworld.triangles[i];
		//if triangle is enclosed by two tetrahedrons we don't have to test it
		//because the edge always has to go through another triangle of those
		//tetrahedrons first to reach the enclosed triangle
		if (tri->front && tri->back) continue;
		//if the edges is totally outside the triangle bounding box
		if (TH_OutsideBoundingBox(v1, v2, tri->mins, tri->maxs)) continue;
		//if one of the edge vertexes is used by this triangle
		for (j = 0; j < 3; j++)
		{
			v = thworld.edges[abs(tri->edges[j])].v[tri->edges[j] < 0];
			if (v == v1 || v == v2) break;
		} //end for
		if (j < 3) continue;
		//get the triangle plane
		plane = &thworld.planes[tri->planenum];
		//if the edge intersects with a triangle then it's not valid
		if (TH_IntersectTrianglePlanes(v1, v2, plane, tri->planes)) return false;
	} //end for
	return true;
} //end of the function TH_TryEdge
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_TryTriangle(int verts[3])
{
	th_plane_t planes[3], triplane;
	vec3_t t1, t2;
	float *p0, *p1, *p2;
	int i, j;

	p0 = thworld.vertexes[verts[0]].v;
	p1 = thworld.vertexes[verts[1]].v;
	p2 = thworld.vertexes[verts[2]].v;

	VectorSubtract(p0, p1, t1);
	VectorSubtract(p2, p1, t2);
	CrossProduct(t1, t2, triplane.normal);
	VectorNormalize(triplane.normal);
	triplane.dist = DotProduct(p0, triplane.normal);
	//
	TH_CreateTrianglePlanes(verts, &triplane, planes);
	//test if any existing edge intersects with this triangle
	for (i = 1; i < thworld.numedges; i++)
	{
		//if the edge is only used by triangles with tetrahedrons at both sides
		if (!thworld.edges[i].usercount) continue;
		//if one of the triangle vertexes is used by this edge
		for (j = 0; j < 3; j++)
		{
			if (verts[j] == thworld.edges[j].v[0] ||
				verts[j] == thworld.edges[j].v[1]) break;
		} //end for
		if (j < 3) continue;
		//if this edge intersects with the triangle
		if (TH_IntersectTrianglePlanes(thworld.edges[i].v[0], thworld.edges[i].v[1], &triplane, planes)) return false;
	} //end for
	return true;
} //end of the function TH_TryTriangle
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AddTriangleToList(th_triangle_t **trianglelist, th_triangle_t *tri)
{
	tri->prev = NULL;
	tri->next = *trianglelist;
	if (*trianglelist) (*trianglelist)->prev = tri;
	*trianglelist = tri;
} //end of the function TH_AddTriangleToList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_RemoveTriangleFromList(th_triangle_t **trianglelist, th_triangle_t *tri)
{
	if (tri->next) tri->next->prev = tri->prev;
	if (tri->prev) tri->prev->next = tri->next;
	else *trianglelist = tri->next;
} //end of the function TH_RemoveTriangleFromList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindTetrahedron1(th_triangle_t *tri, int *triangles)
{
	int i, j, edgenum, side, v1, v2, v3, v4;
	int verts1[3], verts2[3];
	th_triangle_t *tri2;

	//find another triangle with a shared edge
	for (tri2 = tri->next; tri2; tri2 = tri2->next)
	{
		//if the triangles are in the same plane
		if ((tri->planenum & ~1) == (tri2->planenum & ~1)) continue;
		//try to find a shared edge
		for (i = 0; i < 3; i++)
		{
			edgenum = abs(tri->edges[i]);
			for (j = 0; j < 3; j++)
			{
				if (edgenum == abs(tri2->edges[j])) break;
			} //end for
			if (j < 3) break;
		} //end for
		//if the triangles have a shared edge
		if (i < 3)
		{
			edgenum = tri->edges[(i+1)%3];
			if (edgenum < 0) v1 = thworld.edges[abs(edgenum)].v[0];
			else v1 = thworld.edges[edgenum].v[1];
			edgenum = tri2->edges[(j+1)%3];
			if (edgenum < 0) v2 = thworld.edges[abs(edgenum)].v[0];
			else v2 = thworld.edges[edgenum].v[1];
			//try the new edge
			if (TH_TryEdge(v1, v2))
			{
				edgenum = tri->edges[i];
				side = edgenum < 0;
				//get the vertexes of the shared edge
				v3 = thworld.edges[abs(edgenum)].v[side];
				v4 = thworld.edges[abs(edgenum)].v[!side];
				//try the two new triangles
				verts1[0] = v1;
				verts1[1] = v2;
				verts1[2] = v3;
				triangles[2] = TH_FindTriangle(verts1);
				if (triangles[2] || TH_TryTriangle(verts1))
				{
					verts2[0] = v2;
					verts2[1] = v1;
					verts2[2] = v4;
					triangles[3] = TH_FindTriangle(verts2);
					if (triangles[3] || TH_TryTriangle(verts2))
					{
						triangles[0] = tri - thworld.triangles;
						triangles[1] = tri2 - thworld.triangles;
						if (!triangles[2]) triangles[2] = TH_CreateTriangle(verts1);
						if (!triangles[3]) triangles[3] = TH_CreateTriangle(verts2);
						return true;
					} //end if
				} //end if
			} //end if
		} //end if
	} //end for
	return false;
} //end of the function TH_FindTetrahedron
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_FindTetrahedron2(th_triangle_t *tri, int *triangles)
{
	int i, edgenum, v1, verts[3], triverts[3];
	float d;
	th_plane_t *plane;

	//get the verts of this triangle
	for (i = 0; i < 3; i++)
	{
		edgenum = tri->edges[i];
		if (edgenum < 0) verts[i] = thworld.edges[abs(edgenum)].v[1];
		else verts[i] = thworld.edges[edgenum].v[0];
	} //end for
	//
	plane = &thworld.planes[tri->planenum];
	for (v1 = 0; v1 < thworld.numvertexes; v1++)
	{
		//if the vertex is only used by triangles with tetrahedrons at both sides
		if (!thworld.vertexes[v1].usercount) continue;
		//check if the vertex is not coplanar with the triangle
		d = DotProduct(thworld.vertexes[v1].v, plane->normal) - plane->dist;
		if (fabs(d) < 1) continue;
		//check if we can create edges from the triangle towards this new vertex
		for (i = 0; i < 3; i++)
		{
			if (v1 == verts[i]) break;
			if (!TH_TryEdge(v1, verts[i])) break;
		} //end for
		if (i < 3) continue;
		//check if the triangles are valid
		for (i = 0; i < 3; i++)
		{
			triverts[0] = v1;
			triverts[1] = verts[i];
			triverts[2] = verts[(i+1)%3];
			//if the triangle already exists then it is valid
			triangles[i] = TH_FindTriangle(triverts);
			if (!triangles[i])
			{
				if (!TH_TryTriangle(triverts)) break;
			} //end if
		} //end for
		if (i < 3) continue;
		//create the tetrahedron triangles using the new vertex
		for (i = 0; i < 3; i++)
		{
			if (!triangles[i])
			{
				triverts[0] = v1;
				triverts[1] = verts[i];
				triverts[2] = verts[(i+1)%3];
				triangles[i] = TH_CreateTriangle(triverts);
			} //end if
		} //end for
		//add the existing triangle
		triangles[3] = tri - thworld.triangles;
		//
		return true;
	} //end for
	return false;
} //end of the function TH_FindTetrahedron2
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_TetrahedralDecomposition(th_triangle_t *triangles)
{
	int i, thtriangles[4], numtriangles;
	th_triangle_t *donetriangles, *tri;

	donetriangles = NULL;

	/*
	numtriangles = 0;
	qprintf("%6d triangles", numtriangles);
	for (tri = triangles; tri; tri = triangles)
	{
		qprintf("\r%6d", numtriangles++);
		if (!TH_FindTetrahedron1(tri, thtriangles))
		{
//			if (!TH_FindTetrahedron2(tri, thtriangles))
			{
//				Error("triangle without tetrahedron");
				TH_RemoveTriangleFromList(&triangles, tri);
				continue;
			} //end if
		} //end if
		//create a tetrahedron from the triangles
		TH_CreateTetrahedron(thtriangles);
		//
		for (i = 0; i < 4; i++)
		{
			if (thworld.triangles[abs(thtriangles[i])].front &&
				thworld.triangles[abs(thtriangles[i])].back)
			{
				TH_RemoveTriangleFromList(&triangles, &thworld.triangles[abs(thtriangles[i])]);
				TH_AddTriangleToList(&donetriangles, &thworld.triangles[abs(thtriangles[i])]);
				TH_FreeTriangleEdges(&thworld.triangles[abs(thtriangles[i])]);
			} //end if
			else
			{
				TH_AddTriangleToList(&triangles, &thworld.triangles[abs(thtriangles[i])]);
			} //end else
		} //end for
	} //end for*/
	qprintf("%6d tetrahedrons", thworld.numtetrahedrons);
	do
	{
		do
		{
			numtriangles = 0;
			for (i = 1; i < thworld.numtriangles; i++)
			{
				tri = &thworld.triangles[i];
				if (tri->front && tri->back) continue;
				//qprintf("\r%6d", numtriangles++);
				if (!TH_FindTetrahedron1(tri, thtriangles))
				{
//					if (!TH_FindTetrahedron2(tri, thtriangles))
					{
						continue;
					} //end if
				} //end if
				numtriangles++;
				//create a tetrahedron from the triangles
				TH_CreateTetrahedron(thtriangles);
				qprintf("\r%6d", thworld.numtetrahedrons);
			} //end for
		} while(numtriangles);
 		for (i = 1; i < thworld.numtriangles; i++)
		{
			tri = &thworld.triangles[i];
			if (tri->front && tri->back) continue;
			//qprintf("\r%6d", numtriangles++);
//			if (!TH_FindTetrahedron1(tri, thtriangles))
			{
				if (!TH_FindTetrahedron2(tri, thtriangles))
				{
					continue;
				} //end if
			} //end if
			numtriangles++;
			//create a tetrahedron from the triangles
			TH_CreateTetrahedron(thtriangles);
			qprintf("\r%6d", thworld.numtetrahedrons);
		} //end for
	} while(numtriangles);
	//
	numtriangles = 0;
	for (i = 1; i < thworld.numtriangles; i++)
	{
		tri = &thworld.triangles[i];
		if (!tri->front && !tri->back) numtriangles++;
	} //end for
	Log_Print("\r%6d triangles with front only\n", numtriangles);
	Log_Print("\r%6d tetrahedrons\n", thworld.numtetrahedrons-1);
} //end of the function TH_TetrahedralDecomposition
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AASFaceVertex(aas_face_t *face, int index, vec3_t vertex)
{
	int edgenum, side;

	edgenum = aasworld.edgeindex[face->firstedge + index];
	side = edgenum < 0;
	VectorCopy(aasworld.vertexes[aasworld.edges[abs(edgenum)].v[side]], vertex);
} //end of the function TH_AASFaceVertex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TH_Colinear(float *v0, float *v1, float *v2)
{
	vec3_t t1, t2, vcross;
	float d;
	
	VectorSubtract(v1, v0, t1);
	VectorSubtract(v2, v0, t2);
	CrossProduct (t1, t2, vcross);
	d = VectorLength( vcross );

	// if cross product is zero point is colinear
	if (d < 10)
	{
		return true;
	} //end if
	return false;
} //end of the function TH_Colinear
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_FaceCenter(aas_face_t *face, vec3_t center)
{
	int i, edgenum, side;
	aas_edge_t *edge;

	VectorClear(center);
	for (i = 0; i < face->numedges; i++)
	{
		edgenum = abs(aasworld.edgeindex[face->firstedge + i]);
		side = edgenum < 0;
		edge = &aasworld.edges[abs(edgenum)];
		VectorAdd(aasworld.vertexes[edge->v[side]], center, center);
	} //end for
	VectorScale(center, 1.0 / face->numedges, center);
} //end of the function TH_FaceCenter
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
th_triangle_t *TH_CreateAASFaceTriangles(aas_face_t *face)
{
	int i, first, verts[3], trinum;
	vec3_t p0, p1, p2, p3, p4, center;
	th_triangle_t *tri, *triangles;

	triangles = NULL;
	//find three points that are not colinear
	for (i = 0; i < face->numedges; i++)
	{
		TH_AASFaceVertex(face, (face->numedges + i-2)%face->numedges, p0);
		TH_AASFaceVertex(face, (face->numedges + i-1)%face->numedges, p1);
		TH_AASFaceVertex(face, (i  )%face->numedges, p2);
		if (TH_Colinear(p2, p0, p1)) continue;
		TH_AASFaceVertex(face, (i+1)%face->numedges, p3);
		TH_AASFaceVertex(face, (i+2)%face->numedges, p4);
		if (TH_Colinear(p2, p3, p4)) continue;
		break;
	} //end for
	//if there are three points that are not colinear
	if (i < face->numedges)
	{
		//normal triangulation
		first = i; //left and right most point of three non-colinear points
		TH_AASFaceVertex(face, first, p0);
		verts[0] = TH_FindOrCreateVertex(p0);
		for (i = 1; i < face->numedges-1; i++)
		{
			TH_AASFaceVertex(face, (first+i  )%face->numedges, p1);
			TH_AASFaceVertex(face, (first+i+1)%face->numedges, p2);
			verts[1] = TH_FindOrCreateVertex(p1);
			verts[2] = TH_FindOrCreateVertex(p2);
			trinum = TH_CreateTriangle(verts);
			tri = &thworld.triangles[trinum];
			tri->front = -1;
			TH_AddTriangleToList(&triangles, tri);
		} //end for
	} //end if
	else
	{
		//fan triangulation
		TH_FaceCenter(face, center);
		//
		verts[0] = TH_FindOrCreateVertex(center);
		for (i = 0; i < face->numedges; i++)
		{
			TH_AASFaceVertex(face, (i  )%face->numedges, p1);
			TH_AASFaceVertex(face, (i+1)%face->numedges, p2);
			if (TH_Colinear(center, p1, p2)) continue;
			verts[1] = TH_FindOrCreateVertex(p1);
			verts[2] = TH_FindOrCreateVertex(p2);
			trinum = TH_CreateTriangle(verts);
			tri = &thworld.triangles[trinum];
			tri->front = -1;
			TH_AddTriangleToList(&triangles, tri);
		} //end for
	} //end else
	return triangles;
} //end of the function TH_CreateAASFaceTriangles
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
th_triangle_t *TH_AASToTriangleMesh(void)
{
	int i, j, facenum, otherareanum;
	aas_face_t *face;
	th_triangle_t *tri, *nexttri, *triangles;

	triangles = NULL;
	for (i = 1; i < aasworld.numareas; i++)
	{
		//if (!(aasworld.areasettings[i].presencetype & PRESENCE_NORMAL)) continue;
		for (j = 0; j < aasworld.areas[i].numfaces; j++)
		{
			facenum = abs(aasworld.faceindex[aasworld.areas[i].firstface + j]);
			face = &aasworld.faces[facenum];
			//only convert solid faces into triangles
			if (!(face->faceflags & FACE_SOLID))
			{
				/*
				if (face->frontarea == i) otherareanum = face->backarea;
				else otherareanum = face->frontarea;
				if (aasworld.areasettings[otherareanum].presencetype & PRESENCE_NORMAL) continue;
				*/
				continue;
			} //end if
			//
			tri = TH_CreateAASFaceTriangles(face);
			for (; tri; tri = nexttri)
			{
				nexttri = tri->next;
				TH_AddTriangleToList(&triangles, tri);
			} //end for
		} //end if
	} //end for
	return triangles;
} //end of the function TH_AASToTriangleMesh
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void TH_AASToTetrahedrons(char *filename)
{
	th_triangle_t *triangles, *tri, *lasttri;
	int cnt;

	if (!AAS_LoadAASFile(filename, 0, 0))
		Error("couldn't load %s\n", filename);

	//
	TH_InitMaxTH();
	//create a triangle mesh from the solid faces in the AAS file
	triangles = TH_AASToTriangleMesh();
	//
	cnt = 0;
	lasttri = NULL;
	for (tri = triangles; tri; tri = tri->next)
	{
		cnt++;
		if (tri->prev != lasttri) Log_Print("BAH\n");
		lasttri = tri;
	} //end for
	Log_Print("%6d triangles\n", cnt);
	//create a tetrahedral decomposition of the world bounded by triangles
	TH_TetrahedralDecomposition(triangles);
	//
	TH_FreeMaxTH();
} //end of the function TH_AASToTetrahedrons
