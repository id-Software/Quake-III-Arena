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

// window system independent camera view code

typedef enum
{
	cd_wire,
	cd_solid,
	cd_texture,
  cd_light,
	cd_blend
} camera_draw_mode;

typedef struct
{
	int		width, height;

	qboolean	timing;

	vec3_t	origin;
	vec3_t	angles;

	camera_draw_mode	draw_mode;

	vec3_t	color;			// background 

	vec3_t	forward, right, up;	// move matrix

	vec3_t	vup, vpn, vright;	// view matrix
} camera_t;

