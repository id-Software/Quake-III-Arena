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

#include "tr_local.h"

/*

This file does all of the processing necessary to turn a raw grid of points
read from the map file into a srfBspSurface_t ready for rendering.

The level of detail solution is direction independent, based only on subdivided
distance from the true curve.

Only a single entry point:

srfBspSurface_t *R_SubdividePatchToGrid( int width, int height,
								srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] ) {

*/


/*
============
LerpDrawVert
============
*/
static void LerpDrawVert( srfVert_t *a, srfVert_t *b, srfVert_t *out ) {
	out->xyz[0] = 0.5f * (a->xyz[0] + b->xyz[0]);
	out->xyz[1] = 0.5f * (a->xyz[1] + b->xyz[1]);
	out->xyz[2] = 0.5f * (a->xyz[2] + b->xyz[2]);

	out->st[0] = 0.5f * (a->st[0] + b->st[0]);
	out->st[1] = 0.5f * (a->st[1] + b->st[1]);

	out->lightmap[0] = 0.5f * (a->lightmap[0] + b->lightmap[0]);
	out->lightmap[1] = 0.5f * (a->lightmap[1] + b->lightmap[1]);

	out->color[0] = ((int)a->color[0] + (int)b->color[0]) >> 1;
	out->color[1] = ((int)a->color[1] + (int)b->color[1]) >> 1;
	out->color[2] = ((int)a->color[2] + (int)b->color[2]) >> 1;
	out->color[3] = ((int)a->color[3] + (int)b->color[3]) >> 1;
}

/*
============
Transpose
============
*/
static void Transpose( int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] ) {
	int		i, j;
	srfVert_t	temp;

	if ( width > height ) {
		for ( i = 0 ; i < height ; i++ ) {
			for ( j = i + 1 ; j < width ; j++ ) {
				if ( j < height ) {
					// swap the value
					temp = ctrl[j][i];
					ctrl[j][i] = ctrl[i][j];
					ctrl[i][j] = temp;
				} else {
					// just copy
					ctrl[j][i] = ctrl[i][j];
				}
			}
		}
	} else {
		for ( i = 0 ; i < width ; i++ ) {
			for ( j = i + 1 ; j < height ; j++ ) {
				if ( j < width ) {
					// swap the value
					temp = ctrl[i][j];
					ctrl[i][j] = ctrl[j][i];
					ctrl[j][i] = temp;
				} else {
					// just copy
					ctrl[i][j] = ctrl[j][i];
				}
			}
		}
	}

}


/*
=================
MakeMeshNormals

Handles all the complicated wrapping and degenerate cases
=================
*/
static void MakeMeshNormals( int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] ) {
	int		i, j, k, dist;
	vec3_t	normal;
	vec3_t	sum;
	int		count = 0;
	vec3_t	base;
	vec3_t	delta;
	int		x, y;
	srfVert_t	*dv;
	vec3_t		around[8], temp;
	qboolean	good[8];
	qboolean	wrapWidth, wrapHeight;
	float		len;
static	int	neighbors[8][2] = {
	{0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}
	};

	wrapWidth = qfalse;
	for ( i = 0 ; i < height ; i++ ) {
		VectorSubtract( ctrl[i][0].xyz, ctrl[i][width-1].xyz, delta );
		len = VectorLengthSquared( delta );
		if ( len > 1.0 ) {
			break;
		}
	}
	if ( i == height ) {
		wrapWidth = qtrue;
	}

	wrapHeight = qfalse;
	for ( i = 0 ; i < width ; i++ ) {
		VectorSubtract( ctrl[0][i].xyz, ctrl[height-1][i].xyz, delta );
		len = VectorLengthSquared( delta );
		if ( len > 1.0 ) {
			break;
		}
	}
	if ( i == width) {
		wrapHeight = qtrue;
	}


	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			count = 0;
			dv = &ctrl[j][i];
			VectorCopy( dv->xyz, base );
			for ( k = 0 ; k < 8 ; k++ ) {
				VectorClear( around[k] );
				good[k] = qfalse;

				for ( dist = 1 ; dist <= 3 ; dist++ ) {
					x = i + neighbors[k][0] * dist;
					y = j + neighbors[k][1] * dist;
					if ( wrapWidth ) {
						if ( x < 0 ) {
							x = width - 1 + x;
						} else if ( x >= width ) {
							x = 1 + x - width;
						}
					}
					if ( wrapHeight ) {
						if ( y < 0 ) {
							y = height - 1 + y;
						} else if ( y >= height ) {
							y = 1 + y - height;
						}
					}

					if ( x < 0 || x >= width || y < 0 || y >= height ) {
						break;					// edge of patch
					}
					VectorSubtract( ctrl[y][x].xyz, base, temp );
					if ( VectorNormalize2( temp, temp ) == 0 ) {
						continue;				// degenerate edge, get more dist
					} else {
						good[k] = qtrue;
						VectorCopy( temp, around[k] );
						break;					// good edge
					}
				}
			}

			VectorClear( sum );
			for ( k = 0 ; k < 8 ; k++ ) {
				if ( !good[k] || !good[(k+1)&7] ) {
					continue;	// didn't get two points
				}
				CrossProduct( around[(k+1)&7], around[k], normal );
				if ( VectorNormalize2( normal, normal ) == 0 ) {
					continue;
				}
				VectorAdd( normal, sum, sum );
				count++;
			}
			//if ( count == 0 ) {
			//	printf("bad normal\n");
			//}
			{
				vec3_t fNormal;
				VectorNormalize2(sum, fNormal);
				R_VaoPackNormal(dv->normal, fNormal);
			}
		}
	}
}

