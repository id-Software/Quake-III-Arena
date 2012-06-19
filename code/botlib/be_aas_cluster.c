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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		be_aas_cluster.c
 *
 * desc:		area clustering
 *
 * $Archive: /MissionPack/code/botlib/be_aas_cluster.c $
 *
 *****************************************************************************/

#include "../qcommon/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_log.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "aasfile.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"
#include "be_aas_cluster.h"

extern botlib_import_t botimport;

#define AAS_MAX_PORTALS					65536
#define AAS_MAX_PORTALINDEXSIZE			65536
#define AAS_MAX_CLUSTERS				65536
//
#define MAX_PORTALAREAS			1024

// do not flood through area faces, only use reachabilities
int nofaceflood = qtrue;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveClusterAreas(void)
{
	int i;

	for (i = 1; i < aasworld.numareas; i++)
	{
		aasworld.areasettings[i].cluster = 0;
	} //end for
} //end of the function AAS_RemoveClusterAreas
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ClearCluster(int clusternum)
{
	int i;

	for (i = 1; i < aasworld.numareas; i++)
	{
		if (aasworld.areasettings[i].cluster == clusternum)
		{
			aasworld.areasettings[i].cluster = 0;
		} //end if
	} //end for
} //end of the function AAS_ClearCluster
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemovePortalsClusterReference(int clusternum)
{
	int portalnum;

	for (portalnum = 1; portalnum < aasworld.numportals; portalnum++)
	{
		if (aasworld.portals[portalnum].frontcluster == clusternum)
		{
			aasworld.portals[portalnum].frontcluster = 0;
		} //end if
		if (aasworld.portals[portalnum].backcluster == clusternum)
		{
			aasworld.portals[portalnum].backcluster = 0;
		} //end if
	} //end for
} //end of the function AAS_RemovePortalsClusterReference
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_UpdatePortal(int areanum, int clusternum)
{
	int portalnum;
	aas_portal_t *portal;
	aas_cluster_t *cluster;

	//find the portal of the area
	for (portalnum = 1; portalnum < aasworld.numportals; portalnum++)
	{
		if (aasworld.portals[portalnum].areanum == areanum) break;
	} //end for
	//
	if (portalnum == aasworld.numportals)
	{
		AAS_Error("no portal of area %d\n", areanum);
		return qtrue;
	} //end if
	//
	portal = &aasworld.portals[portalnum];
	//if the portal is already fully updated
	if (portal->frontcluster == clusternum) return qtrue;
	if (portal->backcluster == clusternum) return qtrue;
	//if the portal has no front cluster yet
	if (!portal->frontcluster)
	{
		portal->frontcluster = clusternum;
	} //end if
	//if the portal has no back cluster yet
	else if (!portal->backcluster)
	{
		portal->backcluster = clusternum;
	} //end else if
	else
	{
		//remove the cluster portal flag contents
		aasworld.areasettings[areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
		Log_Write("portal area %d is seperating more than two clusters\r\n", areanum);
		return qfalse;
	} //end else
	if (aasworld.portalindexsize >= AAS_MAX_PORTALINDEXSIZE)
	{
		AAS_Error("AAS_MAX_PORTALINDEXSIZE\n");
		return qtrue;
	} //end if
	//set the area cluster number to the negative portal number
	aasworld.areasettings[areanum].cluster = -portalnum;
	//add the portal to the cluster using the portal index
	cluster = &aasworld.clusters[clusternum];
	aasworld.portalindex[cluster->firstportal + cluster->numportals] = portalnum;
	aasworld.portalindexsize++;
	cluster->numportals++;
	return qtrue;
} //end of the function AAS_UpdatePortal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_FloodClusterAreas_r(int areanum, int clusternum)
{
	aas_area_t *area;
	aas_face_t *face;
	int facenum, i;

	//
	if (areanum <= 0 || areanum >= aasworld.numareas)
	{
		AAS_Error("AAS_FloodClusterAreas_r: areanum out of range\n");
		return qfalse;
	} //end if
	//if the area is already part of a cluster
	if (aasworld.areasettings[areanum].cluster > 0)
	{
		if (aasworld.areasettings[areanum].cluster == clusternum) return qtrue;
		//
		//there's a reachability going from one cluster to another only in one direction
		//
		AAS_Error("cluster %d touched cluster %d at area %d\n",
				clusternum, aasworld.areasettings[areanum].cluster, areanum);
		return qfalse;
	} //end if
	//don't add the cluster portal areas to the clusters
	if (aasworld.areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL)
	{
		return AAS_UpdatePortal(areanum, clusternum);
	} //end if
	//set the area cluster number
	aasworld.areasettings[areanum].cluster = clusternum;
	aasworld.areasettings[areanum].clusterareanum =
				aasworld.clusters[clusternum].numareas;
	//the cluster has an extra area
	aasworld.clusters[clusternum].numareas++;

	area = &aasworld.areas[areanum];
	//use area faces to flood into adjacent areas
	if (!nofaceflood)
	{
		for (i = 0; i < area->numfaces; i++)
		{
			facenum = abs(aasworld.faceindex[area->firstface + i]);
			face = &aasworld.faces[facenum];
			if (face->frontarea == areanum)
			{
				if (face->backarea) if (!AAS_FloodClusterAreas_r(face->backarea, clusternum)) return qfalse;
			} //end if
			else
			{
				if (face->frontarea) if (!AAS_FloodClusterAreas_r(face->frontarea, clusternum)) return qfalse;
			} //end else
		} //end for
	} //end if
	//use the reachabilities to flood into other areas
	for (i = 0; i < aasworld.areasettings[areanum].numreachableareas; i++)
	{
		if (!aasworld.reachability[
					aasworld.areasettings[areanum].firstreachablearea + i].areanum)
		{
			continue;
		} //end if
		if (!AAS_FloodClusterAreas_r(aasworld.reachability[
				aasworld.areasettings[areanum].firstreachablearea + i].areanum, clusternum)) return qfalse;
	} //end for
	return qtrue;
} //end of the function AAS_FloodClusterAreas_r
//===========================================================================
// try to flood from all areas without cluster into areas with a cluster set
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_FloodClusterAreasUsingReachabilities(int clusternum)
{
	int i, j, areanum;

	for (i = 1; i < aasworld.numareas; i++)
	{
		//if this area already has a cluster set
		if (aasworld.areasettings[i].cluster)
			continue;
		//if this area is a cluster portal
		if (aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
			continue;
		//loop over the reachable areas from this area
		for (j = 0; j < aasworld.areasettings[i].numreachableareas; j++)
		{
			//the reachable area
			areanum = aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].areanum;
			//if this area is a cluster portal
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL)
				continue;
			//if this area has a cluster set
			if (aasworld.areasettings[areanum].cluster)
			{
				if (!AAS_FloodClusterAreas_r(i, clusternum))
					return qfalse;
				i = 0;
				break;
			} //end if
		} //end for
	} //end for
	return qtrue;
} //end of the function AAS_FloodClusterAreasUsingReachabilities
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_NumberClusterPortals(int clusternum)
{
	int i, portalnum;
	aas_cluster_t *cluster;
	aas_portal_t *portal;

	cluster = &aasworld.clusters[clusternum];
	for (i = 0; i < cluster->numportals; i++)
	{
		portalnum = aasworld.portalindex[cluster->firstportal + i];
		portal = &aasworld.portals[portalnum];
		if (portal->frontcluster == clusternum)
		{
			portal->clusterareanum[0] = cluster->numareas++;
		} //end if
		else
		{
			portal->clusterareanum[1] = cluster->numareas++;
		} //end else
	} //end for
} //end of the function AAS_NumberClusterPortals
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_NumberClusterAreas(int clusternum)
{
	int i, portalnum;
	aas_cluster_t *cluster;
	aas_portal_t *portal;

	aasworld.clusters[clusternum].numareas = 0;
	aasworld.clusters[clusternum].numreachabilityareas = 0;
	//number all areas in this cluster WITH reachabilities
	for (i = 1; i < aasworld.numareas; i++)
	{
		//
		if (aasworld.areasettings[i].cluster != clusternum) continue;
		//
		if (!AAS_AreaReachability(i)) continue;
		//
		aasworld.areasettings[i].clusterareanum = aasworld.clusters[clusternum].numareas;
		//the cluster has an extra area
		aasworld.clusters[clusternum].numareas++;
		aasworld.clusters[clusternum].numreachabilityareas++;
	} //end for
	//number all portals in this cluster WITH reachabilities
	cluster = &aasworld.clusters[clusternum];
	for (i = 0; i < cluster->numportals; i++)
	{
		portalnum = aasworld.portalindex[cluster->firstportal + i];
		portal = &aasworld.portals[portalnum];
		if (!AAS_AreaReachability(portal->areanum)) continue;
		if (portal->frontcluster == clusternum)
		{
			portal->clusterareanum[0] = cluster->numareas++;
			aasworld.clusters[clusternum].numreachabilityareas++;
		} //end if
		else
		{
			portal->clusterareanum[1] = cluster->numareas++;
			aasworld.clusters[clusternum].numreachabilityareas++;
		} //end else
	} //end for
	//number all areas in this cluster WITHOUT reachabilities
	for (i = 1; i < aasworld.numareas; i++)
	{
		//
		if (aasworld.areasettings[i].cluster != clusternum) continue;
		//
		if (AAS_AreaReachability(i)) continue;
		//
		aasworld.areasettings[i].clusterareanum = aasworld.clusters[clusternum].numareas;
		//the cluster has an extra area
		aasworld.clusters[clusternum].numareas++;
	} //end for
	//number all portals in this cluster WITHOUT reachabilities
	cluster = &aasworld.clusters[clusternum];
	for (i = 0; i < cluster->numportals; i++)
	{
		portalnum = aasworld.portalindex[cluster->firstportal + i];
		portal = &aasworld.portals[portalnum];
		if (AAS_AreaReachability(portal->areanum)) continue;
		if (portal->frontcluster == clusternum)
		{
			portal->clusterareanum[0] = cluster->numareas++;
		} //end if
		else
		{
			portal->clusterareanum[1] = cluster->numareas++;
		} //end else
	} //end for
} //end of the function AAS_NumberClusterAreas
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_FindClusters(void)
{
	int i;
	aas_cluster_t *cluster;

	AAS_RemoveClusterAreas();
	//
	for (i = 1; i < aasworld.numareas; i++)
	{
		//if the area is already part of a cluster
		if (aasworld.areasettings[i].cluster)
			continue;
		// if not flooding through faces only use areas that have reachabilities
		if (nofaceflood)
		{
			if (!aasworld.areasettings[i].numreachableareas)
				continue;
		} //end if
		//if the area is a cluster portal
		if (aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
			continue;
		if (aasworld.numclusters >= AAS_MAX_CLUSTERS)
		{
			AAS_Error("AAS_MAX_CLUSTERS\n");
			return qfalse;
		} //end if
		cluster = &aasworld.clusters[aasworld.numclusters];
		cluster->numareas = 0;
		cluster->numreachabilityareas = 0;
		cluster->firstportal = aasworld.portalindexsize;
		cluster->numportals = 0;
		//flood the areas in this cluster
		if (!AAS_FloodClusterAreas_r(i, aasworld.numclusters))
			return qfalse;
		if (!AAS_FloodClusterAreasUsingReachabilities(aasworld.numclusters))
			return qfalse;
		//number the cluster areas
		//AAS_NumberClusterPortals(aasworld.numclusters);
		AAS_NumberClusterAreas(aasworld.numclusters);
		//Log_Write("cluster %d has %d areas\r\n", aasworld.numclusters, cluster->numareas);
		aasworld.numclusters++;
	} //end for
	return qtrue;
} //end of the function AAS_FindClusters
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CreatePortals(void)
{
	int i;
	aas_portal_t *portal;

	for (i = 1; i < aasworld.numareas; i++)
	{
		//if the area is a cluster portal
		if (aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			if (aasworld.numportals >= AAS_MAX_PORTALS)
			{
				AAS_Error("AAS_MAX_PORTALS\n");
				return;
			} //end if
			portal = &aasworld.portals[aasworld.numportals];
			portal->areanum = i;
			portal->frontcluster = 0;
			portal->backcluster = 0;
			aasworld.numportals++;
		} //end if
	} //end for
} //end of the function AAS_CreatePortals
/*
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_MapContainsTeleporters(void)
{
	bsp_entity_t *entities, *ent;
	char *classname;

	entities = AAS_ParseBSPEntities();

	for (ent = entities; ent; ent = ent->next)
	{
		classname = AAS_ValueForBSPEpairKey(ent, "classname");
		if (classname && !strcmp(classname, "misc_teleporter"))
		{
			AAS_FreeBSPEntities(entities);
			return qtrue;
		} //end if
	} //end for
	return qfalse;
} //end of the function AAS_MapContainsTeleporters
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NonConvexFaces(aas_face_t *face1, aas_face_t *face2, int side1, int side2)
{
	int i, j, edgenum;
	aas_plane_t *plane1, *plane2;
	aas_edge_t *edge;
	

	plane1 = &aasworld.planes[face1->planenum ^ side1];
	plane2 = &aasworld.planes[face2->planenum ^ side2];

	//check if one of the points of face1 is at the back of the plane of face2
	for (i = 0; i < face1->numedges; i++)
	{
		edgenum = abs(aasworld.edgeindex[face1->firstedge + i]);
		edge = &aasworld.edges[edgenum];
		for (j = 0; j < 2; j++)
		{
			if (DotProduct(plane2->normal, aasworld.vertexes[edge->v[j]]) -
							plane2->dist < -0.01) return qtrue;
		} //end for
	} //end for
	for (i = 0; i < face2->numedges; i++)
	{
		edgenum = abs(aasworld.edgeindex[face2->firstedge + i]);
		edge = &aasworld.edges[edgenum];
		for (j = 0; j < 2; j++)
		{
			if (DotProduct(plane1->normal, aasworld.vertexes[edge->v[j]]) -
							plane1->dist < -0.01) return qtrue;
		} //end for
	} //end for

	return qfalse;
} //end of the function AAS_NonConvexFaces
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_CanMergeAreas(int *areanums, int numareas)
{
	int i, j, s, face1num, face2num, side1, side2, fn1, fn2;
	aas_face_t *face1, *face2;
	aas_area_t *area1, *area2;

	for (i = 0; i < numareas; i++)
	{
		area1 = &aasworld.areas[areanums[i]];
		for (fn1 = 0; fn1 < area1->numfaces; fn1++)
		{
			face1num = abs(aasworld.faceindex[area1->firstface + fn1]);
			face1 = &aasworld.faces[face1num];
			side1 = face1->frontarea != areanums[i];
			//check if the face isn't a shared one with one of the other areas
			for (s = 0; s < numareas; s++)
			{
				if (s == i) continue;
				if (face1->frontarea == s || face1->backarea == s) break;
			} //end for
			//if the face was a shared one
			if (s != numareas) continue;
			//
			for (j = 0; j < numareas; j++)
			{
				if (j == i) continue;
				area2 = &aasworld.areas[areanums[j]];
				for (fn2 = 0; fn2 < area2->numfaces; fn2++)
				{
					face2num = abs(aasworld.faceindex[area2->firstface + fn2]);
					face2 = &aasworld.faces[face2num];
					side2 = face2->frontarea != areanums[j];
					//check if the face isn't a shared one with one of the other areas
					for (s = 0; s < numareas; s++)
					{
						if (s == j) continue;
						if (face2->frontarea == s || face2->backarea == s) break;
					} //end for
					//if the face was a shared one
					if (s != numareas) continue;
					//
					if (AAS_NonConvexFaces(face1, face2, side1, side2)) return qfalse;
				} //end for
			} //end for
		} //end for
	} //end for
	return qtrue;
} //end of the function AAS_CanMergeAreas
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_NonConvexEdges(aas_edge_t *edge1, aas_edge_t *edge2, int side1, int side2, int planenum)
{
	int i;
	vec3_t edgevec1, edgevec2, normal1, normal2;
	float dist1, dist2;
	aas_plane_t *plane;

	plane = &aasworld.planes[planenum];
	VectorSubtract(aasworld.vertexes[edge1->v[1]], aasworld.vertexes[edge1->v[0]], edgevec1);
	VectorSubtract(aasworld.vertexes[edge2->v[1]], aasworld.vertexes[edge2->v[0]], edgevec2);
	if (side1) VectorInverse(edgevec1);
	if (side2) VectorInverse(edgevec2);
	//
	CrossProduct(edgevec1, plane->normal, normal1);
	dist1 = DotProduct(normal1, aasworld.vertexes[edge1->v[0]]);
	CrossProduct(edgevec2, plane->normal, normal2);
	dist2 = DotProduct(normal2, aasworld.vertexes[edge2->v[0]]);

	for (i = 0; i < 2; i++)
	{
		if (DotProduct(aasworld.vertexes[edge1->v[i]], normal2) - dist2 < -0.01) return qfalse;
	} //end for
	for (i = 0; i < 2; i++)
	{
		if (DotProduct(aasworld.vertexes[edge2->v[i]], normal1) - dist1 < -0.01) return qfalse;
	} //end for
	return qtrue;
} //end of the function AAS_NonConvexEdges
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_CanMergeFaces(int *facenums, int numfaces, int planenum)
{
	int i, j, s, edgenum1, edgenum2, side1, side2, en1, en2, ens;
	aas_face_t *face1, *face2, *otherface;
	aas_edge_t *edge1, *edge2;

	for (i = 0; i < numfaces; i++)
	{
		face1 = &aasworld.faces[facenums[i]];
		for (en1 = 0; en1 < face1->numedges; en1++)
		{
			edgenum1 = aasworld.edgeindex[face1->firstedge + en1];
			side1 = (edgenum1 < 0) ^ (face1->planenum != planenum);
			edgenum1 = abs(edgenum1);
			edge1 = &aasworld.edges[edgenum1];
			//check if the edge is shared with another face
			for (s = 0; s < numfaces; s++)
			{
				if (s == i) continue;
				otherface = &aasworld.faces[facenums[s]];
				for (ens = 0; ens < otherface->numedges; ens++)
				{
					if (edgenum1 == abs(aasworld.edgeindex[otherface->firstedge + ens])) break;
				} //end for
				if (ens != otherface->numedges) break;
			} //end for
			//if the edge was shared
			if (s != numfaces) continue;
			//
			for (j = 0; j < numfaces; j++)
			{
				if (j == i) continue;
				face2 = &aasworld.faces[facenums[j]];
				for (en2 = 0; en2 < face2->numedges; en2++)
				{
					edgenum2 = aasworld.edgeindex[face2->firstedge + en2];
					side2 = (edgenum2 < 0) ^ (face2->planenum != planenum);
					edgenum2 = abs(edgenum2);
					edge2 = &aasworld.edges[edgenum2];
					//check if the edge is shared with another face
					for (s = 0; s < numfaces; s++)
					{
						if (s == i) continue;
						otherface = &aasworld.faces[facenums[s]];
						for (ens = 0; ens < otherface->numedges; ens++)
						{
							if (edgenum2 == abs(aasworld.edgeindex[otherface->firstedge + ens])) break;
						} //end for
						if (ens != otherface->numedges) break;
					} //end for
					//if the edge was shared
					if (s != numfaces) continue;
					//
					if (AAS_NonConvexEdges(edge1, edge2, side1, side2, planenum)) return qfalse;
				} //end for
			} //end for
		} //end for
	} //end for
	return qtrue;
} //end of the function AAS_CanMergeFaces*/
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ConnectedAreas_r(int *areanums, int numareas, int *connectedareas, int curarea)
{
	int i, j, otherareanum, facenum;
	aas_area_t *area;
	aas_face_t *face;

	connectedareas[curarea] = qtrue;
	area = &aasworld.areas[areanums[curarea]];
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = abs(aasworld.faceindex[area->firstface + i]);
		face = &aasworld.faces[facenum];
		//if the face is solid
		if (face->faceflags & FACE_SOLID) continue;
		//get the area at the other side of the face
		if (face->frontarea != areanums[curarea]) otherareanum = face->frontarea;
		else otherareanum = face->backarea;
		//check if the face is leading to one of the other areas
		for (j = 0; j < numareas; j++)
		{
			if (areanums[j] == otherareanum) break;
		} //end for
		//if the face isn't leading to one of the other areas
		if (j == numareas) continue;
		//if the other area is already connected
		if (connectedareas[j]) continue;
		//recursively proceed with the other area
		AAS_ConnectedAreas_r(areanums, numareas, connectedareas, j);
	} //end for
} //end of the function AAS_ConnectedAreas_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_ConnectedAreas(int *areanums, int numareas)
{
	int connectedareas[MAX_PORTALAREAS], i;

	Com_Memset(connectedareas, 0, sizeof(connectedareas));
	if (numareas < 1) return qfalse;
	if (numareas == 1) return qtrue;
	AAS_ConnectedAreas_r(areanums, numareas, connectedareas, 0);
	for (i = 0; i < numareas; i++)
	{
		if (!connectedareas[i]) return qfalse;
	} //end for
	return qtrue;
} //end of the function AAS_ConnectedAreas
//===========================================================================
// gets adjacent areas with less presence types recursively
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_GetAdjacentAreasWithLessPresenceTypes_r(int *areanums, int numareas, int curareanum)
{
	int i, j, presencetype, otherpresencetype, otherareanum, facenum;
	aas_area_t *area;
	aas_face_t *face;

	areanums[numareas++] = curareanum;
	area = &aasworld.areas[curareanum];
	presencetype = aasworld.areasettings[curareanum].presencetype;
	for (i = 0; i < area->numfaces; i++)
	{
		facenum = abs(aasworld.faceindex[area->firstface + i]);
		face = &aasworld.faces[facenum];
		//if the face is solid
		if (face->faceflags & FACE_SOLID) continue;
		//the area at the other side of the face
		if (face->frontarea != curareanum) otherareanum = face->frontarea;
		else otherareanum = face->backarea;
		//
		otherpresencetype = aasworld.areasettings[otherareanum].presencetype;
		//if the other area has less presence types
		if ((presencetype & ~otherpresencetype) &&
				!(otherpresencetype & ~presencetype))
		{
			//check if the other area isn't already in the list
			for (j = 0; j < numareas; j++)
			{
				if (otherareanum == areanums[j]) break;
			} //end for
			//if the other area isn't already in the list
			if (j == numareas)
			{
				if (numareas >= MAX_PORTALAREAS)
				{
					AAS_Error("MAX_PORTALAREAS\n");
					return numareas;
				} //end if
				numareas = AAS_GetAdjacentAreasWithLessPresenceTypes_r(areanums, numareas, otherareanum);
			} //end if
		} //end if
	} //end for
	return numareas;
} //end of the function AAS_GetAdjacentAreasWithLessPresenceTypes_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_CheckAreaForPossiblePortals(int areanum)
{
	int i, j, k, fen, ben, frontedgenum, backedgenum, facenum;
	int areanums[MAX_PORTALAREAS], numareas, otherareanum;
	int numareafrontfaces[MAX_PORTALAREAS], numareabackfaces[MAX_PORTALAREAS];
	int frontfacenums[MAX_PORTALAREAS], backfacenums[MAX_PORTALAREAS];
	int numfrontfaces, numbackfaces;
	int frontareanums[MAX_PORTALAREAS], backareanums[MAX_PORTALAREAS];
	int numfrontareas, numbackareas;
	int frontplanenum, backplanenum, faceplanenum;
	aas_area_t *area;
	aas_face_t *frontface, *backface, *face;

	//if it isn't already a portal
	if (aasworld.areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL) return 0;
	//it must be a grounded area
	if (!(aasworld.areasettings[areanum].areaflags & AREA_GROUNDED)) return 0;
	//
	Com_Memset(numareafrontfaces, 0, sizeof(numareafrontfaces));
	Com_Memset(numareabackfaces, 0, sizeof(numareabackfaces));
	numfrontfaces = numbackfaces = 0;
	numfrontareas = numbackareas = 0;
	frontplanenum = backplanenum = -1;
	//add any adjacent areas with less presence types
	numareas = AAS_GetAdjacentAreasWithLessPresenceTypes_r(areanums, 0, areanum);
	//
	for (i = 0; i < numareas; i++)
	{
		area = &aasworld.areas[areanums[i]];
		for (j = 0; j < area->numfaces; j++)
		{
			facenum = abs(aasworld.faceindex[area->firstface + j]);
			face = &aasworld.faces[facenum];
			//if the face is solid
			if (face->faceflags & FACE_SOLID) continue;
			//check if the face is shared with one of the other areas
			for (k = 0; k < numareas; k++)
			{
				if (k == i) continue;
				if (face->frontarea == areanums[k] || face->backarea == areanums[k]) break;
			} //end for
			//if the face is shared
			if (k != numareas) continue;
			//the number of the area at the other side of the face
			if (face->frontarea == areanums[i]) otherareanum = face->backarea;
			else otherareanum = face->frontarea;
			//if the other area already is a cluter portal
			if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL) return 0;
			//number of the plane of the area
			faceplanenum = face->planenum & ~1;
			//
			if (frontplanenum < 0 || faceplanenum == frontplanenum)
			{
				frontplanenum = faceplanenum;
				frontfacenums[numfrontfaces++] = facenum;
				for (k = 0; k < numfrontareas; k++)
				{
					if (frontareanums[k] == otherareanum) break;
				} //end for
				if (k == numfrontareas) frontareanums[numfrontareas++] = otherareanum;
				numareafrontfaces[i]++;
			} //end if
			else if (backplanenum < 0 || faceplanenum == backplanenum)
			{
				backplanenum = faceplanenum;
				backfacenums[numbackfaces++] = facenum;
				for (k = 0; k < numbackareas; k++)
				{
					if (backareanums[k] == otherareanum) break;
				} //end for
				if (k == numbackareas) backareanums[numbackareas++] = otherareanum;
				numareabackfaces[i]++;
			} //end else
			else
			{
				return 0;
			} //end else
		} //end for
	} //end for
	//every area should have at least one front face and one back face
	for (i = 0; i < numareas; i++)
	{
		if (!numareafrontfaces[i] || !numareabackfaces[i]) return 0;
	} //end for
	//the front areas should all be connected
	if (!AAS_ConnectedAreas(frontareanums, numfrontareas)) return 0;
	//the back areas should all be connected
	if (!AAS_ConnectedAreas(backareanums, numbackareas)) return 0;
	//none of the front faces should have a shared edge with a back face
	for (i = 0; i < numfrontfaces; i++)
	{
		frontface = &aasworld.faces[frontfacenums[i]];
		for (fen = 0; fen < frontface->numedges; fen++)
		{
			frontedgenum = abs(aasworld.edgeindex[frontface->firstedge + fen]);
			for (j = 0; j < numbackfaces; j++)
			{
				backface = &aasworld.faces[backfacenums[j]];
				for (ben = 0; ben < backface->numedges; ben++)
				{
					backedgenum = abs(aasworld.edgeindex[backface->firstedge + ben]);
					if (frontedgenum == backedgenum) break;
				} //end for
				if (ben != backface->numedges) break;
			} //end for
			if (j != numbackfaces) break;
		} //end for
		if (fen != frontface->numedges) break;
	} //end for
	if (i != numfrontfaces) return 0;
	//set the cluster portal contents
	for (i = 0; i < numareas; i++)
	{
		aasworld.areasettings[areanums[i]].contents |= AREACONTENTS_CLUSTERPORTAL;
		//this area can be used as a route portal
		aasworld.areasettings[areanums[i]].contents |= AREACONTENTS_ROUTEPORTAL;
		Log_Write("possible portal: %d\r\n", areanums[i]);
	} //end for
	//
	return numareas;
} //end of the function AAS_CheckAreaForPossiblePortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FindPossiblePortals(void)
{
	int i, numpossibleportals;

	numpossibleportals = 0;
	for (i = 1; i < aasworld.numareas; i++)
	{
		numpossibleportals += AAS_CheckAreaForPossiblePortals(i);
	} //end for
	botimport.Print(PRT_MESSAGE, "\r%6d possible portal areas\n", numpossibleportals);
} //end of the function AAS_FindPossiblePortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveAllPortals(void)
{
	int i;

	for (i = 1; i < aasworld.numareas; i++)
	{
		aasworld.areasettings[i].contents &= ~AREACONTENTS_CLUSTERPORTAL;
	} //end for
} //end of the function AAS_RemoveAllPortals

