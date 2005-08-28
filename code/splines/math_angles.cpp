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
#include "q_shared.h"
#include <float.h>

angles_t ang_zero( 0.0f, 0.0f, 0.0f );

void toAngles( mat3_t &src, angles_t &dst ) {
	double		theta;
	double		cp;
	double		sp;

	sp = src[ 0 ][ 2 ];

	// cap off our sin value so that we don't get any NANs
	if ( sp > 1.0 ) {
		sp = 1.0;
	} else if ( sp < -1.0 ) {
		sp = -1.0;
	}

	theta = -asin( sp );
	cp = cos( theta );

	if ( cp > 8192 * FLT_EPSILON ) {
		dst.pitch	= theta * 180 / M_PI;
		dst.yaw		= atan2( src[ 0 ][ 1 ], src[ 0 ][ 0 ] ) * 180 / M_PI;
		dst.roll	= atan2( src[ 1 ][ 2 ], src[ 2 ][ 2 ] ) * 180 / M_PI;
	} else {
		dst.pitch	= theta * 180 / M_PI;
		dst.yaw		= -atan2( src[ 1 ][ 0 ], src[ 1 ][ 1 ] ) * 180 / M_PI;
		dst.roll	= 0;
	}
}

void toAngles( quat_t &src, angles_t &dst ) {
	mat3_t temp;

	toMatrix( src, temp );
	toAngles( temp, dst );
}

void toAngles( idVec3_t &src, angles_t &dst ) {
	dst.pitch	= src[ 0 ];
	dst.yaw		= src[ 1 ];
	dst.roll	= src[ 2 ];
}

void angles_t::toVectors( idVec3_t *forward, idVec3_t *right, idVec3_t *up ) {
	float			angle;
	static float	sr, sp, sy, cr, cp, cy; // static to help MS compiler fp bugs
	
	angle = yaw * ( M_PI * 2 / 360 );
	sy = sin( angle );
	cy = cos( angle );

	angle = pitch * ( M_PI * 2 / 360 );
	sp = sin( angle );
	cp = cos( angle );

	angle = roll * ( M_PI * 2 / 360 );
	sr = sin( angle );
	cr = cos( angle );

	if ( forward ) {
		forward->set( cp * cy, cp * sy, -sp );
	}

	if ( right ) {
		right->set( -sr * sp * cy + cr * sy, -sr * sp * sy + -cr * cy, -sr * cp );
	}

	if ( up ) {
		up->set( cr * sp * cy + -sr * -sy, cr * sp * sy + -sr * cy, cr * cp );
	}
}

idVec3_t angles_t::toForward( void ) {
	float			angle;
	static float	sp, sy, cp, cy; // static to help MS compiler fp bugs
	
	angle = yaw * ( M_PI * 2 / 360 );
	sy = sin( angle );
	cy = cos( angle );

	angle = pitch * ( M_PI * 2 / 360 );
	sp = sin( angle );
	cp = cos( angle );

	return idVec3_t( cp * cy, cp * sy, -sp );
}

/*
=================
Normalize360

returns angles normalized to the range [0 <= angle < 360]
=================
*/
angles_t& angles_t::Normalize360( void ) {
	pitch	= (360.0 / 65536) * ( ( int )( pitch	* ( 65536 / 360.0 ) ) & 65535 );
	yaw		= (360.0 / 65536) * ( ( int )( yaw		* ( 65536 / 360.0 ) ) & 65535 );
	roll	= (360.0 / 65536) * ( ( int )( roll		* ( 65536 / 360.0 ) ) & 65535 );

	return *this;
}


/*
=================
Normalize180

returns angles normalized to the range [-180 < angle <= 180]
=================
*/
angles_t& angles_t::Normalize180( void ) {
	Normalize360();

	if ( pitch > 180.0 ) {
		pitch -= 360.0;
	}
	
	if ( yaw > 180.0 ) {
		yaw  -= 360.0;
	}

	if ( roll > 180.0 ) {
		roll -= 360.0;
	}
	return *this;
}
