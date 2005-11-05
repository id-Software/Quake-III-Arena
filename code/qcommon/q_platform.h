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
//
#ifndef __Q_PLATFORM_H
#define __Q_PLATFORM_H

// this is for determining if we have an asm version of a C function
#ifdef Q3_VM

#define id386 0
#define idppc 0
#define idppc_altivec 0

#else

#if (defined _M_IX86 || defined __i386__) && !defined(C_ONLY)
#define id386 1
#else
#define id386 0
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || \
		defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
#define idppc 1
#if defined(__VEC__)
#define idppc_altivec 1
#else
#define idppc_altivec 0
#endif
#else
#define idppc 0
#define idppc_altivec 0
#endif

#endif

#ifndef __ASM_I386__ // don't include the C bits if included from qasm.h

// for windows fastcall option

#define QDECL

short	ShortSwap (short l);
int		LongSwap (int l);
float	FloatSwap (const float *f);

//================================================================= WIN32 ===

#ifdef _WIN32

#undef QDECL
#define QDECL __cdecl

#ifdef _MSC_VER
#define OS_STRING "win_msvc"

#ifdef _M_IX86
#define ARCH_STRING "x86"
#elif defined _M_ALPHA
#define ARCH_STRING "AXP"
#else
#error "Unsupported architecture"
#endif

#elif defined __MINGW32__
#define OS_STRING "win_mingw"

#ifdef __i386__
#define ARCH_STRING "x86"
#else
#error "Unsupported architecture"
#endif

#else
#error "Unsupported compiler"
#endif


#define ID_INLINE __inline

static ID_INLINE short BigShort( short l) { return ShortSwap(l); }
#define LittleShort
static ID_INLINE int BigLong(int l) { return LongSwap(l); }
#define LittleLong
static ID_INLINE float BigFloat(const float l) { return FloatSwap(&l); }
#define LittleFloat

#define PATH_SEP '\\'

#endif

//============================================================== MAC OS X ===

#if defined(MACOS_X)

#define __cdecl
#define __declspec(x)
#define stricmp strcasecmp
#define ID_INLINE inline

#define OS_STRING "macosx"

#ifdef __ppc__
#define ARCH_STRING "ppc"
#elif defined __i386__
#define ARCH_STRING "i386"
#else
#error "Unsupported architecture"
#endif

#define PATH_SEP '/'

#define __rlwimi(out, in, shift, maskBegin, maskEnd) \
	asm("rlwimi %0,%1,%2,%3,%4" : "=r" (out) : "r" (in), \
			"i" (shift), "i" (maskBegin), "i" (maskEnd))
#define __dcbt(addr, offset) asm("dcbt %0,%1" : : "b" (addr), "r" (offset))

static ID_INLINE unsigned int __lwbrx(register void *addr,
		register int offset) {
	register unsigned int word;

	asm("lwbrx %0,%2,%1" : "=r" (word) : "r" (addr), "b" (offset));
	return word;
}

static ID_INLINE unsigned short __lhbrx(register void *addr,
		register int offset) {
	register unsigned short halfword;

	asm("lhbrx %0,%2,%1" : "=r" (halfword) : "r" (addr), "b" (offset));
	return halfword;
}

static ID_INLINE float __fctiw(register float f) {
	register float fi;

	asm("fctiw %0,%1" : "=f" (fi) : "f" (f));
	return fi;
}

#define BigShort
static ID_INLINE short LittleShort(short l) { return ShortSwap(l); }
#define BigLong
static ID_INLINE int LittleLong(int l) { return LongSwap(l); }
#define BigFloat
static ID_INLINE float LittleFloat(const float l) { return FloatSwap(&l); }

#endif

//=================================================================== MAC ===

#ifdef __MACOS__

#include <MacTypes.h>
#define ID_INLINE inline

#define OS_STRING "macos"
#define ARCH_STRING "ppc"

#define PATH_SEP ':'

void Sys_PumpEvents( void );

#define BigShort
static ID_INLINE short LittleShort(short l) { return ShortSwap(l); }
#define BigLong
static ID_INLINE int LittleLong(int l) { return LongSwap(l); }
#define BigFloat
static ID_INLINE float LittleFloat(const float l) { return FloatSwap(&l); }

#endif

//================================================================= LINUX ===

// the mac compiler can't handle >32k of locals, so we
// just waste space and make big arrays static...
#ifdef __linux__

// bk001205 - from Makefile
#define stricmp strcasecmp

#define ID_INLINE inline

#define OS_STRING "linux"

