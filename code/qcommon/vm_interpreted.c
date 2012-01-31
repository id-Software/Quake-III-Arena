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
#include "vm_local.h"

#ifdef DEBUG_VM // bk001204
static char	*opnames[256] = {
	"OP_UNDEF", 

	"OP_IGNORE", 

	"OP_BREAK",

	"OP_ENTER",
	"OP_LEAVE",
	"OP_CALL",
	"OP_PUSH",
	"OP_POP",

	"OP_CONST",

	"OP_LOCAL",

	"OP_JUMP",

	//-------------------

	"OP_EQ",
	"OP_NE",

	"OP_LTI",
	"OP_LEI",
	"OP_GTI",
	"OP_GEI",

	"OP_LTU",
	"OP_LEU",
	"OP_GTU",
	"OP_GEU",

	"OP_EQF",
	"OP_NEF",

	"OP_LTF",
	"OP_LEF",
	"OP_GTF",
	"OP_GEF",

	//-------------------

	"OP_LOAD1",
	"OP_LOAD2",
	"OP_LOAD4",
	"OP_STORE1",
	"OP_STORE2",
	"OP_STORE4",
	"OP_ARG",

	"OP_BLOCK_COPY",

	//-------------------

	"OP_SEX8",
	"OP_SEX16",

	"OP_NEGI",
	"OP_ADD",
	"OP_SUB",
	"OP_DIVI",
	"OP_DIVU",
	"OP_MODI",
	"OP_MODU",
	"OP_MULI",
	"OP_MULU",

	"OP_BAND",
	"OP_BOR",
	"OP_BXOR",
	"OP_BCOM",

	"OP_LSH",
	"OP_RSHI",
	"OP_RSHU",

	"OP_NEGF",
	"OP_ADDF",
	"OP_SUBF",
	"OP_DIVF",
	"OP_MULF",

	"OP_CVIF",
	"OP_CVFI"
};
#endif

#if idppc
    #if defined(__GNUC__)
        static inline unsigned int loadWord(void *addr) {
            unsigned int word;
            
            asm("lwbrx %0,0,%1" : "=r" (word) : "r" (addr));
            return word;
        }
    #else
	#define loadWord(addr) __lwbrx(addr,0)
    #endif
#else
	#define	loadWord(addr) *((int *)addr)
#endif

char *VM_Indent( vm_t *vm ) {
	static char	*string = "                                        ";
	if ( vm->callLevel > 20 ) {
		return string;
	}
	return string + 2 * ( 20 - vm->callLevel );
}

void VM_StackTrace( vm_t *vm, int programCounter, int programStack ) {
	int		count;

	count = 0;
	do {
		Com_Printf( "%s\n", VM_ValueToSymbol( vm, programCounter ) );
		programStack =  *(int *)&vm->dataBase[programStack+4];
		programCounter = *(int *)&vm->dataBase[programStack];
	} while ( programCounter != -1 && ++count < 32 );

}


