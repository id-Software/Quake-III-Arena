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

.code

; Call to compiled code after setting up the register environment for the VM
; prototype:
; uint8_t qvmcall64(int *programStack, int *opStack, intptr_t *instructionPointers, byte *dataBase);
;
; This call-stub has its own custom calling convention due to pushing all non-volatile registers
; to the stack. The game uses set/longjmp which on Windows uses "RtlUnwindEx" to unwind the callstack.
; This function cannot be unwound by default due to the custom calling convention.
; To allow unwinding, we need to add custom SEH unwind data to the function.

qvmcall64 PROC FRAME
  push r12							; push all non-volatile registers to stack
  .pushreg r12
  push r13
  .pushreg r13
  push r14
  .pushreg r14
  push r15
  .pushreg r15
  push rdi
  .pushreg rdi
  push rsi
  .pushreg rsi
  push rbx
  .pushreg rbx
  push rbp
  .pushreg rbp
  
  ; need to save pointer in rcx so we can write back the programData value to caller
  push rcx
  .pushreg rcx
  
  .endprolog						; custom unwind data ends here

  ; registers r8 and r9 have correct value already thanx to __fastcall
  xor rbx, rbx						; opStackOfs starts out being 0
  mov rdi, rdx						; opStack
  mov esi, dword ptr [rcx]			; programStack
  
  call qword ptr [r8]				; instructionPointers[0] is also the entry point

  pop rcx

  mov dword ptr [rcx], esi			; write back the programStack value
  mov al, bl						; return opStack offset

  pop rbp							; restore all non-volatile registers after the call
  pop rbx
  pop rsi
  pop rdi
  pop r15
  pop r14
  pop r13
  pop r12
  
  ret
qvmcall64 ENDP

end
