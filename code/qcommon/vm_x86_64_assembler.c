/*
===========================================================================
vm_x86_64_assembler.c -- assembler for x86-64

Copyright (C) 2007 Ludwig Nussel <ludwig.nussel@suse.de>, Novell inc.

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

#define _ISOC99_SOURCE

#include "vm_local.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <inttypes.h>

// Ignore __attribute__ on non-gcc platforms
#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

static char* out;
static unsigned compiledOfs;
static unsigned assembler_pass;

static const char* cur_line;

static FILE* fout;

#define crap(fmt, args...) do { \
	_crap(__FUNCTION__, fmt, ##args); \
} while(0)

#define CRAP_INVALID_ARGS crap("invalid arguments %s, %s", argtype2str(arg1.type),argtype2str(arg2.type));

#ifdef DEBUG
#define debug(fmt, args...) printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif

static __attribute__ ((noreturn)) __attribute__ ((format (printf, 2, 3))) void _crap(const char* func, const char* fmt, ...)
{
	va_list ap;
	fprintf(stderr, "%s() - ", func);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	if(cur_line && cur_line[0])
		fprintf(stderr, "-> %s\n", cur_line);
	exit(1);
}

static void emit1(unsigned char v)
{
	int writecnt;
	
	if(assembler_pass)
	{
		out[compiledOfs++] = v;

		if(fout)
			writecnt = fwrite(&v, 1, 1, fout);
			
		debug("%02hx ", v);
	}
	else
	{
		++compiledOfs;
	}
}

static inline void emit2(u16 v)
{
	emit1(v&0xFF);
	emit1((v>>8)&0xFF);
}

static inline void emit4(u32 v)
{
	emit1(v&0xFF);
	emit1((v>>8)&0xFF);
	emit1((v>>16)&0xFF);
	emit1((v>>24)&0xFF);
}

static inline void emit8(u64 v)
{
	emit4(v&0xFFFFFFFF);
	emit4((v>>32)&0xFFFFFFFF);
}

enum {
	REX_W = 0x08,
	REX_R = 0x04,
	REX_X = 0x02,
	REX_B = 0x01,
};

enum {
	MODRM_MOD_00 = 0x00,
	MODRM_MOD_01 = 0x01 << 6,
	MODRM_MOD_10 = 0x02 << 6,
	MODRM_MOD_11 = 0x03 << 6,
	MODRM_RM_SIB = 0x04,
};

typedef enum
{
	T_NONE      = 0x00,
	T_REGISTER  = 0x01,
	T_IMMEDIATE = 0x02,
	T_MEMORY    = 0x04,
	T_LABEL     = 0x08,
	T_ABSOLUTE  = 0x80
} argtype_t;

typedef enum {
	R_8   = 0x100, 
	R_16  = 0x200, 
	R_64  = 0x800, 
	R_MSZ = 0xF00,  // size mask
	R_XMM = 0x2000, // xmm register. year, sucks
	R_EAX =  0x00,
	R_EBX =  0x03,
	R_ECX =  0x01,
	R_EDX =  0x02,
	R_ESI =  0x06,
	R_EDI =  0x07,
	R_ESP =  0x04,
	R_RAX =  R_EAX | R_64,
	R_RBX =  R_EBX | R_64,
	R_RCX =  R_ECX | R_64,
	R_RDX =  R_EDX | R_64,
	R_RSI =  R_ESI | R_64,
	R_RDI =  R_EDI | R_64,
	R_RSP =  R_ESP | R_64,
	R_R8  =  0x08  | R_64,
	R_R9  =  0x09  | R_64,
	R_R10 =  0x0A  | R_64,
	R_R15 =  0x0F  | R_64,
	R_AL  =  R_EAX | R_8,
	R_AX  =  R_EAX | R_16,
	R_BL  =  R_EBX | R_8,
	R_CL  =  R_ECX | R_8,
	R_XMM0 = 0x00  | R_XMM,
	R_MGP =  0x0F, // mask for general purpose registers
} reg_t;

typedef enum {
	MODRM_SIB = 0,
	MODRM_NOSIB = 0x3,
} modrm_sib_t;

typedef struct {
	unsigned disp;
	argtype_t basetype;
	union {
		u64 imm;
		reg_t reg;
	} base;
	argtype_t indextype;
	union {
		u64 imm;
		reg_t reg;
	} index;
	unsigned scale;
} memref_t;

#define LABELLEN 32

typedef struct {
	argtype_t type;
	union {
		u64 imm;
		reg_t reg;
		memref_t mem;
		char label[LABELLEN];
	} v;
	int absolute:1;
} arg_t;

typedef void (*emitfunc)(const char* op, arg_t arg1, arg_t arg2, void* data);

typedef struct {
	char* mnemonic;
	emitfunc func;
	void* data;
} op_t;

typedef struct {
	u8 xmmprefix;
	u8 subcode; // in modrm
	u8 rmcode;  // opcode for reg/mem, reg
	u8 mrcode;  // opcode for reg, reg/mem
	u8 rcode8;  // opcode for reg8/mem8
	u8 rcode;  // opcode for reg/mem
} opparam_t;

static opparam_t params_add = { subcode: 0, rmcode: 0x01, };
static opparam_t params_or = { subcode: 1, rmcode: 0x09, };
static opparam_t params_and = { subcode: 4, rmcode: 0x21, };
static opparam_t params_sub = { subcode: 5, rmcode: 0x29, };
static opparam_t params_xor = { subcode: 6, rmcode: 0x31, };
static opparam_t params_cmp = { subcode: 7, rmcode: 0x39, mrcode: 0x3b, };
static opparam_t params_dec = { subcode: 1, rcode: 0xff, rcode8: 0xfe, };
static opparam_t params_sar = { subcode: 7, rcode: 0xd3, rcode8: 0xd2, };
static opparam_t params_shl = { subcode: 4, rcode: 0xd3, rcode8: 0xd2, };
static opparam_t params_shr = { subcode: 5, rcode: 0xd3, rcode8: 0xd2, };
static opparam_t params_idiv = { subcode: 7, rcode: 0xf7, rcode8: 0xf6, };
static opparam_t params_div = { subcode: 6, rcode: 0xf7, rcode8: 0xf6, };
static opparam_t params_imul = { subcode: 5, rcode: 0xf7, rcode8: 0xf6, };
static opparam_t params_mul = { subcode: 4, rcode: 0xf7, rcode8: 0xf6, };
static opparam_t params_neg = { subcode: 3, rcode: 0xf7, rcode8: 0xf6, };
static opparam_t params_not = { subcode: 2, rcode: 0xf7, rcode8: 0xf6, };

static opparam_t params_cvtsi2ss = { xmmprefix: 0xf3, rmcode: 0x2a };
static opparam_t params_cvttss2si = { xmmprefix: 0xf3, rmcode: 0x2c };
static opparam_t params_addss = { xmmprefix: 0xf3, mrcode: 0x58 };
static opparam_t params_divss = { xmmprefix: 0xf3, mrcode: 0x5e };
static opparam_t params_movss = { xmmprefix: 0xf3, mrcode: 0x10, rmcode: 0x11 };
static opparam_t params_mulss = { xmmprefix: 0xf3, mrcode: 0x59 };
static opparam_t params_subss = { xmmprefix: 0xf3, mrcode: 0x5c };
static opparam_t params_ucomiss = { mrcode: 0x2e };

/* ************************* */

