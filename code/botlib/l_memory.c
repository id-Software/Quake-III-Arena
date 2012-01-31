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

/*****************************************************************************
 * name:		l_memory.c
 *
 * desc:		memory allocation
 *
 * $Archive: /MissionPack/code/botlib/l_memory.c $
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "l_log.h"
#include "be_interface.h"

//#define MEMDEBUG
//#define MEMORYMANEGER

#define MEM_ID		0x12345678l
#define HUNK_ID		0x87654321l

int allocatedmemory;
int totalmemorysize;
int numblocks;

#ifdef MEMORYMANEGER

typedef struct memoryblock_s
{
	unsigned long int id;
	void *ptr;
	int size;
#ifdef MEMDEBUG
	char *label;
	char *file;
	int line;
#endif //MEMDEBUG
	struct memoryblock_s *prev, *next;
} memoryblock_t;

memoryblock_t *memory;

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void LinkMemoryBlock(memoryblock_t *block)
{
	block->prev = NULL;
	block->next = memory;
	if (memory) memory->prev = block;
	memory = block;
} //end of the function LinkMemoryBlock
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void UnlinkMemoryBlock(memoryblock_t *block)
{
	if (block->prev) block->prev->next = block->next;
	else memory = block->next;
	if (block->next) block->next->prev = block->prev;
} //end of the function UnlinkMemoryBlock
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
	memoryblock_t *block;
  assert(botimport.GetMemory); // bk001129 - was NULL'ed
	ptr = botimport.GetMemory(size + sizeof(memoryblock_t));
	block = (memoryblock_t *) ptr;
	block->id = MEM_ID;
	block->ptr = (char *) ptr + sizeof(memoryblock_t);
	block->size = size + sizeof(memoryblock_t);
#ifdef MEMDEBUG
	block->label = label;
	block->file = file;
	block->line = line;
#endif //MEMDEBUG
	LinkMemoryBlock(block);
	allocatedmemory += block->size;
	totalmemorysize += block->size + sizeof(memoryblock_t);
	numblocks++;
	return block->ptr;
} //end of the function GetMemoryDebug
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetClearedMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetMemoryDebug(size, label, file, line);
#else
	ptr = GetMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
} //end of the function GetClearedMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
	memoryblock_t *block;

	ptr = botimport.HunkAlloc(size + sizeof(memoryblock_t));
	block = (memoryblock_t *) ptr;
	block->id = HUNK_ID;
	block->ptr = (char *) ptr + sizeof(memoryblock_t);
	block->size = size + sizeof(memoryblock_t);
#ifdef MEMDEBUG
	block->label = label;
	block->file = file;
	block->line = line;
#endif //MEMDEBUG
	LinkMemoryBlock(block);
	allocatedmemory += block->size;
	totalmemorysize += block->size + sizeof(memoryblock_t);
	numblocks++;
	return block->ptr;
} //end of the function GetHunkMemoryDebug
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetClearedHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetHunkMemoryDebug(size, label, file, line);
#else
	ptr = GetHunkMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
} //end of the function GetClearedHunkMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
memoryblock_t *BlockFromPointer(void *ptr, char *str)
{
	memoryblock_t *block;

	if (!ptr)
	{
#ifdef MEMDEBUG
		//char *crash = (char *) NULL;
		//crash[0] = 1;
		botimport.Print(PRT_FATAL, "%s: NULL pointer\n", str);
#endif // MEMDEBUG
		return NULL;
	} //end if
	block = (memoryblock_t *) ((char *) ptr - sizeof(memoryblock_t));
	if (block->id != MEM_ID && block->id != HUNK_ID)
	{
		botimport.Print(PRT_FATAL, "%s: invalid memory block\n", str);
		return NULL;
	} //end if
	if (block->ptr != ptr)
	{
		botimport.Print(PRT_FATAL, "%s: memory block pointer invalid\n", str);
		return NULL;
	} //end if
	return block;
} //end of the function BlockFromPointer
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeMemory(void *ptr)
{
	memoryblock_t *block;

	block = BlockFromPointer(ptr, "FreeMemory");
	if (!block) return;
	UnlinkMemoryBlock(block);
	allocatedmemory -= block->size;
	totalmemorysize -= block->size + sizeof(memoryblock_t);
	numblocks--;
	//
	if (block->id == MEM_ID)
	{
		botimport.FreeMemory(block);
	} //end if
} //end of the function FreeMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AvailableMemory(void)
{
	return botimport.AvailableMemory();
} //end of the function AvailableMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int MemoryByteSize(void *ptr)
{
	memoryblock_t *block;

	block = BlockFromPointer(ptr, "MemoryByteSize");
	if (!block) return 0;
	return block->size;
} //end of the function MemoryByteSize
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void PrintUsedMemorySize(void)
{
	botimport.Print(PRT_MESSAGE, "total allocated memory: %d KB\n", allocatedmemory >> 10);
	botimport.Print(PRT_MESSAGE, "total botlib memory: %d KB\n", totalmemorysize >> 10);
	botimport.Print(PRT_MESSAGE, "total memory blocks: %d\n", numblocks);
} //end of the function PrintUsedMemorySize
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void PrintMemoryLabels(void)
{
	memoryblock_t *block;
	int i;

	PrintUsedMemorySize();
	i = 0;
	Log_Write("============= Botlib memory log ==============\r\n");
	Log_Write("\r\n");
	for (block = memory; block; block = block->next)
	{
#ifdef MEMDEBUG
		if (block->id == HUNK_ID)
		{
			Log_Write("%6d, hunk %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label);
		} //end if
		else
		{
			Log_Write("%6d,      %p, %8d: %24s line %6d: %s\r\n", i, block->ptr, block->size, block->file, block->line, block->label);
		} //end else
#endif //MEMDEBUG
		i++;
	} //end for
} //end of the function PrintMemoryLabels
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void DumpMemory(void)
{
	memoryblock_t *block;

	for (block = memory; block; block = memory)
	{
		FreeMemory(block->ptr);
	} //end for
	totalmemorysize = 0;
	allocatedmemory = 0;
} //end of the function DumpMemory

#else

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
	unsigned long int *memid;

	ptr = botimport.GetMemory(size + sizeof(unsigned long int));
	if (!ptr) return NULL;
	memid = (unsigned long int *) ptr;
	*memid = MEM_ID;
	return (unsigned long int *) ((char *) ptr + sizeof(unsigned long int));
} //end of the function GetMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetClearedMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetClearedMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetMemoryDebug(size, label, file, line);
#else
	ptr = GetMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
} //end of the function GetClearedMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
	unsigned long int *memid;

	ptr = botimport.HunkAlloc(size + sizeof(unsigned long int));
	if (!ptr) return NULL;
	memid = (unsigned long int *) ptr;
	*memid = HUNK_ID;
	return (unsigned long int *) ((char *) ptr + sizeof(unsigned long int));
} //end of the function GetHunkMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef MEMDEBUG
void *GetClearedHunkMemoryDebug(unsigned long size, char *label, char *file, int line)
#else
void *GetClearedHunkMemory(unsigned long size)
#endif //MEMDEBUG
{
	void *ptr;
#ifdef MEMDEBUG
	ptr = GetHunkMemoryDebug(size, label, file, line);
#else
	ptr = GetHunkMemory(size);
#endif //MEMDEBUG
	Com_Memset(ptr, 0, size);
	return ptr;
} //end of the function GetClearedHunkMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void FreeMemory(void *ptr)
{
	unsigned long int *memid;

	memid = (unsigned long int *) ((char *) ptr - sizeof(unsigned long int));

	if (*memid == MEM_ID)
	{
		botimport.FreeMemory(memid);
	} //end if
} //end of the function FreeMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AvailableMemory(void)
{
	return botimport.AvailableMemory();
} //end of the function AvailableMemory
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void PrintUsedMemorySize(void)
{
} //end of the function PrintUsedMemorySize
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void PrintMemoryLabels(void)
{
} //end of the function PrintMemoryLabels

#endif
