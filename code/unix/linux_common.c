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
/** 
 * GAS syntax equivalents of the MSVC asm memory calls in common.c
 *
 * The following changes have been made to the asm:
 * 1. Registers are loaded by the inline asm arguments when possible
 * 2. Labels have been changed to local label format (0,1,etc.) to allow inlining
 *
 * HISTORY:
 *	AH - Created on 08 Dec 2000
 */

#include <unistd.h>   // AH - for size_t
#include <string.h>

// bk001207 - we need something under Linux, too. Mac?
#if 1 // defined(C_ONLY) // bk010102 - dedicated?
void Com_Memcpy (void* dest, const void* src, const size_t count) {
  memcpy(dest, src, count);
}

void Com_Memset (void* dest, const int val, const size_t count) {
  memset(dest, val, count);
}

#else

typedef enum {
  PRE_READ,         // prefetch assuming that buffer is used for reading only
  PRE_WRITE,        // prefetch assuming that buffer is used for writing only
  PRE_READ_WRITE    // prefetch assuming that buffer is used for both reading and writing
} e_prefetch;

void Com_Prefetch (const void *s, const unsigned int bytes, e_prefetch type);

void _copyDWord (unsigned int* dest, const unsigned int constant, const unsigned int count) {
	// MMX version not used on standard Pentium MMX
	// because the dword version is faster (with
	// proper destination prefetching)
		__asm__ __volatile__ (" \
			//mov			eax,constant		// eax = val \
			//mov			edx,dest			// dest \
			//mov			ecx,count \
			movd		%%eax, %%mm0 \
			punpckldq	%%mm0, %%mm0 \
\
			// ensure that destination is qword aligned \
\
			testl		$7, %%edx				// qword padding?\
			jz		0f	\
			movl		%%eax, (%%edx) \
			decl		%%ecx \
			addl		$4, %%edx \
\
0:			movl		%%ecx, %%ebx				\
			andl		$0xfffffff0, %%ecx	\
			jz		2f \
			jmp		1f \
			.align 		16 \
\
			// funny ordering here to avoid commands \
			// that cross 32-byte boundaries (the \
			// [edx+0] version has a special 3-byte opcode... \
1:			movq		%%mm0, 8(%%edx) \
			movq		%%mm0, 16(%%edx) \
			movq		%%mm0, 24(%%edx) \
			movq		%%mm0, 32(%%edx) \
			movq		%%mm0, 40(%%edx) \
			movq		%%mm0, 48(%%edx) \
			movq		%%mm0, 56(%%edx) \
			movq		%%mm0, (%%edx)\
			addl		$64, %%edx \
			subl		$16, %%ecx \
			jnz		1b \
2:	\
			movl		%%ebx, %%ecx				// ebx = cnt \
			andl		$0xfffffff0, %%ecx				// ecx = cnt&~15 \
			subl		%%ecx, %%ebx \
			jz		6f \
			cmpl		$8, %%ebx \
			jl		3f \
\
			movq		%%mm0, (%%edx) \
			movq		%%mm0, 8(%%edx) \
			movq		%%mm0, 16(%%edx) \
			movq		%%mm0, 24(%%edx) \
			addl		$32, %%edx \
			subl		$8, %%ebx \
			jz		6f \
\
3:			cmpl		$4, %%ebx \
			jl		4f \
			\
			movq		%%mm0, (%%edx) \
			movq		%%mm0, 8(%%edx) \
			addl		$16, %%edx \
			subl		$4, %%ebx \
\
4:			cmpl		$2, %%ebx \
			jl		5f \
			movq		%%mm0, (%%edx) \
			addl		$8, %%edx \
			subl		$2, %%ebx \
\
5:			cmpl		$1, %%ebx \
			jl		6f \
			movl		%%eax, (%%edx) \
6: \
			emms \
	"
	: : "a" (constant), "c" (count), "d" (dest)
	: "%ebx", "%edi", "%esi", "cc", "memory");
}

