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
//-----------------------------------------------------------------------------
//
//  $Logfile:: /MissionPack/code/bspc/map_sin.c                               $

#include "qbsp.h"
#include "l_bsp_sin.h"
#include "aas_map.h"			//AAS_CreateMapBrushes


//====================================================================


/*
===========
Sin_BrushContents
===========
*/

int Sin_BrushContents(mapbrush_t *b)
{
	int			contents;
	side_t		*s;
	int			i;
#ifdef SIN
	float			trans = 0;
#else
	int			trans;
#endif

	s = &b->original_sides[0];
	contents = s->contents;

#ifdef SIN
	trans = sin_texinfo[s->texinfo].translucence;
#else
	trans = texinfo[s->texinfo].flags;
#endif
	for (i=1 ; i<b->numsides ; i++, s++)
	{
		s = &b->original_sides[i];
#ifdef SIN
		trans += sin_texinfo[s->texinfo].translucence;
#else
		trans |= texinfo[s->texinfo].flags;
#endif
		if (s->contents != contents)
		{
#ifdef SIN
      if ( 
            ( s->contents & CONTENTS_DETAIL && !(contents & CONTENTS_DETAIL) ) ||
            ( !(s->contents & CONTENTS_DETAIL) && contents & CONTENTS_DETAIL ) 
         )
         {
         s->contents |= CONTENTS_DETAIL;
         contents |= CONTENTS_DETAIL;
         continue;
         }
#endif
			printf ("Entity %i, Brush %i: mixed face contents\n"
				, b->entitynum, b->brushnum);
			break;
		}
	}


#ifdef SIN
	if (contents & CONTENTS_FENCE)
	{
//		contents |= CONTENTS_TRANSLUCENT;
		contents |= CONTENTS_DETAIL;
		contents |= CONTENTS_DUMMYFENCE;
		contents &= ~CONTENTS_SOLID;
		contents &= ~CONTENTS_FENCE;
		contents |= CONTENTS_WINDOW;
	}
#endif

	// if any side is translucent, mark the contents
	// and change solid to window
#ifdef SIN
	if ( trans > 0 )
#else
	if ( trans & (SURF_TRANS33|SURF_TRANS66) )
#endif
	{
		contents |= CONTENTS_Q2TRANSLUCENT;
		if (contents & CONTENTS_SOLID)
		{
			contents &= ~CONTENTS_SOLID;
			contents |= CONTENTS_WINDOW;
		}
	}

	return contents;
} //*/


//============================================================================



