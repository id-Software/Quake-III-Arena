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
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#include "q_shared.h"
#include "qcommon.h"

// don't change, this is hardcoded into x86 VMs, opStack protection relies
// on this
#define	OPSTACK_SIZE	1024
#define	OPSTACK_MASK	(OPSTACK_SIZE-1)

// don't change
// Hardcoded in q3asm an reserved at end of bss
#define	PROGRAM_STACK_SIZE	0x10000
#define	PROGRAM_STACK_MASK	(PROGRAM_STACK_SIZE-1)

typedef enum {
	OP_UNDEF, 

	OP_IGNORE, 

	OP_BREAK,

	OP_ENTER,
	OP_LEAVE,
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,
	OP_LOCAL,

	OP_JUMP,

	//-------------------

	OP_EQ,
	OP_NE,

	OP_LTI,
	OP_LEI,
	OP_GTI,
	OP_GEI,

	OP_LTU,
	OP_LEU,
	OP_GTU,
	OP_GEU,

	OP_EQF,
	OP_NEF,

	OP_LTF,
	OP_LEF,
	OP_GTF,
	OP_GEF,

	//-------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,				// *(stack[top-1]) = stack[top]
	OP_ARG,

	OP_BLOCK_COPY,

	//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
} opcode_t;



typedef int	vmptr_t;

typedef struct vmSymbol_s {
	struct vmSymbol_s	*next;
	int		symValue;
	int		profileCount;
	char	symName[1];		// variable sized
} vmSymbol_t;

#define	VM_OFFSET_PROGRAM_STACK		0
#define	VM_OFFSET_SYSTEM_CALL		4

struct vm_s {
    // DO NOT MOVE OR CHANGE THESE WITHOUT CHANGING THE VM_OFFSET_* DEFINES
    // USED BY THE ASM CODE
    int			programStack;		// the vm may be recursively entered
    intptr_t			(*systemCall)( intptr_t *parms );

	//------------------------------------
   
	char		name[MAX_QPATH];
	void	*searchPath;				// hint for FS_ReadFileDir()

	// for dynamic linked modules
	void		*dllHandle;
	intptr_t			(QDECL *entryPoint)( int callNum, ... );
	void (*destroy)(vm_t* self);

	// for interpreted modules
	qboolean	currentlyInterpreting;

	qboolean	compiled;
	byte		*codeBase;
	int			entryOfs;
	int			codeLength;

	intptr_t	*instructionPointers;
	int			instructionCount;

	byte		*dataBase;
	int			dataMask;

	int			stackBottom;		// if programStack < stackBottom, error

	int			numSymbols;
	struct vmSymbol_s	*symbols;

	int			callLevel;		// counts recursive VM_Call
	int			breakFunction;		// increment breakCount on function entry to this
	int			breakCount;

	byte		*jumpTableTargets;
	int			numJumpTableTargets;
};


extern	vm_t	*currentVM;
extern	int		vm_debugLevel;

void VM_Compile( vm_t *vm, vmHeader_t *header );
int	VM_CallCompiled( vm_t *vm, int *args );

void VM_PrepareInterpreter( vm_t *vm, vmHeader_t *header );
int	VM_CallInterpreted( vm_t *vm, int *args );

vmSymbol_t *VM_ValueToFunctionSymbol( vm_t *vm, int value );
int VM_SymbolToValue( vm_t *vm, const char *symbol );
const char *VM_ValueToSymbol( vm_t *vm, int value );
void VM_LogSyscalls( int *args );

void VM_BlockCopy(unsigned int dest, unsigned int src, size_t n);
