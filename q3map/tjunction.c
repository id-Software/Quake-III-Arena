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

typedef struct edgePoint_s {
	float		intercept;
	vec3_t		xyz;
	struct edgePoint_s	*prev, *next;
} edgePoint_t;

typedef struct edgeLine_s {
	vec3_t		normal1;
	float		dist1;
	
	vec3_t		normal2;
	float		dist2;
	
	vec3_t		origin;
	vec3_t		dir;

	edgePoint_t	chain;		// unused element of doubly linked list
} edgeLine_t;

typedef struct {
	float		length;
	drawVert_t	*dv[2];
} originalEdge_t;

#define	MAX_ORIGINAL_EDGES	0x10000
originalEdge_t	originalEdges[MAX_ORIGINAL_EDGES];
int				numOriginalEdges;


#define	MAX_EDGE_LINES		0x10000
edgeLine_t		edgeLines[MAX_EDGE_LINES];
int				numEdgeLines;

int				c_degenerateEdges;
int				c_addedVerts;
int				c_totalVerts;

int				c_natural, c_rotate, c_cant;

// these should be whatever epsilon we actually expect,
// plus SNAP_INT_TO_FLOAT 
#define	LINE_POSITION_EPSILON	0.25
#define	POINT_ON_LINE_EPSILON	0.25

/*
====================
InsertPointOnEdge
====================
*/
void InsertPointOnEdge( vec3_t v, edgeLine_t *e ) {
	vec3_t		delta;
	float		d;
	edgePoint_t	*p, *scan;

	VectorSubtract( v, e->origin, delta );
	d = DotProduct( delta, e->dir );

	p = malloc( sizeof(edgePoint_t) );
	p->intercept = d;
	VectorCopy( v, p->xyz );

	if ( e->chain.next == &e->chain ) {
		e->chain.next = e->chain.prev = p;
		p->next = p->prev = &e->chain;
		return;
	}

	scan = e->chain.next;
	for ( ; scan != &e->chain ; scan = scan->next ) {
		d = p->intercept - scan->intercept;
		if ( d > -LINE_POSITION_EPSILON && d < LINE_POSITION_EPSILON ) {
			free( p );
			return;		// the point is already set
		}

		if ( p->intercept < scan->intercept ) {
			// insert here
			p->prev = scan->prev;
			p->next = scan;
			scan->prev->next = p;
			scan->prev = p;
			return;
		}
	}

	// add at the end
	p->prev = scan->prev;
	p->next = scan;
	scan->prev->next = p;
	scan->prev = p;
}


/*
====================
AddEdge
====================
*/
int AddEdge( vec3_t v1, vec3_t v2, qboolean createNonAxial ) {
	int			i;
	edgeLine_t	*e;
	float		d;
	vec3_t		dir;

	VectorSubtract( v2, v1, dir );
	d = VectorNormalize( dir, dir );
	if ( d < 0.1 ) {
		// if we added a 0 length vector, it would make degenerate planes
		c_degenerateEdges++;
		return -1;
	}

	if ( !createNonAxial ) {
		if ( fabs( dir[0] + dir[1] + dir[2] ) != 1.0 ) {
			if ( numOriginalEdges == MAX_ORIGINAL_EDGES ) {
				Error( "MAX_ORIGINAL_EDGES" );
			}
			originalEdges[ numOriginalEdges ].dv[0] = (drawVert_t *)v1;
			originalEdges[ numOriginalEdges ].dv[1] = (drawVert_t *)v2;
			originalEdges[ numOriginalEdges ].length = d;
			numOriginalEdges++;
			return -1;
		}
	}

	for ( i = 0 ; i < numEdgeLines ; i++ ) {
		e = &edgeLines[i];

		d = DotProduct( v1, e->normal1 ) - e->dist1;
		if ( d < -POINT_ON_LINE_EPSILON || d > POINT_ON_LINE_EPSILON ) {
			continue;
		}
		d = DotProduct( v1, e->normal2 ) - e->dist2;
		if ( d < -POINT_ON_LINE_EPSILON || d > POINT_ON_LINE_EPSILON ) {
			continue;
		}

		d = DotProduct( v2, e->normal1 ) - e->dist1;
		if ( d < -POINT_ON_LINE_EPSILON || d > POINT_ON_LINE_EPSILON ) {
			continue;
		}
		d = DotProduct( v2, e->normal2 ) - e->dist2;
		if ( d < -POINT_ON_LINE_EPSILON || d > POINT_ON_LINE_EPSILON ) {
			continue;
		}

		// this is the edge
		InsertPointOnEdge( v1, e );
		InsertPointOnEdge( v2, e );
		return i;
	}

	// create a new edge
	if ( numEdgeLines >= MAX_EDGE_LINES ) {
		Error( "MAX_EDGE_LINES" );
	}

	e = &edgeLines[ numEdgeLines ];
	numEdgeLines++;

	e->chain.next = e->chain.prev = &e->chain;

	VectorCopy( v1, e->origin );
	VectorCopy( dir, e->dir );

	MakeNormalVectors( e->dir, e->normal1, e->normal2 );
	e->dist1 = DotProduct( e->origin, e->normal1 );
	e->dist2 = DotProduct( e->origin, e->normal2 );

	InsertPointOnEdge( v1, e );
	InsertPointOnEdge( v2, e );

	return numEdgeLines - 1;
}

