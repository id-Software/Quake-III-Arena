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
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#ifndef __macosx_local_h
#define __macosx_local_h

#include "qcommon.h"

#ifdef __cplusplus
typedef void NSDictionary;
typedef void NSOpenGLContext;
typedef void NSWindow;

extern "C" {
#else
#import <Foundation/NSGeometry.h>
@class NSEvent, NSOpenGLContext, NSWindow;
#endif

#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/CGLTypes.h>

// In macosx_input.m
extern void Sys_InitInput(void);
extern void Sys_ShutdownInput(void);
extern void Sys_SetMouseInputRect(CGRect newRect);
extern CGDirectDisplayID Sys_DisplayToUse(void);

// In macosx_sys.m
extern void Sys_QueEvent(int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr);
extern void Sys_AnnoyingBanner();

// In macosx_glimp.m
extern qboolean Sys_IsHidden;
extern qboolean Sys_Hide();
extern qboolean Sys_Unhide();

typedef struct {
    CGDirectDisplayID     display;
    CGTableCount          tableSize;
    CGGammaValue	 *red;
    CGGammaValue	 *blue;
    CGGammaValue	 *green;
} glwgamma_t;

typedef struct
{
    CGDirectDisplayID     display;
    NSDictionary         *desktopMode;
    NSDictionary         *gameMode;

    CGDisplayCount        displayCount;
    glwgamma_t           *originalDisplayGammaTables;
    glwgamma_t            inGameTable;
    glwgamma_t            tempTable;
    
    NSOpenGLContext      *_ctx;
    CGLContextObj         _cgl_ctx;
    qboolean              _ctx_is_current;
    NSWindow             *window;
    
    FILE                 *log_fp;
    
    unsigned int          bufferSwapCount;
    unsigned int          glPauseCount;
} glwstate_t;

extern glwstate_t glw_state;

#define OSX_SetGLContext(context) \
do { \
    NSOpenGLContext *_context = (context); \
    glw_state._ctx = _context; \
    glw_state._cgl_ctx = [_context cglContext]; \
} while (0)

#define OSX_GetNSGLContext() glw_state._ctx
#define OSX_GetCGLContext() glw_state._cgl_ctx

#define OSX_GLContextIsCurrent() glw_state._ctx_is_current
#define OSX_GLContextSetCurrent() \
do { \
  [glw_state._ctx makeCurrentContext]; \
  glw_state._ctx_is_current = (glw_state._ctx != nil); \
} while (0)

#define OSX_GLContextClearCurrent() \
do { \
  [NSOpenGLContext clearCurrentContext]; \
  glw_state._ctx_is_current = NO; \
} while (0)


extern void Sys_PauseGL();
extern void Sys_ResumeGL();


#import "macosx_timers.h"

#ifdef OMNI_TIMER
extern OTStampList glThreadStampList;
#define GLSTAMP(name, data) OTStampListAddStamp(glThreadStampList, name, data)
#else
#define GLSTAMP(name, data)
#endif

#ifdef __cplusplus
}
#endif

#endif // __macosx_local_h