static unsigned hashkey(const char *string, unsigned len) {
	unsigned register hash, i;

	hash = 0;
	for (i = 0; i < len && string[i] != '\0'; ++i) {
		hash += string[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	return hash;
}

struct hashentry {
	char* label;
	unsigned address;
	struct hashentry* next;
};
static struct hashentry* labelhash[1021];

// no dup check!
static void hash_add_label(const char* label, unsigned address)
{
	struct hashentry* h;
	unsigned i = hashkey(label, -1U);
	int labellen;
	
	i %= ARRAY_LEN(labelhash);
	h = Z_Malloc(sizeof(struct hashentry));
	
	labellen = strlen(label) + 1;
	h->label = Z_Malloc(labellen);
	memcpy(h->label, label, labellen);
	
	h->address = address;
	h->next = labelhash[i];
	labelhash[i] = h;
}

static unsigned lookup_label(const char* label)
{
	struct hashentry* h;
	unsigned i = hashkey(label, -1U);
	i %= ARRAY_LEN(labelhash);
	for(h = labelhash[i]; h; h = h->next )
	{
		if(!strcmp(h->label, label))
			return h->address;
	}
	if(assembler_pass)
		crap("label %s undefined", label);
	return 0;
}

static void labelhash_free(void)
{
	struct hashentry* h;
	unsigned i;
	unsigned z = 0, min = -1U, max = 0, t = 0;
	for ( i = 0; i < ARRAY_LEN(labelhash); ++i)
	{
		unsigned n = 0;
		h = labelhash[i];
		while(h)
		{
			struct hashentry* next = h->next;
			Z_Free(h->label);
			Z_Free(h);
			h = next;
			++n;
		}
		t+=n;
		if(!n) ++z;
		//else printf("%u\n", n);
		min = MIN(min, n);
		max = MAX(max, n);
	}
	printf("total %u, hsize %"PRIu64", zero %u, min %u, max %u\n", t, ARRAY_LEN(labelhash), z, min, max);
	memset(labelhash, 0, sizeof(labelhash));
}

/* ************************* */


static const char* argtype2str(argtype_t t)
{
	switch(t)
	{
		case T_NONE: return "none";
		case T_REGISTER: return "register";
		case T_IMMEDIATE: return "immediate";
		case T_MEMORY: return "memory";
		case T_LABEL: return "label";
		default: crap("invalid type");
	}
	/* not reached */
	return T_NONE;
}

/* ************************* */

static inline int iss8(int64_t v)
{
	return (SCHAR_MIN <= v && v <= SCHAR_MAX);
}

static inline int isu8(u64 v)
{
	return (v <= UCHAR_MAX);
}

static inline int iss16(int64_t v)
{
	return (SHRT_MIN <= v && v <= SHRT_MAX);
}

static inline int isu16(u64 v)
{
	return (v <= USHRT_MAX);
}

static inline int iss32(int64_t v)
{
	return (INT_MIN <= v && v <= INT_MAX);
}

static inline int isu32(u64 v)
{
	return (v <= UINT_MAX);
}

static void emit_opsingle(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 op = (u8)((uint64_t) data);

	if(arg1.type != T_NONE || arg2.type != T_NONE)
		CRAP_INVALID_ARGS;

	emit1(op);
}

static void emit_opsingle16(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	emit1(0x66);
	emit_opsingle(mnemonic, arg1, arg2, data);
}

static void compute_rexmodrmsib(u8* rex_r, u8* modrm_r, u8* sib_r, arg_t* arg1, arg_t* arg2)
{
	u8 rex = 0;
	u8 modrm = 0;
	u8 sib = 0;

	if((arg1->type == T_REGISTER && arg2->type == T_REGISTER)
	&& ((arg1->v.reg & R_MSZ) != (arg2->v.reg & R_MSZ))
	&& !((arg1->v.reg & R_XMM) || (arg2->v.reg & R_XMM)))
		crap("both registers must be of same width");

	if((arg1->type == T_REGISTER && arg1->v.reg & R_64)
	|| (arg2->type == T_REGISTER && arg2->v.reg & R_64))
	{
		rex |= REX_W;
	}

	if(arg1->type == T_REGISTER)
	{
		if((arg1->v.reg & R_MGP) > 0x07)
			rex |= REX_R;

		modrm |= (arg1->v.reg & 0x07) << 3;
	}

	if(arg2->type == T_REGISTER)
	{
		if((arg2->v.reg & R_MGP) > 0x07)
			rex |= REX_B;

		modrm |= (arg2->v.reg & 0x07);
	}

	if(arg2->type == T_MEMORY)
	{
		if((arg2->v.mem.basetype == T_REGISTER && !(arg2->v.mem.base.reg & R_64))
		|| (arg2->v.mem.indextype == T_REGISTER && !(arg2->v.mem.index.reg & R_64)))
		{
			crap("only 64bit base/index registers are %x %x", arg2->v.mem.base.reg, arg2->v.mem.index.reg);
		}

		if(arg2->v.mem.indextype == T_REGISTER)
		{
			modrm |= MODRM_RM_SIB;
			if(!arg2->v.mem.disp)
			{
				modrm |= MODRM_MOD_00;
			}
			else if(iss8(arg2->v.mem.disp))
			{
				modrm |= MODRM_MOD_01;
			}
			else if(isu32(arg2->v.mem.disp))
			{
				modrm |= MODRM_MOD_10;
			}
			else
			{
				crap("invalid displacement");
			}

			if((arg2->v.mem.index.reg & R_MGP) > 0x07)
				rex |= REX_X;

			if((arg2->v.mem.base.reg & R_MGP) > 0x07)
				rex |= REX_B;

			if(arg2->v.mem.basetype != T_REGISTER)
				crap("base must be register");
			switch(arg2->v.mem.scale)
			{
				case 1: break;
				case 2: sib |= 1 << 6; break;
				case 4: sib |= 2 << 6; break;
				case 8: sib |= 3 << 6; break;
			}
			sib |= (arg2->v.mem.index.reg & 0x07) << 3;
			sib |= (arg2->v.mem.base.reg & 0x07);
		}
		else if(arg2->v.mem.indextype == T_NONE)
		{
			if(!arg2->v.mem.disp)
			{
				modrm |= MODRM_MOD_00;
			}
			else if(iss8(arg2->v.mem.disp))
			{
				modrm |= MODRM_MOD_01;
			}
			else if(isu32(arg2->v.mem.disp))
			{
				modrm |= MODRM_MOD_10;
			}
			else
			{
				crap("invalid displacement");
			}

			if(arg2->v.mem.basetype != T_REGISTER)
				crap("todo: base != register");

			if((arg2->v.mem.base.reg & R_MGP) > 0x07)
				rex |= REX_B;

			modrm |= arg2->v.mem.base.reg & 0x07;
		}
		else
		{
			crap("invalid indextype");
		}
	}
	else
	{
		modrm |= MODRM_MOD_11;
	}

	if(rex)
		rex |= 0x40; // XXX

	*rex_r = rex;
	*modrm_r = modrm;
	*sib_r = sib;
}

static void maybe_emit_displacement(arg_t* arg)
{
	if(arg->type != T_MEMORY)
		return;

	if(arg->v.mem.disp)
	{
		if(iss8(arg->v.mem.disp))
		{
			emit1((u8)arg->v.mem.disp);
		}
		else if(isu32(arg->v.mem.disp))
		{
			emit4(arg->v.mem.disp);
		}
		else
		{
			crap("invalid displacement");
		}
	}
}

/* one byte operator with register added to operator */
static void emit_opreg(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 op = (u8)((uint64_t) data);

	if(arg1.type != T_REGISTER || arg2.type != T_NONE)
		CRAP_INVALID_ARGS;

	if((arg1.v.reg & R_MGP) > 0x07)
		emit1(0x40 | REX_B);

	op |= (arg1.v.reg & 0x07);

	emit1(op);
}

/* operator which operates on reg/mem */
static void emit_op_rm(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 rex, modrm, sib;
	opparam_t* params = data;

	if((arg1.type != T_REGISTER && arg1.type != T_MEMORY) || arg2.type != T_NONE)
		CRAP_INVALID_ARGS;

	compute_rexmodrmsib(&rex, &modrm, &sib, &arg2, &arg1);

	modrm |= params->subcode << 3;

	if(arg1.v.reg & R_16)
		emit1(0x66);

	if(rex) emit1(rex);
	if(arg1.v.reg & R_8)
		emit1(params->rcode8); // op reg8/mem8,
	else
		emit1(params->rcode); // op reg/mem,
	emit1(modrm);
	if((modrm & 0x07) == MODRM_RM_SIB)
		emit1(sib);

	maybe_emit_displacement(&arg1);
}

/* operator which operates on reg/mem with cl */
static void emit_op_rm_cl(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 rex, modrm, sib;
	opparam_t* params = data;

	if(arg2.type != T_REGISTER || arg1.type != T_REGISTER)
		CRAP_INVALID_ARGS;

	if((arg1.v.reg & R_MGP) != R_ECX && !(arg1.v.reg & R_8))
		crap("only cl register is valid");

	arg1.type = T_NONE; // don't complain, we know it's cl anyways

	compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);

	modrm |= params->subcode << 3;

	if(arg2.v.reg & R_16)
		emit1(0x66);

	if(rex) emit1(rex);
	if(arg2.v.reg & R_8)
		emit1(params->rcode8); // op reg8/mem8,
	else
		emit1(params->rcode); // op reg/mem,
	emit1(modrm);
	if((modrm & 0x07) == MODRM_RM_SIB)
		emit1(sib);

	maybe_emit_displacement(&arg2);
}

static void emit_mov(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 rex = 0;
	u8 modrm = 0;
	u8 sib = 0;

	if(arg1.type == T_IMMEDIATE && arg2.type == T_REGISTER)
	{
		u8 op = 0xb8;
		
		if(arg2.v.reg & R_8)
		{
			if(!iss8(arg1.v.imm))
				crap("value too large for 8bit register");

			op = 0xb0;
		}
		else if(arg2.v.reg & R_16)
		{
			if(!isu16(arg1.v.imm))
				crap("value too large for 16bit register");
			emit1(0x66);
		}
		else if(!(arg2.v.reg & R_64))
		{
			if(!isu32(arg1.v.imm))
				crap("value too large for 32bit register");
		}

		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);

		if(rex) emit1(rex);

		op |= (arg2.v.reg & 0x07);

		emit1(op);

		if(arg2.v.reg & R_8) emit1(arg1.v.imm);
		else if(arg2.v.reg & R_16) emit2(arg1.v.imm);
		else if(arg2.v.reg & R_64) emit8(arg1.v.imm);
		else emit4(arg1.v.imm);
	}
	else if(arg1.type == T_IMMEDIATE && arg2.type == T_MEMORY)
	{
		if(!iss32(arg1.v.imm))
		{
			crap("only 32bit immediates supported");
		}

		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);
		if(rex) emit1(rex);
		emit1(0xc7); // mov reg/mem, imm
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		emit4(arg1.v.imm);
	}
	else if(arg1.type == T_REGISTER && arg2.type == T_REGISTER) // XXX: same as next
	{
		if(arg1.type != T_REGISTER || arg2.type != T_REGISTER)
			crap("both args must be registers");

		if((arg1.v.reg & R_MSZ) != (arg2.v.reg & R_MSZ))
			crap("both registers must be same width");

		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);

		if(rex) emit1(rex);
		emit1(0x89); // mov reg reg/mem,
		emit1(modrm);
	}
	else if(arg1.type == T_REGISTER && arg2.type == T_MEMORY)
	{
		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);

		if(arg1.v.reg & R_16)
			emit1(0x66);

		if(rex) emit1(rex);
		if(arg1.v.reg & R_8)
			emit1(0x88); // mov reg reg/mem,
		else
			emit1(0x89); // mov reg reg/mem,
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		maybe_emit_displacement(&arg2);
	}
	else if(arg1.type == T_MEMORY && arg2.type == T_REGISTER)
	{
		compute_rexmodrmsib(&rex, &modrm, &sib, &arg2, &arg1);

		if(arg2.v.reg & R_16)
			emit1(0x66);

		if(rex) emit1(rex);
		if(arg2.v.reg & R_8)
			emit1(0x8a); // mov reg/mem, reg
		else
			emit1(0x8b); // mov reg/mem, reg
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		maybe_emit_displacement(&arg1);
	}
	else
		CRAP_INVALID_ARGS;
}

