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
 * name:		l_log.h
 *
 * desc:		log file
 *
 * $Archive: /source/code/botlib/l_log.h $
 *
 *****************************************************************************/

//open a log file
void Log_Open(char *filename);
//close the current log file
void Log_Close(void);
//close log file if present
void Log_Shutdown(void);
//write to the current opened log file
void QDECL Log_Write(char *fmt, ...);
//write to the current opened log file with a time stamp
void QDECL Log_WriteTimeStamped(char *fmt, ...);
//returns a pointer to the log file
FILE *Log_FilePointer(void);
//flush log file
void Log_Flush(void);

