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
#ifndef __MATHLIB__
#define __MATHLIB__

// mathlib.h

#include <math.h>

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

#define	SIDE_FRONT		0
#define	SIDE_ON			2
#define	SIDE_BACK		1
#define	SIDE_CROSS		-2

#define	Q_PI	3.14159265358979323846

extern vec3_t vec3_origin;

#define	EQUAL_EPSILON	0.001

qboolean VectorCompare (vec3_t v1, vec3_t v2);

#define DotProduct(x,y) (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c) {c[0]=a[0]-b[0];c[1]=a[1]-b[1];c[2]=a[2]-b[2];}
#define VectorAdd(a,b,c) {c[0]=a[0]+b[0];c[1]=a[1]+b[1];c[2]=a[2]+b[2];}
#define VectorCopy(a,b) {b[0]=a[0];b[1]=a[1];b[2]=a[2];}
#define VectorSet(v, a, b, c) {v[0]=a;v[1]=b;v[2]=c;}

vec_t Q_rint (vec_t in);
vec_t _DotProduct (vec3_t v1, vec3_t v2);
void _VectorSubtract (vec3_t va, vec3_t vb, vec3_t out);
void _VectorAdd (vec3_t va, vec3_t vb, vec3_t out);
void _VectorCopy (vec3_t in, vec3_t out);

float VectorLength(vec3_t v);

void VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc);

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross);
vec_t VectorNormalize (vec3_t v);
void VectorInverse (vec3_t v);
void VectorScale (vec3_t v, vec_t scale, vec3_t out);
void VectorPolar(vec3_t v, float radius, float theta, float phi);
void VectorSnap(vec3_t v);

void _Vector53Copy (vec5_t in, vec3_t out);
void _Vector5Scale (vec5_t v, vec_t scale, vec5_t out);
void _Vector5Add (vec5_t va, vec5_t vb, vec5_t out);

// NOTE: added these from Ritual's Q3Radiant
void ClearBounds (vec3_t mins, vec3_t maxs);
void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs);

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void VectorToAngles( vec3_t vec, vec3_t angles );
#define VectorClear(x) {x[0] = x[1] = x[2] = 0;}

#define ZERO_EPSILON 1.0E-6
#define RAD2DEG( a ) ( ( (a) * 180.0f ) / Q_PI )
#define DEG2RAD( a ) ( ( (a) * Q_PI ) / 180.0f )


#endif
