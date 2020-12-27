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

 * File includes code from GNU binutils, exactly:
 * - include/opcode/ppc.h - licensed under GPL v1 or later
 * - opcodes/ppc-opc.c - licensed under GPL v2 or later
 *
 * ppc.h -- Header file for PowerPC opcode table
 *   Copyright 1994, 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006,
 *   2007 Free Software Foundation, Inc.
 *   Written by Ian Lance Taylor, Cygnus Suppor
 *
 *   This file is part of GDB, GAS, and the GNU binutils.
 *
 * ppc-opc.c -- PowerPC opcode list
 *   Copyright 1994, 1995, 1996, 1997, 1998, 2000, 2001, 2002, 2003, 2004,
 *   2005, 2006, 2007 Free Software Foundation, Inc.
 *   Written by Ian Lance Taylor, Cygnus Support
 *
 *   This file is part of GDB, GAS, and the GNU binutils.
 *
 */

#include "vm_local.h"
#include "vm_powerpc_asm.h"

#include <string.h>
#include <stdio.h>
#include <inttypes.h>

/* return nop on error */
#define ASM_ERROR_OPC (0x60000000)

/*
 * BEGIN OF ppc.h
 */

#define ppc_cpu_t int

struct powerpc_opcode
{
	const char *name;
	unsigned long opcode;
	unsigned long mask;
	ppc_cpu_t flags;
	unsigned char operands[8];
};

static const struct powerpc_opcode powerpc_opcodes[];

#define PPC_OPCODE_PPC			 1
#define PPC_OPCODE_POWER		 2
#define PPC_OPCODE_POWER2		 4
#define PPC_OPCODE_32			 8
#define PPC_OPCODE_64		      0x10
#define PPC_OPCODE_601		      0x20
#define PPC_OPCODE_COMMON	      0x40
#define PPC_OPCODE_ANY		      0x80
#define PPC_OPCODE_64_BRIDGE	     0x100
#define PPC_OPCODE_ALTIVEC	     0x200
#define PPC_OPCODE_403		     0x400
#define PPC_OPCODE_BOOKE	     0x800
#define PPC_OPCODE_BOOKE64	    0x1000
#define PPC_OPCODE_440		    0x2000
#define PPC_OPCODE_POWER4	    0x4000
#define PPC_OPCODE_NOPOWER4	    0x8000
#define PPC_OPCODE_CLASSIC	   0x10000
#define PPC_OPCODE_SPE		   0x20000
#define PPC_OPCODE_ISEL		   0x40000
#define PPC_OPCODE_EFS		   0x80000
#define PPC_OPCODE_BRLOCK	  0x100000
#define PPC_OPCODE_PMR		  0x200000
#define PPC_OPCODE_CACHELCK	  0x400000
#define PPC_OPCODE_RFMCI	  0x800000
#define PPC_OPCODE_POWER5	 0x1000000
#define PPC_OPCODE_E300		 0x2000000
#define PPC_OPCODE_POWER6	 0x4000000
#define PPC_OPCODE_CELL		 0x8000000
#define PPC_OPCODE_PPCPS	0x10000000
#define PPC_OPCODE_E500MC	0x20000000
#define PPC_OPCODE_405		0x40000000
#define PPC_OPCODE_VSX		0x80000000

#define PPC_OP(i) (((i) >> 26) & 0x3f)

struct powerpc_operand
{
	unsigned int bitm;
	int shift;
	unsigned long (*insert)
		(unsigned long, long, int, const char **);
	unsigned long flags;
};

static const struct powerpc_operand powerpc_operands[];

#define PPC_OPERAND_SIGNED (0x1)
#define PPC_OPERAND_SIGNOPT (0x2)
#define PPC_OPERAND_FAKE (0x4)
#define PPC_OPERAND_PARENS (0x8)
#define PPC_OPERAND_CR (0x10)
#define PPC_OPERAND_GPR (0x20)
#define PPC_OPERAND_GPR_0 (0x40)
#define PPC_OPERAND_FPR (0x80)
#define PPC_OPERAND_RELATIVE (0x100)
#define PPC_OPERAND_ABSOLUTE (0x200)
#define PPC_OPERAND_OPTIONAL (0x400)
#define PPC_OPERAND_NEXT (0x800)
#define PPC_OPERAND_NEGATIVE (0x1000)
#define PPC_OPERAND_VR (0x2000)
#define PPC_OPERAND_DS (0x4000)
#define PPC_OPERAND_DQ (0x8000)
#define PPC_OPERAND_PLUS1 (0x10000)
#define PPC_OPERAND_FSL (0x20000)
#define PPC_OPERAND_FCR (0x40000)
#define PPC_OPERAND_UDI (0x80000)
#define PPC_OPERAND_VSR (0x100000)

/*
 * END OF ppc.h
 */

#define PPC_DEST_ARCH PPC_OPCODE_PPC