static void emit_subaddand(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 rex = 0;
	u8 modrm = 0;
	u8 sib = 0;

	opparam_t* params = data;

	if(arg1.type == T_IMMEDIATE && arg2.type == T_REGISTER)
	{
		if(!iss32(arg1.v.imm))
		{
			crap("only 8 and 32 bit immediates supported");
		}

		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);
		modrm |= params->subcode << 3;

		if(rex) emit1(rex);

		if(arg2.v.reg & R_8)
		{
			emit1(0x80); // sub reg8/mem8, imm8
			emit1(modrm);
			emit1(arg1.v.imm & 0xFF);
		}
		else if(iss8(arg1.v.imm))
		{
			emit1(0x83); // sub reg/mem, imm8
			emit1(modrm);
			emit1(arg1.v.imm & 0xFF);
		}
		else
		{
			emit1(0x81); // sub reg/mem, imm32
			emit1(modrm);
			emit4(arg1.v.imm);
		}
	}
	else if(arg1.type == T_REGISTER && (arg2.type == T_MEMORY || arg2.type == T_REGISTER))
	{
		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);

		if(rex) emit1(rex);
		emit1(params->rmcode); // sub reg/mem, reg
		emit1(modrm);
		if(arg2.type == T_MEMORY && (modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		maybe_emit_displacement(&arg2);
	}
	else if(arg1.type == T_MEMORY && arg2.type == T_REGISTER && params->mrcode)
	{
		compute_rexmodrmsib(&rex, &modrm, &sib, &arg2, &arg1);

		if(rex) emit1(rex);
		emit1(params->mrcode); // sub reg, reg/mem
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		maybe_emit_displacement(&arg1);
	}
	else
		CRAP_INVALID_ARGS;
}

