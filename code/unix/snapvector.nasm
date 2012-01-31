;===========================================================================
;Copyright (C) 1999-2005 Id Software, Inc.
;
;This file is part of Quake III Arena source code.
;
;Quake III Arena source code is free software; you can redistribute it
;and/or modify it under the terms of the GNU General Public License as
;published by the Free Software Foundation; either version 2 of the License,
;or (at your option) any later version.
;
;Quake III Arena source code is distributed in the hope that it will be
;useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with Foobar; if not, write to the Free Software
;Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
;===========================================================================

;
; Sys_SnapVector NASM code (Andrew Henderson)
; See win32/win_shared.c for the Win32 equivalent
; This code is provided to ensure that the
;  rounding behavior (and, if necessary, the
;  precision) of DLL and QVM code are identical
;  e.g. for network-visible operations.
; See ftol.nasm for operations on a single float,
;  as used in compiled VM and DLL code that does
;  not use this system trap.
;


segment .data

fpucw	dd	0
cw037F  dd 0x037F   ; Rounding to nearest (even). 

segment .text

; void Sys_SnapVector( float *v )
global Sys_SnapVector
Sys_SnapVector:
	push 	eax
	push 	ebp
	mov	ebp, esp

	fnstcw	[fpucw]
	mov	eax, dword [ebp + 12]
	fldcw	[cw037F]	
	fld	dword [eax]
	fistp	dword [eax]
	fild	dword [eax]
	fstp	dword [eax]
	fld	dword [eax + 4]
	fistp	dword [eax + 4]
	fild	dword [eax + 4]
	fstp	dword [eax + 4]
	fld	dword [eax + 8]
	fistp	dword [eax + 8]
	fild	dword [eax + 8]
	fstp	dword [eax + 8]
	fldcw	[fpucw]
	
	pop ebp
	pop eax
	ret
	
; void Sys_SnapVectorCW( float *v, unsigned short int cw )
global Sys_SnapVectorCW
Sys_SnapVector_cw:
	push 	eax
	push 	ebp
	mov	ebp, esp

	fnstcw	[fpucw]
	mov	eax, dword [ebp + 12]
	fldcw	[ebp + 16]	
	fld	dword [eax]
	fistp	dword [eax]
	fild	dword [eax]
	fstp	dword [eax]	
	fld	dword [eax + 4]
	fistp	dword [eax + 4]
	fild	dword [eax + 4]
	fstp	dword [eax + 4]
	fld	dword [eax + 8]
	fistp	dword [eax + 8]
	fild	dword [eax + 8]
	fstp	dword [eax + 8]
	fldcw	[fpucw]
	
	pop ebp
	pop eax
	ret