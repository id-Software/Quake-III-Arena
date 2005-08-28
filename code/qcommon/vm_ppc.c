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
// vm_ppc.c
// ppc dynamic compiler

#include "vm_local.h"

#pragma opt_pointer_analysis off


typedef enum {
    R_REAL_STACK = 1,
	// registers 3-11 are the parameter passing registers
	
	// state
	R_STACK = 3,			// local
	R_OPSTACK,				// global

	// constants	
	R_MEMBASE,			// global
	R_MEMMASK,
	R_ASMCALL,			// global
    R_INSTRUCTIONS,		// global
    R_NUM_INSTRUCTIONS,	// global
    R_CVM,				// currentVM
    
	// temps
	R_TOP = 11,
	R_SECOND = 12,
	R_EA = 2				// effective address calculation
	 
} regNums_t;

#define	RG_REAL_STACK		r1
#define	RG_STACK			r3
#define	RG_OPSTACK			r4
#define	RG_MEMBASE			r5
#define	RG_MEMMASK			r6
#define	RG_ASMCALL			r7
#define	RG_INSTRUCTIONS		r8
#define	RG_NUM_INSTRUCTIONS	r9
#define	RG_CVM				r10
#define	RG_TOP				r12
#define	RG_SECOND			r13
#define	RG_EA 				r14

