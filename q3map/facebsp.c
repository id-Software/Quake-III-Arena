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


int			c_faceLeafs;


/*
================
AllocBspFace
================
*/
bspface_t	*AllocBspFace( void ) {
	bspface_t	*f;

	f = malloc(sizeof(*f));
	memset( f, 0, sizeof(*f) );

	return f;
}

/*
================
FreeBspFace
================
*/
void	FreeBspFace( bspface_t *f ) {
	if ( f->w ) {
		FreeWinding( f->w );
	}
	free( f );
}


/*
================
SelectSplitPlaneNum
================
*/
int hintsplit;

#define	BLOCK_SIZE	1024
int SelectSplitPlaneNum( node_t *node, bspface_t *list ) {
	bspface_t	*split;
	bspface_t	*check;
	bspface_t	*bestSplit;
	int			splits, facing, front, back;
	int			side;
	plane_t		*plane;
	int			value, bestValue;
	int			i;
	vec3_t		normal;
	float		dist;
	int			planenum;

	hintsplit = qfalse;
	// if it is crossing a 1k block boundary, force a split
	for ( i = 0 ; i < 2 ; i++ ) {
		dist = BLOCK_SIZE * ( floor( node->mins[i] / BLOCK_SIZE ) + 1 );	
		if ( node->maxs[i] > dist ) {
			VectorClear( normal );
			normal[i] = 1;
			planenum = FindFloatPlane( normal, dist );
			return planenum;
		}
	}

	// pick one of the face planes
	bestValue = -99999;
	bestSplit = list;

	for ( split = list ; split ; split = split->next ) {
		split->checked = qfalse;
	}

	for ( split = list ; split ; split = split->next ) {
		if ( split->checked ) {
			continue;
		}
		plane = &mapplanes[ split->planenum ];
		splits = 0;
		facing = 0;
		front = 0;
		back = 0;
		for ( check = list ; check ; check = check->next ) {
			if ( check->planenum == split->planenum ) {
				facing++;
				check->checked = qtrue;	// won't need to test this plane again
				continue;
			}
			side = WindingOnPlaneSide( check->w, plane->normal, plane->dist );
			if ( side == SIDE_CROSS ) {
				splits++;
			} else if ( side == SIDE_FRONT ) {
				front++;
			} else if ( side == SIDE_BACK ) {
				back++;
			}
		}
		value =  5*facing - 5*splits; // - abs(front-back);
		if ( plane->type < 3 ) {
			value+=5;		// axial is better
		}
		value += split->priority;		// prioritize hints higher

		if ( value > bestValue ) {
			bestValue = value;
			bestSplit = split;
		}
	}

	if ( bestValue == -99999 ) {
		return -1;
	}

	if (bestSplit->hint)
		hintsplit = qtrue;

	return bestSplit->planenum;
}

int	CountFaceList( bspface_t *list ) {
	int		c;
	c = 0;
	for ( ; list ; list = list->next ) {
		c++;
	}
	return c;
}

/*
================
BuildFaceTree_r
================
*/
void	BuildFaceTree_r( node_t *node, bspface_t *list ) {
	bspface_t	*split;
	bspface_t	*next;
	int			side;
	plane_t		*plane;
	bspface_t	*newFace;
	bspface_t	*childLists[2];
	winding_t	*frontWinding, *backWinding;
	int			i;
	int			splitPlaneNum;

	i = CountFaceList( list );

	splitPlaneNum = SelectSplitPlaneNum( node, list );
	// if we don't have any more faces, this is a node
	if ( splitPlaneNum == -1 ) {
		node->planenum = PLANENUM_LEAF;
		c_faceLeafs++;
		return;
	}

	// partition the list
	node->planenum = splitPlaneNum;
	node->hint = hintsplit;
	plane = &mapplanes[ splitPlaneNum ];
	childLists[0] = NULL;
	childLists[1] = NULL;
	for ( split = list ; split ; split = next ) {
		next = split->next;

		if ( split->planenum == node->planenum ) {
			FreeBspFace( split );
			continue;
		}

		side = WindingOnPlaneSide( split->w, plane->normal, plane->dist );

		if ( side == SIDE_CROSS ) {
			ClipWindingEpsilon( split->w, plane->normal, plane->dist, CLIP_EPSILON * 2,
				&frontWinding, &backWinding );
			if ( frontWinding ) {
				newFace = AllocBspFace();
				newFace->w = frontWinding;
				newFace->next = childLists[0];
				newFace->planenum = split->planenum;
				newFace->priority = split->priority;
				newFace->hint = split->hint;
				childLists[0] = newFace;
			}
			if ( backWinding ) {
				newFace = AllocBspFace();
				newFace->w = backWinding;
				newFace->next = childLists[1];
				newFace->planenum = split->planenum;
				newFace->priority = split->priority;
				newFace->hint = split->hint;
				childLists[1] = newFace;
			}
			FreeBspFace( split );
		} else if ( side == SIDE_FRONT ) {
			split->next = childLists[0];
			childLists[0] = split;
		} else if ( side == SIDE_BACK ) {
			split->next = childLists[1];
			childLists[1] = split;
		}
	}


	// recursively process children
	for ( i = 0 ; i < 2 ; i++ ) {
		node->children[i] = AllocNode();
		node->children[i]->parent = node;
		VectorCopy( node->mins, node->children[i]->mins );
		VectorCopy( node->maxs, node->children[i]->maxs );
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( plane->normal[i] == 1 ) {
			node->children[0]->mins[i] = plane->dist;
			node->children[1]->maxs[i] = plane->dist;
			break;
		}
	}

	for ( i = 0 ; i < 2 ; i++ ) {
		BuildFaceTree_r ( node->children[i], childLists[i]);
	}
}


