/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Ludwig Nussel <ludwig.nussel@web.de>

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
// vm_x86_64.c -- load time compiler and execution environment for x86-64

#include "vm_local.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#ifdef DEBUG_VM
#define Dfprintf(fd, args...) fprintf(fd, ##args)
static FILE* qdasmout;
#else
#define Dfprintf(args...)
#endif

static void VM_Destroy_Compiled(vm_t* self);

/*
 
  |=====================|
  ^       dataMask      ^- programStack rdi
  |
  +- r8

  eax	scratch
  ebx	scratch
  ecx	scratch (required for shifts)
  edx	scratch (required for divisions)
  rsi	stack pointer (opStack)
  rdi	program frame pointer (programStack)
  r8    pointer data (vm->dataBase)
  r10   start of generated code
*/


static long callAsmCall(long callProgramStack, long callSyscallNum)
{
	vm_t *savedVM;
	long ret = 0x77;
	long args[11];
//	int iargs[11];
	int i;

//	Dfprintf(stderr, "callAsmCall(%ld, %ld)\n", callProgramStack, callSyscallNum);
//	Com_Printf("-> callAsmCall %s, level %d, num %ld\n", currentVM->name, currentVM->callLevel, callSyscallNum);

	savedVM = currentVM;

	// save the stack to allow recursive VM entry
	currentVM->programStack = callProgramStack - 4;

	args[0] = callSyscallNum;
//	iargs[0] = callSyscallNum;
	for(i = 0; i < 10; ++i)
	{
//		iargs[i+1] = *(int *)((byte *)currentVM->dataBase + callProgramStack + 8 + 4*i);
		args[i+1] = *(int *)((byte *)currentVM->dataBase + callProgramStack + 8 + 4*i);
	}
	ret = currentVM->systemCall(args);

 	currentVM = savedVM;
//	Com_Printf("<- callAsmCall %s, level %d, num %ld\n", currentVM->name, currentVM->callLevel, callSyscallNum);

	return ret;
}

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
#endif // DEBUG_VM

static unsigned char op_argsize[256] = 
{
	[OP_ENTER]      = 4,
	[OP_LEAVE]      = 4,
	[OP_CONST]      = 4,
	[OP_LOCAL]      = 4,
	[OP_EQ]         = 4,
	[OP_NE]         = 4,
	[OP_LTI]        = 4,
	[OP_LEI]        = 4,
	[OP_GTI]        = 4,
	[OP_GEI]        = 4,
	[OP_LTU]        = 4,
	[OP_LEU]        = 4,
	[OP_GTU]        = 4,
	[OP_GEU]        = 4,
	[OP_EQF]        = 4,
	[OP_NEF]        = 4,
	[OP_LTF]        = 4,
	[OP_LEF]        = 4,
	[OP_GTF]        = 4,
	[OP_GEF]        = 4,
	[OP_ARG]        = 1,
	[OP_BLOCK_COPY] = 4,
};

