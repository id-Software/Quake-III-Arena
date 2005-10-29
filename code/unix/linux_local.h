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
// linux_local.h: Linux-specific Quake3 header file

void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );
qboolean Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );
void Sys_SendKeyEvents (void);

// Input subsystem

void IN_Init (void);
void IN_Frame (void);
void IN_Shutdown (void);


void IN_JoyMove( void );
void IN_StartupJoystick( void );

// GL subsystem
qboolean QGL_Init( const char *dllname );
void QGL_EnableLogging( qboolean enable );
void QGL_Shutdown( void );

// bk001130 - win32
// void IN_JoystickCommands (void);

char *strlwr (char *s);

// signals.c
void InitSig(void);
