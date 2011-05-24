/*
===========================================================================
Copyright (C) 2009 David S. Miller <davem@davemloft.net>

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

/* This code is based almost entirely upon the vm_powerpc.c code by
 * Przemyslaw Iskra.  All I did was make it work on Sparc :-) -DaveM
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <stddef.h>

#include "vm_local.h"
#include "vm_sparc.h"

/* exit() won't be called but use it because it is marked with noreturn */
#define DIE( reason ) \
	do { \
		Com_Error(ERR_DROP, "vm_sparc compiler error: " reason); \
		exit(1); \
	} while(0)

/* Select Length - first value on 32 bits, second on 64 */
#ifdef __arch64__
#define SL(a, b) (b)
#else
#define SL(a, b) (a)
#endif

#define rTMP		G1
#define rVMDATA		G2
#define rPSTACK		G3
#define rDATABASE	G4
#define rDATAMASK	G5

struct sparc_opcode {
	const char	*name;
	unsigned int	opcode;
	unsigned int	mask;
	unsigned char	args[4];
#define ARG_NONE	0
#define ARG_RS1		1
#define ARG_RS2		2
#define ARG_RD		3
#define ARG_SIMM13	4
#define ARG_DISP30	5
#define ARG_IMM22	6
#define ARG_DISP22	7
#define ARG_SWTRAP	8
};

#define ARG_RS1_RS2_RD		{ ARG_RS1, ARG_RS2, ARG_RD }
#define ARG_RS1_SIMM13_RD	{ ARG_RS1, ARG_SIMM13, ARG_RD }
#define ARG_RS1_RS2		{ ARG_RS1, ARG_RS2 }
#define ARG_RS2_RD		{ ARG_RS2, ARG_RD }

#define OP_MASK		0xc0000000
#define OP2_MASK	0x01c00000
#define OP3_MASK	0x01f80000
#define OPF_MASK	0x00003fe0

#define IMM		0x00002000

#define FMT1(op)		((op) << 30), OP_MASK
#define FMT2(op,op2)		((op) << 30)|((op2)<<22), (OP_MASK | OP2_MASK)
#define FMT3(op,op3)		((op) << 30)|((op3)<<19), (OP_MASK | OP3_MASK | IMM)
#define FMT3I(op,op3)		((op) << 30)|((op3)<<19)|IMM, (OP_MASK | OP3_MASK | IMM)
#define FMT3F(op,op3,opf)	((op) << 30)|((op3)<<19)|((opf)<<5), \
				(OP_MASK | OP3_MASK | OPF_MASK)

#define BICC(A,COND)		FMT2(0,((A<<7)|(COND<<3)|0x2))
#define BFCC(A,COND)		FMT2(0,((A<<7)|(COND<<3)|0x6))
#define TICC(COND)		FMT3I(0,((COND<<6)|0x3a))

enum sparc_iname {
	CALL, NOP, SETHI,

	BA, BN, BNE, BE, BG, BLE, BGE, BL, BGU, BLEU, BCC, BCS,
	BPOS, BNEG, BVC, BVS,

	ADDI, ADD,
	ANDI, AND,
	ORI, OR,
	XORI, XOR,
	SUBI, SUB,
	ANDNI, ANDN,
	ORNI, ORN,
	XNORI, XNOR,

	UMULI, UMUL,
	SMULI, SMUL,
	UDIVI, UDIV,
	SDIVI, SDIV,

	SUBCCI, SUBCC,

	SLLI, SLL,
	SRLI, SRL,
	SRAI, SRA,

	WRI, WR,

	SAVEI, SAVE,
	RESTOREI, RESTORE,

	TA,

	JMPLI, JMPL,

	LDXI, LDX,
	LDUWI, LDUW,
	LDUHI, LDUH,
	LDUBI, LDUB,

	STXI, STX,
	STWI, STW,
	STHI, STH,
	STBI, STB,

	LDFI, LDF,
	STFI, STF,

	FADD, FSUB, FCMP, FSTOI, FITOS, FNEG, FDIV, FMUL,
	FBE, FBNE, FBL, FBGE, FBG, FBLE,
};

#define LDLI	SL(LDUWI, LDXI)
#define LDL	SL(LDUW, LDX)
#define STLI	SL(STWI, STXI)
#define STL	SL(STW, STX)

#define SPARC_NOP	0x01000000