static void MakeMeshTangentVectors(int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], int numIndexes,
								   glIndex_t indexes[(MAX_GRID_SIZE-1)*(MAX_GRID_SIZE-1)*2*3])
{
	int             i, j;
	srfVert_t      *dv[3];
	static srfVert_t       ctrl2[MAX_GRID_SIZE * MAX_GRID_SIZE];
	glIndex_t  *tri;

	// FIXME: use more elegant way
	for(i = 0; i < width; i++)
	{
		for(j = 0; j < height; j++)
		{
			dv[0] = &ctrl2[j * width + i];
			*dv[0] = ctrl[j][i];
		}
	}

	for(i = 0, tri = indexes; i < numIndexes; i += 3, tri += 3)
	{
		dv[0] = &ctrl2[tri[0]];
		dv[1] = &ctrl2[tri[1]];
		dv[2] = &ctrl2[tri[2]];

		R_CalcTangentVectors(dv);
	}

	for(i = 0; i < width; i++)
	{
		for(j = 0; j < height; j++)
		{
			dv[0] = &ctrl2[j * width + i];
			dv[1] = &ctrl[j][i];

			VectorCopy4(dv[0]->tangent, dv[1]->tangent);
		}
	}
}


static int MakeMeshIndexes(int width, int height, glIndex_t indexes[(MAX_GRID_SIZE-1)*(MAX_GRID_SIZE-1)*2*3])
{
	int             i, j;
	int             numIndexes;
	int             w, h;

	h = height - 1;
	w = width - 1;
	numIndexes = 0;
	for(i = 0; i < h; i++)
	{
		for(j = 0; j < w; j++)
		{
			int             v1, v2, v3, v4;

			// vertex order to be reckognized as tristrips
			v1 = i * width + j + 1;
			v2 = v1 - 1;
			v3 = v2 + width;
			v4 = v3 + 1;

			indexes[numIndexes++] = v2;
			indexes[numIndexes++] = v3;
			indexes[numIndexes++] = v1;

			indexes[numIndexes++] = v1;
			indexes[numIndexes++] = v3;
			indexes[numIndexes++] = v4;
		}
	}

	return numIndexes;
}


/*
============
InvertCtrl
============
*/
static void InvertCtrl( int width, int height, srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] ) {
	int		i, j;
	srfVert_t	temp;

	for ( i = 0 ; i < height ; i++ ) {
		for ( j = 0 ; j < width/2 ; j++ ) {
			temp = ctrl[i][j];
			ctrl[i][j] = ctrl[i][width-1-j];
			ctrl[i][width-1-j] = temp;
		}
	}
}


/*
=================
InvertErrorTable
=================
*/
static void InvertErrorTable( float errorTable[2][MAX_GRID_SIZE], int width, int height ) {
	int		i;
	float	copy[2][MAX_GRID_SIZE];

	Com_Memcpy( copy, errorTable, sizeof( copy ) );

	for ( i = 0 ; i < width ; i++ ) {
		errorTable[1][i] = copy[0][i];	//[width-1-i];
	}

	for ( i = 0 ; i < height ; i++ ) {
		errorTable[0][i] = copy[1][height-1-i];
	}

}

