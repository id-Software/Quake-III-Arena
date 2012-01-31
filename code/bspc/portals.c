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

int		c_active_portals;
int		c_peak_portals;
int		c_boundary;
int		c_boundary_sides;
int		c_portalmemory;

//portal_t *portallist = NULL;
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
portal_t *AllocPortal (void)
{
	portal_t	*p;
	
	p = GetMemory(sizeof(portal_t));
	memset (p, 0, sizeof(portal_t));

	if (numthreads == 1)
	{
		c_active_portals++;
		if (c_active_portals > c_peak_portals)
		{
			c_peak_portals = c_active_portals;
		} //end if
		c_portalmemory += MemorySize(p);	
	} //end if

//	p->nextportal = portallist;
//	portallist = p;
	
	return p;
} //end of the function AllocPortal
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FreePortal (portal_t *p)
{
	if (p->winding) FreeWinding(p->winding);
	if (numthreads == 1)
	{
		c_active_portals--;
		c_portalmemory -= MemorySize(p);
	} //end if
	FreeMemory(p);
} //end of the function FreePortal
//===========================================================================
// Returns the single content bit of the
// strongest visible content present
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int VisibleContents (int contents)
{
	int		i;

	for (i=1 ; i<=LAST_VISIBLE_CONTENTS ; i<<=1)
		if (contents & i )
			return i;

	return 0;
} //end of the function VisibleContents
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int ClusterContents (node_t *node)
{
	int		c1, c2, c;

	if (node->planenum == PLANENUM_LEAF)
		return node->contents;

	c1 = ClusterContents(node->children[0]);
	c2 = ClusterContents(node->children[1]);
	c = c1|c2;

	// a cluster may include some solid detail areas, but
	// still be seen into
	if ( ! (c1&CONTENTS_SOLID) || ! (c2&CONTENTS_SOLID) )
		c &= ~CONTENTS_SOLID;
	return c;
} //end of the function ClusterContents

//===========================================================================
// Returns true if the portal is empty or translucent, allowing
// the PVS calculation to see through it.
// The nodes on either side of the portal may actually be clusters,
// not leaves, so all contents should be ored together
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean Portal_VisFlood (portal_t *p)
{
	int		c1, c2;

	if (!p->onnode)
		return false;	// to global outsideleaf

	c1 = ClusterContents(p->nodes[0]);
	c2 = ClusterContents(p->nodes[1]);

	if (!VisibleContents (c1^c2))
		return true;

	if (c1 & (CONTENTS_Q2TRANSLUCENT|CONTENTS_DETAIL))
		c1 = 0;
	if (c2 & (CONTENTS_Q2TRANSLUCENT|CONTENTS_DETAIL))
		c2 = 0;

	if ( (c1|c2) & CONTENTS_SOLID )
		return false;		// can't see through solid

	if (! (c1 ^ c2))
		return true;		// identical on both sides

	if (!VisibleContents (c1^c2))
		return true;
	return false;
} //end of the function Portal_VisFlood
//===========================================================================
// The entity flood determines which areas are
// "outside" on the map, which are then filled in.
// Flowing from side s to side !s
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean Portal_EntityFlood (portal_t *p, int s)
{
	if (p->nodes[0]->planenum != PLANENUM_LEAF
		|| p->nodes[1]->planenum != PLANENUM_LEAF)
		Error ("Portal_EntityFlood: not a leaf");

	// can never cross to a solid 
	if ( (p->nodes[0]->contents & CONTENTS_SOLID)
	|| (p->nodes[1]->contents & CONTENTS_SOLID) )
		return false;

	// can flood through everything else
	return true;
}


//=============================================================================

