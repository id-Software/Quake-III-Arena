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

void WriteFloat2 (FILE *f, vec_t v)
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
	if (node->planenum != PLANENUM_LEAF && !node->detail_seperator)
	{
		WritePortalFile_r (node->children[0]);
		WritePortalFile_r (node->children[1]);
		return;
	}
	
	if (node->contents & CONTENTS_SOLID)
		return;

	for (p = node->portals ; p ; p=p->next[s])
	{
		w = p->winding;
		s = (p->nodes[1] == node);
		if (w && p->nodes[0] == node)
		{
			if (!Portal_VisFlood (p))
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
			for (i=0 ; i<w->numpoints ; i++)
			{
				fprintf (pf,"(");
				WriteFloat2 (pf, w->p[i][0]);
				WriteFloat2 (pf, w->p[i][1]);
				WriteFloat2 (pf, w->p[i][2]);
				fprintf (pf,") ");
			}
			fprintf (pf,"\n");
		}
	}

}

/*
================
FillLeafNumbers_r

All of the leafs under node will have the same cluster
================
*/
void FillLeafNumbers_r (node_t *node, int num)
{
	if (node->planenum == PLANENUM_LEAF)
	{
		if (node->contents & CONTENTS_SOLID)
			node->cluster = -1;
		else
			node->cluster = num;
		return;
	}
	node->cluster = num;
	FillLeafNumbers_r (node->children[0], num);
	FillLeafNumbers_r (node->children[1], num);
}

/*
================
NumberLeafs_r
================
*/
void NumberLeafs_r (node_t *node)
{
	portal_t	*p;

	if (node->planenum != PLANENUM_LEAF && !node->detail_seperator)
	{	// decision node
		node->cluster = -99;
		NumberLeafs_r (node->children[0]);
		NumberLeafs_r (node->children[1]);
		return;
	}
	
	// either a leaf or a detail cluster

	if ( node->contents & CONTENTS_SOLID )
	{	// solid block, viewpoint never inside
		node->cluster = -1;
		return;
	}

	FillLeafNumbers_r (node, num_visclusters);
	num_visclusters++;

	// count the portals
	for (p = node->portals ; p ; )
	{
		if (p->nodes[0] == node)		// only write out from first leaf
		{
			if (Portal_VisFlood (p))
				num_visportals++;
			p = p->next[0];
		}
		else
			p = p->next[1];		
	}

}


/*
================
CreateVisPortals_r
================
*/
void CreateVisPortals_r (node_t *node)
{
	// stop as soon as we get to a detail_seperator, which
	// means that everything below is in a single cluster
	if (node->planenum == PLANENUM_LEAF || node->detail_seperator )
		return;

	MakeNodePortal (node);
	SplitNodePortals (node);

	CreateVisPortals_r (node->children[0]);
	CreateVisPortals_r (node->children[1]);
}

/*
================
FinishVisPortals_r
================
*/
void FinishVisPortals2_r (node_t *node)
{
	if (node->planenum == PLANENUM_LEAF)
		return;

	MakeNodePortal (node);
	SplitNodePortals (node);

	FinishVisPortals2_r (node->children[0]);
	FinishVisPortals2_r (node->children[1]);
}

void FinishVisPortals_r (node_t *node)
{
	if (node->planenum == PLANENUM_LEAF)
		return;

	if (node->detail_seperator)
	{
		FinishVisPortals2_r (node);
		return;
	}

	FinishVisPortals_r (node->children[0]);
	FinishVisPortals_r (node->children[1]);
}


int		clusterleaf;
void SaveClusters_r (node_t *node)
{
	if (node->planenum == PLANENUM_LEAF)
	{
		dleafs[clusterleaf++].cluster = node->cluster;
		return;
	}
	SaveClusters_r (node->children[0]);
	SaveClusters_r (node->children[1]);
}

/*
================
WritePortalFile
================
*/
void WritePortalFile (tree_t *tree)
{
	char	filename[1024];
	node_t *headnode;

	qprintf ("--- WritePortalFile ---\n");

	headnode = tree->headnode;
	num_visclusters = 0;
	num_visportals = 0;

	Tree_FreePortals_r (headnode);

	MakeHeadnodePortals (tree);

	CreateVisPortals_r (headnode);

// set the cluster field in every leaf and count the total number of portals

	NumberLeafs_r (headnode);
	
// write the file
	sprintf (filename, "%s.prt", source);
	printf ("writing %s\n", filename);
	pf = fopen (filename, "w");
	if (!pf)
		Error ("Error opening %s", filename);
		
	fprintf (pf, "%s\n", PORTALFILE);
	fprintf (pf, "%i\n", num_visclusters);
	fprintf (pf, "%i\n", num_visportals);

	qprintf ("%5i visclusters\n", num_visclusters);
	qprintf ("%5i visportals\n", num_visportals);

	WritePortalFile_r (headnode);

	fclose (pf);

	// we need to store the clusters out now because ordering
	// issues made us do this after writebsp...
	clusterleaf = 1;
	SaveClusters_r (headnode);
}

