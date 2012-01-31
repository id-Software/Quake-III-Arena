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
#include "l_bsp_q2.h"

int nummiptex;
textureref_t textureref[MAX_MAP_TEXTURES];

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int FindMiptex (char *name)
{
	int i;
	char path[1024];
	miptex_t	*mt;

	for (i = 0; i < nummiptex; i++)
	{
		if (!strcmp (name, textureref[i].name))
		{
			return i;
		} //end if
	} //end for
	if (nummiptex == MAX_MAP_TEXTURES)
		Error ("MAX_MAP_TEXTURES");
	strcpy (textureref[i].name, name);

	// load the miptex to get the flags and values
	sprintf (path, "%stextures/%s.wal", gamedir, name);
	if (TryLoadFile (path, (void **)&mt) != -1)
	{
		textureref[i].value = LittleLong (mt->value);
		textureref[i].flags = LittleLong (mt->flags);
		textureref[i].contents = LittleLong (mt->contents);
		strcpy (textureref[i].animname, mt->animname);
		FreeMemory(mt);
	} //end if
	nummiptex++;

	if (textureref[i].animname[0])
		FindMiptex (textureref[i].animname);

	return i;
} //end of the function FindMipTex
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
vec3_t	baseaxis[18] =
{
{0,0,1}, {1,0,0}, {0,-1,0},		// floor
{0,0,-1}, {1,0,0}, {0,-1,0},		// ceiling
{1,0,0}, {0,1,0}, {0,0,-1},		// west wall
{-1,0,0}, {0,1,0}, {0,0,-1},		// east wall
{0,1,0}, {1,0,0}, {0,0,-1},		// south wall
{0,-1,0}, {1,0,0}, {0,0,-1}		// north wall
};

void TextureAxisFromPlane(plane_t *pln, vec3_t xv, vec3_t yv)
{
	int		bestaxis;
	vec_t	dot,best;
	int		i;
	
	best = 0;
	bestaxis = 0;
	
	for (i=0 ; i<6 ; i++)
	{
		dot = DotProduct (pln->normal, baseaxis[i*3]);
		if (dot > best)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
	VectorCopy (baseaxis[bestaxis*3+1], xv);
	VectorCopy (baseaxis[bestaxis*3+2], yv);
} //end of the function TextureAxisFromPlane
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int TexinfoForBrushTexture(plane_t *plane, brush_texture_t *bt, vec3_t origin)
{
	vec3_t	vecs[2];
	int		sv, tv;
	vec_t	ang, sinv, cosv;
	vec_t	ns, nt;
	texinfo_t	tx, *tc;
	int		i, j, k;
	float	shift[2];
	brush_texture_t		anim;
	int				mt;

	if (!bt->name[0])
		return 0;

	memset (&tx, 0, sizeof(tx));
	strcpy (tx.texture, bt->name);

	TextureAxisFromPlane(plane, vecs[0], vecs[1]);

	shift[0] = DotProduct (origin, vecs[0]);
	shift[1] = DotProduct (origin, vecs[1]);

	if (!bt->scale[0])
		bt->scale[0] = 1;
	if (!bt->scale[1])
		bt->scale[1] = 1;


// rotate axis
	if (bt->rotate == 0)
		{ sinv = 0 ; cosv = 1; }
	else if (bt->rotate == 90)
		{ sinv = 1 ; cosv = 0; }
	else if (bt->rotate == 180)
		{ sinv = 0 ; cosv = -1; }
	else if (bt->rotate == 270)
		{ sinv = -1 ; cosv = 0; }
	else
	{	
		ang = bt->rotate / 180 * Q_PI;
		sinv = sin(ang);
		cosv = cos(ang);
	}

	if (vecs[0][0])
		sv = 0;
	else if (vecs[0][1])
		sv = 1;
	else
		sv = 2;
				
	if (vecs[1][0])
		tv = 0;
	else if (vecs[1][1])
		tv = 1;
	else
		tv = 2;
					
	for (i=0 ; i<2 ; i++)
	{
		ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
		nt = sinv * vecs[i][sv] +  cosv * vecs[i][tv];
		vecs[i][sv] = ns;
		vecs[i][tv] = nt;
	}

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			tx.vecs[i][j] = vecs[i][j] / bt->scale[i];

	tx.vecs[0][3] = bt->shift[0] + shift[0];
	tx.vecs[1][3] = bt->shift[1] + shift[1];
	tx.flags = bt->flags;
	tx.value = bt->value;

	//
	// find the texinfo
	//
	tc = texinfo;
	for (i=0 ; i<numtexinfo ; i++, tc++)
	{
		if (tc->flags != tx.flags)
			continue;
		if (tc->value != tx.value)
			continue;
		for (j=0 ; j<2 ; j++)
		{
			if (strcmp (tc->texture, tx.texture))
				goto skip;
			for (k=0 ; k<4 ; k++)
			{
				if (tc->vecs[j][k] != tx.vecs[j][k])
					goto skip;
			}
		}
		return i;
skip:;
	}
	*tc = tx;
	numtexinfo++;

	// load the next animation
	mt = FindMiptex (bt->name);
	if (textureref[mt].animname[0])
	{
		anim = *bt;
		strcpy (anim.name, textureref[mt].animname);
		tc->nexttexinfo = TexinfoForBrushTexture (plane, &anim, origin);
	}
	else
		tc->nexttexinfo = -1;


	return i;
} //end of the function TexinfoForBrushTexture