/*
=================
ParseBrush
=================
* /
void ParseBrush (entity_t *mapent)
{
	mapbrush_t		*b;
	int			i,j, k;
	int			mt;
	side_t		*side, *s2;
	int			planenum;
	brush_texture_t	td;
#ifdef SIN
   textureref_t newref;
#endif
	int			planepts[3][3];

	if (nummapbrushes == MAX_MAP_BRUSHES)
		Error ("nummapbrushes == MAX_MAP_BRUSHES");

	b = &mapbrushes[nummapbrushes];
	b->original_sides = &brushsides[nummapbrushsides];
	b->entitynum = num_entities-1;
	b->brushnum = nummapbrushes - mapent->firstbrush;

	do
	{
		if (!GetToken (true))
			break;
		if (!strcmp (token, "}") )
			break;

		if (nummapbrushsides == MAX_MAP_BRUSHSIDES)
			Error ("MAX_MAP_BRUSHSIDES");
		side = &brushsides[nummapbrushsides];

		// read the three point plane definition
		for (i=0 ; i<3 ; i++)
		{
			if (i != 0)
				GetToken (true);
			if (strcmp (token, "(") )
				Error ("parsing brush");
			
			for (j=0 ; j<3 ; j++)
			{
				GetToken (false);
				planepts[i][j] = atoi(token);
			}
			
			GetToken (false);
			if (strcmp (token, ")") )
				Error ("parsing brush");
				
		}


		//
		// read the texturedef
		//
		GetToken (false);
		strcpy (td.name, token);

		GetToken (false);
		td.shift[0] = atoi(token);
		GetToken (false);
		td.shift[1] = atoi(token);
		GetToken (false);
#ifdef SIN
		td.rotate = atof(token);	
#else
		td.rotate = atoi(token);	
#endif
		GetToken (false);
		td.scale[0] = atof(token);
		GetToken (false);
		td.scale[1] = atof(token);

		// find default flags and values
		mt = FindMiptex (td.name);
#ifdef SIN
      // clear out the masks on newref
      memset(&newref,0,sizeof(newref));
      // copy over the name
      strcpy( newref.name, td.name );

      ParseSurfaceInfo( &newref );
      MergeRefs( &bsp_textureref[mt], &newref, &td.tref );
      side->contents = td.tref.contents;
      side->surf = td.tref.flags;
#else
		td.flags = textureref[mt].flags;
		td.value = textureref[mt].value;
		side->contents = textureref[mt].contents;
		side->surf = td.flags = textureref[mt].flags;

		if (TokenAvailable())
		{
			GetToken (false);
			side->contents = atoi(token);
			GetToken (false);
			side->surf = td.flags = atoi(token);
			GetToken (false);
			td.value = atoi(token);
		}
#endif

		// translucent objects are automatically classified as detail
#ifdef SIN
		if ( td.tref.translucence > 0 )
#else
		if (side->surf & (SURF_TRANS33|SURF_TRANS66) )
#endif
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
#ifndef SIN // I think this is a bug of some kind
			side->surf &= ~CONTENTS_DETAIL;
#endif
		}

		//
		// find the plane number
		//
		planenum = PlaneFromPoints (planepts[0], planepts[1], planepts[2]);
		if (planenum == -1)
		{
			printf ("Entity %i, Brush %i: plane with no normal\n"
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
				printf ("Entity %i, Brush %i: duplicate plane\n"
					, b->entitynum, b->brushnum);
				break;
			}
			if ( s2->planenum == (planenum^1) )
			{
				printf ("Entity %i, Brush %i: mirrored plane\n"
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
#ifdef SIN
		side->texinfo = TexinfoForBrushTexture (&mapplanes[planenum],
			&td, vec3_origin, &newref);
      // 
      // save off lightinfo
      //
		side->lightinfo = LightinfoForBrushTexture ( &td );
#else
		side->texinfo = TexinfoForBrushTexture (&mapplanes[planenum],
			&td, vec3_origin);

#endif

		// save the td off in case there is an origin brush and we
		// have to recalculate the texinfo
		side_brushtextures[nummapbrushsides] = td;
#ifdef SIN
      // save off the merged tref for animating textures
		side_newrefs[nummapbrushsides] = newref;
#endif

		nummapbrushsides++;
		b->numsides++;
	} while (1);

	// get the content for the entire brush
	b->contents = Sin_BrushContents (b);

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

	AddBrushBevels (b);

	nummapbrushes++;
	mapent->numbrushes++;		
} //*/

/*
================
MoveBrushesToWorld

Takes all of the brushes from the current entity and
adds them to the world's brush list.

Used by func_group and func_areaportal
================
* /
void MoveBrushesToWorld (entity_t *mapent)
{
	int			newbrushes;
	int			worldbrushes;
	mapbrush_t	*temp;
	int			i;

	// this is pretty gross, because the brushes are expected to be
	// in linear order for each entity

	newbrushes = mapent->numbrushes;
	worldbrushes = entities[0].numbrushes;

	temp = malloc(newbrushes*sizeof(mapbrush_t));
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
	free (temp);

	mapent->numbrushes = 0;
} //*/

