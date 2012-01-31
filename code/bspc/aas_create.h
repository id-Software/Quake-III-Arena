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

#define AREA_PORTAL			1

//temporary AAS face
typedef struct tmp_face_s
{
	int num;						//face number
	int planenum;					//number of the plane the face is in
	winding_t *winding;				//winding of the face
	struct tmp_area_s *frontarea;	//area at the front of the face
	struct tmp_area_s *backarea;	//area at the back of the face
	int faceflags;					//flags of this face
	int aasfacenum;					//the number of the aas face used for this face
	//double link list pointers for front and back area
	struct tmp_face_s *prev[2], *next[2];
	//links in the list with faces
	struct tmp_face_s *l_prev, *l_next;
} tmp_face_t;

//temporary AAS area settings
typedef struct tmp_areasettings_s
{
	//could also add all kind of statistic fields
	int contents;					//contents of the area
	int modelnum;					//bsp model inside this area
	int areaflags;					//area flags
	int presencetype;				//how a bot can be present in this area
	int numreachableareas;			//number of reachable areas from this one
	int firstreachablearea;			//first reachable area in the reachable area index
} tmp_areasettings_t;

//temporary AAS area
typedef struct tmp_area_s
{
	int areanum;						//number of the area
	struct tmp_face_s *tmpfaces;		//the faces of the area
	int presencetype;					//presence type of the area
	int contents;						//area contents
	int modelnum;						//bsp model inside this area
	int invalid;						//true if the area is invalid
	tmp_areasettings_t *settings;		//area settings
	struct tmp_area_s *mergedarea;		//points to the new area after merging
										//when mergedarea != 0 the area has only the
										//seperating face of the merged areas
	int aasareanum;						//number of the aas area created for this tmp area
	//links in the list with areas
	struct tmp_area_s *l_prev, *l_next;
} tmp_area_t;

//temporary AAS node
typedef struct tmp_node_s
{
	int planenum;					//node plane number
	struct tmp_area_s *tmparea;		//points to an area if this node is an area
	struct tmp_node_s *children[2];	//child nodes of this node
} tmp_node_t;

#define NODEBUF_SIZE			128
//node buffer
typedef struct tmp_nodebuf_s
{
	int numnodes;
	struct tmp_nodebuf_s *next;
	tmp_node_t nodes[NODEBUF_SIZE];
} tmp_nodebuf_t;

//the whole temorary AAS
typedef struct tmp_aas_s
{
	//faces
	int numfaces;
	int facenum;
	tmp_face_t *faces;
	//areas
	int numareas;
	int areanum;
	tmp_area_t *areas;
	//area settings
	int numareasettings;
	tmp_areasettings_t *areasettings;
	//nodes
	int numnodes;
	tmp_node_t *nodes;
	//node buffer
	tmp_nodebuf_t *nodebuffer;
} tmp_aas_t;

extern tmp_aas_t tmpaasworld;

//creates a .AAS file with the given name from an already loaded map
void AAS_Create(char *aasfile);
//adds a face side to an area
void AAS_AddFaceSideToArea(tmp_face_t *tmpface, int side, tmp_area_t *tmparea);
//remvoes a face from an area
void AAS_RemoveFaceFromArea(tmp_face_t *tmpface, tmp_area_t *tmparea);
//allocate a tmp face
tmp_face_t *AAS_AllocTmpFace(void);
//free the tmp face
void AAS_FreeTmpFace(tmp_face_t *tmpface);
//allocate a tmp area
tmp_area_t *AAS_AllocTmpArea(void);
//free a tmp area
void AAS_FreeTmpArea(tmp_area_t *tmparea);
//allocate a tmp node
tmp_node_t *AAS_AllocTmpNode(void);
//free a tmp node
void AAS_FreeTmpNode(tmp_node_t *node);
//checks if an area is ok
void AAS_CheckArea(tmp_area_t *tmparea);
//flips the area faces where needed
void AAS_FlipAreaFaces(tmp_area_t *tmparea);
//returns true if the face is a gap seen from the given side
int AAS_GapFace(tmp_face_t *tmpface, int side);
//returns true if the face is a ground face
int AAS_GroundFace(tmp_face_t *tmpface);