int		c_tinyportals;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AddPortalToNodes (portal_t *p, node_t *front, node_t *back)
{
	if (p->nodes[0] || p->nodes[1])
		Error ("AddPortalToNode: allready included");

	p->nodes[0] = front;
	p->next[0] = front->portals;
	front->portals = p;
	
	p->nodes[1] = back;
	p->next[1] = back->portals;
	back->portals = p;
} //end of the function AddPortalToNodes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemovePortalFromNode (portal_t *portal, node_t *l)
{
	portal_t	**pp, *t;

	int s, i, n;
	portal_t *p;
	portal_t *portals[4096];
	
// remove reference to the current portal
	pp = &l->portals;
	while (1)
	{
		t = *pp;
		if (!t)
			Error ("RemovePortalFromNode: portal not in leaf");	

		if ( t == portal )
			break;

		if (t->nodes[0] == l)
			pp = &t->next[0];
		else if (t->nodes[1] == l)
			pp = &t->next[1];
		else
			Error ("RemovePortalFromNode: portal not bounding leaf");
	}
	
	if (portal->nodes[0] == l)
	{
		*pp = portal->next[0];
		portal->nodes[0] = NULL;
	} //end if
	else if (portal->nodes[1] == l)
	{
		*pp = portal->next[1];	
		portal->nodes[1] = NULL;
	} //end else if
	else
	{
		Error("RemovePortalFromNode: mislinked portal");
	} //end else
//#ifdef ME
	n = 0;
	for (p = l->portals; p; p = p->next[s])
	{
		for (i = 0; i < n; i++)
		{
			if (p == portals[i]) Error("RemovePortalFromNode: circular linked\n");
		} //end for
		if (p->nodes[0] != l && p->nodes[1] != l)
		{
			Error("RemovePortalFromNodes: portal does not belong to node\n");
		} //end if
		portals[n] = p;
		s = (p->nodes[1] == l);
//		if (++n >= 4096) Error("RemovePortalFromNode: more than 4096 portals\n");
	} //end for
//#endif
} //end of the function RemovePortalFromNode
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void PrintPortal (portal_t *p)
{
	int			i;
	winding_t	*w;
	
	w = p->winding;
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.0f,%5.0f,%5.0f)\n",w->p[i][0]
		, w->p[i][1], w->p[i][2]);
} //end of the function PrintPortal
//===========================================================================
// The created portals will face the global outside_node
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#define	SIDESPACE	8

void MakeHeadnodePortals (tree_t *tree)
{
	vec3_t		bounds[2];
	int			i, j, n;
	portal_t	*p, *portals[6];
	plane_t		bplanes[6], *pl;
	node_t *node;

	node = tree->headnode;

// pad with some space so there will never be null volume leaves
	for (i=0 ; i<3 ; i++)
	{
		bounds[0][i] = tree->mins[i] - SIDESPACE;
		bounds[1][i] = tree->maxs[i] + SIDESPACE;
		if ( bounds[0][i] > bounds[1][i] ) {
			Error("empty BSP tree");
		}
	}
	
	tree->outside_node.planenum = PLANENUM_LEAF;
	tree->outside_node.brushlist = NULL;
	tree->outside_node.portals = NULL;
	tree->outside_node.contents = 0;

	for (i=0 ; i<3 ; i++)
		for (j=0 ; j<2 ; j++)
		{
			n = j*3 + i;

			p = AllocPortal ();
			portals[n] = p;
			
			pl = &bplanes[n];
			memset (pl, 0, sizeof(*pl));
			if (j)
			{
				pl->normal[i] = -1;
				pl->dist = -bounds[j][i];
			}
			else
			{
				pl->normal[i] = 1;
				pl->dist = bounds[j][i];
			}
			p->plane = *pl;
			p->winding = BaseWindingForPlane (pl->normal, pl->dist);
			AddPortalToNodes (p, node, &tree->outside_node);
		}
		
// clip the basewindings by all the other planes
	for (i=0 ; i<6 ; i++)
	{
		for (j=0 ; j<6 ; j++)
		{
			if (j == i) continue;
			ChopWindingInPlace (&portals[i]->winding, bplanes[j].normal, bplanes[j].dist, ON_EPSILON);
		} //end for
	} //end for
} //end of the function MakeHeadNodePortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#define	BASE_WINDING_EPSILON		0.001
#define	SPLIT_WINDING_EPSILON	0.001

winding_t *BaseWindingForNode (node_t *node)
{
	winding_t	*w;
	node_t		*n;
	plane_t		*plane;
	vec3_t		normal;
	vec_t		dist;

	w = BaseWindingForPlane (mapplanes[node->planenum].normal
		, mapplanes[node->planenum].dist);

	// clip by all the parents
	for (n=node->parent ; n && w ; )
	{
		plane = &mapplanes[n->planenum];

		if (n->children[0] == node)
		{	// take front
			ChopWindingInPlace (&w, plane->normal, plane->dist, BASE_WINDING_EPSILON);
		}
		else
		{	// take back
			VectorSubtract (vec3_origin, plane->normal, normal);
			dist = -plane->dist;
			ChopWindingInPlace (&w, normal, dist, BASE_WINDING_EPSILON);
		}
		node = n;
		n = n->parent;
	}

	return w;
} //end of the function BaseWindingForNode
//===========================================================================
// create the new portal by taking the full plane winding for the cutting
// plane and clipping it by all of parents of this node
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean WindingIsTiny (winding_t *w);