#if defined __i386__
#define ARCH_STRING "i386"
#elif defined __x86_64__
#define ARCH_STRING "x86_64"
#elif defined __powerpc64__
#define ARCH_STRING "ppc64"
#elif defined __powerpc__
#define ARCH_STRING "ppc"
#elif defined __s390__
#define ARCH_STRING "s390"
#elif defined __s390x__
#define ARCH_STRING "s390x"
#elif defined __ia64__
#define ARCH_STRING "ia64"
#elif defined __alpha__
#define ARCH_STRING "alpha"
#elif defined __sparc__
#define ARCH_STRING "sparc"
#elif defined __arm__
#define ARCH_STRING "arm"
#elif defined __cris__
#define ARCH_STRING "cris"
#elif defined __hppa__
#define ARCH_STRING "hppa"
#elif defined __mips__
#define ARCH_STRING "mips"
#elif defined __sh__
#define ARCH_STRING "sh"
#else
#error "Unsupported architecture"
#endif

#define PATH_SEP '/'

#if __FLOAT_WORD_ORDER == __LITTLE_ENDIAN
ID_INLINE static short BigShort( short l) { return ShortSwap(l); }
#define LittleShort
ID_INLINE static int BigLong(int l) { return LongSwap(l); }
#define LittleLong
ID_INLINE static float BigFloat(const float l) { return FloatSwap(&l); }
#define LittleFloat
#else
#define BigShort
ID_INLINE static short LittleShort(short l) { return ShortSwap(l); }
#define BigLong
ID_INLINE static int LittleLong(int l) { return LongSwap(l); }
#define BigFloat
ID_INLINE static float LittleFloat(const float l) { return FloatSwap(&l); }
#endif

#endif

//=============================================================== FreeBSD ===

#ifdef __FreeBSD__ // rb010123

#define stricmp strcasecmp

#define ID_INLINE inline

#define OS_STRING "freebsd"

#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __axp__
#define ARCH_STRING "alpha"
#else
#error "Unsupported architecture"
#endif

#define PATH_SEP '/'

#if !idppc
static short BigShort( short l) { return ShortSwap(l); }
#define LittleShort
static int BigLong(int l) { return LongSwap(l); }
#define LittleLong
static float BigFloat(const float l) { return FloatSwap(&l); }
#define LittleFloat
#else
#define BigShort
static short LittleShort(short l) { return ShortSwap(l); }
#define BigLong
static int LittleLong(int l) { return LongSwap(l); }
#define BigFloat
static float LittleFloat(const float l) { return FloatSwap(&l); }
#endif

#endif

//================================================================= SUNOS ===

#ifdef __sun

#include <sys/isa_defs.h>
#include <sys/byteorder.h>

// bk001205 - from Makefile
#define stricmp strcasecmp

#define ID_INLINE inline

#define OS_STRING "solaris"

#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __sparc
#define ARCH_STRING "sparc"
#else
#error "Unsupported architecture"
#endif

#define PATH_SEP '/'

#if defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#define BigShort
ID_INLINE static short LittleShort(short l) { return ShortSwap(l); }
#define BigLong
ID_INLINE static int LittleLong(int l) { return LongSwap(l); }
#define BigFloat
ID_INLINE static float LittleFloat(const float l) { return FloatSwap(&l); }

#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
ID_INLINE static short BigShort( short l) { return ShortSwap(l); }
#define LittleShort
ID_INLINE static int BigLong(int l) { return LongSwap(l); }
#define LittleLong
ID_INLINE static float BigFloat(const float l) { return FloatSwap(&l); }
#define LittleFloat

#else
#error "Either _BIG_ENDIAN or _LITTLE_ENDIAN must be #defined, but not both."
#endif

#endif

//================================================================== Q3VM ===

#ifdef Q3_VM

#define ID_INLINE

#define OS_STRING "q3vm"
#define ARCH_STRING "bytecode"

#define PATH_SEP '/'

#define LittleShort
#define LittleLong
#define LittleFloat
#define BigShort
#define BigLong
#define BigFloat

#endif

//===========================================================================

//catch missing defines in above blocks
#if !defined( OS_STRING ) || !defined( ARCH_STRING )
#error "OS_STRING or ARCH_STRING not defined"
#endif

#ifndef ID_INLINE
#error "ID_INLINE not defined"
#endif

#ifndef PATH_SEP
#error "PATH_SEP not defined"
#endif

#if !defined(BigLong) && !defined(LittleLong)
#error "Endianness not defined"
#endif

#ifdef NDEBUG
#define PLATFORM_STRING OS_STRING "-" ARCH_STRING
#else
#define PLATFORM_STRING OS_STRING "-" ARCH_STRING "-debug"
#endif

#endif

#endif