// this doesn't have the low order bits set for instructions i'm not using...
typedef enum {
	PPC_TDI		= 0x08000000,
	PPC_TWI		= 0x0c000000,
	PPC_MULLI	= 0x1c000000,
	PPC_SUBFIC	= 0x20000000,
	PPC_CMPI	= 0x28000000,
	PPC_CMPLI	= 0x2c000000,
	PPC_ADDIC	= 0x30000000,
	PPC_ADDIC_	= 0x34000000,
	PPC_ADDI	= 0x38000000,
	PPC_ADDIS	= 0x3c000000,
	PPC_BC		= 0x40000000,
	PPC_SC		= 0x44000000,
	PPC_B		= 0x48000000,

	PPC_MCRF	= 0x4c000000,
	PPC_BCLR	= 0x4c000020,
	PPC_RFID	= 0x4c000000,
	PPC_CRNOR	= 0x4c000000,
	PPC_RFI		= 0x4c000000,
	PPC_CRANDC	= 0x4c000000,
	PPC_ISYNC	= 0x4c000000,
	PPC_CRXOR	= 0x4c000000,
	PPC_CRNAND	= 0x4c000000,
	PPC_CREQV	= 0x4c000000,
	PPC_CRORC	= 0x4c000000,
	PPC_CROR	= 0x4c000000,
//------------
	PPC_BCCTR	= 0x4c000420,
	PPC_RLWIMI	= 0x50000000,
	PPC_RLWINM	= 0x54000000,
	PPC_RLWNM	= 0x5c000000,
	PPC_ORI		= 0x60000000,
	PPC_ORIS	= 0x64000000,
	PPC_XORI	= 0x68000000,
	PPC_XORIS	= 0x6c000000,
	PPC_ANDI_	= 0x70000000,
	PPC_ANDIS_	= 0x74000000,
	PPC_RLDICL	= 0x78000000,
	PPC_RLDICR	= 0x78000000,
	PPC_RLDIC	= 0x78000000,
	PPC_RLDIMI	= 0x78000000,
	PPC_RLDCL	= 0x78000000,
	PPC_RLDCR	= 0x78000000,
	PPC_CMP		= 0x7c000000,
	PPC_TW		= 0x7c000000,
	PPC_SUBFC	= 0x7c000010,
	PPC_MULHDU	= 0x7c000000,
	PPC_ADDC	= 0x7c000014,
	PPC_MULHWU	= 0x7c000000,
	PPC_MFCR	= 0x7c000000,
	PPC_LWAR	= 0x7c000000,
	PPC_LDX		= 0x7c000000,
	PPC_LWZX	= 0x7c00002e,
	PPC_SLW		= 0x7c000030,
	PPC_CNTLZW	= 0x7c000000,
	PPC_SLD		= 0x7c000000,
	PPC_AND		= 0x7c000038,
	PPC_CMPL	= 0x7c000040,
	PPC_SUBF	= 0x7c000050,
	PPC_LDUX	= 0x7c000000,
//------------
	PPC_DCBST	= 0x7c000000,
	PPC_LWZUX	= 0x7c00006c,
	PPC_CNTLZD	= 0x7c000000,
	PPC_ANDC	= 0x7c000000,
	PPC_TD		= 0x7c000000,
	PPC_MULHD	= 0x7c000000,
	PPC_MULHW	= 0x7c000000,
	PPC_MTSRD	= 0x7c000000,
	PPC_MFMSR	= 0x7c000000,
	PPC_LDARX	= 0x7c000000,
	PPC_DCBF	= 0x7c000000,
	PPC_LBZX	= 0x7c0000ae,
	PPC_NEG		= 0x7c000000,
	PPC_MTSRDIN	= 0x7c000000,
	PPC_LBZUX	= 0x7c000000,
	PPC_NOR		= 0x7c0000f8,
	PPC_SUBFE	= 0x7c000000,
	PPC_ADDE	= 0x7c000000,
	PPC_MTCRF	= 0x7c000000,
	PPC_MTMSR	= 0x7c000000,
	PPC_STDX	= 0x7c000000,
	PPC_STWCX_	= 0x7c000000,
	PPC_STWX	= 0x7c00012e,
	PPC_MTMSRD	= 0x7c000000,
	PPC_STDUX	= 0x7c000000,
	PPC_STWUX	= 0x7c00016e,
	PPC_SUBFZE	= 0x7c000000,
	PPC_ADDZE	= 0x7c000000,
	PPC_MTSR	= 0x7c000000,
	PPC_STDCX_	= 0x7c000000,
	PPC_STBX	= 0x7c0001ae,
	PPC_SUBFME	= 0x7c000000,
	PPC_MULLD	= 0x7c000000,
//------------
	PPC_ADDME	= 0x7c000000,
	PPC_MULLW	= 0x7c0001d6,
	PPC_MTSRIN	= 0x7c000000,
	PPC_DCBTST	= 0x7c000000,
	PPC_STBUX	= 0x7c000000,
	PPC_ADD		= 0x7c000214,
	PPC_DCBT	= 0x7c000000,
	PPC_LHZX	= 0x7c00022e,
	PPC_EQV		= 0x7c000000,
	PPC_TLBIE	= 0x7c000000,
	PPC_ECIWX	= 0x7c000000,
	PPC_LHZUX	= 0x7c000000,
	PPC_XOR		= 0x7c000278,
	PPC_MFSPR	= 0x7c0002a6,
	PPC_LWAX	= 0x7c000000,
	PPC_LHAX	= 0x7c000000,
	PPC_TLBIA	= 0x7c000000,
	PPC_MFTB	= 0x7c000000,
	PPC_LWAUX	= 0x7c000000,
	PPC_LHAUX	= 0x7c000000,
	PPC_STHX	= 0x7c00032e,
	PPC_ORC		= 0x7c000338,
	PPC_SRADI	= 0x7c000000,
	PPC_SLBIE	= 0x7c000000,
	PPC_ECOWX	= 0x7c000000,
	PPC_STHUX	= 0x7c000000,
	PPC_OR		= 0x7c000378,
	PPC_DIVDU	= 0x7c000000,
	PPC_DIVWU	= 0x7c000396,
	PPC_MTSPR	= 0x7c0003a6,
	PPC_DCBI	= 0x7c000000,
	PPC_NAND	= 0x7c000000,
	PPC_DIVD	= 0x7c000000,
//------------
	PPC_DIVW	= 0x7c0003d6,
	PPC_SLBIA	= 0x7c000000,
	PPC_MCRXR	= 0x7c000000,
	PPC_LSWX	= 0x7c000000,
	PPC_LWBRX	= 0x7c000000,
	PPC_LFSX	= 0x7c000000,
	PPC_SRW		= 0x7c000430,
	PPC_SRD		= 0x7c000000,
	PPC_TLBSYNC	= 0x7c000000,
	PPC_LFSUX	= 0x7c000000,
	PPC_MFSR	= 0x7c000000,
	PPC_LSWI	= 0x7c000000,
	PPC_SYNC	= 0x7c000000,
	PPC_LFDX	= 0x7c000000,
	PPC_LFDUX	= 0x7c000000,
	PPC_MFSRIN	= 0x7c000000,
	PPC_STSWX	= 0x7c000000,
	PPC_STWBRX	= 0x7c000000,
	PPC_STFSX	= 0x7c000000,
	PPC_STFSUX	= 0x7c000000,
	PPC_STSWI	= 0x7c000000,
	PPC_STFDX	= 0x7c000000,
	PPC_DCBA	= 0x7c000000,
	PPC_STFDUX	= 0x7c000000,
	PPC_LHBRX	= 0x7c000000,
	PPC_SRAW	= 0x7c000630,
	PPC_SRAD	= 0x7c000000,
	PPC_SRAWI	= 0x7c000000,
	PPC_EIEIO	= 0x7c000000,
	PPC_STHBRX	= 0x7c000000,
	PPC_EXTSH	= 0x7c000734,
	PPC_EXTSB	= 0x7c000774,
	PPC_ICBI	= 0x7c000000,
//------------
	PPC_STFIWX	= 0x7c0007ae,
	PPC_EXTSW	= 0x7c000000,
	PPC_DCBZ	= 0x7c000000,
	PPC_LWZ		= 0x80000000,
	PPC_LWZU	= 0x84000000,
	PPC_LBZ		= 0x88000000,
	PPC_LBZU	= 0x8c000000,
	PPC_STW		= 0x90000000,
	PPC_STWU	= 0x94000000,
	PPC_STB		= 0x98000000,
	PPC_STBU	= 0x9c000000,
	PPC_LHZ		= 0xa0000000,
	PPC_LHZU	= 0xa4000000,
	PPC_LHA		= 0xa8000000,
	PPC_LHAU	= 0xac000000,
	PPC_STH		= 0xb0000000,
	PPC_STHU	= 0xb4000000,
	PPC_LMW		= 0xb8000000,
	PPC_STMW	= 0xbc000000,
	PPC_LFS		= 0xc0000000,
	PPC_LFSU	= 0xc4000000,
	PPC_LFD		= 0xc8000000,
	PPC_LFDU	= 0xcc000000,
	PPC_STFS	= 0xd0000000,
	PPC_STFSU	= 0xd4000000,
	PPC_STFD	= 0xd8000000,
	PPC_STFDU	= 0xdc000000,
	PPC_LD		= 0xe8000000,
	PPC_LDU		= 0xe8000001,
	PPC_LWA		= 0xe8000002,
	PPC_FDIVS	= 0xec000024,
	PPC_FSUBS	= 0xec000028,
	PPC_FADDS	= 0xec00002a,
//------------
	PPC_FSQRTS	= 0xec000000,
	PPC_FRES	= 0xec000000,
	PPC_FMULS	= 0xec000032,
	PPC_FMSUBS	= 0xec000000,
	PPC_FMADDS	= 0xec000000,
	PPC_FNMSUBS	= 0xec000000,
	PPC_FNMADDS	= 0xec000000,
	PPC_STD		= 0xf8000000,
	PPC_STDU	= 0xf8000001,
	PPC_FCMPU	= 0xfc000000,
	PPC_FRSP	= 0xfc000018,
	PPC_FCTIW	= 0xfc000000,
	PPC_FCTIWZ	= 0xfc00001e,
	PPC_FDIV	= 0xfc000000,
	PPC_FSUB	= 0xfc000028,
	PPC_FADD	= 0xfc000000,
	PPC_FSQRT	= 0xfc000000,
	PPC_FSEL	= 0xfc000000,
	PPC_FMUL	= 0xfc000000,
	PPC_FRSQRTE	= 0xfc000000,
	PPC_FMSUB	= 0xfc000000,
	PPC_FMADD	= 0xfc000000,
	PPC_FNMSUB	= 0xfc000000,
	PPC_FNMADD	= 0xfc000000,
	PPC_FCMPO	= 0xfc000000,
	PPC_MTFSB1	= 0xfc000000,
	PPC_FNEG	= 0xfc000050,
	PPC_MCRFS	= 0xfc000000,
	PPC_MTFSB0	= 0xfc000000,
	PPC_FMR		= 0xfc000000,
	PPC_MTFSFI	= 0xfc000000,
	PPC_FNABS	= 0xfc000000,
	PPC_FABS	= 0xfc000000,
//------------
	PPC_MFFS	= 0xfc000000,
	PPC_MTFSF	= 0xfc000000,
	PPC_FCTID	= 0xfc000000,
	PPC_FCTIDZ	= 0xfc000000,
	PPC_FCFID	= 0xfc000000
	
} ppcOpcodes_t;


