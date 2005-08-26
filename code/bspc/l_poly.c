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

#include <malloc.h>
#include "l_cmd.h"
#include "l_math.h"
#include "l_poly.h"
#include "l_log.h"
#include "l_mem.h"

#define	BOGUS_RANGE		65535

extern int numthreads;

// counters are only bumped when running single threaded,
// because they are an awefull coherence problem
int c_active_windings;
int c_peak_windings;
int c_winding_allocs;
int c_winding_points;
int c_windingmemory;
int c_peak_windingmemory;

char windingerror[1024];

void pw(winding_t *w)
{
	int		i;
	for (i=0 ; i<w->numpoints ; i++)
		printf ("(%5.3f, %5.3f, %5.3f)\n",w->p[i][0], w->p[i][1],w->p[i][2]);
}


void ResetWindings(void)
{
	c_active_windings = 0;
	c_peak_windings = 0;
	c_winding_allocs = 0;
	c_winding_points = 0;
	c_windingmemory = 0;
	c_peak_windingmemory = 0;

	strcpy(windingerror, "");
} //end of the function ResetWindings
/*
=============
AllocWinding
=============
*/
winding_t *AllocWinding (int points)
{
	winding_t	*w;
	int			s;

	s = sizeof(vec_t)*3*points + sizeof(int);
	w = GetMemory(s);
	memset(w, 0, s);

	if (numthreads == 1)
	{
		c_winding_allocs++;
		c_winding_points += points;
		c_active_windings++;
		if (c_active_windings > c_peak_windings)
			c_peak_windings = c_active_windings;
		c_windingmemory += MemorySize(w);
		if (c_windingmemory > c_peak_windingmemory)
			c_peak_windingmemory = c_windingmemory;
	} //end if
	return w;
} //end of the function AllocWinding

void FreeWinding (winding_t *w)
{
	if (*(unsigned *)w == 0xdeaddead)
		Error ("FreeWinding: freed a freed winding");

	if (numthreads == 1)
	{
		c_active_windings--;
		c_windingmemory -= MemorySize(w);
	} //end if

	*(unsigned *)w = 0xdeaddead;

	FreeMemory(w);
} //end of the function FreeWinding

int WindingMemory(void)
{
	return c_windingmemory;
} //end of the function WindingMemory

int WindingPeakMemory(void)
{
	return c_peak_windingmemory;
} //end of the function WindingPeakMemory

int ActiveWindings(void)
{
	return c_active_windings;
} //end of the function ActiveWindings
/*
============
RemoveColinearPoints
============
*/
int	c_removed;

void RemoveColinearPoints (winding_t *w)
{
	int		i, j, k;
	vec3_t	v1, v2;
	int		nump;
	vec3_t	p[MAX_POINTS_ON_WINDING];

	nump = 0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		j = (i+1)%w->numpoints;
		k = (i+w->numpoints-1)%w->numpoints;
		VectorSubtract (w->p[j], w->p[i], v1);
		VectorSubtract (w->p[i], w->p[k], v2);
		VectorNormalize(v1);
		VectorNormalize(v2);
		if (DotProduct(v1, v2) < 0.999)
		{
			if (nump >= MAX_POINTS_ON_WINDING)
				Error("RemoveColinearPoints: MAX_POINTS_ON_WINDING");
			VectorCopy (w->p[i], p[nump]);
			nump++;
		}
	}

	if (nump == w->numpoints)
		return;

	if (numthreads == 1)
		c_removed += w->numpoints - nump;
	w->numpoints = nump;
	memcpy (w->p, p, nump*sizeof(p[0]));
}

/*
============
WindingPlane
============
*/
void WindingPlane (winding_t *w, vec3_t normal, vec_t *dist)
{
	vec3_t v1, v2;
	int i;

	//find two vectors each longer than 0.5 units
	for (i = 0; i < w->numpoints; i++)
	{
		VectorSubtract(w->p[(i+1) % w->numpoints], w->p[i], v1);
		VectorSubtract(w->p[(i+2) % w->numpoints], w->p[i], v2);
		if (VectorLength(v1) > 0.5 && VectorLength(v2) > 0.5) break;
	} //end for
	CrossProduct(v2, v1, normal);
	VectorNormalize(normal);
	*dist = DotProduct(w->p[0], normal);
} //end of the function WindingPlane

/*
=============
WindingArea
=============
*/
vec_t	WindingArea (winding_t *w)
{
	int		i;
	vec3_t	d1, d2, cross;
	vec_t	total;

	total = 0;
	for (i=2 ; i<w->numpoints ; i++)
	{
		VectorSubtract (w->p[i-1], w->p[0], d1);
		VectorSubtract (w->p[i], w->p[0], d2);
		CrossProduct (d1, d2, cross);
		total += 0.5 * VectorLength ( cross );
	}
	return total;
}

