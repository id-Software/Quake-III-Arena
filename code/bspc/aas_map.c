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
#include "l_mem.h"
#include "../botlib/aasfile.h"		//aas_bbox_t
#include "aas_store.h"				//AAS_MAX_BBOXES
#include "aas_cfg.h"
#include "../game/surfaceflags.h"

#define SPAWNFLAG_NOT_EASY			0x00000100
#define SPAWNFLAG_NOT_MEDIUM		0x00000200
#define SPAWNFLAG_NOT_HARD			0x00000400
#define SPAWNFLAG_NOT_DEATHMATCH	0x00000800
#define SPAWNFLAG_NOT_COOP			0x00001000

#define STATE_TOP				0
#define STATE_BOTTOM			1
#define STATE_UP				2
#define STATE_DOWN			3

#define DOOR_START_OPEN		1
#define DOOR_REVERSE			2
#define DOOR_CRUSHER			4
#define DOOR_NOMONSTER		8
#define DOOR_TOGGLE			32
#define DOOR_X_AXIS			64
#define DOOR_Y_AXIS			128

#define BBOX_NORMAL_EPSILON			0.0001

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
vec_t BoxOriginDistanceFromPlane(vec3_t normal, vec3_t mins, vec3_t maxs, int side)
{
	vec3_t v1, v2;
	int i;

	if (side)
	{
		for (i = 0; i < 3; i++)
		{
			if (normal[i] > BBOX_NORMAL_EPSILON) v1[i] = maxs[i];
			else if (normal[i] < -BBOX_NORMAL_EPSILON) v1[i] = mins[i];
			else v1[i] = 0;
		} //end for
	} //end if
	else
	{
		for (i = 0; i < 3; i++)
		{
			if (normal[i] > BBOX_NORMAL_EPSILON) v1[i] = mins[i];
			else if (normal[i] < -BBOX_NORMAL_EPSILON) v1[i] = maxs[i];
			else v1[i] = 0;
		} //end for
	} //end else
	VectorCopy(normal, v2);
	VectorInverse(v2);
	return DotProduct(v1, v2);
} //end of the function BoxOriginDistanceFromPlane
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
vec_t CapsuleOriginDistanceFromPlane(vec3_t normal, vec3_t mins, vec3_t maxs)
{
	float offset_up, offset_down, width, radius;

	width = maxs[0] - mins[0];
	// if the box is less high then it is wide
	if (maxs[2] - mins[2] < width) {
		width = maxs[2] - mins[2];
	}
	radius = width * 0.5;
	// offset to upper and lower sphere
	offset_up = maxs[2] - radius;
	offset_down = -mins[2] - radius;

	// if normal points upward
	if ( normal[2] > 0 ) {
		// touches lower sphere first
		return normal[2] * offset_down + radius;
	}
	else {
		// touched upper sphere first
		return -normal[2] * offset_up + radius;
	}
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ExpandMapBrush(mapbrush_t *brush, vec3_t mins, vec3_t maxs)
{
	int sn;
	float dist;
	side_t *s;
	plane_t *plane;

	for (sn = 0; sn < brush->numsides; sn++)
	{
		s = brush->original_sides + sn;
		plane = &mapplanes[s->planenum];
		dist = plane->dist;
		if (capsule_collision) {
			dist += CapsuleOriginDistanceFromPlane(plane->normal, mins, maxs);
		}
		else {
			dist += BoxOriginDistanceFromPlane(plane->normal, mins, maxs, 0);
		}
		s->planenum = FindFloatPlane(plane->normal, dist);
		//the side isn't a bevel after expanding
		s->flags &= ~SFL_BEVEL;
		//don't skip the surface
		s->surf &= ~SURF_SKIP;
		//make sure the texinfo is not TEXINFO_NODE
		//when player clip contents brushes are read from the bsp tree
		//they have the texinfo field set to TEXINFO_NODE
		//s->texinfo = 0;
	} //end for
} //end of the function AAS_ExpandMapBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetTexinfo(mapbrush_t *brush)
{
	int n;
	side_t *side;

	if (brush->contents & (CONTENTS_LADDER
									| CONTENTS_AREAPORTAL
									| CONTENTS_CLUSTERPORTAL
									| CONTENTS_TELEPORTER
									| CONTENTS_JUMPPAD
									| CONTENTS_DONOTENTER
									| CONTENTS_WATER
									| CONTENTS_LAVA
									| CONTENTS_SLIME
									| CONTENTS_WINDOW
									| CONTENTS_PLAYERCLIP))
	{
		//we just set texinfo to 0 because these brush sides MUST be used as
		//bsp splitters textured or not textured
		for (n = 0; n < brush->numsides; n++)
		{
			side = brush->original_sides + n;
			//side->flags |= SFL_TEXTURED|SFL_VISIBLE;
			side->texinfo = 0;
		} //end for
	} //end if
	else
	{
		//only use brush sides as splitters if they are textured
		//texinfo of non-textured sides will be set to TEXINFO_NODE
		for (n = 0; n < brush->numsides; n++)
		{
			side = brush->original_sides + n;
			//don't use side as splitter (set texinfo to TEXINFO_NODE) if not textured
			if (side->flags & (SFL_TEXTURED|SFL_BEVEL)) side->texinfo = 0;
			else side->texinfo = TEXINFO_NODE;
		} //end for
	} //end else
} //end of the function AAS_SetTexinfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FreeBrushWindings(mapbrush_t *brush)
{
	int n;
	side_t *side;
	//
	for (n = 0; n < brush->numsides; n++)
	{
		side = brush->original_sides + n;
		//
		if (side->winding) FreeWinding(side->winding);
	} //end for
} //end of the function FreeBrushWindings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AddMapBrushSide(mapbrush_t *brush, int planenum)
{
	side_t *side;
	//
	if (nummapbrushsides >= MAX_MAPFILE_BRUSHSIDES)
		Error ("MAX_MAPFILE_BRUSHSIDES");
	//
	side = brush->original_sides + brush->numsides;
	side->original = NULL;
	side->winding = NULL;
	side->contents = brush->contents;
	side->flags &= ~(SFL_BEVEL|SFL_VISIBLE);
	side->surf = 0;
	side->planenum = planenum;
	side->texinfo = 0;
	//
	nummapbrushsides++;
	brush->numsides++;
} //end of the function AAS_AddMapBrushSide
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FixMapBrush(mapbrush_t *brush)
{
	int i, j, planenum;
	float dist;
	winding_t *w;
	plane_t *plane, *plane1, *plane2;
	side_t *side;
	vec3_t normal;

	//calculate the brush bounds
	ClearBounds(brush->mins, brush->maxs);
	for (i = 0; i < brush->numsides; i++)
	{
		plane = &mapplanes[brush->original_sides[i].planenum];
		w = BaseWindingForPlane(plane->normal, plane->dist);
		for (j = 0; j < brush->numsides && w; j++)
		{
			if (i == j) continue;
			//there are no brush bevels marked but who cares :)
			if (brush->original_sides[j].flags & SFL_BEVEL) continue;
			plane = &mapplanes[brush->original_sides[j].planenum^1];
			ChopWindingInPlace(&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		} //end for

		side = &brush->original_sides[i];
		side->winding = w;
		if (w)
		{
			for (j = 0; j < w->numpoints; j++)
			{
				AddPointToBounds(w->p[j], brush->mins, brush->maxs);
			} //end for
		} //end if
	} //end for
	//
	for (i = 0; i < brush->numsides; i++)
	{
		for (j = 0; j < brush->numsides; j++)
		{
			if (i == j) continue;
			plane1 = &mapplanes[brush->original_sides[i].planenum];
			plane2 = &mapplanes[brush->original_sides[j].planenum];
			if (WindingsNonConvex(brush->original_sides[i].winding,
									brush->original_sides[j].winding,
									plane1->normal, plane2->normal,
									plane1->dist, plane2->dist))
			{
				Log_Print("non convex brush");
			} //end if
		} //end for
	} //end for

	//NOW close the fucking brush!!
	for (i = 0; i < 3; i++)
	{
		if (brush->mins[i] < -MAX_MAP_BOUNDS)
		{
			VectorClear(normal);
			normal[i] = -1;
			dist = MAX_MAP_BOUNDS - 10;
			planenum = FindFloatPlane(normal, dist);
			//
			Log_Print("mins out of range: added extra brush side\n");
			AAS_AddMapBrushSide(brush, planenum);
		} //end if
		if (brush->maxs[i] > MAX_MAP_BOUNDS)
		{
			VectorClear(normal);
			normal[i] = 1;
			dist = MAX_MAP_BOUNDS - 10;
			planenum = FindFloatPlane(normal, dist);
			//
			Log_Print("maxs out of range: added extra brush side\n");
			AAS_AddMapBrushSide(brush, planenum);
		} //end if
		if (brush->mins[i] > MAX_MAP_BOUNDS || brush->maxs[i] < -MAX_MAP_BOUNDS)
		{
			Log_Print("entity %i, brush %i: no visible sides on brush\n", brush->entitynum, brush->brushnum);
		} //end if
	} //end for
	//free all the windings
	FreeBrushWindings(brush);
} //end of the function AAS_FixMapBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_MakeBrushWindings(mapbrush_t *ob)
{
	int			i, j;
	winding_t	*w;
	side_t		*side;
	plane_t		*plane, *plane1, *plane2;

	ClearBounds (ob->mins, ob->maxs);

	for (i = 0; i < ob->numsides; i++)
	{
		plane = &mapplanes[ob->original_sides[i].planenum];
		w = BaseWindingForPlane(plane->normal, plane->dist);
		for (j = 0; j <ob->numsides && w; j++)
		{
			if (i == j) continue;
			if (ob->original_sides[j].flags & SFL_BEVEL) continue;
			plane = &mapplanes[ob->original_sides[j].planenum^1];
			ChopWindingInPlace(&w, plane->normal, plane->dist, 0); //CLIP_EPSILON);
		}

		side = &ob->original_sides[i];
		side->winding = w;
		if (w)
		{
			side->flags |= SFL_VISIBLE;
			for (j = 0; j < w->numpoints; j++)
				AddPointToBounds (w->p[j], ob->mins, ob->maxs);
		}
	}
	//check if the brush is convex
	for (i = 0; i < ob->numsides; i++)
	{
		for (j = 0; j < ob->numsides; j++)
		{
			if (i == j) continue;
			plane1 = &mapplanes[ob->original_sides[i].planenum];
			plane2 = &mapplanes[ob->original_sides[j].planenum];
			if (WindingsNonConvex(ob->original_sides[i].winding,
									ob->original_sides[j].winding,
									plane1->normal, plane2->normal,
									plane1->dist, plane2->dist))
			{
				Log_Print("non convex brush");
			} //end if
		} //end for
	} //end for
	//check for out of bound brushes
	for (i = 0; i < 3; i++)
	{
		//IDBUG: all the indexes into the mins and maxs were zero (not using i)
		if (ob->mins[i] < -MAX_MAP_BOUNDS || ob->maxs[i] > MAX_MAP_BOUNDS)
		{
			Log_Print("entity %i, brush %i: bounds out of range\n", ob->entitynum, ob->brushnum);
			Log_Print("ob->mins[%d] = %f, ob->maxs[%d] = %f\n", i, ob->mins[i], i, ob->maxs[i]);
			ob->numsides = 0; //remove the brush
			break;
		} //end if
		if (ob->mins[i] > MAX_MAP_BOUNDS || ob->maxs[i] < -MAX_MAP_BOUNDS)
		{
			Log_Print("entity %i, brush %i: no visible sides on brush\n", ob->entitynum, ob->brushnum);
			Log_Print("ob->mins[%d] = %f, ob->maxs[%d] = %f\n", i, ob->mins[i], i, ob->maxs[i]);
			ob->numsides = 0; //remove the brush
			break;
		} //end if
	} //end for
	return true;
} //end of the function AAS_MakeBrushWindings
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
mapbrush_t *AAS_CopyMapBrush(mapbrush_t *brush, entity_t *mapent)
{
	int n;
	mapbrush_t *newbrush;
	side_t *side, *newside;

	if (nummapbrushes >= MAX_MAPFILE_BRUSHES)
		Error ("MAX_MAPFILE_BRUSHES");

	newbrush = &mapbrushes[nummapbrushes];
	newbrush->original_sides = &brushsides[nummapbrushsides];
	newbrush->entitynum = brush->entitynum;
	newbrush->brushnum = nummapbrushes - mapent->firstbrush;
	newbrush->numsides = brush->numsides;
	newbrush->contents = brush->contents;

	//copy the sides
	for (n = 0; n < brush->numsides; n++)
	{
		if (nummapbrushsides >= MAX_MAPFILE_BRUSHSIDES)
			Error ("MAX_MAPFILE_BRUSHSIDES");
		side = brush->original_sides + n;

		newside = newbrush->original_sides + n;
		newside->original = NULL;
		newside->winding = NULL;
		newside->contents = side->contents;
		newside->flags = side->flags;
		newside->surf = side->surf;
		newside->planenum = side->planenum;
		newside->texinfo = side->texinfo;
		nummapbrushsides++;
	} //end for
	//
	nummapbrushes++;
	mapent->numbrushes++;
	return newbrush;
} //end of the function AAS_CopyMapBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int mark_entities[MAX_MAP_ENTITIES];

int AAS_AlwaysTriggered_r(char *targetname)
{
	int i;

	if (!strlen(targetname)) {
		return false;
	}
	//
	for (i = 0; i < num_entities; i++) {
		// if the entity will activate the given targetname
		if ( !strcmp(targetname, ValueForKey(&entities[i], "target")) ) {
			// if this activator is present in deathmatch
			if (!(atoi(ValueForKey(&entities[i], "spawnflags")) & SPAWNFLAG_NOT_DEATHMATCH)) {
				// if it is a trigger_always entity
				if (!strcmp("trigger_always", ValueForKey(&entities[i], "classname"))) {
					return true;
				}
				// check for possible trigger_always entities activating this entity
				if ( mark_entities[i] ) {
					Warning( "entity %d, classname %s has recursive targetname %s\n", i,
										ValueForKey(&entities[i], "classname"), targetname );
					return false;
				}
				mark_entities[i] = true;
				if ( AAS_AlwaysTriggered_r(ValueForKey(&entities[i], "targetname")) ) {
					return true;
				}
			}
		}
	}
	return false;
}

int AAS_AlwaysTriggered(char *targetname) {
	memset( mark_entities, 0, sizeof(mark_entities) );
	return AAS_AlwaysTriggered_r( targetname );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_ValidEntity(entity_t *mapent)
{
	int i;
	char target[1024];

	//all world brushes are used for AAS
	if (mapent == &entities[0])
	{
		return true;
	} //end if
	//some of the func_wall brushes are also used for AAS
	else if (!strcmp("func_wall", ValueForKey(mapent, "classname")))
	{
		//Log_Print("found func_wall entity %d\n", mapent - entities);
		//if the func wall is used in deathmatch
		if (!(atoi(ValueForKey(mapent, "spawnflags")) & SPAWNFLAG_NOT_DEATHMATCH))
		{
			//Log_Print("func_wall USED in deathmatch mode %d\n", atoi(ValueForKey(mapent, "spawnflags")));
			return true;
		} //end if
	} //end else if
	else if (!strcmp("func_door_rotating", ValueForKey(mapent, "classname")))
	{
		//if the func_door_rotating is present in deathmatch
		if (!(atoi(ValueForKey(mapent, "spawnflags")) & SPAWNFLAG_NOT_DEATHMATCH))
		{
			//if the func_door_rotating is always activated in deathmatch
			if (AAS_AlwaysTriggered(ValueForKey(mapent, "targetname")))
			{
				//Log_Print("found func_door_rotating in deathmatch\ntargetname %s\n", ValueForKey(mapent, "targetname"));
				return true;
			} //end if
		} //end if
	} //end else if
	else if (!strcmp("trigger_hurt", ValueForKey(mapent, "classname")))
	{
		//"dmg" is the damage, for instance: "dmg" "666"
		return true;
	} //end else if
	else if (!strcmp("trigger_push", ValueForKey(mapent, "classname")))
	{
		return true;
	} //end else if
	else if (!strcmp("trigger_multiple", ValueForKey(mapent, "classname")))
	{
		//find out if the trigger_multiple is pointing to a target_teleporter
		strcpy(target, ValueForKey(mapent, "target"));
		for (i = 0; i < num_entities; i++)
		{
			//if the entity will activate the given targetname
			if (!strcmp(target, ValueForKey(&entities[i], "targetname")))
			{
				if (!strcmp("target_teleporter", ValueForKey(&entities[i], "classname")))
				{
					return true;
				} //end if
			} //end if
		} //end for
	} //end else if
	else if (!strcmp("trigger_teleport", ValueForKey(mapent, "classname")))
	{
		return true;
	} //end else if
	else if (!strcmp("func_static", ValueForKey(mapent, "classname")))
	{
		//FIXME: easy/medium/hard/deathmatch specific?
		return true;
	} //end else if
	else if (!strcmp("func_door", ValueForKey(mapent, "classname")))
	{
		return true;
	} //end else if
	return false;
} //end of the function AAS_ValidEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TransformPlane(int planenum, vec3_t origin, vec3_t angles)
{
	float newdist, matrix[3][3];
	vec3_t normal;

	//rotate the node plane
	VectorCopy(mapplanes[planenum].normal, normal);
	CreateRotationMatrix(angles, matrix);
	RotatePoint(normal, matrix);
	newdist = mapplanes[planenum].dist + DotProduct(normal, origin);
	return FindFloatPlane(normal, newdist);
} //end of the function AAS_TransformPlane
//===========================================================================
// this function sets the func_rotating_door in it's final position
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_PositionFuncRotatingBrush(entity_t *mapent, mapbrush_t *brush)
{
	int spawnflags, i;
	float distance;
	vec3_t movedir, angles, pos1, pos2;
	side_t *s;

	spawnflags = FloatForKey(mapent, "spawnflags");
	VectorClear(movedir);
	if (spawnflags & DOOR_X_AXIS)
		movedir[2] = 1.0;		//roll
	else if (spawnflags & DOOR_Y_AXIS)
		movedir[0] = 1.0;		//pitch
	else // Z_AXIS
		movedir[1] = 1.0;		//yaw

	// check for reverse rotation
	if (spawnflags & DOOR_REVERSE)
		VectorInverse(movedir);

	distance = FloatForKey(mapent, "distance");
	if (!distance) distance = 90;

	GetVectorForKey(mapent, "angles", angles);
	VectorCopy(angles, pos1);
	VectorMA(angles, -distance, movedir, pos2);
	// if it starts open, switch the positions
	if (spawnflags & DOOR_START_OPEN)
	{
		VectorCopy(pos2, angles);
		VectorCopy(pos1, pos2);
		VectorCopy(angles, pos1);
		VectorInverse(movedir);
	} //end if
	//
	for (i = 0; i < brush->numsides; i++)
	{
		s = &brush->original_sides[i];
		s->planenum = AAS_TransformPlane(s->planenum, mapent->origin, pos2);
	} //end for
	//
	FreeBrushWindings(brush);
	AAS_MakeBrushWindings(brush);
	AddBrushBevels(brush);
	FreeBrushWindings(brush);
} //end of the function AAS_PositionFuncRotatingBrush
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_PositionBrush(entity_t *mapent, mapbrush_t *brush)
{
	side_t *s;
	float newdist;
	int i, notteam;
	char *model;

	if (!strcmp(ValueForKey(mapent, "classname"), "func_door_rotating"))
	{
		AAS_PositionFuncRotatingBrush(mapent, brush);
	} //end if
	else
	{
		if (mapent->origin[0] || mapent->origin[1] || mapent->origin[2])
		{
			for (i = 0; i < brush->numsides; i++)
			{
				s = &brush->original_sides[i];
				newdist = mapplanes[s->planenum].dist +
						DotProduct(mapplanes[s->planenum].normal, mapent->origin);
				s->planenum = FindFloatPlane(mapplanes[s->planenum].normal, newdist);
			} //end for
		} //end if
		//if it's a trigger hurt
		if (!strcmp("trigger_hurt", ValueForKey(mapent, "classname")))
		{
			notteam = FloatForKey(mapent, "bot_notteam");
			if ( notteam == 1 ) {
				brush->contents |= CONTENTS_NOTTEAM1;
			}
			else if ( notteam == 2 ) {
				brush->contents |= CONTENTS_NOTTEAM2;
			}
			else {
				// always avoid so set lava contents
				brush->contents |= CONTENTS_LAVA;
			}
		} //end if
		//
		else if (!strcmp("trigger_push", ValueForKey(mapent, "classname")))
		{
			//set the jumppad contents
			brush->contents = CONTENTS_JUMPPAD;
			//Log_Print("found trigger_push brush\n");
		} //end if
		//
		else if (!strcmp("trigger_multiple", ValueForKey(mapent, "classname")))
		{
			//set teleporter contents
			brush->contents = CONTENTS_TELEPORTER;
			//Log_Print("found trigger_multiple teleporter brush\n");
		} //end if
		//
		else if (!strcmp("trigger_teleport", ValueForKey(mapent, "classname")))
		{
			//set teleporter contents
			brush->contents = CONTENTS_TELEPORTER;
			//Log_Print("found trigger_teleport teleporter brush\n");
		} //end if
		else if (!strcmp("func_door", ValueForKey(mapent, "classname")))
		{
			//set mover contents
			brush->contents = CONTENTS_MOVER;
			//get the model number
			model = ValueForKey(mapent, "model");
			brush->modelnum = atoi(model+1);
		} //end if
	} //end else
} //end of the function AAS_PositionBrush
//===========================================================================
// uses the global cfg_t cfg
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CreateMapBrushes(mapbrush_t *brush, entity_t *mapent, int addbevels)
{
	int i;
	//side_t *s;
	mapbrush_t *bboxbrushes[16];

	//if the brushes are not from an entity used for AAS
	if (!AAS_ValidEntity(mapent))
	{
		nummapbrushsides -= brush->numsides;
		brush->numsides = 0;
		return;
	} //end if
	//
	AAS_PositionBrush(mapent, brush);
	//from all normal solid brushes only the textured brush sides will
	//be used as bsp splitters, so set the right texinfo reference here
	AAS_SetTexinfo(brush);
	//remove contents detail flag, otherwise player clip contents won't be
	//bsped correctly for AAS!
	brush->contents &= ~CONTENTS_DETAIL;
	//if the brush has contents area portal it should be the only contents
	if (brush->contents & (CONTENTS_AREAPORTAL|CONTENTS_CLUSTERPORTAL))
	{
		brush->contents = CONTENTS_CLUSTERPORTAL;
		brush->leafnum = -1;
	} //end if
	//window and playerclip are used for player clipping, make them solid
	if (brush->contents & (CONTENTS_WINDOW | CONTENTS_PLAYERCLIP))
	{
		//
		brush->contents &= ~(CONTENTS_WINDOW | CONTENTS_PLAYERCLIP);
		brush->contents |= CONTENTS_SOLID;
		brush->leafnum = -1;
	} //end if
	//
	if (brush->contents & CONTENTS_BOTCLIP)
	{
		brush->contents = CONTENTS_SOLID;
		brush->leafnum = -1;
	} //end if
	//
	//Log_Write("brush %d contents = ", brush->brushnum);
	//PrintContents(brush->contents);
	//Log_Write("\r\n");
	//if not one of the following brushes then the brush is NOT used for AAS
	if (!(brush->contents & (CONTENTS_SOLID
									| CONTENTS_LADDER
									| CONTENTS_CLUSTERPORTAL
									| CONTENTS_DONOTENTER
									| CONTENTS_TELEPORTER
									| CONTENTS_JUMPPAD
									| CONTENTS_WATER
									| CONTENTS_LAVA
									| CONTENTS_SLIME
									| CONTENTS_MOVER
									)))
	{
		nummapbrushsides -= brush->numsides;
		brush->numsides = 0;
		return;
	} //end if
	//fix the map brush
	//AAS_FixMapBrush(brush);
	//if brush bevels should be added (for real map brushes, not bsp map brushes)
	if (addbevels)
	{
		//NOTE: we first have to get the mins and maxs of the brush before
		//			creating the brush bevels... the mins and maxs are used to
		//			create them. so we call MakeBrushWindings to get the mins
		//			and maxs and then after creating the bevels we free the
		//			windings because they are created for all sides (including
		//			bevels) a little later
		AAS_MakeBrushWindings(brush);
		AddBrushBevels(brush);
		FreeBrushWindings(brush);
	} //end if
	//NOTE: add the brush to the WORLD entity!!!
	mapent = &entities[0];
	//there's at least one new brush for now
	nummapbrushes++;
	mapent->numbrushes++;
	//liquid brushes are expanded for the maximum possible bounding box
	if (brush->contents & (CONTENTS_WATER
									| CONTENTS_LAVA
									| CONTENTS_SLIME 
									| CONTENTS_TELEPORTER
									| CONTENTS_JUMPPAD
									| CONTENTS_DONOTENTER
									| CONTENTS_MOVER
									))
	{
		brush->expansionbbox = 0;
		//NOTE: the first bounding box is the max
		//FIXME: use max bounding box created from all bboxes
		AAS_ExpandMapBrush(brush, cfg.bboxes[0].mins, cfg.bboxes[0].maxs);
		AAS_MakeBrushWindings(brush);
	} //end if
	//area portal brushes are NOT expanded
	else if (brush->contents & CONTENTS_CLUSTERPORTAL)
	{
		brush->expansionbbox = 0;
		//NOTE: the first bounding box is the max
		//FIXME: use max bounding box created from all bboxes
		AAS_ExpandMapBrush(brush, cfg.bboxes[0].mins, cfg.bboxes[0].maxs);
		AAS_MakeBrushWindings(brush);
	} //end if
	//all solid brushes are expanded for all bounding boxes
	else if (brush->contents & (CONTENTS_SOLID
										| CONTENTS_LADDER
										))
	{
		//brush for the first bounding box
		bboxbrushes[0] = brush;
		//make a copy for the other bounding boxes
		for (i = 1; i < cfg.numbboxes; i++)
		{
			bboxbrushes[i] = AAS_CopyMapBrush(brush, mapent);
		} //end for
		//expand every brush for it's bounding box and create windings
		for (i = 0; i < cfg.numbboxes; i++)
		{
			AAS_ExpandMapBrush(bboxbrushes[i], cfg.bboxes[i].mins, cfg.bboxes[i].maxs);
			bboxbrushes[i]->expansionbbox = cfg.bboxes[i].presencetype;
			AAS_MakeBrushWindings(bboxbrushes[i]);
		} //end for
	} //end else
} //end of the function AAS_CreateMapBrushes
