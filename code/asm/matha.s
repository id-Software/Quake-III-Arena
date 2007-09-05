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
// math.s
// x86 assembly-language math routines.

#include "qasm.h"


#if	id386

	.data

	.align	4
Ljmptab:	.long	Lcase0, Lcase1, Lcase2, Lcase3
			.long	Lcase4, Lcase5, Lcase6, Lcase7

	.text

// TODO: rounding needed?
// stack parameter offset
#define	val	4

.globl C(Invert24To16)
C(Invert24To16):

	movl	val(%esp),%ecx
	movl	$0x100,%edx		// 0x10000000000 as dividend
	cmpl	%edx,%ecx
	jle		LOutOfRange

	subl	%eax,%eax
	divl	%ecx

	ret

LOutOfRange:
	movl	$0xFFFFFFFF,%eax
	ret

#if 0

#define	in	4
#define out	8

	.align 2
.globl C(TransformVector)
C(TransformVector):
	movl	in(%esp),%eax
	movl	out(%esp),%edx

	flds	(%eax)		// in[0]
	fmuls	C(vright)		// in[0]*vright[0]
	flds	(%eax)		// in[0] | in[0]*vright[0]
	fmuls	C(vup)		// in[0]*vup[0] | in[0]*vright[0]
	flds	(%eax)		// in[0] | in[0]*vup[0] | in[0]*vright[0]
	fmuls	C(vpn)		// in[0]*vpn[0] | in[0]*vup[0] | in[0]*vright[0]

	flds	4(%eax)		// in[1] | ...
	fmuls	C(vright)+4	// in[1]*vright[1] | ...
	flds	4(%eax)		// in[1] | in[1]*vright[1] | ...
	fmuls	C(vup)+4		// in[1]*vup[1] | in[1]*vright[1] | ...
	flds	4(%eax)		// in[1] | in[1]*vup[1] | in[1]*vright[1] | ...
	fmuls	C(vpn)+4		// in[1]*vpn[1] | in[1]*vup[1] | in[1]*vright[1] | ...
	fxch	%st(2)		// in[1]*vright[1] | in[1]*vup[1] | in[1]*vpn[1] | ...

	faddp	%st(0),%st(5)	// in[1]*vup[1] | in[1]*vpn[1] | ...
	faddp	%st(0),%st(3)	// in[1]*vpn[1] | ...
	faddp	%st(0),%st(1)	// vpn_accum | vup_accum | vright_accum

	flds	8(%eax)		// in[2] | ...
	fmuls	C(vright)+8	// in[2]*vright[2] | ...
	flds	8(%eax)		// in[2] | in[2]*vright[2] | ...
	fmuls	C(vup)+8		// in[2]*vup[2] | in[2]*vright[2] | ...
	flds	8(%eax)		// in[2] | in[2]*vup[2] | in[2]*vright[2] | ...
	fmuls	C(vpn)+8		// in[2]*vpn[2] | in[2]*vup[2] | in[2]*vright[2] | ...
	fxch	%st(2)		// in[2]*vright[2] | in[2]*vup[2] | in[2]*vpn[2] | ...

	faddp	%st(0),%st(5)	// in[2]*vup[2] | in[2]*vpn[2] | ...
	faddp	%st(0),%st(3)	// in[2]*vpn[2] | ...
	faddp	%st(0),%st(1)	// vpn_accum | vup_accum | vright_accum

	fstps	8(%edx)		// out[2]
	fstps	4(%edx)		// out[1]
	fstps	(%edx)		// out[0]

	ret

#endif

#define EMINS	4+4
#define EMAXS	4+8
#define P		4+12

	.align 2
.globl C(BoxOnPlaneSide)
C(BoxOnPlaneSide):
	pushl	%ebx

	movl	P(%esp),%edx
	movl	EMINS(%esp),%ecx
	xorl	%eax,%eax
	movl	EMAXS(%esp),%ebx
	movb	pl_signbits(%edx),%al
	cmpb	$8,%al
	jge		Lerror
	flds	pl_normal(%edx)		// p->normal[0]
	fld		%st(0)				// p->normal[0] | p->normal[0]
	// bk000422 - warning: missing prefix `*' in absolute indirect address, maybe misassembled!
	// bk001129 - fix from Andrew Henderson, was: Ljmptab(,%eax,4) 
	jmp		*Ljmptab(,%eax,4)