void WindingBounds (winding_t *w, vec3_t mins, vec3_t maxs)
{
	vec_t	v;
	int		i,j;

	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;

	for (i=0 ; i<w->numpoints ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			v = w->p[i][j];
			if (v < mins[j])
				mins[j] = v;
			if (v > maxs[j])
				maxs[j] = v;
		}
	}
}

/*
=============
WindingCenter
=============
*/
void	WindingCenter (winding_t *w, vec3_t center)
{
	int		i;
	float	scale;

	VectorCopy (vec3_origin, center);
	for (i=0 ; i<w->numpoints ; i++)
		VectorAdd (w->p[i], center, center);

	scale = 1.0/w->numpoints;
	VectorScale (center, scale, center);
}

/*
=================
BaseWindingForPlane
=================
*/
winding_t *BaseWindingForPlane (vec3_t normal, vec_t dist)
{
	int		i, x;
	vec_t	max, v;
	vec3_t	org, vright, vup;
	winding_t	*w;
	
// find the major axis

	max = -BOGUS_RANGE;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
		Error ("BaseWindingForPlane: no axis found");
		
	VectorCopy (vec3_origin, vup);	
	switch (x)
	{
	case 0:
	case 1:
		vup[2] = 1;
		break;		
	case 2:
		vup[0] = 1;
		break;		
	}

	v = DotProduct (vup, normal);
	VectorMA (vup, -v, normal, vup);
	VectorNormalize (vup);
		
	VectorScale (normal, dist, org);
	
	CrossProduct (vup, normal, vright);
	
	VectorScale (vup, BOGUS_RANGE, vup);
	VectorScale (vright, BOGUS_RANGE, vright);

// project a really big	axis aligned box onto the plane
	w = AllocWinding (4);
	
	VectorSubtract (org, vright, w->p[0]);
	VectorAdd (w->p[0], vup, w->p[0]);
	
	VectorAdd (org, vright, w->p[1]);
	VectorAdd (w->p[1], vup, w->p[1]);
	
	VectorAdd (org, vright, w->p[2]);
	VectorSubtract (w->p[2], vup, w->p[2]);
	
	VectorSubtract (org, vright, w->p[3]);
	VectorSubtract (w->p[3], vup, w->p[3]);
	
	w->numpoints = 4;
	
	return w;	
}

/*
==================
CopyWinding
==================
*/
winding_t *CopyWinding (winding_t *w)
{
	int			size;
	winding_t	*c;

	c = AllocWinding (w->numpoints);
	size = (int)((winding_t *)0)->p[w->numpoints];
	memcpy (c, w, size);
	return c;
}

/*
==================
ReverseWinding
==================
*/
winding_t	*ReverseWinding (winding_t *w)
{
	int			i;
	winding_t	*c;

	c = AllocWinding (w->numpoints);
	for (i=0 ; i<w->numpoints ; i++)
	{
		VectorCopy (w->p[w->numpoints-1-i], c->p[i]);
	}
	c->numpoints = w->numpoints;
	return c;
}


/*
=============
ClipWindingEpsilon
=============
*/
void ClipWindingEpsilon (winding_t *in, vec3_t normal, vec_t dist, 
				vec_t epsilon, winding_t **front, winding_t **back)
{
	vec_t	dists[MAX_POINTS_ON_WINDING+4];
	int		sides[MAX_POINTS_ON_WINDING+4];
	int		counts[3];
	//MrElusive: DOH can't use statics when unsing multithreading!!!
	vec_t dot;		// VC 4.2 optimizer bug if not static
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t	*f, *b;
	int		maxpts;
	
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	*front = *back = NULL;

	if (!counts[0])
	{
		*back = CopyWinding (in);
		return;
	}
	if (!counts[1])
	{
		*front = CopyWinding (in);
		return;
	}

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	*front = f = AllocWinding (maxpts);
	*back = b = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
			VectorCopy (p1, b->p[b->numpoints]);
			b->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}
		if (sides[i] == SIDE_BACK)
		{
			VectorCopy (p1, b->p[b->numpoints]);
			b->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		p2 = in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, f->p[f->numpoints]);
		f->numpoints++;
		VectorCopy (mid, b->p[b->numpoints]);
		b->numpoints++;
	}
	
	if (f->numpoints > maxpts || b->numpoints > maxpts)
		Error ("ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING || b->numpoints > MAX_POINTS_ON_WINDING)
		Error ("ClipWinding: MAX_POINTS_ON_WINDING");
}