#if 0
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FloodCluster_r(int areanum, int clusternum)
{
	int i, otherareanum;
	aas_face_t *face;
	aas_area_t *area;

	//set cluster mark
	aasworld.areasettings[areanum].cluster = clusternum;
	//if the area is a portal
	//if (aasworld.areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL) return;
	//
	area = &aasworld.areas[areanum];
	//use area faces to flood into adjacent areas
	for (i = 0; i < area->numfaces; i++)
	{
		face = &aasworld.faces[abs(aasworld.faceindex[area->firstface + i])];
		//
		if (face->frontarea != areanum) otherareanum = face->frontarea;
		else otherareanum = face->backarea;
		//if there's no area at the other side
		if (!otherareanum) continue;
		//if the area is a portal
		if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL) continue;
		//if the area is already marked
		if (aasworld.areasettings[otherareanum].cluster) continue;
		//
		AAS_FloodCluster_r(otherareanum, clusternum);
	} //end for
	//use the reachabilities to flood into other areas
	for (i = 0; i < aasworld.areasettings[areanum].numreachableareas; i++)
	{
		otherareanum = aasworld.reachability[
					aasworld.areasettings[areanum].firstreachablearea + i].areanum;
		if (!otherareanum)
		{
			continue;
			AAS_Error("reachability %d has zero area\n", aasworld.areasettings[areanum].firstreachablearea + i);
		} //end if
		//if the area is a portal
		if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL) continue;
		//if the area is already marked
		if (aasworld.areasettings[otherareanum].cluster) continue;
		//
		AAS_FloodCluster_r(otherareanum, clusternum);
	} //end for
} //end of the function AAS_FloodCluster_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveTeleporterPortals(void)
{
	int i, j, areanum;

	for (i = 1; i < aasworld.numareas; i++)
	{
		for (j = 0; j < aasworld.areasettings[i].numreachableareas; j++)
		{
			areanum = aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].areanum;
			if (aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].traveltype == TRAVEL_TELEPORT)
			{
				aasworld.areasettings[i].contents &= ~AREACONTENTS_CLUSTERPORTAL;
				aasworld.areasettings[areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
				break;
			} //end if
		} //end for
	} //end for
} //end of the function AAS_RemoveTeleporterPortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FloodClusterReachabilities(int clusternum)
{
	int i, j, areanum;

	for (i = 1; i < aasworld.numareas; i++)
	{
		//if this area already has a cluster set
		if (aasworld.areasettings[i].cluster) continue;
		//if this area is a cluster portal
		if (aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL) continue;
		//loop over the reachable areas from this area
		for (j = 0; j < aasworld.areasettings[i].numreachableareas; j++)
		{
			//the reachable area
			areanum = aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].areanum;
			//if this area is a cluster portal
			if (aasworld.areasettings[areanum].contents & AREACONTENTS_CLUSTERPORTAL) continue;
			//if this area has a cluster set
			if (aasworld.areasettings[areanum].cluster == clusternum)
			{
				AAS_FloodCluster_r(i, clusternum);
				i = 0;
				break;
			} //end if
		} //end for
	} //end for
} //end of the function AAS_FloodClusterReachabilities

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveNotClusterClosingPortals(void)
{
	int i, j, k, facenum, otherareanum, nonclosingportals;
	aas_area_t *area;
	aas_face_t *face;

	AAS_RemoveTeleporterPortals();
	//
	nonclosingportals = 0;
	for (i = 1; i < aasworld.numareas; i++)
	{
		if (!(aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)) continue;
		//find a non-portal area adjacent to the portal area and flood
		//the cluster from there
		area = &aasworld.areas[i];
		for (j = 0; j < area->numfaces; j++)
		{
			facenum = abs(aasworld.faceindex[area->firstface + j]);
			face = &aasworld.faces[facenum];
			//
			if (face->frontarea != i) otherareanum = face->frontarea;
			else otherareanum = face->backarea;
			//
			if (!otherareanum) continue;
			//
			if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL)
			{
				continue;
			} //end if
			//reset all cluster fields
			AAS_RemoveClusterAreas();
			//
			AAS_FloodCluster_r(otherareanum, 1);
			AAS_FloodClusterReachabilities(1);
			//check if all adjacent non-portal areas have a cluster set
			for (k = 0; k < area->numfaces; k++)
			{
				facenum = abs(aasworld.faceindex[area->firstface + k]);
				face = &aasworld.faces[facenum];
				//
				if (face->frontarea != i) otherareanum = face->frontarea;
				else otherareanum = face->backarea;
				//
				if (!otherareanum) continue;
				//
				if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL)
				{
					continue;
				} //end if
				//
				if (!aasworld.areasettings[otherareanum].cluster) break;
			} //end for
			//if all adjacent non-portal areas have a cluster set then the portal
			//didn't seal a cluster
			if (k >= area->numfaces)
			{
				aasworld.areasettings[i].contents &= ~AREACONTENTS_CLUSTERPORTAL;
				nonclosingportals++;
				//recheck all the other portals again
				i = 0;
				break;
			} //end if
		} //end for
	} //end for
	botimport.Print(PRT_MESSAGE, "\r%6d non closing portals removed\n", nonclosingportals);
} //end of the function AAS_RemoveNotClusterClosingPortals

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

