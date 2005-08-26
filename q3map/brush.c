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


int		c_active_brushes;

int		c_nodes;

// if a brush just barely pokes onto the other side,
// let it slide by without chopping
#define	PLANESIDE_EPSILON	0.001
//0.1




/*
================
CountBrushList
================
*/
int	CountBrushList (bspbrush_t *brushes)
{
	int	c;

	c = 0;
	for ( ; brushes ; brushes = brushes->next)
		c++;
	return c;
}


/*
================
AllocBrush
================
*/
bspbrush_t *AllocBrush (int numsides)
{
	bspbrush_t	*bb;
	int			c;

	c = (int)&(((bspbrush_t *)0)->sides[numsides]);
	bb = malloc(c);
	memset (bb, 0, c);
	if (numthreads == 1)
		c_active_brushes++;
	return bb;
}

/*
================
FreeBrush
================
*/
void FreeBrush (bspbrush_t *brushes)
{
	int			i;

	for (i=0 ; i<brushes->numsides ; i++)
		if (brushes->sides[i].winding)
			FreeWinding(brushes->sides[i].winding);
	free (brushes);
	if (numthreads == 1)
		c_active_brushes--;
}


/*
================
FreeBrushList
================
*/
void FreeBrushList (bspbrush_t *brushes)
{
	bspbrush_t	*next;

	for ( ; brushes ; brushes = next)
	{
		next = brushes->next;

		FreeBrush (brushes);
	}		
}

/*
==================
CopyBrush

Duplicates the brush, the sides, and the windings
==================
*/
bspbrush_t *CopyBrush (bspbrush_t *brush)
{
	bspbrush_t *newbrush;
	int			size;
	int			i;
	
	size = (int)&(((bspbrush_t *)0)->sides[brush->numsides]);

	newbrush = AllocBrush (brush->numsides);
	memcpy (newbrush, brush, size);

	for (i=0 ; i<brush->numsides ; i++)
	{
		if (brush->sides[i].winding)
			newbrush->sides[i].winding = CopyWinding (brush->sides[i].winding);
	}

	return newbrush;
}


/*
================
DrawBrushList
================
*/
void DrawBrushList (bspbrush_t *brush)
{
	int		i;
	side_t	*s;

	GLS_BeginScene ();
	for ( ; brush ; brush=brush->next)
	{
		for (i=0 ; i<brush->numsides ; i++)
		{
			s = &brush->sides[i];
			if (!s->winding)
				continue;
			GLS_Winding (s->winding, 0);
		}
	}
	GLS_EndScene ();
}



/*
================
WriteBrushList
================
*/
void WriteBrushList (char *name, bspbrush_t *brush, qboolean onlyvis)
{
	int		i;
	side_t	*s;
	FILE	*f;

	qprintf ("writing %s\n", name);
	f = SafeOpenWrite (name);

	for ( ; brush ; brush=brush->next)
	{
		for (i=0 ; i<brush->numsides ; i++)
		{
			s = &brush->sides[i];
			if (!s->winding)
				continue;
			if (onlyvis && !s->visible)
				continue;
			OutputWinding (brush->sides[i].winding, f);
		}
	}

	fclose (f);
}


/*
=============
PrintBrush
=============
*/
void PrintBrush (bspbrush_t *brush)
{
	int		i;

	_printf ("brush: %p\n", brush);
	for (i=0;i<brush->numsides ; i++)
	{
		pw(brush->sides[i].winding);
		_printf ("\n");
	}
}

/*
==================
BoundBrush

Sets the mins/maxs based on the windings
returns false if the brush doesn't enclose a valid volume
==================
*/
qboolean BoundBrush (bspbrush_t *brush)
{
	int			i, j;
	winding_t	*w;

	ClearBounds (brush->mins, brush->maxs);
	for (i=0 ; i<brush->numsides ; i++)
	{
		w = brush->sides[i].winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
			AddPointToBounds (w->p[j], brush->mins, brush->maxs);
	}

	for (i=0 ; i<3 ; i++) {
		if (brush->mins[i] < MIN_WORLD_COORD || brush->maxs[i] > MAX_WORLD_COORD
			|| brush->mins[i] >= brush->maxs[i] ) {
			return qfalse;
		}
	}

	return qtrue;
}

