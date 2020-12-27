/*
===========================================================================
Copyright (C) 2009 David S. Miller <davem@davemloft.net>
Copyright (C) 2013,2014 SUSE Linux Products GmbH

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

ARMv7l VM by Ludwig Nussel <ludwig.nussel@suse.de>

TODO: optimization

Docu:
http://www.coranac.com/tonc/text/asm.htm
http://www.heyrick.co.uk/armwiki/Category:Opcodes
ARMv7-A_ARMv7-R_DDI0406_2007.pdf
*/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <stddef.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vm_local.h"
#define R0	0
#define R1	1
#define R2	2
#define R3	3
#define R4	4

#define R12	12

#define FP	11
#define SP	13
#define LR	14
#define PC	15

#define APSR_nzcv	15

#define S14     14
#define S15     15

#define rOPSTACK	5
#define rOPSTACKBASE	6
#define rCODEBASE	7
#define rPSTACK		8
#define rDATABASE	9
#define rDATAMASK	10

#define bit(x) (1<<x)

/* arm eabi, builtin gcc functions */
int __aeabi_idiv (int, int);
unsigned __aeabi_uidiv (unsigned, unsigned);
void __aeabi_idivmod(void);
void __aeabi_uidivmod(void);

/* exit() won't be called but use it because it is marked with noreturn */
#define DIE( reason, args... ) \
	do { \
		Com_Error(ERR_DROP, "vm_arm compiler error: " reason, ##args); \
		exit(1); \
	} while(0)

/*
 * opcode information table:
 * - length of immediate value
 * - returned register type
 * - required register(s) type
 */
#define opImm0	0x0000 /* no immediate */
#define opImm1	0x0001 /* 1 byte immadiate value after opcode */
#define opImm4	0x0002 /* 4 bytes immediate value after opcode */

#define opRet0	0x0000 /* returns nothing */
#define opRetI	0x0004 /* returns integer */
#define opRetF	0x0008 /* returns float */
#define opRetIF	(opRetI | opRetF) /* returns integer or float */

#define opArg0	0x0000 /* requires nothing */
#define opArgI	0x0010 /* requires integer(s) */
#define opArgF	0x0020 /* requires float(s) */
#define opArgIF	(opArgI | opArgF) /* requires integer or float */

#define opArg2I	0x0040 /* requires second argument, integer */
#define opArg2F	0x0080 /* requires second argument, float */
#define opArg2IF (opArg2I | opArg2F) /* requires second argument, integer or float */

static const unsigned char vm_opInfo[256] =
{
	[OP_UNDEF]	= opImm0,
	[OP_IGNORE]	= opImm0,
	[OP_BREAK]	= opImm0,
	[OP_ENTER]	= opImm4,
			/* OP_LEAVE has to accept floats, they will be converted to ints */
	[OP_LEAVE]	= opImm4 | opRet0 | opArgIF,
			/* only STORE4 and POP use values from OP_CALL,
			 * no need to convert floats back */
	[OP_CALL]	= opImm0 | opRetI | opArgI,
	[OP_PUSH]	= opImm0 | opRetIF,
	[OP_POP]	= opImm0 | opRet0 | opArgIF,
	[OP_CONST]	= opImm4 | opRetIF,
	[OP_LOCAL]	= opImm4 | opRetI,
	[OP_JUMP]	= opImm0 | opRet0 | opArgI,

	[OP_EQ]		= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_NE]		= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LTI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LEI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GTI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GEI]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LTU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_LEU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GTU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_GEU]	= opImm4 | opRet0 | opArgI | opArg2I,
	[OP_EQF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_NEF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_LTF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_LEF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_GTF]	= opImm4 | opRet0 | opArgF | opArg2F,
	[OP_GEF]	= opImm4 | opRet0 | opArgF | opArg2F,

	[OP_LOAD1]	= opImm0 | opRetI | opArgI,
	[OP_LOAD2]	= opImm0 | opRetI | opArgI,
	[OP_LOAD4]	= opImm0 | opRetIF| opArgI,
	[OP_STORE1]	= opImm0 | opRet0 | opArgI | opArg2I,
	[OP_STORE2]	= opImm0 | opRet0 | opArgI | opArg2I,
	[OP_STORE4]	= opImm0 | opRet0 | opArgIF| opArg2I,
	[OP_ARG]	= opImm1 | opRet0 | opArgIF,
	[OP_BLOCK_COPY]	= opImm4 | opRet0 | opArgI | opArg2I,

	[OP_SEX8]	= opImm0 | opRetI | opArgI,
	[OP_SEX16]	= opImm0 | opRetI | opArgI,
	[OP_NEGI]	= opImm0 | opRetI | opArgI,
	[OP_ADD]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_SUB]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_DIVI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_DIVU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MODI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MODU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MULI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_MULU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BAND]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BOR]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BXOR]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_BCOM]	= opImm0 | opRetI | opArgI,
	[OP_LSH]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_RSHI]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_RSHU]	= opImm0 | opRetI | opArgI | opArg2I,
	[OP_NEGF]	= opImm0 | opRetF | opArgF,
	[OP_ADDF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_SUBF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_DIVF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_MULF]	= opImm0 | opRetF | opArgF | opArg2F,
	[OP_CVIF]	= opImm0 | opRetF | opArgI,
	[OP_CVFI]	= opImm0 | opRetI | opArgF,
};

#ifdef DEBUG_VM
static const char *opnames[256] = {
	"OP_UNDEF", "OP_IGNORE", "OP_BREAK", "OP_ENTER", "OP_LEAVE", "OP_CALL",
	"OP_PUSH", "OP_POP", "OP_CONST", "OP_LOCAL", "OP_JUMP",
	"OP_EQ", "OP_NE", "OP_LTI", "OP_LEI", "OP_GTI", "OP_GEI",
	"OP_LTU", "OP_LEU", "OP_GTU", "OP_GEU", "OP_EQF", "OP_NEF",
	"OP_LTF", "OP_LEF", "OP_GTF", "OP_GEF",
	"OP_LOAD1", "OP_LOAD2", "OP_LOAD4", "OP_STORE1", "OP_STORE2",
	"OP_STORE4", "OP_ARG", "OP_BLOCK_COPY",
	"OP_SEX8", "OP_SEX16",
	"OP_NEGI", "OP_ADD", "OP_SUB", "OP_DIVI", "OP_DIVU",
	"OP_MODI", "OP_MODU", "OP_MULI", "OP_MULU", "OP_BAND",
	"OP_BOR", "OP_BXOR", "OP_BCOM", "OP_LSH", "OP_RSHI", "OP_RSHU",
	"OP_NEGF", "OP_ADDF", "OP_SUBF", "OP_DIVF", "OP_MULF",
	"OP_CVIF", "OP_CVFI",
};