void AAS_RemoveNotClusterClosingPortals(void)
{
	int i, j, facenum, otherareanum, nonclosingportals, numseperatedclusters;
	aas_area_t *area;
	aas_face_t *face;

	AAS_RemoveTeleporterPortals();
	//
	nonclosingportals = 0;
	for (i = 1; i < aasworld.numareas; i++)
	{
		if (!(aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)) continue;
		//
		numseperatedclusters = 0;
		//reset all cluster fields
		AAS_RemoveClusterAreas();
		//find a non-portal area adjacent to the portal area and flood
		//the cluster from there
		area = &aasworld.areas[i];
		for (j = 0; j < area->numfaces; j++)
		{
			facenum = abs(aasworld.faceindex[area->firstface + j]);
			face = &aasworld.faces[facenum];
			//
			if (face->frontarea != i) otherareanum = face->frontarea;
			else otherareanum = face->backarea;
			//if not solid at the other side of the face
			if (!otherareanum) continue;
			//don't flood into other portals
			if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL) continue;
			//if the area already has a cluster set
			if (aasworld.areasettings[otherareanum].cluster) continue;
			//another cluster is seperated by this portal
			numseperatedclusters++;
			//flood the cluster
			AAS_FloodCluster_r(otherareanum, numseperatedclusters);
			AAS_FloodClusterReachabilities(numseperatedclusters);
		} //end for
		//use the reachabilities to flood into other areas
		for (j = 0; j < aasworld.areasettings[i].numreachableareas; j++)
		{
			otherareanum = aasworld.reachability[
						aasworld.areasettings[i].firstreachablearea + j].areanum;
			//this should never be qtrue but we check anyway
			if (!otherareanum) continue;
			//don't flood into other portals
			if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL) continue;
			//if the area already has a cluster set
			if (aasworld.areasettings[otherareanum].cluster) continue;
			//another cluster is seperated by this portal
			numseperatedclusters++;
			//flood the cluster
			AAS_FloodCluster_r(otherareanum, numseperatedclusters);
			AAS_FloodClusterReachabilities(numseperatedclusters);
		} //end for
		//a portal must seperate no more and no less than 2 clusters
		if (numseperatedclusters != 2)
		{
			aasworld.areasettings[i].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			nonclosingportals++;
			//recheck all the other portals again
			i = 0;
		} //end if
	} //end for
	botimport.Print(PRT_MESSAGE, "\r%6d non closing portals removed\n", nonclosingportals);
} //end of the function AAS_RemoveNotClusterClosingPortals

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

