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


extern	int	c_nodes;

void RemovePortalFromNode (portal_t *portal, node_t *l);

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
}



/*
=============
FreeTreePortals_r
=============
*/
void FreeTreePortals_r (node_t *node)
{
	portal_t	*p, *nextp;
	int			s;

	// free children
	if (node->planenum != PLANENUM_LEAF)
	{
		FreeTreePortals_r (node->children[0]);
		FreeTreePortals_r (node->children[1]);
	}

	// free portals
	for (p=node->portals ; p ; p=nextp)
	{
		s = (p->nodes[1] == node);
		nextp = p->next[s];

		RemovePortalFromNode (p, p->nodes[!s]);
		FreePortal (p);
	}
	node->portals = NULL;
}

/*
=============
FreeTree_r
=============
*/
void FreeTree_r (node_t *node)
{
	// free children
	if (node->planenum != PLANENUM_LEAF)
	{
		FreeTree_r (node->children[0]);
		FreeTree_r (node->children[1]);
	}

	// free bspbrushes
	FreeBrushList (node->brushlist);

	// free the node
	if (node->volume)
		FreeBrush (node->volume);

	if (numthreads == 1)
		c_nodes--;
	free (node);
}


/*
=============
FreeTree
=============
*/
void FreeTree (tree_t *tree)
{
	FreeTreePortals_r (tree->headnode);
	FreeTree_r (tree->headnode);
	free (tree);
}

//===============================================================

void PrintTree_r (node_t *node, int depth)
{
	int		i;
	plane_t	*plane;
	bspbrush_t	*bb;

	for (i=0 ; i<depth ; i++)
		_printf ("  ");
	if (node->planenum == PLANENUM_LEAF)
	{
		if (!node->brushlist)
			_printf ("NULL\n");
		else
		{
			for (bb=node->brushlist ; bb ; bb=bb->next)
				_printf ("%i ", bb->original->brushnum);
			_printf ("\n");
		}
		return;
	}

	plane = &mapplanes[node->planenum];
	_printf ("#%i (%5.2f %5.2f %5.2f):%5.2f\n", node->planenum,
		plane->normal[0], plane->normal[1], plane->normal[2],
		plane->dist);
	PrintTree_r (node->children[0], depth+1);
	PrintTree_r (node->children[1], depth+1);
}