/*
=============
ChopWindingInPlace
=============
*/
void ChopWindingInPlace (winding_t **inout, vec3_t normal, vec_t dist, vec_t epsilon)
{
	winding_t *in;
	vec_t	dists[MAX_POINTS_ON_WINDING+4];
	int sides[MAX_POINTS_ON_WINDING+4];
	int counts[3];
	//MrElusive: DOH can't use statics when unsing multithreading!!!
	vec_t dot;		// VC 4.2 optimizer bug if not static
	int i, j;
	vec_t *p1, *p2;
	vec3_t mid;
	winding_t *f;
	int maxpts;

	in = *inout;
	counts[0] = counts[1] = counts[2] = 0;

// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->p[i], normal);
		dot -= dist;
		dists[i] = dot;
		if (dot > epsilon)
			sides[i] = SIDE_FRONT;
		else if (dot < -epsilon)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
	if (!counts[0])
	{
		FreeWinding (in);
		*inout = NULL;
		return;
	}
	if (!counts[1])
		return;		// inout stays the same

	maxpts = in->numpoints+4;	// cant use counts[0]+2 because
								// of fp grouping errors

	f = AllocWinding (maxpts);
		
	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->p[i];
		
		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, f->p[f->numpoints]);
			f->numpoints++;
		}

		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
	// generate a split point
		p2 = in->p[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (normal[j] == 1)
				mid[j] = dist;
			else if (normal[j] == -1)
				mid[j] = -dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, f->p[f->numpoints]);
		f->numpoints++;
	}
	
	if (f->numpoints > maxpts)
		Error ("ClipWinding: points exceeded estimate");
	if (f->numpoints > MAX_POINTS_ON_WINDING)
		Error ("ClipWinding: MAX_POINTS_ON_WINDING");

	FreeWinding (in);
	*inout = f;
}


/*
=================
ChopWinding

Returns the fragment of in that is on the front side
of the cliping plane.  The original is freed.
=================
*/
winding_t	*ChopWinding (winding_t *in, vec3_t normal, vec_t dist)
{
	winding_t	*f, *b;

	ClipWindingEpsilon (in, normal, dist, ON_EPSILON, &f, &b);
	FreeWinding (in);
	if (b)
		FreeWinding (b);
	return f;
}


/*
=================
CheckWinding

=================
*/
void CheckWinding (winding_t *w)
{
	int		i, j;
	vec_t	*p1, *p2;
	vec_t	d, edgedist;
	vec3_t	dir, edgenormal, facenormal;
	vec_t	area;
	vec_t	facedist;

	if (w->numpoints < 3)
		Error ("CheckWinding: %i points",w->numpoints);
	
	area = WindingArea(w);
	if (area < 1)
		Error ("CheckWinding: %f area", area);

	WindingPlane (w, facenormal, &facedist);
	
	for (i=0 ; i<w->numpoints ; i++)
	{
		p1 = w->p[i];

		for (j=0 ; j<3 ; j++)
			if (p1[j] > BOGUS_RANGE || p1[j] < -BOGUS_RANGE)
				Error ("CheckWinding: BUGUS_RANGE: %f",p1[j]);

		j = i+1 == w->numpoints ? 0 : i+1;
		
	// check the point is on the face plane
		d = DotProduct (p1, facenormal) - facedist;
		if (d < -ON_EPSILON || d > ON_EPSILON)
			Error ("CheckWinding: point off plane");
	
	// check the edge isnt degenerate
		p2 = w->p[j];
		VectorSubtract (p2, p1, dir);
		
		if (VectorLength (dir) < ON_EPSILON)
			Error ("CheckWinding: degenerate edge");
			
		CrossProduct (facenormal, dir, edgenormal);
		VectorNormalize (edgenormal);
		edgedist = DotProduct (p1, edgenormal);
		edgedist += ON_EPSILON;
		
	// all other points must be on front side
		for (j=0 ; j<w->numpoints ; j++)
		{
			if (j == i)
				continue;
			d = DotProduct (w->p[j], edgenormal);
			if (d > edgedist)
				Error ("CheckWinding: non-convex");
		}
	}
}


/*
============
WindingOnPlaneSide
============
*/
int WindingOnPlaneSide (winding_t *w, vec3_t normal, vec_t dist)
{
	qboolean	front, back;
	int			i;
	vec_t		d;

	front = false;
	back = false;
	for (i=0 ; i<w->numpoints ; i++)
	{
		d = DotProduct (w->p[i], normal) - dist;
		if (d < -ON_EPSILON)
		{
			if (front)
				return SIDE_CROSS;
			back = true;
			continue;
		}
		if (d > ON_EPSILON)
		{
			if (back)
				return SIDE_CROSS;
			front = true;
			continue;
		}
	}

	if (back)
		return SIDE_BACK;
	if (front)
		return SIDE_FRONT;
	return SIDE_ON;
}