/*
==================
PutPointsOnCurve
==================
*/
static void PutPointsOnCurve( srfVert_t	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], 
							 int width, int height ) {
	int			i, j;
	srfVert_t	prev, next;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 1 ; j < height ; j += 2 ) {
			LerpDrawVert( &ctrl[j][i], &ctrl[j+1][i], &prev );
			LerpDrawVert( &ctrl[j][i], &ctrl[j-1][i], &next );
			LerpDrawVert( &prev, &next, &ctrl[j][i] );
		}
	}


	for ( j = 0 ; j < height ; j++ ) {
		for ( i = 1 ; i < width ; i += 2 ) {
			LerpDrawVert( &ctrl[j][i], &ctrl[j][i+1], &prev );
			LerpDrawVert( &ctrl[j][i], &ctrl[j][i-1], &next );
			LerpDrawVert( &prev, &next, &ctrl[j][i] );
		}
	}
}

/*
=================
R_CreateSurfaceGridMesh
=================
*/
void R_CreateSurfaceGridMesh(srfBspSurface_t *grid, int width, int height,
								srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], float errorTable[2][MAX_GRID_SIZE],
								int numIndexes, glIndex_t indexes[(MAX_GRID_SIZE-1)*(MAX_GRID_SIZE-1)*2*3]) {
	int i, j;
	srfVert_t	*vert;
	vec3_t		tmpVec;

	// copy the results out to a grid
	Com_Memset(grid, 0, sizeof(*grid));

#ifdef PATCH_STITCHING
	grid->widthLodError = /*ri.Hunk_Alloc*/ ri.Malloc( width * 4 );
	Com_Memcpy( grid->widthLodError, errorTable[0], width * 4 );

	grid->heightLodError = /*ri.Hunk_Alloc*/ ri.Malloc( height * 4 );
	Com_Memcpy( grid->heightLodError, errorTable[1], height * 4 );

	grid->numIndexes = numIndexes;
	grid->indexes = ri.Malloc(grid->numIndexes * sizeof(glIndex_t));
	Com_Memcpy(grid->indexes, indexes, numIndexes * sizeof(glIndex_t));

	grid->numVerts = (width * height);
	grid->verts = ri.Malloc(grid->numVerts * sizeof(srfVert_t));
#else
	grid->widthLodError = ri.Hunk_Alloc( width * 4 );
	Com_Memcpy( grid->widthLodError, errorTable[0], width * 4 );

	grid->heightLodError = ri.Hunk_Alloc( height * 4 );
	Com_Memcpy( grid->heightLodError, errorTable[1], height * 4 );

	grid->numIndexes = numIndexes;
	grid->indexes = ri.Hunk_Alloc(grid->numIndexes * sizeof(glIndex_t), h_low);
	Com_Memcpy(grid->indexes, indexes, numIndexes * sizeof(glIndex_t));

	grid->numVerts = (width * height);
	grid->verts = ri.Hunk_Alloc(grid->numVerts * sizeof(srfVert_t), h_low);
#endif

	grid->width = width;
	grid->height = height;
	grid->surfaceType = SF_GRID;
	ClearBounds( grid->cullBounds[0], grid->cullBounds[1] );
	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			vert = &grid->verts[j*width+i];
			*vert = ctrl[j][i];
			AddPointToBounds( vert->xyz, grid->cullBounds[0], grid->cullBounds[1] );
		}
	}

	// compute local origin and bounds
	VectorAdd( grid->cullBounds[0], grid->cullBounds[1], grid->cullOrigin );
	VectorScale( grid->cullOrigin, 0.5f, grid->cullOrigin );
	VectorSubtract( grid->cullBounds[0], grid->cullOrigin, tmpVec );
	grid->cullRadius = VectorLength( tmpVec );

	VectorCopy( grid->cullOrigin, grid->lodOrigin );
	grid->lodRadius = grid->cullRadius;
	//
}

/*
=================
R_FreeSurfaceGridMesh
=================
*/
static void R_FreeSurfaceGridMeshData( srfBspSurface_t *grid ) {
	ri.Free(grid->widthLodError);
	ri.Free(grid->heightLodError);
	ri.Free(grid->indexes);
	ri.Free(grid->verts);
}