static const struct sparc_opcode sparc_opcodes[] = {
	{ "call",	FMT1(1), { ARG_DISP30 }, },
	{ "nop",	SPARC_NOP, 0xffffffff, { ARG_NONE }, }, /* sethi %hi(0), %g0 */
	{ "sethi",	FMT2(0,4), { ARG_IMM22, ARG_RD }, },
	{ "ba",		BICC(0,8), { ARG_DISP22 }, },
	{ "bn",		BICC(0,0), { ARG_DISP22 }, },
	{ "bne",	BICC(0,9), { ARG_DISP22 }, },
	{ "be",		BICC(0,1), { ARG_DISP22 }, },
	{ "bg",		BICC(0,10), { ARG_DISP22 }, },
	{ "ble",	BICC(0,2), { ARG_DISP22 }, },
	{ "bge",	BICC(0,11), { ARG_DISP22 }, },
	{ "bl",		BICC(0,3), { ARG_DISP22 }, },
	{ "bgu",	BICC(0,12), { ARG_DISP22 }, },
	{ "bleu",	BICC(0,4), { ARG_DISP22 }, },
	{ "bcc",	BICC(0,13), { ARG_DISP22 }, },
	{ "bcs",	BICC(0,5), { ARG_DISP22 }, },
	{ "bpos",	BICC(0,14), { ARG_DISP22 }, },
	{ "bneg",	BICC(0,6), { ARG_DISP22 }, },
	{ "bvc",	BICC(0,15), { ARG_DISP22 }, },
	{ "bvs",	BICC(0,7), { ARG_DISP22 }, },

	{ "add",	FMT3I(2, 0x00), ARG_RS1_SIMM13_RD, },
	{ "add",	FMT3 (2, 0x00), ARG_RS1_RS2_RD,    },
	{ "and",	FMT3I(2, 0x01), ARG_RS1_SIMM13_RD, },
	{ "and",	FMT3 (2, 0x01), ARG_RS1_RS2_RD,    },
	{ "or",		FMT3I(2, 0x02), ARG_RS1_SIMM13_RD, },
	{ "or",		FMT3 (2, 0x02), ARG_RS1_RS2_RD,    },
	{ "xor",	FMT3I(2, 0x03), ARG_RS1_SIMM13_RD, },
	{ "xor",	FMT3 (2, 0x03), ARG_RS1_RS2_RD,    },
	{ "sub",	FMT3I(2, 0x04), ARG_RS1_SIMM13_RD, },
	{ "sub",	FMT3 (2, 0x04), ARG_RS1_RS2_RD,    },
	{ "andn",	FMT3I(2, 0x05), ARG_RS1_SIMM13_RD, },
	{ "andn",	FMT3 (2, 0x05), ARG_RS1_RS2_RD,    },
	{ "orn",	FMT3I(2, 0x06), ARG_RS1_SIMM13_RD, },
	{ "orn",	FMT3 (2, 0x06), ARG_RS1_RS2_RD,    },
	{ "xnor",	FMT3I(2, 0x07), ARG_RS1_SIMM13_RD, },
	{ "xnor",	FMT3 (2, 0x07), ARG_RS1_RS2_RD,    },

	{ "umul",	FMT3I(2, 0x0a), ARG_RS1_SIMM13_RD, },
	{ "umul",	FMT3 (2, 0x0a), ARG_RS1_RS2_RD,    },
	{ "smul",	FMT3I(2, 0x0b), ARG_RS1_SIMM13_RD, },
	{ "smul",	FMT3 (2, 0x0b), ARG_RS1_RS2_RD,    },
	{ "udiv",	FMT3I(2, 0x0e), ARG_RS1_SIMM13_RD, },
	{ "udiv",	FMT3 (2, 0x0e), ARG_RS1_RS2_RD,    },
	{ "sdiv",	FMT3I(2, 0x0f), ARG_RS1_SIMM13_RD, },
	{ "sdiv",	FMT3 (2, 0x0f), ARG_RS1_RS2_RD,    },

	{ "subcc",	FMT3I(2, 0x14), ARG_RS1_SIMM13_RD, },
	{ "subcc",	FMT3 (2, 0x14), ARG_RS1_RS2_RD,    },

	{ "sll",	FMT3I(2, 0x25), ARG_RS1_SIMM13_RD, },
	{ "sll",	FMT3 (2, 0x25), ARG_RS1_RS2_RD,    },
	{ "srl",	FMT3I(2, 0x26), ARG_RS1_SIMM13_RD, },
	{ "srl",	FMT3 (2, 0x26), ARG_RS1_RS2_RD,    },
	{ "sra",	FMT3I(2, 0x27), ARG_RS1_SIMM13_RD, },
	{ "sra",	FMT3 (2, 0x27), ARG_RS1_RS2_RD,    },

	{ "wr",		FMT3I(2, 0x30), ARG_RS1_SIMM13_RD, },
	{ "wr",		FMT3 (2, 0x30), ARG_RS1_SIMM13_RD, },

	{ "save",	FMT3I(2,0x3c), ARG_RS1_SIMM13_RD, },
	{ "save",	FMT3 (2,0x3c), ARG_RS1_RS2_RD,    },
	{ "restore",	FMT3I(2,0x3d), ARG_RS1_SIMM13_RD, },
	{ "restore",	FMT3 (2,0x3d), ARG_RS1_RS2_RD,    },
	{ "ta",		TICC(8), { ARG_SWTRAP, ARG_NONE }, },
	{ "jmpl",	FMT3I(2,0x38), ARG_RS1_SIMM13_RD, },
	{ "jmpl",	FMT3 (2,0x38), ARG_RS1_RS2_RD,    },

	{ "ldx",	FMT3I(3,0x0b), ARG_RS1_SIMM13_RD, },
	{ "ldx",	FMT3 (3,0x0b), ARG_RS1_RS2_RD,    },
	{ "lduw",	FMT3I(3,0x00), ARG_RS1_SIMM13_RD, },
	{ "lduw",	FMT3 (3,0x00), ARG_RS1_RS2_RD,    },
	{ "lduh",	FMT3I(3,0x02), ARG_RS1_SIMM13_RD, },
	{ "lduh",	FMT3 (3,0x02), ARG_RS1_RS2_RD,    },
	{ "ldub",	FMT3I(3,0x01), ARG_RS1_SIMM13_RD, },
	{ "ldub",	FMT3 (3,0x01), ARG_RS1_RS2_RD,    },

	{ "stx",	FMT3I(3,0x0e), ARG_RS1_SIMM13_RD, },
	{ "stx",	FMT3 (3,0x0e), ARG_RS1_RS2_RD,    },
	{ "stw",	FMT3I(3,0x04), ARG_RS1_SIMM13_RD, },
	{ "stw",	FMT3 (3,0x04), ARG_RS1_RS2_RD,    },
	{ "sth",	FMT3I(3,0x06), ARG_RS1_SIMM13_RD, },
	{ "sth",	FMT3 (3,0x06), ARG_RS1_RS2_RD,    },
	{ "stb",	FMT3I(3,0x05), ARG_RS1_SIMM13_RD, },
	{ "stb",	FMT3 (3,0x05), ARG_RS1_RS2_RD,    },

	{ "ldf",	FMT3I(3,0x20), ARG_RS1_SIMM13_RD, },
	{ "ldf",	FMT3 (3,0x20), ARG_RS1_RS2_RD,    },
	{ "stf",	FMT3I(3,0x24), ARG_RS1_SIMM13_RD, },
	{ "stf",	FMT3 (3,0x24), ARG_RS1_RS2_RD, },

	{ "fadd",	FMT3F(2,0x34,0x041), ARG_RS1_RS2_RD, },
	{ "fsub",	FMT3F(2,0x34,0x045), ARG_RS1_RS2_RD, },
	{ "fcmp",	FMT3F(2,0x35,0x051), ARG_RS1_RS2, },
	{ "fstoi",	FMT3F(2,0x34,0x0d1), ARG_RS2_RD, },
	{ "fitos",	FMT3F(2,0x34,0x0c4), ARG_RS2_RD, },

	{ "fneg",	FMT3F(2,0x34,0x005), ARG_RS2_RD, },
	{ "fdiv",	FMT3F(2,0x34,0x04d), ARG_RS1_RS2_RD, },
	{ "fmul",	FMT3F(2,0x34,0x049), ARG_RS1_RS2_RD, },

	{ "fbe",	BFCC(0,9), { ARG_DISP22 }, },
	{ "fbne",	BFCC(0,1), { ARG_DISP22 }, },
	{ "fbl",	BFCC(0,4), { ARG_DISP22 }, },
	{ "fbge",	BFCC(0,11), { ARG_DISP22 }, },
	{ "fbg",	BFCC(0,6), { ARG_DISP22 }, },
	{ "fble",	BFCC(0,13), { ARG_DISP22 }, },
};
#define SPARC_NUM_OPCODES (ARRAY_LEN(sparc_opcodes))

#define RS1(X)			(((X) & 0x1f) << 14)
#define RS2(X)			(((X) & 0x1f) << 0)
#define RD(X)			(((X) & 0x1f) << 25)
#define SIMM13(X)		(((X) & 0x1fff) << 0)
#define IMM22(X)		(((X) & 0x3fffff) << 0)
#define DISP30(X)		((((X) >> 2) & 0x3fffffff) << 0)
#define DISP22(X)		((((X) >> 2) & 0x3fffff) << 0)
#define SWTRAP(X)		(((X) & 0x7f) << 0)

#define SIMM13_P(X)		((unsigned int) (X) + 0x1000 < 0x2000)

static void vimm(unsigned int val, int bits, int shift, int sgned, int arg_index)
{
	unsigned int orig_val = val;
	int orig_bits = bits;

	if (sgned) {
		int x = (int) val;
		if (x < 0)
			x = -x;
		val = (unsigned int) x;
		bits--;
	}
	if (val & ~((1U << bits) - 1U)) {
		Com_Printf("VM ERROR: immediate value 0x%08x out of %d bit range\n",
			   orig_val, orig_bits);
		DIE("sparc VM bug");
	}
}