// optimized memory copy routine that handles all alignment
// cases and block sizes efficiently
void Com_Memcpy (void* dest, const void* src, const size_t count) {
	Com_Prefetch (src, count, PRE_READ);
	__asm__ __volatile__ (" \
		pushl		%%edi \
		pushl		%%esi \
		//mov		ecx,count \
		cmpl		$0, %%ecx						// count = 0 check (just to be on the safe side) \
		je		6f \
		//mov		edx,dest \
		movl		%0, %%ebx \
		cmpl		$32, %%ecx						// padding only? \
		jl		1f \
\
		movl		%%ecx, %%edi					\
		andl		$0xfffffe00, %%edi					// edi = count&~31 \
		subl		$32, %%edi \
\
		.align 16 \
0: \
		movl		(%%ebx, %%edi, 1), %%eax \
		movl		4(%%ebx, %%edi, 1), %%esi \
		movl		%%eax, (%%edx, %%edi, 1) \
		movl		%%esi, 4(%%edx, %%edi, 1) \
		movl		8(%%ebx, %%edi, 1), %%eax \
		movl		12(%%ebx, %%edi, 1), %%esi \
		movl		%%eax, 8(%%edx, %%edi, 1) \
		movl		%%esi, 12(%%edx, %%edi, 1) \
		movl		16(%%ebx, %%edi, 1), %%eax \
		movl		20(%%ebx, %%edi, 1), %%esi \
		movl		%%eax, 16(%%edx, %%edi, 1) \
		movl		%%esi, 20(%%edx, %%edi, 1) \
		movl		24(%%ebx, %%edi, 1), %%eax \
		movl		28(%%ebx, %%edi, 1), %%esi \
		movl		%%eax, 24(%%edx, %%edi, 1) \
		movl		%%esi, 28(%%edx, %%edi, 1) \
		subl		$32, %%edi \
		jge		0b \
		\
		movl		%%ecx, %%edi \
		andl		$0xfffffe00, %%edi \
		addl		%%edi, %%ebx					// increase src pointer \
		addl		%%edi, %%edx					// increase dst pointer \
		andl		$31, %%ecx					// new count \
		jz		6f					// if count = 0, get outta here \
\
1: \
		cmpl		$16, %%ecx \
		jl		2f \
		movl		(%%ebx), %%eax \
		movl		%%eax, (%%edx) \
		movl		4(%%ebx), %%eax \
		movl		%%eax, 4(%%edx) \
		movl		8(%%ebx), %%eax \
		movl		%%eax, 8(%%edx) \
		movl		12(%%ebx), %%eax \
		movl		%%eax, 12(%%edx) \
		subl		$16, %%ecx \
		addl		$16, %%ebx \
		addl		$16, %%edx \
2: \
		cmpl		$8, %%ecx \
		jl		3f \
		movl		(%%ebx), %%eax \
		movl		%%eax, (%%edx) \
		movl		4(%%ebx), %%eax \
		subl		$8, %%ecx \
		movl		%%eax, 4(%%edx) \
		addl		$8, %%ebx \
		addl		$8, %%edx \
3: \
		cmpl		$4, %%ecx \
		jl		4f \
		movl		(%%ebx), %%eax	// here 4-7 bytes \
		addl		$4, %%ebx \
		subl		$4, %%ecx \
		movl		%%eax, (%%edx) \
		addl		$4, %%edx \
4:							// 0-3 remaining bytes \
		cmpl		$2, %%ecx \
		jl		5f \
		movw		(%%ebx), %%ax	// two bytes \
		cmpl		$3, %%ecx				// less than 3? \
		movw		%%ax, (%%edx) \
		jl		6f \
		movb		2(%%ebx), %%al	// last byte \
		movb		%%al, 2(%%edx) \
		jmp		6f \
5: \
		cmpl		$1, %%ecx \
		jl		6f \
		movb		(%%ebx), %%al \
		movb		%%al, (%%edx) \
6: \
		popl		%%esi \
		popl		%%edi \
	"
	: : "m" (src), "d" (dest), "c" (count)
	: "%eax", "%ebx", "%edi", "%esi", "cc", "memory");
}

void Com_Memset (void* dest, const int val, const size_t count)
{
	unsigned int fillval;

	if (count < 8)
	{
		__asm__ __volatile__ (" \
			//mov		edx,dest \
			//mov		eax, val \
			movb		%%al, %%ah \
			movl		%%eax, %%ebx \
			andl		$0xffff, %%ebx \
			shll		$16, %%eax \
			addl		%%ebx, %%eax	// eax now contains pattern \
			//mov		ecx,count \
			cmpl		$4, %%ecx \
			jl		0f \
			movl		%%eax, (%%edx)	// copy first dword \
			addl		$4, %%edx \
			subl		$4, %%ecx \
	0:		cmpl		$2, %%ecx \
			jl		1f \
			movw		%%ax, (%%edx)	// copy 2 bytes \
			addl		$2, %%edx \
			subl		$2, %%ecx \
	1:		cmpl		$0, %%ecx \
			je		2f \
			movb		%%al, (%%edx)	// copy single byte \
	2:		 \
		"
		: : "d" (dest), "a" (val), "c" (count)
		: "%ebx", "%edi", "%esi", "cc", "memory");
		
		return;
	}

	fillval = val;
	
	fillval = fillval|(fillval<<8);
	fillval = fillval|(fillval<<16);		// fill dword with 8-bit pattern

	_copyDWord ((unsigned int*)(dest),fillval, count/4);
	
	__asm__ __volatile__ ("     		// padding of 0-3 bytes \
		//mov		ecx,count \
		movl		%%ecx, %%eax \
		andl		$3, %%ecx \
		jz		1f \
		andl		$0xffffff00, %%eax \
		//mov		ebx,dest \
		addl		%%eax, %%edx \
		movl		%0, %%eax \
		cmpl		$2, %%ecx \
		jl		0f \
		movw		%%ax, (%%edx) \
		cmpl		$2, %%ecx \
		je		1f					\
		movb		%%al, 2(%%edx)		\
		jmp		1f \
0:		\
		cmpl		$0, %%ecx\
		je		1f\
		movb		%%al, (%%edx)\
1:	\
	"
	: : "m" (fillval), "c" (count), "d" (dest)
	: "%eax", "%ebx", "%edi", "%esi", "cc", "memory");	
}

void Com_Prefetch (const void *s, const unsigned int bytes, e_prefetch type)
{
	// write buffer prefetching is performed only if
	// the processor benefits from it. Read and read/write
	// prefetching is always performed.

	switch (type)
	{
		case PRE_WRITE : break;
		case PRE_READ:
		case PRE_READ_WRITE:

		__asm__ __volatile__ ("\
			//mov		ebx,s\
			//mov		ecx,bytes\
			cmpl		$4096, %%ecx				// clamp to 4kB\
			jle		0f\
			movl		$4096, %%ecx\
	0:\
			addl		$0x1f, %%ecx\
			shrl		$5, %%ecx					// number of cache lines\
			jz		2f\
			jmp		1f\
\
			.align 16\
	1:		testb		%%al, (%%edx)\
			addl		$32, %%edx\
			decl		%%ecx\
			jnz		1b\
	2:\
		"
		: : "d" (s), "c" (bytes)
		: "%eax", "%ebx", "%edi", "%esi", "memory", "cc");
		
		break;
	}
}

#endif
