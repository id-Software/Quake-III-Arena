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
 "@(#) $Id: SDL_cpuinfo.h,v 1.5 2004/01/24 05:47:18 slouken Exp $";
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* CPU feature detection for SDL                                       */

#ifndef _SDL_cpuinfo_h
#define _SDL_cpuinfo_h

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* This function returns true if the CPU has the RDTSC instruction
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasRDTSC();

/* This function returns true if the CPU has MMX features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasMMX();

/* This function returns true if the CPU has MMX Ext. features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasMMXExt();

/* This function returns true if the CPU has 3DNow features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_Has3DNow();

/* This function returns true if the CPU has 3DNow! Ext. features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_Has3DNowExt();

/* This function returns true if the CPU has SSE features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE();

/* This function returns true if the CPU has SSE2 features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasSSE2();

/* This function returns true if the CPU has AltiVec features
 */
extern DECLSPEC SDL_bool SDLCALL SDL_HasAltiVec();

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_cpuinfo_h */
