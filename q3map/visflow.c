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
#include "vis.h"

/*

  each portal will have a list of all possible to see from first portal

  if (!thread->portalmightsee[portalnum])

  portal mightsee

  for p2 = all other portals in leaf
	get sperating planes
	for all portals that might be seen by p2
		mark as unseen if not present in seperating plane
	flood fill a new mightsee
	save as passagemightsee


  void CalcMightSee (leaf_t *leaf, 
*/

int CountBits (byte *bits, int numbits)
{
	int		i;
	int		c;

	c = 0;
	for (i=0 ; i<numbits ; i++)
		if (bits[i>>3] & (1<<(i&7)) )
			c++;

	return c;
}

int		c_fullskip;
int		c_portalskip, c_leafskip;
int		c_vistest, c_mighttest;

int		c_chop, c_nochop;

int		active;

void CheckStack (leaf_t *leaf, threaddata_t *thread)
{
	pstack_t	*p, *p2;

	for (p=thread->pstack_head.next ; p ; p=p->next)
	{
//		_printf ("=");
		if (p->leaf == leaf)
			Error ("CheckStack: leaf recursion");
		for (p2=thread->pstack_head.next ; p2 != p ; p2=p2->next)
			if (p2->leaf == p->leaf)
				Error ("CheckStack: late leaf recursion");
	}
//	_printf ("\n");
}


winding_t *AllocStackWinding (pstack_t *stack)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if (stack->freewindings[i])
		{
			stack->freewindings[i] = 0;
			return &stack->windings[i];
		}
	}

	Error ("AllocStackWinding: failed");

	return NULL;
}

void FreeStackWinding (winding_t *w, pstack_t *stack)
{
	int		i;

	i = w - stack->windings;

	if (i<0 || i>2)
		return;		// not from local

	if (stack->freewindings[i])
		Error ("FreeStackWinding: allready free");
	stack->freewindings[i] = 1;
}

/*
==============
VisChopWinding

==============
*/
winding_t	*VisChopWinding (winding_t *in, pstack_t *stack, plane_t *split)
{
	vec_t	dists[128];
	int		sides[128];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t	*neww;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if (!counts[1])
		return in;		// completely on front side
	
	if (!counts[0])
	{
		FreeStackWinding (in, stack);
		return NULL;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	neww = AllocStackWinding (stack);

	neww->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING)
		{
			FreeStackWinding (neww, stack);
			return in;		// can't chop -- fall back to original
		}

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING)
		{
			FreeStackWinding (neww, stack);
			return in;		// can't chop -- fall back to original
		}

		// generate a split point
		p2 = in->points[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}
	
	// free the original winding
	FreeStackWinding (in, stack);
	
	return neww;
}