static void emit_condjump(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	unsigned off;
	int disp;
	unsigned char opcode = (unsigned char)(((uint64_t)data)&0xFF);

	if(arg1.type != T_LABEL || arg2.type != T_NONE)
		crap("%s: argument must be label", mnemonic);

	emit1(opcode);

	off = lookup_label(arg1.v.label);
	disp = off-(compiledOfs+1);
	if(assembler_pass && abs(disp) > 127)
		crap("cannot jump that far (%x -> %x = %x)", compiledOfs, off, disp);

	emit1(disp);
}

static void emit_jmp(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	if((arg1.type != T_LABEL && arg1.type != T_REGISTER && arg1.type != T_MEMORY) || arg2.type != T_NONE)
		CRAP_INVALID_ARGS;

	if(arg1.type == T_LABEL)
	{
		unsigned off;
		int disp;

		off = lookup_label(arg1.v.label);
		disp = off-(compiledOfs+5);
		emit1(0xe9);
		emit4(disp);
	}
	else
	{
		u8 rex, modrm, sib;

		if(arg1.type == T_REGISTER)
		{
			if(!arg1.absolute)
				crap("jmp must be absolute");

			if((arg1.v.reg & R_64) != R_64)
				crap("register must be 64bit");

			arg1.v.reg ^= R_64; // no rex required for call
		}

		compute_rexmodrmsib(&rex, &modrm, &sib, &arg2, &arg1);

		modrm |= 0x4 << 3;

		if(rex) emit1(rex);
		emit1(0xff);
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);
		maybe_emit_displacement(&arg1);
	}
}