#define NOTIMPL(x) \
	do { Com_Error(ERR_DROP, "instruction not implemented: %s", opnames[x]); } while(0)
#else
#define NOTIMPL(x) \
	do { Com_Printf(S_COLOR_RED "instruction not implemented: %x\n", x); vm->compiled = qfalse; return; } while(0)
#endif

static void VM_Destroy_Compiled(vm_t *vm)
{
	if (vm->codeBase) {
		if (munmap(vm->codeBase, vm->codeLength))
			Com_Printf(S_COLOR_RED "Memory unmap failed, possible memory leak\n");
	}
	vm->codeBase = NULL;
}

/*
=================
ErrJump
Error handler for jump/call to invalid instruction number
=================
*/

static void __attribute__((__noreturn__)) ErrJump(unsigned num)
{
	Com_Error(ERR_DROP, "program tried to execute code outside VM (%x)", num);
}

static int asmcall(int call, int pstack)
{
	// save currentVM so as to allow for recursive VM entry
	vm_t *savedVM = currentVM;
	int i, ret;

	// modify VM stack pointer for recursive VM entry
	currentVM->programStack = pstack - 4;

	if (sizeof(intptr_t) == sizeof(int)) {
		intptr_t *argPosition = (intptr_t *)((byte *)currentVM->dataBase + pstack + 4);
		argPosition[0] = -1 - call;
		ret = currentVM->systemCall(argPosition);
	} else {
		intptr_t args[MAX_VMSYSCALL_ARGS];

		args[0] = -1 - call;
		int *argPosition = (int *)((byte *)currentVM->dataBase + pstack + 4);
		for( i = 1; i < ARRAY_LEN(args); i++ )
			args[i] = argPosition[i];

		ret = currentVM->systemCall(args);
	}

	currentVM = savedVM;

	return ret;
}

void _emit(vm_t *vm, unsigned isn, int pass)
{
#if 0
	static int fd = -2;
	if (fd == -2)
		fd = open("code.bin", O_TRUNC|O_WRONLY|O_CREAT, 0644);
	if (fd > 0)
		write(fd, &isn, 4);
#endif

	if (pass)
		memcpy(vm->codeBase+vm->codeLength, &isn, 4);
	vm->codeLength+=4;
}

#define emit(isn) _emit(vm, isn, pass)

static unsigned char off8(unsigned val)
{
	if (val&3)
		DIE("offset must be multiple of four");
	if (val > 1020)
		DIE("offset too large");
	return val>>2;
}

// ARM is really crazy ...
static unsigned short rimm(unsigned val)
{
	unsigned shift = 0;
	if (val < 256)
		return val;
	// rotate the value until it fits
	while (shift < 16 && (val>255 || !(val&3))) {
		val =  (val&3)<<30 | val>>2;
		++shift;
	}
	if (shift > 15 || val > 255) {
		DIE("immediate cannot be encoded (%d, %d)\n", shift, val);
	}
	return (16-shift)<<8 | val;
}

// same as rimm but doesn't die, returns 0 if not encodable so don't call with zero as argument!
static unsigned short can_encode(unsigned val)
{
	unsigned shift = 0;
	if (!val)
		DIE("can_encode: invalid argument");
	if (val < 256)
		return val;
	// rotate the value until it fits
	while (shift < 16 && (val>255 || !(val&3))) {
		val =  (val&3)<<30 | val>>2;
		++shift;
	}
	if (shift > 15 || val > 255) {
		return 0;
	}
	return (16-shift)<<8 | val;
}

#define PREINDEX (1<<24)

#define rASR(i, reg) (0b10<<5 | ((i&31)<<7) | reg)
#define rLSL(i, reg) (0b00<<5 | ((i&31)<<7) | reg)
#define rLSR(i, reg) (0b01<<5 | ((i&31)<<7) | reg)
#define rROR(i, reg) (0b11<<5 | ((i&31)<<7) | reg)

// conditions
#define EQ (0b0000<<28)
#define NE (0b0001<<28)
#define CS (0b0010<<28)
#define HS CS
#define CC (0b0011<<28)
#define LO CC
#define MI (0b0100<<28)
#define PL (0b0101<<28)
#define VS (0b0110<<28)
#define VC (0b0111<<28)
#define HI (0b1000<<28)
#define LS (0b1001<<28)
#define GE (0b1010<<28)
#define LT (0b1011<<28)
#define GT (0b1100<<28)
#define LE (0b1101<<28)
#define AL (0b1110<<28)
#define cond(what, op) (what | (op&~AL))

// XXX: v not correctly computed
#define BKPT(v) (AL | 0b10010<<20 | ((v&~0xF)<<4) | 0b0111<<4 | (v&0xF))

#define YIELD (0b110010<<20 | 0b1111<<12 | 1)
#define NOP cond(AL, YIELD)

// immediate value must fit in 0xFF!
#define ANDi(dst, src, i) (AL | (0b001<<25) | (0b00000<<20) | (src<<16) | (dst<<12) | rimm(i))
#define EORi(dst, src, i) (AL | (0b001<<25) | (0b00010<<20) | (src<<16) | (dst<<12) | rimm(i))
#define SUBi(dst, src, i) (AL | (0b001<<25) | (0b00100<<20) | (src<<16) | (dst<<12) | rimm(i))
#define RSBi(dst, src, i) (AL | (0b001<<25) | (0b00110<<20) | (src<<16) | (dst<<12) | rimm(i))
#define ADDi(dst, src, i) (AL | (0b001<<25) | (0b01000<<20) | (src<<16) | (dst<<12) | rimm(i))
#define ADCi(dst, src, i) (AL | (0b001<<25) | (0b01010<<20) | (src<<16) | (dst<<12) | rimm(i))
#define SBCi(dst, src, i) (AL | (0b001<<25) | (0b01100<<20) | (src<<16) | (dst<<12) | rimm(i))
#define RSCi(dst, src, i) (AL | (0b001<<25) | (0b01110<<20) | (src<<16) | (dst<<12) | rimm(i))

#define ORRi(dst, src, i) (AL | (0b001<<25) | (0b11000<<20) | (src<<16) | (dst<<12) | rimm(i))
#define MOVi(dst,      i) (AL | (0b001<<25) | (0b11010<<20) |             (dst<<12) | rimm(i))
#define BICi(dst, src, i) (AL | (0b001<<25) | (0b11100<<20) | (src<<16) | (dst<<12) | rimm(i))
#define MVNi(dst,      i) (AL | (0b001<<25) | (0b11110<<20) |             (dst<<12) | rimm(i))

