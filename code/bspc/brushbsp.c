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

#include <assert.h>

/*
each side has a count of the other sides it splits

the best split will be the one that minimizes the total split counts
of all remaining sides

precalc side on plane table

evaluate split side
{
cost = 0
for all sides
	for all sides
		get 
		if side splits side and splitside is on same child
			cost++;
}
*/

int c_nodes;
int c_nonvis;
int c_active_brushes;
int c_solidleafnodes;
int c_totalsides;
int c_brushmemory;
int c_peak_brushmemory;
int c_nodememory;
int c_peak_totalbspmemory;

// if a brush just barely pokes onto the other side,
// let it slide by without chopping
#define	PLANESIDE_EPSILON	0.001
//0.1

//#ifdef DEBUG
typedef struct cname_s
{
	int value;
	char *name;
} cname_t;

cname_t contentnames[] =
{
	{CONTENTS_SOLID,"CONTENTS_SOLID"},
	{CONTENTS_WINDOW,"CONTENTS_WINDOW"},
	{CONTENTS_AUX,"CONTENTS_AUX"},
	{CONTENTS_LAVA,"CONTENTS_LAVA"},
	{CONTENTS_SLIME,"CONTENTS_SLIME"},
	{CONTENTS_WATER,"CONTENTS_WATER"},
	{CONTENTS_MIST,"CONTENTS_MIST"},
	{LAST_VISIBLE_CONTENTS,"LAST_VISIBLE_CONTENTS"},

	{CONTENTS_AREAPORTAL,"CONTENTS_AREAPORTAL"},
	{CONTENTS_PLAYERCLIP,"CONTENTS_PLAYERCLIP"},
	{CONTENTS_MONSTERCLIP,"CONTENTS_MONSTERCLIP"},
	{CONTENTS_CURRENT_0,"CONTENTS_CURRENT_0"},
	{CONTENTS_CURRENT_90,"CONTENTS_CURRENT_90"},
	{CONTENTS_CURRENT_180,"CONTENTS_CURRENT_180"},
	{CONTENTS_CURRENT_270,"CONTENTS_CURRENT_270"},
	{CONTENTS_CURRENT_UP,"CONTENTS_CURRENT_UP"},
	{CONTENTS_CURRENT_DOWN,"CONTENTS_CURRENT_DOWN"},
	{CONTENTS_ORIGIN,"CONTENTS_ORIGIN"},
	{CONTENTS_MONSTER,"CONTENTS_MONSTER"},
	{CONTENTS_DEADMONSTER,"CONTENTS_DEADMONSTER"},
	{CONTENTS_DETAIL,"CONTENTS_DETAIL"},
	{CONTENTS_Q2TRANSLUCENT,"CONTENTS_TRANSLUCENT"},
	{CONTENTS_LADDER,"CONTENTS_LADDER"},
	{0, 0}
};

void PrintContents(int contents)
{
	int i;

	for (i = 0; contentnames[i].value; i++)
	{
		if (contents & contentnames[i].value)
		{
			Log_Write("%s,", contentnames[i].name);
		} //end if
	} //end for
} //end of the function PrintContents