static void emit_call(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 rex, modrm, sib;

	if((arg1.type != T_REGISTER && arg1.type != T_IMMEDIATE) || arg2.type != T_NONE)
		CRAP_INVALID_ARGS;

	if(arg1.type == T_REGISTER)
	{
		if(!arg1.absolute)
			crap("call must be absolute");

		if((arg1.v.reg & R_64) != R_64)
			crap("register must be 64bit");

		arg1.v.reg ^= R_64; // no rex required for call

		compute_rexmodrmsib(&rex, &modrm, &sib, &arg2, &arg1);

		modrm |= 0x2 << 3;

		if(rex) emit1(rex);
		emit1(0xff);
		emit1(modrm);
	}
	else 
	{
		if(!isu32(arg1.v.imm))
			crap("must be 32bit argument");
		emit1(0xe8);
		emit4(arg1.v.imm);
	}
}


static void emit_twobyte(const char* mnemonic, arg_t arg1, arg_t arg2, void* data)
{
	u8 rex, modrm, sib;

	opparam_t* params = data;

	if(arg1.type == T_REGISTER && (arg2.type == T_MEMORY || arg2.type == T_REGISTER))
	{
		compute_rexmodrmsib(&rex, &modrm, &sib, &arg1, &arg2);

		if(params->xmmprefix) emit1(params->xmmprefix);
		if(rex) emit1(rex);
		emit1(0x0f);
		emit1(params->rmcode); // sub reg/mem, reg
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		maybe_emit_displacement(&arg2);
	}
	else if(arg1.type == T_MEMORY && arg2.type == T_REGISTER && params->mrcode)
	{
		compute_rexmodrmsib(&rex, &modrm, &sib, &arg2, &arg1);

		if(params->xmmprefix) emit1(params->xmmprefix);
		if(rex) emit1(rex);
		emit1(0x0f);
		emit1(params->mrcode); // sub reg, reg/mem
		emit1(modrm);
		if((modrm & 0x07) == MODRM_RM_SIB)
			emit1(sib);

		maybe_emit_displacement(&arg1);
	}
	else
		CRAP_INVALID_ARGS;
}