static unsigned int sparc_assemble(enum sparc_iname iname, const int argc, const int *argv)
{
	const struct sparc_opcode *op = &sparc_opcodes[iname];
	unsigned int insn = op->opcode;
	int i, flt, rd_flt;

	flt = (op->name[0] == 'f');
	rd_flt = flt || (op->name[2] == 'f');

	for (i = 0; op->args[i] != ARG_NONE; i++) {
		int val = argv[i];

		switch (op->args[i]) {
		case ARG_RS1: insn |= RS1(val); break;
		case ARG_RS2: insn |= RS2(val); break;
		case ARG_RD:  insn |= RD(val); break;
		case ARG_SIMM13: insn |= SIMM13(val); vimm(val,13,0,1,i); break;
		case ARG_DISP30: insn |= DISP30(val); vimm(val,30,0,1,i); break;
		case ARG_IMM22: insn |= IMM22(val); vimm(val,22,0,0,i); break;
		case ARG_DISP22: insn |= DISP22(val); vimm(val,22,0,1,i); break;
		case ARG_SWTRAP: insn |= SWTRAP(val); vimm(val,7,0,0,i); break;
		}
	}

	return insn;
}

#define IN(inst, args...) \
({	const int argv[] = { args }; \
	const int argc = ARRAY_LEN(argv); \
	sparc_assemble(inst, argc, argv); \
})

#if 0
static void pgreg(int reg_num, int arg_index, int flt)
{
	if (!flt) {
		const char *fmt[] = { "%g", "%o", "%l", "%i" };

		Com_Printf("%s%s%d",
			   (arg_index ? ", " : ""),
			   fmt[reg_num >> 3], reg_num & 7);
	} else
		Com_Printf("%s%%f%d", (arg_index ? ", " : ""), reg_num);
}

static void pimm(unsigned int val, int bits, int shift, int sgned, int arg_index)
	
{
	val >>= shift;
	val &= ((1 << bits) - 1);
	if (sgned) {
		int sval = val << (32 - bits);
		sval >>= (32 - bits);
		Com_Printf("%s%d",
			   (arg_index ? ", " : ""), sval);
	} else
		Com_Printf("%s0x%08x",
			   (arg_index ? ", " : ""), val);
}

static void sparc_disassemble(unsigned int insn)
{
	int op_idx;

	for (op_idx = 0; op_idx < SPARC_NUM_OPCODES; op_idx++) {
		const struct sparc_opcode *op = &sparc_opcodes[op_idx];
		int i, flt, rd_flt;

		if ((insn & op->mask) != op->opcode)
			continue;

		flt = (op->name[0] == 'f');
		rd_flt = flt || (op->name[2] == 'f');

		Com_Printf("ASM: %7s\t", op->name);
		for (i = 0; op->args[i] != ARG_NONE; i++) {
			switch (op->args[i]) {
			case ARG_RS1: pgreg((insn >> 14) & 0x1f, i, flt); break;
			case ARG_RS2: pgreg((insn >> 0) & 0x1f, i, flt); break;
			case ARG_RD:  pgreg((insn >> 25) & 0x1f, i, rd_flt); break;
			case ARG_SIMM13: pimm(insn, 13, 0, 1, i); break;
			case ARG_DISP30: pimm(insn, 30, 0, 0, i); break;
			case ARG_IMM22: pimm(insn, 22, 0, 0, i); break;
			case ARG_DISP22: pimm(insn, 22, 0, 0, i); break;
			case ARG_SWTRAP: pimm(insn, 7, 0, 0, i); break;
			}
		}
		Com_Printf("\n");
		return;
	}
}
#endif

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

static void VM_Destroy_Compiled(vm_t *vm)
{
	if (vm->codeBase) {
		if (munmap(vm->codeBase, vm->codeLength))
			Com_Printf(S_COLOR_RED "Memory unmap failed, possible memory leak\n");
	}
	vm->codeBase = NULL;
}

typedef struct VM_Data {
	unsigned int dataLength;
	unsigned int codeLength;
	unsigned int *CallThunk;
	int (*AsmCall)(int, int);
	void (*BlockCopy)(unsigned int, unsigned int, unsigned int);
	unsigned int *iPointers;
	void (*ErrJump)(void);
	unsigned int data[0];
} vm_data_t;

#ifdef offsetof
# define VM_Data_Offset(field)		offsetof(vm_data_t, field)
#else
# define OFFSET(structName, field) \
	((void *)&(((structName *)NULL)->field) - NULL)
# define VM_Data_Offset(field)		OFFSET(vm_data_t, field)
#endif

struct src_insn {
	unsigned char		op;
	unsigned int		i_count;

	union {
		unsigned int	i;
		signed int	si;
		signed short	ss[2];
		unsigned short	us[2];
		unsigned char	b;
	} arg;

	unsigned char		dst_reg_flags;
	unsigned char		src1_reg_flags;
	unsigned char		src2_reg_flags;
#define REG_FLAGS_FLOAT		0x1

	struct src_insn		*next;
};

struct dst_insn;
struct jump_insn {
	enum sparc_iname	jump_iname;
	int			jump_dest_insn;
	struct dst_insn		*parent;
	struct jump_insn	*next;
};

struct dst_insn {
	struct dst_insn		*next;

	unsigned int		count;
	unsigned int		i_count;

	struct jump_insn	*jump;
	unsigned int		length;
	unsigned int		code[0];
};

#define HUNK_SIZE		29
struct data_hunk {
	struct data_hunk *next;
	int count;
	unsigned int data[HUNK_SIZE];
};

struct func_info {
	struct src_insn		*first;
	struct src_insn		*last;
	int			has_call;
	int			need_float_tmp;

	struct src_insn		*cached_const;

	int			stack_space;
	int			gpr_pos;
#define rFIRST(fp)		((fp)->gpr_pos - 1)
#define rSECOND(fp)		((fp)->gpr_pos - 2)
#define POP_GPR(fp)		((fp)->gpr_pos--)
#define PUSH_GPR(fp)		((fp)->gpr_pos++)

	int			fpr_pos;
#define fFIRST(fp)		((fp)->fpr_pos - 1)
#define fSECOND(fp)		((fp)->fpr_pos - 2)
#define POP_FPR(fp)		((fp)->fpr_pos--)
#define PUSH_FPR(fp)		((fp)->fpr_pos++)

#define INSN_BUF_SIZE		50
	unsigned int		insn_buf[INSN_BUF_SIZE];
	int			insn_index;

	int			saved_icount;
	int			force_emit;

	struct jump_insn	*jump_first;
	struct jump_insn	*jump_last;

	struct dst_insn		*dst_first;
	struct dst_insn		*dst_last;
	int			dst_count;

	struct dst_insn		**dst_by_i_count;

	struct data_hunk	*data_first;
	int			data_num;
};

#define THUNK_ICOUNT		-1