//dist1= p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
//dist2= p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
Lcase0:
	fmuls	(%ebx)				// p->normal[0]*emaxs[0] | p->normal[0]
	flds	pl_normal+4(%edx)	// p->normal[1] | p->normal[0]*emaxs[0] |
								//  p->normal[0]
	fxch	%st(2)				// p->normal[0] | p->normal[0]*emaxs[0] |
								//  p->normal[1]
	fmuls	(%ecx)				// p->normal[0]*emins[0] |
								//  p->normal[0]*emaxs[0] | p->normal[1]
	fxch	%st(2)				// p->normal[1] | p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fld		%st(0)				// p->normal[1] | p->normal[1] |
								//  p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fmuls	4(%ebx)				// p->normal[1]*emaxs[1] | p->normal[1] |
								//  p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	flds	pl_normal+8(%edx)	// p->normal[2] | p->normal[1]*emaxs[1] |
								//  p->normal[1] | p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fxch	%st(2)				// p->normal[1] | p->normal[1]*emaxs[1] |
								//  p->normal[2] | p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fmuls	4(%ecx)				// p->normal[1]*emins[1] |
								//  p->normal[1]*emaxs[1] |
								//  p->normal[2] | p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fxch	%st(2)				// p->normal[2] | p->normal[1]*emaxs[1] |
								//  p->normal[1]*emins[1] |
								//  p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fld		%st(0)				// p->normal[2] | p->normal[2] |
								//  p->normal[1]*emaxs[1] |
								//  p->normal[1]*emins[1] |
								//  p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fmuls	8(%ebx)				// p->normal[2]*emaxs[2] |
								//  p->normal[2] |
								//  p->normal[1]*emaxs[1] |
								//  p->normal[1]*emins[1] |
								//  p->normal[0]*emaxs[0] |
								//  p->normal[0]*emins[0]
	fxch	%st(5)				// p->normal[0]*emins[0] |
								//  p->normal[2] |
								//  p->normal[1]*emaxs[1] |
								//  p->normal[1]*emins[1] |
								//  p->normal[0]*emaxs[0] |
								//  p->normal[2]*emaxs[2]
	faddp	%st(0),%st(3)		//p->normal[2] |
								// p->normal[1]*emaxs[1] |
								// p->normal[1]*emins[1]+p->normal[0]*emins[0]|
								// p->normal[0]*emaxs[0] |
								// p->normal[2]*emaxs[2]
	fmuls	8(%ecx)				//p->normal[2]*emins[2] |
								// p->normal[1]*emaxs[1] |
								// p->normal[1]*emins[1]+p->normal[0]*emins[0]|
								// p->normal[0]*emaxs[0] |
								// p->normal[2]*emaxs[2]
	fxch	%st(1)				//p->normal[1]*emaxs[1] |
								// p->normal[2]*emins[2] |
								// p->normal[1]*emins[1]+p->normal[0]*emins[0]|
								// p->normal[0]*emaxs[0] |
								// p->normal[2]*emaxs[2]
	faddp	%st(0),%st(3)		//p->normal[2]*emins[2] |
								// p->normal[1]*emins[1]+p->normal[0]*emins[0]|
								// p->normal[0]*emaxs[0]+p->normal[1]*emaxs[1]|
								// p->normal[2]*emaxs[2]
	fxch	%st(3)				//p->normal[2]*emaxs[2] +
								// p->normal[1]*emins[1]+p->normal[0]*emins[0]|
								// p->normal[0]*emaxs[0]+p->normal[1]*emaxs[1]|
								// p->normal[2]*emins[2]
	faddp	%st(0),%st(2)		//p->normal[1]*emins[1]+p->normal[0]*emins[0]|
								// dist1 | p->normal[2]*emins[2]

	jmp		LSetSides

//dist1= p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
//dist2= p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
Lcase1:
	fmuls	(%ecx)				// emins[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ebx)				// emaxs[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ebx)				// emaxs[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ecx)				// emins[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

	jmp		LSetSides

//dist1= p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
//dist2= p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
Lcase2:
	fmuls	(%ebx)				// emaxs[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ecx)				// emins[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ecx)				// emins[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ebx)				// emaxs[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

	jmp		LSetSides

//dist1= p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
//dist2= p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
Lcase3:
	fmuls	(%ecx)				// emins[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ebx)				// emaxs[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ecx)				// emins[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ebx)				// emaxs[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

	jmp		LSetSides

//dist1= p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
//dist2= p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
Lcase4:
	fmuls	(%ebx)				// emaxs[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ecx)				// emins[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ebx)				// emaxs[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ecx)				// emins[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

	jmp		LSetSides

//dist1= p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
//dist2= p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
Lcase5:
	fmuls	(%ecx)				// emins[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ebx)				// emaxs[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ebx)				// emaxs[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ecx)				// emins[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

	jmp		LSetSides

//dist1= p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
//dist2= p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
Lcase6:
	fmuls	(%ebx)				// emaxs[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ecx)				// emins[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ecx)				// emins[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ebx)				// emaxs[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

	jmp		LSetSides

//dist1= p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
//dist2= p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
Lcase7:
	fmuls	(%ecx)				// emins[0]
	flds	pl_normal+4(%edx)
	fxch	%st(2)
	fmuls	(%ebx)				// emaxs[0]
	fxch	%st(2)
	fld		%st(0)
	fmuls	4(%ecx)				// emins[1]
	flds	pl_normal+8(%edx)
	fxch	%st(2)
	fmuls	4(%ebx)				// emaxs[1]
	fxch	%st(2)
	fld		%st(0)
	fmuls	8(%ecx)				// emins[2]
	fxch	%st(5)
	faddp	%st(0),%st(3)
	fmuls	8(%ebx)				// emaxs[2]
	fxch	%st(1)
	faddp	%st(0),%st(3)
	fxch	%st(3)
	faddp	%st(0),%st(2)

LSetSides:

//	sides = 0;
//	if (dist1 >= p->dist)
//		sides = 1;
//	if (dist2 < p->dist)
//		sides |= 2;

	faddp	%st(0),%st(2)		// dist1 | dist2
	fcomps	pl_dist(%edx)
	xorl	%ecx,%ecx
	fnstsw	%ax
	fcomps	pl_dist(%edx)
	andb	$1,%ah
	xorb	$1,%ah
	addb	%ah,%cl

	fnstsw	%ax
	andb	$1,%ah
	addb	%ah,%ah
	addb	%ah,%cl

//	return sides;

	popl	%ebx
	movl	%ecx,%eax	// return status

	ret


Lerror:
	movl	1, %eax
	ret

#endif	// id386
