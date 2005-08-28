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
//
// sys_dosa.s
// x86 assembly-language DOS-dependent routines.

#include "qasm.h"


	.data

	.align	4
fpenv:
	.long	0, 0, 0, 0, 0, 0, 0, 0

	.text

.globl C(MaskExceptions)
C(MaskExceptions):
	fnstenv	fpenv
	orl		$0x3F,fpenv
	fldenv	fpenv

	ret

#if 0
.globl C(unmaskexceptions)
C(unmaskexceptions):
	fnstenv	fpenv
	andl		$0xFFFFFFE0,fpenv
	fldenv	fpenv

	ret
#endif

	.data

	.align	4
.globl	ceil_cw, single_cw, full_cw, cw, pushed_cw
ceil_cw:	.long	0
single_cw:	.long	0
full_cw:	.long	0
cw:			.long	0
pushed_cw:	.long	0

	.text

.globl C(Sys_LowFPPrecision)
C(Sys_LowFPPrecision):
	fldcw	single_cw

	ret

.globl C(Sys_HighFPPrecision)
C(Sys_HighFPPrecision):
	fldcw	full_cw

	ret

.globl C(Sys_PushFPCW_SetHigh)
C(Sys_PushFPCW_SetHigh):
	fnstcw	pushed_cw
	fldcw	full_cw

	ret

.globl C(Sys_PopFPCW)
C(Sys_PopFPCW):
	fldcw	pushed_cw

	ret

.globl C(Sys_SetFPCW)
C(Sys_SetFPCW):
	fnstcw	cw
	movl	cw,%eax
#if	id386
	andb	$0xF0,%ah
	orb		$0x03,%ah	// round mode, 64-bit precision
#endif
	movl	%eax,full_cw

#if	id386
	andb	$0xF0,%ah
	orb		$0x0C,%ah	// chop mode, single precision
#endif
	movl	%eax,single_cw

#if	id386
	andb	$0xF0,%ah
	orb		$0x08,%ah	// ceil mode, single precision
#endif
	movl	%eax,ceil_cw

	ret