/*
==============
ClipToSeperators

Source, pass, and target are an ordering of portals.

Generates seperating planes canidates by taking two points from source and one
point from pass, and clips target by them.

If target is totally clipped away, that portal can not be seen through.

Normal clip keeps target on the same side as pass, which is correct if the
order goes source, pass, target.  If the order goes pass, source, target then
flipclip should be set.
==============
*/
winding_t	*ClipToSeperators (winding_t *source, winding_t *pass, winding_t *target, qboolean flipclip, pstack_t *stack)
{
	int			i, j, k, l;
	plane_t		plane;
	vec3_t		v1, v2;
	float		d;
	vec_t		length;
	int			counts[3];
	qboolean		fliptest;

	// check all combinations	
	for (i=0 ; i<source->numpoints ; i++)
	{
		l = (i+1)%source->numpoints;
		VectorSubtract (source->points[l] , source->points[i], v1);

		// find a vertex of pass that makes a plane that puts all of the
		// vertexes of pass on the front side and all of the vertexes of
		// source on the back side
		for (j=0 ; j<pass->numpoints ; j++)
		{
			VectorSubtract (pass->points[j], source->points[i], v2);

			plane.normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
			plane.normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
			plane.normal[2] = v1[0]*v2[1] - v1[1]*v2[0];
			
			// if points don't make a valid plane, skip it

			length = plane.normal[0] * plane.normal[0]
			+ plane.normal[1] * plane.normal[1]
			+ plane.normal[2] * plane.normal[2];
			
			if (length < ON_EPSILON)
				continue;

			length = 1/sqrt(length);
			
			plane.normal[0] *= length;
			plane.normal[1] *= length;
			plane.normal[2] *= length;

			plane.dist = DotProduct (pass->points[j], plane.normal);

			//
			// find out which side of the generated seperating plane has the
			// source portal
			//
#if 1
			fliptest = qfalse;
			for (k=0 ; k<source->numpoints ; k++)
			{
				if (k == i || k == l)
					continue;
				d = DotProduct (source->points[k], plane.normal) - plane.dist;
				if (d < -ON_EPSILON)
				{	// source is on the negative side, so we want all
					// pass and target on the positive side
					fliptest = qfalse;
					break;
				}
				else if (d > ON_EPSILON)
				{	// source is on the positive side, so we want all
					// pass and target on the negative side
					fliptest = qtrue;
					break;
				}
			}
			if (k == source->numpoints)
				continue;		// planar with source portal
#else
			fliptest = flipclip;
#endif
			//
			// flip the normal if the source portal is backwards
			//
			if (fliptest)
			{
				VectorSubtract (vec3_origin, plane.normal, plane.normal);
				plane.dist = -plane.dist;
			}
#if 1
			//
			// if all of the pass portal points are now on the positive side,
			// this is the seperating plane
			//
			counts[0] = counts[1] = counts[2] = 0;
			for (k=0 ; k<pass->numpoints ; k++)
			{
				if (k==j)
					continue;
				d = DotProduct (pass->points[k], plane.normal) - plane.dist;
				if (d < -ON_EPSILON)
					break;
				else if (d > ON_EPSILON)
					counts[0]++;
				else
					counts[2]++;
			}
			if (k != pass->numpoints)
				continue;	// points on negative side, not a seperating plane
				
			if (!counts[0])
				continue;	// planar with seperating plane
#else
			k = (j+1)%pass->numpoints;
			d = DotProduct (pass->points[k], plane.normal) - plane.dist;
			if (d < -ON_EPSILON)
				continue;
			k = (j+pass->numpoints-1)%pass->numpoints;
			d = DotProduct (pass->points[k], plane.normal) - plane.dist;
			if (d < -ON_EPSILON)
				continue;			
#endif
			//
			// flip the normal if we want the back side
			//
			if (flipclip)
			{
				VectorSubtract (vec3_origin, plane.normal, plane.normal);
				plane.dist = -plane.dist;
			}

#ifdef SEPERATORCACHE
			stack->seperators[flipclip][stack->numseperators[flipclip]] = plane;
			if (++stack->numseperators[flipclip] >= MAX_SEPERATORS)
				Error("MAX_SEPERATORS");
#endif
			//MrE: fast check first
			d = DotProduct (stack->portal->origin, plane.normal) - plane.dist;
			//if completely at the back of the seperator plane
			if (d < -stack->portal->radius)
				return NULL;
			//if completely on the front of the seperator plane
			if (d > stack->portal->radius)
				break;

			//
			// clip target by the seperating plane
			//
			target = VisChopWinding (target, stack, &plane);
			if (!target)
				return NULL;		// target is not visible

			break;		// optimization by Antony Suter
		}
	}
	
	return target;
}