static int ops_sorted = 0;
static op_t ops[] = {
	{ "addb", emit_subaddand, &params_add },
	{ "addl", emit_subaddand, &params_add },
	{ "addq", emit_subaddand, &params_add },
	{ "addss", emit_twobyte, &params_addss },
	{ "andb", emit_subaddand, &params_and },
	{ "andl", emit_subaddand, &params_and },
	{ "andq", emit_subaddand, &params_and },
	{ "callq", emit_call, NULL },
	{ "cbw", emit_opsingle16, (void*)0x98 },
	{ "cdq", emit_opsingle, (void*)0x99 },
	{ "cmpb", emit_subaddand, &params_cmp },
	{ "cmpl", emit_subaddand, &params_cmp },
	{ "cmpq", emit_subaddand, &params_cmp },
	{ "cvtsi2ss", emit_twobyte, &params_cvtsi2ss },
	{ "cvttss2si", emit_twobyte, &params_cvttss2si },
	{ "cwde", emit_opsingle, (void*)0x98 },
	{ "decl", emit_op_rm, &params_dec },
	{ "decq", emit_op_rm, &params_dec },
	{ "divl", emit_op_rm, &params_div },
	{ "divq", emit_op_rm, &params_div },
	{ "divss", emit_twobyte, &params_divss },
	{ "idivl", emit_op_rm, &params_idiv },
	{ "imull", emit_op_rm, &params_imul },
	{ "int3", emit_opsingle, (void*)0xcc },
	{ "ja", emit_condjump, (void*)0x77 },
	{ "jbe", emit_condjump, (void*)0x76 },
	{ "jb", emit_condjump, (void*)0x72 },
	{ "je", emit_condjump, (void*)0x74 },
	{ "jl", emit_condjump, (void*)0x7c },
	{ "jmp", emit_jmp, NULL },
	{ "jmpq", emit_jmp, NULL },
	{ "jnae", emit_condjump, (void*)0x72 },
	{ "jna", emit_condjump, (void*)0x76 },
	{ "jnbe", emit_condjump, (void*)0x77 },
	{ "jnb", emit_condjump, (void*)0x73 },
	{ "jnc", emit_condjump, (void*)0x73 },
	{ "jne", emit_condjump, (void*)0x75 },
	{ "jnge", emit_condjump, (void*)0x7c },
	{ "jng", emit_condjump, (void*)0x7e },
	{ "jnle", emit_condjump, (void*)0x7f },
	{ "jnl", emit_condjump, (void*)0x7d },
	{ "jnz", emit_condjump, (void*)0x75 },
	{ "jp", emit_condjump, (void*)0x7a },
	{ "jz", emit_condjump, (void*)0x74 },
	{ "movb", emit_mov, NULL },
	{ "movl", emit_mov, NULL },
	{ "movq", emit_mov, NULL },
	{ "movss", emit_twobyte, &params_movss },
	{ "movw", emit_mov, NULL },
	{ "mull", emit_op_rm, &params_mul },
	{ "mulss", emit_twobyte, &params_mulss },
	{ "negl", emit_op_rm, &params_neg },
	{ "negq", emit_op_rm, &params_neg },
	{ "nop", emit_opsingle, (void*)0x90 },
	{ "notl", emit_op_rm, &params_not },
	{ "notq", emit_op_rm, &params_not },
	{ "orb",   emit_subaddand, &params_or },
	{ "orl",  emit_subaddand, &params_or },
	{ "orq",  emit_subaddand, &params_or },
	{ "pop", emit_opreg, (void*)0x58 },
	{ "push", emit_opreg, (void*)0x50 },
	{ "ret", emit_opsingle, (void*)0xc3 },
	{ "sarl", emit_op_rm_cl, &params_sar },
	{ "shl", emit_op_rm_cl, &params_shl },
	{ "shrl", emit_op_rm_cl, &params_shr },
	{ "subb", emit_subaddand, &params_sub },
	{ "subl", emit_subaddand, &params_sub },
	{ "subq", emit_subaddand, &params_sub },
	{ "subss", emit_twobyte, &params_subss },
	{ "ucomiss", emit_twobyte, &params_ucomiss },
	{ "xorb",  emit_subaddand, &params_xor },
	{ "xorl",  emit_subaddand, &params_xor },
	{ "xorq",  emit_subaddand, &params_xor },
	{ NULL, NULL, NULL }
};

static int opsort(const void* A, const void* B)
{
	const op_t* a = A;
	const op_t* b = B;
	return strcmp(a->mnemonic, b->mnemonic);
}

