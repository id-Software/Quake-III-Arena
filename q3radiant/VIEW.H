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


typedef struct
{
	int		x, y;
	int		w, h;

	int		redraws;	// clear to 0 when need update

	vec3_t	color;

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	forward, right, up;

	int		draw_filtered;
	int		draw_wire;
	int		draw_solid;
	int		draw_textured;
	int		draw_blend;
} vieworg_t;

extern	vieworg_t	*view_org;
extern	int			keysdown;

void View_Init (void);
void View_Draw (void);
void View_MouseDown (int x, int y, int buttons);
void View_KeyUp (int key);
void View_KeyDown (int key);
void View_Reshape(int w, int h);

