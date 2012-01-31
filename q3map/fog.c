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


int			c_fogFragment;
int			c_fogPatchFragments;

/*
====================
DrawSurfToMesh
====================
*/
mesh_t	*DrawSurfToMesh( mapDrawSurface_t *ds ) {
	mesh_t		*m;

	m = malloc( sizeof( *m ) );
	m->width = ds->patchWidth;
	m->height = ds->patchHeight;
	m->verts = malloc( sizeof(m->verts[0]) * m->width * m->height );
	memcpy( m->verts, ds->verts, sizeof(m->verts[0]) * m->width * m->height );

	return m;
}


/*
====================
SplitMeshByPlane
====================
*/
void SplitMeshByPlane( mesh_t *in, vec3_t normal, float dist, mesh_t **front, mesh_t **back ) {
	int		w, h, split;
	float	d[MAX_PATCH_SIZE][MAX_PATCH_SIZE];
	drawVert_t	*dv, *v1, *v2;
	int		c_front, c_back, c_on;
	mesh_t	*f, *b;
	int		i;
	float	frac;
	int		frontAprox, backAprox;

	for ( i = 0 ; i < 2 ; i++ ) {
		dv = in->verts;
		c_front = 0;
		c_back = 0;
		c_on = 0;
		for ( h = 0 ; h < in->height ; h++ ) {
			for ( w = 0 ; w < in->width ; w++, dv++ ) {
				d[h][w] = DotProduct( dv->xyz, normal ) - dist;
				if ( d[h][w] > ON_EPSILON ) {
					c_front++;
				} else if ( d[h][w] < -ON_EPSILON ) {
					c_back++;
				} else {
					c_on++;
				}
			}
		}

		*front = NULL;
		*back = NULL;

		if ( !c_front ) {
			*back = in;
			return;
		}
		if ( !c_back ) {
			*front = in;
			return;
		}

		// find a split point
		split = -1;
		for ( w = 0 ; w < in->width -1 ; w++ ) {
			if ( ( d[0][w] < 0 ) != ( d[0][w+1] < 0 ) ) {
				if ( split == -1 ) {
					split = w;
					break;
				}
			}
		}

		if ( split == -1 ) {
			if ( i == 1 ) {
				qprintf( "No crossing points in patch\n");
				*front = in;
				return;
			}

			in = TransposeMesh( in );
			InvertMesh( in );
			continue;
		}

		// make sure the split point stays the same for all other rows
		for ( h = 1 ; h < in->height ; h++ ) {
			for ( w = 0 ; w < in->width -1 ; w++ ) {
				if ( ( d[h][w] < 0 ) != ( d[h][w+1] < 0 ) ) {
					if ( w != split ) {
						_printf( "multiple crossing points for patch -- can't clip\n");
						*front = in;
						return;
					}
				}
			}
			if ( ( d[h][split] < 0 ) == ( d[h][split+1] < 0 ) ) {
				_printf( "differing crossing points for patch -- can't clip\n");
				*front = in;
				return;
			}
		}

		break;
	}


	// create two new meshes
	f = malloc( sizeof( *f ) );
	f->width = split + 2;
	if ( ! (f->width & 1) ) {
		f->width++;
		frontAprox = 1;
	} else {
		frontAprox = 0;
	}
	if ( f->width > MAX_PATCH_SIZE ) {
		Error( "MAX_PATCH_SIZE after split");
	}
	f->height = in->height;
	f->verts = malloc( sizeof(f->verts[0]) * f->width * f->height );

	b = malloc( sizeof( *b ) );
	b->width = in->width - split;
	if ( ! (b->width & 1) ) {
		b->width++;
		backAprox = 1;
	} else {
		backAprox = 0;
	}
	if ( b->width > MAX_PATCH_SIZE ) {
		Error( "MAX_PATCH_SIZE after split");
	}
	b->height = in->height;
	b->verts = malloc( sizeof(b->verts[0]) * b->width * b->height );

	if ( d[0][0] > 0 ) {
		*front = f;
		*back = b;
	} else {
		*front = b;
		*back = f;
	}

	// distribute the points
	for ( w = 0 ; w < in->width ; w++ ) {
		for ( h = 0 ; h < in->height ; h++ ) {
			if ( w <= split ) {
				f->verts[ h * f->width + w ] = in->verts[ h * in->width + w ];
			} else {
				b->verts[ h * b->width + w - split + backAprox ] = in->verts[ h * in->width + w ];
			}
		}
	}

	// clip the crossing line
	for ( h = 0 ; h < in->height ; h++ ) {
		dv = &f->verts[ h * f->width + split + 1 ];
		v1 = &in->verts[ h * in->width + split ];
		v2 = &in->verts[ h * in->width + split + 1 ];
		frac = d[h][split] / ( d[h][split] - d[h][split+1] );
		for ( i = 0 ; i < 10 ; i++ ) {
			dv->xyz[i] = v1->xyz[i] + frac * ( v2->xyz[i] - v1->xyz[i] );
		}
		dv->xyz[10] = 0;//set all 4 colors to 0 
		if ( frontAprox ) {
			f->verts[ h * f->width + split + 2 ] = *dv;
		}
		b->verts[ h * b->width ] = *dv;
		if ( backAprox ) {
			b->verts[ h * b->width + 1 ] = *dv;
		}
	}

	/*
PrintMesh( in );
_printf("\n");
PrintMesh( f );
_printf("\n");
PrintMesh( b );
_printf("\n");
	*/

	FreeMesh( in );
}