/*
================
ParseMapEntity
================
* /
qboolean	Sin_ParseMapEntity (void)
{
	entity_t	*mapent;
	epair_t		*e;
	side_t		*s;
	int			i, j;
	int			startbrush, startsides;
	vec_t		newdist;
	mapbrush_t	*b;

	if (!GetToken (true))
		return false;

	if (strcmp (token, "{") )
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
		if (!GetToken (true))
			Error ("ParseEntity: EOF without closing brace");
		if (!strcmp (token, "}") )
			break;
		if (!strcmp (token, "{") )
			ParseBrush (mapent);
		else
		{
			e = ParseEpair ();
#ifdef SIN
         //HACK HACK HACK
         // MED Gotta do this here
         if ( !stricmp(e->key, "surfacefile") )
            {
            if (!surfacefile[0])
               {
               strcpy( surfacefile, e->value );
               }
		      printf ("--- ParseSurfaceFile ---\n");
		      printf ("Surface script: %s\n", surfacefile);
		      if (!ParseSurfaceFile(surfacefile))
               {
		         Error ("Script file not found: %s\n", surfacefile);
               }
            }
#endif
			e->next = mapent->epairs;
			mapent->epairs = e;
		}
	} while (1);

#ifdef SIN
    if (!(strlen(ValueForKey(mapent, "origin")))  && ((num_entities-1) != 0))
        {
        mapbrush_t     *brush;
        vec3_t		    origin;
  	    char		    string[32];
        vec3_t          mins, maxs;
        int			    start, end;
        // Calculate bounds

        start = mapent->firstbrush;
	    end = start + mapent->numbrushes;
	    ClearBounds (mins, maxs);

	    for (j=start ; j<end ; j++)
            {
	        brush = &mapbrushes[j];
		    if (!brush->numsides)
			    continue;	// not a real brush (origin brush) - shouldn't happen
		    AddPointToBounds (brush->mins, mins, maxs);
		    AddPointToBounds (brush->maxs, mins, maxs);
            }

        // Set the origin to be the centroid of the entity.
        VectorAdd ( mins, maxs, origin);
		VectorScale( origin, 0.5f, origin );

		sprintf (string, "%i %i %i", (int)origin[0], (int)origin[1], (int)origin[2]);
		SetKeyValue ( mapent, "origin", string);
//        qprintf("Setting origin to %s\n",string);
        }
#endif

	GetVectorForKey (mapent, "origin", mapent->origin);

#ifdef SIN
	if (
         (!strcmp ("func_areaportal", ValueForKey (mapent, "classname"))) ||
         (!strcmp ("func_group", ValueForKey (mapent, "classname"))) ||
      	(!strcmp ("detail", ValueForKey (mapent, "classname")) && !entitydetails)
      )
      {
      VectorClear( mapent->origin );
      }
#endif

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
#ifdef SIN
				s->texinfo = TexinfoForBrushTexture (&mapplanes[s->planenum],
					&side_brushtextures[s-brushsides], mapent->origin, &side_newrefs[s-brushsides]);
            // 
            // save off lightinfo
            //
            s->lightinfo = LightinfoForBrushTexture (	&side_brushtextures[s-brushsides] );
#else
				s->texinfo = TexinfoForBrushTexture (&mapplanes[s->planenum],
					&side_brushtextures[s-brushsides], mapent->origin);
#endif
			}
			MakeBrushWindings (b);
		}
	}

	// group entities are just for editor convenience
	// toss all brushes into the world entity
	if (!strcmp ("func_group", ValueForKey (mapent, "classname")))
	{
		MoveBrushesToWorld (mapent);
		mapent->numbrushes = 0;
		mapent->wasdetail = true;
      FreeValueKeys( mapent );
		return true;
	}
#ifdef SIN
	// detail entities are just for editor convenience
	// toss all brushes into the world entity as detail brushes
	if (!strcmp ("detail", ValueForKey (mapent, "classname")) && !entitydetails)
	{
		for (i=0 ; i<mapent->numbrushes ; i++)
		{
         int j;
         side_t * s;
			b = &mapbrushes[mapent->firstbrush + i];
   	   if (nodetail)
            {
            b->numsides = 0;
            continue;
            }
         if (!fulldetail)
            {
   	      // set the contents for the entire brush
	         b->contents |= CONTENTS_DETAIL;
			   // set the contents in the sides as well
			   for (j=0, s=b->original_sides ; j<b->numsides ; j++,s++)
		   	   {
               s->contents |= CONTENTS_DETAIL;
	   	   	}
            }
         else
            {
   	      // set the contents for the entire brush
	         b->contents |= CONTENTS_SOLID;
			   // set the contents in the sides as well
			   for (j=0, s=b->original_sides ; j<b->numsides ; j++,s++)
		   	   {
               s->contents |= CONTENTS_SOLID;
	   	   	}
            }
		}
		MoveBrushesToWorld (mapent);
		mapent->wasdetail = true;
      FreeValueKeys( mapent );
      // kill off the entity
   	// num_entities--;
		return true;
	}
#endif

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
		MoveBrushesToWorld (mapent);
		return true;
	}

	return true;
} //end of the function Sin_ParseMapEntity */

