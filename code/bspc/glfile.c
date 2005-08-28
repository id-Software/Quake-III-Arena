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

#include "qbsp.h"

int		c_glfaces;

int PortalVisibleSides (portal_t *p)
{
	int		fcon, bcon;

	if (!p->onnode)
		return 0;		// outside

	fcon = p->nodes[0]->contents;
	bcon = p->nodes[1]->contents;

	// same contents never create a face
	if (fcon == bcon)
		return 0;

	// FIXME: is this correct now?
	if (!fcon)
		return 1;
	if (!bcon)
		return 2;
	return 0;
}

void OutputWinding (winding_t *w, FILE *glview)
{
	static	int	level = 128;
	vec_t		light;
	int			i;

	fprintf (glview, "%i\n", w->numpoints);
	level+=28;
	light = (level&255)/255.0;
	for (i=0 ; i<w->numpoints ; i++)
	{
		fprintf (glview, "%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
			w->p[i][0],
			w->p[i][1],
			w->p[i][2],
			light,
			light,
			light);
	}
	fprintf (glview, "\n");
}

/*
=============
OutputPortal
=============
*/
void OutputPortal (portal_t *p, FILE *glview)
{
	winding_t	*w;
	int		sides;

	sides = PortalVisibleSides (p);
	if (!sides)
		return;

	c_glfaces++;

	w = p->winding;

	if (sides == 2)		// back side
		w = ReverseWinding (w);

	OutputWinding (w, glview);

	if (sides == 2)
		FreeWinding(w);
}

/*
=============
WriteGLView_r
=============
*/
void WriteGLView_r (node_t *node, FILE *glview)
{
	portal_t	*p, *nextp;

	if (node->planenum != PLANENUM_LEAF)
	{
		WriteGLView_r (node->children[0], glview);
		WriteGLView_r (node->children[1], glview);
		return;
	}

	// write all the portals
	for (p=node->portals ; p ; p=nextp)
	{
		if (p->nodes[0] == node)
		{
			OutputPortal (p, glview);
			nextp = p->next[0];
		}
		else
			nextp = p->next[1];
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLView (tree_t *tree, char *source)
{
	char	name[1024];
	FILE	*glview;

	c_glfaces = 0;
	sprintf (name, "%s%s.gl",outbase, source);
	printf ("Writing %s\n", name);

	glview = fopen (name, "w");
	if (!glview)
		Error ("Couldn't open %s", name);
	WriteGLView_r (tree->headnode, glview);
	fclose (glview);

	printf ("%5i c_glfaces\n", c_glfaces);
}

