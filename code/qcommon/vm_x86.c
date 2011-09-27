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
// vm_x86.c -- load time compiler and execution environment for x86

#include "vm_local.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #ifdef __FreeBSD__
  #include <sys/types.h>
  #endif

  #include <sys/mman.h> // for PROT_ stuff

  /* need this on NX enabled systems (i386 with PAE kernel or
   * noexec32=on x86_64) */
  #define VM_X86_MMAP
  
  // workaround for systems that use the old MAP_ANON macro
  #ifndef MAP_ANONYMOUS
    #define MAP_ANONYMOUS MAP_ANON
  #endif
#endif

static void VM_Destroy_Compiled(vm_t* self);

/*

  eax		scratch
  ebx/bl	opStack offset
  ecx		scratch (required for shifts)
  edx		scratch (required for divisions)
  esi		program stack
  edi   	opStack base
x86_64:
  r8		vm->instructionPointers
  r9		vm->dataBase

*/

#define VMFREE_BUFFERS() do {Z_Free(buf); Z_Free(jused);} while(0)
static	byte	*buf = NULL;
static	byte	*jused = NULL;
static	int		jusedSize = 0;
static	int		compiledOfs = 0;
static	byte	*code = NULL;
static	int		pc = 0;

#define FTOL_PTR

static	int	instruction, pass;
static	int	lastConst = 0;
static	int	oc0, oc1, pop0, pop1;
static	int jlabel;

typedef enum 
{
	LAST_COMMAND_NONE	= 0,
	LAST_COMMAND_MOV_STACK_EAX,
	LAST_COMMAND_SUB_BL_1,
	LAST_COMMAND_SUB_BL_2,
} ELastCommand;

typedef enum
{
	VM_JMP_VIOLATION = 0,
	VM_BLOCK_COPY = 1
} ESysCallType;

static	ELastCommand	LastCommand;

static int iss8(int32_t v)
{
	return (SCHAR_MIN <= v && v <= SCHAR_MAX);
}

#if 0
static int isu8(uint32_t v)
{
	return (v <= UCHAR_MAX);
}
#endif

static int NextConstant4(void)
{
	return (code[pc] | (code[pc+1]<<8) | (code[pc+2]<<16) | (code[pc+3]<<24));
}

static int	Constant4( void ) {
	int		v;

	v = NextConstant4();
	pc += 4;
	return v;
}

static int	Constant1( void ) {
	int		v;

	v = code[pc];
	pc += 1;
	return v;
}

static void Emit1( int v ) 
{
	buf[ compiledOfs ] = v;
	compiledOfs++;

	LastCommand = LAST_COMMAND_NONE;
}

static void Emit2(int v)
{
	Emit1(v & 255);
	Emit1((v >> 8) & 255);
}


static void Emit4(int v)
{
	Emit1(v & 0xFF);
	Emit1((v >> 8) & 0xFF);
	Emit1((v >> 16) & 0xFF);
	Emit1((v >> 24) & 0xFF);
}

static void EmitPtr(void *ptr)
{
	intptr_t v = (intptr_t) ptr;
	
	Emit4(v);
#if idx64
	Emit1((v >> 32) & 0xFF);
	Emit1((v >> 40) & 0xFF);
	Emit1((v >> 48) & 0xFF);
	Emit1((v >> 56) & 0xFF);
#endif
}

static int Hex( int c ) {
	if ( c >= 'a' && c <= 'f' ) {
		return 10 + c - 'a';
	}
	if ( c >= 'A' && c <= 'F' ) {
		return 10 + c - 'A';
	}
	if ( c >= '0' && c <= '9' ) {
		return c - '0';
	}

	VMFREE_BUFFERS();
	Com_Error( ERR_DROP, "Hex: bad char '%c'", c );

	return 0;
}
static void EmitString( const char *string ) {
	int		c1, c2;
	int		v;

	while ( 1 ) {
		c1 = string[0];
		c2 = string[1];

		v = ( Hex( c1 ) << 4 ) | Hex( c2 );
		Emit1( v );

		if ( !string[2] ) {
			break;
		}
		string += 3;
	}
}
static void EmitRexString(byte rex, const char *string)
{
#if idx64
	if(rex)
		Emit1(rex);
#endif

	EmitString(string);
}


#define MASK_REG(modrm, mask) \
	EmitString("81"); \
	EmitString((modrm)); \
	Emit4((mask))

// add bl, bytes
#define STACK_PUSH(bytes) \
	EmitString("80 C3"); \
	Emit1(bytes)

// sub bl, bytes
#define STACK_POP(bytes) \
	EmitString("80 EB"); \
	Emit1(bytes)

static void EmitCommand(ELastCommand command)
{
	switch(command)
	{
		case LAST_COMMAND_MOV_STACK_EAX:
			EmitString("89 04 9F");		// mov dword ptr [edi + ebx * 4], eax
			break;

		case LAST_COMMAND_SUB_BL_1:
			STACK_POP(1);			// sub bl, 1
			break;

		case LAST_COMMAND_SUB_BL_2:
			STACK_POP(2);			// sub bl, 2
			break;
		default:
			break;
	}
	LastCommand = command;
}

static void EmitPushStack(vm_t *vm)
{
	if (!jlabel)
	{
		if(LastCommand == LAST_COMMAND_SUB_BL_1)
		{	// sub bl, 1
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			return;
		}
		if(LastCommand == LAST_COMMAND_SUB_BL_2)
		{	// sub bl, 2
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			STACK_POP(1);		//	sub bl, 1
			return;
		}
	}

	STACK_PUSH(1);		// add bl, 1
}

static void EmitMovEAXStack(vm_t *vm, int andit)
{
	if(!jlabel)
	{
		if(LastCommand == LAST_COMMAND_MOV_STACK_EAX) 
		{	// mov [edi + ebx * 4], eax
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
		}
		else if(pop1 == OP_CONST && buf[compiledOfs-7] == 0xC7 && buf[compiledOfs-6] == 0x04 && buf[compiledOfs - 5] == 0x9F)
		{	// mov [edi + ebx * 4], 0x12345678
			compiledOfs -= 7;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			EmitString("B8");	// mov	eax, 0x12345678

			if(andit)
				Emit4(lastConst & andit);
			else
				Emit4(lastConst);
			
			return;
		}
		else if(pop1 != OP_DIVI && pop1 != OP_DIVU && pop1 != OP_MULI && pop1 != OP_MULU &&
			pop1 != OP_STORE4 && pop1 != OP_STORE2 && pop1 != OP_STORE1)
		{	
			EmitString("8B 04 9F");	// mov eax, dword ptr [edi + ebx * 4]
		}
	}
	else
		EmitString("8B 04 9F");		// mov eax, dword ptr [edi + ebx * 4]

	if(andit)
	{
		EmitString("25");		// and eax, 0x12345678
		Emit4(andit);
	}
}

