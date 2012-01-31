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
#ifndef __TERRAIN_H__
#define __TERRAIN_H__

void			Terrain_SetEpair( terrainMesh_t *p, const char *pKey, const char *pValue );
const char*		Terrain_GetKeyValue( terrainMesh_t *p, const char *pKey );
int				Terrain_MemorySize( terrainMesh_t *p );
void			Terrain_Delete( terrainMesh_t *p );
terrainMesh_t	*MakeNewTerrain( int width, int height, qtexture_t *texture = NULL );
brush_t			*AddBrushForTerrain( terrainMesh_t *pm, bool bLinkToWorld = true );
terrainMesh_t	*Terrain_Duplicate( terrainMesh_t *pFrom );
void			Terrain_BrushToMesh( void );
brush_t			*Terrain_Parse( void );
void			Terrain_Write( terrainMesh_t *p, CMemFile *file );
void			Terrain_Write( terrainMesh_t *p, FILE *file );
void			Terrain_Select( terrainMesh_t *p );
void			Terrain_Deselect( terrainMesh_t *p );
void			Terrain_Move( terrainMesh_t *pm, const vec3_t vMove, bool bRebuild = false );
void			UpdateTerrainInspector( void );
void			Terrain_CalcBounds( terrainMesh_t *p, vec3_t &vMin, vec3_t &vMax );
void			CalcTriNormal( const vec3_t a, const vec3_t b, const vec3_t c, vec3_t o );
void			Terrain_CalcVertPos( terrainMesh_t *p, int x, int y, vec3_t vert );
void			Terrain_CalcNormals( terrainMesh_t *p );
void			Terrain_FindReplaceTexture( terrainMesh_t *p, const char *pFind, const char *pReplace, bool bForce );
bool			Terrain_HasTexture( terrainMesh_t *p, const char *name );
void			Terrain_ReplaceQTexture( terrainMesh_t *p, qtexture_t *pOld, qtexture_t *pNew );
void			Terrain_SetTexture( terrainMesh_t *p, texdef_t *tex_def );
void			Terrain_Scale( terrainMesh_t *p, const vec3_t vOrigin, const vec3_t vAmt, bool bRebuild = true );
bool			Terrain_DragScale( terrainMesh_t *p, vec3_t vAmt, vec3_t vMove );
void			Terrain_ApplyMatrix( terrainMesh_t *p, const vec3_t vOrigin, const vec3_t vMatrix[ 3 ], bool bSnap );
void			Terrain_DrawFace( brush_t *brush, terrainFace_t *terraface );
void			DrawTerrain( terrainMesh_t *pm, bool bPoints, bool bShade = false );
void			Terrain_DrawCam( terrainMesh_t *pm );
void			Terrain_DrawXY( terrainMesh_t *pm, entity_t *owner );
bool			OnlyTerrainSelected( void );
bool			AnyTerrainSelected( void );
terrainMesh_t	*SingleTerrainSelected( void );
void			Terrain_Edit( void );
void			Terrain_SelectPointByRay ( vec3_t org, vec3_t dir, int buttons );
void			Terrain_AddMovePoint( vec3_t v, bool bMulti, bool bFull, int buttons );
void			Terrain_UpdateSelected( vec3_t vMove );
int				Terrain_PointInMoveList( terrainVert_t *pf );
void			Terrain_AddPoint( terrainMesh_t *p, terrainVert_t *v );
void			Terrain_RemovePointFromMoveList( terrainVert_t *v );
void			Terrain_SelectAreaPoints( void );
bool			RayTriangleIntersect( vec3_t orig, vec3_t dir, vec3_t vert1, vec3_t vert2, vec3_t vert3, float *t );
terrainFace_t	*Terrain_Ray( vec3_t origin, vec3_t dir, brush_t *b, float *dist );
void			Select_TerrainFacesFromBrush( brush_t *brush );
void			SetTerrainTexdef( brush_t *brush, terrainFace_t *vert, texdef_t *texdef );
void			RotateTerrainFaceTexture( terrainFace_t *vert, int nAxis, float fDeg );
void			TerrainFace_FitTexture( terrainFace_t *vert );
void			Select_TerrainFace( brush_t * brush, terrainFace_t *terraface );
void			Terrain_Init( void );

#endif