static op_t* getop(const char* n)
{
#if 0
	op_t* o = ops;
	while(o->mnemonic)
	{
		if(!strcmp(o->mnemonic, n))
			return o;
		++o;
	}

#else
	unsigned int m, t, b;
	int r;
	t = ARRAY_LEN(ops)-1;

	r = m = -1;

	do
	{
		if(r < 0)
			b = m + 1;
		else
			t = m - 1;

		m = ((t - b) >> 1) + b;

		if((r = strcmp(ops[m].mnemonic, n)) == 0)
			return &ops[m];
	} while(b <= t && t);
#endif

	return NULL;
}

static reg_t parsereg(const char* str)
{
	const char* s = str;
	if(*s == 'a' && s[1] == 'l' && !s[2])
	{
		return R_AL;
	}
	else if(*s == 'a' && s[1] == 'x' && !s[2])
	{
		return R_AX;
	}
	if(*s == 'b' && s[1] == 'l' && !s[2])
	{
		return R_BL;
	}
	if(*s == 'c' && s[1] == 'l' && !s[2])
	{
		return R_CL;
	}
	if(*s == 'x')
	{
		if(!strcmp(s, "xmm0"))
			return R_XMM0;
	}
	else if(*s == 'r' && s[1])
	{
		++s;
		if(s[1] == 'x')
		{
			switch(*s++)
			{
				case 'a': return R_RAX;
				case 'b': return R_RBX;
				case 'c': return R_RCX;
				case 'd': return R_RDX;
			}
		}
		else if(s[1] == 'i')
		{
			switch(*s++)
			{
				case 's': return R_RSI;
				case 'd': return R_RDI;
			}
		}
		else if(s[0] == 's' && s[1] == 'p' && !s[2])
		{
			return R_RSP;
		}
		else if(*s == '8' && !s[1])
			return R_R8;
		else if(*s == '9' && !s[1])
			return R_R9;
		else if(*s == '1' && s[1] == '0')
			return R_R10;
		else if(*s == '1' && s[1] == '5')
			return R_R15;
	}
	else if(*s == 'e' && s[1])
	{
		++s;
		if(s[1] == 'x')
		{
			switch(*s++)
			{
				case 'a': return R_EAX;
				case 'b': return R_EBX;
				case 'c': return R_ECX;
				case 'd': return R_EDX;
			}
		}
		else if(s[1] == 'i')
		{
			switch(*s++)
			{
				case 's': return R_ESI;
				case 'd': return R_EDI;
			}
		}
	}

	crap("invalid register %s", str);

	return 0;
}

typedef enum {
	TOK_LABEL = 0x80,
	TOK_INT = 0x81,
	TOK_END = 0x82,
	TOK_INVALID = 0x83,
} token_t;

