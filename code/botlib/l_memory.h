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
 * name:		l_memory.h
 *
 * desc:		memory management
 *
 * $Archive: /source/code/botlib/l_memory.h $
 *
 *****************************************************************************/

//#define MEMDEBUG

#ifdef MEMDEBUG
#define GetMemory(size)				GetMemoryDebug(size, #size, __FILE__, __LINE__);
#define GetClearedMemory(size)		GetClearedMemoryDebug(size, #size, __FILE__, __LINE__);
//allocate a memory block of the given size
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line);
//allocate a memory block of the given size and clear it
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line);
//
#define GetHunkMemory(size)			GetHunkMemoryDebug(size, #size, __FILE__, __LINE__);
#define GetClearedHunkMemory(size)	GetClearedHunkMemoryDebug(size, #size, __FILE__, __LINE__);
//allocate a memory block of the given size
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line);
//allocate a memory block of the given size and clear it
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line);
#else
//allocate a memory block of the given size
void *GetMemory(unsigned long size);
//allocate a memory block of the given size and clear it
void *GetClearedMemory(unsigned long size);
//
#ifdef BSPC
#define GetHunkMemory GetMemory
#define GetClearedHunkMemory GetClearedMemory
#else
//allocate a memory block of the given size
void *GetHunkMemory(unsigned long size);
//allocate a memory block of the given size and clear it
void *GetClearedHunkMemory(unsigned long size);
#endif
#endif

//free the given memory block
void FreeMemory(void *ptr);
//returns the amount available memory
int AvailableMemory(void);
//prints the total used memory size
void PrintUsedMemorySize(void);
//print all memory blocks with label
void PrintMemoryLabels(void);
//returns the size of the memory block in bytes
int MemoryByteSize(void *ptr);
//free all allocated memory
void DumpMemory(void);