ppc_instruction_t
asm_instruction( powerpc_iname_t sname, const int argc, const long int *argv )
{
	const char *errmsg = NULL;
	const char *name;
	unsigned long int ret;
	const struct powerpc_opcode *opcode = NULL;
	int argi, argj;

	opcode = &powerpc_opcodes[ sname ];
	name = opcode->name;

	if ( ! opcode ) {
		printf( "Can't find opcode %d\n", sname );
		return ASM_ERROR_OPC;
	}
	if ( ( opcode->flags & PPC_DEST_ARCH ) != PPC_DEST_ARCH ) {
		printf( "opcode %s not defined for this arch\n", name );
		return ASM_ERROR_OPC;
	}

	ret = opcode->opcode;

	argi = argj = 0;
	while ( opcode->operands[ argi ] != 0 ) {
		long int op = 0;
		const struct powerpc_operand *operand = &powerpc_operands[ opcode->operands[ argi ] ];

		if ( ! (operand->flags & PPC_OPERAND_FAKE) ) {
			if ( argj >= argc ) {
				printf( "Not enough arguments for %s, got %d\n", name, argc );
				return ASM_ERROR_OPC;
			}

			op = argv[ argj++ ];
		}

		if ( operand->insert ) {
			errmsg = NULL;
			ret = operand->insert( ret, op, PPC_DEST_ARCH, &errmsg );
			if ( errmsg ) {
				printf( "%s: error while inserting operand %d (0x%.2lx): %s\n",
					name, argi, op, errmsg );
			}
		} else {
			unsigned long int opu = *(unsigned long int *)&op;
			unsigned long int bitm = operand->bitm;
			unsigned long int bitm_full = bitm | ( bitm & 1 ? 0 : 0xf );

			if ( operand->flags & PPC_OPERAND_SIGNED ) {
				bitm_full >>= 1;

				if ( ( opu & ~bitm_full ) != 0 && ( opu | bitm_full ) != -1 )
					printf( "%s: signed operand nr.%d to wide. op: %.8lx, mask: %.8lx\n",
						name, argi, opu, bitm );
			} else {
				if ( ( opu & ~bitm_full ) != 0 )
					printf( "%s: unsigned operand nr.%d to wide. op: %.8lx, mask: %.8lx\n",
						name, argi, opu, bitm );
			}
			if ( (bitm & 1) == 0 ) {
				if ( opu & 0xf & ~bitm )
					printf( "%s: operand nr.%d not aligned correctly. op: %.8lx, mask: %.8lx\n",
						name, argi, opu, bitm );
			}

			ret |= ( op & operand->bitm ) << operand->shift;
		}
		argi++;
	}
	if ( argc > argj ) {
		printf( "Too many arguments for %s, got %d\n", name, argc );
		return ASM_ERROR_OPC;
	}

	return ret;
}


/*
 * BEGIN OF ppc-opc.c
 */

#define ATTRIBUTE_UNUSED
#define _(x) (x)

/* Local insertion and extraction functions. */

static unsigned long insert_bdm (unsigned long, long, int, const char **);
static unsigned long insert_bo (unsigned long, long, int, const char **);
static unsigned long insert_ras (unsigned long, long, int, const char **);
static unsigned long insert_rbs (unsigned long, long, int, const char **);

/* The operands table.

   The fields are bitm, shift, insert, extract, flags.
   */

