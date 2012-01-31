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

extern int c_nodes;
int	c_pruned;
int freedtreemem = 0;

void RemovePortalFromNode (portal_t *portal, node_t *l);

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
node_t *NodeForPoint (node_t *node, vec3_t origin)
{
	plane_t	*plane;
	vec_t	d;

	while (node->planenum != PLANENUM_LEAF)
	{
		plane = &mapplanes[node->planenum];
		d = DotProduct (origin, plane->normal) - plane->dist;
		if (d >= 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	return node;
} //end of the function NodeForPoint
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Tree_FreePortals_r (node_t *node)
{
	portal_t	*p, *nextp;
	int			s;

	// free children
	if (node->planenum != PLANENUM_LEAF)
	{
		Tree_FreePortals_r(node->children[0]);
		Tree_FreePortals_r(node->children[1]);
	}

	// free portals
	for (p = node->portals; p; p = nextp)
	{
		s = (p->nodes[1] == node);
		nextp = p->next[s];

		RemovePortalFromNode (p, p->nodes[!s]);
#ifdef ME
		if (p->winding) freedtreemem += MemorySize(p->winding);
		freedtreemem += MemorySize(p);
#endif //ME
		FreePortal(p);
	}
	node->portals = NULL;
} //end of the function Tree_FreePortals_r
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Tree_Free_r (node_t *node)
{
//	face_t *f, *nextf;
	bspbrush_t *brush, *nextbrush;

	//free children
	if (node->planenum != PLANENUM_LEAF)
	{
		Tree_Free_r (node->children[0]);
		Tree_Free_r (node->children[1]);
	} //end if
	//free bspbrushes
//	FreeBrushList (node->brushlist);
	for (brush = node->brushlist; brush; brush = nextbrush)
	{
		nextbrush = brush->next;
#ifdef ME
		freedtreemem += MemorySize(brush);
#endif //ME
		FreeBrush(brush);
	} //end for
	node->brushlist = NULL;

	/*
	NOTE: only used when creating Q2 bsp
	// free faces
	for (f = node->faces; f; f = nextf)
	{
		nextf = f->next;
#ifdef ME
		if (f->w) freedtreemem += MemorySize(f->w);
		freedtreemem += sizeof(face_t);
#endif //ME
		FreeFace(f);
	} //end for
	*/

	// free the node
	if (node->volume)
	{
#ifdef ME
		freedtreemem += MemorySize(node->volume);
#endif //ME
		FreeBrush (node->volume);
	} //end if

	if (numthreads == 1) c_nodes--;
#ifdef ME
	freedtreemem += MemorySize(node);
#endif //ME
	FreeMemory(node);
} //end of the function Tree_Free_r
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Tree_Free(tree_t *tree)
{
	//if no tree just return
	if (!tree) return;
	//
	freedtreemem = 0;
	//
	Tree_FreePortals_r(tree->headnode);
	Tree_Free_r(tree->headnode);
#ifdef ME
	freedtreemem += MemorySize(tree);
#endif //ME
	FreeMemory(tree);
#ifdef ME
	Log_Print("freed ");
	PrintMemorySize(freedtreemem);
	Log_Print(" of tree memory\n");
#endif //ME
} //end of the function Tree_Free
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
tree_t *Tree_Alloc(void)
{
	tree_t	*tree;

	tree = GetMemory(sizeof(*tree));
	memset (tree, 0, sizeof(*tree));
	ClearBounds (tree->mins, tree->maxs);

	return tree;
} //end of the function Tree_Alloc
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Tree_Print_r (node_t *node, int depth)
{
	int		i;
	plane_t	*plane;
	bspbrush_t	*bb;

	for (i=0 ; i<depth ; i++)
		printf ("  ");
	if (node->planenum == PLANENUM_LEAF)
	{
		if (!node->brushlist)
			printf ("NULL\n");
		else
		{
			for (bb=node->brushlist ; bb ; bb=bb->next)
				printf ("%i ", bb->original->brushnum);
			printf ("\n");
		}
		return;
	}

	plane = &mapplanes[node->planenum];
	printf ("#%i (%5.2f %5.2f %5.2f):%5.2f\n", node->planenum,
		plane->normal[0], plane->normal[1], plane->normal[2],
		plane->dist);
	Tree_Print_r (node->children[0], depth+1);
	Tree_Print_r (node->children[1], depth+1);
} //end of the function Tree_Print_r
//===========================================================================
// NODES THAT DON'T SEPERATE DIFFERENT CONTENTS CAN BE PRUNED
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Tree_PruneNodes_r (node_t *node)
{
	bspbrush_t *b, *next;

	if (node->planenum == PLANENUM_LEAF) return;

	Tree_PruneNodes_r (node->children[0]);
	Tree_PruneNodes_r (node->children[1]);

	if (create_aas)
	{
		if ((node->children[0]->contents & CONTENTS_LADDER) ||
				(node->children[1]->contents & CONTENTS_LADDER)) return;
	}

	if ((node->children[0]->contents & CONTENTS_SOLID)
		&& (node->children[1]->contents & CONTENTS_SOLID))
	{
		if (node->faces)
			Error ("node->faces seperating CONTENTS_SOLID");
		if (node->children[0]->faces || node->children[1]->faces)
			Error ("!node->faces with children");
		// FIXME: free stuff
		node->planenum = PLANENUM_LEAF;
		node->contents = CONTENTS_SOLID;
		node->detail_seperator = false;

		if (node->brushlist)
			Error ("PruneNodes: node->brushlist");
		// combine brush lists
		node->brushlist = node->children[1]->brushlist;

		for (b = node->children[0]->brushlist; b; b = next)
		{
			next = b->next;
			b->next = node->brushlist;
			node->brushlist = b;
		} //end for
		//free the child nodes
		FreeMemory(node->children[0]);
		FreeMemory(node->children[1]);
		//two nodes are cut away
		c_pruned += 2;
	} //end if
} //end of the function Tree_PruneNodes_r
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void Tree_PruneNodes(node_t *node)
{
	Log_Print("------- Prune Nodes --------\n");
	c_pruned = 0;
	Tree_PruneNodes_r(node);
	Log_Print("%5i pruned nodes\n", c_pruned);
} //end of the function Tree_PruneNodes