// the newly generated code
static	unsigned	*buf;
static	int		compiledOfs;	// in dwords

// fromt the original bytecode
static	byte	*code;
static	int		pc;

void AsmCall( void );

double	itofConvert[2];

static int	Constant4( void ) {
	int		v;

	v = code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24);
	pc += 4;
	return v;
}

static int	Constant1( void ) {
	int		v;

	v = code[pc];
	pc += 1;
	return v;
}

static void Emit4( int i ) {
	buf[ compiledOfs ] = i;
	compiledOfs++;
}

static void Inst( int opcode, int destReg, int aReg, int bReg ) {
    unsigned		r;

    r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( bReg << 11 ) ;
    buf[ compiledOfs ] = r;
    compiledOfs++;
}

static void Inst4( int opcode, int destReg, int aReg, int bReg, int cReg ) {
    unsigned		r;

    r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( bReg << 11 ) | ( cReg << 6 );
    buf[ compiledOfs ] = r;
    compiledOfs++;
}

static void InstImm( int opcode, int destReg, int aReg, int immediate ) {
	unsigned		r;
	
	if ( immediate > 32767 || immediate < -32768 ) {
	    Com_Error( ERR_FATAL, "VM_Compile: immediate value %i out of range, opcode %x,%d,%d", immediate, opcode, destReg, aReg );
	}
	r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( immediate & 0xffff );
	buf[ compiledOfs ] = r;
	compiledOfs++;
}

static void InstImmU( int opcode, int destReg, int aReg, int immediate ) {
	unsigned		r;
	
	if ( immediate > 0xffff || immediate < 0 ) {
		Com_Error( ERR_FATAL, "VM_Compile: immediate value %i out of range", immediate );
	}
    r = opcode | ( destReg << 21 ) | ( aReg << 16 ) | ( immediate & 0xffff );
	buf[ compiledOfs ] = r;
	compiledOfs++;
}

static qboolean	rtopped;
static int pop0, pop1, oc0, oc1;
static vm_t *tvm;
static int instruction;
static byte *jused;
static int pass;

static void ltop() {
    if (rtopped == qfalse) {
	InstImm( PPC_LWZ, R_TOP, R_OPSTACK, 0 );		// get value from opstack
    }
}

static void ltopandsecond() {
    if (pass>=0 && buf[compiledOfs-1] == (PPC_STWU |  R_TOP<<21 | R_OPSTACK<<16 | 4 ) && jused[instruction]==0 ) {
	compiledOfs--;
	if (!pass) {
	    tvm->instructionPointers[instruction] = compiledOfs * 4;
	}
	InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
    } else if (pass>=0 && buf[compiledOfs-1] == (PPC_STW |  R_TOP<<21 | R_OPSTACK<<16 | 0 )  && jused[instruction]==0 ) {
	compiledOfs--;
	if (!pass) {
	    tvm->instructionPointers[instruction] = compiledOfs * 4;
	}
	InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, -4 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
    } else {
	ltop();		// get value from opstack
	InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, -4 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
    }
    rtopped = qfalse;
}

// TJW: Unused
#if 0
static void fltop() {
    if (rtopped == qfalse) {
	InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
    }
}
#endif

static void fltopandsecond() {
	InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
	InstImm( PPC_LFS, R_SECOND, R_OPSTACK, -4 );	// get value from opstack
	InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
    rtopped = qfalse;
	return;
}