/*
====================
ChopPatchByBrush
====================
*/
qboolean ChopPatchByBrush( mapDrawSurface_t *ds, bspbrush_t *b ) {
	int			i, j;
	side_t		*s;
	plane_t		*plane;
	mesh_t		*outside[MAX_BRUSH_SIDES];
	int			numOutside;
	mesh_t		*m, *front, *back;
	mapDrawSurface_t	*newds;

	m = DrawSurfToMesh( ds );
	numOutside = 0;

	// only split by the top and bottom planes to avoid
	// some messy patch clipping issues

	for ( i = 4 ; i <= 5 ; i++ ) {
		s = &b->sides[ i ];
		plane = &mapplanes[ s->planenum ];

		SplitMeshByPlane( m, plane->normal, plane->dist, &front, &back );

		if ( !back ) {
			// nothing actually contained inside
			for ( j = 0 ; j < numOutside ; j++ ) {
				FreeMesh( outside[j] );
			}
			return qfalse;
		}
		m = back;

		if ( front ) {
			if ( numOutside == MAX_BRUSH_SIDES ) {
				Error( "MAX_BRUSH_SIDES" );
			}
			outside[ numOutside ] = front;
			numOutside++;
		}
	}

	// all of outside fragments become seperate drawsurfs
	c_fogPatchFragments += numOutside;
	for ( i = 0 ; i < numOutside ; i++ ) {
		newds = DrawSurfaceForMesh( outside[ i ] );
		newds->shaderInfo = ds->shaderInfo;
		FreeMesh( outside[ i ] );
	}

	// replace ds with m
	ds->patchWidth = m->width;
	ds->patchHeight = m->height;
	ds->numVerts = m->width * m->height;
	free( ds->verts );
	ds->verts = malloc( ds->numVerts * sizeof( *ds->verts ) );
	memcpy( ds->verts, m->verts, ds->numVerts * sizeof( *ds->verts ) );

	FreeMesh( m );

	return qtrue;
}

//===============================================================================

/*
====================
WindingFromDrawSurf
====================
*/
winding_t	*WindingFromDrawSurf( mapDrawSurface_t *ds ) {
	winding_t	*w;
	int			i;

	w = AllocWinding( ds->numVerts );
	w->numpoints = ds->numVerts;
	for ( i = 0 ; i < ds->numVerts ; i++ ) {
		VectorCopy( ds->verts[i].xyz, w->p[i] );
	}
	return w;
}