//#ifdef ME
	#define	CONTINUOUS_EPSILON	0.005
//#else
//	#define	CONTINUOUS_EPSILON	0.001
//#endif

/*
=============
TryMergeWinding

If two polygons share a common edge and the edges that meet at the
common points are both inside the other polygons, merge them

Returns NULL if the faces couldn't be merged, or the new face.
The originals will NOT be freed.
=============
*/

winding_t *TryMergeWinding (winding_t *f1, winding_t *f2, vec3_t planenormal)
{
	vec_t		*p1, *p2, *p3, *p4, *back;
	winding_t	*newf;
	int			i, j, k, l;
	vec3_t		normal, delta;
	vec_t		dot;
	qboolean	keep1, keep2;
	

	//
	// find a common edge
	//	
	p1 = p2 = NULL;	// stop compiler warning
	j = 0;			// 
	
	for (i = 0; i < f1->numpoints; i++)
	{
		p1 = f1->p[i];
		p2 = f1->p[(i+1) % f1->numpoints];
		for (j = 0; j < f2->numpoints; j++)
		{
			p3 = f2->p[j];
			p4 = f2->p[(j+1) % f2->numpoints];
			for (k = 0; k < 3; k++)
			{
				if (fabs(p1[k] - p4[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
				if (fabs(p2[k] - p3[k]) > 0.1)//EQUAL_EPSILON) //ME
					break;
			} //end for
			if (k==3)
				break;
		} //end for
		if (j < f2->numpoints)
			break;
	} //end for
	
	if (i == f1->numpoints)
		return NULL;			// no matching edges

	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	back = f1->p[(i+f1->numpoints-1)%f1->numpoints];
	VectorSubtract (p1, back, delta);
	CrossProduct (planenormal, delta, normal);
	VectorNormalize (normal);
	
	back = f2->p[(j+2)%f2->numpoints];
	VectorSubtract (back, p1, delta);
	dot = DotProduct (delta, normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep1 = (qboolean)(dot < -CONTINUOUS_EPSILON);
	
	back = f1->p[(i+2)%f1->numpoints];
	VectorSubtract (back, p2, delta);
	CrossProduct (planenormal, delta, normal);
	VectorNormalize (normal);

	back = f2->p[(j+f2->numpoints-1)%f2->numpoints];
	VectorSubtract (back, p2, delta);
	dot = DotProduct (delta, normal);
	if (dot > CONTINUOUS_EPSILON)
		return NULL;			// not a convex polygon
	keep2 = (qboolean)(dot < -CONTINUOUS_EPSILON);

	//
	// build the new polygon
	//
	newf = AllocWinding (f1->numpoints + f2->numpoints);
	
	// copy first polygon
	for (k=(i+1)%f1->numpoints ; k != i ; k=(k+1)%f1->numpoints)
	{
		if (k==(i+1)%f1->numpoints && !keep2)
			continue;
		
		VectorCopy (f1->p[k], newf->p[newf->numpoints]);
		newf->numpoints++;
	}
	
	// copy second polygon
	for (l= (j+1)%f2->numpoints ; l != j ; l=(l+1)%f2->numpoints)
	{
		if (l==(j+1)%f2->numpoints && !keep1)
			continue;
		VectorCopy (f2->p[l], newf->p[newf->numpoints]);
		newf->numpoints++;
	}

	return newf;
}

//#ifdef ME
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
winding_t *MergeWindings(winding_t *w1, winding_t *w2, vec3_t planenormal)
{
	winding_t *neww;
	float dist;
	int i, j, n, found, insertafter;
	int sides[MAX_POINTS_ON_WINDING+4];
	vec3_t newp[MAX_POINTS_ON_WINDING+4];
	int numpoints;
	vec3_t edgevec, sepnormal, v;

	RemoveEqualPoints(w1, 0.2);
	numpoints = w1->numpoints;
	memcpy(newp, w1->p, w1->numpoints * sizeof(vec3_t));
	//
	for (i = 0; i < w2->numpoints; i++)
	{
		VectorCopy(w2->p[i], v);
		for (j = 0; j < numpoints; j++)
		{
			VectorSubtract(newp[(j+1)%numpoints],
							newp[(j)%numpoints], edgevec);
			CrossProduct(edgevec, planenormal, sepnormal);
			VectorNormalize(sepnormal);
			if (VectorLength(sepnormal) < 0.9)
			{
				//remove the point from the new winding
				for (n = j; n < numpoints-1; n++)
				{
					VectorCopy(newp[n+1], newp[n]);
					sides[n] = sides[n+1];
				} //end for
				numpoints--;
				j--;
				Log_Print("MergeWindings: degenerate edge on winding %f %f %f\n", sepnormal[0],
																				sepnormal[1],
																				sepnormal[2]);
				continue;
			} //end if
			dist = DotProduct(newp[(j)%numpoints], sepnormal);
			if (DotProduct(v, sepnormal) - dist < -0.1) sides[j] = SIDE_BACK;
			else sides[j] = SIDE_FRONT;
		} //end for
		//remove all unnecesary points
		for (j = 0; j < numpoints;)
		{
			if (sides[j] == SIDE_BACK
				&& sides[(j+1)%numpoints] == SIDE_BACK)
			{
				//remove the point from the new winding
				for (n = (j+1)%numpoints; n < numpoints-1; n++)
				{
					VectorCopy(newp[n+1], newp[n]);
					sides[n] = sides[n+1];
				} //end for
				numpoints--;
			} //end if
			else
			{
				j++;
			} //end else
		} //end for
		//
		found = false;
		for (j = 0; j < numpoints; j++)
		{
			if (sides[j] == SIDE_FRONT
				&& sides[(j+1)%numpoints] == SIDE_BACK)
			{
				if (found) Log_Print("Warning: MergeWindings: front to back found twice\n");
				found = true;
			} //end if
		} //end for
		//
		for (j = 0; j < numpoints; j++)
		{
			if (sides[j] == SIDE_FRONT
				&& sides[(j+1)%numpoints] == SIDE_BACK)
			{
				insertafter = (j+1)%numpoints;
				//insert the new point after j+1
				for (n = numpoints-1; n > insertafter; n--)
				{
					VectorCopy(newp[n], newp[n+1]);
				} //end for
				numpoints++;
				VectorCopy(v, newp[(insertafter+1)%numpoints]);
				break;
			} //end if
		} //end for
	} //end for
	neww = AllocWinding(numpoints);
	neww->numpoints = numpoints;
	memcpy(neww->p, newp, numpoints * sizeof(vec3_t));
	RemoveColinearPoints(neww);
	return neww;
} //end of the function MergeWindings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char *WindingErrorString(void)
{
	return windingerror;
} //end of the function WindingErrorString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int WindingError(winding_t *w)
{
	int		i, j;
	vec_t	*p1, *p2;
	vec_t	d, edgedist;
	vec3_t	dir, edgenormal, facenormal;
	vec_t	area;
	vec_t	facedist;

	if (w->numpoints < 3)
	{
		sprintf(windingerror, "winding %i points", w->numpoints);
		return WE_NOTENOUGHPOINTS;
	} //end if
	
	area = WindingArea(w);
	if (area < 1)
	{
		sprintf(windingerror, "winding %f area", area);
		return WE_SMALLAREA;
	} //end if

	WindingPlane (w, facenormal, &facedist);
	
	for (i=0 ; i<w->numpoints ; i++)
	{
		p1 = w->p[i];

		for (j=0 ; j<3 ; j++)
		{
			if (p1[j] > BOGUS_RANGE || p1[j] < -BOGUS_RANGE)
			{
				sprintf(windingerror, "winding point %d BUGUS_RANGE \'%f %f %f\'", j, p1[0], p1[1], p1[2]);
				return WE_POINTBOGUSRANGE;
			} //end if
		} //end for

		j = i+1 == w->numpoints ? 0 : i+1;
		
	// check the point is on the face plane
		d = DotProduct (p1, facenormal) - facedist;
		if (d < -ON_EPSILON || d > ON_EPSILON)
		{
			sprintf(windingerror, "winding point %d off plane", i);
			return WE_POINTOFFPLANE;
		} //end if
	
	// check the edge isnt degenerate
		p2 = w->p[j];
		VectorSubtract (p2, p1, dir);
		
		if (VectorLength (dir) < ON_EPSILON)
		{
			sprintf(windingerror, "winding degenerate edge %d-%d", i, j);
			return WE_DEGENERATEEDGE;
		} //end if
			
		CrossProduct (facenormal, dir, edgenormal);
		VectorNormalize (edgenormal);
		edgedist = DotProduct (p1, edgenormal);
		edgedist += ON_EPSILON;
		
	// all other points must be on front side
		for (j=0 ; j<w->numpoints ; j++)
		{
			if (j == i)
				continue;
			d = DotProduct (w->p[j], edgenormal);
			if (d > edgedist)
			{
				sprintf(windingerror, "winding non-convex");
				return WE_NONCONVEX;
			} //end if
		} //end for
	} //end for
	return WE_NONE;
} //end of the function WindingError
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void RemoveEqualPoints(winding_t *w, float epsilon)
{
	int i, nump;
	vec3_t v;
	vec3_t p[MAX_POINTS_ON_WINDING];

	VectorCopy(w->p[0], p[0]);
	nump = 1;
	for (i = 1; i < w->numpoints; i++)
	{
		VectorSubtract(w->p[i], p[nump-1], v);
		if (VectorLength(v) > epsilon)
		{
			if (nump >= MAX_POINTS_ON_WINDING)
				Error("RemoveColinearPoints: MAX_POINTS_ON_WINDING");
			VectorCopy (w->p[i], p[nump]);
			nump++;
		} //end if
	} //end for

	if (nump == w->numpoints)
		return;

	w->numpoints = nump;
	memcpy(w->p, p, nump * sizeof(p[0]));
} //end of the function RemoveEqualPoints
//===========================================================================
// adds the given point to a winding at the given spot
// (for instance when spot is zero then the point is added at position zero)
// the original winding is NOT freed
//
// Parameter:				-
// Returns:					the new winding with the added point
// Changes Globals:		-
//===========================================================================
winding_t *AddWindingPoint(winding_t *w, vec3_t point, int spot)
{
	int i, j;
	winding_t *neww;

	if (spot > w->numpoints)
	{
		Error("AddWindingPoint: num > w->numpoints");
	} //end if
	if (spot < 0)
	{
		Error("AddWindingPoint: num < 0");
	} //end if
	neww = AllocWinding(w->numpoints + 1);
	neww->numpoints = w->numpoints + 1;
	for (i = 0, j = 0; i < neww->numpoints; i++)
	{
		if (i == spot)
		{
			VectorCopy(point, neww->p[i]);
		} //end if
		else
		{
			VectorCopy(w->p[j], neww->p[i]);
			j++;
		} //end else
	} //end for
	return neww;
} //end of the function AddWindingPoint
//===========================================================================
// the position where the new point should be added in the winding is
// stored in *spot
//
// Parameter:				-
// Returns:					true if the point is on the winding
// Changes Globals:		-
//===========================================================================
#define MELT_ON_EPSILON		0.2

int PointOnWinding(winding_t *w, vec3_t normal, float dist, vec3_t point, int *spot)
{
	int i, j;
	vec3_t v1, v2;
	vec3_t edgenormal, edgevec;
	float edgedist, dot;

	*spot = 0;
	//the point must be on the winding plane
	dot = DotProduct(point, normal) - dist;
	if (dot < -MELT_ON_EPSILON || dot > MELT_ON_EPSILON) return false;
	//
	for (i = 0; i < w->numpoints; i++)
	{
		j = (i+1) % w->numpoints;
		//get a plane orthogonal to the winding plane through the edge
		VectorSubtract(w->p[j], w->p[i], edgevec);
		CrossProduct(normal, edgevec, edgenormal);
		VectorNormalize(edgenormal);
		edgedist = DotProduct(edgenormal, w->p[i]);
		//point must be not too far from the plane
		dot = DotProduct(point, edgenormal) - edgedist;
		if (dot < -MELT_ON_EPSILON || dot > MELT_ON_EPSILON) continue;
		//vector from first point of winding to the point to test
		VectorSubtract(point, w->p[i], v1);
		//vector from second point of winding to the point to test
		VectorSubtract(point, w->p[j], v2);
		//if the length of the vector is not larger than 0.5 units then
		//the point is assumend to be the same as one of the winding points
		if (VectorNormalize(v1) < 0.5) return false;
		if (VectorNormalize(v2) < 0.5) return false;
		//point must be between the two winding points
		//(the two vectors must be directed towards each other, and on the
		//same straight line)
		if (DotProduct(v1, v2) < -0.99)
		{
			*spot = i + 1;
			return true;
		} //end if
	} //end for
	return false;
} //end of the function PointOnWinding
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int FindPlaneSeperatingWindings(winding_t *w1, winding_t *w2, vec3_t dir,
											vec3_t normal, float *dist)
{
	int i, i2, j, j2, n;
	int sides1[3], sides2[3];
	float dist1, dist2, dot, diff;
	vec3_t normal1, normal2;
	vec3_t v1, v2;

	for (i = 0; i < w1->numpoints; i++)
	{
		i2 = (i+1) % w1->numpoints;
		//
		VectorSubtract(w1->p[i2], w1->p[i], v1);
		if (VectorLength(v1) < 0.1)
		{
			//Log_Write("FindPlaneSeperatingWindings: winding1 with degenerate edge\r\n");
			continue;
		} //end if
		CrossProduct(v1, dir, normal1);
		VectorNormalize(normal1);
		dist1 = DotProduct(normal1, w1->p[i]);
		//
		for (j = 0; j < w2->numpoints; j++)
		{
			j2 = (j+1) % w2->numpoints;
			//
			VectorSubtract(w2->p[j2], w2->p[j], v2);
			if (VectorLength(v2) < 0.1)
			{
				//Log_Write("FindPlaneSeperatingWindings: winding2 with degenerate edge\r\n");
				continue;
			} //end if
			CrossProduct(v2, dir, normal2);
			VectorNormalize(normal2);
			dist2 = DotProduct(normal2, w2->p[j]);
			//
			diff = dist1 - dist2;
			if (diff < -0.1 || diff > 0.1)
			{
				dist2 = -dist2;
				VectorNegate(normal2, normal2);
				diff = dist1 - dist2;
				if (diff < -0.1 || diff > 0.1) continue;
			} //end if
			//check if the normal vectors are equal
			for (n = 0; n < 3; n++)
			{
				diff = normal1[n] - normal2[n];
				if (diff < -0.0001 || diff > 0.0001) break;
			} //end for
			if (n != 3) continue;
			//check on which side of the seperating plane the points of
			//the first winding are
			sides1[0] = sides1[1] = sides1[2] = 0;
			for (n = 0; n < w1->numpoints; n++)
			{
				dot = DotProduct(w1->p[n], normal1) - dist1;
				if (dot > 0.1) sides1[0]++;
				else if (dot < -0.1) sides1[1]++;
				else sides1[2]++;
			} //end for
			//check on which side of the seperating plane the points of
			//the second winding are
			sides2[0] = sides2[1] = sides2[2] = 0;
			for (n = 0; n < w2->numpoints; n++)
			{
				//used normal1 and dist1 (they are equal to normal2 and dist2)
				dot = DotProduct(w2->p[n], normal1) - dist1;
				if (dot > 0.1) sides2[0]++;
				else if (dot < -0.1) sides2[1]++;
				else sides2[2]++;
			} //end for
			//if the first winding has points at both sides
			if (sides1[0] && sides1[1])
			{
				Log_Write("FindPlaneSeperatingWindings: winding1 non-convex\r\n");
				continue;
			} //end if
			//if the second winding has points at both sides
			if (sides2[0] && sides2[1])
			{
				Log_Write("FindPlaneSeperatingWindings: winding2 non-convex\r\n");
				continue;
			} //end if
			//
			if ((!sides1[0] && !sides1[1]) || (!sides2[0] && !sides2[1]))
			{
				//don't use one of the winding planes as the seperating plane
				continue;
			} //end if
			//the windings must be at different sides of the seperating plane
			if ((!sides1[0] && !sides2[1]) || (!sides1[1] && !sides2[0]))
			{
				VectorCopy(normal1, normal);
				*dist = dist1;
				return true;
			} //end if
		} //end for
	} //end for
	return false;
} //end of the function FindPlaneSeperatingWindings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#define WCONVEX_EPSILON		0.2

int WindingsNonConvex(winding_t *w1, winding_t *w2,
							 vec3_t normal1, vec3_t normal2,
							 float dist1, float dist2)
{
	int i;

	if (!w1 || !w2) return false;

	//check if one of the points of face1 is at the back of the plane of face2
	for (i = 0; i < w1->numpoints; i++)
	{
		if (DotProduct(normal2, w1->p[i]) - dist2 > WCONVEX_EPSILON) return true;
	} //end for
	//check if one of the points of face2 is at the back of the plane of face1
	for (i = 0; i < w2->numpoints; i++)
	{
		if (DotProduct(normal1, w2->p[i]) - dist1 > WCONVEX_EPSILON) return true;
	} //end for

	return false;
} //end of the function WindingsNonConvex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
/*
#define VERTEX_EPSILON		0.5

qboolean EqualVertexes(vec3_t v1, vec3_t v2)
{
	float diff;

	diff = v1[0] - v2[0];
	if (diff > -VERTEX_EPSILON && diff < VERTEX_EPSILON)
	{
		diff = v1[1] - v2[1];
		if (diff > -VERTEX_EPSILON && diff < VERTEX_EPSILON)
		{
			diff = v1[2] - v2[2];
			if (diff > -VERTEX_EPSILON && diff < VERTEX_EPSILON)
			{
				return true;
			} //end if
		} //end if
	} //end if
	return false;
} //end of the function EqualVertexes

#define	CONTINUOUS_EPSILON	0.001

winding_t *AAS_MergeWindings(winding_t *w1, winding_t *w2, vec3_t windingnormal)
{
	int n, i, k;
	vec3_t normal, delta;
	winding_t *winding, *neww;
	float dist, dot;
	int p1, p2;
	int points[2][64];
	int numpoints[2] = {0, 0};
	int newnumpoints;
	int keep[2];

	if (!FindPlaneSeperatingWindings(w1, w2, windingnormal, normal, &dist)) return NULL;

	//for both windings
	for (n = 0; n < 2; n++)
	{
		if (n == 0) winding = w1;
		else winding = w2;
		//get the points of the winding which are on the seperating plane
		for (i = 0; i < winding->numpoints; i++)
		{
			dot = DotProduct(winding->p[i], normal) - dist;
			if (dot > -ON_EPSILON && dot < ON_EPSILON)
			{
				//don't allow more than 64 points on the seperating plane
				if (numpoints[n] >= 64) Error("AAS_MergeWindings: more than 64 points on seperating plane\n");
				points[n][numpoints[n]++] = i;
			} //end if
		} //end for
		//there must be at least two points of each winding on the seperating plane
		if (numpoints[n] < 2) return NULL;
	} //end for

	//if the first point of winding1 (which is on the seperating plane) is unequal
	//to the last point of winding2 (which is on the seperating plane)
	if (!EqualVertexes(w1->p[points[0][0]], w2->p[points[1][numpoints[1]-1]]))
	{
		return NULL;
	} //end if
	//if the last point of winding1 (which is on the seperating plane) is unequal
	//to the first point of winding2 (which is on the seperating plane)
	if (!EqualVertexes(w1->p[points[0][numpoints[0]-1]], w2->p[points[1][0]]))
	{
		return NULL;
	} //end if
	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	//first point of winding1 which is on the seperating plane
	p1 = points[0][0];
	//point before p1
	p2 = (p1 + w1->numpoints - 1) % w1->numpoints;
	VectorSubtract(w1->p[p1], w1->p[p2], delta);
	CrossProduct(windingnormal, delta, normal);
	VectorNormalize(normal, normal);

	//last point of winding2 which is on the seperating plane
	p1 = points[1][numpoints[1]-1];
	//point after p1
	p2 = (p1 + 1) % w2->numpoints;
	VectorSubtract(w2->p[p2], w2->p[p1], delta);
	dot = DotProduct(delta, normal);
	if (dot > CONTINUOUS_EPSILON) return NULL; //merging would create a non-convex polygon
	keep[0] = (qboolean)(dot < -CONTINUOUS_EPSILON);

	//first point of winding2 which is on the seperating plane
	p1 = points[1][0];
	//point before p1
	p2 = (p1 + w2->numpoints - 1) % w2->numpoints;
	VectorSubtract(w2->p[p1], w2->p[p2], delta);
	CrossProduct(windingnormal, delta, normal);
	VectorNormalize(normal, normal);

	//last point of winding1 which is on the seperating plane
	p1 = points[0][numpoints[0]-1];
	//point after p1
	p2 = (p1 + 1) % w1->numpoints;
	VectorSubtract(w1->p[p2], w1->p[p1], delta);
	dot = DotProduct(delta, normal);
	if (dot > CONTINUOUS_EPSILON) return NULL; //merging would create a non-convex polygon
	keep[1] = (qboolean)(dot < -CONTINUOUS_EPSILON);

	//number of points on the new winding
	newnumpoints = w1->numpoints - numpoints[0] + w2->numpoints - numpoints[1] + 2;
	//allocate the winding
	neww = AllocWinding(newnumpoints);
	neww->numpoints = newnumpoints;
	//copy all the points
	k = 0;
	//for both windings
	for (n = 0; n < 2; n++)
	{
		if (n == 0) winding = w1;
		else winding = w2;
		//copy the points of the winding starting with the last point on the
		//seperating plane and ending before the first point on the seperating plane
		for (i = points[n][numpoints[n]-1]; i != points[n][0]; i = (i+1)%winding->numpoints)
		{
			if (k >= newnumpoints)
			{
				Log_Print("numpoints[0] = %d\n", numpoints[0]);
				Log_Print("numpoints[1] = %d\n", numpoints[1]);
				Error("AAS_MergeWindings: k = %d >= newnumpoints = %d\n", k, newnumpoints);
			} //end if
			VectorCopy(winding->p[i], neww->p[k]);
			k++;
		} //end for
	} //end for
	RemoveEqualPoints(neww);
	if (!WindingIsOk(neww, 1))
	{
		Log_Print("AAS_MergeWindings: winding not ok after merging\n");
		FreeWinding(neww);
		return NULL;
	} //end if
	return neww;
} //end of the function AAS_MergeWindings*/
//#endif //ME