void MakeNodePortal (node_t *node)
{
	portal_t	*new_portal, *p;
	winding_t	*w;
	vec3_t		normal;
	float		dist;
	int			side;

	w = BaseWindingForNode (node);

	// clip the portal by all the other portals in the node
	for (p = node->portals; p && w; p = p->next[side])	
	{
		if (p->nodes[0] == node)
		{
			side = 0;
			VectorCopy (p->plane.normal, normal);
			dist = p->plane.dist;
		} //end if
		else if (p->nodes[1] == node)
		{
			side = 1;
			VectorSubtract (vec3_origin, p->plane.normal, normal);
			dist = -p->plane.dist;
		} //end else if
		else
		{
			Error ("MakeNodePortal: mislinked portal");
		} //end else
		ChopWindingInPlace (&w, normal, dist, 0.1);
	} //end for

	if (!w)
	{
		return;
	} //end if

	if (WindingIsTiny (w))
	{
		c_tinyportals++;
		FreeWinding(w);
		return;
	} //end if

#ifdef DEBUG
/* //NOTE: don't use this winding ok check
	// all the invalid windings only have a degenerate edge
	if (WindingError(w))
	{
		Log_Print("MakeNodePortal: %s\n", WindingErrorString());
		FreeWinding(w);
		return;
	} //end if*/
#endif //DEBUG


	new_portal = AllocPortal();
	new_portal->plane = mapplanes[node->planenum];

#ifdef ME
	new_portal->planenum = node->planenum;
#endif //ME

	new_portal->onnode = node;
	new_portal->winding = w;
	AddPortalToNodes (new_portal, node->children[0], node->children[1]);
} //end of the function MakeNodePortal
//===========================================================================
// Move or split the portals that bound node so that the node's
// children have portals instead of node.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void SplitNodePortals (node_t *node)
{
	portal_t	*p, *next_portal, *new_portal;
	node_t *f, *b, *other_node;
	int side;
	plane_t *plane;
	winding_t *frontwinding, *backwinding;

	plane = &mapplanes[node->planenum];
	f = node->children[0];
	b = node->children[1];

	for (p = node->portals ; p ; p = next_portal)	
	{
		if (p->nodes[0] == node) side = 0;
		else if (p->nodes[1] == node) side = 1;
		else Error ("SplitNodePortals: mislinked portal");
		next_portal = p->next[side];

		other_node = p->nodes[!side];
		RemovePortalFromNode (p, p->nodes[0]);
		RemovePortalFromNode (p, p->nodes[1]);

//
// cut the portal into two portals, one on each side of the cut plane
//
		ClipWindingEpsilon (p->winding, plane->normal, plane->dist,
				SPLIT_WINDING_EPSILON, &frontwinding, &backwinding);

		if (frontwinding && WindingIsTiny(frontwinding))
		{
			FreeWinding (frontwinding);
			frontwinding = NULL;
			c_tinyportals++;
		} //end if

		if (backwinding && WindingIsTiny(backwinding))
		{
			FreeWinding (backwinding);
			backwinding = NULL;
			c_tinyportals++;
		} //end if

#ifdef DEBUG
/* 	//NOTE: don't use this winding ok check
		// all the invalid windings only have a degenerate edge
		if (frontwinding && WindingError(frontwinding))
		{
			Log_Print("SplitNodePortals: front %s\n", WindingErrorString());
			FreeWinding(frontwinding);
			frontwinding = NULL;
		} //end if
		if (backwinding && WindingError(backwinding))
		{
			Log_Print("SplitNodePortals: back %s\n", WindingErrorString());
			FreeWinding(backwinding);
			backwinding = NULL;
		} //end if*/
#endif //DEBUG

		if (!frontwinding && !backwinding)
		{	// tiny windings on both sides
			continue;
		}

		if (!frontwinding)
		{
			FreeWinding (backwinding);
			if (side == 0) AddPortalToNodes (p, b, other_node);
			else AddPortalToNodes (p, other_node, b);
			continue;
		}
		if (!backwinding)
		{
			FreeWinding (frontwinding);
			if (side == 0) AddPortalToNodes (p, f, other_node);
			else AddPortalToNodes (p, other_node, f);
			continue;
		}
		
	// the winding is split
		new_portal = AllocPortal();
		*new_portal = *p;
		new_portal->winding = backwinding;
		FreeWinding (p->winding);
		p->winding = frontwinding;

		if (side == 0)
		{
			AddPortalToNodes (p, f, other_node);
			AddPortalToNodes (new_portal, b, other_node);
		} //end if
		else
		{
			AddPortalToNodes (p, other_node, f);
			AddPortalToNodes (new_portal, other_node, b);
		} //end else
	}

	node->portals = NULL;
} //end of the function SplitNodePortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void CalcNodeBounds (node_t *node)
{
	portal_t	*p;
	int			s;
	int			i;

	// calc mins/maxs for both leaves and nodes
	ClearBounds (node->mins, node->maxs);
	for (p = node->portals ; p ; p = p->next[s])	
	{
		s = (p->nodes[1] == node);
		for (i=0 ; i<p->winding->numpoints ; i++)
			AddPointToBounds (p->winding->p[i], node->mins, node->maxs);
	}
} //end of the function CalcNodeBounds
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int c_numportalizednodes;

