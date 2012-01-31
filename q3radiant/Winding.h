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



//returns true if the planes are equal
int			Plane_Equal(plane_t *a, plane_t *b, int flip);
//returns false if the points are colinear
int			Plane_FromPoints(vec3_t p1, vec3_t p2, vec3_t p3, plane_t *plane);
//returns true if the points are equal
int			Point_Equal(vec3_t p1, vec3_t p2, float epsilon);

//allocate a winding
winding_t*	Winding_Alloc(int points);
//free the winding
void		Winding_Free(winding_t *w);
//create a base winding for the plane
winding_t*	Winding_BaseForPlane (plane_t *p);
//make a winding clone
winding_t*	Winding_Clone(winding_t *w );
//creates the reversed winding
winding_t*	Winding_Reverse(winding_t *w);
//remove a point from the winding
void		Winding_RemovePoint(winding_t *w, int point);
//inserts a point to a winding, creating a new winding
winding_t*	Winding_InsertPoint(winding_t *w, vec3_t point, int spot);
//returns true if the planes are concave
int			Winding_PlanesConcave(winding_t *w1, winding_t *w2,
									 vec3_t normal1, vec3_t normal2,
									 float dist1, float dist2);
//returns true if the winding is tiny
int			Winding_IsTiny(winding_t *w);
//returns true if the winding is huge
int			Winding_IsHuge(winding_t *w);
//clip the winding with the plane
winding_t*	Winding_Clip(winding_t *in, plane_t *split, qboolean keepon);
//split the winding with the plane
void		Winding_SplitEpsilon(winding_t *in, vec3_t normal, double dist, 
								vec_t epsilon, winding_t **front, winding_t **back);
//try to merge the windings, returns the new merged winding or NULL
winding_t *Winding_TryMerge(winding_t *f1, winding_t *f2, vec3_t planenormal, int keep);
//create a plane for the winding
void		Winding_Plane(winding_t *w, vec3_t normal, double *dist);
//returns the winding area
float		Winding_Area(winding_t *w);
//returns the bounds of the winding
void		Winding_Bounds(winding_t *w, vec3_t mins, vec3_t maxs);
//returns true if the point is inside the winding
int			Winding_PointInside(winding_t *w, plane_t *plane, vec3_t point, float epsilon);
//returns true if the vector intersects with the winding
int			Winding_VectorIntersect(winding_t *w, plane_t *plane, vec3_t p1, vec3_t p2, float epsilon);