//===================================================================

/*
================
LoadMapFile
================
* /
void Sin_LoadMapFile (char *filename)
{		
	int		i;
#ifdef SIN
   int num_detailsides=0;
   int num_detailbrushes=0;
   int num_worldsides=0;
   int num_worldbrushes=0;
   int      j,k;
#endif

	qprintf ("--- LoadMapFile ---\n");

	LoadScriptFile (filename);

	nummapbrushsides = 0;
	num_entities = 0;
	
	while (ParseMapEntity ())
	{
	}

	ClearBounds (map_mins, map_maxs);
	for (i=0 ; i<entities[0].numbrushes ; i++)
	{
		if (mapbrushes[i].mins[0] > 4096)
			continue;	// no valid points
		AddPointToBounds (mapbrushes[i].mins, map_mins, map_maxs);
		AddPointToBounds (mapbrushes[i].maxs, map_mins, map_maxs);
	}
#ifdef SIN
   for (j=0;  j<num_entities; j++)
      {
	   for (i=0 ; i<entities[j].numbrushes ; i++)
	      {
         side_t * s;
         mapbrush_t *b;
			b = &mapbrushes[entities[j].firstbrush + i];
         if (b->numsides && b->contents & CONTENTS_DETAIL)
            num_detailbrushes++;
         else if (b->numsides)
            num_worldbrushes++;
			for (k=0, s=b->original_sides ; k<b->numsides ; k++,s++)
			   {
            if (s->contents & CONTENTS_DETAIL)
               num_detailsides++;
            else
               num_worldsides++;
			   }
   	   }
      }
#endif

	qprintf ("%5i brushes\n", nummapbrushes);
	qprintf ("%5i clipbrushes\n", c_clipbrushes);
	qprintf ("%5i total sides\n", nummapbrushsides);
	qprintf ("%5i boxbevels\n", c_boxbevels);
	qprintf ("%5i edgebevels\n", c_edgebevels);
	qprintf ("%5i entities\n", num_entities);
	qprintf ("%5i planes\n", nummapplanes);
	qprintf ("%5i areaportals\n", c_areaportals);
	qprintf ("size: %5.0f,%5.0f,%5.0f to %5.0f,%5.0f,%5.0f\n", map_mins[0],map_mins[1],map_mins[2],
		map_maxs[0],map_maxs[1],map_maxs[2]);
#ifdef SIN
	qprintf ("%5i detailbrushes\n", num_detailbrushes);
	qprintf ("%5i worldbrushes\n", num_worldbrushes);
	qprintf ("%5i detailsides\n", num_detailsides);
	qprintf ("%5i worldsides\n", num_worldsides);
#endif

} //end of the function Sin_LoadMap */