static unsigned char nexttok(const char** str, char* label, u64* val)
{
	const char* s = *str;

	if(label) *label = 0;
	if(val) *val = 0;

	while(*s && *s == ' ') ++s;

	if(!*s)
	{
		return TOK_END;
	}
	else if(*s == '$' || *s == '*' || *s == '%' || *s == '-' || *s == ')' || *s == '(' || *s == ',')
	{
		*str = s+1;
		return *s;
	}
	else if(*s >= 'a' && *s <= 'z')
	{
		size_t a = strspn(s+1, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
		if(a+1 >= LABELLEN)
			crap("label %s too long", s);
		if(label)
		{
			strncpy(label, s, a+1);
			label[a+1] = 0;
		}
		*str = s+a+1;
		return TOK_LABEL;
	}
	else if(*s >= '0' && *s <= '9')
	{
		char* endptr = NULL;
		u64 v = strtoull(s, &endptr, 0);
		if(endptr && (endptr-s == 0))
			crap("invalid integer %s", s);
		if(val) *val = v;
		*str = endptr;
		return TOK_INT;
	}
	crap("can't parse '%s'", *str);
	return TOK_INVALID;
}

static arg_t parsearg(const char** str)
{
	arg_t arg;
	const char* s = *str;
	char label[20];
	u64 val;
	int negative = 1;
	unsigned ttype;

	arg.type = T_NONE;
	arg.absolute = 0;

	while(*s && *s == ' ') ++s;

	switch(nexttok(&s, label, &val))
	{
		case '$' :
			ttype = nexttok(&s, NULL, &val);
			if(ttype == '-')
			{
				negative = -1;
				ttype = nexttok(&s, NULL, &val);
			}
			if(ttype != TOK_INT)
				crap("expected integer");
			arg.type = T_IMMEDIATE;
			arg.v.imm = negative * val;
			break;
		case '*' :
			if((ttype = nexttok(&s, NULL, NULL)) != '%')
			{
				if(ttype == '(')
					goto tok_memory;
				crap("expected '%%'");
			}
			arg.absolute = 1;
			/* fall through */
		case '%' :
			if(nexttok(&s, label, &val) != TOK_LABEL)
				crap("expected label");
			arg.type = T_REGISTER;
			arg.v.reg = parsereg(label);
			break;
		case TOK_LABEL:
			arg.type = T_LABEL;
			strncpy(arg.v.label, label, LABELLEN);
			break;
		case '-':
			negative = -1;
			if(nexttok(&s, NULL, &val) != TOK_INT)
				crap("expected integer");
			/* fall through */
		case TOK_INT:
			if(nexttok(&s, label, NULL) != '(')
				crap("expected '('"); // mov to/from fixed address not supported
			/* fall through */
		case '(':
tok_memory:
			arg.type = T_MEMORY;
			arg.v.mem.indextype = T_NONE;
			arg.v.mem.disp = negative * val;
			ttype = nexttok(&s, label, &val);
			if(ttype == '%' && nexttok(&s, label, &val) != TOK_LABEL)
			{
				crap("expected register");
			}
			if (ttype == '%')
			{
				arg.v.mem.basetype = T_REGISTER;
				arg.v.mem.base.reg = parsereg(label);
			}
			else if (ttype == TOK_INT)
			{
				arg.v.mem.basetype = T_IMMEDIATE;
				arg.v.mem.base.imm = val;
			}
			if((ttype = nexttok(&s, NULL, NULL)) == ',')
			{
				ttype = nexttok(&s, label, &val);
				if(ttype == '%' && nexttok(&s, label, &val) != TOK_LABEL)
				{
					crap("expected register");
				}
				if (ttype == '%')
				{
					arg.v.mem.indextype = T_REGISTER;
					arg.v.mem.index.reg = parsereg(label);
				}
				else if (ttype == TOK_INT)
				{
					crap("index must be register");
					arg.v.mem.indextype = T_IMMEDIATE;
					arg.v.mem.index.imm = val;
				}
				if(nexttok(&s, NULL, NULL) != ',')
					crap("expected ','");
				if(nexttok(&s, NULL, &val) != TOK_INT)
					crap("expected integer");
				if(val != 1 && val != 2 && val != 4 && val != 8)
					crap("scale must 1, 2, 4 or 8");
				arg.v.mem.scale = val;

				ttype = nexttok(&s, NULL, NULL);
			}
			if(ttype != ')')
			{
				crap("expected ')' or ','");
			}
			break;
		default:
			crap("invalid token %hu in %s", *(unsigned char*)s, *str);
			break;
	}

	*str = s;

	return arg;
}

/* ************************* */

void assembler_init(int pass)
{
	compiledOfs = 0;
	assembler_pass = pass;
	if(!pass)
	{
		labelhash_free();
		cur_line = NULL;
	}
	if(!ops_sorted)
	{
		ops_sorted = 1;
		qsort(ops, ARRAY_LEN(ops)-1, sizeof(ops[0]), opsort);
	}
}

size_t assembler_get_code_size(void)
{
	return compiledOfs;
}

void assembler_set_output(char* buf)
{
	out = buf;
}

void assemble_line(const char* input, size_t len)
{
	char line[4096];
	char* s;
	op_t* o;
	char* opn;
	arg_t arg1, arg2;

	arg1.type = T_NONE;
	arg2.type = T_NONE;
	opn = NULL;
	o = NULL;

	if(len < 1)
		return;

	if(len >= sizeof(line))
		crap("line too long");

	memcpy(line, input, sizeof(line));
	cur_line = input;

	if(line[len-1] == '\n') line[--len] = 0;
	if(line[len-1] == ':')
	{
		line[--len] = 0;
		if(assembler_pass)
			debug("%s: 0x%x\n", line, compiledOfs);
		else
			hash_add_label(line, compiledOfs);
	}
	else
	{
		opn = line;
		s = strchr(line, ' ');
		if(s)
		{
			*s++ = 0;
			arg1 = parsearg((const char**)&s);
			if(*s)
			{
				if(*s != ',')
					crap("expected ',', got '%c'", *s);
				++s;
				arg2 = parsearg((const char**)&s);
			}
		}

		if(!opn)
		{
			crap("no operator in %s", line);
		}

		o = getop(opn);
		if(!o)
		{
			crap("cannot handle op %s", opn);
		}
		o->func(opn, arg1, arg2, o->data);
		if(assembler_pass)
			debug("   - %s%s", cur_line, cur_line[strlen(cur_line)-1]=='\n'?"":"\n");
	}
}

#ifdef SA_STANDALONE
int main(int argc, char* argv[])
{
	char line[4096];
	size_t len;
	int pass;
	FILE* file = NULL;

	if(argc < 2)
	{
		crap("specify file");
	}

	file = fopen(argv[1], "r");
	if(!file)
	{
		crap("can't open file");
	}

	if(argc > 2)
	{
		fout = fopen(argv[2], "w");
		if(!fout)
		{
			crap("can't open %s for writing", argv[2]);
		}
	}

	for(pass = 0; pass < 2; ++pass)
	{
		if(fseek(file, 0, SEEK_SET))
			crap("can't rewind file");

		if(pass)
		{
			char* b = malloc(assembler_get_code_size());
			if(!b)
				crap("cannot allocate memory");
			assembler_set_output(b);
		}

		assembler_init(pass);

		while(fgets(line, sizeof(line), file))
		{
			len = strlen(line);
			if(!len) continue;

			assemble_line(line, len);
		}
	}

	assembler_init(0);

	fclose(file);

	return 0;
}
#endif