/*
====================
AddSurfaceEdges
====================
*/
void AddSurfaceEdges( mapDrawSurface_t *ds ) {
	int		i;

	for ( i = 0 ; i < ds->numVerts ; i++ ) {
		// save the edge number in the lightmap field
		// so we don't need to look it up again
		ds->verts[i].lightmap[0] = 
			AddEdge( ds->verts[i].xyz, ds->verts[(i+1) % ds->numVerts].xyz, qfalse );
	}
}

/*
================
ColinearEdge
================
*/
qboolean ColinearEdge( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	vec3_t	midpoint, dir, offset, on;
	float	d;

	VectorSubtract( v2, v1, midpoint );
	VectorSubtract( v3, v1, dir );
	d = VectorNormalize( dir, dir );
	if ( d == 0 ) {
		return qfalse;	// degenerate
	}

	d = DotProduct( midpoint, dir );
	VectorScale( dir, d, on );
	VectorSubtract( midpoint, on, offset );
	d = VectorLength ( offset );

	if ( d < 0.1 ) {
		return qtrue;
	}

	return qfalse;
}

/*
====================
AddPatchEdges

Add colinear border edges, which will fix some classes of patch to
brush tjunctions
====================
*/
void AddPatchEdges( mapDrawSurface_t *ds ) {
	int		i;
	float	*v1, *v2, *v3;

	for ( i = 0 ; i < ds->patchWidth - 2; i+=2 ) {
		v1 = ds->verts[ i ].xyz;
		v2 = ds->verts[ i + 1 ].xyz;
		v3 = ds->verts[ i + 2 ].xyz;

		// if v2 is the midpoint of v1 to v3, add an edge from v1 to v3
		if ( ColinearEdge( v1, v2, v3 ) ) {
			AddEdge( v1, v3, qfalse );
		}

		v1 = ds->verts[ ( ds->patchHeight - 1 ) * ds->patchWidth + i ].xyz;
		v2 = ds->verts[ ( ds->patchHeight - 1 ) * ds->patchWidth + i + 1 ].xyz;
		v3 = ds->verts[ ( ds->patchHeight - 1 ) * ds->patchWidth + i + 2 ].xyz;

		// if v2 is on the v1 to v3 line, add an edge from v1 to v3
		if ( ColinearEdge( v1, v2, v3 ) ) {
			AddEdge( v1, v3, qfalse );
		}
	}

	for ( i = 0 ; i < ds->patchHeight - 2 ; i+=2 ) {
		v1 = ds->verts[ i * ds->patchWidth ].xyz;
		v2 = ds->verts[ ( i + 1 ) * ds->patchWidth ].xyz;
		v3 = ds->verts[ ( i + 2 ) * ds->patchWidth ].xyz;

		// if v2 is the midpoint of v1 to v3, add an edge from v1 to v3
		if ( ColinearEdge( v1, v2, v3 ) ) {
			AddEdge( v1, v3, qfalse );
		}

		v1 = ds->verts[ ( ds->patchWidth - 1 ) + i * ds->patchWidth ].xyz;
		v2 = ds->verts[ ( ds->patchWidth - 1 ) + ( i + 1 ) * ds->patchWidth ].xyz;
		v3 = ds->verts[ ( ds->patchWidth - 1 ) + ( i + 2 ) * ds->patchWidth ].xyz;

		// if v2 is the midpoint of v1 to v3, add an edge from v1 to v3
		if ( ColinearEdge( v1, v2, v3 ) ) {
			AddEdge( v1, v3, qfalse );
		}
	}


}