/*
=================
VM_Compile
=================
*/
void VM_Compile( vm_t *vm, vmHeader_t *header ) {
	int		op;
	int		maxLength;
	int		v;
	int		i;
        
	// set up the into-to-float variables
   	((int *)itofConvert)[0] = 0x43300000;
   	((int *)itofConvert)[1] = 0x80000000;
   	((int *)itofConvert)[2] = 0x43300000;

	// allocate a very large temp buffer, we will shrink it later
	maxLength = header->codeLength * 8;
	buf = Z_Malloc( maxLength );
	jused = Z_Malloc(header->instructionCount + 2);
	Com_Memset(jused, 0, header->instructionCount+2);
	
    // compile everything twice, so the second pass will have valid instruction
    // pointers for branches
    for ( pass = -1 ; pass < 2 ; pass++ ) {

	rtopped = qfalse;
        // translate all instructions
        pc = 0;
	
	pop0 = 343545;
	pop1 = 2443545;
	oc0 = -2343535;
	oc1 = 24353454;
	tvm = vm;
        code = (byte *)header + header->codeOffset;
        compiledOfs = 0;
#ifndef __GNUC__
		// metrowerks seems to require this header in front of functions
		Emit4( (int)(buf+2) );
		Emit4( 0 );
#endif

		for ( instruction = 0 ; instruction < header->instructionCount ; instruction++ ) {
            if ( compiledOfs*4 > maxLength - 16 ) {
                Com_Error( ERR_DROP, "VM_Compile: maxLength exceeded" );
            }

            op = code[ pc ];
            if ( !pass ) {
                vm->instructionPointers[ instruction ] = compiledOfs * 4;
            }
            pc++;
            switch ( op ) {
            case 0:
                break;
            case OP_BREAK:
                InstImmU( PPC_ADDI, R_TOP, 0, 0 );
                InstImm( PPC_LWZ, R_TOP, R_TOP, 0 );			// *(int *)0 to crash to debugger
		rtopped = qfalse;
                break;
            case OP_ENTER:
                InstImm( PPC_ADDI, R_STACK, R_STACK, -Constant4() );	// sub R_STACK, R_STACK, imm
		rtopped = qfalse;
                break;
            case OP_CONST:
                v = Constant4();
		if (code[pc] == OP_LOAD4 || code[pc] == OP_LOAD2 || code[pc] == OP_LOAD1) {
		    v &= vm->dataMask;
		}
                if ( v < 32768 && v >= -32768 ) {
                    InstImmU( PPC_ADDI, R_TOP, 0, v & 0xffff );
                } else {
                    InstImmU( PPC_ADDIS, R_TOP, 0, (v >> 16)&0xffff );
                    if ( v & 0xffff ) {
                        InstImmU( PPC_ORI, R_TOP, R_TOP, v & 0xffff );
                    }
                }
		if (code[pc] == OP_LOAD4) {
		    Inst( PPC_LWZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
		    pc++;
		    instruction++;
		} else if (code[pc] == OP_LOAD2) {
		    Inst( PPC_LHZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
		    pc++;
		    instruction++;
		} else if (code[pc] == OP_LOAD1) {
		    Inst( PPC_LBZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
		    pc++;
		    instruction++;
		}
		if (code[pc] == OP_STORE4) {
		    InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );	// get value from opstack
		    InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		    //Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );	// mask it
		    Inst( PPC_STWX, R_TOP, R_SECOND, R_MEMBASE );	// store from memory base
		    pc++;
		    instruction++;
		    rtopped = qfalse;
		    break;
		} else if (code[pc] == OP_STORE2) {
		    InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );	// get value from opstack
		    InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		    //Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );	// mask it
		    Inst( PPC_STHX, R_TOP, R_SECOND, R_MEMBASE );	// store from memory base
		    pc++;
		    instruction++;
		    rtopped = qfalse;
		    break;
		} else if (code[pc] == OP_STORE1) {
		    InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );	// get value from opstack
		    InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		    //Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );	// mask it
		    Inst( PPC_STBX, R_TOP, R_SECOND, R_MEMBASE );	// store from memory base
		    pc++;
		    instruction++;
		    rtopped = qfalse;
		    break;
		}
		if (code[pc] == OP_JUMP) {
		    jused[v] = 1;
		}
		InstImm( PPC_STWU, R_TOP, R_OPSTACK, 4 );
		rtopped = qtrue;
		break;
            case OP_LOCAL:
		oc0 = oc1;
		oc1 = Constant4();
		if (code[pc] == OP_LOAD4 || code[pc] == OP_LOAD2 || code[pc] == OP_LOAD1) {
		    oc1 &= vm->dataMask;
		}
                InstImm( PPC_ADDI, R_TOP, R_STACK, oc1 );
		if (code[pc] == OP_LOAD4) {
		    Inst( PPC_LWZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
		    pc++;
		    instruction++;
		} else if (code[pc] == OP_LOAD2) {
		    Inst( PPC_LHZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
		    pc++;
		    instruction++;
		} else if (code[pc] == OP_LOAD1) {
		    Inst( PPC_LBZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
		    pc++;
		    instruction++;
		}
		if (code[pc] == OP_STORE4) {
		    InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );		// get value from opstack
		    InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		    //Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );	// mask it
		    Inst( PPC_STWX, R_TOP, R_SECOND, R_MEMBASE );	// store from memory base
		    pc++;
		    instruction++;
		    rtopped = qfalse;
		    break;
		} else if (code[pc] == OP_STORE2) {
		    InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );		// get value from opstack
		    InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		    //Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );	// mask it
		    Inst( PPC_STHX, R_TOP, R_SECOND, R_MEMBASE );	// store from memory base
		    pc++;
		    instruction++;
		    rtopped = qfalse;
		    break;
		} else if (code[pc] == OP_STORE1) {
		    InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, 0 );		// get value from opstack
		    InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		    //Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );	// mask it
		    Inst( PPC_STBX, R_TOP, R_SECOND, R_MEMBASE );	// store from memory base
		    pc++;
		    instruction++;
		    rtopped = qfalse;
		    break;
		}
                InstImm( PPC_STWU, R_TOP, R_OPSTACK, 4 );
		rtopped = qtrue;
                break;
            case OP_ARG:
                ltop();							// get value from opstack
                InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
                InstImm( PPC_ADDI, R_EA, R_STACK, Constant1() );	// location to put it
                Inst( PPC_STWX, R_TOP, R_EA, R_MEMBASE );
		rtopped = qfalse;
                break;
            case OP_CALL:
                Inst( PPC_MFSPR, R_SECOND, 8, 0 );			// move from link register
                InstImm( PPC_STWU, R_SECOND, R_REAL_STACK, -16 );	// save off the old return address

                Inst( PPC_MTSPR, R_ASMCALL, 9, 0 );			// move to count register
                Inst( PPC_BCCTR | 1, 20, 0, 0 );			// jump and link to the count register

                InstImm( PPC_LWZ, R_SECOND, R_REAL_STACK, 0 );		// fetch the old return address
                InstImm( PPC_ADDI, R_REAL_STACK, R_REAL_STACK, 16 );
                Inst( PPC_MTSPR, R_SECOND, 8, 0 );			// move to link register
		rtopped = qfalse;
                break;
            case OP_PUSH:
                InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, 4 );
		rtopped = qfalse;
                break;
            case OP_POP:
                InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
		rtopped = qfalse;
                break;
            case OP_LEAVE:
                InstImm( PPC_ADDI, R_STACK, R_STACK, Constant4() );	// add R_STACK, R_STACK, imm
                Inst( PPC_BCLR, 20, 0, 0 );							// branch unconditionally to link register
		rtopped = qfalse;
                break;
            case OP_LOAD4:
                ltop();							// get value from opstack
		//Inst( PPC_AND, R_MEMMASK, R_TOP, R_TOP );		// mask it
                Inst( PPC_LWZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );
		rtopped = qtrue;
                break;
            case OP_LOAD2:
                ltop();							// get value from opstack
		//Inst( PPC_AND, R_MEMMASK, R_TOP, R_TOP );		// mask it
                Inst( PPC_LHZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );
		rtopped = qtrue;
                break;
            case OP_LOAD1:
                ltop();							// get value from opstack
		//Inst( PPC_AND, R_MEMMASK, R_TOP, R_TOP );		// mask it
                Inst( PPC_LBZX, R_TOP, R_TOP, R_MEMBASE );		// load from memory base
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );
		rtopped = qtrue;
                break;
            case OP_STORE4:
                ltopandsecond();					// get value from opstack
		//Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );		// mask it
                Inst( PPC_STWX, R_TOP, R_SECOND, R_MEMBASE );		// store from memory base
		rtopped = qfalse;
                break;
            case OP_STORE2:
                ltopandsecond();					// get value from opstack
		//Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );		// mask it
                Inst( PPC_STHX, R_TOP, R_SECOND, R_MEMBASE );		// store from memory base
		rtopped = qfalse;
                break;
            case OP_STORE1:
                ltopandsecond();					// get value from opstack
		//Inst( PPC_AND, R_MEMMASK, R_SECOND, R_SECOND );		// mask it
                Inst( PPC_STBX, R_TOP, R_SECOND, R_MEMBASE );		// store from memory base
		rtopped = qfalse;
                break;

            case OP_EQ:
                ltopandsecond();					// get value from opstack
                Inst( PPC_CMP, 0, R_SECOND, R_TOP );
                i = Constant4();
				jused[i] = 1;
                InstImm( PPC_BC, 4, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];                    
                } else {
                    v = 0;             
                }
                Emit4(PPC_B | (v&0x3ffffff) );
				rtopped = qfalse;
                break;
            case OP_NE:
                ltopandsecond();					// get value from opstack
                Inst( PPC_CMP, 0, R_SECOND, R_TOP );
                i = Constant4();
				jused[i] = 1;
                InstImm( PPC_BC, 12, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];                    
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 2, v );

				rtopped = qfalse;
                break;
            case OP_LTI:
                ltopandsecond();					// get value from opstack
                Inst( PPC_CMP, 0, R_SECOND, R_TOP );
                i = Constant4();
				jused[i] = 1;
                InstImm( PPC_BC, 4, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 0, v );
				rtopped = qfalse;
                break;
            case OP_LEI:
                ltopandsecond();					// get value from opstack
                Inst( PPC_CMP, 0, R_SECOND, R_TOP );
                i = Constant4();
				jused[i] = 1;
                InstImm( PPC_BC, 12, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 1, v );
				rtopped = qfalse;
                break;
            case OP_GTI:
                ltopandsecond();		// get value from opstack
                Inst( PPC_CMP, 0, R_SECOND, R_TOP );
                i = Constant4();
				jused[i] = 1;
                InstImm( PPC_BC, 4, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 1, v );
				rtopped = qfalse;
                break;
            case OP_GEI:
                ltopandsecond();		// get value from opstack
                Inst( PPC_CMP, 0, R_SECOND, R_TOP );
                i = Constant4();
				jused[i] = 1;
                InstImm( PPC_BC, 12, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 0, v );
				rtopped = qfalse;
                break;
            case OP_LTU:
                ltopandsecond();		// get value from opstack
                Inst( PPC_CMPL, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 4, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 0, v );
		rtopped = qfalse;
                break;
            case OP_LEU:
                ltopandsecond();		// get value from opstack
                Inst( PPC_CMPL, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 12, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 1, v );
		rtopped = qfalse;
                break;
            case OP_GTU:
                ltopandsecond();		// get value from opstack
                Inst( PPC_CMPL, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 4, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 1, v );
		rtopped = qfalse;
                break;
            case OP_GEU:
                ltopandsecond();		// get value from opstack
                Inst( PPC_CMPL, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 12, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 0, v );
		rtopped = qfalse;
                break;
                
            case OP_EQF:
                fltopandsecond();		// get value from opstack
                Inst( PPC_FCMPU, 0, R_TOP, R_SECOND );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 4, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 2, v );
		rtopped = qfalse;
                break;			
            case OP_NEF:
                fltopandsecond();		// get value from opstack
                Inst( PPC_FCMPU, 0, R_TOP, R_SECOND );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 12, 2, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 2, v );
		rtopped = qfalse;
                break;			
            case OP_LTF:
                fltopandsecond();		// get value from opstack
                Inst( PPC_FCMPU, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 4, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 0, v );
		rtopped = qfalse;
                break;			
            case OP_LEF:
                fltopandsecond();		// get value from opstack
                Inst( PPC_FCMPU, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 12, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 1, v );
		rtopped = qfalse;
                break;			
            case OP_GTF:
                fltopandsecond();		// get value from opstack
                Inst( PPC_FCMPU, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 4, 1, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 12, 1, v );
		rtopped = qfalse;
                break;			
            case OP_GEF:
                fltopandsecond();		// get value from opstack
                Inst( PPC_FCMPU, 0, R_SECOND, R_TOP );
                i = Constant4();
		jused[i] = 1;
                InstImm( PPC_BC, 12, 0, 8 );
                if ( pass==1 ) {
                    v = vm->instructionPointers[ i ] - (int)&buf[compiledOfs];
                } else {
                    v = 0;
                }
                Emit4(PPC_B | (unsigned int)(v&0x3ffffff) );