/*
====================
ChopFaceByBrush

There may be a fragment contained in the brush
====================
*/
qboolean ChopFaceByBrush( mapDrawSurface_t *ds, bspbrush_t *b ) {
	int			i, j;
	side_t		*s;
	plane_t		*plane;
	winding_t	*w;
	winding_t	*front, *back;
	winding_t	*outside[MAX_BRUSH_SIDES];
	int			numOutside;
	mapDrawSurface_t	*newds;
	drawVert_t		*dv;
	shaderInfo_t	*si;
	float		mins[2];

	// brush primitive :
	// axis base
	vec3_t		texX,texY;
	vec_t		x,y;

	w = WindingFromDrawSurf( ds );
	numOutside = 0;

	for ( i = 0 ; i < b->numsides ; i++ ) {
		s = &b->sides[ i ];
		if ( s->backSide ) {
			continue;
		}
		plane = &mapplanes[ s->planenum ];

		// handle coplanar outfacing (don't fog)
		if ( ds->side->planenum == s->planenum ) {
			return qfalse;
		}

		// handle coplanar infacing (keep inside)
		if ( ( ds->side->planenum ^ 1 ) == s->planenum ) {
			continue;
		}

		// general case
		ClipWindingEpsilon( w, plane->normal, plane->dist, ON_EPSILON,
			&front, &back );
		FreeWinding( w );
		if ( !back ) {
			// nothing actually contained inside
			for ( j = 0 ; j < numOutside ; j++ ) {
				FreeWinding( outside[j] );
			}
			return qfalse;
		}
		if ( front ) {
			if ( numOutside == MAX_BRUSH_SIDES ) {
				Error( "MAX_BRUSH_SIDES" );
			}
			outside[ numOutside ] = front;
			numOutside++;
		}
		w = back;
	}

	// all of outside fragments become seperate drawsurfs
	// linked to the same side
	c_fogFragment += numOutside;
	s = ds->side;

	for ( i = 0 ; i < numOutside ; i++ ) {
		newds = DrawSurfaceForSide( ds->mapBrush, s, outside[i] );
		FreeWinding( outside[i] );
	}


	// replace ds->verts with the verts for w
	ds->numVerts = w->numpoints;
	free( ds->verts );

	ds->verts = malloc( ds->numVerts * sizeof( *ds->verts ) );
	memset( ds->verts, 0, ds->numVerts * sizeof( *ds->verts ) );

	si = s->shaderInfo;

	mins[0] = 9999;
	mins[1] = 9999;

	// compute s/t coordinates from brush primitive texture matrix
	// compute axis base
	ComputeAxisBase( mapplanes[s->planenum].normal, texX, texY );

	for ( j = 0 ; j < w->numpoints ; j++ ) {
		dv = ds->verts + j;
		VectorCopy( w->p[j], dv->xyz );
	
		if (g_bBrushPrimit==BPRIMIT_OLDBRUSHES)
		{
			// calculate texture s/t
			dv->st[0] = s->vecs[0][3] + DotProduct( s->vecs[0],	dv->xyz );
			dv->st[1] = s->vecs[1][3] + DotProduct( s->vecs[1],	dv->xyz );	
			dv->st[0] /= si->width;
			dv->st[1] /= si->height;
		}
		else
		{
			// calculate texture s/t from brush primitive texture matrix
			x = DotProduct( dv->xyz, texX );
			y = DotProduct( dv->xyz, texY );
			dv->st[0]=s->texMat[0][0]*x+s->texMat[0][1]*y+s->texMat[0][2];
			dv->st[1]=s->texMat[1][0]*x+s->texMat[1][1]*y+s->texMat[1][2];
		}

		if ( dv->st[0] < mins[0] ) {
			mins[0] = dv->st[0];
		}
		if ( dv->st[1] < mins[1] ) {
			mins[1] = dv->st[1];
		}

		// copy normal
		VectorCopy ( mapplanes[s->planenum].normal, dv->normal );
	}

	// adjust the texture coordinates to be as close to 0 as possible
	if ( !si->globalTexture ) {
		mins[0] = floor( mins[0] );
		mins[1] = floor( mins[1] );
		for ( i = 0 ; i < w->numpoints ; i++ ) {
			dv = ds->verts + i;
			dv->st[0] -= mins[0];
			dv->st[1] -= mins[1];
		}
	}

	return qtrue;
}

