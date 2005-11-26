/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_endian.h,v 1.15 2005/03/30 12:38:03 pmandin Exp $";
#endif

/* Functions for reading and writing endian-specific values */

#ifndef _SDL_endian_h
#define _SDL_endian_h

/* These functions read and write data of the specified endianness, 
   dynamically translating to the host machine endianness.

   e.g.: If you want to read a 16 bit value on big-endian machine from
         an open file containing little endian values, you would use:
		value = SDL_ReadLE16(rp);
         Note that the read/write functions use SDL_RWops pointers
         instead of FILE pointers.  This allows you to read and write
         endian values from large chunks of memory as well as files 
         and other data sources.
*/

#include <stdio.h>

#include "SDL_types.h"
#include "SDL_rwops.h"
#include "SDL_byteorder.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Use inline functions for compilers that support them, and static
   functions for those that do not.  Because these functions become
   static for compilers that do not support inline functions, this
   header should only be included in files that actually use them.
*/
#if defined(__GNUC__) && defined(__i386__)
static __inline__ Uint16 SDL_Swap16(Uint16 x)
{
	__asm__("xchgb %b0,%h0" : "=q" (x) :  "0" (x));
	return x;
}
#elif defined(__GNUC__) && defined(__x86_64__)
static __inline__ Uint16 SDL_Swap16(Uint16 x)
{
	__asm__("xchgb %b0,%h0" : "=Q" (x) :  "0" (x));
	return x;
}
#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
static __inline__ Uint16 SDL_Swap16(Uint16 x)
{
	Uint16 result;

	__asm__("rlwimi %0,%2,8,16,23" : "=&r" (result) : "0" (x >> 8), "r" (x));
	return result;
}
#elif defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
static __inline__ Uint16 SDL_Swap16(Uint16 x)
{
	__asm__("rorw #8,%0" : "=d" (x) :  "0" (x) : "cc");
	return x;
}
#else
static __inline__ Uint16 SDL_Swap16(Uint16 x) {
	return((x<<8)|(x>>8));
}
#endif

#if defined(__GNUC__) && defined(__i386__)
static __inline__ Uint32 SDL_Swap32(Uint32 x)
{
	__asm__("bswap %0" : "=r" (x) : "0" (x));
	return x;
}
#elif defined(__GNUC__) && defined(__x86_64__)
static __inline__ Uint32 SDL_Swap32(Uint32 x)
{
	__asm__("bswapl %0" : "=r" (x) : "0" (x));
	return x;
}
#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
static __inline__ Uint32 SDL_Swap32(Uint32 x)
{
	Uint32 result;

	__asm__("rlwimi %0,%2,24,16,23" : "=&r" (result) : "0" (x>>24), "r" (x));
	__asm__("rlwimi %0,%2,8,8,15"   : "=&r" (result) : "0" (result),    "r" (x));
	__asm__("rlwimi %0,%2,24,0,7"   : "=&r" (result) : "0" (result),    "r" (x));
	return result;
}
#elif defined(__GNUC__) && (defined(__M68000__) || defined(__M68020__))
static __inline__ Uint32 SDL_Swap32(Uint32 x)
{
	__asm__("rorw #8,%0\n\tswap %0\n\trorw #8,%0" : "=d" (x) :  "0" (x) : "cc");
	return x;
}
#else
static __inline__ Uint32 SDL_Swap32(Uint32 x) {
	return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}
#endif

#ifdef SDL_HAS_64BIT_TYPE
#if defined(__GNUC__) && defined(__i386__)
static __inline__ Uint64 SDL_Swap64(Uint64 x)
{
	union { 
		struct { Uint32 a,b; } s;
		Uint64 u;
	} v;
	v.u = x;
	__asm__("bswapl %0 ; bswapl %1 ; xchgl %0,%1" 
	        : "=r" (v.s.a), "=r" (v.s.b) 
	        : "0" (v.s.a), "1" (v.s.b)); 
	return v.u;
}
#elif defined(__GNUC__) && defined(__x86_64__)
static __inline__ Uint64 SDL_Swap64(Uint64 x)
{
	__asm__("bswapq %0" : "=r" (x) : "0" (x));
	return x;
}
#else
static __inline__ Uint64 SDL_Swap64(Uint64 x)
{
	Uint32 hi, lo;

	/* Separate into high and low 32-bit values and swap them */
	lo = (Uint32)(x&0xFFFFFFFF);
	x >>= 32;
	hi = (Uint32)(x&0xFFFFFFFF);
	x = SDL_Swap32(lo);
	x <<= 32;
	x |= SDL_Swap32(hi);
	return(x);
}
#endif
#else
/* This is mainly to keep compilers from complaining in SDL code.
   If there is no real 64-bit datatype, then compilers will complain about
   the fake 64-bit datatype that SDL provides when it compiles user code.
*/
#define SDL_Swap64(X)	(X)
#endif /* SDL_HAS_64BIT_TYPE */


/* Byteswap item from the specified endianness to the native endianness */
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(X)	(X)
#define SDL_SwapLE32(X)	(X)
#define SDL_SwapLE64(X)	(X)
#define SDL_SwapBE16(X)	SDL_Swap16(X)
#define SDL_SwapBE32(X)	SDL_Swap32(X)
#define SDL_SwapBE64(X)	SDL_Swap64(X)
#else
#define SDL_SwapLE16(X)	SDL_Swap16(X)
#define SDL_SwapLE32(X)	SDL_Swap32(X)
#define SDL_SwapLE64(X)	SDL_Swap64(X)
#define SDL_SwapBE16(X)	(X)
#define SDL_SwapBE32(X)	(X)
#define SDL_SwapBE64(X)	(X)
#endif

/* Read an item of the specified endianness and return in native format */
extern DECLSPEC Uint16 SDLCALL SDL_ReadLE16(SDL_RWops *src);
extern DECLSPEC Uint16 SDLCALL SDL_ReadBE16(SDL_RWops *src);
extern DECLSPEC Uint32 SDLCALL SDL_ReadLE32(SDL_RWops *src);
extern DECLSPEC Uint32 SDLCALL SDL_ReadBE32(SDL_RWops *src);
extern DECLSPEC Uint64 SDLCALL SDL_ReadLE64(SDL_RWops *src);
extern DECLSPEC Uint64 SDLCALL SDL_ReadBE64(SDL_RWops *src);

/* Write an item of native format to the specified endianness */
extern DECLSPEC int SDLCALL SDL_WriteLE16(SDL_RWops *dst, Uint16 value);
extern DECLSPEC int SDLCALL SDL_WriteBE16(SDL_RWops *dst, Uint16 value);
extern DECLSPEC int SDLCALL SDL_WriteLE32(SDL_RWops *dst, Uint32 value);
extern DECLSPEC int SDLCALL SDL_WriteBE32(SDL_RWops *dst, Uint32 value);
extern DECLSPEC int SDLCALL SDL_WriteLE64(SDL_RWops *dst, Uint64 value);
extern DECLSPEC int SDLCALL SDL_WriteBE64(SDL_RWops *dst, Uint64 value);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_endian_h */
