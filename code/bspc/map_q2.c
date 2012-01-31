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

//===========================================================================
// ANSI, Area Navigational System Interface
// AAS,  Area Awareness System
//===========================================================================

#include "qbsp.h"
#include "l_mem.h"
#include "../botlib/aasfile.h"			//aas_bbox_t
#include "aas_store.h"		//AAS_MAX_BBOXES
#include "aas_cfg.h"
#include "aas_map.h"			//AAS_CreateMapBrushes
#include "l_bsp_q2.h"


#ifdef ME

#define NODESTACKSIZE		1024

int nodestack[NODESTACKSIZE];
int *nodestackptr;
int nodestacksize = 0;
int brushmodelnumbers[MAX_MAPFILE_BRUSHES];
int dbrushleafnums[MAX_MAPFILE_BRUSHES];
int dplanes2mapplanes[MAX_MAPFILE_PLANES];

#endif //ME

//====================================================================

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_CreateMapTexinfo(void)
{
	int i;

	for (i = 0; i < numtexinfo; i++)
	{
		memcpy(map_texinfo[i].vecs, texinfo[i].vecs, sizeof(float) * 2 * 4);
		map_texinfo[i].flags = texinfo[i].flags;
		map_texinfo[i].value = texinfo[i].value;
		strcpy(map_texinfo[i].texture, texinfo[i].texture);
		map_texinfo[i].nexttexinfo = 0;
	} //end for
} //end of the function Q2_CreateMapTexinfo

/*
===========
Q2_BrushContents
===========
*/
int	Q2_BrushContents (mapbrush_t *b)
{
	int			contents;
	side_t		*s;
	int			i;
	int			trans;

	s = &b->original_sides[0];
	contents = s->contents;
	trans = texinfo[s->texinfo].flags;
	for (i = 1; i < b->numsides; i++, s++)
	{
		s = &b->original_sides[i];
		trans |= texinfo[s->texinfo].flags;
		if (s->contents != contents)
		{
			Log_Print("Entity %i, Brush %i: mixed face contents\n"
				, b->entitynum, b->brushnum);
			Log_Print("texture name = %s\n", texinfo[s->texinfo].texture);
			break;
		}
	}

	// if any side is translucent, mark the contents
	// and change solid to window
	if ( trans & (SURF_TRANS33|SURF_TRANS66) )
	{
		contents |= CONTENTS_Q2TRANSLUCENT;
		if (contents & CONTENTS_SOLID)
		{
			contents &= ~CONTENTS_SOLID;
			contents |= CONTENTS_WINDOW;
		}
	}

	return contents;
}

#ifdef ME

#define BBOX_NORMAL_EPSILON			0.0001

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void MakeAreaPortalBrush(mapbrush_t *brush)
{
	int sn;
	side_t *s;

	brush->contents = CONTENTS_AREAPORTAL;

	for (sn = 0; sn < brush->numsides; sn++)
	{
		s = brush->original_sides + sn;
		//make sure the surfaces are not hint or skip
		s->surf &= ~(SURF_HINT|SURF_SKIP);
		//
		s->texinfo = 0;
		s->contents = CONTENTS_AREAPORTAL;
	} //end for
} //end of the function MakeAreaPortalBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void DPlanes2MapPlanes(void)
{
	int i;

	for (i = 0; i < numplanes; i++)
	{
		dplanes2mapplanes[i] = FindFloatPlane(dplanes[i].normal, dplanes[i].dist);
	} //end for
} //end of the function DPlanes2MapPlanes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void MarkVisibleBrushSides(mapbrush_t *brush)
{
	int n, i, planenum;
	side_t *side;
	dface_t *face;
	//
	for (n = 0; n < brush->numsides; n++)
	{
		side = brush->original_sides + n;
		//if this side is a bevel or the leaf number of the brush is unknown
		if ((side->flags & SFL_BEVEL) || brush->leafnum < 0)
		{
			//this side is a valid splitter
			side->flags |= SFL_VISIBLE;
			continue;
		} //end if
		//assum this side will not be used as a splitter
		side->flags &= ~SFL_VISIBLE;
		//check if the side plane is used by a visible face
		for (i = 0; i < numfaces; i++)
		{
			face = &dfaces[i];
			planenum = dplanes2mapplanes[face->planenum];
			if ((planenum & ~1) == (side->planenum & ~1))
			{
				//this side is a valid splitter
				side->flags |= SFL_VISIBLE;
			} //end if
		} //end for
	} //end for
} //end of the function MarkVisibleBrushSides

