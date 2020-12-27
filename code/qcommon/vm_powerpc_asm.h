/*
===========================================================================
Copyright (C) 2008 Przemyslaw Iskra <sparky@pld-linux.org>

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

#ifndef VM_POWERPC_ASM_H
#define VM_POWERPC_ASM_H

/*
 * Register information according to:
 * http://refspecs.freestandards.org/elf/elfspec_ppc.pdf
 */

#define r0	0	// volatile
#define r1	1	// caller safe ( stack pointer )
#define r2	2	// reserved
#define r3	3	// callee safe
#define r4	4	// callee safe
#define r5	5	// callee safe
#define r6	6	// callee safe
#define r7	7	// callee safe
#define r8	8	// callee safe
#define r9	9	// callee safe
#define r10	10	// callee safe
#define r11	11	// volatile
#define r12	12	// volatile
#define r13	13	// reserved ( small data area )
#define r14	14	// caller safe
#define r15	15	// caller safe
#define r16	16	// caller safe
#define r17	17	// caller safe
#define r18	18	// caller safe
#define r19	19	// caller safe
#define r20	20	// caller safe
#define r21	21	// caller safe
#define r22	22	// caller safe
#define r23	23	// caller safe
#define r24	24	// caller safe
#define r25	25	// caller safe
#define r26	26	// caller safe
#define r27	27	// caller safe
#define r28	28	// caller safe
#define r29	29	// caller safe
#define r30	30	// caller safe
#define r31	31	// caller safe ( environment pointers )

#define f0	0	// callee safe
#define f1	1	// callee safe
#define f2	2	// callee safe
#define f3	3	// callee safe
#define f4	4	// callee safe
#define f5	5	// callee safe
#define f6	6	// callee safe
#define f7	7	// callee safe
#define f8	8	// callee safe
#define f9	9	// callee safe
#define f10	10	// callee safe
#define f11	11	// callee safe
#define f12	12	// callee safe
#define f13	13	// callee safe
#define f14	14	// caller safe
#define f15	15	// caller safe
#define f16	16	// caller safe
#define f17	17	// caller safe
#define f18	18	// caller safe
#define f19	19	// caller safe
#define f20	20	// caller safe
#define f21	21	// caller safe
#define f22	22	// caller safe
#define f23	23	// caller safe
#define f24	24	// caller safe
#define f25	25	// caller safe
#define f26	26	// caller safe
#define f27	27	// caller safe
#define f28	28	// caller safe
#define f29	29	// caller safe
#define f30	30	// caller safe
#define f31	31	// caller safe

#define cr0	0	// volatile
#define cr1	1	// volatile
#define cr2	2	// caller safe
#define cr3	3	// caller safe
#define cr4	4	// caller safe
#define cr5	5	// volatile
#define cr6	6	// volatile
#define cr7	7	// volatile

#define lt	0
#define gt	1
#define eq	2
#define so	3

// branch bo field values
#define branchLikely	1
#define branchFalse	4
#define branchTrue	12
#define branchAlways	20

// branch extensions (change branch type)
#define branchExtLink	0x0001


/*
 * This list must match exactly the powerpc_opcodes list from vm_powerpc_asm.c
 * If you're changing the original list remember to regenerate this one. You
 * may do so using this perl script:
   perl -p -e 'BEGIN{%t=("-"=>m=>"+"=>p=>"."=>d=>);$l=""}$o=0 if/^}/;
	if($o && s/^{ "(.*?)([\.+-])?".+/i\U$1\E$t{$2}/s){$_.="_" while$l{$_};
	$l{$_}=1;if(length $l.$_ > 70){$s=$_;$_="\t$l\n";$l="$s,"}else
	{$l.=" $_,";$_=undef}}else{$o=1 if/powerpc_opcodes.*=/;$_=undef};
	END{print "\t$l\n"}' < vm_powerpc_asm.c
 */

typedef enum powerpc_iname {
	iCMPLWI, iCMPWI, iCMPW, iCMPLW, iFCMPU, iLI, iLIS, iADDI, iADDIS,
	iBLTm, iBC, iBCL, iB, iBL, iBLR, iBCTR, iBCTRL, iRLWINM, iNOP, iORI,
	iXORIS, iLDX, iLWZX, iSLW, iAND, iSUB, iLBZX, iNEG, iNOT, iSTWX, iSTBX,
	iMULLW, iADD, iLHZX, iXOR, iMFLR, iSTHX, iMR, iOR, iDIVWU, iMTLR,
	iMTCTR, iDIVW, iLFSX, iSRW, iSTFSX, iSRAW, iEXTSH, iEXTSB, iLWZ, iLBZ,
	iSTW, iSTWU, iSTB, iLHZ, iSTH, iLFS, iLFD, iSTFS, iSTFD, iLD, iFDIVS,
	iFSUBS, iFADDS, iFMULS, iSTD, iSTDU, iFRSP, iFCTIWZ, iFSUB, iFNEG,
} powerpc_iname_t;

#include <stdint.h>

typedef uint32_t ppc_instruction_t;

extern ppc_instruction_t
asm_instruction( powerpc_iname_t, const int, const long int * );

#define IN( inst, args... ) \
({\
	const long int argv[] = { args };\
	const int argc = sizeof( argv ) / sizeof( argv[0] ); \
	asm_instruction( inst, argc, argv );\
})

#endif