#define MOVW(dst,      i) (AL |  (0b11<<24)                 | ((((i)>>12)&0xF)<<16) | (dst<<12) | ((i)&((1<<12)-1)))
#define MOVT(dst,      i) (AL |  (0b11<<24) |  (0b0100<<20) | ((((i)>>12)&0xF)<<16) | (dst<<12) | ((i)&((1<<12)-1)))

#define TSTi(     src, i) (AL | (0b001<<25) | (0b10001<<20) | (src<<16) |             rimm(i))
#define TEQi(     src, i) (AL | (0b001<<25) | (0b10011<<20) | (src<<16) |             rimm(i))
#define CMPi(     src, i) (AL | (0b001<<25) | (0b10101<<20) | (src<<16) |             rimm(i))
#define CMNi(     src, i) (AL | (0b001<<25) | (0b10111<<20) | (src<<16) |             rimm(i))

#define ANDSi(dst, src, i) (ANDi(dst, src, i) | (1<<20))
#define EORSi(dst, src, i) (EORi(dst, src, i) | (1<<20))
#define SUBSi(dst, src, i) (SUBi(dst, src, i) | (1<<20))
#define RSBSi(dst, src, i) (RSBi(dst, src, i) | (1<<20))
#define ADDSi(dst, src, i) (ADDi(dst, src, i) | (1<<20))
#define ADCSi(dst, src, i) (ADCi(dst, src, i) | (1<<20))
#define SBCSi(dst, src, i) (SBCi(dst, src, i) | (1<<20))
#define RSCSi(dst, src, i) (RSCi(dst, src, i) | (1<<20))

#define ORRSi(dst, src, i) (ORRi(dst, src, i) | (1<<20))
#define MOVSi(dst,      i) (MOVi(dst,      i) | (1<<20))
#define BICSi(dst, src, i) (BICi(dst, src, i) | (1<<20))
#define MVNSi(dst,      i) (MVNi(dst, src, i) | (1<<20))

#define AND(dst, src, reg) (AL | (0b000<<25) | (0b00000<<20) | (src<<16) | (dst<<12) | reg)
#define EOR(dst, src, reg) (AL | (0b000<<25) | (0b00010<<20) | (src<<16) | (dst<<12) | reg)
#define SUB(dst, src, reg) (AL | (0b000<<25) | (0b00100<<20) | (src<<16) | (dst<<12) | reg)
#define RSB(dst, src, reg) (AL | (0b000<<25) | (0b00110<<20) | (src<<16) | (dst<<12) | reg)
#define ADD(dst, src, reg) (AL | (0b000<<25) | (0b01000<<20) | (src<<16) | (dst<<12) | reg)
#define ADC(dst, src, reg) (AL | (0b000<<25) | (0b01010<<20) | (src<<16) | (dst<<12) | reg)
#define SBC(dst, src, reg) (AL | (0b000<<25) | (0b01100<<20) | (src<<16) | (dst<<12) | reg)
#define RSC(dst, src, reg) (AL | (0b000<<25) | (0b01110<<20) | (src<<16) | (dst<<12) | reg)

#define ORR(dst, src, reg) (AL | (0b000<<25) | (0b11000<<20) | (src<<16) | (dst<<12) | reg)
#define MOV(dst,      src) (AL | (0b000<<25) | (0b11010<<20) |             (dst<<12) | src)

#define LSL(dst, src, reg) (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | (reg<<8)     | (0b0001<<4) | src)
#define LSR(dst, src, reg) (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | (reg<<8)     | (0b0011<<4) | src)
#define ASR(dst, src, reg) (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | (reg<<8)     | (0b0101<<4) | src)
#define ROR(dst, src, reg) (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | (reg<<8)     | (0b0111<<4) | src)

#define LSLi(dst, src, i)  (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | ((i&0x1F)<<7) | (0b000<<4) | src)
#define LSRi(dst, src, i)  (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | ((i&0x1F)<<7) | (0b010<<4) | src)
#define ASRi(dst, src, i)  (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | ((i&0x1F)<<7) | (0b100<<4) | src)
#define RORi(dst, src, i)  (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) | ((i&0x1F)<<7) | (0b110<<4) | src)
#define RRX(dst, src)      (AL | (0b000<<25) | (0b1101<<21) | (0<<20) | (dst<<12) |                 (0b110<<4) | src)

#define BIC(dst, src, reg) (AL | (0b000<<25) | (0b11100<<20) | (src<<16) | (dst<<12) | reg)
#define MVN(dst,      reg) (AL | (0b000<<25) | (0b11110<<20) |             (dst<<12) | reg)

#define TST(     src, reg) (AL | (0b000<<25) | (0b10001<<20) | (src<<16) |             reg)
#define TEQ(     src, reg) (AL | (0b000<<25) | (0b10011<<20) | (src<<16) |             reg)
#define CMP(     src, reg) (AL | (0b000<<25) | (0b10101<<20) | (src<<16) |             reg)
#define CMN(     src, reg) (AL | (0b000<<25) | (0b10111<<20) | (src<<16) |             reg)

#define LDRa(dst, base, off)   (AL | (0b011<<25) | (0b1100<<21) | (1<<20) | base<<16 | dst<<12 | off)
#define LDRx(dst, base, off)   (AL | (0b011<<25) | (0b1000<<21) | (1<<20) | base<<16 | dst<<12 | off)

#define LDRai(dst, base, off)  (AL | (0b010<<25) | (0b1100<<21) | (1<<20) | base<<16 | dst<<12 | rimm(off))
#define LDRxi(dst, base, off)  (AL | (0b010<<25) | (0b1000<<21) | (1<<20) | base<<16 | dst<<12 | rimm(off))
#define LDRxiw(dst, base, off) (AL | (0b010<<25) | (0b1001<<21) | (1<<20) | base<<16 | dst<<12 | rimm(off))

#define LDRTa(dst, base, off)  (AL | (0b011<<25) | (0b0101<<21) | (1<<20) | base<<16 | dst<<12 | off)
#define LDRTx(dst, base, off)  (AL | (0b011<<25) | (0b0001<<21) | (1<<20) | base<<16 | dst<<12 | off)
#define LDRTai(dst, base, off) (AL | (0b010<<25) | (0b0101<<21) | (1<<20) | base<<16 | dst<<12 | rimm(off))
#define LDRTxi(dst, base, off) (AL | (0b010<<25) | (0b0001<<21) | (1<<20) | base<<16 | dst<<12 | rimm(off))