/*
==================
RecursiveLeafFlow

Flood fill through the leafs
If src_portal is NULL, this is the originating leaf
==================
*/
void RecursiveLeafFlow (int leafnum, threaddata_t *thread, pstack_t *prevstack)
{
	pstack_t	stack;
	vportal_t	*p;
	plane_t		backplane;
	leaf_t 		*leaf;
	int			i, j, n;
	long		*test, *might, *prevmight, *vis, more;
	int			pnum;

	thread->c_chains++;

	leaf = &leafs[leafnum];
//	CheckStack (leaf, thread);

	prevstack->next = &stack;

	stack.next = NULL;
	stack.leaf = leaf;
	stack.portal = NULL;
	stack.depth = prevstack->depth + 1;

#ifdef SEPERATORCACHE
	stack.numseperators[0] = 0;
	stack.numseperators[1] = 0;
#endif

	might = (long *)stack.mightsee;
	vis = (long *)thread->base->portalvis;
	
	// check all portals for flowing into other leafs	
	for (i = 0; i < leaf->numportals; i++)
	{
		p = leaf->portals[i];
		if (p->removed)
			continue;
		pnum = p - portals;

		/* MrE: portal trace debug code
		{
			int portaltrace[] = {13, 16, 17, 37};
			pstack_t *s;

			s = &thread->pstack_head;
			for (j = 0; s->next && j < sizeof(portaltrace)/sizeof(int) - 1; j++, s = s->next)
			{
				if (s->portal->num != portaltrace[j])
					break;
			}
			if (j >= sizeof(portaltrace)/sizeof(int) - 1)
			{
				if (p->num == portaltrace[j])
					n = 0; //traced through all the portals
			}
		}
		*/

		if ( ! (prevstack->mightsee[pnum >> 3] & (1<<(pnum&7)) ) )
		{
			continue;	// can't possibly see it
		}

		// if the portal can't see anything we haven't allready seen, skip it
		if (p->status == stat_done)
		{
			test = (long *)p->portalvis;
		}
		else
		{
			test = (long *)p->portalflood;
		}

		more = 0;
		prevmight = (long *)prevstack->mightsee;
		for (j=0 ; j<portallongs ; j++)
		{
			might[j] = prevmight[j] & test[j];
			more |= (might[j] & ~vis[j]);
		}
		
		if (!more && 
			(thread->base->portalvis[pnum>>3] & (1<<(pnum&7))) )
		{	// can't see anything new
			continue;
		}

		// get plane of portal, point normal into the neighbor leaf
		stack.portalplane = p->plane;
		VectorSubtract (vec3_origin, p->plane.normal, backplane.normal);
		backplane.dist = -p->plane.dist;
		
//		c_portalcheck++;
		
		stack.portal = p;
		stack.next = NULL;
		stack.freewindings[0] = 1;
		stack.freewindings[1] = 1;
		stack.freewindings[2] = 1;
		
#if 1
		{
			float d;

			d = DotProduct (p->origin, thread->pstack_head.portalplane.normal);
			d -= thread->pstack_head.portalplane.dist;
			if (d < -p->radius)
			{
				continue;
			}
			else if (d > p->radius)
			{
				stack.pass = p->winding;
			}
			else	
			{
				stack.pass = VisChopWinding (p->winding, &stack, &thread->pstack_head.portalplane);
				if (!stack.pass)
					continue;
			}
		}
#else
		stack.pass = VisChopWinding (p->winding, &stack, &thread->pstack_head.portalplane);
		if (!stack.pass)
			continue;
#endif

	
#if 1
		{
			float d;

			d = DotProduct (thread->base->origin, p->plane.normal);
			d -= p->plane.dist;
			//MrE: vis-bug fix
			//if (d > p->radius)
			if (d > thread->base->radius)
			{
				continue;
			}
			//MrE: vis-bug fix
			//if (d < -p->radius)
			else if (d < -thread->base->radius)
			{
				stack.source = prevstack->source;
			}
			else	
			{
				stack.source = VisChopWinding (prevstack->source, &stack, &backplane);
				//FIXME: shouldn't we create a new source origin and radius for fast checks?
				if (!stack.source)
					continue;
			}
		}
#else
		stack.source = VisChopWinding (prevstack->source, &stack, &backplane);
		if (!stack.source)
			continue;
#endif

		if (!prevstack->pass)
		{	// the second leaf can only be blocked if coplanar

			// mark the portal as visible
			thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

			RecursiveLeafFlow (p->leaf, thread, &stack);
			continue;
		}

#ifdef SEPERATORCACHE
		if (stack.numseperators[0])
		{
			for (n = 0; n < stack.numseperators[0]; n++)
			{
				stack.pass = VisChopWinding (stack.pass, &stack, &stack.seperators[0][n]);
				if (!stack.pass)
					break;		// target is not visible
			}
			if (n < stack.numseperators[0])
				continue;
		}
		else
		{
			stack.pass = ClipToSeperators (prevstack->source, prevstack->pass, stack.pass, qfalse, &stack);
		}
#else
		stack.pass = ClipToSeperators (stack.source, prevstack->pass, stack.pass, qfalse, &stack);
#endif
		if (!stack.pass)
			continue;

#ifdef SEPERATORCACHE
		if (stack.numseperators[1])
		{
			for (n = 0; n < stack.numseperators[1]; n++)
			{
				stack.pass = VisChopWinding (stack.pass, &stack, &stack.seperators[1][n]);
				if (!stack.pass)
					break;		// target is not visible
			}
		}
		else
		{
			stack.pass = ClipToSeperators (prevstack->pass, prevstack->source, stack.pass, qtrue, &stack);
		}
#else
		stack.pass = ClipToSeperators (prevstack->pass, stack.source, stack.pass, qtrue, &stack);
#endif
		if (!stack.pass)
			continue;

		// mark the portal as visible
		thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

		// flow through it for real
		RecursiveLeafFlow (p->leaf, thread, &stack);
		//
		stack.next = NULL;
	}	
}

/*
===============
PortalFlow

generates the portalvis bit vector
===============
*/
void PortalFlow (int portalnum)
{
	threaddata_t	data;
	int				i;
	vportal_t		*p;
	int				c_might, c_can;

#ifdef MREDEBUG
	_printf("\r%6d", portalnum);
#endif

	p = sorted_portals[portalnum];

	if (p->removed)
	{
		p->status = stat_done;
		return;
	}

	p->status = stat_working;

	c_might = CountBits (p->portalflood, numportals*2);

	memset (&data, 0, sizeof(data));
	data.base = p;
	
	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	data.pstack_head.depth = 0;
	for (i=0 ; i<portallongs ; i++)
		((long *)data.pstack_head.mightsee)[i] = ((long *)p->portalflood)[i];

	RecursiveLeafFlow (p->leaf, &data, &data.pstack_head);

	p->status = stat_done;

	c_can = CountBits (p->portalvis, numportals*2);

	qprintf ("portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n", 
		(int)(p - portals),	c_might, c_can, data.c_chains);
}