/*
=================
R_SubdividePatchToGrid
=================
*/
void R_SubdividePatchToGrid( srfBspSurface_t *grid, int width, int height,
								srfVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] ) {
	int			i, j, k, l;
	srfVert_t_cleared( prev );
	srfVert_t_cleared( next );
	srfVert_t_cleared( mid );
	float		len, maxLen;
	int			dir;
	int			t;
	srfVert_t	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float		errorTable[2][MAX_GRID_SIZE];
	int			numIndexes;
	static glIndex_t indexes[(MAX_GRID_SIZE-1)*(MAX_GRID_SIZE-1)*2*3];
	int consecutiveComplete;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			ctrl[j][i] = points[j*width+i];
		}
	}

	for ( dir = 0 ; dir < 2 ; dir++ ) {

		for ( j = 0 ; j < MAX_GRID_SIZE ; j++ ) {
			errorTable[dir][j] = 0;
		}

		consecutiveComplete = 0;

		// horizontal subdivisions
		for ( j = 0 ; ; j = (j + 2) % (width - 1) ) {
			// check subdivided midpoints against control points

			// FIXME: also check midpoints of adjacent patches against the control points
			// this would basically stitch all patches in the same LOD group together.

			maxLen = 0;
			for ( i = 0 ; i < height ; i++ ) {
				vec3_t		midxyz;
				vec3_t		midxyz2;
				vec3_t		dir;
				vec3_t		projected;
				float		d;

				// calculate the point on the curve
				for ( l = 0 ; l < 3 ; l++ ) {
					midxyz[l] = (ctrl[i][j].xyz[l] + ctrl[i][j+1].xyz[l] * 2
							+ ctrl[i][j+2].xyz[l] ) * 0.25f;
				}

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				VectorSubtract( midxyz, ctrl[i][j].xyz, midxyz );
				VectorSubtract( ctrl[i][j+2].xyz, ctrl[i][j].xyz, dir );
				VectorNormalize( dir );

				d = DotProduct( midxyz, dir );
				VectorScale( dir, d, projected );
				VectorSubtract( midxyz, projected, midxyz2);
				len = VectorLengthSquared( midxyz2 );			// we will do the sqrt later
				if ( len > maxLen ) {
					maxLen = len;
				}
			}

			maxLen = sqrt(maxLen);

			// if all the points are on the lines, remove the entire columns
			if ( maxLen < 0.1f ) {
				errorTable[dir][j+1] = 999;
				// if we go over the whole grid twice without adding any columns, stop
				if (++consecutiveComplete >= width)
					break;
				continue;
			}

			// see if we want to insert subdivided columns
			if ( width + 2 > MAX_GRID_SIZE ) {
				errorTable[dir][j+1] = 1.0f/maxLen;
				break;	// can't subdivide any more
			}

			if ( maxLen <= r_subdivisions->value ) {
				errorTable[dir][j+1] = 1.0f/maxLen;
				// if we go over the whole grid twice without adding any columns, stop
				if (++consecutiveComplete >= width)
					break;
				continue;	// didn't need subdivision
			}

			errorTable[dir][j+2] = 1.0f/maxLen;

			consecutiveComplete = 0;

			// insert two columns and replace the peak
			width += 2;
			for ( i = 0 ; i < height ; i++ ) {
				LerpDrawVert( &ctrl[i][j], &ctrl[i][j+1], &prev );
				LerpDrawVert( &ctrl[i][j+1], &ctrl[i][j+2], &next );
				LerpDrawVert( &prev, &next, &mid );

				for ( k = width - 1 ; k > j + 3 ; k-- ) {
					ctrl[i][k] = ctrl[i][k-2];
				}
				ctrl[i][j + 1] = prev;
				ctrl[i][j + 2] = mid;
				ctrl[i][j + 3] = next;
			}

			// skip the new one, we'll get it on the next pass
			j += 2;
		}

		Transpose( width, height, ctrl );
		t = width;
		width = height;
		height = t;
	}


	// put all the aproximating points on the curve
	PutPointsOnCurve( ctrl, width, height );

	// cull out any rows or columns that are colinear
	for ( i = 1 ; i < width-1 ; i++ ) {
		if ( errorTable[0][i] != 999 ) {
			continue;
		}
		for ( j = i+1 ; j < width ; j++ ) {
			for ( k = 0 ; k < height ; k++ ) {
				ctrl[k][j-1] = ctrl[k][j];
			}
			errorTable[0][j-1] = errorTable[0][j];
		}
		width--;
	}

	for ( i = 1 ; i < height-1 ; i++ ) {
		if ( errorTable[1][i] != 999 ) {
			continue;
		}
		for ( j = i+1 ; j < height ; j++ ) {
			for ( k = 0 ; k < width ; k++ ) {
				ctrl[j-1][k] = ctrl[j][k];
			}
			errorTable[1][j-1] = errorTable[1][j];
		}
		height--;
	}

#if 1
	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if ( height > width ) {
		Transpose( width, height, ctrl );
		InvertErrorTable( errorTable, width, height );
		t = width;
		width = height;
		height = t;
		InvertCtrl( width, height, ctrl );
	}