void EmitMovECXStack(vm_t *vm)
{
	if(!jlabel)
	{
		if(LastCommand == LAST_COMMAND_MOV_STACK_EAX) // mov [edi + ebx * 4], eax
		{
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			EmitString("89 C1");		// mov ecx, eax
			return;
		}
		if(pop1 == OP_DIVI || pop1 == OP_DIVU || pop1 == OP_MULI || pop1 == OP_MULU ||
			pop1 == OP_STORE4 || pop1 == OP_STORE2 || pop1 == OP_STORE1) 
		{	
			EmitString("89 C1");		// mov ecx, eax
			return;
		}
	}

	EmitString("8B 0C 9F");		// mov ecx, dword ptr [edi + ebx * 4]
}


void EmitMovEDXStack(vm_t *vm, int andit)
{
	if(!jlabel)
	{
		if(LastCommand == LAST_COMMAND_MOV_STACK_EAX)
		{	// mov dword ptr [edi + ebx * 4], eax
			compiledOfs -= 3;
			vm->instructionPointers[instruction - 1] = compiledOfs;

			EmitString("8B D0");	// mov edx, eax
		}
		else if(pop1 == OP_DIVI || pop1 == OP_DIVU || pop1 == OP_MULI || pop1 == OP_MULU ||
			pop1 == OP_STORE4 || pop1 == OP_STORE2 || pop1 == OP_STORE1)
		{	
			EmitString("8B D0");	// mov edx, eax
		}
		else if(pop1 == OP_CONST && buf[compiledOfs-7] == 0xC7 && buf[compiledOfs-6] == 0x07 && buf[compiledOfs - 5] == 0x9F)
		{	// mov dword ptr [edi + ebx * 4], 0x12345678
			compiledOfs -= 7;
			vm->instructionPointers[instruction - 1] = compiledOfs;
			EmitString("BA");		// mov edx, 0x12345678

			if(andit)
				Emit4(lastConst & andit);
			else
				Emit4(lastConst);
			
			return;
		}
		else
			EmitString("8B 14 9F");	// mov edx, dword ptr [edi + ebx * 4]
		
	}
	else
		EmitString("8B 14 9F");		// mov edx, dword ptr [edi + ebx * 4]
	
	if(andit)
		MASK_REG("E2", andit);		// and edx, 0x12345678
}

#define JUSED(x) \
	do { \
		if (x < 0 || x >= vm->instructionCount) { \
			VMFREE_BUFFERS(); \
			Com_Error( ERR_DROP, \
					"VM_CompileX86: jump target out of range at offset %d", pc ); \
		} \
		jused[x] = 1; \
	} while(0)

#define SET_JMPOFS(x) do { buf[(x)] = compiledOfs - ((x) + 1); } while(0)


/*
=================
ErrJump
Error handler for jump/call to invalid instruction number
=================
*/

static void ErrJump(void)
{ 
	Com_Error(ERR_DROP, "program tried to execute code outside VM");
	exit(1);
}

/*
=================
DoSyscall
Uses asm to retrieve arguments from registers to work around different calling conventions
=================
*/

#if defined(_MSC_VER) && idx64

extern void qsyscall64(void);
extern uint8_t qvmcall64(int *programStack, int *opStack, intptr_t *instructionPointers, byte *dataBase);