#define emit(x...) \
	do { fprintf(fh_s, ##x); fputc('\n', fh_s); } while(0)
 
// integer compare and jump
#define IJ(op) \
	emit("subq $8, %%rsi"); \
	emit("movl 4(%%rsi), %%eax"); \
	emit("cmpl 8(%%rsi), %%eax"); \
	emit(op " i_%08x", instruction+1); \
	emit("jmp i_%08x", iarg);

#ifdef USE_X87
#define FJ(bits, op) \
	emit("subq $8, %%rsi");\
	emit("flds 4(%%rsi)");\
	emit("fcomps 8(%%rsi)");\
	emit("fnstsw %%ax");\
	emit("testb $" #bits ", %%ah");\
	emit(op " i_%08x", instruction+1);\
	emit("jmp i_%08x", iarg);
#define XJ(x)
#else
#define FJ(x, y)
#define XJ(op) \
	emit("subq $8, %%rsi");\
	emit("movss 4(%%rsi), %%xmm0");\
	emit("ucomiss 8(%%rsi), %%xmm0");\
	emit("jp i_%08x", instruction+1);\
	emit(op " i_%08x", instruction+1);\
	emit("jmp i_%08x", iarg);
#endif

#define SIMPLE(op) \
	emit("subq $4, %%rsi"); \
	emit("movl 4(%%rsi), %%eax"); \
	emit(op " %%eax, 0(%%rsi)");

#ifdef USE_X87
#define FSIMPLE(op) \
	emit("subq $4, %%rsi"); \
	emit("flds 0(%%rsi)"); \
	emit(op " 4(%%rsi)"); \
	emit("fstps 0(%%rsi)");
#define XSIMPLE(op)
#else
#define FSIMPLE(op)
#define XSIMPLE(op) \
	emit("subq $4, %%rsi"); \
	emit("movss 0(%%rsi), %%xmm0"); \
	emit(op " 4(%%rsi), %%xmm0"); \
	emit("movss %%xmm0, 0(%%rsi)");
#endif

#define SHIFT(op) \
	emit("subq $4, %%rsi"); \
	emit("movl 4(%%rsi), %%ecx"); \
	emit("movl 0(%%rsi), %%eax"); \
	emit(op " %%cl, %%eax"); \
	emit("movl %%eax, 0(%%rsi)");

#if 1
#define RANGECHECK(reg) \
	emit("andl $0x%x, %%" #reg, vm->dataMask);
#elif 0
#define RANGECHECK(reg) \
	emit("pushl %%" #reg); \
	emit("andl $0x%x, %%" #reg, ~vm->dataMask); \
	emit("jz rangecheck_ok_i_%08x", instruction); \
	emit("int3"); \
	emit("rangecheck_ok_i_%08x:", instruction); \
	emit("popl %%" #reg); \
	emit("andl $0x%x, %%" #reg, vm->dataMask);
#else
#define RANGECHECK(reg)
#endif

#ifdef DEBUG_VM
#define NOTIMPL(x) \
	do { Com_Error(ERR_DROP, "instruction not implemented: %s\n", opnames[x]); } while(0)
#else
#define NOTIMPL(x) \
	do { Com_Printf(S_COLOR_RED "instruction not implemented: %x\n", x); vm->compiled = qfalse; return; } while(0)
#endif

static void* getentrypoint(vm_t* vm)
{
       return vm->codeBase+64; // skip ELF header
}

char* mmapfile(const char* fn, size_t* size)
{
	int fd = -1;
	char* mem = NULL;
	struct stat stb;

	fd = open(fn, O_RDONLY);
	if(fd == -1)
		goto out;

	if(fstat(fd, &stb) == -1)
		goto out;

	*size = stb.st_size;

	mem = mmap(NULL, stb.st_size, PROT_READ|PROT_EXEC, MAP_SHARED, fd, 0);
	if(mem == (void*)-1)
		mem = NULL;

out:
	if(fd != -1)
		close(fd);

	return mem;
}

static int doas(char* in, char* out, unsigned char** compiledcode)
{
	unsigned char* mem;
	size_t size = -1;
	pid_t pid;

	Com_Printf("running assembler < %s > %s\n", in, out);
	pid = fork();
	if(pid == -1)
	{
		Com_Printf(S_COLOR_RED "can't fork\n");
		return -1;
	}

	if(!pid)
	{
		char* const argv[] = {
			"as",
			"-o",
			out,
			in,
			NULL
		};

		execvp(argv[0], argv);
		_exit(-1);
	}
	else
	{
		int status;
		if(waitpid(pid, &status, 0) == -1)
		{
			Com_Printf(S_COLOR_RED "can't wait for as: %s\n", strerror(errno));
			return -1;
		}

		if(!WIFEXITED(status))
		{
			Com_Printf(S_COLOR_RED "as died\n");
			return -1;
		}
		if(WEXITSTATUS(status))
		{
			Com_Printf(S_COLOR_RED "as failed with status %d\n", WEXITSTATUS(status));
			return -1;
		}
	}

	Com_Printf("done\n");

	mem = (unsigned char*)mmapfile(out, &size);
	if(!mem)
	{
		Com_Printf(S_COLOR_RED "can't mmap object file %s: %s\n", out, strerror(errno));
		return -1;
	}

	*compiledcode = mem;

	return size;
}

static void block_copy_vm(unsigned dest, unsigned src, unsigned count)
{
	unsigned dataMask = currentVM->dataMask;

	if ((dest & dataMask) != dest
	|| (src & dataMask) != src
	|| ((dest+count) & dataMask) != dest + count
	|| ((src+count) & dataMask) != src + count)
	{
		Com_Error(ERR_DROP, "OP_BLOCK_COPY out of range!\n");
	}

	memcpy(currentVM->dataBase+dest, currentVM->dataBase+src, count);
}

/*
=================
VM_Compile
=================
*/
void VM_Compile( vm_t *vm, vmHeader_t *header ) {
	unsigned char op;
	int pc;
	unsigned instruction;
	char* code;
	unsigned iarg = 0;
	unsigned char barg = 0;
	void* entryPoint;

	char fn_s[2*MAX_QPATH]; // output file for assembler code
	char fn_o[2*MAX_QPATH]; // file written by as
#ifdef DEBUG_VM
	char fn_d[MAX_QPATH]; // disassembled
#endif
	FILE* fh_s;
	int fd_s, fd_o;
	byte* compiledcode;
	int   compiledsize;

	Com_Printf("compiling %s\n", vm->name);

#ifdef DEBUG_VM
	snprintf(fn_s, sizeof(fn_s), "%.63s.s", vm->name);
	snprintf(fn_o, sizeof(fn_o), "%.63s.o", vm->name);
	fd_s = open(fn_s, O_CREAT|O_WRONLY, 0644);
	fd_o = open(fn_o, O_CREAT|O_WRONLY, 0644);
#else
	snprintf(fn_s, sizeof(fn_s), "/tmp/%.63s.s_XXXXXX", vm->name);
	snprintf(fn_o, sizeof(fn_o), "/tmp/%.63s.o_XXXXXX", vm->name);
	fd_s = mkstemp(fn_s);
	fd_o = mkstemp(fn_o);
#endif
	if(fd_s == -1 || fd_o == -1)
	{
		if(fd_s != -1) close(fd_s);
		if(fd_o != -1) close(fd_o);
		unlink(fn_s);
		unlink(fn_o);

		Com_Printf(S_COLOR_RED "can't create temporary file %s for vm\n", fn_s);
		vm->compiled = qfalse;
		return;
	}

#ifdef DEBUG_VM
	strcpy(fn_d,vm->name);
	strcat(fn_d, ".qdasm");

	qdasmout = fopen(fn_d, "w");
#endif

	fh_s = fdopen(fd_s, "wb");
	if(!fh_s)
	{
		Com_Printf(S_COLOR_RED "can't write %s\n", fn_s);
		vm->compiled = qfalse;
		return;
	}

	// translate all instructions
	pc = 0;
	code = (char *)header + header->codeOffset;

	emit("start:");
	emit("or %%r8, %%r8"); // check whether to set up instruction pointers
	emit("jnz main");
	emit("jmp setupinstructionpointers");

	emit("main:");

	for ( instruction = 0; instruction < header->instructionCount; ++instruction )
	{
		op = code[ pc ];
		++pc;

		vm->instructionPointers[instruction] = pc;

#if 0
		emit("nop");
		emit("movq $%d, %%r15", instruction);
		emit("nop");
#endif

		if(op_argsize[op] == 4)
		{
			iarg = *(int*)(code+pc);
			pc += 4;
			Dfprintf(qdasmout, "%s %8u\n", opnames[op], iarg);
		}
		else if(op_argsize[op] == 1)
		{
			barg = code[pc++];
			Dfprintf(qdasmout, "%s %8hhu\n", opnames[op], barg);
		}
		else
		{
			Dfprintf(qdasmout, "%s\n", opnames[op]);
		}
		emit("i_%08x:", instruction);
		switch ( op )
		{
			case OP_UNDEF:
				NOTIMPL(op);
				break;
			case OP_IGNORE:
				emit("nop");
				break;
			case OP_BREAK:
				emit("int3");
				break;
			case OP_ENTER:
				emit("subl $%d, %%edi", iarg);
				RANGECHECK(edi);
				break;
			case OP_LEAVE:
				emit("addl $%d, %%edi", iarg);          // get rid of stack frame
				emit("ret");
				break;
			case OP_CALL:
				emit("movl 0(%%rsi), %%eax");  // get instr from stack
				emit("subq $4, %%rsi");
				emit("movl $%d, 0(%%r8, %%rdi, 1)", instruction+1);  // save next instruction
				emit("orl %%eax, %%eax");
				emit("jl callSyscall%d", instruction);
				emit("movq $%lu, %%rbx", (unsigned long)vm->instructionPointers);
				emit("movl (%%rbx, %%rax, 4), %%eax"); // load new relative jump address
				emit("addq %%r10, %%rax");
				emit("callq *%%rax");
				emit("jmp i_%08x", instruction+1);
				emit("callSyscall%d:", instruction);
//				emit("fnsave 4(%%rsi)");
				emit("push %%rsi");
				emit("push %%rdi");
				emit("push %%r8");
				emit("push %%r9");
				emit("push %%r10");
				emit("movq %%rsp, %%rbx"); // we need to align the stack pointer
				emit("subq $8, %%rbx");    //   |
				emit("andq $127, %%rbx");  //   |
				emit("subq %%rbx, %%rsp"); // <-+
				emit("push %%rbx");
				emit("negl %%eax");        // convert to actual number
				emit("decl %%eax");
				                           // first argument already in rdi
				emit("movq %%rax, %%rsi"); // second argument in rsi
				emit("movq $%lu, %%rax", (unsigned long)callAsmCall);
				emit("callq *%%rax");
				emit("pop %%rbx");
				emit("addq %%rbx, %%rsp");
				emit("pop %%r10");
				emit("pop %%r9");
				emit("pop %%r8");
				emit("pop %%rdi");
				emit("pop %%rsi");
//				emit("frstor 4(%%rsi)");
				emit("addq $4, %%rsi");
				emit("movl %%eax, (%%rsi)"); // store return value
				break;
			case OP_PUSH:
				emit("addq $4, %%rsi");
				break;
			case OP_POP:
				emit("subq $4, %%rsi");
				break;
			case OP_CONST:
				emit("addq $4, %%rsi");
				emit("movl $%d, 0(%%rsi)", iarg);
				break;
			case OP_LOCAL:
				emit("movl %%edi, %%ebx");
				emit("addl $%d,%%ebx", iarg);
				emit("addq $4, %%rsi");
				emit("movl %%ebx, 0(%%rsi)");
				break;
			case OP_JUMP:
				emit("movl 0(%%rsi), %%eax"); // get instr from stack
				emit("subq $4, %%rsi");
				emit("movq $%lu, %%rbx", (unsigned long)vm->instructionPointers);
				emit("movl (%%rbx, %%rax, 4), %%eax"); // load new relative jump address
				emit("addq %%r10, %%rax");
				emit("jmp *%%rax");
				break;
			case OP_EQ:
				IJ("jne");
				break;
			case OP_NE:
				IJ("je");
				break;
			case OP_LTI:
				IJ("jnl");
				break;
			case OP_LEI:
				IJ("jnle");
				break;
			case OP_GTI:
				IJ("jng");
				break;
			case OP_GEI:
				IJ("jnge");
				break;
			case OP_LTU:
				IJ("jnb");
				break;
			case OP_LEU:
				IJ("jnbe");
				break;
			case OP_GTU:
				IJ("jna");
				break;
			case OP_GEU:
				IJ("jnae");
				break;
			case OP_EQF:
				FJ(0x40, "jz");
				XJ("jnz");
				break;
			case OP_NEF:
				FJ(0x40, "jnz");
#ifndef USE_X87
				emit("subq $8, %%rsi");
				emit("movss 4(%%rsi), %%xmm0");
				emit("ucomiss 8(%%rsi), %%xmm0");
				emit("jp dojump_i_%08x", instruction);
				emit("jz i_%08x", instruction+1);
				emit("dojump_i_%08x:", instruction);
				emit("jmp i_%08x", iarg);
#endif
				break;
			case OP_LTF:
				FJ(0x01, "jz");
				XJ("jnc");
				break;
			case OP_LEF:
				FJ(0x41, "jz");
				XJ("ja");
				break;
			case OP_GTF:
				FJ(0x41, "jnz");
				XJ("jbe");
				break;
			case OP_GEF:
				FJ(0x01, "jnz");
				XJ("jb");
				break;
			case OP_LOAD1:
				emit("movl 0(%%rsi), %%eax"); // get value from stack
				RANGECHECK(eax);
				emit("movb 0(%%r8, %%rax, 1), %%al"); // deref into eax
				emit("andq $255, %%rax");
				emit("movl %%eax, 0(%%rsi)"); // store on stack
				break;
			case OP_LOAD2:
				emit("movl 0(%%rsi), %%eax"); // get value from stack
				RANGECHECK(eax);
				emit("movw 0(%%r8, %%rax, 1), %%ax"); // deref into eax
				emit("movl %%eax, 0(%%rsi)"); // store on stack
				break;
			case OP_LOAD4:
				emit("movl 0(%%rsi), %%eax"); // get value from stack
				RANGECHECK(eax); // not a pointer!?
				emit("movl 0(%%r8, %%rax, 1), %%eax"); // deref into eax
				emit("movl %%eax, 0(%%rsi)"); // store on stack
				break;
			case OP_STORE1:
				emit("movl 0(%%rsi), %%eax"); // get value from stack
				emit("andq $255, %%rax");
				emit("movl -4(%%rsi), %%ebx"); // get pointer from stack
				RANGECHECK(ebx);
				emit("movb %%al, 0(%%r8, %%rbx, 1)"); // store in memory
				emit("subq $8, %%rsi");
				break;
			case OP_STORE2:
				emit("movl 0(%%rsi), %%eax"); // get value from stack
				emit("movl -4(%%rsi), %%ebx"); // get pointer from stack
				RANGECHECK(ebx);
				emit("movw %%ax, 0(%%r8, %%rbx, 1)"); // store in memory
				emit("subq $8, %%rsi");
				break;
			case OP_STORE4:
				emit("movl -4(%%rsi), %%ebx"); // get pointer from stack
				RANGECHECK(ebx);
				emit("movl 0(%%rsi), %%ecx"); // get value from stack
				emit("movl %%ecx, 0(%%r8, %%rbx, 1)"); // store in memory
				emit("subq $8, %%rsi");
				break;
			case OP_ARG:
				emit("subq $4, %%rsi");
				emit("movl 4(%%rsi), %%eax"); // get value from stack
				emit("movl $0x%hhx, %%ebx", barg);
				emit("addl %%edi, %%ebx");
				RANGECHECK(ebx);
				emit("movl %%eax, 0(%%r8,%%rbx, 1)"); // store in args space
				break;
			case OP_BLOCK_COPY:

				emit("subq $8, %%rsi");
				emit("push %%rsi");
				emit("push %%rdi");
				emit("push %%r8");
				emit("push %%r9");
				emit("push %%r10");
				emit("movl 4(%%rsi), %%edi");  // 1st argument dest
				emit("movl 8(%%rsi), %%esi");  // 2nd argument src
				emit("movl $%d, %%edx", iarg); // 3rd argument count
				emit("movq $%lu, %%rax", (unsigned long)block_copy_vm);
				emit("callq *%%rax");
				emit("pop %%r10");
				emit("pop %%r9");
				emit("pop %%r8");
				emit("pop %%rdi");
				emit("pop %%rsi");

				break;
			case OP_SEX8:
				emit("movw 0(%%rsi), %%ax");
				emit("andq $255, %%rax");
				emit("cbw");
				emit("cwde");
				emit("movl %%eax, 0(%%rsi)");
				break;
			case OP_SEX16:
				emit("movw 0(%%rsi), %%ax");
				emit("cwde");
				emit("movl %%eax, 0(%%rsi)");
				break;
			case OP_NEGI:
				emit("negl 0(%%rsi)");
				break;
			case OP_ADD:
				SIMPLE("addl");
				break;
			case OP_SUB:
				SIMPLE("subl");
				break;
			case OP_DIVI:
				emit("subq $4, %%rsi");
				emit("movl 0(%%rsi), %%eax");
				emit("cdq");
				emit("idivl 4(%%rsi)");
				emit("movl %%eax, 0(%%rsi)");
				break;
			case OP_DIVU:
				emit("subq $4, %%rsi");
				emit("movl 0(%%rsi), %%eax");
				emit("xorq %%rdx, %%rdx");
				emit("divl 4(%%rsi)");
				emit("movl %%eax, 0(%%rsi)");
				break;
			case OP_MODI:
				emit("subq $4, %%rsi");
				emit("movl 0(%%rsi), %%eax");
				emit("xorl %%edx, %%edx");
				emit("cdq");
				emit("idivl 4(%%rsi)");
				emit("movl %%edx, 0(%%rsi)");
				break;
			case OP_MODU:
				emit("subq $4, %%rsi");
				emit("movl 0(%%rsi), %%eax");
				emit("xorl %%edx, %%edx");
				emit("divl 4(%%rsi)");
				emit("movl %%edx, 0(%%rsi)");
				break;
			case OP_MULI:
				emit("subq $4, %%rsi");
				emit("movl 0(%%rsi), %%eax");
				emit("imull 4(%%rsi)");
				emit("movl %%eax, 0(%%rsi)");
				break;
			case OP_MULU:
				emit("subq $4, %%rsi");
				emit("movl 0(%%rsi), %%eax");
				emit("mull 4(%%rsi)");
				emit("movl %%eax, 0(%%rsi)");
				break;
			case OP_BAND:
				SIMPLE("andl");
				break;
			case OP_BOR:
				SIMPLE("orl");
				break;
			case OP_BXOR:
				SIMPLE("xorl");
				break;
			case OP_BCOM:
				emit("notl 0(%%rsi)");
				break;
			case OP_LSH:
				SHIFT("shl");
				break;
			case OP_RSHI:
				SHIFT("sarl");
				break;
			case OP_RSHU:
				SHIFT("shrl");
				break;
			case OP_NEGF:
#ifdef USE_X87
				emit("flds 0(%%rsi)");
				emit("fchs");
				emit("fstps 0(%%rsi)");
#else
				emit("movl $0x80000000, %%eax");
				emit("xorl %%eax, 0(%%rsi)");
#endif
				break;
			case OP_ADDF:
				FSIMPLE("fadds");
				XSIMPLE("addss");
				break;
			case OP_SUBF:
				FSIMPLE("fsubs");
				XSIMPLE("subss");
				break;
			case OP_DIVF:
				FSIMPLE("fdivs");
				XSIMPLE("divss");
				break;
			case OP_MULF:
				FSIMPLE("fmuls");
				XSIMPLE("mulss");
				break;
			case OP_CVIF:
#ifdef USE_X87
				emit("filds 0(%%rsi)");
				emit("fstps 0(%%rsi)");
#else
				emit("movl 0(%%rsi), %%eax");
				emit("cvtsi2ss %%eax, %%xmm0");
				emit("movss %%xmm0, 0(%%rsi)");
#endif
				break;
			case OP_CVFI:
#ifdef USE_X87
				emit("flds 0(%%rsi)");
				emit("fnstcw 4(%%rsi)");
				emit("movw $0x0F7F, 8(%%rsi)"); // round toward zero
				emit("fldcw 8(%%rsi)");
				emit("fistpl 0(%%rsi)");
				emit("fldcw 4(%%rsi)");
#else
				emit("movss 0(%%rsi), %%xmm0");
				emit("cvttss2si %%xmm0, %%eax");
				emit("movl %%eax, 0(%%rsi)");
#endif
				break;
			default:
				NOTIMPL(op);
				break;
		}
	}


	emit("setupinstructionpointers:");
	emit("movq $%lu, %%rax", (unsigned long)vm->instructionPointers);
	for ( instruction = 0; instruction < header->instructionCount; ++instruction )
	{
		emit("movl $i_%08x-start, %d(%%rax)", instruction, instruction*4);
	}
	emit("ret");

	emit("debugger:");
	if(1);
	{
		int i = 6;
		while(i--)
		{
			emit("nop");
			emit("int3");
		}
	}

	fflush(fh_s);
	fclose(fh_s);

	compiledsize = doas(fn_s, fn_o, &compiledcode);
	if(compiledsize == -1)
	{
		vm->compiled = qfalse;
		goto out;
	}

	vm->codeBase   = compiledcode; // remember to skip ELF header!
	vm->codeLength = compiledsize;

	vm->destroy = VM_Destroy_Compiled;
	
	entryPoint = getentrypoint(vm);

//	__asm__ __volatile__ ("int3");
	Com_Printf("computing jump table\n");

	// call code with r8 set to zero to set up instruction pointers
	__asm__ __volatile__ (
		"	xorq %%r8,%%r8		\r\n" \
		"	movq %0,%%r10		\r\n" \
		"	callq *%%r10		\r\n" \
		:
		: "m" (entryPoint)
		: "%r8", "%r10", "%rax"
	);

#ifdef DEBUG_VM
	fflush(qdasmout);
	fclose(qdasmout);
#endif

	Com_Printf( "VM file %s compiled to %i bytes of code (%p - %p)\n", vm->name, vm->codeLength, vm->codeBase, vm->codeBase+vm->codeLength );

out:
	close(fd_o);

#ifndef DEBUG_VM
	if(!com_developer->integer)
	{
		unlink(fn_o);
		unlink(fn_s);
	}
#endif
}


void VM_Destroy_Compiled(vm_t* self)
{
	munmap(self->codeBase, self->codeLength);
}

/*
==============
VM_CallCompiled

This function is called directly by the generated code
==============
*/

#ifdef DEBUG_VM
static char* memData;
#endif

int	VM_CallCompiled( vm_t *vm, int *args ) {
	int		programCounter;
	int		programStack;
	int		stackOnEntry;
	byte	*image;
	void	*entryPoint;
	void	*opStack;
	int stack[1024] = { 0xDEADBEEF };

	currentVM = vm;

	++vm->callLevel;
//	Com_Printf("entering %s level %d, call %d, arg1 = 0x%x\n", vm->name, vm->callLevel, args[0], args[1]);

	// interpret the code
	vm->currentlyInterpreting = qtrue;

//	callMask = vm->dataMask;

	// we might be called recursively, so this might not be the very top
	programStack = vm->programStack;
	stackOnEntry = programStack;

	// set up the stack frame 
	image = vm->dataBase;
#ifdef DEBUG_VM
	memData = (char*)image;
#endif

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
	*(int *)&image[ programStack + 4 ] = 0x77777777;	// return stack
	*(int *)&image[ programStack ] = -1;	// will terminate the loop on return

	// off we go into generated code...
	entryPoint = getentrypoint(vm);
	opStack = &stack;

	__asm__ __volatile__ (
		"	movq %5,%%rsi		\r\n" \
		"	movl %4,%%edi		\r\n" \
		"	movq %2,%%r10		\r\n" \
		"	movq %3,%%r8		\r\n" \
		"       subq $24, %%rsp # fix alignment as call pushes one value \r\n" \
		"	callq *%%r10		\r\n" \
		"       addq $24, %%rsp          \r\n" \
		"	movl %%edi, %0		\r\n" \
		"	movq %%rsi, %1		\r\n" \
		: "=m" (programStack), "=m" (opStack)
		: "m" (entryPoint), "m" (vm->dataBase), "m" (programStack), "m" (opStack)
		: "%rsi", "%rdi", "%rax", "%rbx", "%rcx", "%rdx", "%r8", "%r10", "%r15", "%xmm0"
	);

	if ( opStack != &stack[1] ) {
		Com_Error( ERR_DROP, "opStack corrupted in compiled code (offset %ld)\n", (long int) ((void *) &stack[1] - opStack));
	}
	if ( programStack != stackOnEntry - 48 ) {
		Com_Error( ERR_DROP, "programStack corrupted in compiled code\n" );
	}

//	Com_Printf("exiting %s level %d\n", vm->name, vm->callLevel);
	--vm->callLevel;
	vm->programStack = stackOnEntry;

	return *(int *)opStack;
}