/*
==================
CreateBrushWindings

makes basewindigs for sides and mins / maxs for the brush
returns false if the brush doesn't enclose a valid volume
==================
*/
qboolean CreateBrushWindings (bspbrush_t *brush)
{
	int			i, j;
	winding_t	*w;
	side_t		*side;
	plane_t		*plane;

	for ( i = 0; i < brush->numsides; i++ )
	{
		side = &brush->sides[i];
		// don't create a winding for a bevel
		if ( side->bevel ) {
			continue;
		}
		plane = &mapplanes[side->planenum];
		w = BaseWindingForPlane (plane->normal, plane->dist);
		for ( j = 0; j < brush->numsides && w; j++ )
		{
			if (i == j)
				continue;
			if ( brush->sides[j].planenum == ( brush->sides[i].planenum ^ 1 ) )
				continue;		// back side clipaway
			if (brush->sides[j].bevel)
				continue;
			if (brush->sides[j].backSide)
				continue;
			plane = &mapplanes[brush->sides[j].planenum^1];
			ChopWindingInPlace (&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		}
		// free any existing winding
		if ( side->winding ) {
			FreeWinding( side->winding );
		}
		side->winding = w;
	}

	return BoundBrush (brush);
}

/*
==================
BrushFromBounds

Creates a new axial brush
==================
*/
bspbrush_t	*BrushFromBounds (vec3_t mins, vec3_t maxs)
{
	bspbrush_t	*b;
	int			i;
	vec3_t		normal;
	vec_t		dist;

	b = AllocBrush (6);
	b->numsides = 6;
	for (i=0 ; i<3 ; i++)
	{
		VectorClear (normal);
		normal[i] = 1;
		dist = maxs[i];
		b->sides[i].planenum = FindFloatPlane (normal, dist);

		normal[i] = -1;
		dist = -mins[i];
		b->sides[3+i].planenum = FindFloatPlane (normal, dist);
	}

	CreateBrushWindings (b);

	return b;
}

/*
==================
BrushVolume

==================
*/
vec_t BrushVolume (bspbrush_t *brush)
{
	int			i;
	winding_t	*w;
	vec3_t		corner;
	vec_t		d, area, volume;
	plane_t		*plane;

	if (!brush)
		return 0;

	// grab the first valid point as the corner

	w = NULL;
	for (i=0 ; i<brush->numsides ; i++)
	{
		w = brush->sides[i].winding;
		if (w)
			break;
	}
	if (!w)
		return 0;
	VectorCopy (w->p[0], corner);

	// make tetrahedrons to all other faces

	volume = 0;
	for ( ; i<brush->numsides ; i++)
	{
		w = brush->sides[i].winding;
		if (!w)
			continue;
		plane = &mapplanes[brush->sides[i].planenum];
		d = -(DotProduct (corner, plane->normal) - plane->dist);
		area = WindingArea (w);
		volume += d*area;
	}

	volume /= 3;
	return volume;
}


/*
==================
WriteBspBrushMap
==================
*/
void WriteBspBrushMap (char *name, bspbrush_t *list)
{
	FILE	*f;
	side_t	*s;
	int		i;
	winding_t	*w;

	_printf ("writing %s\n", name);
	f = fopen (name, "wb");
	if (!f)
		Error ("Can't write %s\b", name);

	fprintf (f, "{\n\"classname\" \"worldspawn\"\n");

	for ( ; list ; list=list->next )
	{
		fprintf (f, "{\n");
		for (i=0,s=list->sides ; i<list->numsides ; i++,s++)
		{
			w = BaseWindingForPlane (mapplanes[s->planenum].normal, mapplanes[s->planenum].dist);

			fprintf (f,"( %i %i %i ) ", (int)w->p[0][0], (int)w->p[0][1], (int)w->p[0][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[1][0], (int)w->p[1][1], (int)w->p[1][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]);

			fprintf (f, "notexture 0 0 0 1 1\n" );
			FreeWinding (w);
		}
		fprintf (f, "}\n");
	}
	fprintf (f, "}\n");

	fclose (f);

}


//=====================================================================================

/*
====================
FilterBrushIntoTree_r

====================
*/
int FilterBrushIntoTree_r( bspbrush_t *b, node_t *node ) {
	bspbrush_t		*front, *back;
	int				c;

	if ( !b ) {
		return 0;
	}

	// add it to the leaf list
	if ( node->planenum == PLANENUM_LEAF ) {
		b->next = node->brushlist;
		node->brushlist = b;

		// classify the leaf by the structural brush
		if ( !b->detail ) {
			if ( b->opaque ) {
				node->opaque = qtrue;
				node->areaportal = qfalse;
			} else if ( b->contents & CONTENTS_AREAPORTAL ) {
				if ( !node->opaque ) {
					node->areaportal = qtrue;
				}
			}
		}

		return 1;
	}

	// split it by the node plane
	SplitBrush ( b, node->planenum, &front, &back );
	FreeBrush( b );

	c = 0;
	c += FilterBrushIntoTree_r( front, node->children[0] );
	c += FilterBrushIntoTree_r( back, node->children[1] );

	return c;
}

/*
=====================
FilterDetailBrushesIntoTree

Fragment all the detail brushes into the structural leafs
=====================
*/
void FilterDetailBrushesIntoTree( entity_t *e, tree_t *tree ) {
	bspbrush_t			*b, *newb;
	int					r;
	int					c_unique, c_clusters;
	int					i;

	qprintf( "----- FilterDetailBrushesIntoTree -----\n");

	c_unique = 0;
	c_clusters = 0;
	for ( b = e->brushes ; b ; b = b->next ) {
		if ( !b->detail ) {
			continue;
		}
		c_unique++;
		newb = CopyBrush( b );
		r = FilterBrushIntoTree_r( newb, tree->headnode );
		c_clusters += r;

		// mark all sides as visible so drawsurfs are created
		if ( r ) {
			for ( i = 0 ; i < b->numsides ; i++ ) {
				if ( b->sides[i].winding ) {
					b->sides[i].visible = qtrue;
				}
			}
		}
	}

	qprintf( "%5i detail brushes\n", c_unique );
	qprintf( "%5i cluster references\n", c_clusters );
}

/*
=====================
FilterStructuralBrushesIntoTree

Mark the leafs as opaque and areaportals
=====================
*/
void FilterStructuralBrushesIntoTree( entity_t *e, tree_t *tree ) {
	bspbrush_t			*b, *newb;
	int					r;
	int					c_unique, c_clusters;
	int					i;

	qprintf( "----- FilterStructuralBrushesIntoTree -----\n");

	c_unique = 0;
	c_clusters = 0;
	for ( b = e->brushes ; b ; b = b->next ) {
		if ( b->detail ) {
			continue;
		}
		c_unique++;
		newb = CopyBrush( b );
		r = FilterBrushIntoTree_r( newb, tree->headnode );
		c_clusters += r;

		// mark all sides as visible so drawsurfs are created
		if ( r ) {
			for ( i = 0 ; i < b->numsides ; i++ ) {
				if ( b->sides[i].winding ) {
					b->sides[i].visible = qtrue;
				}
			}
		}
	}

	qprintf( "%5i structural brushes\n", c_unique );
	qprintf( "%5i cluster references\n", c_clusters );
}



/*
================
AllocTree
================
*/
tree_t *AllocTree (void)
{
	tree_t	*tree;

	tree = malloc(sizeof(*tree));
	memset (tree, 0, sizeof(*tree));
	ClearBounds (tree->mins, tree->maxs);

	return tree;
}

/*
================
AllocNode
================
*/
node_t *AllocNode (void)
{
	node_t	*node;

	node = malloc(sizeof(*node));
	memset (node, 0, sizeof(*node));

	return node;
}


/*
================
WindingIsTiny

Returns true if the winding would be crunched out of
existance by the vertex snapping.
================
*/
#define	EDGE_LENGTH	0.2
qboolean WindingIsTiny (winding_t *w)
{
/*
	if (WindingArea (w) < 1)
		return qtrue;
	return qfalse;
*/
	int		i, j;
	vec_t	len;
	vec3_t	delta;
	int		edges;

	edges = 0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		j = i == w->numpoints - 1 ? 0 : i+1;
		VectorSubtract (w->p[j], w->p[i], delta);
		len = VectorLength (delta);
		if (len > EDGE_LENGTH)
		{
			if (++edges == 3)
				return qfalse;
		}
	}
	return qtrue;
}

/*
================
WindingIsHuge

Returns true if the winding still has one of the points
from basewinding for plane
================
*/
qboolean WindingIsHuge (winding_t *w)
{
	int		i, j;

	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
			if (w->p[i][j] <= MIN_WORLD_COORD || w->p[i][j] >= MAX_WORLD_COORD)
				return qtrue;
	}
	return qfalse;
}