void AAS_AddTeleporterPortals(void)
{
	int j, area2num, facenum, otherareanum;
	char *target, *targetname, *classname;
	bsp_entity_t *entities, *ent, *dest;
	vec3_t origin, destorigin, mins, maxs, end;
	vec3_t bbmins, bbmaxs;
	aas_area_t *area;
	aas_face_t *face;
	aas_trace_t trace;
	aas_link_t *areas, *link;

	entities = AAS_ParseBSPEntities();

	for (ent = entities; ent; ent = ent->next)
	{
		classname = AAS_ValueForBSPEpairKey(ent, "classname");
		if (classname && !strcmp(classname, "misc_teleporter"))
		{
			if (!AAS_VectorForBSPEpairKey(ent, "origin", origin))
			{
				botimport.Print(PRT_ERROR, "teleporter (%s) without origin\n", target);
				continue;
			} //end if
			//
			target = AAS_ValueForBSPEpairKey(ent, "target");
			if (!target)
			{
				botimport.Print(PRT_ERROR, "teleporter (%s) without target\n", target);
				continue;
			} //end if
			for (dest = entities; dest; dest = dest->next)
			{
				classname = AAS_ValueForBSPEpairKey(dest, "classname");
				if (classname && !strcmp(classname, "misc_teleporter_dest"))
				{
					targetname = AAS_ValueForBSPEpairKey(dest, "targetname");
					if (targetname && !strcmp(targetname, target))
					{
						break;
					} //end if
				} //end if
			} //end for
			if (!dest)
			{
				botimport.Print(PRT_ERROR, "teleporter without destination (%s)\n", target);
				continue;
			} //end if
			if (!AAS_VectorForBSPEpairKey(dest, "origin", destorigin))
			{
				botimport.Print(PRT_ERROR, "teleporter destination (%s) without origin\n", target);
				continue;
			} //end if
			destorigin[2] += 24; //just for q2e1m2, the dork has put the telepads in the ground
			VectorCopy(destorigin, end);
			end[2] -= 100;
			trace = AAS_TraceClientBBox(destorigin, end, PRESENCE_CROUCH, -1);
			if (trace.startsolid)
			{
				botimport.Print(PRT_ERROR, "teleporter destination (%s) in solid\n", target);
				continue;
			} //end if
			VectorCopy(trace.endpos, destorigin);
			area2num = AAS_PointAreaNum(destorigin);
			//reset all cluster fields
			for (j = 0; j < aasworld.numareas; j++)
			{
				aasworld.areasettings[j].cluster = 0;
			} //end for
			//
			VectorSet(mins, -8, -8, 8);
			VectorSet(maxs, 8, 8, 24);
			//
			AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, bbmins, bbmaxs);
			//
			VectorAdd(origin, mins, mins);
			VectorAdd(origin, maxs, maxs);
			//add bounding box size
			VectorSubtract(mins, bbmaxs, mins);
			VectorSubtract(maxs, bbmins, maxs);
			//link an invalid (-1) entity
			areas = AAS_AASLinkEntity(mins, maxs, -1);
			//
			for (link = areas; link; link = link->next_area)
			{
				if (!AAS_AreaGrounded(link->areanum)) continue;
				//add the teleporter portal mark
				aasworld.areasettings[link->areanum].contents |= AREACONTENTS_CLUSTERPORTAL |
																			AREACONTENTS_TELEPORTAL;
			} //end for
			//
			for (link = areas; link; link = link->next_area)
			{
				if (!AAS_AreaGrounded(link->areanum)) continue;
				//find a non-portal area adjacent to the portal area and flood
				//the cluster from there
				area = &aasworld.areas[link->areanum];
				for (j = 0; j < area->numfaces; j++)
				{
					facenum = abs(aasworld.faceindex[area->firstface + j]);
					face = &aasworld.faces[facenum];
					//
					if (face->frontarea != link->areanum) otherareanum = face->frontarea;
					else otherareanum = face->backarea;
					//
					if (!otherareanum) continue;
					//
					if (aasworld.areasettings[otherareanum].contents & AREACONTENTS_CLUSTERPORTAL)
					{
						continue;
					} //end if
					//
					AAS_FloodCluster_r(otherareanum, 1);
				} //end for
			} //end for
			//if the teleport destination IS in the same cluster
			if (aasworld.areasettings[area2num].cluster)
			{
				for (link = areas; link; link = link->next_area)
				{
					if (!AAS_AreaGrounded(link->areanum)) continue;
					//add the teleporter portal mark
					aasworld.areasettings[link->areanum].contents &= ~(AREACONTENTS_CLUSTERPORTAL |
																				AREACONTENTS_TELEPORTAL);
				} //end for
			} //end if
		} //end if
	} //end for
	AAS_FreeBSPEntities(entities);
} //end of the function AAS_AddTeleporterPortals

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AddTeleporterPortals(void)
{
	int i, j, areanum;

	for (i = 1; i < aasworld.numareas; i++)
	{
		for (j = 0; j < aasworld.areasettings[i].numreachableareas; j++)
		{
			if (aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].traveltype != TRAVEL_TELEPORT) continue;
			areanum = aasworld.reachability[aasworld.areasettings[i].firstreachablearea + j].areanum;
			aasworld.areasettings[areanum].contents |= AREACONTENTS_CLUSTERPORTAL;
		} //end for
	} //end for
} //end of the function AAS_AddTeleporterPortals

