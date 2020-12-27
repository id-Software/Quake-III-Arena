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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "../renderercommon/tr_common.h"


qboolean ( * qwglSwapIntervalEXT)( int interval );
void ( * qglMultiTexCoord2fARB )( GLenum texture, float s, float t );
void ( * qglActiveTextureARB )( GLenum texture );
void ( * qglClientActiveTextureARB )( GLenum texture );


void ( * qglLockArraysEXT)( int, int);
void ( * qglUnlockArraysEXT) ( void );


void		GLimp_EndFrame( void ) {
}

void 		GLimp_Init( void ) {
}

void		GLimp_Shutdown( void ) {
}

void		GLimp_EnableLogging( qboolean enable ) {
}

void		GLimp_LogComment( char *comment ) {
}

qboolean	QGL_Init( const char *dllname ) {
	return qtrue;
}

void		QGL_Shutdown( void ) {
}

void		GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] ) {
}

void		GLimp_Minimize( void ) {
}