//============================================================

/*
==================
BrushMostlyOnSide

==================
*/
int BrushMostlyOnSide (bspbrush_t *brush, plane_t *plane)
{
	int			i, j;
	winding_t	*w;
	vec_t		d, max;
	int			side;

	max = 0;
	side = PSIDE_FRONT;
	for (i=0 ; i<brush->numsides ; i++)
	{
		w = brush->sides[i].winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
		{
			d = DotProduct (w->p[j], plane->normal) - plane->dist;
			if (d > max)
			{
				max = d;
				side = PSIDE_FRONT;
			}
			if (-d > max)
			{
				max = -d;
				side = PSIDE_BACK;
			}
		}
	}
	return side;
}

/*
================
SplitBrush

Generates two new brushes, leaving the original
unchanged
================
*/
void SplitBrush (bspbrush_t *brush, int planenum,
	bspbrush_t **front, bspbrush_t **back)
{
	bspbrush_t	*b[2];
	int			i, j;
	winding_t	*w, *cw[2], *midwinding;
	plane_t		*plane, *plane2;
	side_t		*s, *cs;
	float		d, d_front, d_back;

	*front = *back = NULL;
	plane = &mapplanes[planenum];

	// check all points
	d_front = d_back = 0;
	for (i=0 ; i<brush->numsides ; i++)
	{
		w = brush->sides[i].winding;
		if (!w)
			continue;
		for (j=0 ; j<w->numpoints ; j++)
		{
			d = DotProduct (w->p[j], plane->normal) - plane->dist;
			if (d > 0 && d > d_front)
				d_front = d;
			if (d < 0 && d < d_back)
				d_back = d;
		}
	}
	if (d_front < 0.1) // PLANESIDE_EPSILON)
	{	// only on back
		*back = CopyBrush (brush);
		return;
	}
	if (d_back > -0.1) // PLANESIDE_EPSILON)
	{	// only on front
		*front = CopyBrush (brush);
		return;
	}

	// create a new winding from the split plane

	w = BaseWindingForPlane (plane->normal, plane->dist);
	for (i=0 ; i<brush->numsides && w ; i++)
	{
		if ( brush->sides[i].backSide ) {
			continue;	// fake back-sided polygons never split
		}
		plane2 = &mapplanes[brush->sides[i].planenum ^ 1];
		ChopWindingInPlace (&w, plane2->normal, plane2->dist, 0); // PLANESIDE_EPSILON);
	}

	if (!w || WindingIsTiny (w) )
	{	// the brush isn't really split
		int		side;

		side = BrushMostlyOnSide (brush, plane);
		if (side == PSIDE_FRONT)
			*front = CopyBrush (brush);
		if (side == PSIDE_BACK)
			*back = CopyBrush (brush);
		return;
	}

	if (WindingIsHuge (w))
	{
		qprintf ("WARNING: huge winding\n");
	}

	midwinding = w;

	// split it for real

	for (i=0 ; i<2 ; i++)
	{
		b[i] = AllocBrush (brush->numsides+1);
		memcpy( b[i], brush, sizeof( bspbrush_t ) - sizeof( brush->sides ) );
		b[i]->numsides = 0;
		b[i]->next = NULL;
		b[i]->original = brush->original;
	}

	// split all the current windings

	for (i=0 ; i<brush->numsides ; i++)
	{
		s = &brush->sides[i];
		w = s->winding;
		if (!w)
			continue;
		ClipWindingEpsilon (w, plane->normal, plane->dist,
			0 /*PLANESIDE_EPSILON*/, &cw[0], &cw[1]);
		for (j=0 ; j<2 ; j++)
		{
			if (!cw[j])
				continue;
/*
			if (WindingIsTiny (cw[j]))
			{
				FreeWinding (cw[j]);
				continue;
			}
*/
			cs = &b[j]->sides[b[j]->numsides];
			b[j]->numsides++;
			*cs = *s;
			cs->winding = cw[j];
		}
	}


	// see if we have valid polygons on both sides

	for (i=0 ; i<2 ; i++)
	{
		BoundBrush (b[i]);
		for (j=0 ; j<3 ; j++)
		{
			if (b[i]->mins[j] < MIN_WORLD_COORD || b[i]->maxs[j] > MAX_WORLD_COORD)
			{
				qprintf ("bogus brush after clip\n");
				break;
			}
		}

		if (b[i]->numsides < 3 || j < 3)
		{
			FreeBrush (b[i]);
			b[i] = NULL;
		}
	}

	if ( !(b[0] && b[1]) )
	{
		if (!b[0] && !b[1])
			qprintf ("split removed brush\n");
		else
			qprintf ("split not on both sides\n");
		if (b[0])
		{
			FreeBrush (b[0]);
			*front = CopyBrush (brush);
		}
		if (b[1])
		{
			FreeBrush (b[1]);
			*back = CopyBrush (brush);
		}
		return;
	}

	// add the midwinding to both sides
	for (i=0 ; i<2 ; i++)
	{
		cs = &b[i]->sides[b[i]->numsides];
		b[i]->numsides++;

		cs->planenum = planenum^i^1;
		cs->shaderInfo = NULL;
		if (i==0)
			cs->winding = CopyWinding (midwinding);
		else
			cs->winding = midwinding;
	}

{
	vec_t	v1;
	int		i;

	for (i=0 ; i<2 ; i++)
	{
		v1 = BrushVolume (b[i]);
		if (v1 < 1.0)
		{
			FreeBrush (b[i]);
			b[i] = NULL;
//			qprintf ("tiny volume after clip\n");
		}
	}
}

	*front = b[0];
	*back = b[1];
}
