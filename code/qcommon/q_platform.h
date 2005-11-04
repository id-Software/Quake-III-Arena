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

#if (defined _M_IX86 || defined __i386__) && \
		!defined __sun && !defined(C_ONLY)
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

// buildstring will be incorporated into the version string
#ifdef _MSC_VER
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING "win-x86"
#elif defined _M_ALPHA
#define CPUSTRING "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING "win-x86-debug"
#elif defined _M_ALPHA
#define CPUSTRING "win-AXP-debug"
#endif
#endif
#elif defined __MINGW32__
#ifdef NDEBUG
#ifdef __i386__
#define CPUSTRING "mingw-x86"
#endif
#else
#ifdef __i386__
#define CPUSTRING "mingw-x86-debug"
#endif
#endif
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

#ifdef __ppc__
#define CPUSTRING "MacOSX-ppc"
#elif defined __i386__
#define CPUSTRING "MacOSX-i386"
#else
#define CPUSTRING "MacOSX-other"
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

#define CPUSTRING "MacOS-PPC"

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

#ifdef __i386__
#define CPUSTRING "linux-i386"
#elif defined __axp__
#define CPUSTRING "linux-alpha"
#elif defined __x86_64__
#define CPUSTRING "linux-x86_64"
#elif defined __powerpc64__
#define CPUSTRING "linux-ppc64"
#elif defined __powerpc__
#define CPUSTRING "linux-ppc"
#elif defined __s390__
#define CPUSTRING "linux-s390"
#elif defined __s390x__
#define CPUSTRING "linux-s390x"
#elif defined __ia64__
#define CPUSTRING "linux-ia64"
#else
#define CPUSTRING "linux-other"
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

#ifdef __i386__
#define CPUSTRING "freebsd-i386"
#elif defined __axp__
#define CPUSTRING "freebsd-alpha"
#else
#define CPUSTRING "freebsd-other"
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

#ifdef __i386__
#define CPUSTRING "Solaris-i386"
#elif defined __sparc
#define CPUSTRING "Solaris-sparc"
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

#define CPUSTRING "q3vm"

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
#ifndef CPUSTRING
#error "CPUSTRING not defined"
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

#endif

#endif
