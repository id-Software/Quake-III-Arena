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
//

/*****************************************************************************
 * name:		be_ea.h
 *
 * desc:		elementary actions
 *
 * $Archive: /source/code/botlib/be_ea.h $
 *
 *****************************************************************************/

//ClientCommand elementary actions
void EA_Say(int client, char *str);
void EA_SayTeam(int client, char *str);
void EA_Command(int client, char *command );

void EA_Action(int client, int action);
void EA_Crouch(int client);
void EA_Walk(int client);
void EA_MoveUp(int client);
void EA_MoveDown(int client);
void EA_MoveForward(int client);
void EA_MoveBack(int client);
void EA_MoveLeft(int client);
void EA_MoveRight(int client);
void EA_Attack(int client);
void EA_Respawn(int client);
void EA_Talk(int client);
void EA_Gesture(int client);
void EA_Use(int client);

//regular elementary actions
void EA_SelectWeapon(int client, int weapon);
void EA_Jump(int client);
void EA_DelayedJump(int client);
void EA_Move(int client, vec3_t dir, float speed);
void EA_View(int client, vec3_t viewangles);

//send regular input to the server
void EA_EndRegular(int client, float thinktime);
void EA_GetInput(int client, float thinktime, bot_input_t *input);
void EA_ResetInput(int client);
//setup and shutdown routines
int EA_Setup(void);
void EA_Shutdown(void);
