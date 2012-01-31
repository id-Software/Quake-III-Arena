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
//#include "../game/q_shared.h"
#include "math_vector.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h

#define LERP_DELTA 1e-6

idVec3_t vec_zero( 0.0f, 0.0f, 0.0f );

Bounds	boundsZero;

float idVec3_t::toYaw( void ) {
	float yaw;
	
	if ( ( y == 0 ) && ( x == 0 ) ) {
		yaw = 0;
	} else {
		yaw = atan2( y, x ) * 180 / M_PI;
		if ( yaw < 0 ) {
			yaw += 360;
		}
	}

	return yaw;
}

float idVec3_t::toPitch( void ) {
	float	forward;
	float	pitch;
	
	if ( ( x == 0 ) && ( y == 0 ) ) {
		if ( z > 0 ) {
			pitch = 90;
		} else {
			pitch = 270;
		}
	} else {
		forward = ( float )idSqrt( x * x + y * y );
		pitch = atan2( z, forward ) * 180 / M_PI;
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	return pitch;
}

/*
angles_t idVec3_t::toAngles( void ) {
	float forward;
	float yaw;
	float pitch;
	
	if ( ( x == 0 ) && ( y == 0 ) ) {
		yaw = 0;
		if ( z > 0 ) {
			pitch = 90;
		} else {
			pitch = 270;
		}
	} else {
		yaw = atan2( y, x ) * 180 / M_PI;
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = ( float )idSqrt( x * x + y * y );
		pitch = atan2( z, forward ) * 180 / M_PI;
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	return angles_t( -pitch, yaw, 0 );
}
*/

idVec3_t LerpVector( idVec3_t &w1, idVec3_t &w2, const float t ) {
	float omega, cosom, sinom, scale0, scale1;

	cosom = w1 * w2;
	if ( ( 1.0 - cosom ) > LERP_DELTA ) {
		omega = acos( cosom );
		sinom = sin( omega );
		scale0 = sin( ( 1.0 - t ) * omega ) / sinom;
		scale1 = sin( t * omega ) / sinom;
	} else {
		scale0 = 1.0 - t;
		scale1 = t;
	}

	return ( w1 * scale0 + w2 * scale1 );
}

/*
=============
idVec3_t::string

This is just a convenience function
for printing vectors
=============
*/
char *idVec3_t::string( void ) {
	static	int		index = 0;
	static	char	str[ 8 ][ 36 ];
	char	*s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = (index + 1)&7;

	sprintf( s, "%.2f %.2f %.2f", x, y, z );

	return s;
}