static unsigned int sparc_push_data(struct func_info * const fp, unsigned int val)
{
	struct data_hunk *last, *dp = fp->data_first;
	int off = 0;

	last = NULL;
	while (dp) {
		int i;

		for (i = 0; i < dp->count; i++) {
			if (dp->data[i] == val) {
				off += i;
				return VM_Data_Offset(data[off]);
			}
		}
		off += dp->count;
		last = dp;
		dp = dp->next;
	}

	dp = last;
	if (!dp || dp->count >= HUNK_SIZE) {
		struct data_hunk *new = Z_Malloc(sizeof(*new));
		if (!dp)
			fp->data_first = new;
		else
			dp->next = new;
		dp = new;
		dp->count = 0;
		dp->next = NULL;
	}
	dp->data[dp->count++] = val;
	fp->data_num = off + 1;
	return VM_Data_Offset(data[off]);
}

static void dst_insn_insert_tail(struct func_info * const fp,
				 struct dst_insn *dp)
{
	if (!fp->dst_first) {
		fp->dst_first = fp->dst_last = dp;
	} else {
		fp->dst_last->next = dp;
		fp->dst_last = dp;
	}
}

static void jump_insn_insert_tail(struct func_info * const fp,
				  struct jump_insn *jp)
{
	if (!fp->jump_first) {
		fp->jump_first = fp->jump_last = jp;
	} else {
		fp->jump_last->next = jp;
		fp->jump_last = jp;
	}
}

static struct dst_insn *dst_new(struct func_info * const fp, unsigned int length,
				struct jump_insn *jp, int insns_size)
{
	struct dst_insn *dp = Z_Malloc(sizeof(struct dst_insn) + insns_size);

	dp->length = length;
	dp->jump = jp;
	dp->count = fp->dst_count++;
	dp->i_count = fp->saved_icount;
	dp->next = NULL;
	if (fp->saved_icount != THUNK_ICOUNT)
		fp->dst_by_i_count[fp->saved_icount] = dp;

	return dp;
}

static void dst_insn_append(struct func_info * const fp)
{
	int insns_size = (sizeof(unsigned int) * fp->insn_index);
	struct dst_insn *dp;

	dp = dst_new(fp, fp->insn_index, NULL, insns_size);
	if (insns_size)
		memcpy(&dp->code[0], fp->insn_buf, insns_size);
	dst_insn_insert_tail(fp, dp);

	fp->insn_index = 0;
}

static void ErrJump(void)
{ 
	Com_Error(ERR_DROP, "program tried to execute code outside VM\n");
	exit(1);
}

static void jump_insn_append(vm_t *vm, struct func_info * const fp, enum sparc_iname iname, int dest)
{
	struct jump_insn *jp = Z_Malloc(sizeof(*jp));
	struct dst_insn *dp;

	if (dest < 0 || dest >= vm->instructionCount)
		ErrJump();

	dp = dst_new(fp, 2, jp, 0);

	jp->jump_iname = iname;
	jp->jump_dest_insn = dest;
	jp->parent = dp;
	jp->next = NULL;

	jump_insn_insert_tail(fp, jp);
	dst_insn_insert_tail(fp, dp);
}

static void start_emit(struct func_info * const fp, int i_count)
{
	fp->saved_icount = i_count;
	fp->insn_index = 0;
	fp->force_emit = 0;
}

static void __do_emit_one(struct func_info * const fp, unsigned int insn)
{
	fp->insn_buf[fp->insn_index++] = insn;
}

#define in(inst, args...) __do_emit_one(fp,  IN(inst, args))

static void end_emit(struct func_info * const fp)
{
	if (fp->insn_index || fp->force_emit)
		dst_insn_append(fp);
}

static void emit_jump(vm_t *vm, struct func_info * const fp, enum sparc_iname iname, int dest)
{
	end_emit(fp);
	jump_insn_append(vm, fp, iname, dest);
}

static void analyze_function(struct func_info * const fp)
{
	struct src_insn *value_provider[20] = { NULL };
	struct src_insn *sp = fp->first;
	int opstack_depth = 0;

	while ((sp = sp->next) != NULL) {
		unsigned char opi, op = sp->op;

		opi = vm_opInfo[op];
		if (opi & opArgIF) {
			struct src_insn *vp = value_provider[--opstack_depth];
			unsigned char vpopi = vm_opInfo[vp->op];

			if ((opi & opArgI) && (vpopi & opRetI)) {
				/* src1 and dst are integers */
			} else if ((opi & opArgF) && (vpopi & opRetF)) {
				/* src1 and dst are floats */
				vp->dst_reg_flags |= REG_FLAGS_FLOAT;
				sp->src1_reg_flags = REG_FLAGS_FLOAT;
			} else {
				/* illegal combination */
				DIE("unrecognized instruction combination");
			}
		}
		if (opi & opArg2IF) {
			struct src_insn *vp = value_provider[--opstack_depth];
			unsigned char vpopi = vm_opInfo[vp->op];

			if ((opi & opArg2I) && (vpopi & opRetI)) {
				/* src2 and dst are integers */
			} else if ( (opi & opArg2F) && (vpopi & opRetF) ) {
				/* src2 and dst are floats */
				vp->dst_reg_flags |= REG_FLAGS_FLOAT;
				sp->src2_reg_flags = REG_FLAGS_FLOAT;
			} else {
				/* illegal combination */
				DIE("unrecognized instruction combination");
			}
		}
		if (opi & opRetIF) {
			value_provider[opstack_depth] = sp;
			opstack_depth++;
		}
	}
}

static int asmcall(int call, int pstack)
{
	vm_t *savedVM = currentVM;
	int i, ret;

	currentVM->programStack = pstack - 4;
	if (sizeof(intptr_t) == sizeof(int)) {
		intptr_t *argPosition = (intptr_t *)((byte *)currentVM->dataBase + pstack + 4);
		argPosition[0] = -1 - call;
		ret = currentVM->systemCall(argPosition);
	} else {
		intptr_t args[11];

		args[0] = -1 - call;
		int *argPosition = (int *)((byte *)currentVM->dataBase + pstack + 4);
		for( i = 1; i < 11; i++ )
			args[i] = argPosition[i];

		ret = currentVM->systemCall(args);
	}

	currentVM = savedVM;

	return ret;
}

static void blockcopy(unsigned int dest, unsigned int src, unsigned int count)
{
	unsigned int dataMask = currentVM->dataMask;

	if ((dest & dataMask) != dest ||
	    (src & dataMask) != src ||
	    ((dest+count) & dataMask) != dest + count ||
	    ((src+count) & dataMask) != src + count) {
		DIE("OP_BLOCK_COPY out of range!");
	}

	memcpy(currentVM->dataBase+dest, currentVM->dataBase+src, count);
}

static void do_emit_const(struct func_info * const fp, struct src_insn *sp)
{
	start_emit(fp, sp->i_count);
	if (sp->dst_reg_flags & REG_FLAGS_FLOAT) {
		in(LDFI, rVMDATA, sparc_push_data(fp, sp->arg.i), fFIRST(fp));
	} else {
		if ((sp->arg.i & ~0x3ff) == 0) {
			in(ORI, G0, sp->arg.i & 0x3ff, rFIRST(fp));
		} else if ((sp->arg.i & 0x3ff) == 0) {
			in(SETHI, sp->arg.i >> 10, rFIRST(fp));
		} else {
			in(SETHI, sp->arg.i >> 10, rFIRST(fp));
			in(ORI, rFIRST(fp), sp->arg.i & 0x3ff, rFIRST(fp));
		}
	}
	end_emit(fp);
}

