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

/*****************************************************************************
 * name:		be_aas_entity.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_entity.h $
 *
 *****************************************************************************/

#ifdef AASINTERN
//invalidates all entity infos
void AAS_InvalidateEntities(void);
//unlink not updated entities
void AAS_UnlinkInvalidEntities(void);
//resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks(void);
//updates an entity
int AAS_UpdateEntity(int ent, bot_entitystate_t *state);
//gives the entity data used for collision detection
void AAS_EntityBSPData(int entnum, bsp_entdata_t *entdata);
#endif //AASINTERN

//returns the size of the entity bounding box in mins and maxs
void AAS_EntitySize(int entnum, vec3_t mins, vec3_t maxs);
//returns the BSP model number of the entity
int AAS_EntityModelNum(int entnum);
//returns the origin of an entity with the given model number
int AAS_OriginOfMoverWithModelNum(int modelnum, vec3_t origin);
//returns the best reachable area the entity is situated in
int AAS_BestReachableEntityArea(int entnum);
//returns the info of the given entity
void AAS_EntityInfo(int entnum, aas_entityinfo_t *info);
//returns the next entity
int AAS_NextEntity(int entnum);
//returns the origin of the entity
void AAS_EntityOrigin(int entnum, vec3_t origin);
//returns the entity type
int AAS_EntityType(int entnum);
//returns the model index of the entity
int AAS_EntityModelindex(int entnum);

