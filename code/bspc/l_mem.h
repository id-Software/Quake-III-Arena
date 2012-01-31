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


//=============================================================================

// memory.h
//#define MEMDEBUG
#undef MEMDEBUG

#ifndef MEMDEBUG

void *GetClearedMemory(int size);
void *GetMemory(unsigned long size);

#else

#define GetMemory(size)				GetMemoryDebug(size, #size, __FILE__, __LINE__);
#define GetClearedMemory(size)	GetClearedMemoryDebug(size, #size, __FILE__, __LINE__);
//allocate a memory block of the given size
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line);
//allocate a memory block of the given size and clear it
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line);
//
void PrintMemoryLabels(void);
#endif //MEMDEBUG

void FreeMemory(void *ptr);
int MemorySize(void *ptr);
void PrintMemorySize(unsigned long size);
int TotalAllocatedMemory(void);