/*
==================
RecursivePassageFlow
==================
*/
void RecursivePassageFlow (vportal_t *portal, threaddata_t *thread, pstack_t *prevstack)
{
	pstack_t	stack;
	vportal_t	*p;
	leaf_t 		*leaf;
	passage_t	*passage, *nextpassage;
	int			i, j;
	long		*might, *vis, *prevmight, *cansee, *portalvis, more;
	int			pnum;

	leaf = &leafs[portal->leaf];

	prevstack->next = &stack;

	stack.next = NULL;
	stack.depth = prevstack->depth + 1;

	vis = (long *)thread->base->portalvis;

	passage = portal->passages;
	nextpassage = passage;
	// check all portals for flowing into other leafs	
	for (i = 0; i < leaf->numportals; i++, passage = nextpassage)
	{
		p = leaf->portals[i];
		if ( p->removed ) {
			continue;
		}
		nextpassage = passage->next;
		pnum = p - portals;

		if ( ! (prevstack->mightsee[pnum >> 3] & (1<<(pnum&7)) ) ) {
			continue;	// can't possibly see it
		}

		// mark the portal as visible
		thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

		prevmight = (long *)prevstack->mightsee;
		cansee = (long *)passage->cansee;
		might = (long *)stack.mightsee;
		memcpy(might, prevmight, portalbytes);
		if (p->status == stat_done)
			portalvis = (long *) p->portalvis;
		else
			portalvis = (long *) p->portalflood;
		more = 0;
		for (j = 0; j < portallongs; j++)
		{
			if (*might)
			{
				*might &= *cansee++ & *portalvis++;
				more |= (*might & ~vis[j]);
			}
			else
			{
				cansee++;
				portalvis++;
			}
			might++;
		}

		if ( !more ) {
			// can't see anything new
			continue;
		}

		// flow through it for real
		RecursivePassageFlow(p, thread, &stack);

		stack.next = NULL;
	}
}

/*
===============
PassageFlow
===============
*/
void PassageFlow (int portalnum)
{
	threaddata_t	data;
	int				i;
	vportal_t		*p;
//	int				c_might, c_can;

#ifdef MREDEBUG
	_printf("\r%6d", portalnum);
#endif

	p = sorted_portals[portalnum];

	if (p->removed)
	{
		p->status = stat_done;
		return;
	}

	p->status = stat_working;

//	c_might = CountBits (p->portalflood, numportals*2);

	memset (&data, 0, sizeof(data));
	data.base = p;
	
	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	data.pstack_head.depth = 0;
	for (i=0 ; i<portallongs ; i++)
		((long *)data.pstack_head.mightsee)[i] = ((long *)p->portalflood)[i];

	RecursivePassageFlow (p, &data, &data.pstack_head);

	p->status = stat_done;

	/*
	c_can = CountBits (p->portalvis, numportals*2);

	qprintf ("portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n", 
		(int)(p - portals),	c_might, c_can, data.c_chains);
	*/
}