#ifdef ME		//Begin MAP loading from BSP file
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_CreateMapTexinfo(void)
{
	int i;
	vec_t defaultvec[4] = {1, 0, 0, 0};

	memcpy(map_texinfo[0].vecs[0], defaultvec, sizeof(defaultvec));
	memcpy(map_texinfo[0].vecs[1], defaultvec, sizeof(defaultvec));
	map_texinfo[0].flags = 0;
	map_texinfo[0].value = 0;
	strcpy(map_texinfo[0].texture, "generic/misc/red");	//no texture
	map_texinfo[0].nexttexinfo = -1;
	for (i = 1; i < sin_numtexinfo; i++)
	{
		memcpy(map_texinfo[i].vecs, sin_texinfo[i].vecs, sizeof(float) * 2 * 4);
		map_texinfo[i].flags = sin_texinfo[i].flags;
		map_texinfo[i].value = 0;
		strcpy(map_texinfo[i].texture, sin_texinfo[i].texture);
		map_texinfo[i].nexttexinfo = -1;
	} //end for
} //end of the function Sin_CreateMapTexinfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_SetLeafBrushesModelNumbers(int leafnum, int modelnum)
{
	int i, brushnum;
	sin_dleaf_t *leaf;

	leaf = &sin_dleafs[leafnum];
	for (i = 0; i < leaf->numleafbrushes; i++)
	{
		brushnum = sin_dleafbrushes[leaf->firstleafbrush + i];
		brushmodelnumbers[brushnum] = modelnum;
		dbrushleafnums[brushnum] = leafnum;
	} //end for
} //end of the function Sin_SetLeafBrushesModelNumbers
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_InitNodeStack(void)
{
	nodestackptr = nodestack;
	nodestacksize = 0;
} //end of the function Sin_InitNodeStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_PushNodeStack(int num)
{
	*nodestackptr = num;
	nodestackptr++;
	nodestacksize++;
	//
	if (nodestackptr >= &nodestack[NODESTACKSIZE])
	{
		Error("Sin_PushNodeStack: stack overflow\n");
	} //end if
} //end of the function Sin_PushNodeStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int Sin_PopNodeStack(void)
{
	//if the stack is empty
	if (nodestackptr <= nodestack) return -1;
	//decrease stack pointer
	nodestackptr--;
	nodestacksize--;
	//return the top value from the stack
	return *nodestackptr;
} //end of the function Sin_PopNodeStack
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_SetBrushModelNumbers(entity_t *mapent)
{
	int n, pn;
	int leafnum;

	//
	Sin_InitNodeStack();
	//head node (root) of the bsp tree
	n = sin_dmodels[mapent->modelnum].headnode;
	pn = 0;
	
	do
	{
		//if we are in a leaf (negative node number)
		if (n < 0)
		{
			//number of the leaf
			leafnum = (-n) - 1;
			//set the brush numbers
			Sin_SetLeafBrushesModelNumbers(leafnum, mapent->modelnum);
			//walk back into the tree to find a second child to continue with
			for (pn = Sin_PopNodeStack(); pn >= 0; n = pn, pn = Sin_PopNodeStack())
			{
				//if we took the first child at the parent node
				if (sin_dnodes[pn].children[0] == n) break;
			} //end for
			//if the stack wasn't empty (if not processed whole tree)
			if (pn >= 0)
			{
				//push the parent node again
				Sin_PushNodeStack(pn);
				//we proceed with the second child of the parent node
				n = sin_dnodes[pn].children[1];
			} //end if
		} //end if
		else
		{
			//push the current node onto the stack
			Sin_PushNodeStack(n);
			//walk forward into the tree to the first child
			n = sin_dnodes[n].children[0];
		} //end else
	} while(pn >= 0);
} //end of the function Sin_SetBrushModelNumbers
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_BSPBrushToMapBrush(sin_dbrush_t *bspbrush, entity_t *mapent)
{
	mapbrush_t *b;
	int i, k, n;
	side_t *side, *s2;
	int planenum;
	sin_dbrushside_t *bspbrushside;
	sin_dplane_t *bspplane;

	if (nummapbrushes >= MAX_MAPFILE_BRUSHES)
		Error ("nummapbrushes >= MAX_MAPFILE_BRUSHES");

	b = &mapbrushes[nummapbrushes];
	b->original_sides = &brushsides[nummapbrushsides];
	b->entitynum = mapent-entities;
	b->brushnum = nummapbrushes - mapent->firstbrush;
	b->leafnum = dbrushleafnums[bspbrush - sin_dbrushes];

	for (n = 0; n < bspbrush->numsides; n++)
	{
		//pointer to the bsp brush side
		bspbrushside = &sin_dbrushsides[bspbrush->firstside + n];

		if (nummapbrushsides >= MAX_MAPFILE_BRUSHSIDES)
		{
			Error ("MAX_MAPFILE_BRUSHSIDES");
		} //end if
		//pointer to the map brush side
		side = &brushsides[nummapbrushsides];
		//if the BSP brush side is textured
		if (sin_dbrushsidetextured[bspbrush->firstside + n]) side->flags |= SFL_TEXTURED;
		else side->flags &= ~SFL_TEXTURED;
		//ME: can get side contents and surf directly from BSP file
		side->contents = bspbrush->contents;
		//if the texinfo is TEXINFO_NODE
		if (bspbrushside->texinfo < 0) side->surf = 0;
		else side->surf = sin_texinfo[bspbrushside->texinfo].flags;

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
		bspplane = &sin_dplanes[bspbrushside->planenum];
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
		//NOTE: check for TEXINFO_NODE, otherwise crash in Sin_BrushContents
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
	Sin_BrushContents(b);

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
} //end of the function Sin_BSPBrushToMapBrush
//===========================================================================
//===========================================================================
void Sin_ParseBSPBrushes(entity_t *mapent)
{
	int i, testnum = 0;

	//give all the brushes that belong to this entity the number of the
	//BSP model used by this entity
	Sin_SetBrushModelNumbers(mapent);
	//now parse all the brushes with the correct mapent->modelnum
	for (i = 0; i < sin_numbrushes; i++)
	{
		if (brushmodelnumbers[i] == mapent->modelnum)
		{
			testnum++;
			Sin_BSPBrushToMapBrush(&sin_dbrushes[i], mapent);
		} //end if
	} //end for
} //end of the function Sin_ParseBSPBrushes
//===========================================================================
//===========================================================================
qboolean Sin_ParseBSPEntity(int entnum)
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
	if (model && *model == '*')
	{
		mapent->modelnum = atoi(&model[1]);
		//Log_Print("model = %s\n", model);
		//Log_Print("mapent->modelnum = %d\n", mapent->modelnum);
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
		Sin_ParseBSPBrushes(mapent);
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
} //end of the function Sin_ParseBSPEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Sin_LoadMapFromBSP(char *filename, int offset, int length)
{
	int i;

	Log_Print("-- Sin_LoadMapFromBSP --\n");
	//loaded map type
	loadedmaptype = MAPTYPE_SIN;

	Log_Print("Loading map from %s...\n", filename);
	//load the bsp file
	Sin_LoadBSPFile(filename, offset, length);

	//create an index from bsp planes to map planes
	//DPlanes2MapPlanes();
	//clear brush model numbers
	for (i = 0; i < MAX_MAPFILE_BRUSHES; i++)
		brushmodelnumbers[i] = -1;

	nummapbrushsides = 0;
	num_entities = 0;

	Sin_ParseEntities();
	//
	for (i = 0; i < num_entities; i++)
	{
		Sin_ParseBSPEntity(i);
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
	//
	Sin_CreateMapTexinfo();
} //end of the function Sin_LoadMapFromBSP

void Sin_ResetMapLoading(void)
{
	//reset for map loading from bsp
	memset(nodestack, 0, NODESTACKSIZE * sizeof(int));
	nodestackptr = NULL;
	nodestacksize = 0;
	memset(brushmodelnumbers, 0, MAX_MAPFILE_BRUSHES * sizeof(int));
} //end of the function Sin_ResetMapLoading

//End MAP loading from BSP file

#endif //ME
