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

; MASM version of snapvector conversion function using SSE or FPU
; assume __cdecl calling convention is being used for x86, __fastcall for x64
;
; function prototype:
; void qsnapvector(vec3_t vec)

IFNDEF idx64
.model flat, c
ENDIF

.data

  ALIGN 16
  ssemask DWORD 0FFFFFFFFh, 0FFFFFFFFh, 0FFFFFFFFh, 00000000h
  ssecw DWORD 00001F80h

IFNDEF idx64
  fpucw WORD 037Fh
ENDIF

.code

IFDEF idx64
; qsnapvector using SSE

  qsnapvectorsse PROC
    sub rsp, 8
	stmxcsr [rsp]				; save SSE control word
	ldmxcsr ssecw				; set to round nearest

    push rdi
	mov rdi, rcx				; maskmovdqu uses rdi as implicit memory operand
	movaps xmm1, ssemask		; initialize the mask register for maskmovdqu
    movups xmm0, [rdi]			; here is stored our vector. Read 4 values in one go
	cvtps2dq xmm0, xmm0			; convert 4 single fp to int
	cvtdq2ps xmm0, xmm0			; convert 4 int to single fp
	maskmovdqu xmm0, xmm1		; write 3 values back to memory
	pop rdi

	ldmxcsr [rsp]				; restore sse control word to old value
	add rsp, 8
	ret
  qsnapvectorsse ENDP

ELSE

  qsnapvectorsse PROC
	sub esp, 8
	stmxcsr [esp]				; save SSE control word
	ldmxcsr ssecw				; set to round nearest

    push edi
	mov edi, dword ptr 16[esp]	; maskmovdqu uses edi as implicit memory operand
	movaps xmm1, ssemask		; initialize the mask register for maskmovdqu
    movups xmm0, [edi]			; here is stored our vector. Read 4 values in one go
	cvtps2dq xmm0, xmm0			; convert 4 single fp to int
	cvtdq2ps xmm0, xmm0			; convert 4 int to single fp
	maskmovdqu xmm0, xmm1		; write 3 values back to memory
	pop edi

	ldmxcsr [esp]				; restore sse control word to old value
	add esp, 8
	ret
  qsnapvectorsse ENDP

  qroundx87 macro src
	fld dword ptr src
	fistp dword ptr src
	fild dword ptr src
	fstp dword ptr src
  endm    

  qsnapvectorx87 PROC
	mov eax, dword ptr 4[esp]
	sub esp, 2
	fnstcw word ptr [esp]
	fldcw fpucw
	qroundx87 [eax]
	qroundx87 4[eax]
	qroundx87 8[eax]
	fldcw [esp]
	add esp, 2
  qsnapvectorx87 ENDP

ENDIF

end