/*
==================
RecursivePassagePortalFlow
==================
*/
void RecursivePassagePortalFlow (vportal_t *portal, threaddata_t *thread, pstack_t *prevstack)
{
	pstack_t	stack;
	vportal_t	*p;
	leaf_t 		*leaf;
	plane_t		backplane;
	passage_t	*passage, *nextpassage;
	int			i, j, n;
	long		*might, *vis, *prevmight, *cansee, *portalvis, more;
	int			pnum;

//	thread->c_chains++;

	leaf = &leafs[portal->leaf];
//	CheckStack (leaf, thread);

	prevstack->next = &stack;

	stack.next = NULL;
	stack.leaf = leaf;
	stack.portal = NULL;
	stack.depth = prevstack->depth + 1;

#ifdef SEPERATORCACHE
	stack.numseperators[0] = 0;
	stack.numseperators[1] = 0;
#endif

	vis = (long *)thread->base->portalvis;

	passage = portal->passages;
	nextpassage = passage;
	// check all portals for flowing into other leafs	
	for (i = 0; i < leaf->numportals; i++, passage = nextpassage)
	{
		p = leaf->portals[i];
		if (p->removed)
			continue;
		nextpassage = passage->next;
		pnum = p - portals;

		if ( ! (prevstack->mightsee[pnum >> 3] & (1<<(pnum&7)) ) )
			continue;	// can't possibly see it

		prevmight = (long *)prevstack->mightsee;
		cansee = (long *)passage->cansee;
		might = (long *)stack.mightsee;
		memcpy(might, prevmight, portalbytes);
		if (p->status == stat_done)
			portalvis = (long *) p->portalvis;
		else
			portalvis = (long *) p->portalflood;
		more = 0;
		for (j = 0; j < portallongs; j++)
		{
			if (*might)
			{
				*might &= *cansee++ & *portalvis++;
				more |= (*might & ~vis[j]);
			}
			else
			{
				cansee++;
				portalvis++;
			}
			might++;
		}

		if (!more && (thread->base->portalvis[pnum>>3] & (1<<(pnum&7))) )
		{	// can't see anything new
			continue;
		}

		// get plane of portal, point normal into the neighbor leaf
		stack.portalplane = p->plane;
		VectorSubtract (vec3_origin, p->plane.normal, backplane.normal);
		backplane.dist = -p->plane.dist;
		
//		c_portalcheck++;
		
		stack.portal = p;
		stack.next = NULL;
		stack.freewindings[0] = 1;
		stack.freewindings[1] = 1;
		stack.freewindings[2] = 1;

#if 1
		{
			float d;

			d = DotProduct (p->origin, thread->pstack_head.portalplane.normal);
			d -= thread->pstack_head.portalplane.dist;
			if (d < -p->radius)
			{
				continue;
			}
			else if (d > p->radius)
			{
				stack.pass = p->winding;
			}
			else	
			{
				stack.pass = VisChopWinding (p->winding, &stack, &thread->pstack_head.portalplane);
				if (!stack.pass)
					continue;
			}
		}
#else
		stack.pass = VisChopWinding (p->winding, &stack, &thread->pstack_head.portalplane);
		if (!stack.pass)
			continue;
#endif

	
#if 1
		{
			float d;

			d = DotProduct (thread->base->origin, p->plane.normal);
			d -= p->plane.dist;
			//MrE: vis-bug fix
			//if (d > p->radius)
			if (d > thread->base->radius)
			{
				continue;
			}
			//MrE: vis-bug fix
			//if (d < -p->radius)
			else if (d < -thread->base->radius)
			{
				stack.source = prevstack->source;
			}
			else	
			{
				stack.source = VisChopWinding (prevstack->source, &stack, &backplane);
				//FIXME: shouldn't we create a new source origin and radius for fast checks?
				if (!stack.source)
					continue;
			}
		}
#else
		stack.source = VisChopWinding (prevstack->source, &stack, &backplane);
		if (!stack.source)
			continue;
#endif

		if (!prevstack->pass)
		{	// the second leaf can only be blocked if coplanar

			// mark the portal as visible
			thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

			RecursivePassagePortalFlow (p, thread, &stack);
			continue;
		}

#ifdef SEPERATORCACHE
		if (stack.numseperators[0])
		{
			for (n = 0; n < stack.numseperators[0]; n++)
			{
				stack.pass = VisChopWinding (stack.pass, &stack, &stack.seperators[0][n]);
				if (!stack.pass)
					break;		// target is not visible
			}
			if (n < stack.numseperators[0])
				continue;
		}
		else
		{
			stack.pass = ClipToSeperators (prevstack->source, prevstack->pass, stack.pass, qfalse, &stack);
		}
#else
		stack.pass = ClipToSeperators (stack.source, prevstack->pass, stack.pass, qfalse, &stack);
#endif
		if (!stack.pass)
			continue;

#ifdef SEPERATORCACHE
		if (stack.numseperators[1])
		{
			for (n = 0; n < stack.numseperators[1]; n++)
			{
				stack.pass = VisChopWinding (stack.pass, &stack, &stack.seperators[1][n]);
				if (!stack.pass)
					break;		// target is not visible
			}
		}
		else
		{
			stack.pass = ClipToSeperators (prevstack->pass, prevstack->source, stack.pass, qtrue, &stack);
		}
#else
		stack.pass = ClipToSeperators (prevstack->pass, stack.source, stack.pass, qtrue, &stack);
#endif
		if (!stack.pass)
			continue;

		// mark the portal as visible
		thread->base->portalvis[pnum>>3] |= (1<<(pnum&7));

		// flow through it for real
		RecursivePassagePortalFlow(p, thread, &stack);
		//
		stack.next = NULL;
	}
}

/*
===============
PassagePortalFlow
===============
*/
void PassagePortalFlow (int portalnum)
{
	threaddata_t	data;
	int				i;
	vportal_t		*p;
//	int				c_might, c_can;

#ifdef MREDEBUG
	_printf("\r%6d", portalnum);
#endif

	p = sorted_portals[portalnum];

	if (p->removed)
	{
		p->status = stat_done;
		return;
	}

	p->status = stat_working;

//	c_might = CountBits (p->portalflood, numportals*2);

	memset (&data, 0, sizeof(data));
	data.base = p;
	
	data.pstack_head.portal = p;
	data.pstack_head.source = p->winding;
	data.pstack_head.portalplane = p->plane;
	data.pstack_head.depth = 0;
	for (i=0 ; i<portallongs ; i++)
		((long *)data.pstack_head.mightsee)[i] = ((long *)p->portalflood)[i];

	RecursivePassagePortalFlow (p, &data, &data.pstack_head);

	p->status = stat_done;

	/*
	c_can = CountBits (p->portalvis, numportals*2);

	qprintf ("portal:%4i  mightsee:%4i  cansee:%4i (%i chains)\n", 
		(int)(p - portals),	c_might, c_can, data.c_chains);
	*/
}