//#endif DEBUG

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void ResetBrushBSP(void)
{
	c_nodes = 0;
	c_nonvis = 0;
	c_active_brushes = 0;
	c_solidleafnodes = 0;
	c_totalsides = 0;
	c_brushmemory = 0;
	c_peak_brushmemory = 0;
	c_nodememory = 0;
	c_peak_totalbspmemory = 0;
} //end of the function ResetBrushBSP
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FindBrushInTree (node_t *node, int brushnum)
{
	bspbrush_t	*b;

	if (node->planenum == PLANENUM_LEAF)
	{
		for (b=node->brushlist ; b ; b=b->next)
			if (b->original->brushnum == brushnum)
				Log_Print ("here\n");
		return;
	}
	FindBrushInTree(node->children[0], brushnum);
	FindBrushInTree(node->children[1], brushnum);
} //end of the function FindBrushInTree
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void DrawBrushList (bspbrush_t *brush, node_t *node)
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
			if (s->texinfo == TEXINFO_NODE)
				GLS_Winding (s->winding, 1);
			else if (!(s->flags & SFL_VISIBLE))
				GLS_Winding (s->winding, 2);
			else
				GLS_Winding (s->winding, 0);
		}
	}
	GLS_EndScene ();
} //end of the function DrawBrushList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
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
			if (onlyvis && !(s->flags & SFL_VISIBLE))
				continue;
			OutputWinding (brush->sides[i].winding, f);
		}
	}

	fclose (f);
} //end of the function WriteBrushList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void PrintBrush (bspbrush_t *brush)
{
	int		i;

	printf ("brush: %p\n", brush);
	for (i=0;i<brush->numsides ; i++)
	{
		pw(brush->sides[i].winding);
		printf ("\n");
	} //end for
} //end of the function PrintBrush
//===========================================================================
// Sets the mins/maxs based on the windings
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BoundBrush (bspbrush_t *brush)
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
} //end of the function BoundBrush
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void CreateBrushWindings (bspbrush_t *brush)
{
	int			i, j;
	winding_t	*w;
	side_t		*side;
	plane_t		*plane;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &brush->sides[i];
		plane = &mapplanes[side->planenum];
		w = BaseWindingForPlane (plane->normal, plane->dist);
		for (j=0 ; j<brush->numsides && w; j++)
		{
			if (i == j)
				continue;
			if (brush->sides[j].flags & SFL_BEVEL)
				continue;
			plane = &mapplanes[brush->sides[j].planenum^1];
			ChopWindingInPlace (&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		}

		side->winding = w;
	}

	BoundBrush (brush);
} //end of the function CreateBrushWindings
//===========================================================================
// Creates a new axial brush
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bspbrush_t	*BrushFromBounds (vec3_t mins, vec3_t maxs)
{
	bspbrush_t *b;
	int i;
	vec3_t normal;
	vec_t dist;

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
} //end of the function BrushFromBounds
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int BrushOutOfBounds(bspbrush_t *brush, vec3_t mins, vec3_t maxs, float epsilon)
{
	int i, j, n;
	winding_t *w;
	side_t *side;

	for (i = 0; i < brush->numsides; i++)
	{
		side = &brush->sides[i];
		w = side->winding;
		for (j = 0; j < w->numpoints; j++)
		{
			for (n = 0; n < 3; n++)
			{
				if (w->p[j][n] < (mins[n] + epsilon) || w->p[j][n] > (maxs[n] - epsilon)) return true;
			} //end for
		} //end for
	} //end for
	return false;
} //end of the function BrushOutOfBounds
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
vec_t BrushVolume (bspbrush_t *brush)
{
	int i;
	winding_t *w;
	vec3_t corner;
	vec_t d, area, volume;
	plane_t *plane;

	if (!brush) return 0;

	// grab the first valid point as the corner
	w = NULL;
	for (i = 0; i < brush->numsides; i++)
	{
		w = brush->sides[i].winding;
		if (w) break;
	} //end for
	if (!w) return 0;
	VectorCopy (w->p[0], corner);

	// make tetrahedrons to all other faces
	volume = 0;
	for ( ; i < brush->numsides; i++)
	{
		w = brush->sides[i].winding;
		if (!w) continue;
		plane = &mapplanes[brush->sides[i].planenum];
		d = -(DotProduct (corner, plane->normal) - plane->dist);
		area = WindingArea(w);
		volume += d * area;
	} //end for

	volume /= 3;
	return volume;
} //end of the function BrushVolume
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int CountBrushList (bspbrush_t *brushes)
{
	int c;

	c = 0;
	for ( ; brushes; brushes = brushes->next) c++;
	return c;
} //end of the function CountBrushList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
node_t *AllocNode (void)
{
	node_t	*node;

	node = GetMemory(sizeof(*node));
	memset (node, 0, sizeof(*node));
	if (numthreads == 1)
	{
		c_nodememory += MemorySize(node);
	} //end if
	return node;
} //end of the function AllocNode
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
bspbrush_t *AllocBrush (int numsides)
{
	bspbrush_t	*bb;
	int			c;

	c = (int)&(((bspbrush_t *)0)->sides[numsides]);
	bb = GetMemory(c);
	memset (bb, 0, c);
	if (numthreads == 1)
	{
		c_active_brushes++;
		c_brushmemory += MemorySize(bb);
		if (c_brushmemory > c_peak_brushmemory)
				c_peak_brushmemory = c_brushmemory;
	} //end if
	return bb;
} //end of the function AllocBrush
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeBrush (bspbrush_t *brushes)
{
	int			i;

	for (i=0 ; i<brushes->numsides ; i++)
		if (brushes->sides[i].winding)
			FreeWinding(brushes->sides[i].winding);
	if (numthreads == 1)
	{
		c_active_brushes--;
		c_brushmemory -= MemorySize(brushes);
		if (c_brushmemory < 0) c_brushmemory = 0;
	} //end if
	FreeMemory(brushes);
} //end of the function FreeBrush
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeBrushList (bspbrush_t *brushes)
{
	bspbrush_t	*next;

	for ( ; brushes; brushes = next)
	{
		next = brushes->next;

		FreeBrush(brushes);
	} //end for
} //end of the function FreeBrushList
//===========================================================================
// Duplicates the brush, the sides, and the windings
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
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
} //end of the function CopyBrush
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
node_t *PointInLeaf (node_t *node, vec3_t point)
{
	vec_t		d;
	plane_t		*plane;

	while (node->planenum != PLANENUM_LEAF)
	{
		plane = &mapplanes[node->planenum];
		d = DotProduct (point, plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return node;
} //end of the function PointInLeaf
//===========================================================================
// Returns PSIDE_FRONT, PSIDE_BACK, or PSIDE_BOTH
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#if 0
int BoxOnPlaneSide (vec3_t mins, vec3_t maxs, plane_t *plane)
{
	int		side;
	int		i;
	vec3_t	corners[2];
	vec_t	dist1, dist2;

	// axial planes are easy
	if (plane->type < 3)
	{
		side = 0;
		if (maxs[plane->type] > plane->dist+PLANESIDE_EPSILON)
			side |= PSIDE_FRONT;
		if (mins[plane->type] < plane->dist-PLANESIDE_EPSILON)
			side |= PSIDE_BACK;
		return side;
	}

	// create the proper leading and trailing verts for the box

	for (i=0 ; i<3 ; i++)
	{
		if (plane->normal[i] < 0)
		{
			corners[0][i] = mins[i];
			corners[1][i] = maxs[i];
		}
		else
		{
			corners[1][i] = mins[i];
			corners[0][i] = maxs[i];
		}
	}

	dist1 = DotProduct (plane->normal, corners[0]) - plane->dist;
	dist2 = DotProduct (plane->normal, corners[1]) - plane->dist;
	side = 0;
	if (dist1 >= PLANESIDE_EPSILON)
		side = PSIDE_FRONT;
	if (dist2 < PLANESIDE_EPSILON)
		side |= PSIDE_BACK;

	return side;
}
#else
int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, plane_t *p)
{
	float	dist1, dist2;
	int sides;

	// axial planes are easy
	if (p->type < 3)
	{
		sides = 0;
		if (emaxs[p->type] > p->dist+PLANESIDE_EPSILON) sides |= PSIDE_FRONT;
		if (emins[p->type] < p->dist-PLANESIDE_EPSILON) sides |= PSIDE_BACK;
		return sides;
	} //end if
	
// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
//		assert( 0 );
		break;
	}

	sides = 0;
	if (dist1 - p->dist >= PLANESIDE_EPSILON) sides = PSIDE_FRONT;
	if (dist2 - p->dist < PLANESIDE_EPSILON) sides |= PSIDE_BACK;

//	assert(sides != 0);

	return sides;
}
#endif
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int QuickTestBrushToPlanenum (bspbrush_t *brush, int planenum, int *numsplits)
{
	int i, num;
	plane_t *plane;
	int s;

	*numsplits = 0;

	plane = &mapplanes[planenum];

#ifdef ME
	//fast axial cases
	if (plane->type < 3)
	{
		if (plane->dist + PLANESIDE_EPSILON < brush->mins[plane->type])
			return PSIDE_FRONT;
		if (plane->dist - PLANESIDE_EPSILON > brush->maxs[plane->type])
			return PSIDE_BACK;
	} //end if
#endif //ME*/

	// if the brush actually uses the planenum,
	// we can tell the side for sure
	for (i = 0; i < brush->numsides; i++)
	{
		num = brush->sides[i].planenum;
		if (num >= MAX_MAPFILE_PLANES)
			Error ("bad planenum");
		if (num == planenum)
			return PSIDE_BACK|PSIDE_FACING;
		if (num == (planenum ^ 1) )
			return PSIDE_FRONT|PSIDE_FACING;

	}

	// box on plane side
	s = BoxOnPlaneSide (brush->mins, brush->maxs, plane);

	// if both sides, count the visible faces split
	if (s == PSIDE_BOTH)
	{
		*numsplits += 3;
	}

	return s;
} //end of the function QuickTestBrushToPlanenum
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TestBrushToPlanenum (bspbrush_t *brush, int planenum,
						 int *numsplits, qboolean *hintsplit, int *epsilonbrush)
{
	int i, j, num;
	plane_t *plane;
	int s = 0;
	winding_t *w;
	vec_t d, d_front, d_back;
	int front, back;
	int type;
	float dist;

	*numsplits = 0;
	*hintsplit = false;

	plane = &mapplanes[planenum];

#ifdef ME
	//fast axial cases
	type = plane->type;
	if (type < 3)
	{
		dist = plane->dist;
		if (dist + PLANESIDE_EPSILON < brush->mins[type]) return PSIDE_FRONT;
		if (dist - PLANESIDE_EPSILON > brush->maxs[type]) return PSIDE_BACK;
		if (brush->mins[type] < dist - PLANESIDE_EPSILON &&
					brush->maxs[type] > dist + PLANESIDE_EPSILON) s = PSIDE_BOTH;
	} //end if

	if (s != PSIDE_BOTH)
#endif //ME
	{
		// if the brush actually uses the planenum,
		// we can tell the side for sure
		for (i = 0; i < brush->numsides; i++)
		{
			num = brush->sides[i].planenum;
			if (num >= MAX_MAPFILE_PLANES) Error ("bad planenum");
			if (num == planenum)
			{
				//we don't need to test this side plane again
				brush->sides[i].flags |= SFL_TESTED;
				return PSIDE_BACK|PSIDE_FACING;
			} //end if
			if (num == (planenum ^ 1) )
			{
				//we don't need to test this side plane again
				brush->sides[i].flags |= SFL_TESTED;
				return PSIDE_FRONT|PSIDE_FACING;
			} //end if
		} //end for

		// box on plane side
		s = BoxOnPlaneSide (brush->mins, brush->maxs, plane);

		if (s != PSIDE_BOTH) return s;
	} //end if

	// if both sides, count the visible faces split
	d_front = d_back = 0;

	for (i = 0; i < brush->numsides; i++)
	{
		if (brush->sides[i].texinfo == TEXINFO_NODE)
			continue;		// on node, don't worry about splits
		if (!(brush->sides[i].flags & SFL_VISIBLE))
			continue;		// we don't care about non-visible
		w = brush->sides[i].winding;
		if (!w) continue;
		front = back = 0;
		for (j = 0; j < w->numpoints; j++)
		{
			d = DotProduct(w->p[j], plane->normal) - plane->dist;
			if (d > d_front) d_front = d;
			if (d < d_back) d_back = d;
			if (d > 0.1) // PLANESIDE_EPSILON)
				front = 1;
			if (d < -0.1) // PLANESIDE_EPSILON)
				back = 1;
		} //end for
		if (front && back)
		{
			if ( !(brush->sides[i].surf & SURF_SKIP) )
			{
				(*numsplits)++;
				if (brush->sides[i].surf & SURF_HINT)
				{
					*hintsplit = true;
				} //end if
			} //end if
		} //end if
	} //end for

	if ( (d_front > 0.0 && d_front < 1.0)
		|| (d_back < 0.0 && d_back > -1.0) )
		(*epsilonbrush)++;

#if 0
	if (*numsplits == 0)
	{	//	didn't really need to be split
		if (front) s = PSIDE_FRONT;
		else if (back) s = PSIDE_BACK;
		else s = 0;
	}
#endif

	return s;
} //end of the function TestBrushToPlanenum
//===========================================================================
// Returns true if the winding would be crunched out of
// existance by the vertex snapping.
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#define	EDGE_LENGTH	0.2
qboolean WindingIsTiny (winding_t *w)
{
#if 0
	if (WindingArea (w) < 1)
		return true;
	return false;
#else
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
				return false;
		}
	}
	return true;