//                InstImm( PPC_BC, 4, 0, v );
		rtopped = qfalse;
                break;

            case OP_NEGI:
                ltop();		// get value from opstack
                InstImm( PPC_SUBFIC, R_TOP, R_TOP, 0 );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_ADD:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_ADD, R_TOP, R_TOP, R_SECOND );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_SUB:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_SUBF, R_TOP, R_TOP, R_SECOND );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_DIVI:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_DIVW, R_TOP, R_SECOND, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_DIVU:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_DIVWU, R_TOP, R_SECOND, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_MODI:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_DIVW, R_EA, R_SECOND, R_TOP );
                Inst( PPC_MULLW, R_EA, R_TOP, R_EA );
                Inst( PPC_SUBF, R_TOP, R_EA, R_SECOND );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_MODU:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_DIVWU, R_EA, R_SECOND, R_TOP );
                Inst( PPC_MULLW, R_EA, R_TOP, R_EA );
                Inst( PPC_SUBF, R_TOP, R_EA, R_SECOND );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_MULI:
            case OP_MULU:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_MULLW, R_TOP, R_SECOND, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_BAND:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_AND, R_SECOND, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_BOR:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_OR, R_SECOND, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_BXOR:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_XOR, R_SECOND, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_BCOM:
                ltop();		// get value from opstack
                Inst( PPC_NOR, R_TOP, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_LSH:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_SLW, R_SECOND, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_RSHI:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_SRAW, R_SECOND, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;
            case OP_RSHU:
                ltop();		// get value from opstack
                InstImm( PPC_LWZU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_SRW, R_SECOND, R_TOP, R_TOP );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qtrue;
                break;

            case OP_NEGF:
                InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                Inst( PPC_FNEG, R_TOP, 0, R_TOP );
                InstImm( PPC_STFS, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qfalse;
                break;
            case OP_ADDF:
                InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                InstImm( PPC_LFSU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_FADDS, R_TOP, R_SECOND, R_TOP );
                InstImm( PPC_STFS, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qfalse;
                break;
            case OP_SUBF:
                InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                InstImm( PPC_LFSU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_FSUBS, R_TOP, R_SECOND, R_TOP );
                InstImm( PPC_STFS, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qfalse;
                break;
            case OP_DIVF:
                InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                InstImm( PPC_LFSU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst( PPC_FDIVS, R_TOP, R_SECOND, R_TOP );
                InstImm( PPC_STFS, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qfalse;
                break;
            case OP_MULF:
                InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                InstImm( PPC_LFSU, R_SECOND, R_OPSTACK, -4 );		// get value from opstack
                Inst4( PPC_FMULS, R_TOP, R_SECOND, 0, R_TOP );
                InstImm( PPC_STFS, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qfalse;
                break;

            case OP_CVIF:
                v = (int)&itofConvert;
                InstImmU( PPC_ADDIS, R_EA, 0, (v >> 16)&0xffff );
                InstImmU( PPC_ORI, R_EA, R_EA, v & 0xffff );
		InstImm( PPC_LWZ, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                InstImmU( PPC_XORIS, R_TOP, R_TOP, 0x8000 );
                InstImm( PPC_STW, R_TOP, R_EA, 12 );
                InstImm( PPC_LFD, R_TOP, R_EA, 0 );
                InstImm( PPC_LFD, R_SECOND, R_EA, 8 );
                Inst( PPC_FSUB, R_TOP, R_SECOND, R_TOP );
    //            Inst( PPC_FRSP, R_TOP, 0, R_TOP );
                InstImm( PPC_STFS, R_TOP, R_OPSTACK, 0 );		// save value to opstack
		rtopped = qfalse;
                break;
            case OP_CVFI:
                InstImm( PPC_LFS, R_TOP, R_OPSTACK, 0 );		// get value from opstack
                Inst( PPC_FCTIWZ, R_TOP, 0, R_TOP );
                Inst( PPC_STFIWX, R_TOP, 0, R_OPSTACK );		// save value to opstack
		rtopped = qfalse;
                break;
            case OP_SEX8:
                ltop();	// get value from opstack
                Inst( PPC_EXTSB, R_TOP, R_TOP, 0 );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );
		rtopped = qtrue;
                break;
            case OP_SEX16:
                ltop();	// get value from opstack
                Inst( PPC_EXTSH, R_TOP, R_TOP, 0 );
                InstImm( PPC_STW, R_TOP, R_OPSTACK, 0 );
		rtopped = qtrue;
                break;

            case OP_BLOCK_COPY:
                v = Constant4() >> 2;
                ltop();		// source
                InstImm( PPC_LWZ, R_SECOND, R_OPSTACK, -4 );	// dest
                InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -8 );
                InstImmU( PPC_ADDI, R_EA, 0, v );				// count
				// FIXME: range check
              	Inst( PPC_MTSPR, R_EA, 9, 0 );					// move to count register

                Inst( PPC_ADD, R_TOP, R_TOP, R_MEMBASE );
                InstImm( PPC_ADDI, R_TOP, R_TOP, -4 );
                Inst( PPC_ADD, R_SECOND, R_SECOND, R_MEMBASE );
                InstImm( PPC_ADDI, R_SECOND, R_SECOND, -4 );

                InstImm( PPC_LWZU, R_EA, R_TOP, 4 );		// source
                InstImm( PPC_STWU, R_EA, R_SECOND, 4 );	// dest
                Inst( PPC_BC | 0xfff8 , 16, 0, 0 );					// loop
		rtopped = qfalse;
                break;

            case OP_JUMP:
                ltop();	// get value from opstack
                InstImm( PPC_ADDI, R_OPSTACK, R_OPSTACK, -4 );
                Inst( PPC_RLWINM | ( 29 << 1 ), R_TOP, R_TOP, 2 );
		// FIXME: range check
		Inst( PPC_LWZX, R_TOP, R_TOP, R_INSTRUCTIONS );
                Inst( PPC_MTSPR, R_TOP, 9, 0 );		// move to count register
                Inst( PPC_BCCTR, 20, 0, 0 );		// jump to the count register
		rtopped = qfalse;
                break;
            default:
                Com_Error( ERR_DROP, "VM_CompilePPC: bad opcode %i at instruction %i, offset %i", op, instruction, pc );
            }
	    pop0 = pop1;
	    pop1 = op;
        }

	Com_Printf( "VM file %s pass %d compiled to %i bytes of code\n", vm->name, (pass+1), compiledOfs*4 );

    	if ( pass == 0 ) {
	    // copy to an exact size buffer on the hunk
	    vm->codeLength = compiledOfs * 4;
	    vm->codeBase = Hunk_Alloc( vm->codeLength, h_low );
	    Com_Memcpy( vm->codeBase, buf, vm->codeLength );
	    Z_Free( buf );
	
	    // offset all the instruction pointers for the new location
	    for ( i = 0 ; i < header->instructionCount ; i++ ) {
		vm->instructionPointers[i] += (int)vm->codeBase;
	    }

	    // go back over it in place now to fixup reletive jump targets
	    buf = (unsigned *)vm->codeBase;
	}
    }
    Z_Free( jused );
}

/*
==============
VM_CallCompiled

This function is called directly by the generated code
==============
*/
int	VM_CallCompiled( vm_t *vm, int *args ) {
	int		stack[1024];
	int		programStack;
	int		stackOnEntry;
	byte	*image;

	currentVM = vm;

	// interpret the code
	vm->currentlyInterpreting = qtrue;

	// we might be called recursively, so this might not be the very top
	programStack = vm->programStack;
	stackOnEntry = programStack;
	image = vm->dataBase;
	
	// set up the stack frame 
	programStack -= 48;

	*(int *)&image[ programStack + 44] = args[9];
	*(int *)&image[ programStack + 40] = args[8];
	*(int *)&image[ programStack + 36] = args[7];
	*(int *)&image[ programStack + 32] = args[6];
	*(int *)&image[ programStack + 28] = args[5];
	*(int *)&image[ programStack + 24] = args[4];
	*(int *)&image[ programStack + 20] = args[3];
	*(int *)&image[ programStack + 16] = args[2];
	*(int *)&image[ programStack + 12] = args[1];
	*(int *)&image[ programStack + 8 ] = args[0];
	*(int *)&image[ programStack + 4 ] = 0;	// return stack
	*(int *)&image[ programStack ] = -1;	// will terminate the loop on return

	// off we go into generated code...
	// the PPC calling standard says the parms will all go into R3 - R11, so 
	// no special asm code is needed here
#ifdef __GNUC__
	((void(*)(int, int, int, int, int, int, int, int))(vm->codeBase))( 
		programStack, (int)&stack, 
		(int)image, vm->dataMask, (int)&AsmCall, 
		(int)vm->instructionPointers, vm->instructionPointersLength,
        (int)vm );
#else
	((void(*)(int, int, int, int, int, int, int, int))(vm->codeBase))( 
		programStack, (int)&stack, 
		(int)image, vm->dataMask, *(int *)&AsmCall /* skip function pointer header */, 
		(int)vm->instructionPointers, vm->instructionPointersLength,
        (int)vm );
#endif
	vm->programStack = stackOnEntry;

    vm->currentlyInterpreting = qfalse;

	return stack[1];
}


/*
==================
AsmCall

Put this at end of file because gcc messes up debug line numbers 
==================
*/
#ifdef __GNUC__

void AsmCall( void ) {
asm (
     // pop off the destination instruction
"    lwz		r12,0(r4)	\n"	// RG_TOP, 0(RG_OPSTACK)
"    addi	r4,r4,-4		\n"	// RG_OPSTACK, RG_OPSTACK, -4 \n"

    // see if it is a system trap
"    cmpwi	r12,0			\n"	// RG_TOP, 0 \n"
"    bc		12,0, systemTrap	\n"

    // calling another VM function, so lookup in instructionPointers
"    slwi	r12,r12,2		\n"	// RG_TOP,RG_TOP,2
                        // FIXME: range check
"    lwzx	r12, r8, r12		\n"	// RG_TOP, RG_INSTRUCTIONS(RG_TOP)	
"    mtctr	r12			\n"	// RG_TOP
);

#if defined(MACOS_X) && defined(__OPTIMIZE__)
    // On Mac OS X, gcc doesn't push a frame when we are optimized, so trying to tear it down results in grave disorder.
#warning Mac OS X optimization on, not popping GCC AsmCall frame
#else
    // Mac OS X Server and unoptimized compiles include a GCC AsmCall frame
    asm (
"	lwz		r1,0(r1)	\n"	// pop off the GCC AsmCall frame
"	lmw		r30,-8(r1)	\n"
);
#endif

asm (
"	    bcctr	20,0		\n" // when it hits a leave, it will branch to the current link register

    // calling a system trap
"systemTrap:				\n"
	// convert to positive system call number
"	subfic	r12,r12,-1		\n"

    // save all our registers, including the current link register
"    mflr	r13			\n"	// RG_SECOND		// copy off our link register
"	addi	r1,r1,-92		\n"	// required 24 byets of linkage, 32 bytes of parameter, plus our saves
"    stw		r3,56(r1)	\n"	// RG_STACK, -36(REAL_STACK)
"    stw		r4,60(r1)	\n"	// RG_OPSTACK, 4(RG_REAL_STACK)
"    stw		r5,64(r1)	\n"	// RG_MEMBASE, 8(RG_REAL_STACK)
"    stw		r6,68(r1)	\n"	// RG_MEMMASK, 12(RG_REAL_STACK)
"    stw		r7,72(r1)	\n"	// RG_ASMCALL, 16(RG_REAL_STACK)
"    stw		r8,76(r1)	\n"	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)
"    stw		r9,80(r1)	\n"	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)
"    stw		r10,84(r1)	\n"	// RG_VM, 28(RG_REAL_STACK)
"    stw		r13,88(r1)	\n"	// RG_SECOND, 32(RG_REAL_STACK)	// link register

    // save the vm stack position to allow recursive VM entry
"    addi	r13,r3,-4		\n"	// RG_TOP, RG_STACK, -4
"    stw		r13,0(r10)	\n"	//RG_TOP, VM_OFFSET_PROGRAM_STACK(RG_VM)

    // save the system call number as the 0th parameter
"    add		r3,r3,r5	\n"	// r3,  RG_STACK, RG_MEMBASE		// r3 is the first parameter to vm->systemCalls
"    stwu	r12,4(r3)		\n"	// RG_TOP, 4(r3)

    // make the system call with the address of all the VM parms as a parameter
    // vm->systemCalls( &parms )
"    lwz		r12,4(r10)	\n"	// RG_TOP, VM_OFFSET_SYSTEM_CALL(RG_VM)
"    mtctr	r12			\n"	// RG_TOP
"    bcctrl	20,0			\n"
"    mr		r12,r3			\n"	// RG_TOP, r3

    // pop our saved registers
"   	lwz		r3,56(r1)	\n"	// RG_STACK, 0(RG_REAL_STACK)
"   	lwz		r4,60(r1)	\n"	// RG_OPSTACK, 4(RG_REAL_STACK)
"   	lwz		r5,64(r1)	\n"	// RG_MEMBASE, 8(RG_REAL_STACK)
"   	lwz		r6,68(r1)	\n"	// RG_MEMMASK, 12(RG_REAL_STACK)
"   	lwz		r7,72(r1)	\n"	// RG_ASMCALL, 16(RG_REAL_STACK)
"   	lwz		r8,76(r1)	\n"	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)
"   	lwz		r9,80(r1)	\n"	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)
"   	lwz		r10,84(r1)	\n"	// RG_VM, 28(RG_REAL_STACK)
"   	lwz		r13,88(r1)	\n"	// RG_SECOND, 32(RG_REAL_STACK)
"    addi	r1,r1,92		\n"	// RG_REAL_STACK, RG_REAL_STACK, 36

    // restore the old link register