//===============================================================================


/*
=====================
FogDrawSurfs

Call after the surface list has been pruned, 
before tjunction fixing
before lightmap allocation
=====================
*/
void FogDrawSurfs( void ) {
	int					i, j, k;
	mapDrawSurface_t	*ds;
	bspbrush_t			*b;
	vec3_t				mins, maxs;
	int					c_fogged;
	int					numBaseDrawSurfs;
	dfog_t				*fog;

	qprintf("----- FogDrawsurfs -----\n");

	c_fogged = 0;
	c_fogFragment = 0;

	// find all fog brushes
	for ( b = entities[0].brushes ; b ; b = b->next ) {
		if ( !(b->contents & CONTENTS_FOG) ) {
			continue;
		}

		if ( numFogs == MAX_MAP_FOGS ) {
			Error( "MAX_MAP_FOGS" );
		}
		fog = &dfogs[numFogs];
		numFogs++;
		fog->brushNum = b->outputNumber;

		// find a side with a valid shaderInfo
		// non-axial fog columns may have bevel planes that need to be skipped
		for ( i = 0 ; i < b->numsides ; i++ ) {
			if ( b->sides[i].shaderInfo && (b->sides[i].shaderInfo->contents & CONTENTS_FOG) ) {
				strcpy( fog->shader, b->sides[i].shaderInfo->shader );
				break;
			}
		}
		if ( i == b->numsides ) {
			continue;		// shouldn't happen
		}

		fog->visibleSide = -1;

		// clip each surface into this, but don't clip any of
		// the resulting fragments to the same brush
		numBaseDrawSurfs = numMapDrawSurfs;
		for ( i = 0 ; i < numBaseDrawSurfs ; i++ ) {
			ds = &mapDrawSurfs[i];

			// bound the drawsurf
			ClearBounds( mins, maxs );
			for ( j = 0 ; j < ds->numVerts ; j++ ) {
				AddPointToBounds( ds->verts[j].xyz, mins, maxs );
			}

			// check against the fog brush
			for ( k = 0 ; k < 3 ; k++ ) {
				if ( mins[k] > b->maxs[k] ) {
					break;
				}
				if ( maxs[k] < b->mins[k] ) {
					break;
				}
			}
			if ( k < 3 ) {
				continue;		// bboxes don't intersect
			}

			if ( ds->mapBrush == b ) {
				int		s;

				s = ds->side - b->sides;
				if ( s <= 6 ) {	// not one of the reversed inside faces
					// this is a visible fog plane
					if ( fog->visibleSide != -1 ) {
						_printf( "WARNING: fog brush %i has multiple visible sides\n", b->brushnum );
					}
					fog->visibleSide = s;
				}
			}

			if ( ds->miscModel ) {
				// we could write splitting code for trimodels if we wanted to...
				c_fogged++;
				ds->fogNum = numFogs - 1;
			} else if ( ds->patch ) {
				if ( ChopPatchByBrush( ds, b ) ) {
					c_fogged++;
					ds->fogNum = numFogs - 1;
				}
			} else {
				if ( ChopFaceByBrush( ds, b ) ) {
					c_fogged++;
					ds->fogNum = numFogs - 1;
				}
			}
		}
	}

	// split the drawsurfs by the fog brushes

	qprintf( "%5i fogs\n", numFogs );
	qprintf( "%5i fog polygon fragments\n", c_fogFragment );
	qprintf( "%5i fog patch fragments\n", c_fogPatchFragments );
	qprintf( "%5i fogged drawsurfs\n", c_fogged );
}