#endif
} //end of the function WindingIsTiny
//===========================================================================
// Returns true if the winding still has one of the points
// from basewinding for plane
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
qboolean WindingIsHuge (winding_t *w)
{
	int		i, j;

	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
			if (w->p[i][j] < -BOGUS_RANGE+1 || w->p[i][j] > BOGUS_RANGE-1)
				return true;
	}
	return false;
} //end of the function WindingIsHuge
//===========================================================================
// creates a leaf out of the given nodes with the given brushes
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void LeafNode(node_t *node, bspbrush_t *brushes)
{
	bspbrush_t *b;
	int i;

	node->side = NULL;
	node->planenum = PLANENUM_LEAF;
	node->contents = 0;

	for (b = brushes; b; b = b->next)
	{
		// if the brush is solid and all of its sides are on nodes,
		// it eats everything
		if (b->original->contents & CONTENTS_SOLID)
		{
			for (i=0 ; i<b->numsides ; i++)
				if (b->sides[i].texinfo != TEXINFO_NODE)
					break;
			if (i == b->numsides)
			{
				node->contents = CONTENTS_SOLID;
				break;
			} //end if
		} //end if
		node->contents |= b->original->contents;
	} //end for

	if (create_aas)
	{
		node->expansionbboxes = 0;
		node->contents = 0;
		for (b = brushes; b; b = b->next)
		{
			node->expansionbboxes |= b->original->expansionbbox;
			node->contents |= b->original->contents;
			if (b->original->modelnum)
				node->modelnum = b->original->modelnum;
		} //end for
		if (node->contents & CONTENTS_SOLID)
		{
			if (node->expansionbboxes != cfg.allpresencetypes)
			{
				node->contents &= ~CONTENTS_SOLID;
			} //end if
		} //end if
	} //end if

	node->brushlist = brushes;
} //end of the function LeafNode
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void CheckPlaneAgainstParents (int pnum, node_t *node)
{
	node_t	*p;

	for (p = node->parent; p; p = p->parent)
	{
		if (p->planenum == pnum) Error("Tried parent");
	} //end for
} //end of the function CheckPlaneAgainstParants
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
qboolean CheckPlaneAgainstVolume (int pnum, node_t *node)
{
	bspbrush_t	*front, *back;
	qboolean	good;

	SplitBrush (node->volume, pnum, &front, &back);

	good = (front && back);

	if (front) FreeBrush (front);
	if (back) FreeBrush (back);

	return good;
} //end of the function CheckPlaneAgaintsVolume
//===========================================================================
// Using a hueristic, choses one of the sides out of the brushlist
// to partition the brushes with.
// Returns NULL if there are no valid planes to split with..
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
side_t *SelectSplitSide (bspbrush_t *brushes, node_t *node)
{
	int			value, bestvalue;
	bspbrush_t	*brush, *test;
	side_t		*side, *bestside;
	int			i, pass, numpasses;
	int			pnum;
	int			s;
	int			front, back, both, facing, splits;
	int			bsplits;
	int			bestsplits;
	int			epsilonbrush;
	qboolean	hintsplit = false;

	bestside = NULL;
	bestvalue = -99999;
	bestsplits = 0;

	// the search order goes: visible-structural, visible-detail,
	// nonvisible-structural, nonvisible-detail.
	// If any valid plane is available in a pass, no further
	// passes will be tried.
	numpasses = 2;
	for (pass = 0; pass < numpasses; pass++)
	{
		for (brush = brushes; brush; brush = brush->next)
		{
			// only check detail the second pass
//			if ( (pass & 1) && !(brush->original->contents & CONTENTS_DETAIL) )
//				continue;
//			if ( !(pass & 1) && (brush->original->contents & CONTENTS_DETAIL) )
//				continue;
			for (i = 0; i < brush->numsides; i++)
			{
				side = brush->sides + i;
//				if (side->flags & SFL_BEVEL)
//					continue;	// never use a bevel as a spliter
				if (!side->winding)
					continue;	// nothing visible, so it can't split
				if (side->texinfo == TEXINFO_NODE)
					continue;	// allready a node splitter
				if (side->flags & SFL_TESTED)
					continue;	// we allready have metrics for this plane
//				if (side->surf & SURF_SKIP)
//					continue;	// skip surfaces are never chosen

//				if (!(side->flags & SFL_VISIBLE) && (pass < 2))
//					continue;	// only check visible faces on first pass

				if ((side->flags & SFL_CURVE) && (pass < 1))
					continue;	// only check curves the second pass

				pnum = side->planenum;
				pnum &= ~1;	// allways use positive facing plane

				CheckPlaneAgainstParents (pnum, node);

				if (!CheckPlaneAgainstVolume (pnum, node))
					continue;	// would produce a tiny volume

				front = 0;
				back = 0;
				both = 0;
				facing = 0;
				splits = 0;
				epsilonbrush = 0;

				 //inner loop: optimize
				for (test = brushes; test; test = test->next)
				{
					s = TestBrushToPlanenum (test, pnum, &bsplits, &hintsplit, &epsilonbrush);

					splits += bsplits;
//					if (bsplits && (s&PSIDE_FACING) )
//						Error ("PSIDE_FACING with splits");

					test->testside = s;
					//
					if (s & PSIDE_FACING) facing++;
					if (s & PSIDE_FRONT) front++;
					if (s & PSIDE_BACK) back++;
					if (s == PSIDE_BOTH) both++;
				} //end for

				// give a value estimate for using this plane
				value =  5*facing - 5*splits - abs(front-back);
//					value =  -5*splits;
//					value =  5*facing - 5*splits;
				if (mapplanes[pnum].type < 3)
					value+=5;		// axial is better

				value -= epsilonbrush * 1000;	// avoid!

				// never split a hint side except with another hint
				if (hintsplit && !(side->surf & SURF_HINT) )
					value = -9999999;

				// save off the side test so we don't need
				// to recalculate it when we actually seperate
				// the brushes
				if (value > bestvalue)
				{
					bestvalue = value;
					bestside = side;
					bestsplits = splits;
					for (test = brushes; test ; test = test->next)
						test->side = test->testside;
				} //end if
			} //end for
		} //end for (brush = brushes;

		// if we found a good plane, don't bother trying any
		// other passes
		if (bestside)
		{
			if (pass > 1)
			{
				if (numthreads == 1) c_nonvis++;
			}
			if (pass > 0) node->detail_seperator = true;	// not needed for vis
			break;
		} //end if
	} //end for (pass = 0;

	//
	// clear all the tested flags we set
	//
	for (brush = brushes ; brush ; brush=brush->next)
	{
		for (i = 0; i < brush->numsides; i++)
		{
			brush->sides[i].flags &= ~SFL_TESTED;
		} //end for
	} //end for

	return bestside;
} //end of the function SelectSplitSide
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
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
} //end of the function BrushMostlyOnSide
//===========================================================================
// Generates two new brushes, leaving the original
// unchanged
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void SplitBrush (bspbrush_t *brush, int planenum,
	bspbrush_t **front, bspbrush_t **back)
{
	bspbrush_t	*b[2];
	int			i, j;
	winding_t	*w, *cw[2], *midwinding;
	plane_t		*plane, *plane2;
	side_t		*s, *cs;
	float d, d_front, d_back;

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

	if (d_front < 0.2) // PLANESIDE_EPSILON)
	{	// only on back
		*back = CopyBrush (brush);
		return;
	}
	if (d_back > -0.2) // PLANESIDE_EPSILON)
	{	// only on front
		*front = CopyBrush (brush);
		return;
	}

	// create a new winding from the split plane

	w = BaseWindingForPlane (plane->normal, plane->dist);
	for (i=0 ; i<brush->numsides && w ; i++)
	{
		plane2 = &mapplanes[brush->sides[i].planenum ^ 1];
		ChopWindingInPlace (&w, plane2->normal, plane2->dist, 0); // PLANESIDE_EPSILON);
	}

	if (!w || WindingIsTiny(w))
	{	// the brush isn't really split
		int		side;

		side = BrushMostlyOnSide (brush, plane);
		if (side == PSIDE_FRONT)
			*front = CopyBrush (brush);
		if (side == PSIDE_BACK)
			*back = CopyBrush (brush);
		//free a possible winding
		if (w) FreeWinding(w);
		return;
	}

	if (WindingIsHuge (w))
	{
		Log_Write("WARNING: huge winding\n");
	}

	midwinding = w;

	// split it for real

	for (i=0 ; i<2 ; i++)
	{
		b[i] = AllocBrush (brush->numsides+1);
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
#if 0
			if (WindingIsTiny (cw[j]))
			{
				FreeWinding (cw[j]);
				continue;
			}
#endif
			cs = &b[j]->sides[b[j]->numsides];
			b[j]->numsides++;
			*cs = *s;
//			cs->planenum = s->planenum;
//			cs->texinfo = s->texinfo;
//			cs->original = s->original;
			cs->winding = cw[j];
			cs->flags &= ~SFL_TESTED;
		}
	}


	// see if we have valid polygons on both sides

	for (i=0 ; i<2 ; i++)
	{
		BoundBrush (b[i]);
		for (j=0 ; j<3 ; j++)
		{
			if (b[i]->mins[j] < -MAX_MAP_BOUNDS || b[i]->maxs[j] > MAX_MAP_BOUNDS)
			{
				Log_Write("bogus brush after clip");
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
			Log_Write("split removed brush\r\n");
		else
			Log_Write("split not on both sides\r\n");
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
		cs->texinfo = TEXINFO_NODE; //never use these sides as splitters
		cs->flags &= ~SFL_VISIBLE;
		cs->flags &= ~SFL_TESTED;
		if (i==0)
			cs->winding = CopyWinding (midwinding);
		else
			cs->winding = midwinding;
	}

{
	vec_t	v1;
	int i;

	for (i = 0; i < 2; i++)
	{
		v1 = BrushVolume (b[i]);
		if (v1 < 1.0)
		{
			FreeBrush(b[i]);
			b[i] = NULL;
			//Log_Write("tiny volume after clip");
		}
	}
	if (!b[0] && !b[1])
	{
		Log_Write("two tiny brushes\r\n");
	} //end if
}

	*front = b[0];
	*back = b[1];
} //end of the function SplitBrush
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void SplitBrushList (bspbrush_t *brushes, 
	node_t *node, bspbrush_t **front, bspbrush_t **back)
{
	bspbrush_t	*brush, *newbrush, *newbrush2;
	side_t		*side;
	int			sides;
	int			i;

	*front = *back = NULL;

	for (brush = brushes; brush; brush = brush->next)
	{
		sides = brush->side;

		if (sides == PSIDE_BOTH)
		{	// split into two brushes
			SplitBrush (brush, node->planenum, &newbrush, &newbrush2);
			if (newbrush)
			{
				newbrush->next = *front;
				*front = newbrush;
			} //end if
			if (newbrush2)
			{
				newbrush2->next = *back;
				*back = newbrush2;
			} //end if
			continue;
		} //end if

		newbrush = CopyBrush (brush);

		// if the planenum is actualy a part of the brush
		// find the plane and flag it as used so it won't be tried
		// as a splitter again
		if (sides & PSIDE_FACING)
		{
			for (i=0 ; i<newbrush->numsides ; i++)
			{
				side = newbrush->sides + i;
				if ( (side->planenum& ~1) == node->planenum)
					side->texinfo = TEXINFO_NODE;
			} //end for
		} //end if
		if (sides & PSIDE_FRONT)
		{
			newbrush->next = *front;
			*front = newbrush;
			continue;
		} //end if
		if (sides & PSIDE_BACK)
		{
			newbrush->next = *back;
			*back = newbrush;
			continue;
		} //end if
	} //end for
} //end of the function SplitBrushList
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void CheckBrushLists(bspbrush_t *brushlist1, bspbrush_t *brushlist2)
{
	bspbrush_t *brush1, *brush2;

	for (brush1 = brushlist1; brush1; brush1 = brush1->next)
	{
		for (brush2 = brushlist2; brush2; brush2 = brush2->next)
		{
			assert(brush1 != brush2);
		} //end for
	} //end for
} //end of the function CheckBrushLists
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int numrecurse = 0;

node_t *BuildTree_r (node_t *node, bspbrush_t *brushes)
{
	node_t		*newnode;
	side_t		*bestside;
	int			i, totalmem;
	bspbrush_t	*children[2];

	qprintf("\r%6d", numrecurse);
	numrecurse++;

	if (numthreads == 1)
	{
		totalmem = WindingMemory() + c_nodememory + c_brushmemory;
		if (totalmem > c_peak_totalbspmemory)
			c_peak_totalbspmemory = totalmem;
		c_nodes++;
	} //endif

	if (drawflag)
		DrawBrushList(brushes, node);

	// find the best plane to use as a splitter
	bestside = SelectSplitSide (brushes, node);
	if (!bestside)
	{
		// leaf node
		node->side = NULL;
		node->planenum = -1;
		LeafNode(node, brushes);
		if (node->contents & CONTENTS_SOLID) c_solidleafnodes++;
		if (create_aas)
		{
			//free up memory!!!
			FreeBrushList(node->brushlist);
			node->brushlist = NULL;
			//free the node volume brush
			if (node->volume)
			{
				FreeBrush(node->volume);
				node->volume = NULL;
			} //end if
		} //end if
		return node;
	} //end if

	// this is a splitplane node
	node->side = bestside;
	node->planenum = bestside->planenum & ~1;	// always use front facing

	//split the brush list in two for both children
	SplitBrushList (brushes, node, &children[0], &children[1]);
	//free the old brush list
	FreeBrushList (brushes);

	// allocate children before recursing
	for (i = 0; i < 2; i++)
	{
		newnode = AllocNode ();
		newnode->parent = node;
		node->children[i] = newnode;
	} //end for

	//split the volume brush of the node for the children
	SplitBrush (node->volume, node->planenum, &node->children[0]->volume,
		&node->children[1]->volume);

	if (create_aas)
	{
		//free the volume brush
		if (node->volume)
		{
			FreeBrush(node->volume);
			node->volume = NULL;
		} //end if
	} //end if
	// recursively process children
	for (i = 0; i < 2; i++)
	{
		node->children[i] = BuildTree_r(node->children[i], children[i]);
	} //end for

	return node;
} //end of the function BuildTree_r
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
node_t *firstnode;		//first node in the list
node_t *lastnode;			//last node in the list
int nodelistsize;			//number of nodes in the list
int use_nodequeue = 0;	//use nodequeue, otherwise a node stack is used
int numwaiting = 0;

void (*AddNodeToList)(node_t *node);

//add the node to the front of the node list
//(effectively using a node stack)
void AddNodeToStack(node_t *node)
{
	ThreadLock();

	node->next = firstnode;
	firstnode = node;
	if (!lastnode) lastnode = node;
	nodelistsize++;

	ThreadUnlock();
	//
	ThreadSemaphoreIncrease(1);
} //end of the function AddNodeToStack
//add the node to the end of the node list
//(effectively using a node queue)
void AddNodeToQueue(node_t *node)
{
	ThreadLock();

	node->next = NULL;
	if (lastnode) lastnode->next = node;
	else firstnode = node;
	lastnode = node;
	nodelistsize++;

	ThreadUnlock();
	//
	ThreadSemaphoreIncrease(1);
} //end of the function AddNodeToQueue
//get the first node from the front of the node list
node_t *NextNodeFromList(void)
{
	node_t *node;

	ThreadLock();
	numwaiting++;
	if (!firstnode)
	{
		if (numwaiting >= GetNumThreads()) ThreadSemaphoreIncrease(GetNumThreads());
	} //end if
	ThreadUnlock();

	ThreadSemaphoreWait();

	ThreadLock();

	numwaiting--;

	node = firstnode;
	if (firstnode)
	{
		firstnode = firstnode->next;
		nodelistsize--;
	} //end if
	if (!firstnode) lastnode = NULL;

	ThreadUnlock();

	return node;
} //end of the function NextNodeFromList
//returns the size of the node list
int NodeListSize(void)
{
	int size;

	ThreadLock();
	size = nodelistsize;
	ThreadUnlock();

	return size;
} //end of the function NodeListSize
//
void IncreaseNodeCounter(void)
{
	ThreadLock();
	//if (verbose) printf("\r%6d", numrecurse++);
	qprintf("\r%6d", numrecurse++);
	//qprintf("\r%6d %d, %5d ", numrecurse++, GetNumThreads(), nodelistsize);
	ThreadUnlock();
} //end of the function IncreaseNodeCounter
//thread function, gets nodes from the nodelist and processes them
void BuildTreeThread(int threadid)
{
	node_t *newnode, *node;
	side_t *bestside;
	int i, totalmem;
	bspbrush_t *brushes;

	for (node = NextNodeFromList(); node; )
	{
		//if the nodelist isn't empty try to add another thread
		//if (NodeListSize() > 10) AddThread(BuildTreeThread);
		//display the number of nodes processed so far
		if (numthreads == 1)
			IncreaseNodeCounter();

		brushes = node->brushlist;

		if (numthreads == 1)
		{
			totalmem = WindingMemory() + c_nodememory + c_brushmemory;
			if (totalmem > c_peak_totalbspmemory)
			{
				c_peak_totalbspmemory = totalmem;
			} //end if
			c_nodes++;
		} //endif

		if (drawflag)
		{
			DrawBrushList(brushes, node);
		} //end if

		if (cancelconversion)
		{
			bestside = NULL;
		} //end if
		else
		{
			// find the best plane to use as a splitter
			bestside = SelectSplitSide(brushes, node);
		} //end else
		//if there's no split side left
		if (!bestside)
		{
			//create a leaf out of the node
			LeafNode(node, brushes);
			if (node->contents & CONTENTS_SOLID) c_solidleafnodes++;
			if (create_aas)
			{
				//free up memory!!!
				FreeBrushList(node->brushlist);
				node->brushlist = NULL;
			} //end if
			//free the node volume brush (it is not used anymore)
			if (node->volume)
			{
				FreeBrush(node->volume);
				node->volume = NULL;
			} //end if
			node = NextNodeFromList();
			continue;
		} //end if

		// this is a splitplane node
		node->side = bestside;
		node->planenum = bestside->planenum & ~1;	//always use front facing

		//allocate children
		for (i = 0; i < 2; i++)
		{
			newnode = AllocNode();
			newnode->parent = node;
			node->children[i] = newnode;
		} //end for

		//split the brush list in two for both children
		SplitBrushList(brushes, node, &node->children[0]->brushlist, &node->children[1]->brushlist);

		CheckBrushLists(node->children[0]->brushlist, node->children[1]->brushlist);
		//free the old brush list
		FreeBrushList(brushes);
		node->brushlist = NULL;

		//split the volume brush of the node for the children
		SplitBrush(node->volume, node->planenum, &node->children[0]->volume,
								&node->children[1]->volume);

		if (!node->children[0]->volume || !node->children[1]->volume)
		{
			Error("child without volume brush");
		} //end if

		//free the volume brush
		if (node->volume)
		{
			FreeBrush(node->volume);
			node->volume = NULL;
		} //end if
		//add both children to the node list
		//AddNodeToList(node->children[0]);
		AddNodeToList(node->children[1]);
		node = node->children[0];
	} //end while
	RemoveThread(threadid);
} //end of the function BuildTreeThread
//===========================================================================
// build the bsp tree using a node list
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void BuildTree(tree_t *tree)
{
	int i;

	firstnode = NULL;
	lastnode = NULL;
	//use a node queue or node stack
	if (use_nodequeue) AddNodeToList = AddNodeToQueue;
	else AddNodeToList = AddNodeToStack;
	//setup thread locking
	ThreadSetupLock();
	ThreadSetupSemaphore();
	numwaiting = 0;
	//
	Log_Print("%6d threads max\n", numthreads);
	if (use_nodequeue) Log_Print("breadth first bsp building\n");
	else Log_Print("depth first bsp building\n");
	qprintf("%6d splits", 0);
	//add the first node to the list
	AddNodeToList(tree->headnode);
	//start the threads
	for (i = 0; i < numthreads; i++)
		AddThread(BuildTreeThread);
	//wait for all added threads to be finished
	WaitForAllThreadsFinished();
	//shutdown the thread locking
	ThreadShutdownLock();
	ThreadShutdownSemaphore();
} //end of the function BuildTree
//===========================================================================
// The incoming brush list will be freed before exiting
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
tree_t *BrushBSP(bspbrush_t *brushlist, vec3_t mins, vec3_t maxs)
{
	int i, c_faces, c_nonvisfaces, c_brushes;
	bspbrush_t *b;
	node_t *node;
	tree_t *tree;
	vec_t volume;
//	vec3_t point;

	Log_Print("-------- Brush BSP ---------\n");

	tree = Tree_Alloc();

	c_faces = 0;
	c_nonvisfaces = 0;
	c_brushes = 0;
	c_totalsides = 0;
	for (b = brushlist; b; b = b->next)
	{
		c_brushes++;

		volume = BrushVolume(b);
		if (volume < microvolume)
		{
			Log_Print("WARNING: entity %i, brush %i: microbrush\n",
				b->original->entitynum, b->original->brushnum);
		} //end if

		for (i=0 ; i<b->numsides ; i++)
		{
			if (b->sides[i].flags & SFL_BEVEL)
				continue;
			if (!b->sides[i].winding)
				continue;
			if (b->sides[i].texinfo == TEXINFO_NODE)
				continue;
			if (b->sides[i].flags & SFL_VISIBLE)
			{
				c_faces++;
			} //end if
			else
			{
				c_nonvisfaces++;
				//if (create_aas) b->sides[i].texinfo = TEXINFO_NODE;
			} //end if
		} //end for
		c_totalsides += b->numsides;

		AddPointToBounds (b->mins, tree->mins, tree->maxs);
		AddPointToBounds (b->maxs, tree->mins, tree->maxs);
	} //end for

	Log_Print("%6i brushes\n", c_brushes);
	Log_Print("%6i visible faces\n", c_faces);
	Log_Print("%6i nonvisible faces\n", c_nonvisfaces);
	Log_Print("%6i total sides\n", c_totalsides);

	c_active_brushes = c_brushes;
	c_nodememory = 0;
	c_brushmemory = 0;
	c_peak_brushmemory = 0;

	c_nodes = 0;
	c_nonvis = 0;
	node = AllocNode ();

	//volume of first node (head node)
	node->volume = BrushFromBounds (mins, maxs);
	//
	tree->headnode = node;
	//just get some statistics and the mins/maxs of the node
	numrecurse = 0;
//	qprintf("%6d splits", numrecurse);

	tree->headnode->brushlist = brushlist;
	BuildTree(tree);

	//build the bsp tree with the start node from the brushlist
//	node = BuildTree_r(node, brushlist);

	//if the conversion is cancelled
	if (cancelconversion) return tree;

	qprintf("\n");
	Log_Write("%6d splits\r\n", numrecurse);
//	Log_Print("%6i visible nodes\n", c_nodes/2 - c_nonvis);
//	Log_Print("%6i nonvis nodes\n", c_nonvis);
//	Log_Print("%6i leaves\n", (c_nodes+1)/2);
//	Log_Print("%6i solid leaf nodes\n", c_solidleafnodes);
//	Log_Print("%6i active brushes\n", c_active_brushes);
	if (numthreads == 1)
	{
//		Log_Print("%6i KB of node memory\n", c_nodememory >> 10);
//		Log_Print("%6i KB of brush memory\n", c_brushmemory >> 10);
//		Log_Print("%6i KB of peak brush memory\n", c_peak_brushmemory >> 10);
//		Log_Print("%6i KB of winding memory\n", WindingMemory() >> 10);
//		Log_Print("%6i KB of peak winding memory\n", WindingPeakMemory() >> 10);
		Log_Print("%6i KB of peak total bsp memory\n", c_peak_totalbspmemory >> 10);
	} //end if

	/*
	point[0] = 1485;
	point[1] = 956.125;
	point[2] = 352.125;
	node = PointInLeaf(tree->headnode, point);
	if (node->planenum != PLANENUM_LEAF)
	{
		Log_Print("node not a leaf\n");
	} //end if
	Log_Print("at %f %f %f:\n", point[0], point[1], point[2]);
	PrintContents(node->contents);
	Log_Print("node->expansionbboxes = %d\n", node->expansionbboxes);
	//*/
	return tree;
} //end of the function BrushBSP