#endif

	// calculate indexes
	numIndexes = MakeMeshIndexes(width, height, indexes);

	// calculate normals
	MakeMeshNormals( width, height, ctrl );
	MakeMeshTangentVectors(width, height, ctrl, numIndexes, indexes);

	R_CreateSurfaceGridMesh(grid, width, height, ctrl, errorTable, numIndexes, indexes);
}

/*
===============
R_GridInsertColumn
===============
*/
void R_GridInsertColumn( srfBspSurface_t *grid, int column, int row, vec3_t point, float loderror ) {
	int i, j;
	int width, height, oldwidth;
	srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float errorTable[2][MAX_GRID_SIZE];
	float lodRadius;
	vec3_t lodOrigin;
	int    numIndexes;
	static glIndex_t indexes[(MAX_GRID_SIZE-1)*(MAX_GRID_SIZE-1)*2*3];

	oldwidth = 0;
	width = grid->width + 1;
	if (width > MAX_GRID_SIZE)
		return;
	height = grid->height;
	for (i = 0; i < width; i++) {
		if (i == column) {
			//insert new column
			for (j = 0; j < grid->height; j++) {
				LerpDrawVert( &grid->verts[j * grid->width + i-1], &grid->verts[j * grid->width + i], &ctrl[j][i] );
				if (j == row)
					VectorCopy(point, ctrl[j][i].xyz);
			}
			errorTable[0][i] = loderror;
			continue;
		}
		errorTable[0][i] = grid->widthLodError[oldwidth];
		for (j = 0; j < grid->height; j++) {
			ctrl[j][i] = grid->verts[j * grid->width + oldwidth];
		}
		oldwidth++;
	}
	for (j = 0; j < grid->height; j++) {
		errorTable[1][j] = grid->heightLodError[j];
	}
	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );

	// calculate indexes
	numIndexes = MakeMeshIndexes(width, height, indexes);

	// calculate normals
	MakeMeshNormals( width, height, ctrl );
	MakeMeshTangentVectors(width, height, ctrl, numIndexes, indexes);

	VectorCopy(grid->lodOrigin, lodOrigin);
	lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMeshData(grid);
	// create a new grid
	R_CreateSurfaceGridMesh(grid, width, height, ctrl, errorTable, numIndexes, indexes);
	grid->lodRadius = lodRadius;
	VectorCopy(lodOrigin, grid->lodOrigin);
}

/*
===============
R_GridInsertRow
===============
*/
void R_GridInsertRow( srfBspSurface_t *grid, int row, int column, vec3_t point, float loderror ) {
	int i, j;
	int width, height, oldheight;
	srfVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float errorTable[2][MAX_GRID_SIZE];
	float lodRadius;
	vec3_t lodOrigin;
	int             numIndexes;
	static glIndex_t indexes[(MAX_GRID_SIZE-1)*(MAX_GRID_SIZE-1)*2*3];

	oldheight = 0;
	width = grid->width;
	height = grid->height + 1;
	if (height > MAX_GRID_SIZE)
		return;
	for (i = 0; i < height; i++) {
		if (i == row) {
			//insert new row
			for (j = 0; j < grid->width; j++) {
				LerpDrawVert( &grid->verts[(i-1) * grid->width + j], &grid->verts[i * grid->width + j], &ctrl[i][j] );
				if (j == column)
					VectorCopy(point, ctrl[i][j].xyz);
			}
			errorTable[1][i] = loderror;
			continue;
		}
		errorTable[1][i] = grid->heightLodError[oldheight];
		for (j = 0; j < grid->width; j++) {
			ctrl[i][j] = grid->verts[oldheight * grid->width + j];
		}
		oldheight++;
	}
	for (j = 0; j < grid->width; j++) {
		errorTable[0][j] = grid->widthLodError[j];
	}
	// put all the aproximating points on the curve
	//PutPointsOnCurve( ctrl, width, height );

	// calculate indexes
	numIndexes = MakeMeshIndexes(width, height, indexes);

	// calculate normals
	MakeMeshNormals( width, height, ctrl );
	MakeMeshTangentVectors(width, height, ctrl, numIndexes, indexes);

	VectorCopy(grid->lodOrigin, lodOrigin);
	lodRadius = grid->lodRadius;
	// free the old grid
	R_FreeSurfaceGridMeshData(grid);
	// create a new grid
	R_CreateSurfaceGridMesh(grid, width, height, ctrl, errorTable, numIndexes, indexes);
	grid->lodRadius = lodRadius;
	VectorCopy(lodOrigin, grid->lodOrigin);
}
