/*
===========================================================================
Copyright (C) 2010 James Canete (use.less01@gmail.com)

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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_extramath.h

#ifndef __TR_EXTRAMATH_H__
#define __TR_EXTRAMATH_H__

typedef vec_t matrix_t[16];
typedef int vec2i_t[2];
typedef int vec3i_t[3];
typedef int vec4i_t[4];

void Matrix16Zero( matrix_t out );
void Matrix16Identity( matrix_t out );
void Matrix16Copy( const matrix_t in, matrix_t out );
void Matrix16Multiply( const matrix_t in1, const matrix_t in2, matrix_t out );
void Matrix16Transform( const matrix_t in1, const vec4_t in2, vec4_t out );
qboolean Matrix16Compare(const matrix_t a, const matrix_t b);
void Matrix16Dump( const matrix_t in );
void Matrix16Translation( vec3_t vec, matrix_t out );
void Matrix16Ortho( float left, float right, float bottom, float top, float znear, float zfar, matrix_t out );
void Matrix16View(vec3_t axes[3], vec3_t origin, matrix_t out);
void Matrix16SimpleInverse( const matrix_t in, matrix_t out);

#define VectorCopy2(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1])

#define VectorCopy4(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define VectorSet4(v,x,y,z,w)	((v)[0]=(x),(v)[1]=(y),(v)[2]=(z),(v)[3]=(w))
#define DotProduct4(a,b)        ((a)[0]*(b)[0] + (a)[1]*(b)[1] + (a)[2]*(b)[2] + (a)[3]*(b)[3])
#define VectorScale4(a,b,c)     ((c)[0]=(a)[0]*(b),(c)[1]=(a)[1]*(b),(c)[2]=(a)[2]*(b),(c)[3]=(a)[3]*(b))

#define VectorCopy5(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3],(b)[4]=(a)[4])

#define OffsetByteToFloat(a)    ((float)(a) * 1.0f/127.5f - 1.0f)
#define FloatToOffsetByte(a)    (byte)(((a) + 1.0f) * 127.5f)
#define ByteToFloat(a)          ((float)(a) * 1.0f/255.0f)
#define FloatToByte(a)          (byte)((a) * 255.0f)

#define RGBtosRGB(a)            (((a) < 0.0031308f) ? (12.92f * (a)) : (1.055f * pow((a), 0.41666f) - 0.055f))
#define sRGBtoRGB(a)            (((a) <= 0.04045f)  ? ((a) / 12.92f) : (pow((((a) + 0.055f) / 1.055f), 2.4)) )

static ID_INLINE int VectorCompare4(const vec4_t v1, const vec4_t v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3])
	{
		return 0;
	}
	return 1;
}

static ID_INLINE int VectorCompare5(const vec5_t v1, const vec5_t v2)
{
	if(v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2] || v1[3] != v2[3] || v1[4] != v2[4])
	{
		return 0;
	}
	return 1;
}

void VectorLerp( vec3_t a, vec3_t b, float lerp, vec3_t c);


qboolean SpheresIntersect(vec3_t origin1, float radius1, vec3_t origin2, float radius2);
void BoundingSphereOfSpheres(vec3_t origin1, float radius1, vec3_t origin2, float radius2, vec3_t origin3, float *radius3);

#ifndef SGN
#define SGN(x) (((x) >= 0) ? !!(x) : -1)
#endif

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef CLAMP
#define CLAMP(a,b,c) MIN(MAX((a),(b)),(c))
#endif

int NextPowerOfTwo(int in);
unsigned short FloatToHalf(float in);

#endif