#define LDRBa(dst, base, off)  (AL | (0b011<<25) | (0b1110<<21) | (1<<20) | base<<16 | dst<<12 | off)
#define LDRSBai(dst, base, off) (AL | (0b000<<25) | (0b0110<<21) | (1<<20) | base<<16 | dst<<12 | ((off&0xF0)<<4)|0b1101<<4|(off&0x0F))
#define STRBa(dst, base, off)  (AL | (0b011<<25) | (0b1110<<21) | (0<<20) | base<<16 | dst<<12 | off)

#define LDRHa(dst, base, off)   (AL | (0b000<<25) | (0b1100<<21) | (1<<20) | base<<16 | dst<<12 | (0b1011<<4) | off)
#define LDRSHai(dst, base, off) (AL | (0b000<<25) | (0b1110<<21) | (1<<20) | base<<16 | dst<<12 | ((off&0xF0)<<4)|0b1111<<4|(off&0x0F))
#define STRHa(dst, base, off)   (AL | (0b000<<25) | (0b1100<<21) | (0<<20) | base<<16 | dst<<12 | (0b1011<<4) | off)

#define STRa(dst, base, off)   (AL | (0b011<<25) | (0b1100<<21) | (0<<20) | base<<16 | dst<<12 | off)
#define STRx(dst, base, off)   (AL | (0b011<<25) | (0b1000<<21) | (0<<20) | base<<16 | dst<<12 | off)
#define STRai(dst, base, off)  (AL | (0b010<<25) | (0b1100<<21) | (0<<20) | base<<16 | dst<<12 | rimm(off))
#define STRxi(dst, base, off)  (AL | (0b010<<25) | (0b1000<<21) | (0<<20) | base<<16 | dst<<12 | rimm(off))
#define STRaiw(dst, base, off) (AL | (0b010<<25) | (0b1101<<21) | (0<<20) | base<<16 | dst<<12 | rimm(off))
#define STRxiw(dst, base, off) (AL | (0b010<<25) | (0b1001<<21) | (0<<20) | base<<16 | dst<<12 | rimm(off))

// load with post-increment
#define POP1(reg)              (AL | (0b010<<25) | (0b0100<<21) | (1<<20) |   SP<<16 | reg<<12 | reg)
// store with post-increment
#define PUSH1(reg)             (AL | (0b010<<25) | (0b1001<<21) | (0<<20) |   SP<<16 | reg<<12 | 4)

// branch to target address (for small jumps)
#define Bi(i) \
	(AL | (0b10)<<26 | (1<<25) /*I*/ | (0<<24) /*L*/ | (i))
// call subroutine
#define BLi(i) \
	(AL | (0b10)<<26 | (1<<25) /*I*/ | (1<<24) /*L*/ | (i))
// branch and exchange (register)
#define BX(reg) \
	(AL | 0b00010010<<20 | 0b1111<<16 | 0b1111<<12 | 0b1111<<8| 0b0001<<4 | reg)
// call subroutine (register)
#define BLX(reg) \
	(AL | 0b00010010<<20 | 0b1111<<16 | 0b1111<<12 | 0b1111<<8| 0b0011<<4 | reg)

#define PUSH(mask)    (AL | (0b100100<<22) | (0b10<<20) | (0b1101<<16) |  mask)
#define PUSH2(r1, r2) (AL | (0b100100<<22) | (0b10<<20) | (0b1101<<16) |  1<<r1 | 1<<r2)
//#define PUSH1(reg) STRxiw(SP, reg, 4)

#define POP(mask)     (0xe8bd0000|mask)

#define STM(base, regs) \
	(AL | 0b100<<25 | 0<<24/*P*/| 0<<24/*U*/| 0<<24/*S*/| 0<<24/*W*/ | (base<<16) | (regs&~(1<<16)))

// note: op1 and op2 must not be the same
#define MUL(op1, op2, op3) \
	(AL | 0b0000000<<21 | (1<<20) /*S*/ | (op1<<16) | (op3<<8) | 0b1001<<4 | (op2))

// puts integer in R0
#define emit_MOVR0i(arg) emit_MOVRxi(R0, arg)

// puts integer arg in register reg
#define emit_MOVRxi(reg, arg) do { \
	emit(MOVW(reg, (arg&0xFFFF))); \
	if (arg > 0xFFFF) \
		emit(MOVT(reg, (((arg>>16)&0xFFFF)))); \
	} while(0)

// puts integer arg in register reg. adds nop if only one instr is needed to
// make size constant
#define emit_MOVRxi_or_NOP(reg, arg) do { \
	emit(MOVW(reg, (arg&0xFFFF))); \
	if (arg > 0xFFFF) \
		emit(MOVT(reg, (((arg>>16)&0xFFFF)))); \
	else \
		emit(NOP); \
	} while(0)

// arm core register -> singe precision register
#define VMOVass(Vn, Rt) (AL|(0b1110<<24)|(0b000<<21)|(0<<20)| ((Vn>>1)<<16) | (Rt<<12) | (0b1010<<8) | ((Vn&1)<<7) | (1<<4))
// singe precision register -> arm core register
#define VMOVssa(Rt, Vn) (AL|(0b1110<<24)|(0b000<<21)|(1<<20)| ((Vn>>1)<<16) | (Rt<<12) | (0b1010<<8) | ((Vn&1)<<7) | (1<<4))

#define _VCVT_F(Vd, Vm, opc2, op) \
	(AL|(0b11101<<23)|((Vd&1)<<22)|(0b111<<19)|(opc2<<16)|((Vd>>1)<<12)|(0b101<<9)|(0<<8)|(op<<7)|(1<<6)|((Vm&1)<<5)|(Vm>>1))
#define VCVT_F32_U32(Sd, Sm) _VCVT_F(Sd, Sm, 0b000, 0 /* unsigned */)
#define VCVT_U32_F32(Sd, Sm) _VCVT_F(Sd, Sm, 0b100, 1 /* round zero */)
#define VCVT_F32_S32(Sd, Sm) _VCVT_F(Sd, Sm, 0b000, 1 /* unsigned */)
#define VCVT_S32_F32(Sd, Sm) _VCVT_F(Sd, Sm, 0b101, 1 /* round zero */)