"    mtlr	r13			\n"	// RG_SECOND

    // save off the return value
"    stwu	r12,4(r4)		\n"	// RG_TOP, 0(RG_OPSTACK)

	// GCC adds its own prolog / epilog code
 );
}
#else

// codewarrior version

void asm AsmCall( void ) {

    // pop off the destination instruction

    lwz		r12,0(r4)	// RG_TOP, 0(RG_OPSTACK)

    addi	r4,r4,-4	// RG_OPSTACK, RG_OPSTACK, -4



    // see if it is a system trap

    cmpwi	r12,0		// RG_TOP, 0

    bc		12,0, systemTrap



    // calling another VM function, so lookup in instructionPointers

    slwi	r12,r12,2		// RG_TOP,RG_TOP,2

                        // FIXME: range check

    lwzx	r12, r8, r12	// RG_TOP, RG_INSTRUCTIONS(RG_TOP)	

    mtctr	r12			// RG_TOP



    bcctr	20,0		// when it hits a leave, it will branch to the current link register



    // calling a system trap

systemTrap:

	// convert to positive system call number

	subfic	r12,r12,-1



    // save all our registers, including the current link register

    mflr	r13			// RG_SECOND		// copy off our link register

	addi	r1,r1,-92	// required 24 byets of linkage, 32 bytes of parameter, plus our saves

    stw		r3,56(r1)	// RG_STACK, -36(REAL_STACK)

    stw		r4,60(r1)	// RG_OPSTACK, 4(RG_REAL_STACK)

    stw		r5,64(r1)	// RG_MEMBASE, 8(RG_REAL_STACK)

    stw		r6,68(r1)	// RG_MEMMASK, 12(RG_REAL_STACK)

    stw		r7,72(r1)	// RG_ASMCALL, 16(RG_REAL_STACK)

    stw		r8,76(r1)	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)

    stw		r9,80(r1)	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)

    stw		r10,84(r1)	// RG_VM, 28(RG_REAL_STACK)

    stw		r13,88(r1)	// RG_SECOND, 32(RG_REAL_STACK)	// link register



    // save the vm stack position to allow recursive VM entry

    addi	r13,r3,-4	// RG_TOP, RG_STACK, -4

    stw		r13,0(r10)	//RG_TOP, VM_OFFSET_PROGRAM_STACK(RG_VM)



    // save the system call number as the 0th parameter

    add		r3,r3,r5	// r3,  RG_STACK, RG_MEMBASE		// r3 is the first parameter to vm->systemCalls

    stwu	r12,4(r3)	// RG_TOP, 4(r3)



    // make the system call with the address of all the VM parms as a parameter

    // vm->systemCalls( &parms )

    lwz		r12,4(r10)	// RG_TOP, VM_OFFSET_SYSTEM_CALL(RG_VM)

    

    // perform macos cross fragment fixup crap

    lwz		r9,0(r12)

    stw		r2,52(r1)	// save old TOC

	lwz		r2,4(r12)

	    

    mtctr	r9			// RG_TOP

    bcctrl	20,0

    

    lwz		r2,52(r1)	// restore TOC

    

    mr		r12,r3		// RG_TOP, r3



    // pop our saved registers

   	lwz		r3,56(r1)	// RG_STACK, 0(RG_REAL_STACK)

   	lwz		r4,60(r1)	// RG_OPSTACK, 4(RG_REAL_STACK)

   	lwz		r5,64(r1)	// RG_MEMBASE, 8(RG_REAL_STACK)

   	lwz		r6,68(r1)	// RG_MEMMASK, 12(RG_REAL_STACK)

   	lwz		r7,72(r1)	// RG_ASMCALL, 16(RG_REAL_STACK)

   	lwz		r8,76(r1)	// RG_INSTRUCTIONS, 20(RG_REAL_STACK)

   	lwz		r9,80(r1)	// RG_NUM_INSTRUCTIONS, 24(RG_REAL_STACK)

   	lwz		r10,84(r1)	// RG_VM, 28(RG_REAL_STACK)

   	lwz		r13,88(r1)	// RG_SECOND, 32(RG_REAL_STACK)

    addi	r1,r1,92	// RG_REAL_STACK, RG_REAL_STACK, 36



    // restore the old link register

    mtlr	r13			// RG_SECOND



    // save off the return value

    stwu	r12,4(r4)	// RG_TOP, 0(RG_OPSTACK)



	blr

}




#endif
