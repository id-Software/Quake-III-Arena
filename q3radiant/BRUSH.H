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

// brush.h

// all types moved to qertypes.h
//--typedef struct
//--{
//--	int		numpoints;
//--	int		maxpoints;
//--	float 	points[8][5];			// variable sized
//--} winding_t;


// the normals on planes point OUT of the brush
//--#define	MAXPOINTS	16
//--typedef struct face_s
//--{
//--	struct face_s	*next;
//--	vec3_t		planepts[3];
//--  texdef_t	texdef;
//--  plane_t		plane;
//--
//--	winding_t  *face_winding;
//--
//--	vec3_t		d_color;
//--	qtexture_t *d_texture;
//--
//--} face_t;
//--
//--typedef struct {
//--	vec3_t	xyz;
//--	float	sideST[2];
//--	float	capST[2];
//--} curveVertex_t;
//--
//--typedef struct {
//--	curveVertex_t	v[2];
//--} sideVertex_t;
//--
//--typedef struct brush_s
//--{
//--	struct brush_s	*prev, *next;	// links in active/selected
//--	struct brush_s	*oprev, *onext;	// links in entity
//--	struct entity_s	*owner;
//--	vec3_t	mins, maxs;
//--	face_t     *brush_faces;
//--
//--	qboolean bModelFailed;
//--	//
//--	// curve brush extensions
//--	// all are derived from brush_faces
//--	qboolean	curveBrush;
//--	qboolean	patchBrush;
//--    int nPatchID;
//--} brush_t;

void		Brush_AddToList (brush_t *b, brush_t *list);
void		Brush_Build(brush_t *b, bool bSnap = true, bool bMarkMap = true, bool bConvert = false);
void		Brush_BuildWindings( brush_t *b, bool bSnap = true );
brush_t*	Brush_Clone (brush_t *b);
brush_t*	Brush_FullClone(brush_t *b);
brush_t*	Brush_Create (vec3_t mins, vec3_t maxs, texdef_t *texdef);
void		Brush_Draw( brush_t *b );
void		Brush_DrawXY(brush_t *b, int nViewType);
// set bRemoveNode to false to avoid trying to delete the item in group view tree control
void		Brush_Free (brush_t *b, bool bRemoveNode = true);
int			Brush_MemorySize(brush_t *b);
void		Brush_MakeSided (int sides);
void		Brush_MakeSidedCone (int sides);
void		Brush_Move (brush_t *b, const vec3_t move, bool bSnap = true);
int			Brush_MoveVertex(brush_t *b, vec3_t vertex, vec3_t delta, vec3_t end, bool bSnap = true);
void		Brush_ResetFaceOriginals(brush_t *b);
brush_t*	Brush_Parse (void);
face_t*		Brush_Ray (vec3_t origin, vec3_t dir, brush_t *b, float *dist);
void		Brush_RemoveFromList (brush_t *b);
void		Brush_SplitBrushByFace (brush_t *in, face_t *f, brush_t **front, brush_t **back);
void		Brush_SelectFaceForDragging (brush_t *b, face_t *f, qboolean shear);
void		Brush_SetTexture (brush_t *b, texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale = false, IPluginTexdef* pPlugTexdef=NULL );
void		Brush_SideSelect (brush_t *b, vec3_t origin, vec3_t dir, qboolean shear);
void		Brush_SnapToGrid(brush_t *pb);
void		Brush_Rotate(brush_t *b, vec3_t vAngle, vec3_t vOrigin, bool bBuild = true);
void		Brush_MakeSidedSphere(int sides);
void		Brush_Write (brush_t *b, FILE *f);
void		Brush_Write (brush_t *b, CMemFile* pMemFile);
void		Brush_RemoveEmptyFaces ( brush_t *b );
winding_t*	Brush_MakeFaceWinding (brush_t *b, face_t *face);

int			AddPlanept (float *f);
float		SetShadeForPlane (plane_t *p);

face_t*		Face_Alloc( void );
void		Face_Free( face_t *f );
face_t*		Face_Clone (face_t *f);
void		Face_MakePlane (face_t *f);
void		Face_Draw( face_t *face );
void		Face_TextureVectors (face_t *f, float STfromXYZ[2][4]);
void		SetFaceTexdef (brush_t *b, face_t *f, texdef_t *texdef, brushprimit_texdef_t *brushprimit_texdef, bool bFitScale = false, IPluginTexdef* pPlugTexdef = NULL );

void Face_FitTexture( face_t * face, int nHeight, int nWidth );
void Brush_FitTexture( brush_t *b, int nHeight, int nWidth );
void Brush_SetEpair(brush_t *b, const char *pKey, const char *pValue);
const char* Brush_GetKeyValue(brush_t *b, const char *pKey);
brush_t *Brush_Alloc();
const char* Brush_Name(brush_t *b);