/*
================
FaceBSP

List will be freed before returning
================
*/
tree_t *FaceBSP( bspface_t *list ) {
	tree_t		*tree;
	bspface_t	*face;
	int			i;
	int			count;

	qprintf( "--- FaceBSP ---\n" );

	tree = AllocTree ();

	count = 0;
	for ( face = list ; face ; face = face->next ) {
		count++;
		for ( i = 0 ; i < face->w->numpoints ; i++ ) {
			AddPointToBounds( face->w->p[i], tree->mins, tree->maxs);
		}
	}
	qprintf( "%5i faces\n", count );

	tree->headnode = AllocNode();
	VectorCopy( tree->mins, tree->headnode->mins );
	VectorCopy( tree->maxs, tree->headnode->maxs );
	c_faceLeafs = 0;

	BuildFaceTree_r ( tree->headnode, list );

	qprintf( "%5i leafs\n", c_faceLeafs );

	return tree;
}


/*
=================
BspFaceForPortal
=================
*/
bspface_t *BspFaceForPortal( portal_t *p ) {
	bspface_t	*f;

	f = AllocBspFace();
	f->w = CopyWinding( p->winding );
	f->planenum = p->onnode->planenum & ~1;

	return f;
}



/*
=================
MakeStructuralBspFaceList
=================
*/
bspface_t	*MakeStructuralBspFaceList( bspbrush_t *list ) {
	bspbrush_t	*b;
	int			i;
	side_t		*s;
	winding_t	*w;
	bspface_t	*f, *flist;

	flist = NULL;
	for ( b = list ; b ; b = b->next ) {
		if ( b->detail ) {
			continue;
		}
		for ( i = 0 ; i < b->numsides ; i++ ) {
			s = &b->sides[i];
			w = s->winding;
			if ( !w ) {
				continue;
			}
			f = AllocBspFace();
			f->w = CopyWinding( w );
			f->planenum = s->planenum & ~1;
			f->next = flist;
			if (s->surfaceFlags & SURF_HINT) {
				//f->priority = HINT_PRIORITY;
				f->hint = qtrue;
			}
			flist = f;
		}
	}

	return flist;
}

/*
=================
MakeVisibleBspFaceList
=================
*/
bspface_t	*MakeVisibleBspFaceList( bspbrush_t *list ) {
	bspbrush_t	*b;
	int			i;
	side_t		*s;
	winding_t	*w;
	bspface_t	*f, *flist;

	flist = NULL;
	for ( b = list ; b ; b = b->next ) {
		if ( b->detail ) {
			continue;
		}
		for ( i = 0 ; i < b->numsides ; i++ ) {
			s = &b->sides[i];
			w = s->visibleHull;
			if ( !w ) {
				continue;
			}
			f = AllocBspFace();
			f->w = CopyWinding( w );
			f->planenum = s->planenum & ~1;
			f->next = flist;
			if (s->surfaceFlags & SURF_HINT) {
				//f->priority = HINT_PRIORITY;
				f->hint = qtrue;
			}
			flist = f;
		}
	}

	return flist;
}

