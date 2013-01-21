/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _SDL_thread_h
#define _SDL_thread_h

/**
 *  \file SDL_thread.h
 *  
 *  Header for the SDL thread management routines.
 */

#include "SDL_stdinc.h"
#include "SDL_error.h"

/* Thread synchronization primitives */
#include "SDL_mutex.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/* The SDL thread structure, defined in SDL_thread.c */
struct SDL_Thread;
typedef struct SDL_Thread SDL_Thread;

/* The SDL thread ID */
typedef unsigned long SDL_threadID;

/* The SDL thread priority
 *
 * Note: On many systems you require special privileges to set high priority.
 */
typedef enum {
    SDL_THREAD_PRIORITY_LOW,
    SDL_THREAD_PRIORITY_NORMAL,
    SDL_THREAD_PRIORITY_HIGH
} SDL_ThreadPriority;

/* The function passed to SDL_CreateThread()
   It is passed a void* user context parameter and returns an int.
 */
typedef int (SDLCALL * SDL_ThreadFunction) (void *data);

#if defined(__WIN32__) && !defined(HAVE_LIBC)
/**
 *  \file SDL_thread.h
 *  
 *  We compile SDL into a DLL. This means, that it's the DLL which
 *  creates a new thread for the calling process with the SDL_CreateThread()
 *  API. There is a problem with this, that only the RTL of the SDL.DLL will
 *  be initialized for those threads, and not the RTL of the calling 
 *  application!
 *  
 *  To solve this, we make a little hack here.
 *  
 *  We'll always use the caller's _beginthread() and _endthread() APIs to
 *  start a new thread. This way, if it's the SDL.DLL which uses this API,
 *  then the RTL of SDL.DLL will be used to create the new thread, and if it's
 *  the application, then the RTL of the application will be used.
 *  
 *  So, in short:
 *  Always use the _beginthread() and _endthread() of the calling runtime 
 *  library!
 */
#define SDL_PASSED_BEGINTHREAD_ENDTHREAD
#include <process.h>            /* This has _beginthread() and _endthread() defined! */

typedef uintptr_t(__cdecl * pfnSDL_CurrentBeginThread) (void *, unsigned,
                                                        unsigned (__stdcall *
                                                                  func) (void
                                                                         *),
                                                        void *arg, unsigned,
                                                        unsigned *threadID);
typedef void (__cdecl * pfnSDL_CurrentEndThread) (unsigned code);

/**
 *  Create a thread.
 */
extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data,
                 pfnSDL_CurrentBeginThread pfnBeginThread,
                 pfnSDL_CurrentEndThread pfnEndThread);

/**
 *  Create a thread.
 */
#define SDL_CreateThread(fn, name, data) SDL_CreateThread(fn, name, data, _beginthreadex, _endthreadex)

#else

/**
 *  Create a thread.
 *
 *   Thread naming is a little complicated: Most systems have very small
 *    limits for the string length (BeOS has 32 bytes, Linux currently has 16,
 *    Visual C++ 6.0 has nine!), and possibly other arbitrary rules. You'll
 *    have to see what happens with your system's debugger. The name should be
 *    UTF-8 (but using the naming limits of C identifiers is a better bet).
 *   There are no requirements for thread naming conventions, so long as the
 *    string is null-terminated UTF-8, but these guidelines are helpful in
 *    choosing a name:
 *
 *    http://stackoverflow.com/questions/149932/naming-conventions-for-threads
 *
 *   If a system imposes requirements, SDL will try to munge the string for
 *    it (truncate, etc), but the original string contents will be available
 *    from SDL_GetThreadName().
 */
extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data);

#endif

/**
 * Get the thread name, as it was specified in SDL_CreateThread().
 *  This function returns a pointer to a UTF-8 string that names the
 *  specified thread, or NULL if it doesn't have a name. This is internal
 *  memory, not to be free()'d by the caller, and remains valid until the
 *  specified thread is cleaned up by SDL_WaitThread().
 */
extern DECLSPEC const char *SDLCALL SDL_GetThreadName(SDL_Thread *thread);

/**
 *  Get the thread identifier for the current thread.
 */
extern DECLSPEC SDL_threadID SDLCALL SDL_ThreadID(void);

/**
 *  Get the thread identifier for the specified thread.
 *  
 *  Equivalent to SDL_ThreadID() if the specified thread is NULL.
 */
extern DECLSPEC SDL_threadID SDLCALL SDL_GetThreadID(SDL_Thread * thread);

/**
 *  Set the priority for the current thread
 */
extern DECLSPEC int SDLCALL SDL_SetThreadPriority(SDL_ThreadPriority priority);

/**
 *  Wait for a thread to finish.
 *  
 *  The return code for the thread function is placed in the area
 *  pointed to by \c status, if \c status is not NULL.
 */
extern DECLSPEC void SDLCALL SDL_WaitThread(SDL_Thread * thread, int *status);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_thread_h */

/* vi: set ts=4 sw=4 expandtab: */
