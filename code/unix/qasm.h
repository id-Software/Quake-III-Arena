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
#ifndef __ASM_I386__
#define __ASM_I386__

#include "../qcommon/q_platform.h"

#if defined(__MINGW32__) || defined(MACOS_X)
#undef ELF
#endif

#ifdef __ELF__
.section .note.GNU-stack,"",@progbits
#endif

#ifdef ELF
#define C(label) label
#else
#define C(label) _##label
#endif

// plane_t structure
// !!! if this is changed, it must be changed in q_shared.h too !!!
#define pl_normal	0
#define pl_dist		12
#define pl_type		16
#define pl_signbits	17
#define pl_pad		18
#define pl_size		20

#endif