#define MAYBE_EMIT_CONST(fp)	\
do {	if ((fp)->cached_const) {	       \
		int saved_i_count = (fp)->saved_icount; \
		do_emit_const(fp, (fp)->cached_const); \
		(fp)->saved_icount = saved_i_count; \
	} \
} while (0)

#define EMIT_FALSE_CONST(fp)					\
do {	int saved_i_count = (fp)->saved_icount;			\
	(fp)->saved_icount = (fp)->cached_const->i_count;	\
	dst_insn_append(fp);					\
	(fp)->saved_icount = saved_i_count;			\
} while (0)

static void compile_one_insn(vm_t *vm, struct func_info * const fp, struct src_insn *sp)
{
	start_emit(fp, sp->i_count);

	switch (sp->op) {
	default:
		Com_Printf("VM: Unhandled opcode 0x%02x[%s]\n",
			   sp->op,
			   opnames[sp->op] ? opnames[sp->op] : "UNKNOWN");
		DIE("Unsupported opcode");
		break;

	case OP_ENTER: {
		int stack = SL(64, 128);

		if (fp->need_float_tmp)
			stack += 16;

		in(SAVEI, O6, -stack, O6);
		if (!SIMM13_P(sp->arg.si)) {
			in(SETHI, sp->arg.i >> 10, rTMP);
			in(ORI, rTMP, sp->arg.i & 0x3ff, rTMP);
			in(SUB, rPSTACK, rTMP, rPSTACK);
		} else
			in(SUBI, rPSTACK, sp->arg.si, rPSTACK);
		break;
	}
	case OP_LEAVE:
		if (fp->cached_const && SIMM13_P(fp->cached_const->arg.si)) {
			EMIT_FALSE_CONST(fp);
			if (fp->cached_const->src1_reg_flags & REG_FLAGS_FLOAT)
				DIE("constant float in OP_LEAVE");

			if (!SIMM13_P(sp->arg.si)) {
				in(SETHI, sp->arg.i >> 10, rTMP);
				in(ORI, rTMP, sp->arg.i & 0x3ff, rTMP);
				in(ADD, rPSTACK, rTMP, rPSTACK);
			} else
				in(ADDI, rPSTACK, sp->arg.si, rPSTACK);
			in(JMPLI, I7, 8, G0);
			in(RESTOREI, G0, fp->cached_const->arg.si, O0);
			POP_GPR(fp);
		} else {
			MAYBE_EMIT_CONST(fp);
			if (!SIMM13_P(sp->arg.si)) {
				in(SETHI, sp->arg.i >> 10, rTMP);
				in(ORI, rTMP, sp->arg.i & 0x3ff, rTMP);
				in(ADD, rPSTACK, rTMP, rPSTACK);
			} else
				in(ADDI, rPSTACK, sp->arg.si, rPSTACK);
			if (sp->src1_reg_flags & REG_FLAGS_FLOAT) {
				in(STFI, O6, SL(64, 128), fFIRST(fp));
				in(LDUWI, O6, SL(64, 128), O0);
				in(JMPLI, I7, 8, G0);
				in(RESTORE, O0, G0, O0);
				POP_FPR(fp);
			} else {
				in(JMPLI, I7, 8, G0);
				in(RESTORE, rFIRST(fp), G0, O0);
				POP_GPR(fp);
			}
		}
		assert(fp->gpr_pos == L0);
		assert(fp->fpr_pos == F0);
		break;
	case OP_JUMP:
		if (fp->cached_const) {
			EMIT_FALSE_CONST(fp);
			emit_jump(vm, fp, BA, fp->cached_const->arg.i);
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SETHI, vm->instructionCount >> 10, rTMP);
			in(ORI, rTMP, vm->instructionCount & 0x3ff, rTMP);
			in(SUBCC, rTMP, rFIRST(fp), G0);
			in(BLEU, +4*5);
			in(LDLI, rVMDATA, VM_Data_Offset(ErrJump), rTMP);

			in(SLLI, rFIRST(fp), 2, rFIRST(fp));
			in(LDLI, rVMDATA, VM_Data_Offset(iPointers), rTMP);
			in(LDL, rTMP, rFIRST(fp), rTMP);
			in(JMPL, rTMP, G0, G0);
			in(NOP);
		}
		POP_GPR(fp);
		break;
	case OP_CALL:
		if (fp->cached_const) {
			EMIT_FALSE_CONST(fp);
			if (fp->cached_const->arg.si >= 0) {
				emit_jump(vm, fp, CALL, fp->cached_const->arg.i);
			} else {
				in(LDLI, rVMDATA, VM_Data_Offset(CallThunk), rTMP);
				in(LDLI, rVMDATA, VM_Data_Offset(AsmCall), O3);
				in(ORI, G0, fp->cached_const->arg.si, O0);
				in(JMPL, rTMP, G0, O7);
				in(OR, G0, rPSTACK, O1);
			}
			in(OR, G0, O0, rFIRST(fp));
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SUBCCI, rFIRST(fp), 0, G0);
			in(BL, +4*7);
			in(NOP);

			/* normal call */
			in(SETHI, vm->instructionCount >> 10, rTMP);
			in(ORI, rTMP, vm->instructionCount & 0x3ff, rTMP);
			in(SUBCC, rTMP, rFIRST(fp), G0);
			in(BLEU, +4*9);
			in(LDLI, rVMDATA, VM_Data_Offset(ErrJump), rTMP);
			in(LDLI, rVMDATA, VM_Data_Offset(iPointers), O5);
			in(SLLI, rFIRST(fp), 2, rFIRST(fp));
			in(LDL, O5, rFIRST(fp), rTMP);
			in(BA, +4*4);
			in(NOP);

			/* syscall */
			in(LDLI, rVMDATA, VM_Data_Offset(CallThunk), rTMP);
			in(LDLI, rVMDATA, VM_Data_Offset(AsmCall), O3);

			in(OR, G0, rFIRST(fp), O0);
			in(JMPL, rTMP, G0, O7);
			in(OR, G0, rPSTACK, O1);

			/* return value */
			in(OR, G0, O0, rFIRST(fp));
		}
		break;
	case OP_BLOCK_COPY:
		MAYBE_EMIT_CONST(fp);
		in(LDLI, rVMDATA, VM_Data_Offset(CallThunk), rTMP);
		in(LDLI, rVMDATA, VM_Data_Offset(BlockCopy), O3);
		in(OR, G0, rSECOND(fp), O0);
		in(OR, G0, rFIRST(fp), O1);
		if ((sp->arg.i & ~0x3ff) == 0) {
			in(ORI, G0, sp->arg.i & 0x3ff, O2);
		} else if ((sp->arg.i & 0x3ff) == 0) {
			in(SETHI, sp->arg.i >> 10, O2);
		} else {
			in(SETHI, sp->arg.i >> 10, O2);
			in(ORI, O2, sp->arg.i & 0x3ff, O2);
		}
		in(JMPL, rTMP, G0, O7);
		in(NOP);
		POP_GPR(fp);
		POP_GPR(fp);
		break;

	case OP_PUSH:
		MAYBE_EMIT_CONST(fp);
		if (sp->dst_reg_flags & REG_FLAGS_FLOAT)
			PUSH_FPR(fp);
		else
			PUSH_GPR(fp);
		fp->force_emit = 1;
		break;
	case OP_POP:
		MAYBE_EMIT_CONST(fp);
		if (sp->src1_reg_flags & REG_FLAGS_FLOAT)
			POP_FPR(fp);
		else
			POP_GPR(fp);
		fp->force_emit = 1;
		break;
	case OP_ARG:
		MAYBE_EMIT_CONST(fp);
		in(ADDI, rPSTACK, sp->arg.b, rTMP);
		if (sp->src1_reg_flags & REG_FLAGS_FLOAT) {
			in(STF, rDATABASE, rTMP, fFIRST(fp));
			POP_FPR(fp);
		} else {
			in(STW, rDATABASE, rTMP, rFIRST(fp));
			POP_GPR(fp);
		}
		break;
	case OP_IGNORE:
		MAYBE_EMIT_CONST(fp);
		in(NOP);
		break;
	case OP_BREAK:
		MAYBE_EMIT_CONST(fp);
		in(TA, 0x5);
		break;
	case OP_LOCAL:
		MAYBE_EMIT_CONST(fp);
		PUSH_GPR(fp);
		if (!SIMM13_P(sp->arg.i)) {
			in(SETHI, sp->arg.i >> 10, rTMP);
			in(ORI, rTMP, sp->arg.i & 0x3ff, rTMP);
			in(ADD, rPSTACK, rTMP, rFIRST(fp));
		} else
			in(ADDI, rPSTACK, sp->arg.i, rFIRST(fp));
		break;
	case OP_CONST:
		MAYBE_EMIT_CONST(fp);
		break;
	case OP_LOAD4:
		MAYBE_EMIT_CONST(fp);
		in(AND, rFIRST(fp), rDATAMASK, rFIRST(fp));
		if (sp->dst_reg_flags & REG_FLAGS_FLOAT) {
			PUSH_FPR(fp);
			in(LDF, rFIRST(fp), rDATABASE, fFIRST(fp));
			POP_GPR(fp);
		} else {
			in(LDUW, rFIRST(fp), rDATABASE, rFIRST(fp));
		}
		break;
	case OP_LOAD2:
		MAYBE_EMIT_CONST(fp);
		in(AND, rFIRST(fp), rDATAMASK, rFIRST(fp));
		in(LDUH, rFIRST(fp), rDATABASE, rFIRST(fp));
		break;
	case OP_LOAD1:
		MAYBE_EMIT_CONST(fp);
		in(AND, rFIRST(fp), rDATAMASK, rFIRST(fp));
		in(LDUB, rFIRST(fp), rDATABASE, rFIRST(fp));
		break;
	case OP_STORE4:
		MAYBE_EMIT_CONST(fp);
		if (sp->src1_reg_flags & REG_FLAGS_FLOAT) {
			in(AND, rFIRST(fp), rDATAMASK, rFIRST(fp));
			in(STF, rFIRST(fp), rDATABASE, fFIRST(fp));
			POP_FPR(fp);
		} else {
			in(AND, rSECOND(fp), rDATAMASK, rSECOND(fp));
			in(STW, rSECOND(fp), rDATABASE, rFIRST(fp));
			POP_GPR(fp);
		}
		POP_GPR(fp);
		break;
	case OP_STORE2:
		MAYBE_EMIT_CONST(fp);
		in(AND, rSECOND(fp), rDATAMASK, rSECOND(fp));
		in(STH, rSECOND(fp), rDATABASE, rFIRST(fp));
		POP_GPR(fp);
		POP_GPR(fp);
		break;
	case OP_STORE1:
		MAYBE_EMIT_CONST(fp);
		in(AND, rSECOND(fp), rDATAMASK, rSECOND(fp));
		in(STB, rSECOND(fp), rDATABASE, rFIRST(fp));
		POP_GPR(fp);
		POP_GPR(fp);
		break;
	case OP_EQ:
	case OP_NE:
	case OP_LTI:
	case OP_GEI:
	case OP_GTI:
	case OP_LEI:
	case OP_LTU:
	case OP_GEU:
	case OP_GTU:
	case OP_LEU: {
		enum sparc_iname iname = BA;

		if (fp->cached_const && SIMM13_P(fp->cached_const->arg.si)) {
			EMIT_FALSE_CONST(fp);
			in(SUBCCI, rSECOND(fp), fp->cached_const->arg.si, G0);
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SUBCC, rSECOND(fp), rFIRST(fp), G0);
		}
		switch(sp->op) {
		case OP_EQ: iname = BE; break;
		case OP_NE: iname = BNE; break;
		case OP_LTI: iname = BL; break;
		case OP_GEI: iname = BGE; break;
		case OP_GTI: iname = BG; break;
		case OP_LEI: iname = BLE; break;
		case OP_LTU: iname = BCS; break;
		case OP_GEU: iname = BCC; break;
		case OP_GTU: iname = BGU; break;
		case OP_LEU: iname = BLEU; break;
		}
		emit_jump(vm, fp, iname, sp->arg.i);
		POP_GPR(fp);
		POP_GPR(fp);
		break;
	}

	case OP_SEX8:
		MAYBE_EMIT_CONST(fp);
		in(SLLI, rFIRST(fp), 24, rFIRST(fp));
		in(SRAI, rFIRST(fp), 24, rFIRST(fp));
		break;
	case OP_SEX16:
		MAYBE_EMIT_CONST(fp);
		in(SLLI, rFIRST(fp), 16, rFIRST(fp));
		in(SRAI, rFIRST(fp), 16, rFIRST(fp));
		break;
	case OP_NEGI:
		MAYBE_EMIT_CONST(fp);
		in(SUB, G0, rFIRST(fp), rFIRST(fp));
		break;
	case OP_ADD:
		if (fp->cached_const && SIMM13_P(fp->cached_const->arg.si)) {
			EMIT_FALSE_CONST(fp);
			in(ADDI, rSECOND(fp), fp->cached_const->arg.si, rSECOND(fp));
		} else {
			MAYBE_EMIT_CONST(fp);
			in(ADD, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		}
		POP_GPR(fp);
		break;
	case OP_SUB:
		if (fp->cached_const && SIMM13_P(fp->cached_const->arg.si)) {
			EMIT_FALSE_CONST(fp);
			in(SUBI, rSECOND(fp), fp->cached_const->arg.si, rSECOND(fp));
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SUB, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		}
		POP_GPR(fp);
		break;
	case OP_DIVI:
		MAYBE_EMIT_CONST(fp);
		in(SRAI, rSECOND(fp), 31, rTMP);
		in(WRI, rTMP, 0, Y_REG);
		in(SDIV, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_DIVU:
		MAYBE_EMIT_CONST(fp);
		in(WRI, G0, 0, Y_REG);
		in(UDIV, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_MODI:
		MAYBE_EMIT_CONST(fp);
		in(SRAI, rSECOND(fp), 31, rTMP);
		in(WRI, rTMP, 0, Y_REG);
		in(SDIV, rSECOND(fp), rFIRST(fp), rTMP);
		in(SMUL, rTMP, rFIRST(fp), rTMP);
		in(SUB, rSECOND(fp), rTMP, rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_MODU:
		MAYBE_EMIT_CONST(fp);
		in(WRI, G0, 0, Y_REG);
		in(UDIV, rSECOND(fp), rFIRST(fp), rTMP);
		in(SMUL, rTMP, rFIRST(fp), rTMP);
		in(SUB, rSECOND(fp), rTMP, rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_MULI:
		MAYBE_EMIT_CONST(fp);
		in(SMUL, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_MULU:
		MAYBE_EMIT_CONST(fp);
		in(UMUL, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_BAND:
		MAYBE_EMIT_CONST(fp);
		in(AND, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_BOR:
		MAYBE_EMIT_CONST(fp);
		in(OR, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_BXOR:
		MAYBE_EMIT_CONST(fp);
		in(XOR, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		POP_GPR(fp);
		break;
	case OP_BCOM:
		MAYBE_EMIT_CONST(fp);
		in(XNOR, rFIRST(fp), G0, rFIRST(fp));
		break;
	case OP_LSH:
		if (fp->cached_const) {
			EMIT_FALSE_CONST(fp);
			in(SLLI, rSECOND(fp), fp->cached_const->arg.si, rSECOND(fp));
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SLL, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		}
		POP_GPR(fp);
		break;
	case OP_RSHI:
		if (fp->cached_const) {
			EMIT_FALSE_CONST(fp);
			in(SRAI, rSECOND(fp), fp->cached_const->arg.si, rSECOND(fp));
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SRA, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		}
		POP_GPR(fp);
		break;
	case OP_RSHU:
		if (fp->cached_const) {
			EMIT_FALSE_CONST(fp);
			in(SRLI, rSECOND(fp), fp->cached_const->arg.si, rSECOND(fp));
		} else {
			MAYBE_EMIT_CONST(fp);
			in(SRL, rSECOND(fp), rFIRST(fp), rSECOND(fp));
		}
		POP_GPR(fp);
		break;

	case OP_NEGF:
		MAYBE_EMIT_CONST(fp);
		in(FNEG, fFIRST(fp), fFIRST(fp));
		break;
	case OP_ADDF:
		MAYBE_EMIT_CONST(fp);
		in(FADD, fSECOND(fp), fFIRST(fp), fSECOND(fp));
		POP_FPR(fp);
		break;
	case OP_SUBF:
		MAYBE_EMIT_CONST(fp);
		in(FSUB, fSECOND(fp), fFIRST(fp), fSECOND(fp));
		POP_FPR(fp);
		break;
	case OP_DIVF:
		MAYBE_EMIT_CONST(fp);
		in(FDIV, fSECOND(fp), fFIRST(fp), fSECOND(fp));
		POP_FPR(fp);
		break;
	case OP_MULF:
		MAYBE_EMIT_CONST(fp);
		in(FMUL, fSECOND(fp), fFIRST(fp), fSECOND(fp));
		POP_FPR(fp);
		break;

	case OP_EQF:
	case OP_NEF:
	case OP_LTF:
	case OP_GEF:
	case OP_GTF:
	case OP_LEF: {
		enum sparc_iname iname = FBE;

		MAYBE_EMIT_CONST(fp);
		in(FCMP, fSECOND(fp), fFIRST(fp));
		switch(sp->op) {
		case OP_EQF: iname = FBE; break;
		case OP_NEF: iname = FBNE; break;
		case OP_LTF: iname = FBL; break;
		case OP_GEF: iname = FBGE; break;
		case OP_GTF: iname = FBG; break;
		case OP_LEF: iname = FBLE; break;
		}
		emit_jump(vm, fp, iname, sp->arg.i);
		POP_FPR(fp);
		POP_FPR(fp);
		break;
	}
	case OP_CVIF:
		MAYBE_EMIT_CONST(fp);
		PUSH_FPR(fp);
		in(STWI, O6, SL(64, 128), rFIRST(fp));
		in(LDFI, O6, SL(64, 128), fFIRST(fp));
		in(FITOS, fFIRST(fp), fFIRST(fp));
		POP_GPR(fp);
		break;
	case OP_CVFI:
		MAYBE_EMIT_CONST(fp);
		PUSH_GPR(fp);
		in(FSTOI, fFIRST(fp), fFIRST(fp));
		in(STFI, O6, SL(64, 128), fFIRST(fp));
		in(LDUWI, O6, SL(64, 128), rFIRST(fp));
		POP_FPR(fp);
		break;
	}
	if (sp->op != OP_CONST) {
		fp->cached_const = NULL;
		end_emit(fp);
	} else {
		fp->cached_const = sp;
		if (sp->dst_reg_flags & REG_FLAGS_FLOAT) {
			PUSH_FPR(fp);
		} else {
			PUSH_GPR(fp);
		}
	}
	end_emit(fp);
}

static void free_source_insns(struct func_info * const fp)
{
	struct src_insn *sp = fp->first->next;

	while (sp) {
		struct src_insn *next = sp->next;
		Z_Free(sp);
		sp = next;
	}
}

static void compile_function(vm_t *vm, struct func_info * const fp)
{
	struct src_insn *sp;

	analyze_function(fp);

	fp->gpr_pos = L0;
	fp->fpr_pos = F0;
	fp->insn_index = 0;

	fp->stack_space = SL(64, 128);
	fp->cached_const = NULL;

	sp = fp->first;
	while ((sp = sp->next) != NULL)
		compile_one_insn(vm, fp, sp);

	free_source_insns(fp);
}

/* We have two thunks for sparc.  The first is for the entry into
 * the VM, where setup the fixed global registers.  The second is
 * for calling out to C code from the VM, where we need to preserve
 * those fixed globals across the call.
 */
static void emit_vm_thunk(struct func_info * const fp)
{
	/* int vm_thunk(void *vmdata, int programstack, void *database, int datamask) */
	start_emit(fp, THUNK_ICOUNT);

	in(OR, G0, O0, rVMDATA);
	in(OR, G0, O1, rPSTACK);
	in(OR, G0, O2, rDATABASE);
	in(BA, +4*17);
	in(OR, G0, O3, rDATAMASK);

	/* int call_thunk(int arg0, int arg1, int arg2, int (*func)(int int int)) */
#define CALL_THUNK_INSN_OFFSET		5
	in(SAVEI, O6, -SL(64, 128), O6);

	in(OR, G0, rVMDATA, L0);
	in(OR, G0, rPSTACK, L1);
	in(OR, G0, rDATABASE, L2);
	in(OR, G0, rDATAMASK, L3);

	in(OR, G0, I0, O0);
	in(OR, G0, I1, O1);
	in(JMPL, I3, G0, O7);
	in(OR, G0, I2, O2);

	in(OR, G0, L0, rVMDATA);
	in(OR, G0, L1, rPSTACK);
	in(OR, G0, L2, rDATABASE);
	in(OR, G0, L3, rDATAMASK);

	in(JMPLI, I7, 8, G0);
	in(RESTORE, O0, G0, O0);

	end_emit(fp);
}

static void sparc_compute_code(vm_t *vm, struct func_info * const fp)
{
	struct dst_insn *dp = fp->dst_first;
	unsigned int *code_now, *code_begin;
	unsigned char *data_and_code;
	unsigned int code_length;
	int code_insns = 0, off;
	struct data_hunk *dhp;
	struct jump_insn *jp;
	vm_data_t *data;

	while (dp) {
		code_insns += dp->length;
		dp = dp->next;
	}

	code_length = (sizeof(vm_data_t) +
		       (fp->data_num * sizeof(unsigned int)) +
		       (code_insns * sizeof(unsigned int)));

	data_and_code = mmap(NULL, code_length, PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (data_and_code == MAP_FAILED)
		DIE("Not enough memory");

	code_now = code_begin = (unsigned int *)
		(data_and_code + VM_Data_Offset(data[fp->data_num]));

	dp = fp->dst_first;
	while (dp) {
		int i_count = dp->i_count;

		if (i_count != THUNK_ICOUNT) {
			if (!fp->dst_by_i_count[i_count])
				fp->dst_by_i_count[i_count] = (void *) code_now;
		}
		if (!dp->jump) {
			memcpy(code_now, &dp->code[0], dp->length * sizeof(unsigned int));
			code_now += dp->length;
		} else {
			int i;

			dp->jump->parent = (void *) code_now;

			for (i = 0; i < dp->length; i++)
				code_now[i] = SPARC_NOP;
			code_now += dp->length;
		}

		dp = dp->next;
	}

	jp = fp->jump_first;
	while (jp) {
		unsigned int *from = (void *) jp->parent;
		unsigned int *to = (void *) fp->dst_by_i_count[jp->jump_dest_insn];
		signed int disp = (to - from);

		*from = IN(jp->jump_iname, disp << 2);

		jp = jp->next;
	}

	vm->codeBase = data_and_code;
	vm->codeLength = code_length;

	data = (vm_data_t *) data_and_code;
	data->CallThunk = code_begin + CALL_THUNK_INSN_OFFSET;
	data->AsmCall = asmcall;
	data->BlockCopy = blockcopy;
	data->iPointers = (unsigned int *) vm->instructionPointers;
	data->dataLength = VM_Data_Offset(data[fp->data_num]);
	data->codeLength = (code_now - code_begin) * sizeof(unsigned int);
	data->ErrJump = ErrJump;

#if 0
	{
		unsigned int *insn = code_begin;
		int i;

		Com_Printf("INSN DUMP\n");
		for (i = 0; i < data->codeLength / 4; i+= 8) {
			Com_Printf("\t.word\t0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
				   insn[i + 0], insn[i + 1],
				   insn[i + 2], insn[i + 3],
				   insn[i + 4], insn[i + 5],
				   insn[i + 6], insn[i + 7]);
		}
	}
#endif

	dhp = fp->data_first;
	off = 0;
	while (dhp) {
		struct data_hunk *next = dhp->next;
		int i;

		for (i = 0; i < dhp->count; i++)
			data->data[off + i] = dhp->data[i];

		off += dhp->count;

		Z_Free(dhp);

		dhp = next;
	}
	fp->data_first = NULL;
	fp->data_num = 0;

	dp = fp->dst_first;
	while (dp) {
		struct dst_insn *next = dp->next;
		if (dp->jump)
			Z_Free(dp->jump);
		Z_Free(dp);
		dp = next;
	}
	fp->dst_first = fp->dst_last = NULL;
}

void VM_Compile(vm_t *vm, vmHeader_t *header)
{
	struct func_info fi;
	unsigned char *code;
	int i_count, pc, i;

	memset(&fi, 0, sizeof(fi));

	fi.first = Z_Malloc(sizeof(struct src_insn));
	fi.first->next = NULL;

#ifdef __arch64__
	Z_Free(vm->instructionPointers);
	vm->instructionPointers = Z_Malloc(header->instructionCount *
					   sizeof(void *));
#endif

	fi.dst_by_i_count = (struct dst_insn **) vm->instructionPointers;
	memset(fi.dst_by_i_count, 0, header->instructionCount * sizeof(void *));

	vm->compiled = qfalse;

	emit_vm_thunk(&fi);

	code = (unsigned char *) header + header->codeOffset;
	pc = 0;

	for (i_count = 0; i_count < header->instructionCount; i_count++) {
		unsigned char opi, op = code[pc++];
		struct src_insn *sp;

		if (op == OP_CALL || op == OP_BLOCK_COPY)
			fi.has_call = 1;
		opi = vm_opInfo[op];
		if (op == OP_CVIF || op == OP_CVFI ||
		    (op == OP_LEAVE && (opi & opArgF)))
			fi.need_float_tmp = 1;

		if (op == OP_ENTER) {
			if (fi.first->next)
				compile_function(vm, &fi);
			fi.first->next = NULL;
			fi.last = fi.first;
			fi.has_call = fi.need_float_tmp = 0;
		}

		sp = Z_Malloc(sizeof(*sp));
		sp->op = op;
		sp->i_count = i_count;
		sp->arg.i = 0;
		sp->next = NULL;

		if (vm_opInfo[op] & opImm4) {
			union {
				unsigned char b[4];
				unsigned int i;
			} c = { { code[ pc + 3 ], code[ pc + 2 ],
				  code[ pc + 1 ], code[ pc + 0 ] }, };

			sp->arg.i = c.i;
			pc += 4;
		} else if (vm_opInfo[op] & opImm1) {
			sp->arg.b = code[pc++];
		}

		fi.last->next = sp;
		fi.last = sp;
	}
	compile_function(vm, &fi);

	Z_Free(fi.first);

	memset(fi.dst_by_i_count, 0, header->instructionCount * sizeof(void *));
	sparc_compute_code(vm, &fi);

	for (i = 0; i < header->instructionCount; i++) {
		if (!fi.dst_by_i_count[i]) {
			Com_Printf(S_COLOR_RED "Pointer %d not initialized !\n", i);
			DIE("sparc JIT bug");
		}
	}

	if (mprotect(vm->codeBase, vm->codeLength, PROT_READ|PROT_EXEC)) {
		VM_Destroy_Compiled(vm);
		DIE("mprotect failed");
	}

	vm->destroy = VM_Destroy_Compiled;
	vm->compiled = qtrue;
}

int VM_CallCompiled(vm_t *vm, int *args)
{
	vm_data_t *vm_dataAndCode = (void *) vm->codeBase;
	int programStack = vm->programStack;
	int stackOnEntry = programStack;
	byte *image = vm->dataBase;
	int *argPointer;
	int retVal;

	currentVM = vm;

	vm->currentlyInterpreting = qtrue;

	programStack -= 48;
	argPointer = (int *)&image[ programStack + 8 ];
	memcpy( argPointer, args, 4 * 9 );
	argPointer[-1] = 0;
	argPointer[-2] = -1;

	/* call generated code */
	{
		int (*entry)(void *, int, void *, int);
		entry = (void *)(vm->codeBase + vm_dataAndCode->dataLength);
		retVal = entry(vm->codeBase, programStack, vm->dataBase, vm->dataMask);
	}

	vm->programStack = stackOnEntry;
	vm->currentlyInterpreting = qfalse;

	return retVal;
}