// Microsoft does not support inline assembler on x64 platforms. Meh.
void DoSyscall(int syscallNum, int programStack, int *opStackBase, uint8_t opStackOfs, intptr_t arg)
{
#else
static void DoSyscall(void)
{
	int syscallNum;
	int programStack;
	int *opStackBase;
	uint8_t opStackOfs;
	intptr_t arg;
#endif

	vm_t *savedVM;

#if defined(_MSC_VER)
  #if !idx64
	__asm
	{
		mov	dword ptr syscallNum, eax
		mov	dword ptr programStack, esi
		mov	byte ptr opStackOfs, bl
		mov	dword ptr opStackBase, edi
		mov	dword ptr arg, ecx
	}
  #endif
#else
	__asm__ volatile(
		""
		: "=a" (syscallNum), "=S" (programStack), "=D" (opStackBase), "=b" (opStackOfs),
		  "=c" (arg)
		);
#endif

	// save currentVM so as to allow for recursive VM entry
	savedVM = currentVM;
	// modify VM stack pointer for recursive VM entry
	currentVM->programStack = programStack - 4;

	if(syscallNum < 0)
	{
		int *data;
#if idx64
		int index;
		intptr_t args[16];
#endif
		
		data = (int *) (savedVM->dataBase + programStack + 4);

#if idx64
		args[0] = ~syscallNum;
		for(index = 1; index < ARRAY_LEN(args); index++)
			args[index] = data[index];
			
		opStackBase[opStackOfs + 1] = savedVM->systemCall(args);
#else
		data[0] = ~syscallNum;
		opStackBase[opStackOfs + 1] = savedVM->systemCall((intptr_t *) data);
#endif
	}
	else
	{
		switch(syscallNum)
		{
		case VM_JMP_VIOLATION:
			ErrJump();
		break;
		case VM_BLOCK_COPY: 
			if(opStackOfs < 1)
				Com_Error(ERR_DROP, "VM_BLOCK_COPY failed due to corrupted opStack");
			
			VM_BlockCopy(opStackBase[(opStackOfs - 1)], opStackBase[opStackOfs], arg);
		break;
		default:
			Com_Error(ERR_DROP, "Unknown VM operation %d", syscallNum);
		break;
		}
	}

	currentVM = savedVM;
}

/*
=================
EmitCallRel
Relative call to vm->codeBase + callOfs
=================
*/

void EmitCallRel(vm_t *vm, int callOfs)
{
	EmitString("E8");			// call 0x12345678
	Emit4(callOfs - compiledOfs - 4);
}

/*
=================
EmitCallDoSyscall
Call to DoSyscall()
=================
*/

int EmitCallDoSyscall(vm_t *vm)
{
	// use edx register to store DoSyscall address
#if defined(_MSC_VER) && idx64
	EmitRexString(0x48, "BA");		// mov edx, qsyscall64
	EmitPtr(qsyscall64);
#else
	EmitRexString(0x48, "BA");		// mov edx, DoSyscall
	EmitPtr(DoSyscall);
#endif

	// Push important registers to stack as we can't really make
	// any assumptions about calling conventions.
	EmitString("51");			// push ebx
	EmitString("56");			// push esi
	EmitString("57");			// push edi
#if idx64
	EmitRexString(0x41, "50");		// push r8
	EmitRexString(0x41, "51");		// push r9
#endif

	// align the stack pointer to a 16-byte-boundary
	EmitString("55");			// push ebp
	EmitRexString(0x48, "89 E5");		// mov ebp, esp
	EmitRexString(0x48, "83 E4 F0");	// and esp, 0xFFFFFFF0
			
	// call the syscall wrapper function DoSyscall()

	EmitString("FF D2");			// call edx

	// reset the stack pointer to its previous value
	EmitRexString(0x48, "89 EC");		// mov esp, ebp
	EmitString("5D");			// pop ebp

#if idx64
	EmitRexString(0x41, "59");		// pop r9
	EmitRexString(0x41, "58");		// pop r8
#endif
	EmitString("5F");			// pop edi
	EmitString("5E");			// pop esi
	EmitString("59");			// pop ebx

	EmitString("C3");			// ret

	return compiledOfs;
}

/*
=================
EmitCallErrJump
Emit the code that triggers execution of the jump violation handler
=================
*/

static void EmitCallErrJump(vm_t *vm, int sysCallOfs)
{
	EmitString("B8");			// mov eax, 0x12345678
	Emit4(VM_JMP_VIOLATION);

	EmitCallRel(vm, sysCallOfs);
}

/*
=================
EmitCallProcedure
VM OP_CALL procedure for call destinations obtained at runtime
=================
*/

int EmitCallProcedure(vm_t *vm, int sysCallOfs)
{
	int jmpSystemCall, jmpBadAddr;
	int retval;

	EmitString("8B 04 9F");		// mov eax, dword ptr [edi + ebx * 4]
	STACK_POP(1);			// sub bl, 1
	EmitString("85 C0");		// test eax, eax

	// Jump to syscall code, 1 byte offset should suffice
	EmitString("7C");		// jl systemCall
	jmpSystemCall = compiledOfs++;
		
	/************ Call inside VM ************/
	
	EmitString("81 F8");		// cmp eax, vm->instructionCount
	Emit4(vm->instructionCount);
		
	// Error jump if invalid jump target
	EmitString("73");		// jae badAddr
	jmpBadAddr = compiledOfs++;

#if idx64
	EmitRexString(0x49, "FF 14 C0");	// call qword ptr [r8 + eax * 8]
#else
	EmitString("FF 14 85");			// call dword ptr [vm->instructionPointers + eax * 4]
	Emit4((intptr_t) vm->instructionPointers);
#endif
	EmitString("8B 04 9F");			// mov eax, dword ptr [edi + ebx * 4]
	EmitString("C3");			// ret
		
	// badAddr:
	SET_JMPOFS(jmpBadAddr);
	EmitCallErrJump(vm, sysCallOfs);

	/************ System Call ************/
	
	// systemCall:
	SET_JMPOFS(jmpSystemCall);
	retval = compiledOfs;

	EmitCallRel(vm, sysCallOfs);

	// have opStack reg point at return value
	STACK_PUSH(1);			// add bl, 1
	EmitString("C3");		// ret

	return retval;
}

/*
=================
EmitJumpIns
Jump to constant instruction number
=================
*/

void EmitJumpIns(vm_t *vm, const char *jmpop, int cdest)
{
	JUSED(cdest);

	EmitString(jmpop);	// j??? 0x12345678

	// we only know all the jump addresses in the third pass
	if(pass == 2)
		Emit4(vm->instructionPointers[cdest] - compiledOfs - 4);
	else
		compiledOfs += 4;
}

/*
=================
EmitCallIns
Call to constant instruction number
=================
*/

void EmitCallIns(vm_t *vm, int cdest)
{
	JUSED(cdest);

	EmitString("E8");	// call 0x12345678

	// we only know all the jump addresses in the third pass
	if(pass == 2)
		Emit4(vm->instructionPointers[cdest] - compiledOfs - 4);
	else
		compiledOfs += 4;
}

/*
=================
EmitCallConst
Call to constant instruction number or syscall
=================
*/

void EmitCallConst(vm_t *vm, int cdest, int callProcOfsSyscall)
{
	if(cdest < 0)
	{
		EmitString("B8");	// mov eax, cdest
		Emit4(cdest);

		EmitCallRel(vm, callProcOfsSyscall);
	}
	else
		EmitCallIns(vm, cdest);
}

/*
=================
EmitBranchConditions
Emits x86 branch condition as given in op
=================
*/
void EmitBranchConditions(vm_t *vm, int op)
{
	switch(op)
	{
	case OP_EQ:
		EmitJumpIns(vm, "0F 84", Constant4());	// je 0x12345678
	break;
	case OP_NE:
		EmitJumpIns(vm, "0F 85", Constant4());	// jne 0x12345678
	break;
	case OP_LTI:
		EmitJumpIns(vm, "0F 8C", Constant4());	// jl 0x12345678
	break;
	case OP_LEI:
		EmitJumpIns(vm, "0F 8E", Constant4());	// jle 0x12345678
	break;
	case OP_GTI:
		EmitJumpIns(vm, "0F 8F", Constant4());	// jg 0x12345678
	break;
	case OP_GEI:
		EmitJumpIns(vm, "0F 8D", Constant4());	// jge 0x12345678
	break;
	case OP_LTU:
		EmitJumpIns(vm, "0F 82", Constant4());	// jb 0x12345678
	break;
	case OP_LEU:
		EmitJumpIns(vm, "0F 86", Constant4());	// jbe 0x12345678
	break;
	case OP_GTU:
		EmitJumpIns(vm, "0F 87", Constant4());	// ja 0x12345678
	break;
	case OP_GEU:
		EmitJumpIns(vm, "0F 83", Constant4());	// jae 0x12345678
	break;
	}
}


/*
=================
ConstOptimize
Constant values for immediately following instructions may be translated to immediate values
instead of opStack operations, which will save expensive operations on memory
=================
*/

qboolean ConstOptimize(vm_t *vm, int callProcOfsSyscall)
{
	int v;
	int op1;

	// we can safely perform optimizations only in case if 
	// we are 100% sure that next instruction is not a jump label
	if (vm->jumpTableTargets && !jused[instruction])
		op1 = code[pc+4];
	else
		return qfalse;

	switch ( op1 ) {

	case OP_LOAD4:
		EmitPushStack(vm);
#if idx64
		EmitRexString(0x41, "8B 81");			// mov eax, dword ptr [r9 + 0x12345678]
		Emit4(Constant4() & vm->dataMask);
#else
		EmitString("B8");				// mov eax, 0x12345678
		EmitPtr(vm->dataBase + (Constant4() & vm->dataMask));
		EmitString("8B 00");				// mov eax, dword ptr [eax]
#endif
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax

		pc++;						// OP_LOAD4
		instruction += 1;
		return qtrue;

	case OP_LOAD2:
		EmitPushStack(vm);
#if idx64
		EmitRexString(0x41, "0F B7 81");		// movzx eax, word ptr [r9 + 0x12345678]
		Emit4(Constant4() & vm->dataMask);
#else
		EmitString("B8");				// mov eax, 0x12345678
		EmitPtr(vm->dataBase + (Constant4() & vm->dataMask));
		EmitString("0F B7 00");				// movzx eax, word ptr [eax]
#endif
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax

		pc++;						// OP_LOAD2
		instruction += 1;
		return qtrue;

	case OP_LOAD1:
		EmitPushStack(vm);
#if idx64
		EmitRexString(0x41, "0F B6 81");		// movzx eax, byte ptr [r9 + 0x12345678]
		Emit4(Constant4() & vm->dataMask);
#else
		EmitString("B8");				// mov eax, 0x12345678
		EmitPtr(vm->dataBase + (Constant4() & vm->dataMask));
		EmitString("0F B6 00");				// movzx eax, byte ptr [eax]
#endif
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax

		pc++;						// OP_LOAD1
		instruction += 1;
		return qtrue;

	case OP_STORE4:
		EmitMovEAXStack(vm, (vm->dataMask & ~3));
#if idx64
		EmitRexString(0x41, "C7 04 01");		// mov dword ptr [r9 + eax], 0x12345678
		Emit4(Constant4());
#else
		EmitString("C7 80");				// mov dword ptr [eax + 0x12345678], 0x12345678
		Emit4((intptr_t) vm->dataBase);
		Emit4(Constant4());
#endif
		EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
		pc++;						// OP_STORE4
		instruction += 1;
		return qtrue;

	case OP_STORE2:
		EmitMovEAXStack(vm, (vm->dataMask & ~1));
#if idx64
		Emit1(0x66);					// mov word ptr [r9 + eax], 0x1234
		EmitRexString(0x41, "C7 04 01");
		Emit2(Constant4());
#else
		EmitString("66 C7 80");				// mov word ptr [eax + 0x12345678], 0x1234
		Emit4((intptr_t) vm->dataBase);
		Emit2(Constant4());
#endif
		EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1

		pc++;						// OP_STORE2
		instruction += 1;
		return qtrue;

	case OP_STORE1:
		EmitMovEAXStack(vm, vm->dataMask);
#if idx64
		EmitRexString(0x41, "C6 04 01");		// mov byte [r9 + eax], 0x12
		Emit1(Constant4());
#else
		EmitString("C6 80");				// mov byte ptr [eax + 0x12345678], 0x12
		Emit4((intptr_t) vm->dataBase);
		Emit1(Constant4());
#endif
		EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1

		pc++;						// OP_STORE1
		instruction += 1;
		return qtrue;

	case OP_ADD:
		v = Constant4();

		EmitMovEAXStack(vm, 0);
		if(iss8(v))
		{
			EmitString("83 C0");			// add eax, 0x7F
			Emit1(v);
		}
		else
		{
			EmitString("05");			// add eax, 0x12345678
			Emit4(v);
		}
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);

		pc++;						// OP_ADD
		instruction += 1;
		return qtrue;

	case OP_SUB:
		v = Constant4();

		EmitMovEAXStack(vm, 0);
		if(iss8(v))
		{
			EmitString("83 E8");			// sub eax, 0x7F
			Emit1(v);
		}
		else
		{
			EmitString("2D");			// sub eax, 0x12345678
			Emit4(v);
		}
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);

		pc++;						// OP_SUB
		instruction += 1;
		return qtrue;

	case OP_MULI:
		v = Constant4();

		EmitMovEAXStack(vm, 0);
		if(iss8(v))
		{
			EmitString("6B C0");			// imul eax, 0x7F
			Emit1(v);
		}
		else
		{
			EmitString("69 C0");			// imul eax, 0x12345678
			Emit4(v);
		}
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);
		pc++;						// OP_MULI
		instruction += 1;

		return qtrue;

	case OP_LSH:
		v = NextConstant4();
		if(v < 0 || v > 31)
			break;

		EmitMovEAXStack(vm, 0);
		EmitString("C1 E0");				// shl eax, 0x12
		Emit1(v);
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);

		pc += 5;					// CONST + OP_LSH
		instruction += 1;
		return qtrue;

	case OP_RSHI:
		v = NextConstant4();
		if(v < 0 || v > 31)
			break;
			
		EmitMovEAXStack(vm, 0);
		EmitString("C1 F8");				// sar eax, 0x12
		Emit1(v);
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);

		pc += 5;					// CONST + OP_RSHI
		instruction += 1;
		return qtrue;

	case OP_RSHU:
		v = NextConstant4();
		if(v < 0 || v > 31)
			break;
			
		EmitMovEAXStack(vm, 0);
		EmitString("C1 E8");				// shr eax, 0x12
		Emit1(v);
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);

		pc += 5;					// CONST + OP_RSHU
		instruction += 1;
		return qtrue;
	
	case OP_BAND:
		v = Constant4();

		EmitMovEAXStack(vm, 0);
		if(iss8(v))
		{
			EmitString("83 E0");			// and eax, 0x7F
			Emit1(v);
		}
		else
		{
			EmitString("25");			// and eax, 0x12345678
			Emit4(v);
		}
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);
		
		pc += 1;					// OP_BAND
		instruction += 1;
		return qtrue;

	case OP_BOR:
		v = Constant4();

		EmitMovEAXStack(vm, 0);
		if(iss8(v))
		{
			EmitString("83 C8");			// or eax, 0x7F
			Emit1(v);
		}
		else
		{
			EmitString("0D");			// or eax, 0x12345678
			Emit4(v);
		}
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);
		
		pc += 1;				 	// OP_BOR
		instruction += 1;
		return qtrue;

	case OP_BXOR:
		v = Constant4();
		
		EmitMovEAXStack(vm, 0);
		if(iss8(v))
		{
			EmitString("83 F0");			// xor eax, 0x7F
			Emit1(v);
		}
		else
		{
			EmitString("35");			// xor eax, 0x12345678
			Emit4(v);
		}
		EmitCommand(LAST_COMMAND_MOV_STACK_EAX);
		
		pc += 1;					// OP_BXOR
		instruction += 1;
		return qtrue;

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
		EmitMovEAXStack(vm, 0);
		EmitCommand(LAST_COMMAND_SUB_BL_1);
		EmitString("3D");				// cmp eax, 0x12345678
		Emit4(Constant4());

		pc++;						// OP_*
		EmitBranchConditions(vm, op1);
		instruction++;

		return qtrue;

	case OP_EQF:
	case OP_NEF:
		if(NextConstant4())
			break;
		pc += 5;					// CONST + OP_EQF|OP_NEF

		EmitMovEAXStack(vm, 0);
		EmitCommand(LAST_COMMAND_SUB_BL_1);
		// floating point hack :)
		EmitString("25");				// and eax, 0x7FFFFFFF
		Emit4(0x7FFFFFFF);
		if(op1 == OP_EQF)
			EmitJumpIns(vm, "0F 84", Constant4());	// jz 0x12345678
		else
			EmitJumpIns(vm, "0F 85", Constant4());	// jnz 0x12345678
		
		instruction += 1;
		return qtrue;


	case OP_JUMP:
		EmitJumpIns(vm, "E9", Constant4());		// jmp 0x12345678

		pc += 1;                  // OP_JUMP
		instruction += 1;
		return qtrue;

	case OP_CALL:
		v = Constant4();
		EmitCallConst(vm, v, callProcOfsSyscall);

		pc += 1;                  // OP_CALL
		instruction += 1;
		return qtrue;

	default:
		break;
	}

	return qfalse;
}