winding_t *PassageChopWinding (winding_t *in, winding_t *out, plane_t *split)
{
	vec_t	dists[128];
	int		sides[128];
	int		counts[3];
	vec_t	dot;
	int		i, j;
	vec_t	*p1, *p2;
	vec3_t	mid;
	winding_t	*neww;

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for (i=0 ; i<in->numpoints ; i++)
	{
		dot = DotProduct (in->points[i], split->normal);
		dot -= split->dist;
		dists[i] = dot;
		if (dot > ON_EPSILON)
			sides[i] = SIDE_FRONT;
		else if (dot < -ON_EPSILON)
			sides[i] = SIDE_BACK;
		else
		{
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}

	if (!counts[1])
		return in;		// completely on front side
	
	if (!counts[0])
	{
		return NULL;
	}

	sides[i] = sides[0];
	dists[i] = dists[0];
	
	neww = out;

	neww->numpoints = 0;

	for (i=0 ; i<in->numpoints ; i++)
	{
		p1 = in->points[i];

		if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING)
		{
			return in;		// can't chop -- fall back to original
		}

		if (sides[i] == SIDE_ON)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
			continue;
		}
	
		if (sides[i] == SIDE_FRONT)
		{
			VectorCopy (p1, neww->points[neww->numpoints]);
			neww->numpoints++;
		}
		
		if (sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;
			
		if (neww->numpoints == MAX_POINTS_ON_FIXED_WINDING)
		{
			return in;		// can't chop -- fall back to original
		}

		// generate a split point
		p2 = in->points[(i+1)%in->numpoints];
		
		dot = dists[i] / (dists[i]-dists[i+1]);
		for (j=0 ; j<3 ; j++)
		{	// avoid round off error when possible
			if (split->normal[j] == 1)
				mid[j] = split->dist;
			else if (split->normal[j] == -1)
				mid[j] = -split->dist;
			else
				mid[j] = p1[j] + dot*(p2[j]-p1[j]);
		}
			
		VectorCopy (mid, neww->points[neww->numpoints]);
		neww->numpoints++;
	}
	
	return neww;
}

/*
===============
AddSeperators
===============
*/
int AddSeperators (winding_t *source, winding_t *pass, qboolean flipclip, plane_t *seperators, int maxseperators)
{
	int			i, j, k, l;
	plane_t		plane;
	vec3_t		v1, v2;
	float		d;
	vec_t		length;
	int			counts[3], numseperators;
	qboolean	fliptest;

	numseperators = 0;
	// check all combinations	
	for (i=0 ; i<source->numpoints ; i++)
	{
		l = (i+1)%source->numpoints;
		VectorSubtract (source->points[l] , source->points[i], v1);

		// find a vertex of pass that makes a plane that puts all of the
		// vertexes of pass on the front side and all of the vertexes of
		// source on the back side
		for (j=0 ; j<pass->numpoints ; j++)
		{
			VectorSubtract (pass->points[j], source->points[i], v2);

			plane.normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
			plane.normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
			plane.normal[2] = v1[0]*v2[1] - v1[1]*v2[0];
			
			// if points don't make a valid plane, skip it

			length = plane.normal[0] * plane.normal[0]
			+ plane.normal[1] * plane.normal[1]
			+ plane.normal[2] * plane.normal[2];
			
			if (length < ON_EPSILON)
				continue;

			length = 1/sqrt(length);
			
			plane.normal[0] *= length;
			plane.normal[1] *= length;
			plane.normal[2] *= length;

			plane.dist = DotProduct (pass->points[j], plane.normal);

			//
			// find out which side of the generated seperating plane has the
			// source portal
			//
#if 1
			fliptest = qfalse;
			for (k=0 ; k<source->numpoints ; k++)
			{
				if (k == i || k == l)
					continue;
				d = DotProduct (source->points[k], plane.normal) - plane.dist;
				if (d < -ON_EPSILON)
				{	// source is on the negative side, so we want all
					// pass and target on the positive side
					fliptest = qfalse;
					break;
				}
				else if (d > ON_EPSILON)
				{	// source is on the positive side, so we want all
					// pass and target on the negative side
					fliptest = qtrue;
					break;
				}
			}
			if (k == source->numpoints)
				continue;		// planar with source portal
#else
			fliptest = flipclip;
#endif
			//
			// flip the normal if the source portal is backwards
			//
			if (fliptest)
			{
				VectorSubtract (vec3_origin, plane.normal, plane.normal);
				plane.dist = -plane.dist;
			}
#if 1
			//
			// if all of the pass portal points are now on the positive side,
			// this is the seperating plane
			//
			counts[0] = counts[1] = counts[2] = 0;
			for (k=0 ; k<pass->numpoints ; k++)
			{
				if (k==j)
					continue;
				d = DotProduct (pass->points[k], plane.normal) - plane.dist;
				if (d < -ON_EPSILON)
					break;
				else if (d > ON_EPSILON)
					counts[0]++;
				else
					counts[2]++;
			}
			if (k != pass->numpoints)
				continue;	// points on negative side, not a seperating plane
				
			if (!counts[0])
				continue;	// planar with seperating plane
#else
			k = (j+1)%pass->numpoints;
			d = DotProduct (pass->points[k], plane.normal) - plane.dist;
			if (d < -ON_EPSILON)
				continue;
			k = (j+pass->numpoints-1)%pass->numpoints;
			d = DotProduct (pass->points[k], plane.normal) - plane.dist;
			if (d < -ON_EPSILON)
				continue;			
#endif
			//
			// flip the normal if we want the back side
			//
			if (flipclip)
			{
				VectorSubtract (vec3_origin, plane.normal, plane.normal);
				plane.dist = -plane.dist;
			}

			if (numseperators >= maxseperators)
				Error("max seperators");
			seperators[numseperators] = plane;
			numseperators++;
			break;
		}
	}
	return numseperators;
}

