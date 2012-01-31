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

/*
==============================================================================

PORTAL FILE GENERATION

Save out name.prt for qvis to read
==============================================================================
*/


#define	PORTALFILE	"PRT1"

FILE	*pf;
int		num_visclusters;				// clusters the player can be in
int		num_visportals;
int		num_solidfaces;

void WriteFloat (FILE *f, vec_t v)
{
	if ( fabs(v - Q_rint(v)) < 0.001 )
		fprintf (f,"%i ",(int)Q_rint(v));
	else
		fprintf (f,"%f ",v);
}

/*
=================
WritePortalFile_r
=================
*/
void WritePortalFile_r (node_t *node)
{
	int			i, s;	
	portal_t	*p;
	winding_t	*w;
	vec3_t		normal;
	vec_t		dist;

	// decision node
	if (node->planenum != PLANENUM_LEAF) {
		WritePortalFile_r (node->children[0]);
		WritePortalFile_r (node->children[1]);
		return;
	}
	
	if (node->opaque) {
		return;
	}

	for (p = node->portals ; p ; p=p->next[s])
	{
		w = p->winding;
		s = (p->nodes[1] == node);
		if (w && p->nodes[0] == node)
		{
			if (!Portal_Passable(p))
				continue;
			// write out to the file
			
			// sometimes planes get turned around when they are very near
			// the changeover point between different axis.  interpret the
			// plane the same way vis will, and flip the side orders if needed
			// FIXME: is this still relevent?
			WindingPlane (w, normal, &dist);
			if ( DotProduct (p->plane.normal, normal) < 0.99 )
			{	// backwards...
				fprintf (pf,"%i %i %i ",w->numpoints, p->nodes[1]->cluster, p->nodes[0]->cluster);
			}
			else
				fprintf (pf,"%i %i %i ",w->numpoints, p->nodes[0]->cluster, p->nodes[1]->cluster);
			if (p->hint)
				fprintf (pf, "1 ");
			else
				fprintf (pf, "0 ");
			for (i=0 ; i<w->numpoints ; i++)
			{
				fprintf (pf,"(");
				WriteFloat (pf, w->p[i][0]);
				WriteFloat (pf, w->p[i][1]);
				WriteFloat (pf, w->p[i][2]);
				fprintf (pf,") ");
			}
			fprintf (pf,"\n");
		}
	}

}

/*
=================
WriteFaceFile_r
=================
*/
void WriteFaceFile_r (node_t *node)
{
	int			i, s;	
	portal_t	*p;
	winding_t	*w;

	// decision node
	if (node->planenum != PLANENUM_LEAF) {
		WriteFaceFile_r (node->children[0]);
		WriteFaceFile_r (node->children[1]);
		return;
	}
	
	if (node->opaque) {
		return;
	}

	for (p = node->portals ; p ; p=p->next[s])
	{
		w = p->winding;
		s = (p->nodes[1] == node);
		if (w)
		{
			if (Portal_Passable(p))
				continue;
			// write out to the file

			if (p->nodes[0] == node)
			{
				fprintf (pf,"%i %i ",w->numpoints, p->nodes[0]->cluster);
				for (i=0 ; i<w->numpoints ; i++)
				{
					fprintf (pf,"(");
					WriteFloat (pf, w->p[i][0]);
					WriteFloat (pf, w->p[i][1]);
					WriteFloat (pf, w->p[i][2]);
					fprintf (pf,") ");
				}
				fprintf (pf,"\n");
			}
			else
			{
				fprintf (pf,"%i %i ",w->numpoints, p->nodes[1]->cluster);
				for (i = w->numpoints-1; i >= 0; i--)
				{
					fprintf (pf,"(");
					WriteFloat (pf, w->p[i][0]);
					WriteFloat (pf, w->p[i][1]);
					WriteFloat (pf, w->p[i][2]);
					fprintf (pf,") ");
				}
				fprintf (pf,"\n");
			}
		}
	}
}

/*
================
NumberLeafs_r
================
*/
void NumberLeafs_r (node_t *node)
{
	portal_t	*p;

	if ( node->planenum != PLANENUM_LEAF ) {
		// decision node
		node->cluster = -99;
		NumberLeafs_r (node->children[0]);
		NumberLeafs_r (node->children[1]);
		return;
	}
	
	node->area = -1;

	if ( node->opaque ) {
		// solid block, viewpoint never inside
		node->cluster = -1;
		return;
	}

	node->cluster = num_visclusters;
	num_visclusters++;

	// count the portals
	for (p = node->portals ; p ; )
	{
		if (p->nodes[0] == node)		// only write out from first leaf
		{
			if (Portal_Passable(p))
				num_visportals++;
			else
				num_solidfaces++;
			p = p->next[0];
		}
		else
		{
			if (!Portal_Passable(p))
				num_solidfaces++;
			p = p->next[1];		
		}
	}
}


/*
================
NumberClusters
================
*/
void NumberClusters(tree_t *tree) {
	num_visclusters = 0;
	num_visportals = 0;
	num_solidfaces = 0;

	qprintf ("--- NumberClusters ---\n");
	
	// set the cluster field in every leaf and count the total number of portals
	NumberLeafs_r (tree->headnode);

	qprintf ("%5i visclusters\n", num_visclusters);
	qprintf ("%5i visportals\n", num_visportals);
	qprintf ("%5i solidfaces\n", num_solidfaces);
}

/*
================
WritePortalFile
================
*/
void WritePortalFile (tree_t *tree)
{
	char	filename[1024];

	qprintf ("--- WritePortalFile ---\n");
	
	// write the file
	sprintf (filename, "%s.prt", source);
	_printf ("writing %s\n", filename);
	pf = fopen (filename, "w");
	if (!pf)
		Error ("Error opening %s", filename);
		
	fprintf (pf, "%s\n", PORTALFILE);
	fprintf (pf, "%i\n", num_visclusters);
	fprintf (pf, "%i\n", num_visportals);
	fprintf (pf, "%i\n", num_solidfaces);

	WritePortalFile_r(tree->headnode);
	WriteFaceFile_r(tree->headnode);

	fclose (pf);
}