static const struct powerpc_operand powerpc_operands[] =
{
  /* The zero index is used to indicate the end of the list of
     operands.  */
#define UNUSED 0
  { 0, 0, NULL, 0 },

  /* The BA field in an XL form instruction.  */
#define BA UNUSED + 1
  /* The BI field in a B form or XL form instruction.  */
#define BI BA
#define BI_MASK (0x1f << 16)
  { 0x1f, 16, NULL, PPC_OPERAND_CR },

  /* The BD field in a B form instruction.  The lower two bits are
     forced to zero.  */
#define BD BA + 1
  { 0xfffc, 0, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BD field in a B form instruction when the - modifier is used.
     This sets the y bit of the BO field appropriately.  */
#define BDM BD + 1
  { 0xfffc, 0, insert_bdm,
      PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The BF field in an X or XL form instruction.  */
#define BF BDM + 1
  /* The CRFD field in an X form instruction.  */
#define CRFD BF
  { 0x7, 23, NULL, PPC_OPERAND_CR },

  /* An optional BF field.  This is used for comparison instructions,
     in which an omitted BF field is taken as zero.  */
#define OBF BF + 1
  { 0x7, 23, NULL, PPC_OPERAND_CR | PPC_OPERAND_OPTIONAL },

  /* The BO field in a B form instruction.  Certain values are
     illegal.  */
#define BO OBF + 1
#define BO_MASK (0x1f << 21)
  { 0x1f, 21, insert_bo, 0 },

  /* The condition register number portion of the BI field in a B form
     or XL form instruction.  This is used for the extended
     conditional branch mnemonics, which set the lower two bits of the
     BI field.  This field is optional.  */
#define CR BO + 1
  { 0x7, 18, NULL, PPC_OPERAND_CR | PPC_OPERAND_OPTIONAL },

  /* The D field in a D form instruction.  This is a displacement off
     a register, and implies that the next operand is a register in
     parentheses.  */
#define D CR + 1
  { 0xffff, 0, NULL, PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED },

  /* The DS field in a DS form instruction.  This is like D, but the
     lower two bits are forced to zero.  */
#define DS D + 1
  { 0xfffc, 0, NULL,
    PPC_OPERAND_PARENS | PPC_OPERAND_SIGNED | PPC_OPERAND_DS },

  /* The FRA field in an X or A form instruction.  */
#define FRA DS + 1
#define FRA_MASK (0x1f << 16)
  { 0x1f, 16, NULL, PPC_OPERAND_FPR },

  /* The FRB field in an X or A form instruction.  */
#define FRB FRA + 1
#define FRB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, PPC_OPERAND_FPR },

  /* The FRC field in an A form instruction.  */
#define FRC FRB + 1
#define FRC_MASK (0x1f << 6)
  { 0x1f, 6, NULL, PPC_OPERAND_FPR },

  /* The FRS field in an X form instruction or the FRT field in a D, X
     or A form instruction.  */
#define FRS FRC + 1
#define FRT FRS
  { 0x1f, 21, NULL, PPC_OPERAND_FPR },

  /* The LI field in an I form instruction.  The lower two bits are
     forced to zero.  */
#define LI FRS + 1
  { 0x3fffffc, 0, NULL, PPC_OPERAND_RELATIVE | PPC_OPERAND_SIGNED },

  /* The ME field in an M form instruction.  */
#define ME LI + 1
#define ME_MASK (0x1f << 1)
  { 0x1f, 1, NULL, 0 },

  /* The MB and ME fields in an M form instruction expressed a single
     operand which is a bitmask indicating which bits to select.  This
     is a two operand form using PPC_OPERAND_NEXT.  See the
     description in opcode/ppc.h for what this means.  */
#define MBE ME + 1
  { 0x1f, 6, NULL, PPC_OPERAND_OPTIONAL | PPC_OPERAND_NEXT },

  /* The RA field in an D, DS, DQ, X, XO, M, or MDS form instruction.  */
#define RA MBE + 1
#define RA_MASK (0x1f << 16)
  { 0x1f, 16, NULL, PPC_OPERAND_GPR },

  /* As above, but 0 in the RA field means zero, not r0.  */
#define RA0 RA + 1
  { 0x1f, 16, NULL, PPC_OPERAND_GPR_0 },

  /* The RA field in a D or X form instruction which is an updating
     store or an updating floating point load, which means that the RA
     field may not be zero.  */
#define RAS RA0 + 1
  { 0x1f, 16, insert_ras, PPC_OPERAND_GPR_0 },

  /* The RB field in an X, XO, M, or MDS form instruction.  */
#define RB RAS + 1
#define RB_MASK (0x1f << 11)
  { 0x1f, 11, NULL, PPC_OPERAND_GPR },

  /* The RB field in an X form instruction when it must be the same as
     the RS field in the instruction.  This is used for extended
     mnemonics like mr.  */
#define RBS RB + 1
  { 0x1f, 11, insert_rbs, PPC_OPERAND_FAKE },

  /* The RS field in a D, DS, X, XFX, XS, M, MD or MDS form
     instruction or the RT field in a D, DS, X, XFX or XO form
     instruction.  */
#define RS RBS + 1
#define RT RS
#define RT_MASK (0x1f << 21)
  { 0x1f, 21, NULL, PPC_OPERAND_GPR },

  /* The SH field in an X or M form instruction.  */
#define SH RS + 1
#define SH_MASK (0x1f << 11)
  /* The other UIMM field in an EVX form instruction.  */
#define EVUIMM SH
  { 0x1f, 11, NULL, 0 },

  /* The SI field in a D form instruction.  */
#define SI SH + 1
  { 0xffff, 0, NULL, PPC_OPERAND_SIGNED },

  /* The UI field in a D form instruction.  */
#define UI SI + 1
  { 0xffff, 0, NULL, 0 },

};


/* The functions used to insert and extract complicated operands.  */

/* The BD field in a B form instruction when the - modifier is used.
   This modifier means that the branch is not expected to be taken.
   For chips built to versions of the architecture prior to version 2
   (ie. not Power4 compatible), we set the y bit of the BO field to 1
   if the offset is negative.  When extracting, we require that the y
   bit be 1 and that the offset be positive, since if the y bit is 0
   we just want to print the normal form of the instruction.
   Power4 compatible targets use two bits, "a", and "t", instead of
   the "y" bit.  "at" == 00 => no hint, "at" == 01 => unpredictable,
   "at" == 10 => not taken, "at" == 11 => taken.  The "t" bit is 00001
   in BO field, the "a" bit is 00010 for branch on CR(BI) and 01000
   for branch on CTR.  We only handle the taken/not-taken hint here.
   Note that we don't relax the conditions tested here when
   disassembling with -Many because insns using extract_bdm and
   extract_bdp always occur in pairs.  One or the other will always
   be valid.  */

static unsigned long
insert_bdm (unsigned long insn,
	    long value,
	    int dialect,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  if ((dialect & PPC_OPCODE_POWER4) == 0)
    {
      if ((value & 0x8000) != 0)
	insn |= 1 << 21;
    }
  else
    {
      if ((insn & (0x14 << 21)) == (0x04 << 21))
	insn |= 0x02 << 21;
      else if ((insn & (0x14 << 21)) == (0x10 << 21))
	insn |= 0x08 << 21;
    }
  return insn | (value & 0xfffc);
}


/* Check for legal values of a BO field.  */

static int
valid_bo (long value, int dialect, int extract)
{
  if ((dialect & PPC_OPCODE_POWER4) == 0)
    {
      int valid;
      /* Certain encodings have bits that are required to be zero.
	 These are (z must be zero, y may be anything):
	     001zy
	     011zy
	     1z00y
	     1z01y
	     1z1zz
      */
      switch (value & 0x14)
	{
	default:
	case 0:
	  valid = 1;
	  break;
	case 0x4:
	  valid = (value & 0x2) == 0;
	  break;
	case 0x10:
	  valid = (value & 0x8) == 0;
	  break;
	case 0x14:
	  valid = value == 0x14;
	  break;
	}
      /* When disassembling with -Many, accept power4 encodings too.  */
      if (valid
	  || (dialect & PPC_OPCODE_ANY) == 0
	  || !extract)
	return valid;
    }

  /* Certain encodings have bits that are required to be zero.
     These are (z must be zero, a & t may be anything):
	 0000z
	 0001z
	 0100z
	 0101z
	 001at
	 011at
	 1a00t
	 1a01t
	 1z1zz
  */
  if ((value & 0x14) == 0)
    return (value & 0x1) == 0;
  else if ((value & 0x14) == 0x14)
    return value == 0x14;
  else
    return 1;
}

/* The BO field in a B form instruction.  Warn about attempts to set
   the field to an illegal value.  */

static unsigned long
insert_bo (unsigned long insn,
	   long value,
	   int dialect,
	   const char **errmsg)
{
  if (!valid_bo (value, dialect, 0))
    *errmsg = _("invalid conditional option");
  return insn | ((value & 0x1f) << 21);
}

/* The RA field in a D or X form instruction which is an updating
   store or an updating floating point load, which means that the RA
   field may not be zero.  */

static unsigned long
insert_ras (unsigned long insn,
	    long value,
	    int dialect ATTRIBUTE_UNUSED,
	    const char **errmsg)
{
  if (value == 0)
    *errmsg = _("invalid register operand when updating");
  return insn | ((value & 0x1f) << 16);
}

/* The RB field in an X form instruction when it must be the same as
   the RS field in the instruction.  This is used for extended
   mnemonics like mr.  This operand is marked FAKE.  The insertion
   function just copies the BT field into the BA field, and the
   extraction function just checks that the fields are the same.  */

static unsigned long
insert_rbs (unsigned long insn,
	    long value ATTRIBUTE_UNUSED,
	    int dialect ATTRIBUTE_UNUSED,
	    const char **errmsg ATTRIBUTE_UNUSED)
{
  return insn | (((insn >> 21) & 0x1f) << 11);
}


/* Macros used to form opcodes.  */

/* The main opcode.  */
#define OP(x) ((((unsigned long)(x)) & 0x3f) << 26)
#define OP_MASK OP (0x3f)

/* The main opcode combined with a trap code in the TO field of a D
   form instruction.  Used for extended mnemonics for the trap
   instructions.  */
#define OPTO(x,to) (OP (x) | ((((unsigned long)(to)) & 0x1f) << 21))
#define OPTO_MASK (OP_MASK | TO_MASK)

/* The main opcode combined with a comparison size bit in the L field
   of a D form or X form instruction.  Used for extended mnemonics for
   the comparison instructions.  */
#define OPL(x,l) (OP (x) | ((((unsigned long)(l)) & 1) << 21))
#define OPL_MASK OPL (0x3f,1)

/* An A form instruction.  */
#define A(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1) | (((unsigned long)(rc)) & 1))
#define A_MASK A (0x3f, 0x1f, 1)

/* An A_MASK with the FRB field fixed.  */
#define AFRB_MASK (A_MASK | FRB_MASK)

/* An A_MASK with the FRC field fixed.  */
#define AFRC_MASK (A_MASK | FRC_MASK)

/* An A_MASK with the FRA and FRC fields fixed.  */
#define AFRAFRC_MASK (A_MASK | FRA_MASK | FRC_MASK)

/* An AFRAFRC_MASK, but with L bit clear.  */
#define AFRALFRC_MASK (AFRAFRC_MASK & ~((unsigned long) 1 << 16))

/* A B form instruction.  */
#define B(op, aa, lk) (OP (op) | ((((unsigned long)(aa)) & 1) << 1) | ((lk) & 1))
#define B_MASK B (0x3f, 1, 1)

/* A B form instruction setting the BO field.  */
#define BBO(op, bo, aa, lk) (B ((op), (aa), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define BBO_MASK BBO (0x3f, 0x1f, 1, 1)

/* A BBO_MASK with the y bit of the BO field removed.  This permits
   matching a conditional branch regardless of the setting of the y
   bit.  Similarly for the 'at' bits used for power4 branch hints.  */
#define Y_MASK   (((unsigned long) 1) << 21)
#define AT1_MASK (((unsigned long) 3) << 21)
#define AT2_MASK (((unsigned long) 9) << 21)
#define BBOY_MASK  (BBO_MASK &~ Y_MASK)
#define BBOAT_MASK (BBO_MASK &~ AT1_MASK)

/* A B form instruction setting the BO field and the condition bits of
   the BI field.  */
#define BBOCB(op, bo, cb, aa, lk) \
  (BBO ((op), (bo), (aa), (lk)) | ((((unsigned long)(cb)) & 0x3) << 16))
#define BBOCB_MASK BBOCB (0x3f, 0x1f, 0x3, 1, 1)

/* A BBOCB_MASK with the y bit of the BO field removed.  */
#define BBOYCB_MASK (BBOCB_MASK &~ Y_MASK)
#define BBOATCB_MASK (BBOCB_MASK &~ AT1_MASK)
#define BBOAT2CB_MASK (BBOCB_MASK &~ AT2_MASK)

/* A BBOYCB_MASK in which the BI field is fixed.  */
#define BBOYBI_MASK (BBOYCB_MASK | BI_MASK)
#define BBOATBI_MASK (BBOAT2CB_MASK | BI_MASK)

/* An Context form instruction.  */
#define CTX(op, xop)   (OP (op) | (((unsigned long)(xop)) & 0x7))
#define CTX_MASK CTX(0x3f, 0x7)

/* An User Context form instruction.  */
#define UCTX(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define UCTX_MASK UCTX(0x3f, 0x1f)

/* The main opcode mask with the RA field clear.  */
#define DRA_MASK (OP_MASK | RA_MASK)

/* A DS form instruction.  */
#define DSO(op, xop) (OP (op) | ((xop) & 0x3))
#define DS_MASK DSO (0x3f, 3)

/* A DE form instruction.  */
#define DEO(op, xop) (OP (op) | ((xop) & 0xf))
#define DE_MASK DEO (0x3e, 0xf)

/* An EVSEL form instruction.  */
#define EVSEL(op, xop) (OP (op) | (((unsigned long)(xop)) & 0xff) << 3)
#define EVSEL_MASK EVSEL(0x3f, 0xff)

/* An M form instruction.  */
#define M(op, rc) (OP (op) | ((rc) & 1))
#define M_MASK M (0x3f, 1)

/* An M form instruction with the ME field specified.  */
#define MME(op, me, rc) (M ((op), (rc)) | ((((unsigned long)(me)) & 0x1f) << 1))

/* An M_MASK with the MB and ME fields fixed.  */
#define MMBME_MASK (M_MASK | MB_MASK | ME_MASK)

/* An M_MASK with the SH and ME fields fixed.  */
#define MSHME_MASK (M_MASK | SH_MASK | ME_MASK)

/* An MD form instruction.  */
#define MD(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x7) << 2) | ((rc) & 1))
#define MD_MASK MD (0x3f, 0x7, 1)

/* An MD_MASK with the MB field fixed.  */
#define MDMB_MASK (MD_MASK | MB6_MASK)

/* An MD_MASK with the SH field fixed.  */
#define MDSH_MASK (MD_MASK | SH6_MASK)

/* An MDS form instruction.  */
#define MDS(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0xf) << 1) | ((rc) & 1))
#define MDS_MASK MDS (0x3f, 0xf, 1)

/* An MDS_MASK with the MB field fixed.  */
#define MDSMB_MASK (MDS_MASK | MB6_MASK)

/* An SC form instruction.  */
#define SC(op, sa, lk) (OP (op) | ((((unsigned long)(sa)) & 1) << 1) | ((lk) & 1))
#define SC_MASK (OP_MASK | (((unsigned long)0x3ff) << 16) | (((unsigned long)1) << 1) | 1)

/* A VX form instruction.  */
#define VX(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x7ff))

/* The mask for a VX form instruction.  */
#define VX_MASK	VX(0x3f, 0x7ff)

/* A VA form instruction.  */
#define VXA(op, xop) (OP (op) | (((unsigned long)(xop)) & 0x03f))

/* The mask for a VA form instruction.  */
#define VXA_MASK VXA(0x3f, 0x3f)

/* A VXR form instruction.  */
#define VXR(op, xop, rc) (OP (op) | (((rc) & 1) << 10) | (((unsigned long)(xop)) & 0x3ff))

/* The mask for a VXR form instruction.  */
#define VXR_MASK VXR(0x3f, 0x3ff, 1)

/* An X form instruction.  */
#define X(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

/* A Z form instruction.  */
#define Z(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1))

/* An X form instruction with the RC bit specified.  */
#define XRC(op, xop, rc) (X ((op), (xop)) | ((rc) & 1))

/* A Z form instruction with the RC bit specified.  */
#define ZRC(op, xop, rc) (Z ((op), (xop)) | ((rc) & 1))

/* The mask for an X form instruction.  */
#define X_MASK XRC (0x3f, 0x3ff, 1)

/* The mask for a Z form instruction.  */
#define Z_MASK ZRC (0x3f, 0x1ff, 1)
#define Z2_MASK ZRC (0x3f, 0xff, 1)

/* An X_MASK with the RA field fixed.  */
#define XRA_MASK (X_MASK | RA_MASK)

/* An XRA_MASK with the W field clear.  */
#define XWRA_MASK (XRA_MASK & ~((unsigned long) 1 << 16))

/* An X_MASK with the RB field fixed.  */
#define XRB_MASK (X_MASK | RB_MASK)

/* An X_MASK with the RT field fixed.  */
#define XRT_MASK (X_MASK | RT_MASK)

/* An XRT_MASK mask with the L bits clear.  */
#define XLRT_MASK (XRT_MASK & ~((unsigned long) 0x3 << 21))

/* An X_MASK with the RA and RB fields fixed.  */
#define XRARB_MASK (X_MASK | RA_MASK | RB_MASK)

/* An XRARB_MASK, but with the L bit clear.  */
#define XRLARB_MASK (XRARB_MASK & ~((unsigned long) 1 << 16))

/* An X_MASK with the RT and RA fields fixed.  */
#define XRTRA_MASK (X_MASK | RT_MASK | RA_MASK)

/* An XRTRA_MASK, but with L bit clear.  */
#define XRTLRA_MASK (XRTRA_MASK & ~((unsigned long) 1 << 21))

/* An X form instruction with the L bit specified.  */
#define XOPL(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 1) << 21))

/* The mask for an X form comparison instruction.  */
#define XCMP_MASK (X_MASK | (((unsigned long)1) << 22))

/* The mask for an X form comparison instruction with the L field
   fixed.  */
#define XCMPL_MASK (XCMP_MASK | (((unsigned long)1) << 21))

/* An X form trap instruction with the TO field specified.  */
#define XTO(op, xop, to) (X ((op), (xop)) | ((((unsigned long)(to)) & 0x1f) << 21))
#define XTO_MASK (X_MASK | TO_MASK)

/* An X form tlb instruction with the SH field specified.  */
#define XTLB(op, xop, sh) (X ((op), (xop)) | ((((unsigned long)(sh)) & 0x1f) << 11))
#define XTLB_MASK (X_MASK | SH_MASK)

/* An X form sync instruction.  */
#define XSYNC(op, xop, l) (X ((op), (xop)) | ((((unsigned long)(l)) & 3) << 21))

/* An X form sync instruction with everything filled in except the LS field.  */
#define XSYNC_MASK (0xff9fffff)

/* An X_MASK, but with the EH bit clear.  */
#define XEH_MASK (X_MASK & ~((unsigned long )1))

/* An X form AltiVec dss instruction.  */
#define XDSS(op, xop, a) (X ((op), (xop)) | ((((unsigned long)(a)) & 1) << 25))
#define XDSS_MASK XDSS(0x3f, 0x3ff, 1)

/* An XFL form instruction.  */
#define XFL(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1) | (((unsigned long)(rc)) & 1))
#define XFL_MASK XFL (0x3f, 0x3ff, 1)

/* An X form isel instruction.  */
#define XISEL(op, xop)  (OP (op) | ((((unsigned long)(xop)) & 0x1f) << 1))
#define XISEL_MASK      XISEL(0x3f, 0x1f)

/* An XL form instruction with the LK field set to 0.  */
#define XL(op, xop) (OP (op) | ((((unsigned long)(xop)) & 0x3ff) << 1))

/* An XL form instruction which uses the LK field.  */
#define XLLK(op, xop, lk) (XL ((op), (xop)) | ((lk) & 1))

/* The mask for an XL form instruction.  */
#define XL_MASK XLLK (0x3f, 0x3ff, 1)

/* An XL form instruction which explicitly sets the BO field.  */
#define XLO(op, bo, xop, lk) \
  (XLLK ((op), (xop), (lk)) | ((((unsigned long)(bo)) & 0x1f) << 21))
#define XLO_MASK (XL_MASK | BO_MASK)

/* An XL form instruction which explicitly sets the y bit of the BO
   field.  */
#define XLYLK(op, xop, y, lk) (XLLK ((op), (xop), (lk)) | ((((unsigned long)(y)) & 1) << 21))
#define XLYLK_MASK (XL_MASK | Y_MASK)

/* An XL form instruction which sets the BO field and the condition
   bits of the BI field.  */
#define XLOCB(op, bo, cb, xop, lk) \
  (XLO ((op), (bo), (xop), (lk)) | ((((unsigned long)(cb)) & 3) << 16))
#define XLOCB_MASK XLOCB (0x3f, 0x1f, 0x3, 0x3ff, 1)

#define BB_MASK (0x1f << 11)
/* An XL_MASK or XLYLK_MASK or XLOCB_MASK with the BB field fixed.  */
#define XLBB_MASK (XL_MASK | BB_MASK)
#define XLYBB_MASK (XLYLK_MASK | BB_MASK)
#define XLBOCBBB_MASK (XLOCB_MASK | BB_MASK)

/* A mask for branch instructions using the BH field.  */
#define XLBH_MASK (XL_MASK | (0x1c << 11))

/* An XL_MASK with the BO and BB fields fixed.  */
#define XLBOBB_MASK (XL_MASK | BO_MASK | BB_MASK)

/* An XL_MASK with the BO, BI and BB fields fixed.  */
#define XLBOBIBB_MASK (XL_MASK | BO_MASK | BI_MASK | BB_MASK)

/* An XO form instruction.  */
#define XO(op, xop, oe, rc) \
  (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 1) | ((((unsigned long)(oe)) & 1) << 10) | (((unsigned long)(rc)) & 1))
#define XO_MASK XO (0x3f, 0x1ff, 1, 1)

/* An XO_MASK with the RB field fixed.  */
#define XORB_MASK (XO_MASK | RB_MASK)

/* An XS form instruction.  */
#define XS(op, xop, rc) (OP (op) | ((((unsigned long)(xop)) & 0x1ff) << 2) | (((unsigned long)(rc)) & 1))
#define XS_MASK XS (0x3f, 0x1ff, 1)

/* A mask for the FXM version of an XFX form instruction.  */
#define XFXFXM_MASK (X_MASK | (1 << 11) | (1 << 20))

/* An XFX form instruction with the FXM field filled in.  */
#define XFXM(op, xop, fxm, p4) \
  (X ((op), (xop)) | ((((unsigned long)(fxm)) & 0xff) << 12) \
   | ((unsigned long)(p4) << 20))

#define SPR_MASK (0x3ff << 11)
/* An XFX form instruction with the SPR field filled in.  */
#define XSPR(op, xop, spr) \
  (X ((op), (xop)) | ((((unsigned long)(spr)) & 0x1f) << 16) | ((((unsigned long)(spr)) & 0x3e0) << 6))
#define XSPR_MASK (X_MASK | SPR_MASK)

/* An XFX form instruction with the SPR field filled in except for the
   SPRBAT field.  */
#define XSPRBAT_MASK (XSPR_MASK &~ SPRBAT_MASK)

/* An XFX form instruction with the SPR field filled in except for the
   SPRG field.  */
#define XSPRG_MASK (XSPR_MASK & ~(0x1f << 16))

/* An X form instruction with everything filled in except the E field.  */
#define XE_MASK (0xffff7fff)

/* An X form user context instruction.  */
#define XUC(op, xop)  (OP (op) | (((unsigned long)(xop)) & 0x1f))
#define XUC_MASK      XUC(0x3f, 0x1f)

/* The BO encodings used in extended conditional branch mnemonics.  */
#define BODNZF	(0x0)
#define BODNZFP	(0x1)
#define BODZF	(0x2)
#define BODZFP	(0x3)
#define BODNZT	(0x8)
#define BODNZTP	(0x9)
#define BODZT	(0xa)
#define BODZTP	(0xb)

#define BOF	(0x4)
#define BOFP	(0x5)
#define BOFM4	(0x6)
#define BOFP4	(0x7)
#define BOT	(0xc)
#define BOTP	(0xd)
#define BOTM4	(0xe)
#define BOTP4	(0xf)

#define BODNZ	(0x10)
#define BODNZP	(0x11)
#define BODZ	(0x12)
#define BODZP	(0x13)
#define BODNZM4 (0x18)
#define BODNZP4 (0x19)
#define BODZM4	(0x1a)
#define BODZP4	(0x1b)

#define BOU	(0x14)

/* The BI condition bit encodings used in extended conditional branch
   mnemonics.  */
#define CBLT	(0)
#define CBGT	(1)
#define CBEQ	(2)
#define CBSO	(3)

/* The TO encodings used in extended trap mnemonics.  */
#define TOLGT	(0x1)
#define TOLLT	(0x2)
#define TOEQ	(0x4)
#define TOLGE	(0x5)
#define TOLNL	(0x5)
#define TOLLE	(0x6)
#define TOLNG	(0x6)
#define TOGT	(0x8)
#define TOGE	(0xc)
#define TONL	(0xc)
#define TOLT	(0x10)
#define TOLE	(0x14)
#define TONG	(0x14)
#define TONE	(0x18)
#define TOU	(0x1f)

/* Smaller names for the flags so each entry in the opcodes table will
   fit on a single line.  */
#undef	PPC
#define PPC     PPC_OPCODE_PPC
#define PPCCOM	PPC_OPCODE_PPC | PPC_OPCODE_COMMON
#define PPC64   PPC_OPCODE_64 | PPC_OPCODE_PPC
#define	COM     PPC_OPCODE_POWER | PPC_OPCODE_PPC | PPC_OPCODE_COMMON
#define	COM32   PPC_OPCODE_POWER | PPC_OPCODE_PPC | PPC_OPCODE_COMMON | PPC_OPCODE_32

/* The opcode table.

   The format of the opcode table is:

   NAME	     OPCODE	MASK		FLAGS		{ OPERANDS }

   NAME is the name of the instruction.
   OPCODE is the instruction opcode.
   MASK is the opcode mask; this is used to tell the disassembler
     which bits in the actual opcode must match OPCODE.
   FLAGS are flags indicated what processors support the instruction.
   OPERANDS is the list of operands.

   The disassembler reads the table in order and prints the first
   instruction which matches, so this table is sorted to put more
   specific instructions before more general instructions.  It is also
   sorted by major opcode.  */

static const struct powerpc_opcode powerpc_opcodes[] = {

{ "cmplwi",  OPL(10,0),	OPL_MASK,	PPCCOM,		{ OBF, RA, UI } },
{ "cmpwi",   OPL(11,0),	OPL_MASK,	PPCCOM,		{ OBF, RA, SI } },
{ "cmpw",    XOPL(31,0,0), XCMPL_MASK,	PPCCOM,		{ OBF, RA, RB } },
{ "cmplw",   XOPL(31,32,0), XCMPL_MASK, PPCCOM,	{ OBF, RA, RB } },
{ "fcmpu",   X(63,0),	X_MASK|(3<<21),	COM,		{ BF, FRA, FRB } },

{ "li",	     OP(14),	DRA_MASK,	PPCCOM,		{ RT, SI } },
{ "lis",     OP(15),	DRA_MASK,	PPCCOM,		{ RT, SI } },

{ "addi",    OP(14),	OP_MASK,	PPCCOM,		{ RT, RA0, SI } },
{ "addis",   OP(15),	OP_MASK,	PPCCOM,		{ RT,RA0,SI } },
{ "blt-",    BBOCB(16,BOT,CBLT,0,0), BBOATCB_MASK, PPCCOM,	{ CR, BDM } },
{ "bc",	     B(16,0,0),	B_MASK,		COM,		{ BO, BI, BD } },
{ "bcl",     B(16,0,1),	B_MASK,		COM,		{ BO, BI, BD } },
{ "b",	     B(18,0,0),	B_MASK,		COM,		{ LI } },
{ "bl",      B(18,0,1),	B_MASK,		COM,		{ LI } },
{ "blr",     XLO(19,BOU,16,0), XLBOBIBB_MASK, PPCCOM,	{ 0 } },
{ "bctr",    XLO(19,BOU,528,0), XLBOBIBB_MASK, COM,	{ 0 } },
{ "bctrl",   XLO(19,BOU,528,1), XLBOBIBB_MASK, COM,	{ 0 } },

{ "rlwinm",  M(21,0),	M_MASK,		PPCCOM,		{ RA,RS,SH,MBE,ME } },
{ "nop",     OP(24),	0xffffffff,	PPCCOM,		{ 0 } },
{ "ori",     OP(24),	OP_MASK,	PPCCOM,		{ RA, RS, UI } },
{ "xoris",   OP(27),	OP_MASK,	PPCCOM,		{ RA, RS, UI } },
{ "ldx",     X(31,21),	X_MASK,		PPC64,		{ RT, RA0, RB } },
{ "lwzx",    X(31,23),	X_MASK,		PPCCOM,		{ RT, RA0, RB } },
{ "slw",     XRC(31,24,0), X_MASK,	PPCCOM,		{ RA, RS, RB } },
{ "and",     XRC(31,28,0), X_MASK,	COM,		{ RA, RS, RB } },
{ "sub",     XO(31,40,0,0), XO_MASK,	PPC,		{ RT, RB, RA } },
{ "lbzx",    X(31,87),	X_MASK,		COM,		{ RT, RA0, RB } },
{ "neg",     XO(31,104,0,0), XORB_MASK,	COM,		{ RT, RA } },
{ "not",     XRC(31,124,0), X_MASK,	COM,		{ RA, RS, RBS } },
{ "stwx",    X(31,151), X_MASK,		PPCCOM,		{ RS, RA0, RB } },
{ "stbx",    X(31,215),	X_MASK,		COM,		{ RS, RA0, RB } },
{ "mullw",   XO(31,235,0,0), XO_MASK,	PPCCOM,		{ RT, RA, RB } },
{ "add",     XO(31,266,0,0), XO_MASK,	PPCCOM,		{ RT, RA, RB } },
{ "lhzx",    X(31,279),	X_MASK,		COM,		{ RT, RA0, RB } },
{ "xor",     XRC(31,316,0), X_MASK,	COM,		{ RA, RS, RB } },
{ "mflr",    XSPR(31,339,8), XSPR_MASK, COM,		{ RT } },
{ "sthx",    X(31,407),	X_MASK,		COM,		{ RS, RA0, RB } },
{ "mr",	     XRC(31,444,0), X_MASK,	COM,		{ RA, RS, RBS } },
{ "or",      XRC(31,444,0), X_MASK,	COM,		{ RA, RS, RB } },
{ "divwu",   XO(31,459,0,0), XO_MASK,	PPC,		{ RT, RA, RB } },
{ "mtlr",    XSPR(31,467,8), XSPR_MASK, COM,		{ RS } },
{ "mtctr",   XSPR(31,467,9), XSPR_MASK, COM,		{ RS } },
{ "divw",    XO(31,491,0,0), XO_MASK,	PPC,		{ RT, RA, RB } },
{ "lfsx",    X(31,535),	X_MASK,		COM,		{ FRT, RA0, RB } },
{ "srw",     XRC(31,536,0), X_MASK,	PPCCOM,		{ RA, RS, RB } },
{ "stfsx",   X(31,663), X_MASK,		COM,		{ FRS, RA0, RB } },
{ "sraw",    XRC(31,792,0), X_MASK,	PPCCOM,		{ RA, RS, RB } },
{ "extsh",   XRC(31,922,0), XRB_MASK,	PPCCOM,		{ RA, RS } },
{ "extsb",   XRC(31,954,0), XRB_MASK,	PPC,		{ RA, RS} },

{ "lwz",     OP(32),	OP_MASK,	PPCCOM,		{ RT, D, RA0 } },
{ "lbz",     OP(34),	OP_MASK,	COM,		{ RT, D, RA0 } },
{ "stw",     OP(36),	OP_MASK,	PPCCOM,		{ RS, D, RA0 } },
{ "stwu",    OP(37),	OP_MASK,	PPCCOM,		{ RS, D, RAS } },
{ "stb",     OP(38),	OP_MASK,	COM,		{ RS, D, RA0 } },
{ "lhz",     OP(40),	OP_MASK,	COM,		{ RT, D, RA0 } },
{ "sth",     OP(44),	OP_MASK,	COM,		{ RS, D, RA0 } },
{ "lfs",     OP(48),	OP_MASK,	COM,		{ FRT, D, RA0 } },
{ "lfd",     OP(50),	OP_MASK,	COM,		{ FRT, D, RA0 } },
{ "stfs",    OP(52),	OP_MASK,	COM,		{ FRS, D, RA0 } },
{ "stfd",    OP(54),	OP_MASK,	COM,		{ FRS, D, RA0 } },
{ "ld",      DSO(58,0),	DS_MASK,	PPC64,		{ RT, DS, RA0 } },

{ "fdivs",   A(59,18,0), AFRC_MASK,	PPC,		{ FRT, FRA, FRB } },
{ "fsubs",   A(59,20,0), AFRC_MASK,	PPC,		{ FRT, FRA, FRB } },
{ "fadds",   A(59,21,0), AFRC_MASK,	PPC,		{ FRT, FRA, FRB } },
{ "fmuls",   A(59,25,0), AFRB_MASK,	PPC,		{ FRT, FRA, FRC } },
{ "std",     DSO(62,0),	DS_MASK,	PPC64,		{ RS, DS, RA0 } },
{ "stdu",    DSO(62,1),	DS_MASK,	PPC64,		{ RS, DS, RAS } },
{ "frsp",    XRC(63,12,0), XRA_MASK,	COM,		{ FRT, FRB } },
{ "fctiwz",  XRC(63,15,0), XRA_MASK,	PPCCOM,		{ FRT, FRB } },
{ "fsub",    A(63,20,0), AFRC_MASK,	PPCCOM,		{ FRT, FRA, FRB } },
{ "fneg",    XRC(63,40,0), XRA_MASK,	COM,		{ FRT, FRB } },
};