/*
===============
CreatePassages

MrE: create passages from one portal to all the portals in the leaf the portal leads to
	 every passage has a cansee bit string with all the portals that can be
	 seen through the passage
===============
*/
void CreatePassages(int portalnum)
{
	int i, j, k, n, numseperators, numsee;
	float d;
	vportal_t *portal, *p, *target;
	leaf_t *leaf;
	passage_t	*passage, *lastpassage;
	plane_t seperators[MAX_SEPERATORS*2];
	winding_t *w;
	winding_t in, out, *res;

#ifdef MREDEBUG
	_printf("\r%6d", portalnum);
#endif

	portal = sorted_portals[portalnum];

	if (portal->removed)
	{
		portal->status = stat_done;
		return;
	}

	lastpassage = NULL;
	leaf = &leafs[portal->leaf];
	for (i = 0; i < leaf->numportals; i++)
	{
		target = leaf->portals[i];
		if (target->removed)
			continue;

		passage = (passage_t *) malloc(sizeof(passage_t) + portalbytes);
		memset(passage, 0, sizeof(passage_t) + portalbytes);
		numseperators = AddSeperators(portal->winding, target->winding, qfalse, seperators, MAX_SEPERATORS*2);
		numseperators += AddSeperators(target->winding, portal->winding, qtrue, &seperators[numseperators], MAX_SEPERATORS*2-numseperators);

		passage->next = NULL;
		if (lastpassage)
			lastpassage->next = passage;
		else
			portal->passages = passage;
		lastpassage = passage;

		numsee = 0;
		//create the passage->cansee
		for (j = 0; j < numportals * 2; j++)
		{
			p = &portals[j];
			if (p->removed)
				continue;
			if ( ! (target->portalflood[j >> 3] & (1<<(j&7)) ) )
				continue;
			if ( ! (portal->portalflood[j >> 3] & (1<<(j&7)) ) )
				continue;
			for (k = 0; k < numseperators; k++)
			{
				//
				d = DotProduct (p->origin, seperators[k].normal) - seperators[k].dist;
				//if completely at the back of the seperator plane
				if (d < -p->radius + ON_EPSILON)
					break;
				w = p->winding;
				for (n = 0; n < w->numpoints; n++)
				{
					d = DotProduct (w->points[n], seperators[k].normal) - seperators[k].dist;
					//if at the front of the seperator
					if (d > ON_EPSILON)
						break;
				}
				//if no points are at the front of the seperator
				if (n >= w->numpoints)
					break;
			}
			if (k < numseperators)
				continue;
			memcpy(&in, p->winding, sizeof(winding_t));
			for (k = 0; k < numseperators; k++)
			{
				res = PassageChopWinding(&in, &out, &seperators[k]);
				if (res == &out)
					memcpy(&in, &out, sizeof(winding_t));
				if (res == NULL)
					break;
			}
			if (k < numseperators)
				continue;
			passage->cansee[j >> 3] |= (1<<(j&7));
			numsee++;
		}
	}
}

void PassageMemory(void)
{
	int i, j, totalmem, totalportals;
	vportal_t *portal, *target;
	leaf_t *leaf;

	totalmem = 0;
	totalportals = 0;
	for (i = 0; i < numportals; i++)
	{
		portal = sorted_portals[i];
		if (portal->removed)
			continue;
		leaf = &leafs[portal->leaf];
		for (j = 0; j < leaf->numportals; j++)
		{
			target = leaf->portals[j];
			if (target->removed)
				continue;
			totalmem += sizeof(passage_t) + portalbytes;
			totalportals++;
		}
	}
	_printf("%7i average number of passages per leaf\n", totalportals / numportals);
	_printf("%7i MB required passage memory\n", totalmem >> 10 >> 10);
}