void MakeTreePortals_r (node_t *node)
{
	int		i;

#ifdef ME
	qprintf("\r%6d", ++c_numportalizednodes);
	if (cancelconversion) return;
#endif //ME

	CalcNodeBounds (node);
	if (node->mins[0] >= node->maxs[0])
	{
		Log_Print("WARNING: node without a volume\n");
	}

	for (i=0 ; i<3 ; i++)
	{
		if (node->mins[i] < -MAX_MAP_BOUNDS || node->maxs[i] > MAX_MAP_BOUNDS)
		{
			Log_Print("WARNING: node with unbounded volume\n");
			break;
		}
	}
	if (node->planenum == PLANENUM_LEAF)
		return;

	MakeNodePortal (node);
	SplitNodePortals (node);

	MakeTreePortals_r (node->children[0]);
	MakeTreePortals_r (node->children[1]);
} //end of the function MakeTreePortals_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void MakeTreePortals(tree_t *tree)
{

#ifdef ME
	Log_Print("---- Node Portalization ----\n");
	c_numportalizednodes = 0;
	c_portalmemory = 0;
	qprintf("%6d nodes portalized", c_numportalizednodes);
#endif //ME

	MakeHeadnodePortals(tree);
	MakeTreePortals_r(tree->headnode);

#ifdef ME
	qprintf("\n");
	Log_Write("%6d nodes portalized\r\n", c_numportalizednodes);
	Log_Print("%6d tiny portals\r\n", c_tinyportals);
	Log_Print("%6d KB of portal memory\r\n", c_portalmemory >> 10);
	Log_Print("%6i KB of winding memory\r\n", WindingMemory() >> 10);
#endif //ME
} //end of the function MakeTreePortals

/*
=========================================================

FLOOD ENTITIES

=========================================================
*/
//#define P_NODESTACK

node_t *p_firstnode;
node_t *p_lastnode;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifdef P_NODESTACK
void P_AddNodeToList(node_t *node)
{
	node->next = p_firstnode;
	p_firstnode = node;
	if (!p_lastnode) p_lastnode = node;
} //end of the function P_AddNodeToList
#else //it's a node queue
//add the node to the end of the node list
void P_AddNodeToList(node_t *node)
{
	node->next = NULL;
	if (p_lastnode) p_lastnode->next = node;
	else p_firstnode = node;
	p_lastnode = node;
} //end of the function P_AddNodeToList
#endif //P_NODESTACK
//===========================================================================
// get the first node from the front of the node list
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
node_t *P_NextNodeFromList(void)
{
	node_t *node;

	node = p_firstnode;
	if (p_firstnode) p_firstnode = p_firstnode->next;
	if (!p_firstnode) p_lastnode = NULL;
	return node;
} //end of the function P_NextNodeFromList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FloodPortals(node_t *firstnode)
{
	node_t *node;
	portal_t *p;
	int s;

	firstnode->occupied = 1;
	P_AddNodeToList(firstnode);

	for (node = P_NextNodeFromList(); node; node = P_NextNodeFromList())
	{
		for (p = node->portals; p; p = p->next[s])
		{
			s = (p->nodes[1] == node);
			//if the node at the other side of the portal is occupied already
			if (p->nodes[!s]->occupied) continue;
			//if it isn't possible to flood through this portal
			if (!Portal_EntityFlood(p, s)) continue;
			//
			p->nodes[!s]->occupied = node->occupied + 1;
			//
			P_AddNodeToList(p->nodes[!s]);
		} //end for
	} //end for
} //end of the function FloodPortals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int numrec;

