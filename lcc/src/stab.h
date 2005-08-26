/*      @(#)stab.h 1.11 92/05/11 SMI      */
/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * This file gives definitions supplementing <a.out.h>
 * for permanent symbol table entries.
 * These must have one of the N_STAB bits on,
 * and are subject to relocation according to the masks in <a.out.h>.
 */

#ifndef _STAB_H
#define _STAB_H


#if !defined(_a_out_h) && !defined(_A_OUT_H)
/* this file contains fragments of a.out.h and stab.h relevant to
 * support of stabX processing within ELF files - see the
 * Format of a symbol table entry
 */
struct  nlist {
        union {
                char    *n_name;     /* for use when in-core */
                long    n_strx;      /* index into file string table */
        } n_un;
        unsigned char   n_type;      /* type flag (N_TEXT,..)  */
        char    n_other;             /* unused */
        short   n_desc;              /* see <stab.h> */
        unsigned long   n_value;     /* value of symbol (or sdb offset) */ 
};
 
/*
 * Simple values for n_type.
 */
#define N_UNDF  0x0             /* undefined */
#define N_ABS   0x2             /* absolute */
#define N_TEXT  0x4             /* text */
#define N_DATA  0x6             /* data */
#define N_BSS   0x8             /* bss */
#define N_COMM  0x12            /* common (internal to ld) */
#define N_FN    0x1f            /* file name symbol */
 
#define N_EXT   01              /* external bit, or'ed in */
#define N_TYPE  0x1e            /* mask for all the type bits */

#endif

/*
 * for symbolic debugger, sdb(1):
 */
#define	N_GSYM	0x20		/* global symbol: name,,0,type,0 */
#define	N_FNAME	0x22		/* procedure name (f77 kludge): name,,0 */
#define	N_FUN	0x24		/* procedure: name,,0,linenumber,address */
#define	N_STSYM	0x26		/* static symbol: name,,0,type,address */
#define	N_LCSYM	0x28		/* .lcomm symbol: name,,0,type,address */
#define N_MAIN  0x2a            /* name of main routine : name,,0,0,0 */
#define N_ROSYM 0x2c		/* ro_data objects */
#define N_OBJ	0x38		/* object file path or name */
#define N_OPT	0x3c		/* compiler options */
#define	N_RSYM	0x40		/* register sym: name,,0,type,register */
#define	N_SLINE	0x44		/* src line: 0,,0,linenumber,address */
#define	N_FLINE	0x4c		/* function start.end */
#define	N_SSYM	0x60		/* structure elt: name,,0,type,struct_offset */
#define N_ENDM	0x62		/* last stab emitted for module */
#define	N_SO	0x64		/* source file name: name,,0,0,address */
#define	N_LSYM	0x80		/* local sym: name,,0,type,offset */
#define	N_BINCL 0x82		/* header file: name,,0,0,0 */
#define	N_SOL	0x84		/* #included file name: name,,0,0,address */
#define	N_PSYM	0xa0		/* parameter: name,,0,type,offset */
#define N_EINCL 0xa2		/* end of include file */
#define	N_ENTRY	0xa4		/* alternate entry: name,linenumber,address */
#define	N_LBRAC	0xc0		/* left bracket: 0,,0,nesting level,address */
#define	N_EXCL	0xc2		/* excluded include file */
#define	N_RBRAC	0xe0		/* right bracket: 0,,0,nesting level,address */
#define	N_BCOMM	0xe2		/* begin common: name,, */
#define	N_ECOMM	0xe4		/* end common: name,, */
#define	N_ECOML	0xe8		/* end common (local name): ,,address */
#define	N_LENG	0xfe		/* second stab entry with length information */

/*
 * for the berkeley pascal compiler, pc(1):
 */
#define	N_PC	0x30		/* global pascal symbol: name,,0,subtype,line */
#define	N_WITH	0xea		/* pascal with statement: type,,0,0,offset */

/*
 * for code browser only
 */
#define N_BROWS	0x48		/* path to associated .cb file */

/*
 * Optional langauge designations for N_SO
 */
#define N_SO_AS         1       /* Assembler    */
#define N_SO_C          2       /* C            */
#define N_SO_ANSI_C     3       /* ANSI C       */
#define N_SO_CC         4       /* C++          */
#define N_SO_FORTRAN    5       /* Fortran 77   */
#define N_SO_PASCAL     6       /* Pascal       */

/*
 * Floating point type values
 */
#define NF_NONE		0	/* Undefined type 	*/
#define NF_SINGLE	1	/* IEEE 32 bit float	*/
#define NF_DOUBLE	2	/* IEEE 64 bit float	*/
#define NF_COMPLEX	3	/* Fortran complex 	*/
#define NF_COMPLEX16	4	/* Fortran double complex */
#define NF_COMPLEX32	5	/* Fortran complex*16	*/
#define NF_LDOUBLE 	6	/* Long double		*/

#endif