/*
===============================================================================

This is a rough first-order aproximation that is used to trivially reject some
of the final calculations.


Calculates portalfront and portalflood bit vectors

thinking about:

typedef struct passage_s
{
	struct passage_s	*next;
	struct portal_s		*to;
	stryct sep_s		*seperators;
	byte				*mightsee;
} passage_t;

typedef struct portal_s
{
	struct passage_s	*passages;
	int					leaf;		// leaf portal faces into
} portal_s;

leaf = portal->leaf
clear 
for all portals


calc portal visibility
	clear bit vector
	for all passages
		passage visibility


for a portal to be visible to a passage, it must be on the front of
all seperating planes, and both portals must be behind the new portal

===============================================================================
*/

int		c_flood, c_vis;


/*
==================
SimpleFlood

==================
*/
void SimpleFlood (vportal_t *srcportal, int leafnum)
{
	int		i;
	leaf_t	*leaf;
	vportal_t	*p;
	int		pnum;

	leaf = &leafs[leafnum];
	
	for (i=0 ; i<leaf->numportals ; i++)
	{
		p = leaf->portals[i];
		if (p->removed)
			continue;
		pnum = p - portals;
		if ( ! (srcportal->portalfront[pnum>>3] & (1<<(pnum&7)) ) )
			continue;

		if (srcportal->portalflood[pnum>>3] & (1<<(pnum&7)) )
			continue;

		srcportal->portalflood[pnum>>3] |= (1<<(pnum&7));
		
		SimpleFlood (srcportal, p->leaf);
	}
}

/*
==============
BasePortalVis
==============
*/
void BasePortalVis (int portalnum)
{
	int			j, k;
	vportal_t	*tp, *p;
	float		d;
	winding_t	*w;

	p = portals+portalnum;

	if (p->removed)
		return;

	p->portalfront = malloc (portalbytes);
	memset (p->portalfront, 0, portalbytes);

	p->portalflood = malloc (portalbytes);
	memset (p->portalflood, 0, portalbytes);
	
	p->portalvis = malloc (portalbytes);
	memset (p->portalvis, 0, portalbytes);
	
	for (j=0, tp = portals ; j<numportals*2 ; j++, tp++)
	{
		if (j == portalnum)
			continue;
		if (tp->removed)
			continue;
		/*
		if (farplanedist >= 0)
		{
			vec3_t dir;
			VectorSubtract(p->origin, tp->origin, dir);
			if (VectorLength(dir) > farplanedist - p->radius - tp->radius)
				continue;
		}
		*/
		w = tp->winding;
		for (k=0 ; k<w->numpoints ; k++)
		{
			d = DotProduct (w->points[k], p->plane.normal)
				- p->plane.dist;
			if (d > ON_EPSILON)
				break;
		}
		if (k == w->numpoints)
			continue;	// no points on front

		w = p->winding;
		for (k=0 ; k<w->numpoints ; k++)
		{
			d = DotProduct (w->points[k], tp->plane.normal)
				- tp->plane.dist;
			if (d < -ON_EPSILON)
				break;
		}
		if (k == w->numpoints)
			continue;	// no points on front

		p->portalfront[j>>3] |= (1<<(j&7));
	}
	
	SimpleFlood (p, p->leaf);

	p->nummightsee = CountBits (p->portalflood, numportals*2);
//	_printf ("portal %i: %i mightsee\n", portalnum, p->nummightsee);
	c_flood += p->nummightsee;
}





/*
===============================================================================

This is a second order aproximation 

Calculates portalvis bit vector

WAAAAAAY too slow.

===============================================================================
*/

/*
==================
RecursiveLeafBitFlow

==================
*/
void RecursiveLeafBitFlow (int leafnum, byte *mightsee, byte *cansee)
{
	vportal_t	*p;
	leaf_t 		*leaf;
	int			i, j;
	long		more;
	int			pnum;
	byte		newmight[MAX_PORTALS/8];

	leaf = &leafs[leafnum];
	
	// check all portals for flowing into other leafs
	for (i=0 ; i<leaf->numportals ; i++)
	{
		p = leaf->portals[i];
		if (p->removed)
			continue;
		pnum = p - portals;

		// if some previous portal can't see it, skip
		if (! (mightsee[pnum>>3] & (1<<(pnum&7)) ) )
			continue;

		// if this portal can see some portals we mightsee, recurse
		more = 0;
		for (j=0 ; j<portallongs ; j++)
		{
			((long *)newmight)[j] = ((long *)mightsee)[j] 
				& ((long *)p->portalflood)[j];
			more |= ((long *)newmight)[j] & ~((long *)cansee)[j];
		}

		if (!more)
			continue;	// can't see anything new

		cansee[pnum>>3] |= (1<<(pnum&7));

		RecursiveLeafBitFlow (p->leaf, newmight, cansee);
	}	
}

/*
==============
BetterPortalVis
==============
*/
void BetterPortalVis (int portalnum)
{
	vportal_t	*p;

	p = portals+portalnum;

	if (p->removed)
		return;

	RecursiveLeafBitFlow (p->leaf, p->portalflood, p->portalvis);

	// build leaf vis information
	p->nummightsee = CountBits (p->portalvis, numportals*2);
	c_vis += p->nummightsee;
}


