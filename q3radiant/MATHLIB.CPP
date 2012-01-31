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
// mathlib.c -- math primitives

#include "stdafx.h"
#include "cmdlib.h"
#include "mathlib.h"

vec3_t vec3_origin = {0.0f,0.0f,0.0f};


float VectorLength(vec3_t v)
{
	int		i;
	float	length;
	
	length = 0.0f;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = (float)sqrt (length);

	return length;
}

qboolean VectorCompare (vec3_t v1, vec3_t v2)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
		if (fabs(v1[i]-v2[i]) > EQUAL_EPSILON)
			return false;
			
	return true;
}

vec_t Q_rint (vec_t in)
{
  if (g_PrefsDlg.m_bNoClamp)
    return in;
  else
	  return (float)floor (in + 0.5);
}

void VectorMA (vec3_t va, float scale, vec3_t vb, vec3_t vc)
{
	vc[0] = va[0] + scale*vb[0];
	vc[1] = va[1] + scale*vb[1];
	vc[2] = va[2] + scale*vb[2];
}

void CrossProduct (vec3_t v1, vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]-vb[0];
	out[1] = va[1]-vb[1];
	out[2] = va[2]-vb[2];
}

void _VectorAdd (vec3_t va, vec3_t vb, vec3_t out)
{
	out[0] = va[0]+vb[0];
	out[1] = va[1]+vb[1];
	out[2] = va[2]+vb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

vec_t VectorNormalize (vec3_t v)
{
	int		i;
	float	length;
	
	length = 0.0f;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = (float)sqrt (length);
	if (length == 0)
		return (vec_t)0;
		
	for (i=0 ; i< 3 ; i++)
		v[i] /= length;	

	return length;
}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (vec3_t v, vec_t scale, vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}


void VectorRotate (vec3_t vIn, vec3_t vRotation, vec3_t out)
{
  vec3_t vWork, va;
  VectorCopy(vIn, va);
  VectorCopy(va, vWork);
  int nIndex[3][2];
  nIndex[0][0] = 1; nIndex[0][1] = 2;
  nIndex[1][0] = 2; nIndex[1][1] = 0;
  nIndex[2][0] = 0; nIndex[2][1] = 1;

  for (int i = 0; i < 3; i++)
  {
    if (vRotation[i] != 0)
    {
      double dAngle = vRotation[i] * Q_PI / 180.0;
	  double c = cos(dAngle);
      double s = sin(dAngle);
      vWork[nIndex[i][0]] = va[nIndex[i][0]] * c - va[nIndex[i][1]] * s;
      vWork[nIndex[i][1]] = va[nIndex[i][0]] * s + va[nIndex[i][1]] * c;
    }
    VectorCopy(vWork, va);
  }
  VectorCopy(vWork, out);
}

void VectorRotate (vec3_t vIn, vec3_t vRotation, vec3_t vOrigin, vec3_t out)
{
  vec3_t vTemp, vTemp2;
  VectorSubtract(vIn, vOrigin, vTemp);
  VectorRotate(vTemp, vRotation, vTemp2);
  VectorAdd(vTemp2, vOrigin, out);
}

void VectorPolar(vec3_t v, float radius, float theta, float phi)
{
 	v[0]=float(radius * cos(theta) * cos(phi));
	v[1]=float(radius * sin(theta) * cos(phi));
	v[2]=float(radius * sin(phi));
}

void VectorSnap(vec3_t v)
{
  for (int i = 0; i < 3; i++)
  {
    v[i] = floor (v[i] + 0.5);
  }
}


void _Vector5Add (vec5_t va, vec5_t vb, vec5_t out)
{
	out[0] = va[0]+vb[0];
	out[1] = va[1]+vb[1];
	out[2] = va[2]+vb[2];
	out[3] = va[3]+vb[3];
	out[4] = va[4]+vb[4];
}

void _Vector5Scale (vec5_t v, vec_t scale, vec5_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
	out[3] = v[3] * scale;
	out[4] = v[4] * scale;
}

void _Vector53Copy (vec5_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

// NOTE: added these from Ritual's Q3Radiant
void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;
	
	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over
#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

void AngleVectors (vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs
	
	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);
	
	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = -sr*sp*cy+cr*sy;
		right[1] = -sr*sp*sy-cr*cy;
		right[2] = -sr*cp;
	}
	if (up)
	{
		up[0] = cr*sp*cy+sr*sy;
		up[1] = cr*sp*sy-sr*cy;
		up[2] = cr*cp;
	}
}

void VectorToAngles( vec3_t vec, vec3_t angles )
{
	float forward;
	float yaw, pitch;
	
	if ( ( vec[ 0 ] == 0 ) && ( vec[ 1 ] == 0 ) )
	{
		yaw = 0;
		if ( vec[ 2 ] > 0 )
		{
			pitch = 90;
		}
		else
		{
			pitch = 270;
		}
	}
	else
	{
		yaw = atan2( vec[ 1 ], vec[ 0 ] ) * 180 / M_PI;
		if ( yaw < 0 )
		{
			yaw += 360;
		}
		
		forward = ( float )sqrt( vec[ 0 ] * vec[ 0 ] + vec[ 1 ] * vec[ 1 ] );
		pitch = atan2( vec[ 2 ], forward ) * 180 / M_PI;
		if ( pitch < 0 )
		{
			pitch += 360;
		}
	}
	
	angles[ 0 ] = pitch;
	angles[ 1 ] = yaw;
	angles[ 2 ] = 0;
}
