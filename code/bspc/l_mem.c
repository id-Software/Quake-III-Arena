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
#include "l_log.h"

int allocedmemory;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void PrintMemorySize(unsigned long size)
{
	unsigned long number1, number2, number3;
	number1 = size >> 20;
	number2 = (size & 0xFFFFF) >> 10;
	number3 = (size & 0x3FF);
	if (number1) Log_Print("%ld MB", number1);
	if (number1 && number2) Log_Print(" and ");
	if (number2) Log_Print("%ld KB", number2);
	if (number2 && number3) Log_Print(" and ");
	if (number3) Log_Print("%ld bytes", number3);
} //end of the function PrintFileSize

#ifndef MEMDEBUG
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int MemorySize(void *ptr)
{
#if defined(WIN32) || defined(_WIN32)
	#ifdef __WATCOMC__
		//Intel 32 bits memory addressing, 16 bytes aligned
	return (_msize(ptr) + 15) >> 4 << 4;
	#else
	return _msize(ptr);
	#endif
#else
	return 0;
#endif
} //end of the function MemorySize
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *GetClearedMemory(int size)
{
	void *ptr;

	ptr = (void *) malloc(size);
	if (!ptr) Error("out of memory");
	memset(ptr, 0, size);
	allocedmemory += MemorySize(ptr);
	return ptr;
} //end of the function GetClearedMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *GetMemory(unsigned long size)
{
	void *ptr;
	ptr = malloc(size);
	if (!ptr) Error("out of memory");
	allocedmemory += MemorySize(ptr);
	return ptr;
} //end of the function GetMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FreeMemory(void *ptr)
{
	allocedmemory -= MemorySize(ptr);
	free(ptr);
} //end of the function FreeMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int TotalAllocatedMemory(void)
{
	return allocedmemory;
} //end of the function TotalAllocatedMemory

#else

#define MEM_ID		0x12345678l

int totalmemorysize;
int numblocks;

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
// Parameter:				-
// Returns:					-
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
// Parameter:				-
// Returns:					-
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
// Parameter:				-
// Returns:					-
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

	ptr = malloc(size + sizeof(memoryblock_t));
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
	totalmemorysize += block->size;
	numblocks++;
	return block->ptr;
} //end of the function GetMemoryDebug
//===========================================================================
//
// Parameter:				-
// Returns:					-
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
	memset(ptr, 0, size);
	return ptr;
} //end of the function GetClearedMemoryLabelled
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *GetClearedHunkMemory(unsigned long size)
{
	return GetClearedMemory(size);
} //end of the function GetClearedHunkMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *GetHunkMemory(unsigned long size)
{
	return GetMemory(size);
} //end of the function GetHunkMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
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
		Error("%s: NULL pointer\n", str);
#endif MEMDEBUG
		return NULL;
	} //end if
	block = (memoryblock_t *) ((char *) ptr - sizeof(memoryblock_t));
	if (block->id != MEM_ID)
	{
		Error("%s: invalid memory block\n", str);
	} //end if
	if (block->ptr != ptr)
	{
		
		Error("%s: memory block pointer invalid\n", str);
	} //end if
	return block;
} //end of the function BlockFromPointer
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void FreeMemory(void *ptr)
{
	memoryblock_t *block;

	block = BlockFromPointer(ptr, "FreeMemory");
	if (!block) return;
	UnlinkMemoryBlock(block);
	totalmemorysize -= block->size;
	numblocks--;
	//
	free(block);
} //end of the function FreeMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
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
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int MemorySize(void *ptr)
{
	return MemoryByteSize(ptr);
} //end of the function MemorySize
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void PrintUsedMemorySize(void)
{
	printf("total botlib memory: %d KB\n", totalmemorysize >> 10);
	printf("total memory blocks: %d\n", numblocks);
} //end of the function PrintUsedMemorySize
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void PrintMemoryLabels(void)
{
	memoryblock_t *block;
	int i;

	PrintUsedMemorySize();
	i = 0;
	for (block = memory; block; block = block->next)
	{
#ifdef MEMDEBUG
		Log_Write("%6d, %p, %8d: %24s line %6d: %s", i, block->ptr, block->size, block->file, block->line, block->label);
#endif //MEMDEBUG
		i++;
	} //end for
} //end of the function PrintMemoryLabels
//===========================================================================
//
// Parameter:				-
// Returns:					-
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
} //end of the function DumpMemory
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int TotalAllocatedMemory(void)
{
	return totalmemorysize;
} //end of the function TotalAllocatedMemory
#endif

//===========================================================================
// Q3 Hunk and Z_ memory management
//===========================================================================

typedef struct memhunk_s
{
	void *ptr;
	struct memhunk_s *next;
} memhunk_t;

memhunk_t *memhunk_high;
memhunk_t *memhunk_low;
int memhunk_high_size = 16 * 1024 * 1024;
int memhunk_low_size = 0;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Hunk_ClearHigh(void)
{
	memhunk_t *h, *nexth;

	for (h = memhunk_high; h; h = nexth)
	{
		nexth = h->next;
		FreeMemory(h);
	} //end for
	memhunk_high = NULL;
	memhunk_high_size = 16 * 1024 * 1024;
} //end of the function Hunk_ClearHigh
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *Hunk_Alloc(int size)
{
	memhunk_t *h;

	if (!size) return (void *) memhunk_high_size;
	//
	h = GetClearedMemory(size + sizeof(memhunk_t));
	h->ptr = (char *) h + sizeof(memhunk_t);
	h->next = memhunk_high;
	memhunk_high = h;
	memhunk_high_size -= size;
	return h->ptr;
} //end of the function Hunk_Alloc
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void *Z_Malloc(int size)
{
	return GetClearedMemory(size);
} //end of the function Z_Malloc
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Z_Free (void *ptr)
{
	FreeMemory(ptr);
} //end of the function Z_Free