#define VLDRa(Vd, Rn, i) (AL|(0b1101<<24)|1<<23|((Vd&1)<<22)|1<<20|(Rn<<16)|((Vd>>1)<<12)|(0b1010<<8)|off8(i))
#define VSTRa(Vd, Rn, i) (AL|(0b1101<<24)|1<<23|((Vd&1)<<22)|0<<20|(Rn<<16)|((Vd>>1)<<12)|(0b1010<<8)|off8(i))

#define VNEG_F32(Vd, Vm) \
	(AL|(0b11101<<23)|((Vd&1)<<22)|(0b11<<20)|(1<<16)|((Vd>>1)<<12)|(0b101<<9)|(0<<8)|(1<<6)|((Vm&1)<<5)|(Vm>>1))

#define VADD_F32(Vd, Vn, Vm) \
	(AL|(0b11100<<23)|((Vd&1)<<22)|(0b11<<20)|((Vn>>1)<<16)|((Vd>>1)<<12)|(0b101<<9)|(0<<8)|((Vn&1)<<7)|(0<<6)|((Vm&1)<<5)|(Vm>>1))
#define VSUB_F32(Vd, Vn, Vm) \
	(AL|(0b11100<<23)|((Vd&1)<<22)|(0b11<<20)|((Vn>>1)<<16)|((Vd>>1)<<12)|(0b101<<9)|(0<<8)|((Vn&1)<<7)|(1<<6)|((Vm&1)<<5)|(Vm>>1))
#define VMUL_F32(Vd, Vn, Vm) \
	(AL|(0b11100<<23)|((Vd&1)<<22)|(0b10<<20)|((Vn>>1)<<16)|((Vd>>1)<<12)|(0b101)<<9|(0<<8)|((Vn&1)<<7)|(0<<6)|((Vm&1)<<5)|(Vm>>1))
#define VDIV_F32(Vd, Vn, Vm) \
	(AL|(0b11101<<23)|((Vd&1)<<22)|(0b00<<20)|((Vn>>1)<<16)|((Vd>>1)<<12)|(0b101<<9)|(0<<8)|((Vn&1)<<7)|(0<<6)|((Vm&1)<<5)|(Vm>>1))

#define _VCMP_F32(Vd, Vm, E) \
	(AL|(0b11101<<23)|((Vd&1)<<22)|(0b11<<20)|((0b0100)<<16)|((Vd>>1)<<12)|(0b101<<9)|(0<<8)|(E<<7)|(1<<6)|((Vm&1)<<5)|(Vm>>1))
#define VCMP_F32(Vd, Vm) _VCMP_F32(Vd, Vm, 0)

#define VMRS(Rt) \
	(AL|(0b11101111<<20)|(0b0001<<16)|(Rt<<12)|(0b1010<<8)|(1<<4))

// check if instruction in R0 is within range. Clobbers R1, R12
#define CHECK_JUMP do { \
	static int bytes_to_skip = -1; \
	static unsigned branch = -1; \
	emit_MOVRxi(R1, (unsigned)vm->instructionCount); \
	emit(CMP(R0, R1)); \
	if (branch == -1) \
		branch = vm->codeLength; \
	emit(cond(LT, Bi(j_rel(bytes_to_skip)))); \
	emit_MOVRxi_or_NOP(R12, (unsigned)ErrJump); \
	emit(BLX(R12)); \
	if (bytes_to_skip == -1) \
		bytes_to_skip = vm->codeLength - branch; \
} while(0)

//#define CONST_OPTIMIZE
#ifdef CONST_OPTIMIZE
#define MAYBE_EMIT_CONST() \
	if (got_const) \
	{ \
		got_const = 0; \
		vm->instructionPointers[instruction-1] = assembler_get_code_size(); \
		STACK_PUSH(4); \
		emit("movl $%d, (%%r9, %%rbx, 4)", const_value); \
	}
#else
#define MAYBE_EMIT_CONST()
#endif

// optimize: use load multiple
#define IJ(comparator) do { \
	MAYBE_EMIT_CONST(); \
	emit_MOVRxi(R0, arg.i); \
	CHECK_JUMP; \
	emit(LDRTxi(R0, rOPSTACK, 4)); \
	emit(LDRTxi(R1, rOPSTACK, 4));  \
	emit(CMP(R1, R0)); \
	emit(cond(comparator, Bi(j_rel(vm->instructionPointers[arg.i]-vm->codeLength)))); \
} while (0)

#define FJ(comparator) do { \
	emit_MOVRxi(R0, arg.i); \
	CHECK_JUMP; \
	emit(SUBi(rOPSTACK, rOPSTACK, 8)); \
	emit(VLDRa(S15, rOPSTACK, 4)); \
	emit(VLDRa(S14, rOPSTACK, 8)); \
	emit(VCMP_F32(S15, S14)); \
	emit(VMRS(APSR_nzcv)); \
	emit(cond(comparator, Bi(j_rel(vm->instructionPointers[arg.i]-vm->codeLength)))); \
} while (0)

#define printreg(reg) emit(PUSH1(R3)); emit(BLX(reg)); emit(POP1(R3));

static inline unsigned _j_rel(int x, int pc)
{
	if (x&3) goto err;
	x = (x>>2)-2;
	if (x < 0)
	{
		if ((x&(0xFF<<24)) != 0xFF<<24)
			goto err;
		x &= ~(0xFF<<24);
	}
	else if (x&(0xFF<<24))
		goto err;
	return x;
err:
	DIE("jump %d out of range at %d", x, pc);
}