#endif //ME

/*
=================
Q2_ParseBrush
=================
*/
void Q2_ParseBrush (script_t *script, entity_t *mapent)
{
	mapbrush_t *b;
	int i, j, k;
	int mt;
	side_t *side, *s2;
	int planenum;
	brush_texture_t td;
	int planepts[3][3];
	token_t token;

	if (nummapbrushes >= MAX_MAPFILE_BRUSHES)
		Error ("nummapbrushes == MAX_MAPFILE_BRUSHES");

	b = &mapbrushes[nummapbrushes];
	b->original_sides = &brushsides[nummapbrushsides];
	b->entitynum = num_entities-1;
	b->brushnum = nummapbrushes - mapent->firstbrush;
	b->leafnum = -1;

	do
	{
		if (!PS_ReadToken(script, &token))
			break;
		if (!strcmp(token.string, "}") )
			break;

		//IDBUG: mixed use of MAX_MAPFILE_? and MAX_MAP_? this could
		//			lead to out of bound indexing of the arrays
		if (nummapbrushsides >= MAX_MAPFILE_BRUSHSIDES)
			Error ("MAX_MAPFILE_BRUSHSIDES");
		side = &brushsides[nummapbrushsides];

		//read the three point plane definition
		for (i = 0; i < 3; i++)
		{
			if (i != 0) PS_ExpectTokenString(script, "(");
			for (j = 0; j < 3; j++)
			{
				PS_ExpectAnyToken(script, &token);
				planepts[i][j] = atof(token.string);
			} //end for
			PS_ExpectTokenString(script, ")");
		} //end for

		//
		//read the texturedef
		//
		PS_ExpectAnyToken(script, &token);
		strcpy(td.name, token.string);

		PS_ExpectAnyToken(script, &token);
		td.shift[0] = atol(token.string);
		PS_ExpectAnyToken(script, &token);
		td.shift[1] = atol(token.string);
		PS_ExpectAnyToken(script, &token);
		td.rotate = atol(token.string);
		PS_ExpectAnyToken(script, &token);
		td.scale[0] = atof(token.string);
		PS_ExpectAnyToken(script, &token);
		td.scale[1] = atof(token.string);

		//find default flags and values
		mt = FindMiptex (td.name);
		td.flags = textureref[mt].flags;
		td.value = textureref[mt].value;
		side->contents = textureref[mt].contents;
		side->surf = td.flags = textureref[mt].flags;

		//check if there's a number available
		if (PS_CheckTokenType(script, TT_NUMBER, 0, &token))
		{
			side->contents = token.intvalue;
			PS_ExpectTokenType(script, TT_NUMBER, 0, &token);
			side->surf = td.flags = token.intvalue;
			PS_ExpectTokenType(script, TT_NUMBER, 0, &token);
			td.value = token.intvalue;
		}

		// translucent objects are automatically classified as detail
		if (side->surf & (SURF_TRANS33|SURF_TRANS66) )
			side->contents |= CONTENTS_DETAIL;
		if (side->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
			side->contents |= CONTENTS_DETAIL;
		if (fulldetail)
			side->contents &= ~CONTENTS_DETAIL;
		if (!(side->contents & ((LAST_VISIBLE_CONTENTS-1) 
			| CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_MIST)  ) )
			side->contents |= CONTENTS_SOLID;

		// hints and skips are never detail, and have no content
		if (side->surf & (SURF_HINT|SURF_SKIP) )
		{
			side->contents = 0;
			side->surf &= ~CONTENTS_DETAIL;
		}

#ifdef ME
		//for creating AAS... this side is textured
		side->flags |= SFL_TEXTURED;
#endif //ME
		//
		// find the plane number
		//
		planenum = PlaneFromPoints (planepts[0], planepts[1], planepts[2]);
		if (planenum == -1)
		{
			Log_Print("Entity %i, Brush %i: plane with no normal\n"
				, b->entitynum, b->brushnum);
			continue;
		}

		//
		// see if the plane has been used already
		//
		for (k=0 ; k<b->numsides ; k++)
		{
			s2 = b->original_sides + k;
			if (s2->planenum == planenum)
			{
				Log_Print("Entity %i, Brush %i: duplicate plane\n"
					, b->entitynum, b->brushnum);
				break;
			}
			if ( s2->planenum == (planenum^1) )
			{
				Log_Print("Entity %i, Brush %i: mirrored plane\n"
					, b->entitynum, b->brushnum);
				break;
			}
		}
		if (k != b->numsides)
			continue;		// duplicated

		//
		// keep this side
		//

		side = b->original_sides + b->numsides;
		side->planenum = planenum;
		side->texinfo = TexinfoForBrushTexture (&mapplanes[planenum],
			&td, vec3_origin);

		// save the td off in case there is an origin brush and we
		// have to recalculate the texinfo
		side_brushtextures[nummapbrushsides] = td;

		nummapbrushsides++;
		b->numsides++;
	} while (1);

	// get the content for the entire brush
	b->contents = Q2_BrushContents (b);

#ifdef ME
	if (BrushExists(b))
	{
		c_squattbrushes++;
		b->numsides = 0;
		return;
	} //end if

	if (create_aas)
	{
		//create AAS brushes, and add brush bevels
		AAS_CreateMapBrushes(b, mapent, true);
		//NOTE: if we return here then duplicate plane errors occur for the non world entities
		return;
	} //end if
#endif //ME

	// allow detail brushes to be removed 
	if (nodetail && (b->contents & CONTENTS_DETAIL) )
	{
		b->numsides = 0;
		return;
	}

	// allow water brushes to be removed
	if (nowater && (b->contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) )
	{
		b->numsides = 0;
		return;
	}

	// create windings for sides and bounds for brush
	MakeBrushWindings (b);

	// brushes that will not be visible at all will never be
	// used as bsp splitters
	if (b->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
	{
		c_clipbrushes++;
		for (i=0 ; i<b->numsides ; i++)
			b->original_sides[i].texinfo = TEXINFO_NODE;
	}

	//
	// origin brushes are removed, but they set
	// the rotation origin for the rest of the brushes
	// in the entity.  After the entire entity is parsed,
	// the planenums and texinfos will be adjusted for
	// the origin brush
	//
	if (b->contents & CONTENTS_ORIGIN)
	{
		char	string[32];
		vec3_t	origin;

		if (num_entities == 1)
		{
			Error ("Entity %i, Brush %i: origin brushes not allowed in world"
				, b->entitynum, b->brushnum);
			return;
		}

		VectorAdd (b->mins, b->maxs, origin);
		VectorScale (origin, 0.5, origin);

		sprintf (string, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue (&entities[b->entitynum], "origin", string);

		VectorCopy (origin, entities[b->entitynum].origin);

		// don't keep this brush
		b->numsides = 0;

		return;
	}

	AddBrushBevels(b);

	nummapbrushes++;
	mapent->numbrushes++;		
}

/*
================
Q2_MoveBrushesToWorld

Takes all of the brushes from the current entity and
adds them to the world's brush list.

Used by func_group and func_areaportal
================
*/
void Q2_MoveBrushesToWorld (entity_t *mapent)
{
	int			newbrushes;
	int			worldbrushes;
	mapbrush_t	*temp;
	int			i;

	// this is pretty gross, because the brushes are expected to be
	// in linear order for each entity

	newbrushes = mapent->numbrushes;
	worldbrushes = entities[0].numbrushes;

	temp = GetMemory(newbrushes*sizeof(mapbrush_t));
	memcpy (temp, mapbrushes + mapent->firstbrush, newbrushes*sizeof(mapbrush_t));

#if	0		// let them keep their original brush numbers
	for (i=0 ; i<newbrushes ; i++)
		temp[i].entitynum = 0;
#endif

	// make space to move the brushes (overlapped copy)
	memmove (mapbrushes + worldbrushes + newbrushes,
		mapbrushes + worldbrushes,
		sizeof(mapbrush_t) * (nummapbrushes - worldbrushes - newbrushes) );

	// copy the new brushes down
	memcpy (mapbrushes + worldbrushes, temp, sizeof(mapbrush_t) * newbrushes);

	// fix up indexes
	entities[0].numbrushes += newbrushes;
	for (i=1 ; i<num_entities ; i++)
		entities[i].firstbrush += newbrushes;
	FreeMemory(temp);

	mapent->numbrushes = 0;
}

/*
================
Q2_ParseMapEntity
================
*/
qboolean	Q2_ParseMapEntity(script_t *script)
{
	entity_t	*mapent;
	epair_t *e;
	side_t *s;
	int i, j;
	int startbrush, startsides;
	vec_t newdist;
	mapbrush_t *b;
	token_t token;

	if (!PS_ReadToken(script, &token)) return false;

	if (strcmp(token.string, "{") )
		Error ("ParseEntity: { not found");
	
	if (num_entities == MAX_MAP_ENTITIES)
		Error ("num_entities == MAX_MAP_ENTITIES");

	startbrush = nummapbrushes;
	startsides = nummapbrushsides;

	mapent = &entities[num_entities];
	num_entities++;
	memset (mapent, 0, sizeof(*mapent));
	mapent->firstbrush = nummapbrushes;
	mapent->numbrushes = 0;
//	mapent->portalareas[0] = -1;
//	mapent->portalareas[1] = -1;

	do
	{
		if (!PS_ReadToken(script, &token))
		{
			Error("ParseEntity: EOF without closing brace");
		} //end if
		if (!strcmp(token.string, "}")) break;
		if (!strcmp(token.string, "{"))
		{
			Q2_ParseBrush(script, mapent);
		} //end if
		else
		{
			PS_UnreadLastToken(script);
			e = ParseEpair(script);
			e->next = mapent->epairs;
			mapent->epairs = e;
		} //end else
	} while(1);

	GetVectorForKey(mapent, "origin", mapent->origin);

	//
	// if there was an origin brush, offset all of the planes and texinfo
	//
	if (mapent->origin[0] || mapent->origin[1] || mapent->origin[2])
	{
		for (i=0 ; i<mapent->numbrushes ; i++)
		{
			b = &mapbrushes[mapent->firstbrush + i];
			for (j=0 ; j<b->numsides ; j++)
			{
				s = &b->original_sides[j];
				newdist = mapplanes[s->planenum].dist -
					DotProduct (mapplanes[s->planenum].normal, mapent->origin);
				s->planenum = FindFloatPlane (mapplanes[s->planenum].normal, newdist);
				s->texinfo = TexinfoForBrushTexture (&mapplanes[s->planenum],
					&side_brushtextures[s-brushsides], mapent->origin);
			}
			MakeBrushWindings (b);
		}
	}

	// group entities are just for editor convenience
	// toss all brushes into the world entity
	if (!strcmp ("func_group", ValueForKey (mapent, "classname")))
	{
		Q2_MoveBrushesToWorld (mapent);
		mapent->numbrushes = 0;
		return true;
	}

	// areaportal entities move their brushes, but don't eliminate
	// the entity
	if (!strcmp ("func_areaportal", ValueForKey (mapent, "classname")))
	{
		char	str[128];

		if (mapent->numbrushes != 1)
			Error ("Entity %i: func_areaportal can only be a single brush", num_entities-1);

		b = &mapbrushes[nummapbrushes-1];
		b->contents = CONTENTS_AREAPORTAL;
		c_areaportals++;
		mapent->areaportalnum = c_areaportals;
		// set the portal number as "style"
		sprintf (str, "%i", c_areaportals);
		SetKeyValue (mapent, "style", str);
		Q2_MoveBrushesToWorld (mapent);
		return true;
	}

	return true;
}

//===================================================================

/*
================
LoadMapFile
================
*/
void Q2_LoadMapFile(char *filename)
{		
	int i;
	script_t *script;

	Log_Print("-- Q2_LoadMapFile --\n");
#ifdef ME
	//loaded map type
	loadedmaptype = MAPTYPE_QUAKE2;
	//reset the map loading
	ResetMapLoading();
#endif //ME

	script = LoadScriptFile(filename);
	if (!script)
	{
		Log_Print("couldn't open %s\n", filename);
		return;
	} //end if
	//white spaces and escape characters inside a string are not allowed
	SetScriptFlags(script, SCFL_NOSTRINGWHITESPACES |
									SCFL_NOSTRINGESCAPECHARS |
									SCFL_PRIMITIVE);

	nummapbrushsides = 0;
	num_entities = 0;
	
	while (Q2_ParseMapEntity(script))
	{
	}

	ClearBounds (map_mins, map_maxs);
	for (i=0 ; i<entities[0].numbrushes ; i++)
	{
		if (mapbrushes[i].mins[0] > 4096)
			continue;	// no valid points
		AddPointToBounds (mapbrushes[i].mins, map_mins, map_maxs);
		AddPointToBounds (mapbrushes[i].maxs, map_mins, map_maxs);
	} //end for

	PrintMapInfo();

	//free the script
	FreeScript(script);
//	TestExpandBrushes ();
	//
	Q2_CreateMapTexinfo();
} //end of the function Q2_LoadMapFile

#ifdef ME		//Begin MAP loading from BSP file
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_SetLeafBrushesModelNumbers(int leafnum, int modelnum)
{
	int i, brushnum;
	dleaf_t *leaf;

	leaf = &dleafs[leafnum];
	for (i = 0; i < leaf->numleafbrushes; i++)
	{
		brushnum = dleafbrushes[leaf->firstleafbrush + i];
		brushmodelnumbers[brushnum] = modelnum;
		dbrushleafnums[brushnum] = leafnum;
	} //end for
} //end of the function Q2_SetLeafBrushesModelNumbers
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_InitNodeStack(void)
{
	nodestackptr = nodestack;
	nodestacksize = 0;
} //end of the function Q2_InitNodeStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_PushNodeStack(int num)
{
	*nodestackptr = num;
	nodestackptr++;
	nodestacksize++;
	//
	if (nodestackptr >= &nodestack[NODESTACKSIZE])
	{
		Error("Q2_PushNodeStack: stack overflow\n");
	} //end if
} //end of the function Q2_PushNodeStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Q2_PopNodeStack(void)
{
	//if the stack is empty
	if (nodestackptr <= nodestack) return -1;
	//decrease stack pointer
	nodestackptr--;
	nodestacksize--;
	//return the top value from the stack
	return *nodestackptr;
} //end of the function Q2_PopNodeStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_SetBrushModelNumbers(entity_t *mapent)
{
	int n, pn;
	int leafnum;

	//
	Q2_InitNodeStack();
	//head node (root) of the bsp tree
	n = dmodels[mapent->modelnum].headnode;
	pn = 0;
	
	do
	{
		//if we are in a leaf (negative node number)
		if (n < 0)
		{
			//number of the leaf
			leafnum = (-n) - 1;
			//set the brush numbers
			Q2_SetLeafBrushesModelNumbers(leafnum, mapent->modelnum);
			//walk back into the tree to find a second child to continue with
			for (pn = Q2_PopNodeStack(); pn >= 0; n = pn, pn = Q2_PopNodeStack())
			{
				//if we took the first child at the parent node
				if (dnodes[pn].children[0] == n) break;
			} //end for
			//if the stack wasn't empty (if not processed whole tree)
			if (pn >= 0)
			{
				//push the parent node again
				Q2_PushNodeStack(pn);
				//we proceed with the second child of the parent node
				n = dnodes[pn].children[1];
			} //end if
		} //end if
		else
		{
			//push the current node onto the stack
			Q2_PushNodeStack(n);
			//walk forward into the tree to the first child
			n = dnodes[n].children[0];
		} //end else
	} while(pn >= 0);
} //end of the function Q2_SetBrushModelNumbers
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_BSPBrushToMapBrush(dbrush_t *bspbrush, entity_t *mapent)
{
	mapbrush_t *b;
	int i, k, n;
	side_t *side, *s2;
	int planenum;
	dbrushside_t *bspbrushside;
	dplane_t *bspplane;

	if (nummapbrushes >= MAX_MAPFILE_BRUSHES)
		Error ("nummapbrushes >= MAX_MAPFILE_BRUSHES");

	b = &mapbrushes[nummapbrushes];
	b->original_sides = &brushsides[nummapbrushsides];
	b->entitynum = mapent-entities;
	b->brushnum = nummapbrushes - mapent->firstbrush;
	b->leafnum = dbrushleafnums[bspbrush - dbrushes];

	for (n = 0; n < bspbrush->numsides; n++)
	{
		//pointer to the bsp brush side
		bspbrushside = &dbrushsides[bspbrush->firstside + n];

		if (nummapbrushsides >= MAX_MAPFILE_BRUSHSIDES)
		{
			Error ("MAX_MAPFILE_BRUSHSIDES");
		} //end if
		//pointer to the map brush side
		side = &brushsides[nummapbrushsides];
		//if the BSP brush side is textured
		if (brushsidetextured[bspbrush->firstside + n]) side->flags |= SFL_TEXTURED;
		else side->flags &= ~SFL_TEXTURED;
		//ME: can get side contents and surf directly from BSP file
		side->contents = bspbrush->contents;
		//if the texinfo is TEXINFO_NODE
		if (bspbrushside->texinfo < 0) side->surf = 0;
		else side->surf = texinfo[bspbrushside->texinfo].flags;

		// translucent objects are automatically classified as detail
		if (side->surf & (SURF_TRANS33|SURF_TRANS66) )
			side->contents |= CONTENTS_DETAIL;
		if (side->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
			side->contents |= CONTENTS_DETAIL;
		if (fulldetail)
			side->contents &= ~CONTENTS_DETAIL;
		if (!(side->contents & ((LAST_VISIBLE_CONTENTS-1) 
			| CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_MIST)  ) )
			side->contents |= CONTENTS_SOLID;

		// hints and skips are never detail, and have no content
		if (side->surf & (SURF_HINT|SURF_SKIP) )
		{
			side->contents = 0;
			side->surf &= ~CONTENTS_DETAIL;
		}

		//ME: get a plane for this side
		bspplane = &dplanes[bspbrushside->planenum];
		planenum = FindFloatPlane(bspplane->normal, bspplane->dist);
		//
		// see if the plane has been used already
		//
		//ME: this really shouldn't happen!!!
		//ME: otherwise the bsp file is corrupted??
		//ME: still it seems to happen, maybe Johny Boy's
		//ME: brush bevel adding is crappy ?
		for (k = 0; k < b->numsides; k++)
		{
			s2 = b->original_sides + k;
//			if (DotProduct (mapplanes[s2->planenum].normal, mapplanes[planenum].normal) > 0.999
//							&& fabs(mapplanes[s2->planenum].dist - mapplanes[planenum].dist) < 0.01 )

			if (s2->planenum == planenum)
			{
				Log_Print("Entity %i, Brush %i: duplicate plane\n"
					, b->entitynum, b->brushnum);
				break;
			}
			if ( s2->planenum == (planenum^1) )
			{
				Log_Print("Entity %i, Brush %i: mirrored plane\n"
					, b->entitynum, b->brushnum);
				break;
			}
		}
		if (k != b->numsides)
			continue;		// duplicated

		//
		// keep this side
		//
		//ME: reset pointer to side, why? hell I dunno (pointer is set above already)
		side = b->original_sides + b->numsides;
		//ME: store the plane number
		side->planenum = planenum;
		//ME: texinfo is already stored when bsp is loaded
		//NOTE: check for TEXINFO_NODE, otherwise crash in Q2_BrushContents
		if (bspbrushside->texinfo < 0) side->texinfo = 0;
		else side->texinfo = bspbrushside->texinfo;

		// save the td off in case there is an origin brush and we
		// have to recalculate the texinfo
		// ME: don't need to recalculate because it's already done
		//     (for non-world entities) in the BSP file
//		side_brushtextures[nummapbrushsides] = td;

		nummapbrushsides++;
		b->numsides++;
	} //end for

	// get the content for the entire brush
	b->contents = bspbrush->contents;
	Q2_BrushContents(b);

	if (BrushExists(b))
	{
		c_squattbrushes++;
		b->numsides = 0;
		return;
	} //end if

	//if we're creating AAS
	if (create_aas)
	{
		//create the AAS brushes from this brush, don't add brush bevels
		AAS_CreateMapBrushes(b, mapent, false);
		return;
	} //end if

	// allow detail brushes to be removed 
	if (nodetail && (b->contents & CONTENTS_DETAIL) )
	{
		b->numsides = 0;
		return;
	} //end if

	// allow water brushes to be removed
	if (nowater && (b->contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER)) )
	{
		b->numsides = 0;
		return;
	} //end if

	// create windings for sides and bounds for brush
	MakeBrushWindings(b);

	//mark brushes without winding or with a tiny window as bevels
	MarkBrushBevels(b);

	// brushes that will not be visible at all will never be
	// used as bsp splitters
	if (b->contents & (CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP) )
	{
			c_clipbrushes++;
		for (i = 0; i < b->numsides; i++)
			b->original_sides[i].texinfo = TEXINFO_NODE;
	} //end for

	//
	// origin brushes are removed, but they set
	// the rotation origin for the rest of the brushes
	// in the entity.  After the entire entity is parsed,
	// the planenums and texinfos will be adjusted for
	// the origin brush
	//
	//ME: not needed because the entities in the BSP file already
	//    have an origin set
//	if (b->contents & CONTENTS_ORIGIN)
//	{
//		char	string[32];
//		vec3_t	origin;
//
//		if (num_entities == 1)
//		{
//			Error ("Entity %i, Brush %i: origin brushes not allowed in world"
//				, b->entitynum, b->brushnum);
//			return;
//		}
//
//		VectorAdd (b->mins, b->maxs, origin);
//		VectorScale (origin, 0.5, origin);
//
//		sprintf (string, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
//		SetKeyValue (&entities[b->entitynum], "origin", string);
//
//		VectorCopy (origin, entities[b->entitynum].origin);
//
//		// don't keep this brush
//		b->numsides = 0;
//
//		return;
//	}

	//ME: the bsp brushes already have bevels, so we won't try to
	//    add them again (especially since Johny Boy's bevel adding might
	//    be crappy)
//	AddBrushBevels(b);

	nummapbrushes++;
	mapent->numbrushes++;
} //end of the function Q2_BSPBrushToMapBrush
//===========================================================================
//===========================================================================
void Q2_ParseBSPBrushes(entity_t *mapent)
{
	int i;

	//give all the brushes that belong to this entity the number of the
	//BSP model used by this entity
	Q2_SetBrushModelNumbers(mapent);
	//now parse all the brushes with the correct mapent->modelnum
	for (i = 0; i < numbrushes; i++)
	{
		if (brushmodelnumbers[i] == mapent->modelnum)
		{
			Q2_BSPBrushToMapBrush(&dbrushes[i], mapent);
		} //end if
	} //end for
} //end of the function Q2_ParseBSPBrushes
//===========================================================================
//===========================================================================
qboolean Q2_ParseBSPEntity(int entnum)
{
	entity_t	*mapent;
	char *model;
	int startbrush, startsides;

	startbrush = nummapbrushes;
	startsides = nummapbrushsides;

	mapent = &entities[entnum];//num_entities];
	mapent->firstbrush = nummapbrushes;
	mapent->numbrushes = 0;
	mapent->modelnum = -1;	//-1 = no model

	model = ValueForKey(mapent, "model");
	if (model && strlen(model))
	{
		if (*model != '*')
		{
			Error("Q2_ParseBSPEntity: model number without leading *");
		} //end if
		//get the model number of this entity (skip the leading *)
		mapent->modelnum = atoi(&model[1]);
	} //end if

	GetVectorForKey(mapent, "origin", mapent->origin);

	//if this is the world entity it has model number zero
	//the world entity has no model key
	if (!strcmp("worldspawn", ValueForKey(mapent, "classname")))
	{
		mapent->modelnum = 0;
	} //end if
	//if the map entity has a BSP model (a modelnum of -1 is used for
	//entities that aren't using a BSP model)
	if (mapent->modelnum >= 0)
	{
		//parse the bsp brushes
		Q2_ParseBSPBrushes(mapent);
	} //end if
	//
	//the origin of the entity is already taken into account
	//
	//func_group entities can't be in the bsp file
	//
	//check out the func_areaportal entities
	if (!strcmp ("func_areaportal", ValueForKey (mapent, "classname")))
	{
		c_areaportals++;
		mapent->areaportalnum = c_areaportals;
		return true;
	} //end if
	return true;
} //end of the function Q2_ParseBSPEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Q2_LoadMapFromBSP(char *filename, int offset, int length)
{
	int i;

	Log_Print("-- Q2_LoadMapFromBSP --\n");
	//loaded map type
	loadedmaptype = MAPTYPE_QUAKE2;

	Log_Print("Loading map from %s...\n", filename);
	//load the bsp file
	Q2_LoadBSPFile(filename, offset, length);

	//create an index from bsp planes to map planes
	//DPlanes2MapPlanes();
	//clear brush model numbers
	for (i = 0; i < MAX_MAPFILE_BRUSHES; i++)
		brushmodelnumbers[i] = -1;

	nummapbrushsides = 0;
	num_entities = 0;

	Q2_ParseEntities();
	//
	for (i = 0; i < num_entities; i++)
	{
		Q2_ParseBSPEntity(i);
	} //end for

	//get the map mins and maxs from the world model
	ClearBounds(map_mins, map_maxs);
	for (i = 0; i < entities[0].numbrushes; i++)
	{
		if (mapbrushes[i].mins[0] > 4096)
			continue;	//no valid points
		AddPointToBounds (mapbrushes[i].mins, map_mins, map_maxs);
		AddPointToBounds (mapbrushes[i].maxs, map_mins, map_maxs);
	} //end for

	PrintMapInfo();
	//
	Q2_CreateMapTexinfo();
} //end of the function Q2_LoadMapFromBSP

void Q2_ResetMapLoading(void)
{
	//reset for map loading from bsp
	memset(nodestack, 0, NODESTACKSIZE * sizeof(int));
	nodestackptr = NULL;
	nodestacksize = 0;
	memset(brushmodelnumbers, 0, MAX_MAPFILE_BRUSHES * sizeof(int));
} //end of the function Q2_ResetMapLoading

//End MAP loading from BSP file
#endif //ME

//====================================================================

/*
================
TestExpandBrushes

Expands all the brush planes and saves a new map out
================
*/
void TestExpandBrushes (void)
{
	FILE	*f;
	side_t	*s;
	int		i, j, bn;
	winding_t	*w;
	char	*name = "expanded.map";
	mapbrush_t	*brush;
	vec_t	dist;

	Log_Print("writing %s\n", name);
	f = fopen (name, "wb");
	if (!f)
		Error ("Can't write %s\n", name);

	fprintf (f, "{\n\"classname\" \"worldspawn\"\n");

	for (bn=0 ; bn<nummapbrushes ; bn++)
	{
		brush = &mapbrushes[bn];
		fprintf (f, "{\n");
		for (i=0 ; i<brush->numsides ; i++)
		{
			s = brush->original_sides + i;
			dist = mapplanes[s->planenum].dist;
			for (j=0 ; j<3 ; j++)
				dist += fabs( 16 * mapplanes[s->planenum].normal[j] );

			w = BaseWindingForPlane (mapplanes[s->planenum].normal, dist);

			fprintf (f,"( %i %i %i ) ", (int)w->p[0][0], (int)w->p[0][1], (int)w->p[0][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[1][0], (int)w->p[1][1], (int)w->p[1][2]);
			fprintf (f,"( %i %i %i ) ", (int)w->p[2][0], (int)w->p[2][1], (int)w->p[2][2]);

			fprintf (f, "%s 0 0 0 1 1\n", texinfo[s->texinfo].texture);
			FreeWinding (w);
		}
		fprintf (f, "}\n");
	}
	fprintf (f, "}\n");

	fclose (f);

	Error ("can't proceed after expanding brushes");
} //end of the function TestExpandBrushes

