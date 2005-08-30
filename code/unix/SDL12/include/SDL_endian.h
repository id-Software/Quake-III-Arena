/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

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
 "@(#) $Id: SDL_endian.h,v 1.5 2002/04/11 14:35:13 slouken Exp $";
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

/* The macros used to swap values */
/* Try to use superfast macros on systems that support them */
#ifdef linux
#include <asm/byteorder.h>
#ifdef __arch__swab16
#define SDL_Swap16  __arch__swab16
#endif
#ifdef __arch__swab32
#define SDL_Swap32  __arch__swab32
#endif
#endif /* linux */
/* Use inline functions for compilers that support them, and static
   functions for those that do not.  Because these functions become
   static for compilers that do not support inline functions, this
   header should only be included in files that actually use them.
*/
#ifndef SDL_Swap16
static __inline__ Uint16 SDL_Swap16(Uint16 D) {
	return((D<<8)|(D>>8));
}
#endif
#ifndef SDL_Swap32
static __inline__ Uint32 SDL_Swap32(Uint32 D) {
	return((D<<24)|((D<<8)&0x00FF0000)|((D>>8)&0x0000FF00)|(D>>24));
}
#endif
#ifdef SDL_HAS_64BIT_TYPE
#ifndef SDL_Swap64
static __inline__ Uint64 SDL_Swap64(Uint64 val) {
	Uint32 hi, lo;

	/* Separate into high and low 32-bit values and swap them */
	lo = (Uint32)(val&0xFFFFFFFF);
	val >>= 32;
	hi = (Uint32)(val&0xFFFFFFFF);
	val = SDL_Swap32(lo);
	val <<= 32;
	val |= SDL_Swap32(hi);
	return(val);
}
#endif
#else
#ifndef SDL_Swap64
/* This is mainly to keep compilers from complaining in SDL code.
   If there is no real 64-bit datatype, then compilers will complain about
   the fake 64-bit datatype that SDL provides when it compiles user code.
*/
#define SDL_Swap64(X)	(X)
#endif
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