/*
====================
FixSurfaceJunctions
====================
*/
#define	MAX_SURFACE_VERTS	256
void FixSurfaceJunctions( mapDrawSurface_t *ds ) {
	int			i, j, k;
	edgeLine_t	*e;
	edgePoint_t	*p;
	int			originalVerts;
	int			counts[MAX_SURFACE_VERTS];
	int			originals[MAX_SURFACE_VERTS];
	int			firstVert[MAX_SURFACE_VERTS];
	drawVert_t	verts[MAX_SURFACE_VERTS], *v1, *v2;
	int			numVerts;
	float		start, end, frac;
	vec3_t		delta;

	originalVerts = ds->numVerts;

	numVerts = 0;
	for ( i = 0 ; i < ds->numVerts ; i++ ) {
		counts[i] = 0;
		firstVert[i] = numVerts;

		// copy first vert
		if ( numVerts == MAX_SURFACE_VERTS ) {
			Error( "MAX_SURFACE_VERTS" );
		}
		verts[numVerts] = ds->verts[i];
		originals[numVerts] = i;
		numVerts++;

		// check to see if there are any t junctions before the next vert
		v1 = &ds->verts[i];
		v2 = &ds->verts[ (i+1) % ds->numVerts ];

		j = (int)ds->verts[i].lightmap[0];
		if ( j == -1 ) {
			continue;		// degenerate edge
		}
		e = &edgeLines[ j ];
		
		VectorSubtract( v1->xyz, e->origin, delta );
		start = DotProduct( delta, e->dir );

		VectorSubtract( v2->xyz, e->origin, delta );
		end = DotProduct( delta, e->dir );


		if ( start < end ) {
			p = e->chain.next;
		} else {
			p = e->chain.prev;
		}

		for (  ; p != &e->chain ;  ) {
			if ( start < end ) {
				if ( p->intercept > end - ON_EPSILON ) {
					break;
				}
			} else {
				if ( p->intercept < end + ON_EPSILON ) {
					break;
				}
			}

			if ( 
				( start < end && p->intercept > start + ON_EPSILON ) ||
				( start > end && p->intercept < start - ON_EPSILON ) ) {
				// insert this point
				if ( numVerts == MAX_SURFACE_VERTS ) {
					Error( "MAX_SURFACE_VERTS" );
				}

				// take the exact intercept point
				VectorCopy( p->xyz, verts[ numVerts ].xyz );

				// copy the normal
				VectorCopy( v1->normal, verts[ numVerts ].normal );

				// interpolate the texture coordinates
				frac = ( p->intercept - start ) / ( end - start );
				for ( j = 0 ; j < 2 ; j++ ) {
					verts[ numVerts ].st[j] = v1->st[j] + 
						frac * ( v2->st[j] - v1->st[j] );
				}
				originals[numVerts] = i;
				numVerts++;
				counts[i]++;
			}

			if ( start < end ) {
				p = p->next;
			} else {
				p = p->prev;
			}
		}
	}

	c_addedVerts += numVerts - ds->numVerts;
	c_totalVerts += numVerts;


	// FIXME: check to see if the entire surface degenerated
	// after snapping

	// rotate the points so that the initial vertex is between
	// two non-subdivided edges
	for ( i = 0 ; i < numVerts ; i++ ) {
		if ( originals[ (i+1) % numVerts ] == originals[ i ] ) {
			continue;
		}
		j = (i + numVerts - 1 ) % numVerts;
		k = (i + numVerts - 2 ) % numVerts;
		if ( originals[ j ] == originals[ k ] ) {
			continue;
		}
		break;
	}

	if ( i == 0 ) {
		// fine the way it is
		c_natural++;

		ds->numVerts = numVerts;
		ds->verts = malloc( numVerts * sizeof( *ds->verts ) );
		memcpy( ds->verts, verts, numVerts * sizeof( *ds->verts ) );

		return;
	}
	if ( i == numVerts ) {
		// create a vertex in the middle to start the fan
		c_cant++;

/*
		memset ( &verts[numVerts], 0, sizeof( verts[numVerts] ) );
		for ( i = 0 ; i < numVerts ; i++ ) {
			for ( j = 0 ; j < 10 ; j++ ) {
				verts[numVerts].xyz[j] += verts[i].xyz[j];
			}
		}
		for ( j = 0 ; j < 10 ; j++ ) {
			verts[numVerts].xyz[j] /= numVerts;
		}

		i = numVerts;
		numVerts++;
*/
	} else {
		// just rotate the vertexes
		c_rotate++;

	}

	ds->numVerts = numVerts;
	ds->verts = malloc( numVerts * sizeof( *ds->verts ) );

	for ( j = 0 ; j < ds->numVerts ; j++ ) {
		ds->verts[j] = verts[ ( j + i ) % ds->numVerts ];
	}
}

