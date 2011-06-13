; ===========================================================================
; Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
; 
; This file is part of Quake III Arena source code.
; 
; Quake III Arena source code is free software; you can redistribute it
; and/or modify it under the terms of the GNU General Public License as
; published by the Free Software Foundation; either version 2 of the License,
; or (at your option) any later version.
; 
; Quake III Arena source code is distributed in the hope that it will be
; useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with Quake III Arena source code; if not, write to the Free Software
; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
; ===========================================================================

; Call wrapper for vm_x86 when built with MSVC in 64 bit mode,
; since MSVC does not support inline x64 assembler code anymore.
;
; assumes __fastcall calling convention

DoSyscall PROTO

.code

; Call to static void DoSyscall(int syscallNum, int programStack, int *opStackBase, uint8_t opStackOfs, intptr_t arg)

qsyscall64 PROC
  sub rsp, 28h						; after this esp will be aligned to 16 byte boundary
  mov qword ptr [rsp + 20h], rcx	; 5th parameter "arg" is passed on stack
  mov r9b, bl						; opStackOfs
  mov r8, rdi						; opStackBase
  mov edx, esi						; programStack
  mov ecx, eax						; syscallNum
  mov rax, DoSyscall				; store call address of DoSyscall in rax
  call rax
  add rsp, 28h
  ret
qsyscall64 ENDP


; Call to compiled code after setting up the register environment for the VM
; prototype:
; uint8_t qvmcall64(int *programStack, int *opStack, intptr_t *instructionPointers, byte *dataBase);

qvmcall64 PROC
  push rsi							; push non-volatile registers to stack
  push rdi
  push rbx
  ; need to save pointer in rcx so we can write back the programData value to caller
  push rcx

  ; registers r8 and r9 have correct value already thanx to __fastcall
  xor rbx, rbx						; opStackOfs starts out being 0
  mov rdi, rdx						; opStack
  mov esi, dword ptr [rcx]			; programStack
  
  call qword ptr [r8]				; instructionPointers[0] is also the entry point

  pop rcx

  mov dword ptr [rcx], esi			; write back the programStack value
  mov al, bl						; return opStack offset

  pop rbx
  pop rdi
  pop rsi
  
  ret
qvmcall64 ENDP

end