void FloodPortals_r (node_t *node, int dist)
{
	portal_t *p;
	int s;
//	int i;

	Log_Print("\r%6d", ++numrec);

	if (node->occupied) Error("FloodPortals_r: node already occupied\n");
	if (!node)
	{
		Error("FloodPortals_r: NULL node\n");
	} //end if*/
	node->occupied = dist;

	for (p = node->portals; p; p = p->next[s])
	{
		s = (p->nodes[1] == node);
		//if the node at the other side of the portal is occupied already
		if (p->nodes[!s]->occupied) continue;
		//if it isn't possible to flood through this portal
		if (!Portal_EntityFlood(p, s)) continue;
		//flood recursively through the current portal
		FloodPortals_r(p->nodes[!s], dist+1);
	} //end for
	Log_Print("\r%6d", --numrec);
} //end of the function FloodPortals_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean PlaceOccupant (node_t *headnode, vec3_t origin, entity_t *occupant)
{
	node_t *node;
	vec_t	d;
	plane_t *plane;

	//find the leaf to start in
	node = headnode;
	while(node->planenum != PLANENUM_LEAF)
	{
		if (node->planenum < 0 || node->planenum > nummapplanes)
		{
			Error("PlaceOccupant: invalid node->planenum\n");
		} //end if
		plane = &mapplanes[node->planenum];
		d = DotProduct(origin, plane->normal) - plane->dist;
		if (d >= 0) node = node->children[0];
		else node = node->children[1];
		if (!node)
		{
			Error("PlaceOccupant: invalid child %d\n", d < 0);
		} //end if
	} //end while
	//don't start in solid
//	if (node->contents == CONTENTS_SOLID)
	//ME: replaced because in LeafNode in brushbsp.c
	//    some nodes have contents solid with other contents
	if (node->contents & CONTENTS_SOLID) return false;
	//if the node is already occupied
	if (node->occupied) return false;

	//place the occupant in the first leaf
	node->occupant = occupant;

	numrec = 0;
//	Log_Print("%6d recurses", numrec);
//	FloodPortals_r(node, 1);
//	Log_Print("\n");
	FloodPortals(node);

	return true;
} //end of the function PlaceOccupant
//===========================================================================
// Marks all nodes that can be reached by entites
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean FloodEntities (tree_t *tree)
{
	int i;
	int x, y;
	vec3_t origin;
	char *cl;
	qboolean inside;
	node_t *headnode;

	headnode = tree->headnode;
	Log_Print("------ FloodEntities -------\n");
	inside = false;
	tree->outside_node.occupied = 0;

	//start at entity 1 not the world ( = 0)
	for (i = 1; i < num_entities; i++)
	{
		GetVectorForKey(&entities[i], "origin", origin);
		if (VectorCompare(origin, vec3_origin)) continue;

		cl = ValueForKey(&entities[i], "classname");
		origin[2] += 1;	//so objects on floor are ok

//		Log_Print("flooding from entity %d: %s\n", i, cl);
		//nudge playerstart around if needed so clipping hulls allways
		//have a valid point
		if (!strcmp(cl, "info_player_start"))
		{
			for (x = -16; x <= 16; x += 16)
			{
				for (y = -16; y <= 16; y += 16)
				{
					origin[0] += x;
					origin[1] += y;
					if (PlaceOccupant(headnode, origin, &entities[i]))
					{
						inside = true;
						x = 999; //stop for this info_player_start
						break;
					} //end if
					origin[0] -= x;
					origin[1] -= y;
				} //end for
			} //end for
		} //end if
		else
		{
			if (PlaceOccupant(headnode, origin, &entities[i]))
			{
				inside = true;
			} //end if
		} //end else
	} //end for

	if (!inside)
	{
		Log_Print("WARNING: no entities inside\n");
	} //end if
	else if (tree->outside_node.occupied)
	{
		Log_Print("WARNING: entity reached from outside\n");
	} //end else if

	return (qboolean)(inside && !tree->outside_node.occupied);
} //end of the function FloodEntities

/*
=========================================================

FILL OUTSIDE

=========================================================
*/