/*
================
EdgeCompare
================
*/
int EdgeCompare( const void *elem1, const void *elem2 ) {
	float	d1, d2;

	d1 = ((originalEdge_t *)elem1)->length;
	d2 = ((originalEdge_t *)elem2)->length;

	if ( d1 < d2 ) {
		return -1;
	}
	if ( d2 > d1 ) {
		return 1;
	}
	return 0;
}


/*
================
FixTJunctions

Call after the surface list has been pruned, but before lightmap allocation
================
*/
void FixTJunctions( entity_t *ent ) {
	int					i;
	mapDrawSurface_t	*ds;
	int					axialEdgeLines;
	originalEdge_t		*e;

	qprintf("----- FixTJunctions -----\n");

	numEdgeLines = 0;
	numOriginalEdges = 0;

	// add all the edges
	// this actually creates axial edges, but it
	// only creates originalEdge_t structures
	// for non-axial edges
	for ( i = ent->firstDrawSurf ; i < numMapDrawSurfs ; i++ ) {
		ds = &mapDrawSurfs[i];
		if ( ds->patch ) {
			AddPatchEdges( ds );
		} else if ( ds->shaderInfo->autosprite || ds->shaderInfo->notjunc || ds->miscModel ) {
			// miscModels don't add tjunctions
		} else {
			AddSurfaceEdges( ds );
		}
	}

	axialEdgeLines = numEdgeLines;

	// sort the non-axial edges by length
	qsort( originalEdges, numOriginalEdges, sizeof(originalEdges[0]), EdgeCompare );

	// add the non-axial edges, longest first
	// this gives the most accurate edge description
	for ( i = 0 ; i < numOriginalEdges ; i++ ) {
		e = &originalEdges[i];
		e->dv[0]->lightmap[0] = AddEdge( e->dv[0]->xyz, e->dv[1]->xyz, qtrue );
	}

	qprintf( "%6i axial edge lines\n", axialEdgeLines );
	qprintf( "%6i non-axial edge lines\n", numEdgeLines - axialEdgeLines );
	qprintf( "%6i degenerate edges\n", c_degenerateEdges );

	// insert any needed vertexes
	for ( i = ent->firstDrawSurf ; i < numMapDrawSurfs ; i++ ) {
		ds = &mapDrawSurfs[i];
		if ( ds->patch ) {
			continue;
		}
		if ( ds->shaderInfo->autosprite || ds->shaderInfo->notjunc || ds->miscModel ) {
			continue;
		}

		FixSurfaceJunctions( ds );
	}

	qprintf( "%6i verts added for tjunctions\n", c_addedVerts );
	qprintf( "%6i total verts\n", c_totalVerts );
	qprintf( "%6i naturally ordered\n", c_natural );
	qprintf( "%6i rotated orders\n", c_rotate );
	qprintf( "%6i can't order\n", c_cant );
}