/*
====================
VM_PrepareInterpreter
====================
*/
void VM_PrepareInterpreter( vm_t *vm, vmHeader_t *header ) {
	int		op;
	int		pc;
	byte	*code;
	int		instruction;
	int		*codeBase;

	vm->codeBase = Hunk_Alloc( vm->codeLength*4, h_high );			// we're now int aligned
//	memcpy( vm->codeBase, (byte *)header + header->codeOffset, vm->codeLength );

	// we don't need to translate the instructions, but we still need
	// to find each instructions starting point for jumps
	pc = 0;
	instruction = 0;
	code = (byte *)header + header->codeOffset;
	codeBase = (int *)vm->codeBase;

	while ( instruction < header->instructionCount ) {
		vm->instructionPointers[ instruction ] = pc;
		instruction++;

		op = code[ pc ];
		codeBase[pc] = op;
		if ( pc > header->codeLength ) {
			Com_Error( ERR_FATAL, "VM_PrepareInterpreter: pc > header->codeLength" );
		}

		pc++;

		// these are the only opcodes that aren't a single byte
		switch ( op ) {
		case OP_ENTER:
		case OP_CONST:
		case OP_LOCAL:
		case OP_LEAVE:
		case OP_EQ:
		case OP_NE:
		case OP_LTI:
		case OP_LEI:
		case OP_GTI:
		case OP_GEI:
		case OP_LTU:
		case OP_LEU:
		case OP_GTU:
		case OP_GEU:
		case OP_EQF:
		case OP_NEF:
		case OP_LTF:
		case OP_LEF:
		case OP_GTF:
		case OP_GEF:
		case OP_BLOCK_COPY:
			codeBase[pc+0] = loadWord(&code[pc]);
			pc += 4;
			break;
		case OP_ARG:
			codeBase[pc+0] = code[pc];
			pc += 1;
			break;
		default:
			break;
		}

	}
	pc = 0;
	instruction = 0;
	code = (byte *)header + header->codeOffset;
	codeBase = (int *)vm->codeBase;

	while ( instruction < header->instructionCount ) {
		op = code[ pc ];
		instruction++;
		pc++;
		switch ( op ) {
		case OP_ENTER:
		case OP_CONST:
		case OP_LOCAL:
		case OP_LEAVE:
		case OP_EQ:
		case OP_NE:
		case OP_LTI:
		case OP_LEI:
		case OP_GTI:
		case OP_GEI:
		case OP_LTU:
		case OP_LEU:
		case OP_GTU:
		case OP_GEU:
		case OP_EQF:
		case OP_NEF:
		case OP_LTF:
		case OP_LEF:
		case OP_GTF:
		case OP_GEF:
		case OP_BLOCK_COPY:
			switch(op) {
				case OP_EQ:
				case OP_NE:
				case OP_LTI:
				case OP_LEI:
				case OP_GTI:
				case OP_GEI:
				case OP_LTU:
				case OP_LEU:
				case OP_GTU:
				case OP_GEU:
				case OP_EQF:
				case OP_NEF:
				case OP_LTF:
				case OP_LEF:
				case OP_GTF:
				case OP_GEF:
				codeBase[pc] = vm->instructionPointers[codeBase[pc]];
				break;
			default:
				break;
			}
			pc += 4;
			break;
		case OP_ARG:
			pc += 1;
			break;
		default:
			break;
		}

	}
}

/*
==============
VM_Call


Upon a system call, the stack will look like:

sp+32	parm1
sp+28	parm0
sp+24	return stack
sp+20	return address
sp+16	local1
sp+14	local0
sp+12	arg1
sp+8	arg0
sp+4	return stack
sp		return address

An interpreted function will immediately execute
an OP_ENTER instruction, which will subtract space for
locals from sp
==============
*/
#define	MAX_STACK	256
#define	STACK_MASK	(MAX_STACK-1)
//#define	DEBUG_VM

#define	DEBUGSTR va("%s%i", VM_Indent(vm), opStack-stack )