void VM_Compile(vm_t *vm, vmHeader_t *header)
{
	unsigned char *code;
	int i_count, pc = 0;
	int pass;
	int codeoffsets[2]; // was 1024 but it's only used for OFF_CODE and OFF_IMMEDIATES

#define j_rel(x) (pass?_j_rel(x, pc):0xBAD)
#define OFFSET(i) (pass?(j_rel(codeoffsets[i]-vm->codeLength)):(0xF000000F))
//#define new_offset() (offsidx++)
#define get_offset(i) (codeoffsets[i])
#define save_offset(i) (codeoffsets[i] = vm->codeLength)
#define OFF_CODE 0
#define OFF_IMMEDIATES 1

	vm->compiled = qfalse;

	vm->codeBase = NULL;
	vm->codeLength = 0;

	for (pass = 0; pass < 2; ++pass) {

//	int offsidx = 0;

#ifdef CONST_OPTIMIZE
	// const optimization
	unsigned got_const = 0, const_value = 0;
#endif

	if(pass)
	{
		vm->codeBase = mmap(NULL, vm->codeLength, PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
		if(vm->codeBase == MAP_FAILED)
			Com_Error(ERR_FATAL, "VM_CompileARM: can't mmap memory");
		vm->codeLength = 0;
	}

	//int (*entry)(vm_t*, int*, int*);
	emit(PUSH((((1<<8)-1)<<4)|(1<<14))); // push R4-R11, LR
	emit(SUBi(SP, SP, 12)); // align stack!
	emit(LDRai(rCODEBASE, R0, offsetof(vm_t, codeBase)));
	emit(LDRai(rDATABASE, R0, offsetof(vm_t, dataBase)));
	emit(LDRai(rDATAMASK, R0, offsetof(vm_t, dataMask)));
	emit(LDRai(rPSTACK, R1, 0));
	emit(MOV(rOPSTACK, R2)); // TODO: reverse opstack to avoid writing to return address
	emit(MOV(rOPSTACKBASE, rOPSTACK));

	emit(BLi(OFFSET(OFF_CODE)));

	// save return value in r0
	emit(LDRTxi(R0, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4

	emit(ADDi(SP, SP, 12)); // align stack!
	emit(POP((((1<<8)-1)<<4)|(1<<15))); // pop R4-R11, LR -> PC

	/* save some immediates here */
	emit(BKPT(0));
	emit(BKPT(0));
	save_offset(OFF_IMMEDIATES);
//	emit((unsigned)whatever);
	emit(BKPT(0));
	emit(BKPT(0));

	save_offset(OFF_CODE);
//	offsidx = OFF_IMMEDIATES+1;

	code = (unsigned char *) header + header->codeOffset;
	pc = 0;

	for (i_count = 0; i_count < header->instructionCount; i_count++) {
		union {
			unsigned char b[4];
			unsigned int i;
		} arg;
		unsigned char op = code[pc++];

		vm->instructionPointers[i_count] = vm->codeLength;

		if (vm_opInfo[op] & opImm4)
		{
			memcpy(arg.b, &code[pc], 4);
			pc += 4;
#ifdef EXCESSIVE_DEBUG
			Com_Printf("%d: instruction %d (%s %d), offset %d\n", pass, i_count, opnames[op], arg.i, vm->codeLength);
#endif
		}
		else if (vm_opInfo[op] & opImm1)
		{
			arg.b[0] = code[pc];
			++pc;
#ifdef EXCESSIVE_DEBUG
			Com_Printf("%d: instruction %d (%s %hhd), offset %d\n", pass, i_count, opnames[op], arg.i, vm->codeLength);
#endif
		}
		else
		{
#ifdef EXCESSIVE_DEBUG
			Com_Printf("%d: instruction %d (%s), offset %d\n", pass, i_count, opnames[op], vm->codeLength);
#endif
		}

		// TODO: for debug only
		//emit_MOVRxi(R4, i_count);

		switch ( op )
		{
			case OP_UNDEF:
				break;

			case OP_IGNORE:
				NOTIMPL(op);
				break;

			case OP_BREAK:
				emit(BKPT(0));
				break;

			case OP_ENTER:
				MAYBE_EMIT_CONST();
				emit(PUSH1(LR));
				emit(SUBi(SP, SP, 12)); // align stack
				if (arg.i == 0 || can_encode(arg.i))
				{
					emit(SUBi(rPSTACK, rPSTACK, arg.i)); // pstack -= arg
				}
				else
				{
					emit_MOVR0i(arg.i);
					emit(SUB(rPSTACK, rPSTACK, R0)); // pstack -= arg
				}
				break;

			case OP_LEAVE:
				if (arg.i == 0 || can_encode(arg.i))
				{
					emit(ADDi(rPSTACK, rPSTACK, arg.i)); // pstack += arg
				}
				else
				{
					emit_MOVR0i(arg.i);
					emit(ADD(rPSTACK, rPSTACK, R0)); // pstack += arg
				}
				emit(ADDi(SP, SP, 12));
				emit(0xe49df004); // pop pc
				break;

			case OP_CALL:
#if 0
				// save next instruction
				emit_MOVR0i(i_count);
				emit(STRa(R0, rDATABASE, rPSTACK));      // dataBase[pstack] = r0
#endif
#ifdef CONST_OPTIMIZE
				if (got_const)
				{
					NOTIMPL(op);
				}
				else
#endif
				{
					static int bytes_to_skip = -1;
					static unsigned start_block = -1;
					MAYBE_EMIT_CONST();
					// get instruction nr from stack
					emit(LDRTxi(R0, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4
					emit(CMPi(R0, 0)); // check if syscall
					if (start_block == -1)
						start_block = vm->codeLength;
					emit(cond(LT, Bi(j_rel(bytes_to_skip))));
						CHECK_JUMP;
						emit_MOVRxi_or_NOP(R1, (unsigned)vm->instructionPointers);
						emit(LDRa(R0, R1, rLSL(2, R0))); // r0 = ((int*)r1)[r0]
						emit(ADD(R0, rCODEBASE, R0)); // r0 = codeBase+r0
						emit(BLX(R0));
						emit(Bi(j_rel(vm->instructionPointers[i_count+1]-vm->codeLength)));
					if (bytes_to_skip == -1)
						bytes_to_skip = vm->codeLength - start_block;
					emit(MOV(R1, rPSTACK));
					emit_MOVRxi(R12, (unsigned)asmcall);
					emit(BLX(R12));
					// store return value
					emit(STRaiw(R0, rOPSTACK, 4));      // opstack+=4; *opstack = r0
				}
				break;

			case OP_PUSH:
				MAYBE_EMIT_CONST();
				emit(ADDi(rOPSTACK, rOPSTACK, 4));
				break;

			case OP_POP:
				MAYBE_EMIT_CONST();
				emit(SUBi(rOPSTACK, rOPSTACK, 4));
				break;

			case OP_CONST:
				MAYBE_EMIT_CONST();
				emit_MOVR0i(arg.i);
				emit(STRaiw(R0, rOPSTACK, 4));      // opstack+=4; *opstack = r0
				break;

			case OP_LOCAL:
				MAYBE_EMIT_CONST();
				if (arg.i == 0 || can_encode(arg.i))
				{
					emit(ADDi(R0, rPSTACK, arg.i));     // r0 = pstack+arg
				}
				else
				{
					emit_MOVR0i(arg.i);
					emit(ADD(R0, rPSTACK, R0));     // r0 = pstack+arg
				}
				emit(STRaiw(R0, rOPSTACK, 4));      // opstack+=4; *opstack = r0
				break;

			case OP_JUMP:
#ifdef CONST_OPTIMIZE
				if (got_const)
				{
					NOTIMPL(op);
				}
				else
#endif
				{
					emit(LDRTxi(R0, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4
					CHECK_JUMP;
					emit_MOVRxi(R1, (unsigned)vm->instructionPointers);
					emit(LDRa(R0, R1, rLSL(2, R0))); // r0 = ((int*)r1)[r0]
					emit(ADD(R0, rCODEBASE, R0)); // r0 = codeBase+r0
					emit(BLX(R0));
				}
				break;

			case OP_EQ:
				IJ(EQ);
				break;

			case OP_NE:
				IJ(NE);
				break;

			case OP_LTI:
				IJ(LT);
				break;

			case OP_LEI:
				IJ(LE);
				break;

			case OP_GTI:
				IJ(GT);
				break;

			case OP_GEI:
				IJ(GE);
				break;

			case OP_LTU:
				IJ(LO);
				break;

			case OP_LEU:
				IJ(LS);
				break;

			case OP_GTU:
				IJ(HI);
				break;

			case OP_GEU:
				IJ(HS);
				break;

			case OP_EQF:
				FJ(EQ);
				break;

			case OP_NEF:
				FJ(NE);
				break;

			case OP_LTF:
				FJ(LT);
				break;

			case OP_LEF:
				FJ(LE);
				break;

			case OP_GTF:
				FJ(GT);
				break;

			case OP_GEF:
				FJ(GE);
				break;

			case OP_LOAD1:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));   // r0 = *opstack
				emit(AND(R0, rDATAMASK, R0));    // r0 = r0 & rDATAMASK
				emit(LDRBa(R0, rDATABASE, R0));  // r0 = (unsigned char)dataBase[r0]
				emit(STRai(R0, rOPSTACK, 0));   // *opstack = r0
				break;

			case OP_LOAD2:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));   // r0 = *opstack
				emit(AND(R0, rDATAMASK, R0));    // r0 = r0 & rDATAMASK
				emit(LDRHa(R0, rDATABASE, R0));  // r0 = (unsigned short)dataBase[r0]
				emit(STRai(R0, rOPSTACK, 0));   // *opstack = r0
				break;

			case OP_LOAD4:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));   // r0 = *opstack
				emit(AND(R0, rDATAMASK, R0));    // r0 = r0 & rDATAMASK
				emit(LDRa(R0, rDATABASE, R0));  // r0 = dataBase[r0]
				emit(STRai(R0, rOPSTACK, 0));   // *opstack = r0
				break;

			case OP_STORE1:
				MAYBE_EMIT_CONST();
				emit(LDRTxi(R0, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4
				emit(LDRTxi(R1, rOPSTACK, 4));  // r1 = *opstack; rOPSTACK -= 4
				emit(AND(R1, rDATAMASK, R1));    // r1 = r1 & rDATAMASK
				emit(STRBa(R0, rDATABASE, R1)); // database[r1] = r0
				break;

			case OP_STORE2:
				MAYBE_EMIT_CONST();
				emit(LDRTxi(R0, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4
				emit(LDRTxi(R1, rOPSTACK, 4));  // r1 = *opstack; rOPSTACK -= 4
				emit(AND(R1, rDATAMASK, R1));    // r1 = r1 & rDATAMASK
				emit(STRHa(R0, rDATABASE, R1)); // database[r1] = r0
				break;

			case OP_STORE4:
				MAYBE_EMIT_CONST();
				// optimize: use load multiple
				// value
				emit(LDRTxi(R0, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4
				// pointer
				emit(LDRTxi(R1, rOPSTACK, 4));  // r1 = *opstack; rOPSTACK -= 4
				emit(AND(R1, rDATAMASK, R1));    // r1 = r1 & rDATAMASK
				// store value at pointer
				emit(STRa(R0, rDATABASE, R1)); // database[r1] = r0
				break;

			case OP_ARG:
				MAYBE_EMIT_CONST();
				emit(LDRTxi(R0, rOPSTACK, 4));      // r0 = *opstack; rOPSTACK -= 4
				emit(ADDi(R1, rPSTACK, arg.b[0]));  // r1 = programStack+arg
				emit(AND(R1, rDATAMASK, R1));       // r1 = r1 & rDATAMASK
				emit(STRa(R0, rDATABASE, R1));      // dataBase[r1] = r0
				break;

			case OP_BLOCK_COPY:
				MAYBE_EMIT_CONST();
				emit(LDRTxi(R1, rOPSTACK, 4));  // r0 = *opstack; rOPSTACK -= 4
				emit(LDRTxi(R0, rOPSTACK, 4));
				emit_MOVRxi(R2, arg.i);
				emit_MOVRxi(R12, (unsigned)VM_BlockCopy);
				emit(BLX(R12));
				break;

			case OP_SEX8:
				MAYBE_EMIT_CONST();
				emit(LDRSBai(R0, rOPSTACK, 0));      // sign extend *opstack
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_SEX16:
				MAYBE_EMIT_CONST();
				emit(LDRSHai(R0, rOPSTACK, 0));      // sign extend *opstack
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_NEGI:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(RSBi(R0, R0, 0));         // r0 = -r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_ADD:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(ADD(R0, R1, R0));         // r0 = r1 + r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_SUB:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(SUB(R0, R1, R0));         // r0 = r1 - r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_DIVI:
			case OP_DIVU:
				MAYBE_EMIT_CONST();
				emit(LDRai(R1, rOPSTACK, 0));  // r1 = *opstack
				emit(LDRxiw(R0, rOPSTACK, 4)); // opstack-=4; r0 = *opstack
				if ( op == OP_DIVI )
					emit_MOVRxi(R12, (unsigned)__aeabi_idiv);
				else
					emit_MOVRxi(R12, (unsigned)__aeabi_uidiv);
				emit(BLX(R12));
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_MODI:
			case OP_MODU:
				MAYBE_EMIT_CONST();
				emit(LDRai(R1, rOPSTACK, 0));  // r1 = *opstack
				emit(LDRxiw(R0, rOPSTACK, 4)); // opstack-=4; r0 = *opstack
				if ( op == OP_MODI )
					emit_MOVRxi(R12, (unsigned)__aeabi_idivmod);
				else
					emit_MOVRxi(R12, (unsigned)__aeabi_uidivmod);
				emit(BLX(R12));
				emit(STRai(R1, rOPSTACK, 0));  // *opstack = r1
				break;

			case OP_MULI:
			case OP_MULU:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(MUL(R0, R1, R0));         // r0 = r1 * r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_BAND:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(AND(R0, R1, R0));         // r0 = r1 & r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_BOR:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(ORR(R0, R1, R0));         // r0 = r1 | r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_BXOR:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(EOR(R0, R1, R0));         // r0 = r1 ^ r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_BCOM:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(MVN(R0, R0));             // r0 = ~r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_LSH:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(LSL(R0, R1, R0));         // r0 = r1 << r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_RSHI:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(ASR(R0, R1, R0));         // r0 = r1 >> r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_RSHU:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(LDRxiw(R1, rOPSTACK, 4)); // opstack-=4; r1 = *opstack
				emit(LSR(R0, R1, R0));         // r0 = (unsigned)r1 >> r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;

			case OP_NEGF:
				MAYBE_EMIT_CONST();
				emit(VLDRa(S14, rOPSTACK, 0)); // s14 = *((float*)opstack)
				emit(VNEG_F32(S14, S14));      // s15 = -s14
				emit(VSTRa(S14, rOPSTACK, 0)); // *((float*)opstack) = s15
				break;

			case OP_ADDF:
				MAYBE_EMIT_CONST();
				emit(VLDRa(S14, rOPSTACK, 0));   // s14 = *((float*)opstack)
				// vldr can't modify rOPSTACK so
				// we'd either need to change it
				// with sub or use regular ldr+vmov
				emit(LDRxiw(R0, rOPSTACK, 4));   // opstack-=4; r1 = *opstack
				emit(VMOVass(S15,R0));           // s15 = r0
				emit(VADD_F32(S14, S15, S14));   // s14 = s14 + s15
				emit(VSTRa(S14, rOPSTACK, 0));   // *((float*)opstack) = s15
				break;

			case OP_SUBF:
				emit(VLDRa(S14, rOPSTACK, 0));   // s14 = *((float*)opstack)
				// see OP_ADDF
				emit(LDRxiw(R0, rOPSTACK, 4));   // opstack-=4; r1 = *opstack
				emit(VMOVass(S15,R0));           // s15 = r0
				emit(VSUB_F32(S14, S15, S14));   // s14 = s14 - s15
				emit(VSTRa(S14, rOPSTACK, 0));   // *((float*)opstack) = s15
				break;

			case OP_DIVF:
				emit(VLDRa(S14, rOPSTACK, 0));   // s14 = *((float*)opstack)
				// see OP_ADDF
				emit(LDRxiw(R0, rOPSTACK, 4));   // opstack-=4; r1 = *opstack
				emit(VMOVass(S15,R0));           // s15 = r0
				emit(VDIV_F32(S14, S15, S14));   // s14 = s14 / s15
				emit(VSTRa(S14, rOPSTACK, 0));   // *((float*)opstack) = s15
				break;

			case OP_MULF:
				emit(VLDRa(S14, rOPSTACK, 0));   // s14 = *((float*)opstack)
				// see OP_ADDF
				emit(LDRxiw(R0, rOPSTACK, 4));   // opstack-=4; r1 = *opstack
				emit(VMOVass(S15,R0));           // s15 = r0
				emit(VMUL_F32(S14, S15, S14));   // s14 = s14 * s15
				emit(VSTRa(S14, rOPSTACK, 0));   // *((float*)opstack) = s15
				break;

			case OP_CVIF:
				MAYBE_EMIT_CONST();
				emit(LDRai(R0, rOPSTACK, 0));  // r0 = *opstack
				emit(VMOVass(S14,R0));         // s14 = r0
				emit(VCVT_F32_S32(S14, S14));  // s15 = (float)s14
				emit(VSTRa(S14, rOPSTACK, 0)); // *((float*)opstack) = s15
				break;

			case OP_CVFI:
				MAYBE_EMIT_CONST();
				emit(VLDRa(S14, rOPSTACK, 0)); // s14 = *((float*)opstack)
				emit(VCVT_S32_F32(S14, S14));  // s15 = (int)s14
				emit(VMOVssa(R0,S14));         // s14 = r0
				emit(STRai(R0, rOPSTACK, 0));  // *opstack = r0
				break;
		}
	}

	// never reached
	emit(BKPT(0));
	} // pass

	if (mprotect(vm->codeBase, vm->codeLength, PROT_READ|PROT_EXEC/* |PROT_WRITE */)) {
		VM_Destroy_Compiled(vm);
		DIE("mprotect failed");
	}

	// clear icache, http://blogs.arm.com/software-enablement/141-caches-and-self-modifying-code/ 
	__clear_cache(vm->codeBase, vm->codeBase+vm->codeLength);

	vm->destroy = VM_Destroy_Compiled;
	vm->compiled = qtrue;
}

int VM_CallCompiled(vm_t *vm, int *args)
{
	byte	stack[OPSTACK_SIZE + 15];
	int	*opStack;
	int	programStack = vm->programStack;
	int	stackOnEntry = programStack;
	byte	*image = vm->dataBase;
	int	*argPointer;
	int	retVal;

	currentVM = vm;

	vm->currentlyInterpreting = qtrue;

	programStack -= ( 8 + 4 * MAX_VMMAIN_ARGS );
	argPointer = (int *)&image[ programStack + 8 ];
	memcpy( argPointer, args, 4 * MAX_VMMAIN_ARGS );
	argPointer[-1] = 0;
	argPointer[-2] = -1;


	opStack = PADP(stack, 16);
	*opStack = 0xDEADBEEF;

#if 0
	Com_Printf("r5 opStack:\t\t%p\n", opStack);
	Com_Printf("r7 codeBase:\t\t%p\n", vm->codeBase);
	Com_Printf("r8 programStack:\t0x%x\n", programStack);
	Com_Printf("r9 dataBase:\t\t%p\n", vm->dataBase);
#endif

	/* call generated code */
	{
		//int (*entry)(void *, int, void *, int);
		int (*entry)(vm_t*, int*, int*);

		entry = (void *)(vm->codeBase);
		//__asm__ volatile("bkpt");
		//retVal = entry(vm->codeBase, programStack, vm->dataBase, vm->dataMask);
		retVal = entry(vm, &programStack, opStack);
	}

	if(*opStack != 0xDEADBEEF)
	{
		Com_Error(ERR_DROP, "opStack corrupted in compiled code");
	}

	if(programStack != stackOnEntry - (8 + 4 * MAX_VMMAIN_ARGS))
		Com_Error(ERR_DROP, "programStack corrupted in compiled code");

	vm->programStack = stackOnEntry;
	vm->currentlyInterpreting = qfalse;

	return retVal;
}
