/*
===========================================================================
Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>

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

#include "qasm-inline.h"
#include "../qcommon/q_shared.h"

/*
 * GNU inline asm version of qsnapvector
 */

static unsigned char ssemask[16] __attribute__((aligned(16))) =
{
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00"
};

static unsigned int ssecw __attribute__((aligned(16))) = 0x00001F80;
static unsigned short fpucw = 0x037F;

void qsnapvectorsse(vec3_t vec)
{
	__asm__ volatile
	(
		"sub $4, " ESP "\n"
		"stmxcsr (" ESP ")\n"
		"ldmxcsr %1\n"

		"movaps (%0), %%xmm1\n"
		"movups (" EDI "), %%xmm0\n"
		"cvtps2dq %%xmm0, %%xmm0\n"
		"cvtdq2ps %%xmm0, %%xmm0\n"
		"maskmovdqu %%xmm1, %%xmm0\n"
		
		"ldmxcsr (" ESP ")\n"
		"add $4, " ESP "\n"
		:
		: "r" (ssemask), "m" (ssecw), "D" (vec)
		: "memory", "%xmm0", "%xmm1"
	);
}

#define QROUNDX87(src) \
	"flds " src "\n" \
	"fistp " src "\n" \
	"fild " src "\n" \
	"fstp " src "\n"	

void qsnapvectorx87(vec3_t vec)
{
	__asm__ volatile
	(
        	"sub $2, " ESP "\n"
        	"fnstcw (" ESP ")\n"
        	"fldcw %0\n"
        	QROUNDX87("(" EAX ")")
        	QROUNDX87("4(" EAX ")")
        	QROUNDX87("8(" EAX ")")
        	"fldcw (" ESP ")\n"
        	"add $2, " ESP "\n"
        	:
        	: "m" (fpucw), "a" (vec)
        	: "memory"
	);
}