#endif

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TestPortals(void)
{
	int i;
	aas_portal_t *portal;

	for (i = 1; i < aasworld.numportals; i++)
	{
		portal = &aasworld.portals[i];
		if (!portal->frontcluster)
		{
			aasworld.areasettings[portal->areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			Log_Write("portal area %d has no front cluster\r\n", portal->areanum);
			return qfalse;
		} //end if
		if (!portal->backcluster)
		{
			aasworld.areasettings[portal->areanum].contents &= ~AREACONTENTS_CLUSTERPORTAL;
			Log_Write("portal area %d has no back cluster\r\n", portal->areanum);
			return qfalse;
		} //end if
	} //end for
	return qtrue;
} //end of the function
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CountForcedClusterPortals(void)
{
	int num, i;

	num = 0;
	for (i = 1; i < aasworld.numareas; i++)
	{
		if (aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			Log_Write("area %d is a forced portal area\r\n", i);
			num++;
		} //end if
	} //end for
	botimport.Print(PRT_MESSAGE, "%6d forced portal areas\n", num);
} //end of the function AAS_CountForcedClusterPortals
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateViewPortals(void)
{
	int i;

	for (i = 1; i < aasworld.numareas; i++)
	{
		if (aasworld.areasettings[i].contents & AREACONTENTS_CLUSTERPORTAL)
		{
			aasworld.areasettings[i].contents |= AREACONTENTS_VIEWPORTAL;
		} //end if
	} //end for
} //end of the function AAS_CreateViewPortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetViewPortalsAsClusterPortals(void)
{
	int i;

	for (i = 1; i < aasworld.numareas; i++)
	{
		if (aasworld.areasettings[i].contents & AREACONTENTS_VIEWPORTAL)
		{
			aasworld.areasettings[i].contents |= AREACONTENTS_CLUSTERPORTAL;
		} //end if
	} //end for
} //end of the function AAS_SetViewPortalsAsClusterPortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitClustering(void)
{
	int i, removedPortalAreas;
	int n, total, numreachabilityareas;

	if (!aasworld.loaded) return;
	//if there are clusters
	if (aasworld.numclusters >= 1)
	{
#ifndef BSPC
		//if clustering isn't forced
		if (!((int)LibVarGetValue("forceclustering")) &&
			!((int)LibVarGetValue("forcereachability"))) return;
#endif
	} //end if
	//set all view portals as cluster portals in case we re-calculate the reachabilities and clusters (with -reach)
	AAS_SetViewPortalsAsClusterPortals();
	//count the number of forced cluster portals
	AAS_CountForcedClusterPortals();
	//remove all area cluster marks
	AAS_RemoveClusterAreas();
	//find possible cluster portals
	AAS_FindPossiblePortals();
	//craete portals to for the bot view
	AAS_CreateViewPortals();
	//remove all portals that are not closing a cluster
	//AAS_RemoveNotClusterClosingPortals();
	//initialize portal memory
	if (aasworld.portals) FreeMemory(aasworld.portals);
	aasworld.portals = (aas_portal_t *) GetClearedMemory(AAS_MAX_PORTALS * sizeof(aas_portal_t));
	//initialize portal index memory
	if (aasworld.portalindex) FreeMemory(aasworld.portalindex);
	aasworld.portalindex = (aas_portalindex_t *) GetClearedMemory(AAS_MAX_PORTALINDEXSIZE * sizeof(aas_portalindex_t));
	//initialize cluster memory
	if (aasworld.clusters) FreeMemory(aasworld.clusters);
	aasworld.clusters = (aas_cluster_t *) GetClearedMemory(AAS_MAX_CLUSTERS * sizeof(aas_cluster_t));
	//
	removedPortalAreas = 0;
	botimport.Print(PRT_MESSAGE, "\r%6d removed portal areas", removedPortalAreas);
	while(1)
	{
		botimport.Print(PRT_MESSAGE, "\r%6d", removedPortalAreas);
		//initialize the number of portals and clusters
		aasworld.numportals = 1;		//portal 0 is a dummy
		aasworld.portalindexsize = 0;
		aasworld.numclusters = 1;		//cluster 0 is a dummy
		//create the portals from the portal areas
		AAS_CreatePortals();
		//
		removedPortalAreas++;
		//find the clusters
		if (!AAS_FindClusters())
			continue;
		//test the portals
		if (!AAS_TestPortals())
			continue;
		//
		break;
	} //end while
	botimport.Print(PRT_MESSAGE, "\n");
	//the AAS file should be saved
	aasworld.savefile = qtrue;
	//write the portal areas to the log file
	for (i = 1; i < aasworld.numportals; i++)
	{
		Log_Write("portal %d: area %d\r\n", i, aasworld.portals[i].areanum);
	} //end for
	// report cluster info
	botimport.Print(PRT_MESSAGE, "%6d portals created\n", aasworld.numportals);
	botimport.Print(PRT_MESSAGE, "%6d clusters created\n", aasworld.numclusters);
	for (i = 1; i < aasworld.numclusters; i++)
	{
		botimport.Print(PRT_MESSAGE, "cluster %d has %d reachability areas\n", i,
				aasworld.clusters[i].numreachabilityareas);
	} //end for
	// report AAS file efficiency
	numreachabilityareas = 0;
	total = 0;
	for (i = 0; i < aasworld.numclusters; i++) {
		n = aasworld.clusters[i].numreachabilityareas;
		numreachabilityareas += n;
		total += n * n;
	}
	total += numreachabilityareas * aasworld.numportals;
	//
	botimport.Print(PRT_MESSAGE, "%6i total reachability areas\n", numreachabilityareas);
	botimport.Print(PRT_MESSAGE, "%6i AAS memory/CPU usage (the lower the better)\n", total * 3);
} //end of the function AAS_InitClustering
