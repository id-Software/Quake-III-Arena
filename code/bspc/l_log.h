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

//open a log file
void Log_Open(char *filename);
//close the current log file
void Log_Close(void);
//close log file if present
void Log_Shutdown(void);
//print on stdout and write to the current opened log file
void Log_Print(char *fmt, ...);
//write to the current opened log file
void Log_Write(char *fmt, ...);
//write to the current opened log file with a time stamp
void Log_WriteTimeStamped(char *fmt, ...);
//returns the log file structure
FILE *Log_FileStruct(void);
//flush log file
void Log_Flush(void);

#ifdef WINBSPC
void WinBSPCPrint(char *str);
#endif //WINBSPC
