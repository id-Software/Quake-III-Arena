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

/*

tag all brushes with original contents
brushes may contain multiple contents
there will be no brush overlap after csg phase

*/

int minplanenums[3];
int maxplanenums[3];

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void CheckBSPBrush(bspbrush_t *brush)
{
	int i, j;
	plane_t *plane1, *plane2;

	//check if the brush is convex... flipped planes make a brush non-convex
	for (i = 0; i < brush->numsides; i++)
	{
		for (j = 0; j < brush->numsides; j++)
		{
			if (i == j) continue;
			plane1 = &mapplanes[brush->sides[i].planenum];
			plane2 = &mapplanes[brush->sides[j].planenum];
			//
			if (WindingsNonConvex(brush->sides[i].winding,
									brush->sides[j].winding,
									plane1->normal, plane2->normal,
									plane1->dist, plane2->dist))
			{
				Log_Print("non convex brush");
				break;
			} //end if
		} //end for
	} //end for
	BoundBrush(brush);
	//check for out of bound brushes
	for (i = 0; i < 3; i++)
	{
		if (brush->mins[i] < -MAX_MAP_BOUNDS || brush->maxs[i] > MAX_MAP_BOUNDS)
		{
			Log_Print("brush: bounds out of range\n");
			Log_Print("ob->mins[%d] = %f, ob->maxs[%d] = %f\n", i, brush->mins[i], i, brush->maxs[i]);
			break;
		} //end if
		if (brush->mins[i] > MAX_MAP_BOUNDS || brush->maxs[i] < -MAX_MAP_BOUNDS)
		{
			Log_Print("brush: no visible sides on brush\n");
			Log_Print("ob->mins[%d] = %f, ob->maxs[%d] = %f\n", i, brush->mins[i], i, brush->maxs[i]);
			break;
		} //end if
	} //end for
} //end of the function CheckBSPBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void BSPBrushWindings(bspbrush_t *brush)
{
	int i, j;
	winding_t *w;
	plane_t *plane;

	for (i = 0; i < brush->numsides; i++)
	{
		plane = &mapplanes[brush->sides[i].planenum];
		w = BaseWindingForPlane(plane->normal, plane->dist);
		for (j = 0; j < brush->numsides && w; j++)
		{
			if (i == j) continue;
			plane = &mapplanes[brush->sides[j].planenum^1];
			ChopWindingInPlace(&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		} //end for
		brush->sides[i].winding = w;
	} //end for
} //end of the function BSPBrushWindings
//===========================================================================
// NOTE: can't keep brush->original intact
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *TryMergeBrushes(bspbrush_t *brush1, bspbrush_t *brush2)
{
	int i, j, k, n, shared;
	side_t *side1, *side2, *cs;
	plane_t *plane1, *plane2;
	bspbrush_t *newbrush;

	//check for bounding box overlapp
	for (i = 0; i < 3; i++)
	{
		if (brush1->mins[i] > brush2->maxs[i] + 2
				|| brush1->maxs[i] < brush2->mins[i] - 2)
		{
			return NULL;
		} //end if
	} //end for
	//
	shared = 0;
	//check if the brush is convex... flipped planes make a brush non-convex
	for (i = 0; i < brush1->numsides; i++)
	{
		side1 = &brush1->sides[i];
		//don't check the "shared" sides
		for (k = 0; k < brush2->numsides; k++)
		{
			side2 = &brush2->sides[k];
			if (side1->planenum == (side2->planenum^1))
			{
				shared++;
				//there may only be ONE shared side
				if (shared > 1) return NULL;
				break;
			} //end if
		} //end for
		if (k < brush2->numsides) continue;
		//
		for (j = 0; j < brush2->numsides; j++)
		{
			side2 = &brush2->sides[j];
			//don't check the "shared" sides
			for (n = 0; n < brush1->numsides; n++)
			{
				side1 = &brush1->sides[n];
				if (side1->planenum == (side2->planenum^1)) break;
			} //end for
			if (n < brush1->numsides) continue;
			//
			side1 = &brush1->sides[i];
			//if the side is in the same plane
			//*
			if (side1->planenum == side2->planenum)
			{
				if (side1->texinfo != TEXINFO_NODE &&
					side2->texinfo != TEXINFO_NODE &&
					side1->texinfo != side2->texinfo) return NULL;
				continue;
			} //end if
			//
			plane1 = &mapplanes[side1->planenum];
			plane2 = &mapplanes[side2->planenum];
			//
			if (WindingsNonConvex(side1->winding, side2->winding,
									plane1->normal, plane2->normal,
									plane1->dist, plane2->dist))
			{
				return NULL;
			} //end if
		} //end for
	} //end for
	newbrush = AllocBrush(brush1->numsides + brush2->numsides);
	newbrush->original = brush1->original;
	newbrush->numsides = 0;
	//newbrush->side = brush1->side;	//brush contents
	//fix texinfos for sides lying in the same plane
	for (i = 0; i < brush1->numsides; i++)
	{
		side1 = &brush1->sides[i];
		//
		for (n = 0; n < brush2->numsides; n++)
		{
			side2 = &brush2->sides[n];
			//if both sides are in the same plane get the texinfo right
			if (side1->planenum == side2->planenum)
			{
				if (side1->texinfo == TEXINFO_NODE) side1->texinfo = side2->texinfo;
				if (side2->texinfo == TEXINFO_NODE) side2->texinfo = side1->texinfo;
			} //end if
		} //end for
	} //end for
	//
	for (i = 0; i < brush1->numsides; i++)
	{
		side1 = &brush1->sides[i];
		//don't add the "shared" sides
		for (n = 0; n < brush2->numsides; n++)
		{
			side2 = &brush2->sides[n];
			if (side1->planenum == (side2->planenum ^ 1)) break;
		} //end for
		if (n < brush2->numsides) continue;
		//
		for (n = 0; n < newbrush->numsides; n++)
		{
			cs = &newbrush->sides[n];
			if (cs->planenum == side1->planenum)
			{
				Log_Print("brush duplicate plane\n");
				break;
			} //end if
		} //end if
		if (n < newbrush->numsides) continue;
		//add this side
		cs = &newbrush->sides[newbrush->numsides];
		newbrush->numsides++;
		*cs = *side1;
	} //end for
	for (j = 0; j < brush2->numsides; j++)
	{
		side2 = &brush2->sides[j];
		for (n = 0; n < brush1->numsides; n++)
		{
			side1 = &brush1->sides[n];
			//if the side is in the same plane
			if (side2->planenum == side1->planenum) break;
			//don't add the "shared" sides
			if (side2->planenum == (side1->planenum ^ 1)) break;
		} //end for
		if (n < brush1->numsides) continue;
		//
		for (n = 0; n < newbrush->numsides; n++)
		{
			cs = &newbrush->sides[n];
			if (cs->planenum == side2->planenum)
			{
				Log_Print("brush duplicate plane\n");
				break;
			} //end if
		} //end if
		if (n < newbrush->numsides) continue;
		//add this side
		cs = &newbrush->sides[newbrush->numsides];
		newbrush->numsides++;
		*cs = *side2;
	} //end for
	BSPBrushWindings(newbrush);
	BoundBrush(newbrush);
	CheckBSPBrush(newbrush);
	return newbrush;
} //end of the function TryMergeBrushes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *MergeBrushes(bspbrush_t *brushlist)
{
	int nummerges, merged;
	bspbrush_t *b1, *b2, *tail, *newbrush, *newbrushlist;
	bspbrush_t *lastb2;

	if (!brushlist) return NULL;

	qprintf("%5d brushes merged", nummerges = 0);
	do
	{
		for (tail = brushlist; tail; tail = tail->next)
		{
			if (!tail->next) break;
		} //end for
		merged = 0;
		newbrushlist = NULL;
		for (b1 = brushlist; b1; b1 = brushlist)
		{
			lastb2 = b1;
			for (b2 = b1->next; b2; b2 = b2->next)
			{
				//if the brushes don't have the same contents
				if (b1->original->contents != b2->original->contents ||
					b1->original->expansionbbox != b2->original->expansionbbox) newbrush = NULL;
				else newbrush = TryMergeBrushes(b1, b2);
				if (newbrush)
				{
					tail->next = newbrush;
					lastb2->next = b2->next;
					brushlist = brushlist->next;
					FreeBrush(b1);
					FreeBrush(b2);
					for (tail = brushlist; tail; tail = tail->next)
					{
						if (!tail->next) break;
					} //end for
					merged++;
					qprintf("\r%5d", nummerges++);
					break;
				} //end if
				lastb2 = b2;
			} //end for
			//if b1 can't be merged with any of the other brushes
			if (!b2)
			{
				brushlist = brushlist->next;
				//keep b1
				b1->next = newbrushlist;
				newbrushlist = b1;
			} //end else
		} //end for
		brushlist = newbrushlist;
	} while(merged);
	qprintf("\n");
	return newbrushlist;
} //end of the function MergeBrushes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void SplitBrush2 (bspbrush_t *brush, int planenum,
	bspbrush_t **front, bspbrush_t **back)
{
	SplitBrush (brush, planenum, front, back);
#if 0
	if (*front && (*front)->sides[(*front)->numsides-1].texinfo == -1)
		(*front)->sides[(*front)->numsides-1].texinfo = (*front)->sides[0].texinfo;	// not -1
	if (*back && (*back)->sides[(*back)->numsides-1].texinfo == -1)
		(*back)->sides[(*back)->numsides-1].texinfo = (*back)->sides[0].texinfo;	// not -1
#endif
} //end of the function SplitBrush2
//===========================================================================
// Returns a list of brushes that remain after B is subtracted from A.
// May by empty if A is contained inside B.
// The originals are undisturbed.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *SubtractBrush (bspbrush_t *a, bspbrush_t *b)
{	// a - b = out (list)
	int		i;
	bspbrush_t	*front, *back;
	bspbrush_t	*out, *in;

	in = a;
	out = NULL;
	for (i = 0; i < b->numsides && in; i++)
	{
		SplitBrush2(in, b->sides[i].planenum, &front, &back);
		if (in != a) FreeBrush(in);
		if (front)
		{	// add to list
			front->next = out;
			out = front;
		} //end if
		in = back;
	} //end for
	if (in)
	{
		FreeBrush (in);
	} //end if
	else
	{	// didn't really intersect
		FreeBrushList (out);
		return a;
	} //end else
	return out;
} //end of the function SubtractBrush
//===========================================================================
// Returns a single brush made up by the intersection of the
// two provided brushes, or NULL if they are disjoint.
//
// The originals are undisturbed.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *IntersectBrush (bspbrush_t *a, bspbrush_t *b)
{
	int		i;
	bspbrush_t	*front, *back;
	bspbrush_t	*in;

	in = a;
	for (i=0 ; i<b->numsides && in ; i++)
	{
		SplitBrush2(in, b->sides[i].planenum, &front, &back);
		if (in != a) FreeBrush(in);
		if (front) FreeBrush(front);
		in = back;
	} //end for

	if (in == a) return NULL;

	in->next = NULL;
	return in;
} //end of the function IntersectBrush
//===========================================================================
// Returns true if the two brushes definately do not intersect.
// There will be false negatives for some non-axial combinations.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BrushesDisjoint (bspbrush_t *a, bspbrush_t *b)
{
	int		i, j;

	// check bounding boxes
	for (i=0 ; i<3 ; i++)
		if (a->mins[i] >= b->maxs[i]
		|| a->maxs[i] <= b->mins[i])
			return true;	// bounding boxes don't overlap

	// check for opposing planes
	for (i=0 ; i<a->numsides ; i++)
	{
		for (j=0 ; j<b->numsides ; j++)
		{
			if (a->sides[i].planenum ==
			(b->sides[j].planenum^1) )
				return true;	// opposite planes, so not touching
		}
	}

	return false;	// might intersect
} //end of the function BrushesDisjoint
//===========================================================================
// Returns a content word for the intersection of two brushes.
// Some combinations will generate a combination (water + clip),
// but most will be the stronger of the two contents.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int IntersectionContents (int c1, int c2)
{
	int out;

	out = c1 | c2;

	if (out & CONTENTS_SOLID) out = CONTENTS_SOLID;

	return out;
} //end of the function IntersectionContents
//===========================================================================
// Any planes shared with the box edge will be set to no texinfo
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *ClipBrushToBox(bspbrush_t *brush, vec3_t clipmins, vec3_t clipmaxs)
{
	int		i, j;
	bspbrush_t	*front,	*back;
	int		p;

	for (j=0 ; j<2 ; j++)
	{
		if (brush->maxs[j] > clipmaxs[j])
		{
			SplitBrush (brush, maxplanenums[j], &front, &back);
			if (front)
				FreeBrush (front);
			brush = back;
			if (!brush)
				return NULL;
		}
		if (brush->mins[j] < clipmins[j])
		{
			SplitBrush (brush, minplanenums[j], &front, &back);
			if (back)
				FreeBrush (back);
			brush = front;
			if (!brush)
				return NULL;
		}
	}

	// remove any colinear faces

	for (i=0 ; i<brush->numsides ; i++)
	{
		p = brush->sides[i].planenum & ~1;
		if (p == maxplanenums[0] || p == maxplanenums[1] 
			|| p == minplanenums[0] || p == minplanenums[1])
		{
			brush->sides[i].texinfo = TEXINFO_NODE;
			brush->sides[i].flags &= ~SFL_VISIBLE;
		}
	}
	return brush;
} //end of the function ClipBrushToBox
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *MakeBspBrushList(int startbrush, int endbrush,
											vec3_t clipmins, vec3_t clipmaxs)
{
	mapbrush_t	*mb;
	bspbrush_t	*brushlist, *newbrush;
	int			i, j;
	int			c_faces;
	int			c_brushes;
	int			numsides;
	int			vis;
	vec3_t		normal;
	float		dist;

	for (i=0 ; i<2 ; i++)
	{
		VectorClear (normal);
		normal[i] = 1;
		dist = clipmaxs[i];
		maxplanenums[i] = FindFloatPlane(normal, dist);
		dist = clipmins[i];
		minplanenums[i] = FindFloatPlane(normal, dist);
	}

	brushlist = NULL;
	c_faces = 0;
	c_brushes = 0;

	for (i=startbrush ; i<endbrush ; i++)
	{
		mb = &mapbrushes[i];

		numsides = mb->numsides;
		if (!numsides)
			continue;

		// make sure the brush has at least one face showing
		vis = 0;
		for (j=0 ; j<numsides ; j++)
			if ((mb->original_sides[j].flags & SFL_VISIBLE) && mb->original_sides[j].winding)
				vis++;
#if 0
		if (!vis)
			continue;	// no faces at all
#endif
		// if the brush is outside the clip area, skip it
		for (j=0 ; j<3 ; j++)
			if (mb->mins[j] >= clipmaxs[j]
			|| mb->maxs[j] <= clipmins[j])
			break;
		if (j != 3)
			continue;

		//
		// make a copy of the brush
		//
		newbrush = AllocBrush (mb->numsides);
		newbrush->original = mb;
		newbrush->numsides = mb->numsides;
		memcpy (newbrush->sides, mb->original_sides, numsides*sizeof(side_t));
		for (j=0 ; j<numsides ; j++)
		{
			if (newbrush->sides[j].winding)
				newbrush->sides[j].winding = CopyWinding (newbrush->sides[j].winding);
			if (newbrush->sides[j].surf & SURF_HINT)
				newbrush->sides[j].flags |= SFL_VISIBLE;	// hints are always visible
		}
		VectorCopy (mb->mins, newbrush->mins);
		VectorCopy (mb->maxs, newbrush->maxs);

		//
		// carve off anything outside the clip box
		//
		newbrush = ClipBrushToBox (newbrush, clipmins, clipmaxs);
		if (!newbrush)
			continue;

		c_faces += vis;
		c_brushes++;

		newbrush->next = brushlist;
		brushlist = newbrush;
	}

	return brushlist;
} //end of the function MakeBspBrushList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *AddBrushListToTail (bspbrush_t *list, bspbrush_t *tail)
{
	bspbrush_t	*walk, *next;

	for (walk=list ; walk ; walk=next)
	{	// add to end of list
		next = walk->next;
		walk->next = NULL;
		tail->next = walk;
		tail = walk;
	} //end for
	return tail;
} //end of the function AddBrushListToTail
//===========================================================================
// Builds a new list that doesn't hold the given brush
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *CullList(bspbrush_t *list, bspbrush_t *skip1)
{
	bspbrush_t	*newlist;
	bspbrush_t	*next;

	newlist = NULL;

	for ( ; list ; list = next)
	{
		next = list->next;
		if (list == skip1)
		{
			FreeBrush (list);
			continue;
		}
		list->next = newlist;
		newlist = list;
	}
	return newlist;
} //end of the function CullList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
/*
void WriteBrushMap(char *name, bspbrush_t *list)
{
	FILE	*f;
	side_t	*s;
	int		i;
	winding_t	*w;

	Log_Print("writing %s\n", name);
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

			fprintf (f, "%s 0 0 0 1 1\n", texinfo[s->texinfo].texture);
			FreeWinding (w);
		}
		fprintf (f, "}\n");
	}
	fprintf (f, "}\n");

	fclose (f);
} //end of the function WriteBrushMap
*/
//===========================================================================
// Returns true if b1 is allowed to bite b2
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean BrushGE (bspbrush_t *b1, bspbrush_t *b2)
{
#ifdef ME
	if (create_aas)
	{
		if (b1->original->expansionbbox != b2->original->expansionbbox)
		{
			return false;
		} //end if
		//never have something else bite a ladder brush
		//never have a ladder brush bite something else
		if ( (b1->original->contents & CONTENTS_LADDER)
			&& !(b2->original->contents & CONTENTS_LADDER))
		{ 
			return false;
		} //end if
	} //end if
#endif //ME
	// detail brushes never bite structural brushes
	if ( (b1->original->contents & CONTENTS_DETAIL) 
		&& !(b2->original->contents & CONTENTS_DETAIL) )
	{
		return false;
	} //end if
	if (b1->original->contents & CONTENTS_SOLID)
	{
		return true;
	} //end if
	return false;
} //end of the function BrushGE
//===========================================================================
// Carves any intersecting solid brushes into the minimum number
// of non-intersecting brushes.
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *ChopBrushes (bspbrush_t *head)
{
	bspbrush_t	*b1, *b2, *next;
	bspbrush_t	*tail;
	bspbrush_t	*keep;
	bspbrush_t	*sub, *sub2;
	int			c1, c2;
	int num_csg_iterations;

	Log_Print("-------- Brush CSG ---------\n");
	Log_Print("%6d original brushes\n", CountBrushList (head));

	num_csg_iterations = 0;
	qprintf("%6d output brushes", num_csg_iterations);

#if 0
	if (startbrush == 0)
		WriteBrushList ("before.gl", head, false);
#endif
	keep = NULL;

newlist:
	// find tail
	if (!head) return NULL;

	for (tail = head; tail->next; tail = tail->next)
		;

	for (b1=head ; b1 ; b1=next)
	{
		next = b1->next;

		//if the conversion is cancelled
		if (cancelconversion)
		{
			b1->next = keep;
			keep = b1;
			continue;
		} //end if
		
		for (b2 = b1->next; b2; b2 = b2->next)
		{
			if (BrushesDisjoint (b1, b2))
				continue;

			sub = NULL;
			sub2 = NULL;
			c1 = 999999;
			c2 = 999999;

			if (BrushGE (b2, b1))
			{
				sub = SubtractBrush (b1, b2);
				if (sub == b1)
				{
					continue;		// didn't really intersect
				} //end if
				if (!sub)
				{	// b1 is swallowed by b2
					head = CullList (b1, b1);
					goto newlist;
				}
				c1 = CountBrushList (sub);
			}

			if ( BrushGE (b1, b2) )
			{
				sub2 = SubtractBrush (b2, b1);
				if (sub2 == b2)
					continue;		// didn't really intersect
				if (!sub2)
				{	// b2 is swallowed by b1
					FreeBrushList (sub);
					head = CullList (b1, b2);
					goto newlist;
				}
				c2 = CountBrushList (sub2);
			}

			if (!sub && !sub2)
				continue;		// neither one can bite

			// only accept if it didn't fragment
			// (commenting this out allows full fragmentation)
			if (c1 > 1 && c2 > 1)
			{
				if (sub2)
					FreeBrushList (sub2);
				if (sub)
					FreeBrushList (sub);
				continue;
			}

			if (c1 < c2)
			{
				if (sub2) FreeBrushList (sub2);
				tail = AddBrushListToTail (sub, tail);
				head = CullList (b1, b1);
				goto newlist;
			} //end if
			else
			{
				if (sub) FreeBrushList (sub);
				tail = AddBrushListToTail (sub2, tail);
				head = CullList (b1, b2);
				goto newlist;
			} //end else
		} //end for

		if (!b2)
		{	// b1 is no longer intersecting anything, so keep it
			b1->next = keep;
			keep = b1;
		} //end if
		num_csg_iterations++;
		qprintf("\r%6d", num_csg_iterations);
	} //end for

	if (cancelconversion) return keep;
	//
	qprintf("\n");
	Log_Write("%6d output brushes\r\n", num_csg_iterations);

#if 0
	{
		WriteBrushList ("after.gl", keep, false);
		WriteBrushMap ("after.map", keep);
	}
#endif

	return keep;
} //end of the function ChopBrushes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *InitialBrushList (bspbrush_t *list)
{
	bspbrush_t *b;
	bspbrush_t	*out, *newb;
	int			i;

	// only return brushes that have visible faces
	out = NULL;
	for (b=list ; b ; b=b->next)
	{
#if 0
		for (i=0 ; i<b->numsides ; i++)
			if (b->sides[i].flags & SFL_VISIBLE)
				break;
		if (i == b->numsides)
			continue;
#endif
		newb = CopyBrush (b);
		newb->next = out;
		out = newb;

		// clear visible, so it must be set by MarkVisibleFaces_r
		// to be used in the optimized list
		for (i=0 ; i<b->numsides ; i++)
		{
			newb->sides[i].original = &b->sides[i];
//			newb->sides[i].visible = true;
			b->sides[i].flags &= ~SFL_VISIBLE;
		}
	}

	return out;
} //end of the function InitialBrushList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
bspbrush_t *OptimizedBrushList (bspbrush_t *list)
{
	bspbrush_t *b;
	bspbrush_t	*out, *newb;
	int			i;

	// only return brushes that have visible faces
	out = NULL;
	for (b=list ; b ; b=b->next)
	{
		for (i=0 ; i<b->numsides ; i++)
			if (b->sides[i].flags & SFL_VISIBLE)
				break;
		if (i == b->numsides)
			continue;
		newb = CopyBrush (b);
		newb->next = out;
		out = newb;
	} //end for

//	WriteBrushList ("vis.gl", out, true);
	return out;
} //end of the function OptimizeBrushList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tree_t *ProcessWorldBrushes(int brush_start, int brush_end)
{
	bspbrush_t *brushes;
	tree_t *tree;
	node_t *node;
	vec3_t mins, maxs;

	//take the whole world
	mins[0] = map_mins[0] - 8;
	mins[1] = map_mins[1] - 8;
	mins[2] = map_mins[2] - 8;

	maxs[0] = map_maxs[0] + 8;
	maxs[1] = map_maxs[1] + 8;
	maxs[2] = map_maxs[2] + 8;

	//reset the brush bsp
	ResetBrushBSP();

	// the makelist and chopbrushes could be cached between the passes...

	//create a list with brushes that are within the given mins/maxs
	//some brushes will be cut and only the part that falls within the
	//mins/maxs will be in the bush list
	brushes = MakeBspBrushList(brush_start, brush_end, mins, maxs);
	//

	if (!brushes)
	{
		node = AllocNode ();
		node->planenum = PLANENUM_LEAF;
		node->contents = CONTENTS_SOLID;

		tree = Tree_Alloc();
		tree->headnode = node;
		VectorCopy(mins, tree->mins);
		VectorCopy(maxs, tree->maxs);
	} //end if
	else
	{
		//Carves any intersecting solid brushes into the minimum number
		//of non-intersecting brushes. 
		if (!nocsg)
		{
			brushes = ChopBrushes(brushes);
			/*
			if (create_aas)
			{
				brushes = MergeBrushes(brushes);
			} //end if*/
		} //end if
		//if the conversion is cancelled
		if (cancelconversion)
		{
			FreeBrushList(brushes);
			return NULL;
		} //end if
		//create the actual bsp tree
		tree = BrushBSP(brushes, mins, maxs);
	} //end else
	//return the tree
	return tree;
} //end of the function ProcessWorldBrushes