int c_outside;
int c_inside;
int c_solid;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FillOutside_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FillOutside_r (node->children[0]);
		FillOutside_r (node->children[1]);
		return;
	} //end if
	// anything not reachable by an entity
	// can be filled away (by setting solid contents)
	if (!node->occupied)
	{
		if (!(node->contents & CONTENTS_SOLID))
		{
			c_outside++;
			node->contents |= CONTENTS_SOLID;
		} //end if
		else
		{
			c_solid++;
		} //end else
	} //end if
	else
	{
		c_inside++;
	} //end else
} //end of the function FillOutside_r
//===========================================================================
// Fill all nodes that can't be reached by entities
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FillOutside (node_t *headnode)
{
	c_outside = 0;
	c_inside = 0;
	c_solid = 0;
	Log_Print("------- FillOutside --------\n");
	FillOutside_r (headnode);
	Log_Print("%5i solid leaves\n", c_solid);
	Log_Print("%5i leaves filled\n", c_outside);
	Log_Print("%5i inside leaves\n", c_inside);
} //end of the function FillOutside

/*
=========================================================

FLOOD AREAS

=========================================================
*/

int		c_areas;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FloodAreas_r (node_t *node)
{
	portal_t	*p;
	int			s;
	bspbrush_t	*b;
	entity_t	*e;

	if (node->contents == CONTENTS_AREAPORTAL)
	{
		// this node is part of an area portal
		b = node->brushlist;
		e = &entities[b->original->entitynum];

		// if the current area has allready touched this
		// portal, we are done
		if (e->portalareas[0] == c_areas || e->portalareas[1] == c_areas)
			return;

		// note the current area as bounding the portal
		if (e->portalareas[1])
		{
			Log_Print("WARNING: areaportal entity %i touches > 2 areas\n", b->original->entitynum);
			return;
		}
		if (e->portalareas[0])
			e->portalareas[1] = c_areas;
		else
			e->portalareas[0] = c_areas;

		return;
	} //end if

	if (node->area)
		return;		// allready got it
	node->area = c_areas;

	for (p=node->portals ; p ; p = p->next[s])
	{
		s = (p->nodes[1] == node);
#if 0
		if (p->nodes[!s]->occupied)
			continue;
#endif
		if (!Portal_EntityFlood (p, s))
			continue;

		FloodAreas_r (p->nodes[!s]);
	} //end for
} //end of the function FloodAreas_r
//===========================================================================
// Just decend the tree, and for each node that hasn't had an
// area set, flood fill out from there
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FindAreas_r (node_t *node)
{
	if (node->planenum != PLANENUM_LEAF)
	{
		FindAreas_r (node->children[0]);
		FindAreas_r (node->children[1]);
		return;
	}

	if (node->area)
		return;		// allready got it

	if (node->contents & CONTENTS_SOLID)
		return;

	if (!node->occupied)
		return;			// not reachable by entities

	// area portals are allways only flooded into, never
	// out of
	if (node->contents == CONTENTS_AREAPORTAL)
		return;

	c_areas++;
	FloodAreas_r (node);
} //end of the function FindAreas_r
//===========================================================================
// Just decend the tree, and for each node that hasn't had an
// area set, flood fill out from there
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void SetAreaPortalAreas_r (node_t *node)
{
	bspbrush_t	*b;
	entity_t	*e;

	if (node->planenum != PLANENUM_LEAF)
	{
		SetAreaPortalAreas_r (node->children[0]);
		SetAreaPortalAreas_r (node->children[1]);
		return;
	} //end if

	if (node->contents == CONTENTS_AREAPORTAL)
	{
		if (node->area)
			return;		// allready set

		b = node->brushlist;
		e = &entities[b->original->entitynum];
		node->area = e->portalareas[0];
		if (!e->portalareas[1])
		{
			Log_Print("WARNING: areaportal entity %i doesn't touch two areas\n", b->original->entitynum);
			return;
		} //end if
	} //end if
} //end of the function SetAreaPortalAreas_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
/*
void EmitAreaPortals(node_t *headnode)
{
	int				i, j;
	entity_t		*e;
	dareaportal_t	*dp;

	if (c_areas > MAX_MAP_AREAS)
		Error ("MAX_MAP_AREAS");
	numareas = c_areas+1;
	numareaportals = 1;		// leave 0 as an error

	for (i=1 ; i<=c_areas ; i++)
	{
		dareas[i].firstareaportal = numareaportals;
		for (j=0 ; j<num_entities ; j++)
		{
			e = &entities[j];
			if (!e->areaportalnum)
				continue;
			dp = &dareaportals[numareaportals];
			if (e->portalareas[0] == i)
			{
				dp->portalnum = e->areaportalnum;
				dp->otherarea = e->portalareas[1];
				numareaportals++;
			} //end if
			else if (e->portalareas[1] == i)
			{
				dp->portalnum = e->areaportalnum;
				dp->otherarea = e->portalareas[0];
				numareaportals++;
			} //end else if
		} //end for
		dareas[i].numareaportals = numareaportals - dareas[i].firstareaportal;
	} //end for

	Log_Print("%5i numareas\n", numareas);
	Log_Print("%5i numareaportals\n", numareaportals);
} //end of the function EmitAreaPortals
*/
//===========================================================================
// Mark each leaf with an area, bounded by CONTENTS_AREAPORTAL
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FloodAreas (tree_t *tree)
{
	Log_Print("--- FloodAreas ---\n");
	FindAreas_r (tree->headnode);
	SetAreaPortalAreas_r (tree->headnode);
	Log_Print("%5i areas\n", c_areas);
} //end of the function FloodAreas
//===========================================================================
// Finds a brush side to use for texturing the given portal
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FindPortalSide (portal_t *p)
{
	int			viscontents;
	bspbrush_t	*bb;
	mapbrush_t	*brush;
	node_t		*n;
	int			i,j;
	int			planenum;
	side_t		*side, *bestside;
	float		dot, bestdot;
	plane_t		*p1, *p2;

	// decide which content change is strongest
	// solid > lava > water, etc
	viscontents = VisibleContents (p->nodes[0]->contents ^ p->nodes[1]->contents);
	if (!viscontents)
		return;

	planenum = p->onnode->planenum;
	bestside = NULL;
	bestdot = 0;

	for (j=0 ; j<2 ; j++)
	{
		n = p->nodes[j];
		p1 = &mapplanes[p->onnode->planenum];
		for (bb=n->brushlist ; bb ; bb=bb->next)
		{
			brush = bb->original;
			if ( !(brush->contents & viscontents) )
				continue;
			for (i=0 ; i<brush->numsides ; i++)
			{
				side = &brush->original_sides[i];
				if (side->flags & SFL_BEVEL)
					continue;
				if (side->texinfo == TEXINFO_NODE)
					continue;		// non-visible
				if ((side->planenum&~1) == planenum)
				{	// exact match
					bestside = &brush->original_sides[i];
					goto gotit;
				} //end if
				// see how close the match is
				p2 = &mapplanes[side->planenum&~1];
				dot = DotProduct (p1->normal, p2->normal);
				if (dot > bestdot)
				{
					bestdot = dot;
					bestside = side;
				} //end if
			} //end for
		} //end for
	} //end for

gotit:
	if (!bestside)
		Log_Print("WARNING: side not found for portal\n");

	p->sidefound = true;
	p->side = bestside;
} //end of the function FindPortalSide
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void MarkVisibleSides_r (node_t *node)
{
	portal_t *p;
	int s;

	if (node->planenum != PLANENUM_LEAF)
	{
		MarkVisibleSides_r (node->children[0]);
		MarkVisibleSides_r (node->children[1]);
		return;
	} //end if

	// empty leaves are never boundary leaves
	if (!node->contents) return;

	// see if there is a visible face
	for (p=node->portals ; p ; p = p->next[!s])
	{
		s = (p->nodes[0] == node);
		if (!p->onnode)
			continue;		// edge of world
		if (!p->sidefound)
			FindPortalSide (p);
		if (p->side)
			p->side->flags |= SFL_VISIBLE;
	} //end for
} //end of the function MarkVisibleSides_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void MarkVisibleSides(tree_t *tree, int startbrush, int endbrush)
{
	int		i, j;
	mapbrush_t	*mb;
	int		numsides;

	Log_Print("--- MarkVisibleSides ---\n");

	// clear all the visible flags
	for (i=startbrush ; i<endbrush ; i++)
	{
		mb = &mapbrushes[i];

		numsides = mb->numsides;
		for (j=0 ; j<numsides ; j++)
			mb->original_sides[j].flags &= ~SFL_VISIBLE;
	}

	// set visible flags on the sides that are used by portals
	MarkVisibleSides_r (tree->headnode);
} //end of the function MarkVisibleSides