int	VM_CallInterpreted( vm_t *vm, int *args ) {
	int		stack[MAX_STACK];
	int		*opStack;
	int		programCounter;
	int		programStack;
	int		stackOnEntry;
	byte	*image;
	int		*codeImage;
	int		v1;
	int		dataMask;
#ifdef DEBUG_VM
	vmSymbol_t	*profileSymbol;
#endif

	// interpret the code
	vm->currentlyInterpreting = qtrue;

	// we might be called recursively, so this might not be the very top
	programStack = stackOnEntry = vm->programStack;

#ifdef DEBUG_VM
	profileSymbol = VM_ValueToFunctionSymbol( vm, 0 );
	// uncomment this for debugging breakpoints
	vm->breakFunction = 0;
#endif
	// set up the stack frame 

	image = vm->dataBase;
	codeImage = (int *)vm->codeBase;
	dataMask = vm->dataMask;
	
	// leave a free spot at start of stack so
	// that as long as opStack is valid, opStack-1 will
	// not corrupt anything
	opStack = stack;
	programCounter = 0;

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

	vm->callLevel = 0;
	
	VM_Debug(0);

//	vm_debugLevel=2;
	// main interpreter loop, will exit when a LEAVE instruction
	// grabs the -1 program counter

#define r2 codeImage[programCounter]

	while ( 1 ) {
		int		opcode,	r0, r1;
//		unsigned int	r2;

nextInstruction:
		r0 = ((int *)opStack)[0];
		r1 = ((int *)opStack)[-1];
nextInstruction2:
		opcode = codeImage[ programCounter++ ];
#ifdef DEBUG_VM
		if ( (unsigned)programCounter > vm->codeLength ) {
			Com_Error( ERR_DROP, "VM pc out of range" );
		}

		if ( opStack < stack ) {
			Com_Error( ERR_DROP, "VM opStack underflow" );
		}
		if ( opStack >= stack+MAX_STACK ) {
			Com_Error( ERR_DROP, "VM opStack overflow" );
		}

		if ( programStack <= vm->stackBottom ) {
			Com_Error( ERR_DROP, "VM stack overflow" );
		}

		if ( programStack & 3 ) {
			Com_Error( ERR_DROP, "VM program stack misaligned" );
		}

		if ( vm_debugLevel > 1 ) {
			Com_Printf( "%s %s\n", DEBUGSTR, opnames[opcode] );
		}
		profileSymbol->profileCount++;
#endif

		switch ( opcode ) {
#ifdef DEBUG_VM
		default:
			Com_Error( ERR_DROP, "Bad VM instruction" );  // this should be scanned on load!
#endif
		case OP_BREAK:
			vm->breakCount++;
			goto nextInstruction2;
		case OP_CONST:
			opStack++;
			r1 = r0;
			r0 = *opStack = r2;
			
			programCounter += 4;
			goto nextInstruction2;
		case OP_LOCAL:
			opStack++;
			r1 = r0;
			r0 = *opStack = r2+programStack;

			programCounter += 4;
			goto nextInstruction2;

		case OP_LOAD4:
#ifdef DEBUG_VM
			if ( *opStack & 3 ) {
				Com_Error( ERR_DROP, "OP_LOAD4 misaligned" );
			}
#endif
			r0 = *opStack = *(int *)&image[ r0&dataMask ];
			goto nextInstruction2;
		case OP_LOAD2:
			r0 = *opStack = *(unsigned short *)&image[ r0&dataMask ];
			goto nextInstruction2;
		case OP_LOAD1:
			r0 = *opStack = image[ r0&dataMask ];
			goto nextInstruction2;

		case OP_STORE4:
			*(int *)&image[ r1&(dataMask & ~3) ] = r0;
			opStack -= 2;
			goto nextInstruction;
		case OP_STORE2:
			*(short *)&image[ r1&(dataMask & ~1) ] = r0;
			opStack -= 2;
			goto nextInstruction;
		case OP_STORE1:
			image[ r1&dataMask ] = r0;
			opStack -= 2;
			goto nextInstruction;

		case OP_ARG:
			// single byte offset from programStack
			*(int *)&image[ codeImage[programCounter] + programStack ] = r0;
			opStack--;
			programCounter += 1;
			goto nextInstruction;

		case OP_BLOCK_COPY:
			{
				int		*src, *dest;
				int		i, count, srci, desti;

				count = r2;
				// MrE: copy range check
				srci = r0 & dataMask;
				desti = r1 & dataMask;
				count = ((srci + count) & dataMask) - srci;
				count = ((desti + count) & dataMask) - desti;

				src = (int *)&image[ r0&dataMask ];
				dest = (int *)&image[ r1&dataMask ];
				if ( ( (int)src | (int)dest | count ) & 3 ) {
					Com_Error( ERR_DROP, "OP_BLOCK_COPY not dword aligned" );
				}
				count >>= 2;
				for ( i = count-1 ; i>= 0 ; i-- ) {
					dest[i] = src[i];
				}
				programCounter += 4;
				opStack -= 2;
			}
			goto nextInstruction;

		case OP_CALL:
			// save current program counter
			*(int *)&image[ programStack ] = programCounter;
			
			// jump to the location on the stack
			programCounter = r0;
			opStack--;
			if ( programCounter < 0 ) {
				// system call
				int		r;
				int		temp;
#ifdef DEBUG_VM
				int		stomped;

				if ( vm_debugLevel ) {
					Com_Printf( "%s---> systemcall(%i)\n", DEBUGSTR, -1 - programCounter );
				}
#endif
				// save the stack to allow recursive VM entry
				temp = vm->callLevel;
				vm->programStack = programStack - 4;
#ifdef DEBUG_VM
				stomped = *(int *)&image[ programStack + 4 ];
#endif
				*(int *)&image[ programStack + 4 ] = -1 - programCounter;

//VM_LogSyscalls( (int *)&image[ programStack + 4 ] );
				r = vm->systemCall( (int *)&image[ programStack + 4 ] );

#ifdef DEBUG_VM
				// this is just our stack frame pointer, only needed
				// for debugging
				*(int *)&image[ programStack + 4 ] = stomped;
#endif

				// save return value
				opStack++;
				*opStack = r;
				programCounter = *(int *)&image[ programStack ];
				vm->callLevel = temp;
#ifdef DEBUG_VM
				if ( vm_debugLevel ) {
					Com_Printf( "%s<--- %s\n", DEBUGSTR, VM_ValueToSymbol( vm, programCounter ) );
				}
#endif
			} else {
				programCounter = vm->instructionPointers[ programCounter ];
			}
			goto nextInstruction;

		// push and pop are only needed for discarded or bad function return values
		case OP_PUSH:
			opStack++;
			goto nextInstruction;
		case OP_POP:
			opStack--;
			goto nextInstruction;

		case OP_ENTER:
#ifdef DEBUG_VM
			profileSymbol = VM_ValueToFunctionSymbol( vm, programCounter );
#endif
			// get size of stack frame
			v1 = r2;

			programCounter += 4;
			programStack -= v1;
#ifdef DEBUG_VM
			// save old stack frame for debugging traces
			*(int *)&image[programStack+4] = programStack + v1;
			if ( vm_debugLevel ) {
				Com_Printf( "%s---> %s\n", DEBUGSTR, VM_ValueToSymbol( vm, programCounter - 5 ) );
				if ( vm->breakFunction && programCounter - 5 == vm->breakFunction ) {
					// this is to allow setting breakpoints here in the debugger
					vm->breakCount++;
//					vm_debugLevel = 2;
//					VM_StackTrace( vm, programCounter, programStack );
				}
				vm->callLevel++;
			}
#endif
			goto nextInstruction;
		case OP_LEAVE:
			// remove our stack frame
			v1 = r2;

			programStack += v1;

			// grab the saved program counter
			programCounter = *(int *)&image[ programStack ];
#ifdef DEBUG_VM
			profileSymbol = VM_ValueToFunctionSymbol( vm, programCounter );
			if ( vm_debugLevel ) {
				vm->callLevel--;
				Com_Printf( "%s<--- %s\n", DEBUGSTR, VM_ValueToSymbol( vm, programCounter ) );
			}
#endif
			// check for leaving the VM
			if ( programCounter == -1 ) {
				goto done;
			}
			goto nextInstruction;

		/*
		===================================================================
		BRANCHES
		===================================================================
		*/

		case OP_JUMP:
			programCounter = r0;
			programCounter = vm->instructionPointers[ programCounter ];
			opStack--;
			goto nextInstruction;

		case OP_EQ:
			opStack -= 2;
			if ( r1 == r0 ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_NE:
			opStack -= 2;
			if ( r1 != r0 ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_LTI:
			opStack -= 2;
			if ( r1 < r0 ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_LEI:
			opStack -= 2;
			if ( r1 <= r0 ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_GTI:
			opStack -= 2;
			if ( r1 > r0 ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_GEI:
			opStack -= 2;
			if ( r1 >= r0 ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_LTU:
			opStack -= 2;
			if ( ((unsigned)r1) < ((unsigned)r0) ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_LEU:
			opStack -= 2;
			if ( ((unsigned)r1) <= ((unsigned)r0) ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_GTU:
			opStack -= 2;
			if ( ((unsigned)r1) > ((unsigned)r0) ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_GEU:
			opStack -= 2;
			if ( ((unsigned)r1) >= ((unsigned)r0) ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				goto nextInstruction;
			} else {
				programCounter += 4;
				goto nextInstruction;
			}

		case OP_EQF:
			if ( ((float *)opStack)[-1] == *(float *)opStack ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				opStack -= 2;
				goto nextInstruction;
			} else {
				programCounter += 4;
				opStack -= 2;
				goto nextInstruction;
			}

		case OP_NEF:
			if ( ((float *)opStack)[-1] != *(float *)opStack ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				opStack -= 2;
				goto nextInstruction;
			} else {
				programCounter += 4;
				opStack -= 2;
				goto nextInstruction;
			}

		case OP_LTF:
			if ( ((float *)opStack)[-1] < *(float *)opStack ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				opStack -= 2;
				goto nextInstruction;
			} else {
				programCounter += 4;
				opStack -= 2;
				goto nextInstruction;
			}

		case OP_LEF:
			if ( ((float *)opStack)[-1] <= *(float *)opStack ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				opStack -= 2;
				goto nextInstruction;
			} else {
				programCounter += 4;
				opStack -= 2;
				goto nextInstruction;
			}

		case OP_GTF:
			if ( ((float *)opStack)[-1] > *(float *)opStack ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				opStack -= 2;
				goto nextInstruction;
			} else {
				programCounter += 4;
				opStack -= 2;
				goto nextInstruction;
			}

		case OP_GEF:
			if ( ((float *)opStack)[-1] >= *(float *)opStack ) {
				programCounter = r2;	//vm->instructionPointers[r2];
				opStack -= 2;
				goto nextInstruction;
			} else {
				programCounter += 4;
				opStack -= 2;
				goto nextInstruction;
			}


		//===================================================================

		case OP_NEGI:
			*opStack = -r0;
			goto nextInstruction;
		case OP_ADD:
			opStack[-1] = r1 + r0;
			opStack--;
			goto nextInstruction;
		case OP_SUB:
			opStack[-1] = r1 - r0;
			opStack--;
			goto nextInstruction;
		case OP_DIVI:
			opStack[-1] = r1 / r0;
			opStack--;
			goto nextInstruction;
		case OP_DIVU:
			opStack[-1] = ((unsigned)r1) / ((unsigned)r0);
			opStack--;
			goto nextInstruction;
		case OP_MODI:
			opStack[-1] = r1 % r0;
			opStack--;
			goto nextInstruction;
		case OP_MODU:
			opStack[-1] = ((unsigned)r1) % (unsigned)r0;
			opStack--;
			goto nextInstruction;
		case OP_MULI:
			opStack[-1] = r1 * r0;
			opStack--;
			goto nextInstruction;
		case OP_MULU:
			opStack[-1] = ((unsigned)r1) * ((unsigned)r0);
			opStack--;
			goto nextInstruction;

		case OP_BAND:
			opStack[-1] = ((unsigned)r1) & ((unsigned)r0);
			opStack--;
			goto nextInstruction;
		case OP_BOR:
			opStack[-1] = ((unsigned)r1) | ((unsigned)r0);
			opStack--;
			goto nextInstruction;
		case OP_BXOR:
			opStack[-1] = ((unsigned)r1) ^ ((unsigned)r0);
			opStack--;
			goto nextInstruction;
		case OP_BCOM:
			opStack[-1] = ~ ((unsigned)r0);
			goto nextInstruction;

		case OP_LSH:
			opStack[-1] = r1 << r0;
			opStack--;
			goto nextInstruction;
		case OP_RSHI:
			opStack[-1] = r1 >> r0;
			opStack--;
			goto nextInstruction;
		case OP_RSHU:
			opStack[-1] = ((unsigned)r1) >> r0;
			opStack--;
			goto nextInstruction;

		case OP_NEGF:
			*(float *)opStack =  -*(float *)opStack;
			goto nextInstruction;
		case OP_ADDF:
			*(float *)(opStack-1) = *(float *)(opStack-1) + *(float *)opStack;
			opStack--;
			goto nextInstruction;
		case OP_SUBF:
			*(float *)(opStack-1) = *(float *)(opStack-1) - *(float *)opStack;
			opStack--;
			goto nextInstruction;
		case OP_DIVF:
			*(float *)(opStack-1) = *(float *)(opStack-1) / *(float *)opStack;
			opStack--;
			goto nextInstruction;
		case OP_MULF:
			*(float *)(opStack-1) = *(float *)(opStack-1) * *(float *)opStack;
			opStack--;
			goto nextInstruction;

		case OP_CVIF:
			*(float *)opStack =  (float)*opStack;
			goto nextInstruction;
		case OP_CVFI:
			*opStack = (int) *(float *)opStack;
			goto nextInstruction;
		case OP_SEX8:
			*opStack = (signed char)*opStack;
			goto nextInstruction;
		case OP_SEX16:
			*opStack = (short)*opStack;
			goto nextInstruction;
		}
	}

done:
	vm->currentlyInterpreting = qfalse;

	if ( opStack != &stack[1] ) {
		Com_Error( ERR_DROP, "Interpreter error: opStack = %i", opStack - stack );
	}

	vm->programStack = stackOnEntry;

	// return the result
	return *opStack;
}
