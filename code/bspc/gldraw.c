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

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glaux.h>

#include "qbsp.h"

// can't use the glvertex3fv functions, because the vec3_t fields
// could be either floats or doubles, depending on DOUBLEVEC_T

qboolean	drawflag;
vec3_t	draw_mins, draw_maxs;


#define	WIN_SIZE	512

void InitWindow (void)
{
    auxInitDisplayMode (AUX_SINGLE | AUX_RGB);
    auxInitPosition (0, 0, WIN_SIZE, WIN_SIZE);
    auxInitWindow ("qcsg");
}

void Draw_ClearWindow (void)
{
	static int	init;
	int		w, h, g;
	vec_t	mx, my;

	if (!drawflag)
		return;

	if (!init)
	{
		init = true;
		InitWindow ();
	}

	glClearColor (1,0.8,0.8,0);
	glClear (GL_COLOR_BUFFER_BIT);

	w = (draw_maxs[0] - draw_mins[0]);
	h = (draw_maxs[1] - draw_mins[1]);

	mx = draw_mins[0] + w/2;
	my = draw_mins[1] + h/2;

	g = w > h ? w : h;

	glLoadIdentity ();
    gluPerspective (90,  1,  2,  16384);
	gluLookAt (mx, my, draw_maxs[2] + g/2, mx , my, draw_maxs[2], 0, 1, 0);

	glColor3f (0,0,0);
//	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glDisable (GL_DEPTH_TEST);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#if 0
	glColor4f (1,0,0,0.5);
	glBegin (GL_POLYGON);

	glVertex3f (0, 500, 0);
	glVertex3f (0, 900, 0);
	glVertex3f (0, 900, 100);
	glVertex3f (0, 500, 100);

	glEnd ();
#endif

	glFlush ();

}

void Draw_SetRed (void)
{
	if (!drawflag)
		return;

	glColor3f (1,0,0);
}

void Draw_SetGrey (void)
{
	if (!drawflag)
		return;

	glColor3f (0.5,0.5,0.5);
}

void Draw_SetBlack (void)
{
	if (!drawflag)
		return;

	glColor3f (0,0,0);
}

void DrawWinding (winding_t *w)
{
	int		i;

	if (!drawflag)
		return;

	glColor4f (0,0,0,0.5);
	glBegin (GL_LINE_LOOP);
	for (i=0 ; i<w->numpoints ; i++)
		glVertex3f (w->p[i][0],w->p[i][1],w->p[i][2] );
	glEnd ();

	glColor4f (0,1,0,0.3);
	glBegin (GL_POLYGON);
	for (i=0 ; i<w->numpoints ; i++)
		glVertex3f (w->p[i][0],w->p[i][1],w->p[i][2] );
	glEnd ();

	glFlush ();
}

void DrawAuxWinding (winding_t *w)
{
	int		i;

	if (!drawflag)
		return;

	glColor4f (0,0,0,0.5);
	glBegin (GL_LINE_LOOP);
	for (i=0 ; i<w->numpoints ; i++)
		glVertex3f (w->p[i][0],w->p[i][1],w->p[i][2] );
	glEnd ();

	glColor4f (1,0,0,0.3);
	glBegin (GL_POLYGON);
	for (i=0 ; i<w->numpoints ; i++)
		glVertex3f (w->p[i][0],w->p[i][1],w->p[i][2] );
	glEnd ();

	glFlush ();
}

//============================================================

#define	GLSERV_PORT	25001

qboolean	wins_init;
int			draw_socket;

void GLS_BeginScene (void)
{
	WSADATA	winsockdata;
	WORD	wVersionRequested; 
	struct sockaddr_in	address;
	int		r;

	if (!wins_init)
	{
		wins_init = true;

		wVersionRequested = MAKEWORD(1, 1); 

		r = WSAStartup (MAKEWORD(1, 1), &winsockdata);

		if (r)
			Error ("Winsock initialization failed.");

	}

	// connect a socket to the server

	draw_socket = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (draw_socket == -1)
		Error ("draw_socket failed");

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = GLSERV_PORT;
	r = connect (draw_socket, (struct sockaddr *)&address, sizeof(address));
	if (r == -1)
	{
		closesocket (draw_socket);
		draw_socket = 0;
	}
}

void GLS_Winding (winding_t *w, int code)
{
	byte	buf[1024];
	int		i, j;

	if (!draw_socket)
		return;

	((int *)buf)[0] = w->numpoints;
	((int *)buf)[1] = code;
	for (i=0 ; i<w->numpoints ; i++)
		for (j=0 ; j<3 ; j++)
			((float *)buf)[2+i*3+j] = w->p[i][j];

	send (draw_socket, buf, w->numpoints*12+8, 0);
}

void GLS_EndScene (void)
{
	closesocket (draw_socket);
	draw_socket = 0;
}