/*
=================
VM_Compile
=================
*/
void VM_Compile(vm_t *vm, vmHeader_t *header)
{
	int		op;
	int		maxLength;
	int		v;
	int		i;
        int		callProcOfsSyscall, callProcOfs, callDoSyscallOfs;

	jusedSize = header->instructionCount + 2;

	// allocate a very large temp buffer, we will shrink it later
	maxLength = header->codeLength * 8 + 64;
	buf = Z_Malloc(maxLength);
	jused = Z_Malloc(jusedSize);
	code = Z_Malloc(header->codeLength+32);
	
	Com_Memset(jused, 0, jusedSize);
	Com_Memset(buf, 0, maxLength);

	// copy code in larger buffer and put some zeros at the end
	// so we can safely look ahead for a few instructions in it
	// without a chance to get false-positive because of some garbage bytes
	Com_Memset(code, 0, header->codeLength+32);
	Com_Memcpy(code, (byte *)header + header->codeOffset, header->codeLength );

	// ensure that the optimisation pass knows about all the jump
	// table targets
	for( i = 0; i < vm->numJumpTableTargets; i++ ) {
		jused[ *(int *)(vm->jumpTableTargets + ( i * sizeof( int ) ) ) ] = 1;
	}

	// Start buffer with x86-VM specific procedures
	compiledOfs = 0;

	callDoSyscallOfs = compiledOfs;
	callProcOfs = EmitCallDoSyscall(vm);
	callProcOfsSyscall = EmitCallProcedure(vm, callDoSyscallOfs);
	vm->entryOfs = compiledOfs;

	for(pass=0; pass < 3; pass++) {
	oc0 = -23423;
	oc1 = -234354;
	pop0 = -43435;
	pop1 = -545455;

	// translate all instructions
	pc = 0;
	instruction = 0;
	//code = (byte *)header + header->codeOffset;
	compiledOfs = vm->entryOfs;

	LastCommand = LAST_COMMAND_NONE;

	while(instruction < header->instructionCount)
	{
		if(compiledOfs > maxLength - 16)
		{
	        	VMFREE_BUFFERS();
			Com_Error(ERR_DROP, "VM_CompileX86: maxLength exceeded");
		}

		vm->instructionPointers[ instruction ] = compiledOfs;

		if ( !vm->jumpTableTargets )
			jlabel = 1;
		else 
			jlabel = jused[ instruction ];

		instruction++;

		if(pc > header->codeLength)
		{
		        VMFREE_BUFFERS();
			Com_Error(ERR_DROP, "VM_CompileX86: pc > header->codeLength");
		}

		op = code[ pc ];
		pc++;
		switch ( op ) {
		case 0:
			break;
		case OP_BREAK:
			EmitString("CC");				// int 3
			break;
		case OP_ENTER:
			EmitString("81 EE");				// sub esi, 0x12345678
			Emit4(Constant4());
			break;
		case OP_CONST:
			if(ConstOptimize(vm, callProcOfsSyscall))
				break;

			EmitPushStack(vm);
			EmitString("C7 04 9F");				// mov dword ptr [edi + ebx * 4], 0x12345678
			lastConst = Constant4();

			Emit4(lastConst);
			if(code[pc] == OP_JUMP)
				JUSED(lastConst);

			break;
		case OP_LOCAL:
			EmitPushStack(vm);
			EmitString("8D 86");				// lea eax, [0x12345678 + esi]
			oc0 = oc1;
			oc1 = Constant4();
			Emit4(oc1);
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_ARG:
			EmitMovEAXStack(vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString("8B D6");				// mov edx, esi
			EmitString("81 C2");				// add edx, 0x12345678
			Emit4((Constant1() & 0xFF));
			MASK_REG("E2", vm->dataMask);			// and edx, 0x12345678
#if idx64
			EmitRexString(0x41, "89 04 11");		// mov dword ptr [r9 + edx], eax
#else
			EmitString("89 82");				// mov dword ptr [edx + 0x12345678], eax
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_CALL:
			EmitCallRel(vm, callProcOfs);
			break;
		case OP_PUSH:
			EmitPushStack(vm);
			break;
		case OP_POP:
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_LEAVE:
			v = Constant4();
			EmitString("81 C6");				// add	esi, 0x12345678
			Emit4(v);
			EmitString("C3");				// ret
			break;
		case OP_LOAD4:
			if (code[pc] == OP_CONST && code[pc+5] == OP_ADD && code[pc+6] == OP_STORE4)
			{
				if(oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
					compiledOfs -= 12;
					vm->instructionPointers[instruction - 1] = compiledOfs;
				}

				pc++;				// OP_CONST
				v = Constant4();

				EmitMovEDXStack(vm, vm->dataMask);
				if(v == 1 && oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
#if idx64
					EmitRexString(0x41, "FF 04 11");	// inc dword ptr [r9 + edx]
#else
					EmitString("FF 82");			// inc dword ptr [edx + 0x12345678]
					Emit4((intptr_t) vm->dataBase);
#endif
				}
				else
				{
#if idx64
					EmitRexString(0x41, "8B 04 11");	// mov eax, dword ptr [r9 + edx]
#else
					EmitString("8B 82");			// mov eax, dword ptr [edx + 0x12345678]
					Emit4((intptr_t) vm->dataBase);
#endif
					EmitString("05");			// add eax, v
					Emit4(v);
					
					if (oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
					{
#if idx64
						EmitRexString(0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString("89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4((intptr_t) vm->dataBase);
#endif
					}
					else
					{
						EmitCommand(LAST_COMMAND_SUB_BL_1);	// sub bl, 1
						EmitString("8B 14 9F");			// mov edx, dword ptr [edi + ebx * 4]
						MASK_REG("E2", vm->dataMask);		// and edx, 0x12345678
#if idx64
						EmitRexString(0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString("89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4((intptr_t) vm->dataBase);
#endif
					}
				}

				EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
				pc++;						// OP_ADD
				pc++;						// OP_STORE
				instruction += 3;
				break;
			}

			if(code[pc] == OP_CONST && code[pc+5] == OP_SUB && code[pc+6] == OP_STORE4)
			{
				if(oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
					compiledOfs -= 12;
					vm->instructionPointers[instruction - 1] = compiledOfs;
				}
				
				pc++;					// OP_CONST
				v = Constant4();

				EmitMovEDXStack(vm, vm->dataMask);
				if(v == 1 && oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
				{
#if idx64
					EmitRexString(0x41, "FF 0C 11");	// dec dword ptr [r9 + edx]
#else
					EmitString("FF 8A");			// dec dword ptr [edx + 0x12345678]
					Emit4((intptr_t) vm->dataBase);
#endif
				}
				else
				{
#if idx64
					EmitRexString(0x41, "8B 04 11");	// mov eax, dword ptr [r9 + edx]
#else
					EmitString("8B 82");			// mov eax, dword ptr [edx + 0x12345678]
					Emit4((intptr_t) vm->dataBase);
#endif
					EmitString("2D");			// sub eax, v
					Emit4(v);
					
					if(oc0 == oc1 && pop0 == OP_LOCAL && pop1 == OP_LOCAL)
					{
#if idx64
						EmitRexString(0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString("89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4((intptr_t) vm->dataBase);
#endif
					}
					else
					{
						EmitCommand(LAST_COMMAND_SUB_BL_1);	// sub bl, 1
						EmitString("8B 14 9F");			// mov edx, dword ptr [edi + ebx * 4]
						MASK_REG("E2", vm->dataMask);		// and edx, 0x12345678
#if idx64
						EmitRexString(0x41, "89 04 11");	// mov dword ptr [r9 + edx], eax
#else
						EmitString("89 82");			// mov dword ptr [edx + 0x12345678], eax
						Emit4((intptr_t) vm->dataBase);
#endif
					}
				}
				EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
				pc++;						// OP_SUB
				pc++;						// OP_STORE
				instruction += 3;
				break;
			}

			if(buf[compiledOfs - 3] == 0x89 && buf[compiledOfs - 2] == 0x04 && buf[compiledOfs - 1] == 0x9F)
			{
				compiledOfs -= 3;
				vm->instructionPointers[instruction - 1] = compiledOfs;
				MASK_REG("E0", vm->dataMask);			// and eax, 0x12345678
#if idx64
				EmitRexString(0x41, "8B 04 01");		// mov eax, dword ptr [r9 + eax]
#else
				EmitString("8B 80");				// mov eax, dword ptr [eax + 0x1234567]
				Emit4((intptr_t) vm->dataBase);
#endif
				EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
				break;
			}
			
			EmitMovEAXStack(vm, vm->dataMask);
#if idx64
			EmitRexString(0x41, "8B 04 01");		// mov eax, dword ptr [r9 + eax]
#else
			EmitString("8B 80");				// mov eax, dword ptr [eax + 0x12345678]
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_LOAD2:
			EmitMovEAXStack(vm, vm->dataMask);
#if idx64
			EmitRexString(0x41, "0F B7 04 01");		// movzx eax, word ptr [r9 + eax]
#else
			EmitString("0F B7 80");				// movzx eax, word ptr [eax + 0x12345678]
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_LOAD1:
			EmitMovEAXStack(vm, vm->dataMask);
#if idx64
			EmitRexString(0x41, "0F B6 04 01");		// movzx eax, byte ptr [r9 + eax]
#else
			EmitString("0F B6 80");				// movzx eax, byte ptr [eax + 0x12345678]
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_STORE4:
			EmitMovEAXStack(vm, 0);	
			EmitString("8B 54 9F FC");			// mov edx, dword ptr -4[edi + ebx * 4]
			MASK_REG("E2", vm->dataMask & ~3);		// and edx, 0x12345678
#if idx64
			EmitRexString(0x41, "89 04 11");		// mov dword ptr [r9 + edx], eax
#else
			EmitString("89 82");				// mov dword ptr [edx + 0x12345678], eax
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;
		case OP_STORE2:
			EmitMovEAXStack(vm, 0);	
			EmitString("8B 54 9F FC");			// mov edx, dword ptr -4[edi + ebx * 4]
			MASK_REG("E2", vm->dataMask & ~1);		// and edx, 0x12345678
#if idx64
			Emit1(0x66);					// mov word ptr [r9 + edx], eax
			EmitRexString(0x41, "89 04 11");
#else
			EmitString("66 89 82");				// mov word ptr [edx + 0x12345678], eax
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;
		case OP_STORE1:
			EmitMovEAXStack(vm, 0);	
			EmitString("8B 54 9F FC");			// mov edx, dword ptr -4[edi + ebx * 4]
			MASK_REG("E2", vm->dataMask);			// and edx, 0x12345678
#if idx64
			EmitRexString(0x41, "88 04 11");		// mov byte ptr [r9 + edx], eax
#else
			EmitString("88 82");				// mov byte ptr [edx + 0x12345678], eax
			Emit4((intptr_t) vm->dataBase);
#endif
			EmitCommand(LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;

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
			EmitMovEAXStack(vm, 0);
			EmitCommand(LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			EmitString("39 44 9F 04");			// cmp	eax, dword ptr 4[edi + ebx * 4]

			EmitBranchConditions(vm, op);
		break;
		case OP_EQF:
		case OP_NEF:
		case OP_LTF:
		case OP_LEF:
		case OP_GTF:
		case OP_GEF:
			EmitCommand(LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			EmitString("D9 44 9F 04");			// fld dword ptr 4[edi + ebx * 4]
			EmitString("D8 5C 9F 08");			// fcomp dword ptr 8[edi + ebx * 4]
			EmitString("DF E0");				// fnstsw ax

			switch(op)
			{
			case OP_EQF:
				EmitString("F6 C4 40");			// test	ah,0x40
				EmitJumpIns(vm, "0F 85", Constant4());	// jne 0x12345678
			break;
			case OP_NEF:
				EmitString("F6 C4 40");			// test	ah,0x40
				EmitJumpIns(vm, "0F 84", Constant4());	// je 0x12345678
			break;
			case OP_LTF:
				EmitString("F6 C4 01");			// test	ah,0x01
				EmitJumpIns(vm, "0F 85", Constant4());	// jne 0x12345678
			break;
			case OP_LEF:
				EmitString("F6 C4 41");			// test	ah,0x41
				EmitJumpIns(vm, "0F 85", Constant4());	// jne 0x12345678
			break;
			case OP_GTF:
				EmitString("F6 C4 41");			// test	ah,0x41
				EmitJumpIns(vm, "0F 84", Constant4());	// je 0x12345678
			break;
			case OP_GEF:
				EmitString("F6 C4 01");			// test	ah,0x01
				EmitJumpIns(vm, "0F 84", Constant4());	// je 0x12345678
			break;
			}
		break;			
		case OP_NEGI:
			EmitMovEAXStack(vm, 0);
			EmitString("F7 D8");				// neg eax
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);
			break;
		case OP_ADD:
			EmitMovEAXStack(vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString("01 44 9F FC");			// add dword ptr -4[edi + ebx * 4], eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_SUB:
			EmitMovEAXStack(vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString("29 44 9F FC");			// sub dword ptr -4[edi + ebx * 4], eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_DIVI:
			EmitString("8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString("99");				// cdq
			EmitString("F7 3C 9F");				// idiv dword ptr [edi + ebx * 4]
			EmitString("89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_DIVU:
			EmitString("8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString("33 D2");				// xor edx, edx
			EmitString("F7 34 9F");				// div dword ptr [edi + ebx * 4]
			EmitString("89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MODI:
			EmitString("8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString("99" );				// cdq
			EmitString("F7 3C 9F");				// idiv dword ptr [edi + ebx * 4]
			EmitString("89 54 9F FC");			// mov dword ptr -4[edi + ebx * 4],edx
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MODU:
			EmitString("8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString("33 D2");				// xor edx, edx
			EmitString("F7 34 9F");				// div dword ptr [edi + ebx * 4]
			EmitString("89 54 9F FC");			// mov dword ptr -4[edi + ebx * 4],edx
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MULI:
			EmitString("8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString("F7 2C 9F");				// imul dword ptr [edi + ebx * 4]
			EmitString("89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_MULU:
			EmitString("8B 44 9F FC");			// mov eax,dword ptr -4[edi + ebx * 4]
			EmitString("F7 24 9F");				// mul dword ptr [edi + ebx * 4]
			EmitString("89 44 9F FC");			// mov dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BAND:
			EmitMovEAXStack(vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString("21 44 9F FC");			// and dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BOR:
			EmitMovEAXStack(vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString("09 44 9F FC");			// or dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BXOR:
			EmitMovEAXStack(vm, 0);				// mov eax, dword ptr [edi + ebx * 4]
			EmitString("31 44 9F FC");			// xor dword ptr -4[edi + ebx * 4],eax
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_BCOM:
			EmitString("F7 14 9F");				// not dword ptr [edi + ebx * 4]
			break;
		case OP_LSH:
			EmitMovECXStack(vm);
			EmitString("D3 64 9F FC");			// shl dword ptr -4[edi + ebx * 4], cl
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_RSHI:
			EmitMovECXStack(vm);
			EmitString("D3 7C 9F FC");			// sar dword ptr -4[edi + ebx * 4], cl
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_RSHU:
			EmitMovECXStack(vm);
			EmitString("D3 6C 9F FC");			// shr dword ptr -4[edi + ebx * 4], cl
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_NEGF:
			EmitString("D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString("D9 E0");				// fchs
			EmitString("D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_ADDF:
			EmitString("D9 44 9F FC");			// fld dword ptr -4[edi + ebx * 4]
			EmitString("D8 04 9F");				// fadd dword ptr [edi + ebx * 4]
			EmitString("D9 5C 9F FC");			// fstp dword ptr -4[edi + ebx * 4]
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			break;
		case OP_SUBF:
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			EmitString("D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString("D8 64 9F 04");			// fsub dword ptr 4[edi + ebx * 4]
			EmitString("D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_DIVF:
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			EmitString("D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString("D8 74 9F 04");			// fdiv dword ptr 4[edi + ebx * 4]
			EmitString("D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_MULF:
			EmitCommand(LAST_COMMAND_SUB_BL_1);		// sub bl, 1
			EmitString("D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString("D8 4C 9F 04");			// fmul dword ptr 4[edi + ebx * 4]
			EmitString("D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_CVIF:
			EmitString("DB 04 9F");				// fild dword ptr [edi + ebx * 4]
			EmitString("D9 1C 9F");				// fstp dword ptr [edi + ebx * 4]
			break;
		case OP_CVFI:
#ifndef FTOL_PTR // WHENHELLISFROZENOVER
			// not IEEE complient, but simple and fast
			EmitString("D9 04 9F");				// fld dword ptr [edi + ebx * 4]
			EmitString("DB 1C 9F");				// fistp dword ptr [edi + ebx * 4]
#else // FTOL_PTR
			// call the library conversion function
			EmitRexString(0x48, "BA");			// mov edx, Q_VMftol
			EmitPtr(Q_VMftol);
			EmitRexString(0x48, "FF D2");			// call edx
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
#endif
			break;
		case OP_SEX8:
			EmitString("0F BE 04 9F");			// movsx eax, byte ptr [edi + ebx * 4]
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;
		case OP_SEX16:
			EmitString("0F BF 04 9F");			// movsx eax, word ptr [edi + ebx * 4]
			EmitCommand(LAST_COMMAND_MOV_STACK_EAX);	// mov dword ptr [edi + ebx * 4], eax
			break;

		case OP_BLOCK_COPY:
			EmitString("B8");				// mov eax, 0x12345678
			Emit4(VM_BLOCK_COPY);
			EmitString("B9");				// mov ecx, 0x12345678
			Emit4(Constant4());

			EmitCallRel(vm, callDoSyscallOfs);

			EmitCommand(LAST_COMMAND_SUB_BL_2);		// sub bl, 2
			break;

		case OP_JUMP:
			EmitCommand(LAST_COMMAND_SUB_BL_1);	// sub bl, 1
			EmitString("8B 44 9F 04");		// mov eax, dword ptr 4[edi + ebx * 4]
			EmitString("81 F8");			// cmp eax, vm->instructionCount
			Emit4(vm->instructionCount);
#if idx64
			EmitString("73 04");			// jae +4
			EmitRexString(0x49, "FF 24 C0");        // jmp qword ptr [r8 + eax * 8]
#else
			EmitString("73 07");			// jae +7
			EmitString("FF 24 85");			// jmp dword ptr [instructionPointers + eax * 4]
			Emit4((intptr_t) vm->instructionPointers);
#endif
			EmitCallErrJump(vm, callDoSyscallOfs);
			break;
		default:
		        VMFREE_BUFFERS();
			Com_Error(ERR_DROP, "VM_CompileX86: bad opcode %i at offset %i", op, pc);
		}
		pop0 = pop1;
		pop1 = op;
	}
	}

	// copy to an exact sized buffer with the appropriate permission bits
	vm->codeLength = compiledOfs;
#ifdef VM_X86_MMAP
	vm->codeBase = mmap(NULL, compiledOfs, PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
	if(vm->codeBase == MAP_FAILED)
		Com_Error(ERR_FATAL, "VM_CompileX86: can't mmap memory");
#elif _WIN32
	// allocate memory with EXECUTE permissions under windows.
	vm->codeBase = VirtualAlloc(NULL, compiledOfs, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if(!vm->codeBase)
		Com_Error(ERR_FATAL, "VM_CompileX86: VirtualAlloc failed");
#else
	vm->codeBase = malloc(compiledOfs);
	if(!vm->codeBase)
	        Com_Error(ERR_FATAL, "VM_CompileX86: malloc failed");
#endif

	Com_Memcpy( vm->codeBase, buf, compiledOfs );

#ifdef VM_X86_MMAP
	if(mprotect(vm->codeBase, compiledOfs, PROT_READ|PROT_EXEC))
		Com_Error(ERR_FATAL, "VM_CompileX86: mprotect failed");
#elif _WIN32
	{
		DWORD oldProtect = 0;
		
		// remove write permissions.
		if(!VirtualProtect(vm->codeBase, compiledOfs, PAGE_EXECUTE_READ, &oldProtect))
			Com_Error(ERR_FATAL, "VM_CompileX86: VirtualProtect failed");
	}
#endif

	Z_Free( code );
	Z_Free( buf );
	Z_Free( jused );
	Com_Printf( "VM file %s compiled to %i bytes of code\n", vm->name, compiledOfs );

	vm->destroy = VM_Destroy_Compiled;

	// offset all the instruction pointers for the new location
	for ( i = 0 ; i < header->instructionCount ; i++ ) {
		vm->instructionPointers[i] += (intptr_t) vm->codeBase;
	}
}

void VM_Destroy_Compiled(vm_t* self)
{
#ifdef VM_X86_MMAP
	munmap(self->codeBase, self->codeLength);
#elif _WIN32
	VirtualFree(self->codeBase, 0, MEM_RELEASE);
#else
	free(self->codeBase);
#endif
}

/*
==============
VM_CallCompiled

This function is called directly by the generated code
==============
*/

int VM_CallCompiled(vm_t *vm, int *args)
{
	byte	stack[OPSTACK_SIZE + 15];
	void	*entryPoint;
	int		programStack, stackOnEntry;
	byte	*image;
	int	*opStack;
	int		opStackOfs;

	currentVM = vm;

	// interpret the code
	vm->currentlyInterpreting = qtrue;

	// we might be called recursively, so this might not be the very top
	programStack = stackOnEntry = vm->programStack;

	// set up the stack frame 
	image = vm->dataBase;

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
	entryPoint = vm->codeBase + vm->entryOfs;
	opStack = PADP(stack, 16);
	*opStack = 0xDEADBEEF;
	opStackOfs = 0;

#ifdef _MSC_VER
  #if idx64
	opStackOfs = qvmcall64(&programStack, opStack, vm->instructionPointers, vm->dataBase);
  #else
	__asm
	{
		pushad

		mov	esi, dword ptr programStack
		mov	edi, dword ptr opStack
		mov	ebx, dword ptr opStackOfs

		call	entryPoint

		mov	dword ptr opStackOfs, ebx
		mov	dword ptr opStack, edi
		mov	dword ptr programStack, esi
		
		popad
	}
  #endif		
#elif idx64
	__asm__ volatile(
		"movq %5, %%rax\n"
		"movq %3, %%r8\n"
		"movq %4, %%r9\n"
		"push %%r15\n"
		"push %%r14\n"
		"push %%r13\n"
		"push %%r12\n"
		"callq *%%rax\n"
		"pop %%r12\n"
		"pop %%r13\n"
		"pop %%r14\n"
		"pop %%r15\n"
		: "+S" (programStack), "+D" (opStack), "+b" (opStackOfs)
		: "g" (vm->instructionPointers), "g" (vm->dataBase), "g" (entryPoint)
		: "cc", "memory", "%rax", "%rcx", "%rdx", "%r8", "%r9", "%r10", "%r11"
	);
#else
	__asm__ volatile(
		"calll *%3\n"
		: "+S" (programStack), "+D" (opStack), "+b" (opStackOfs)
		: "g" (entryPoint)
		: "cc", "memory", "%eax", "%ecx", "%edx"
	);
#endif

	if(opStackOfs != 1 || *opStack != 0xDEADBEEF)
	{
		Com_Error(ERR_DROP, "opStack corrupted in compiled code");
	}
	if(programStack != stackOnEntry - 48)
		Com_Error(ERR_DROP, "programStack corrupted in compiled code");

	vm->programStack = stackOnEntry;

	return opStack[opStackOfs];
}
