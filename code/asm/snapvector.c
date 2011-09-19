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
 * See MASM snapvector.asm for commentary
 */

static unsigned char ssemask[16] __attribute__((aligned(16))) =
{
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00"
};

static const unsigned int ssecw __attribute__((aligned(16))) = 0x00001F80;
static const unsigned short fpucw = 0x037F;

void qsnapvectorsse(vec3_t vec)
{
	uint32_t oldcw __attribute__((aligned(16)));
	
	__asm__ volatile
	(
		"stmxcsr %3\n"
		"ldmxcsr %1\n"

		"movaps (%0), %%xmm1\n"
		"movups (%2), %%xmm0\n"
		"movaps %%xmm0, %%xmm2\n"
		"andps %%xmm1, %%xmm0\n"
		"andnps %%xmm2, %%xmm1\n"
		"cvtps2dq %%xmm0, %%xmm0\n"
		"cvtdq2ps %%xmm0, %%xmm0\n"
		"orps %%xmm1, %%xmm0\n"
		"movups %%xmm0, (%2)\n"
		
		"ldmxcsr %3\n"
		:
		: "r" (ssemask), "m" (ssecw), "r" (vec), "m" (oldcw)
		: "memory", "%xmm0", "%xmm1", "%xmm2"
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
		QROUNDX87("(%1)")
		QROUNDX87("4(%1)")
		QROUNDX87("8(%1)")
		"fldcw (" ESP ")\n"
		"add $2, " ESP "\n"
		:
		: "m" (fpucw), "r" (vec)
		: "memory"
	);
}
