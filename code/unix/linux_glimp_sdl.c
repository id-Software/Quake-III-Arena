
#if USE_SDL

/*
 * SDL implementation for Quake 3: Arena's GPL source release.
 *
 * I wrote such a beast originally for Loki's port of Heavy Metal: FAKK2,
 *  and then wrote it again for the Linux client of Medal of Honor: Allied
 *  Assault. Third times a charm, so I'm rewriting this once more for the
 *  GPL release of Quake 3.
 *
 * Written by Ryan C. Gordon (icculus@icculus.org). Please refer to
 *    http://icculus.org/quake3/ for the latest version of this code.
 *
 *  Patches and comments are welcome at the above address.
 *
 * I cut-and-pasted this from linux_glimp.c, and moved it to SDL line-by-line.
 *  There is probably some cruft that could be removed here.
 *
 * You should define USE_SDL=1 and then add this to the makefile.
 *  USE_SDL will disable the X11 target.
 *
 * !!! FIXME: linux_glimp_sdl.c isn't really "linux" specific.
 */

/*
Original copyright on Q3A sources:
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
/*
** GLW_IMP.C
**
** This file contains ALL Linux specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
** GLimp_SwitchFullscreen
** GLimp_SetGamma
**
*/

#include "SDL.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "linux_local.h" // bk001130

#include "unix_glw.h"

#define	WINDOW_CLASS_NAME	"Quake III: Arena"
#define	WINDOW_CLASS_NAME_BRIEF	"quake3"

typedef enum
{
  RSERR_OK,

  RSERR_INVALID_FULLSCREEN,
  RSERR_INVALID_MODE,

  RSERR_UNKNOWN
} rserr_t;

glwstate_t glw_state;

static SDL_Surface *screen = NULL;
static SDL_Joystick *stick = NULL;

static qboolean mouse_avail = qfalse;
static qboolean mouse_active = qfalse;
static qboolean sdlrepeatenabled = qfalse;

static cvar_t *in_mouse;
cvar_t *in_subframe;
cvar_t *in_nograb; // this is strictly for developers

// bk001130 - from cvs1.17 (mkv), but not static
cvar_t   *in_joystick      = NULL;
cvar_t   *in_joystickDebug = NULL;
cvar_t   *joy_threshold    = NULL;

cvar_t  *r_allowSoftwareGL;   // don't abort out if the pixelformat claims software
cvar_t  *r_previousglDriver;

static int win_x, win_y;

static int num_vidmodes;

static int mouse_accel_numerator;
static int mouse_accel_denominator;
static int mouse_threshold;    

/*
* Find the first occurrence of find in s.
*/
// bk001130 - from cvs1.17 (mkv), const
// bk001130 - made first argument const
static const char *Q_stristr( const char *s, const char *find)
{
  register char c, sc;
  register size_t len;

  if ((c = *find++) != 0)
  {
    if (c >= 'a' && c <= 'z')
    {
      c -= ('a' - 'A');
    }
    len = strlen(find);
    do
    {
      do
      {
        if ((sc = *s++) == 0)
          return NULL;
        if (sc >= 'a' && sc <= 'z')
        {
          sc -= ('a' - 'A');
        }
      } while (sc != c);
    } while (Q_stricmpn(s, find, len) != 0);
    s--;
  }
  return s;
}

//#define KBD_DBG

static const char *XLateKey(SDL_keysym *keysym, int *key)
{
  static char buf[2] = { '\0', '\0' };
  *key = 0;

  // these happen to match the ASCII chars.
  if ( ((keysym->sym >= SDLK_a) && (keysym->sym <= SDLK_z)) ||
       ((keysym->sym >= SDLK_0) && (keysym->sym <= SDLK_9)) )
  {
     *key = (int) keysym->sym;
     buf[0] = (char) *key;
     return buf;
  }

  buf[0] = '\0';

  switch (keysym->sym)
  {
  case SDLK_PAGEUP: *key = K_PGUP; break;
  case SDLK_KP9:  *key = K_KP_PGUP; break;
  case SDLK_PAGEDOWN: *key = K_PGDN; break;
  case SDLK_KP3: *key = K_KP_PGDN; break;
  case SDLK_KP7: *key = K_KP_HOME; break;
  case SDLK_HOME:  *key = K_HOME; break;
  case SDLK_KP1:   *key = K_KP_END; break;
  case SDLK_END:   *key = K_END; break;
  case SDLK_KP4: *key = K_KP_LEFTARROW; break;
  case SDLK_LEFT:  *key = K_LEFTARROW; break;
  case SDLK_KP6: *key = K_KP_RIGHTARROW; break;
  case SDLK_RIGHT:  *key = K_RIGHTARROW;    break;
  case SDLK_KP2:    *key = K_KP_DOWNARROW; break;
  case SDLK_DOWN:  *key = K_DOWNARROW; break;
  case SDLK_KP8:    *key = K_KP_UPARROW; break;
  case SDLK_UP:    *key = K_UPARROW;   break;
  case SDLK_ESCAPE: *key = K_ESCAPE;    break;
  case SDLK_KP_ENTER: *key = K_KP_ENTER;  break;
  case SDLK_RETURN: *key = K_ENTER;    break;
  case SDLK_TAB:    *key = K_TAB;      break;
  case SDLK_F1:    *key = K_F1;       break;
  case SDLK_F2:    *key = K_F2;       break;
  case SDLK_F3:    *key = K_F3;       break;
  case SDLK_F4:    *key = K_F4;       break;
  case SDLK_F5:    *key = K_F5;       break;
  case SDLK_F6:    *key = K_F6;       break;
  case SDLK_F7:    *key = K_F7;       break;
  case SDLK_F8:    *key = K_F8;       break;
  case SDLK_F9:    *key = K_F9;       break;
  case SDLK_F10:    *key = K_F10;      break;
  case SDLK_F11:    *key = K_F11;      break;
  case SDLK_F12:    *key = K_F12;      break;

    // bk001206 - from Ryan's Fakk2 
    //case SDLK_BackSpace: *key = 8; break; // ctrl-h
  case SDLK_BACKSPACE: *key = K_BACKSPACE; buf[0] = 8; break; // ctrl-h
  case SDLK_KP_PERIOD: *key = K_KP_DEL; break;
  case SDLK_DELETE: *key = K_DEL; break;
  case SDLK_PAUSE:  *key = K_PAUSE;    break;

  case SDLK_LSHIFT:
  case SDLK_RSHIFT:  *key = K_SHIFT;   break;

  case SDLK_LCTRL:
  case SDLK_RCTRL:  *key = K_CTRL;  break;

  case SDLK_RMETA:
  case SDLK_LMETA:
  case SDLK_RALT:
  case SDLK_LALT: *key = K_ALT;     break;

  case SDLK_KP5: *key = K_KP_5;  break;
  case SDLK_INSERT:   *key = K_INS; break;
  case SDLK_KP0: *key = K_KP_INS; break;
  case SDLK_KP_MULTIPLY: *key = '*'; break;
  case SDLK_KP_PLUS:  *key = K_KP_PLUS; break;
  case SDLK_KP_MINUS: *key = K_KP_MINUS; break;
  case SDLK_KP_DIVIDE: *key = K_KP_SLASH; break;
  case SDLK_SPACE: *key = K_SPACE; break;

  // !!! FIXME: Console key...may not be accurate on all keyboards!
  case SDLK_BACKQUOTE: *key = '~'; break;

  default:
    if (keysym->unicode <= 255)  // maps to ASCII?
    {
        char ch = (char) keysym->unicode;
        if (ch >= 'A' && ch <= 'Z')
            ch = ch - 'A' + 'a';
        // if ctrl is pressed, the keys are not between 'A' and 'Z', for instance ctrl-z == 26 ^Z ^C etc.
        // see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=19
        else if (ch >= 1 && ch <= 26)
            ch = ch + 'a' - 1;

        buf[0] = ch;
        return buf;
    }
    break;
  } 

  return buf;
}

static void install_grabs(void)
{
    SDL_WM_GrabInput(SDL_GRAB_ON);
}

static void uninstall_grabs(void)
{
    SDL_WM_GrabInput(SDL_GRAB_OFF);
}

static void HandleEvents(void)
{
  const int t = 0;  // always just use the current time.
  SDL_Event e;
  const char *p = NULL;
  int key = 0;
  int b = 0;

  if (screen == NULL)
    return;  // no SDL context.

  if (cls.keyCatchers == 0)
  {
    if (sdlrepeatenabled)
    {
        SDL_EnableKeyRepeat(0, 0);
        sdlrepeatenabled = qfalse;
    }
  }
  else
  {
    if (!sdlrepeatenabled)
    {
        SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
        sdlrepeatenabled = qtrue;
    }
  }

  while (SDL_PollEvent(&e))
  {
    switch (e.type)
    {
    case SDL_KEYDOWN:
      p = XLateKey(&e.key.keysym, &key);
      if (key)
      {
        Sys_QueEvent( t, SE_KEY, key, qtrue, 0, NULL );
      }
      if (p)
      {
        while (*p)
        {
          Sys_QueEvent( t, SE_CHAR, *p++, 0, 0, NULL );
        }
      }
      break;

    case SDL_KEYUP:
      XLateKey(&e.key.keysym, &key);
      Sys_QueEvent( t, SE_KEY, key, qfalse, 0, NULL );
      break;

    case SDL_MOUSEMOTION:
      if (mouse_active)
      {
        if (abs(e.motion.xrel) > 1)
          e.motion.xrel *= 2;
        if (abs(e.motion.yrel) > 1)
          e.motion.yrel *= 2;
        Sys_QueEvent( t, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL );
      }
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (e.button.button == 4)
      {
        Sys_QueEvent( t, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
      } else if (e.button.button == 5)
      {
        Sys_QueEvent( t, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
      } else
      {
        // NOTE TTimo there seems to be a weird mapping for K_MOUSE1 K_MOUSE2 K_MOUSE3 ..
        b=-1;
        if (e.button.button == 1)
        {
          b = 0; // K_MOUSE1
        } else if (e.button.button == 2)
        {
          b = 2; // K_MOUSE3
        } else if (e.button.button == 3)
        {
          b = 1; // K_MOUSE2
        } else if (e.button.button == 6)
        {
          b = 3; // K_MOUSE4
        } else if (e.button.button == 7)
        {
          b = 4; // K_MOUSE5
        };

        Sys_QueEvent( t, SE_KEY, K_MOUSE1 + b, qtrue, 0, NULL );
      }
      break;

    case SDL_MOUSEBUTTONUP:
      if (e.button.button == 4)
      {
        Sys_QueEvent( t, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
      } else if (e.button.button == 5)
      {
        Sys_QueEvent( t, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
      } else
      {
        b=-1;
        if (e.button.button == 1)
        {
          b = 0;
        } else if (e.button.button == 2)
        {
          b = 2;
        } else if (e.button.button == 3)
        {
          b = 1;
        } else if (e.button.button == 6)
        {
          b = 3; // K_MOUSE4
        } else if (e.button.button == 7)
        {
          b = 4; // K_MOUSE5
        };
        Sys_QueEvent( t, SE_KEY, K_MOUSE1 + b, qfalse, 0, NULL );
      }
      break;
    case SDL_QUIT:
      Sys_Quit();
      break;
    }
  }
}

// NOTE TTimo for the tty console input, we didn't rely on those .. 
//   it's not very surprising actually cause they are not used otherwise
void KBD_Init(void)
{
}

void KBD_Close(void)
{
}

void IN_ActivateMouse( void ) 
{
  if (!mouse_avail || !screen)
    return;

  if (!mouse_active)
  {
    if (!in_nograb->value)
      install_grabs();
    mouse_active = qtrue;
  }
}

void IN_DeactivateMouse( void ) 
{
  if (!mouse_avail || !screen)
    return;

  if (mouse_active)
  {
    if (!in_nograb->value)
      uninstall_grabs();
    mouse_active = qfalse;
  }
}
/*****************************************************************************/

/*
** GLimp_SetGamma
**
** This routine should only be called if glConfig.deviceSupportsGamma is TRUE
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
  // NOTE TTimo we get the gamma value from cvar, because we can't work with the s_gammatable
  //   the API wasn't changed to avoid breaking other OSes
  float g = Cvar_Get("r_gamma", "1.0", 0)->value;
  SDL_SetGamma(g, g, g);
}

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{
  IN_DeactivateMouse();

  if (stick)
  {
    SDL_JoystickClose(stick);
    stick = NULL;
  }

  SDL_Quit();
  screen = NULL;

  memset( &glConfig, 0, sizeof( glConfig ) );
  memset( &glState, 0, sizeof( glState ) );

  QGL_Shutdown();
}

/*
** GLimp_LogComment
*/
void GLimp_LogComment( char *comment ) 
{
  if ( glw_state.log_fp )
  {
    fprintf( glw_state.log_fp, "%s", comment );
  }
}

/*
** GLW_StartDriverAndSetMode
*/
// bk001204 - prototype needed
static int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen );
static qboolean GLW_StartDriverAndSetMode( const char *drivername, 
                                           int mode, 
                                           qboolean fullscreen )
{
  rserr_t err;

  if (!SDL_WasInit(SDL_INIT_VIDEO))
  {
    ri.Printf( PRINT_ALL, "Calling SDL_Init(SDL_INIT_VIDEO)...\n");
    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
		ri.Printf( PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO) failed: %s\n", SDL_GetError());
        return qfalse;
    }
    ri.Printf( PRINT_ALL, "SDL_Init(SDL_INIT_VIDEO) passed.\n");
  }

  // don't ever bother going into fullscreen with a voodoo card
#if 1	// JDC: I reenabled this
  if ( Q_stristr( drivername, "Voodoo" ) )
  {
    ri.Cvar_Set( "r_fullscreen", "0" );
    r_fullscreen->modified = qfalse;
    fullscreen = qfalse;
  }
#endif
	
	if (fullscreen && in_nograb->value)
	{
		ri.Printf( PRINT_ALL, "Fullscreen not allowed with in_nograb 1\n");
    ri.Cvar_Set( "r_fullscreen", "0" );
    r_fullscreen->modified = qfalse;
    fullscreen = qfalse;		
	}

  err = GLW_SetMode( drivername, mode, fullscreen );

  switch ( err )
  {
  case RSERR_INVALID_FULLSCREEN:
    ri.Printf( PRINT_ALL, "...WARNING: fullscreen unavailable in this mode\n" );
    return qfalse;
  case RSERR_INVALID_MODE:
    ri.Printf( PRINT_ALL, "...WARNING: could not set the given mode (%d)\n", mode );
    return qfalse;
  default:
    break;
  }
  return qtrue;
}

/*
** GLW_SetMode
*/
static int GLW_SetMode( const char *drivername, int mode, qboolean fullscreen )
{
  const char*   glstring; // bk001130 - from cvs1.17 (mkv)
  int sdlcolorbits = 4;
  int colorbits, depthbits, stencilbits;
  int tcolorbits, tdepthbits, tstencilbits;
  int i = 0;
  SDL_Surface *vidscreen = NULL;

  ri.Printf( PRINT_ALL, "Initializing OpenGL display\n");

  ri.Printf (PRINT_ALL, "...setting mode %d:", mode );

  if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) )
  {
    ri.Printf( PRINT_ALL, " invalid mode\n" );
    return RSERR_INVALID_MODE;
  }
  ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight);

  Uint32 flags = SDL_OPENGL;
  if (fullscreen)
    flags |= SDL_FULLSCREEN;

  if (!r_colorbits->value)
    colorbits = 24;
  else
    colorbits = r_colorbits->value;

  if ( !Q_stricmp( r_glDriver->string, _3DFX_DRIVER_NAME ) )
    colorbits = 16;

  if (!r_depthbits->value)
    depthbits = 24;
  else
    depthbits = r_depthbits->value;
  stencilbits = r_stencilbits->value;

  for (i = 0; i < 16; i++)
  {
    // 0 - default
    // 1 - minus colorbits
    // 2 - minus depthbits
    // 3 - minus stencil
    if ((i % 4) == 0 && i)
    {
      // one pass, reduce
      switch (i / 4)
      {
      case 2 :
        if (colorbits == 24)
          colorbits = 16;
        break;
      case 1 :
        if (depthbits == 24)
          depthbits = 16;
        else if (depthbits == 16)
          depthbits = 8;
      case 3 :
        if (stencilbits == 24)
          stencilbits = 16;
        else if (stencilbits == 16)
          stencilbits = 8;
      }
    }

    tcolorbits = colorbits;
    tdepthbits = depthbits;
    tstencilbits = stencilbits;

    if ((i % 4) == 3)
    { // reduce colorbits
      if (tcolorbits == 24)
        tcolorbits = 16;
    }

    if ((i % 4) == 2)
    { // reduce depthbits
      if (tdepthbits == 24)
        tdepthbits = 16;
      else if (tdepthbits == 16)
        tdepthbits = 8;
    }

    if ((i % 4) == 1)
    { // reduce stencilbits
      if (tstencilbits == 24)
        tstencilbits = 16;
      else if (tstencilbits == 16)
        tstencilbits = 8;
      else
        tstencilbits = 0;
    }

    if (tcolorbits == 24)
        sdlcolorbits = 8;

    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, sdlcolorbits );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, sdlcolorbits );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, sdlcolorbits );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
    SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    SDL_WM_SetCaption(WINDOW_CLASS_NAME, WINDOW_CLASS_NAME_BRIEF);
    SDL_ShowCursor(0);
    SDL_EnableUNICODE(1);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    sdlrepeatenabled = qtrue;

    if (!(vidscreen = SDL_SetVideoMode(glConfig.vidWidth, glConfig.vidHeight, colorbits, flags)))
    {
        fprintf(stderr, "SDL_SetVideoMode failed: %s\n", SDL_GetError());
        continue;
    }

    ri.Printf( PRINT_ALL, "Using %d/%d/%d Color bits, %d depth, %d stencil display.\n",
               sdlcolorbits, sdlcolorbits, sdlcolorbits,
               tdepthbits, tstencilbits);

    glConfig.colorBits = tcolorbits;
    glConfig.depthBits = tdepthbits;
    glConfig.stencilBits = tstencilbits;
    break;
  }

  if (!vidscreen)
  {
    ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
    return RSERR_INVALID_MODE;
  }

  screen = vidscreen;

  // bk001130 - from cvs1.17 (mkv)
  glstring = qglGetString (GL_RENDERER);
  ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring );

  // bk010122 - new software token (Indirect)
  if ( !Q_stricmp( glstring, "Mesa X11")
       || !Q_stricmp( glstring, "Mesa GLX Indirect") )
  {
    if ( !r_allowSoftwareGL->integer )
    {
      ri.Printf( PRINT_ALL, "\n\n***********************************************************\n" );
      ri.Printf( PRINT_ALL, " You are using software Mesa (no hardware acceleration)!   \n" );
      ri.Printf( PRINT_ALL, " Driver DLL used: %s\n", drivername ); 
      ri.Printf( PRINT_ALL, " If this is intentional, add\n" );
      ri.Printf( PRINT_ALL, "       \"+set r_allowSoftwareGL 1\"\n" );
      ri.Printf( PRINT_ALL, " to the command line when starting the game.\n" );
      ri.Printf( PRINT_ALL, "***********************************************************\n");
      GLimp_Shutdown( );
      return RSERR_INVALID_MODE;
    } else
    {
      ri.Printf( PRINT_ALL, "...using software Mesa (r_allowSoftwareGL==1).\n" );
    }
  }

  return RSERR_OK;
}

/*
** GLW_InitExtensions
*/
static void GLW_InitExtensions( void )
{
  if ( !r_allowExtensions->integer )
  {
    ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
    return;
  }

  ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

  // GL_S3_s3tc
  if ( Q_stristr( glConfig.extensions_string, "GL_S3_s3tc" ) )
  {
    if ( r_ext_compressed_textures->value )
    {
      glConfig.textureCompression = TC_S3TC;
      ri.Printf( PRINT_ALL, "...using GL_S3_s3tc\n" );
    } else
    {
      glConfig.textureCompression = TC_NONE;
      ri.Printf( PRINT_ALL, "...ignoring GL_S3_s3tc\n" );
    }
  } else
  {
    glConfig.textureCompression = TC_NONE;
    ri.Printf( PRINT_ALL, "...GL_S3_s3tc not found\n" );
  }

  // GL_EXT_texture_env_add
  glConfig.textureEnvAddAvailable = qfalse;
  if ( Q_stristr( glConfig.extensions_string, "EXT_texture_env_add" ) )
  {
    if ( r_ext_texture_env_add->integer )
    {
      glConfig.textureEnvAddAvailable = qtrue;
      ri.Printf( PRINT_ALL, "...using GL_EXT_texture_env_add\n" );
    } else
    {
      glConfig.textureEnvAddAvailable = qfalse;
      ri.Printf( PRINT_ALL, "...ignoring GL_EXT_texture_env_add\n" );
    }
  } else
  {
    ri.Printf( PRINT_ALL, "...GL_EXT_texture_env_add not found\n" );
  }

  // GL_ARB_multitexture
  qglMultiTexCoord2fARB = NULL;
  qglActiveTextureARB = NULL;
  qglClientActiveTextureARB = NULL;
  if ( Q_stristr( glConfig.extensions_string, "GL_ARB_multitexture" ) )
  {
    // !!! FIXME: Use SDL_GL_GetProcAddress instead?
    if ( r_ext_multitexture->value )
    {
      qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) dlsym( glw_state.OpenGLLib, "glMultiTexCoord2fARB" );
      qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) dlsym( glw_state.OpenGLLib, "glActiveTextureARB" );
      qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) dlsym( glw_state.OpenGLLib, "glClientActiveTextureARB" );

      if ( qglActiveTextureARB )
      {
        qglGetIntegerv( GL_MAX_ACTIVE_TEXTURES_ARB, &glConfig.maxActiveTextures );

        if ( glConfig.maxActiveTextures > 1 )
        {
          ri.Printf( PRINT_ALL, "...using GL_ARB_multitexture\n" );
        } else
        {
          qglMultiTexCoord2fARB = NULL;
          qglActiveTextureARB = NULL;
          qglClientActiveTextureARB = NULL;
          ri.Printf( PRINT_ALL, "...not using GL_ARB_multitexture, < 2 texture units\n" );
        }
      }
    } else
    {
      ri.Printf( PRINT_ALL, "...ignoring GL_ARB_multitexture\n" );
    }
  } else
  {
    ri.Printf( PRINT_ALL, "...GL_ARB_multitexture not found\n" );
  }

  // GL_EXT_compiled_vertex_array
  if ( Q_stristr( glConfig.extensions_string, "GL_EXT_compiled_vertex_array" ) )
  {
    if ( r_ext_compiled_vertex_array->value )
    {
      ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
      qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) dlsym( glw_state.OpenGLLib, "glLockArraysEXT" );
      qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) dlsym( glw_state.OpenGLLib, "glUnlockArraysEXT" );
      if (!qglLockArraysEXT || !qglUnlockArraysEXT)
      {
        ri.Error (ERR_FATAL, "bad getprocaddress");
      }
    } else
    {
      ri.Printf( PRINT_ALL, "...ignoring GL_EXT_compiled_vertex_array\n" );
    }
  } else
  {
    ri.Printf( PRINT_ALL, "...GL_EXT_compiled_vertex_array not found\n" );
  }

}

static void GLW_InitGamma()
{
    glConfig.deviceSupportsGamma = qtrue;
}

/*
** GLW_LoadOpenGL
**
** GLimp_win.c internal function that that attempts to load and use 
** a specific OpenGL DLL.
*/
static qboolean GLW_LoadOpenGL( const char *name )
{
  qboolean fullscreen;

  ri.Printf( PRINT_ALL, "...loading %s: ", name );

  // disable the 3Dfx splash screen and set gamma
  // we do this all the time, but it shouldn't hurt anything
  // on non-3Dfx stuff
  putenv("FX_GLIDE_NO_SPLASH=0");

  // Mesa VooDoo hacks
  putenv("MESA_GLX_FX=fullscreen\n");

  // load the QGL layer
  if ( QGL_Init( name ) )
  {
    fullscreen = r_fullscreen->integer;

    // create the window and set up the context
    if ( !GLW_StartDriverAndSetMode( name, r_mode->integer, fullscreen ) )
    {
      if (r_mode->integer != 3)
      {
        if ( !GLW_StartDriverAndSetMode( name, 3, fullscreen ) )
        {
          goto fail;
        }
      } else
        goto fail;
    }

    return qtrue;
  } else
  {
    ri.Printf( PRINT_ALL, "failed\n" );
  }
  fail:

  QGL_Shutdown();

  return qfalse;
}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  
*/
void GLimp_Init( void )
{
  qboolean attemptedlibGL = qfalse;
  qboolean attempted3Dfx = qfalse;
  qboolean success = qfalse;
  char  buf[1024];
  cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );

  r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );

  r_previousglDriver = ri.Cvar_Get( "r_previousglDriver", "", CVAR_ROM );

  InitSig();

  // Hack here so that if the UI 
  if ( *r_previousglDriver->string )
  {
    // The UI changed it on us, hack it back
    // This means the renderer can't be changed on the fly
    ri.Cvar_Set( "r_glDriver", r_previousglDriver->string );
  }

  //
  // load and initialize the specific OpenGL driver
  //
  if ( !GLW_LoadOpenGL( r_glDriver->string ) )
  {
    if ( !Q_stricmp( r_glDriver->string, OPENGL_DRIVER_NAME ) )
    {
      attemptedlibGL = qtrue;
    } else if ( !Q_stricmp( r_glDriver->string, _3DFX_DRIVER_NAME ) )
    {
      attempted3Dfx = qtrue;
    }

    #if 0
    // TTimo
    // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=455
    // old legacy load code, was confusing people who had a bad OpenGL setup
    if ( !attempted3Dfx && !success )
    {
      attempted3Dfx = qtrue;
      if ( GLW_LoadOpenGL( _3DFX_DRIVER_NAME ) )
      {
        ri.Cvar_Set( "r_glDriver", _3DFX_DRIVER_NAME );
        r_glDriver->modified = qfalse;
        success = qtrue;
      }
    }
    #endif

    // try ICD before trying 3Dfx standalone driver
    if ( !attemptedlibGL && !success )
    {
      attemptedlibGL = qtrue;
      if ( GLW_LoadOpenGL( OPENGL_DRIVER_NAME ) )
      {
        ri.Cvar_Set( "r_glDriver", OPENGL_DRIVER_NAME );
        r_glDriver->modified = qfalse;
        success = qtrue;
      }
    }

    if (!success)
      ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );

  }

  // Save it in case the UI stomps it
  ri.Cvar_Set( "r_previousglDriver", r_glDriver->string );

  // This values force the UI to disable driver selection
  glConfig.driverType = GLDRV_ICD;
  glConfig.hardwareType = GLHW_GENERIC;

  // get our config strings
  Q_strncpyz( glConfig.vendor_string, qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
  Q_strncpyz( glConfig.renderer_string, qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
  if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
    glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
  Q_strncpyz( glConfig.version_string, qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
  Q_strncpyz( glConfig.extensions_string, qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );

  //
  // chipset specific configuration
  //
  strcpy( buf, glConfig.renderer_string );
  strlwr( buf );

  //
  // NOTE: if changing cvars, do it within this block.  This allows them
  // to be overridden when testing driver fixes, etc. but only sets
  // them to their default state when the hardware is first installed/run.
  //
  if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) )
  {
    glConfig.hardwareType = GLHW_GENERIC;

    ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );

    // VOODOO GRAPHICS w/ 2MB
    if ( Q_stristr( buf, "voodoo graphics/1 tmu/2 mb" ) )
    {
      ri.Cvar_Set( "r_picmip", "2" );
      ri.Cvar_Get( "r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
    } else
    {
      ri.Cvar_Set( "r_picmip", "1" );

      if ( Q_stristr( buf, "rage 128" ) || Q_stristr( buf, "rage128" ) )
      {
        ri.Cvar_Set( "r_finish", "0" );
      }
      // Savage3D and Savage4 should always have trilinear enabled
      else if ( Q_stristr( buf, "savage3d" ) || Q_stristr( buf, "s3 savage4" ) )
      {
        ri.Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
      }
    }
  }

  //
  // this is where hardware specific workarounds that should be
  // detected/initialized every startup should go.
  //
  if ( Q_stristr( buf, "banshee" ) || Q_stristr( buf, "Voodoo_Graphics" ) )
  {
    glConfig.hardwareType = GLHW_3DFX_2D3D;
  } else if ( Q_stristr( buf, "rage pro" ) || Q_stristr( buf, "RagePro" ) )
  {
    glConfig.hardwareType = GLHW_RAGEPRO;
  } else if ( Q_stristr( buf, "permedia2" ) )
  {
    glConfig.hardwareType = GLHW_PERMEDIA2;
  } else if ( Q_stristr( buf, "riva 128" ) )
  {
    glConfig.hardwareType = GLHW_RIVA128;
  } else if ( Q_stristr( buf, "riva tnt " ) )
  {
  }

  ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

  // initialize extensions
  GLW_InitExtensions();
  GLW_InitGamma();

  InitSig(); // not clear why this is at begin & end of function

  return;
}


/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
  // don't flip if drawing to front buffer
  if ( stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 )
  {
    SDL_GL_SwapBuffers();
  }

  // check logging
  QGL_EnableLogging( (qboolean)r_logFile->integer ); // bk001205 - was ->value
}

// Figure out something to do with this...
#if 0 //def SMP
/*
===========================================================

SMP acceleration

===========================================================
*/

static pthread_mutex_t	smpMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t		renderCommandsEvent = PTHREAD_COND_INITIALIZER;
static pthread_cond_t		renderCompletedEvent = PTHREAD_COND_INITIALIZER;

static void (*glimpRenderThread)( void );

static void *GLimp_RenderThreadWrapper( void *arg )
{
	Com_Printf( "Render thread starting\n" );

  glimpRenderThread();

	qglXMakeCurrent( dpy, None, NULL );

	Com_Printf( "Render thread terminating\n" );

	return arg;
}

qboolean GLimp_SpawnRenderThread( void (*function)( void ) )
{
	pthread_t renderThread;
	int ret;

	pthread_mutex_init( &smpMutex, NULL );

	pthread_cond_init( &renderCommandsEvent, NULL );
	pthread_cond_init( &renderCompletedEvent, NULL );

  glimpRenderThread = function;

	ret = pthread_create( &renderThread,
						  NULL,			// attributes
						  GLimp_RenderThreadWrapper,
						  NULL );		// argument
	if ( ret ) {
		ri.Printf( PRINT_ALL, "pthread_create returned %d: %s", ret, strerror( ret ) );
    return qfalse;
	} else {
		ret = pthread_detach( renderThread );
		if ( ret ) {
			ri.Printf( PRINT_ALL, "pthread_detach returned %d: %s", ret, strerror( ret ) );
		}
  }

  return qtrue;
}

static volatile void    *smpData = NULL;
static volatile qboolean smpDataReady;

void *GLimp_RendererSleep( void )
{
	void  *data;

	qglXMakeCurrent( dpy, None, NULL );

	pthread_mutex_lock( &smpMutex );
	{
		smpData = NULL;
		smpDataReady = qfalse;

		// after this, the front end can exit GLimp_FrontEndSleep
		pthread_cond_signal( &renderCompletedEvent );

		while ( !smpDataReady ) {
			pthread_cond_wait( &renderCommandsEvent, &smpMutex );
		}

		data = (void *)smpData;
	}
	pthread_mutex_unlock( &smpMutex );

	qglXMakeCurrent( dpy, win, ctx );

  return data;
}

void GLimp_FrontEndSleep( void )
{
	pthread_mutex_lock( &smpMutex );
	{
		while ( smpData ) {
			pthread_cond_wait( &renderCompletedEvent, &smpMutex );
		}
	}
	pthread_mutex_unlock( &smpMutex );

	qglXMakeCurrent( dpy, win, ctx );
}

void GLimp_WakeRenderer( void *data )
{
	qglXMakeCurrent( dpy, None, NULL );

	pthread_mutex_lock( &smpMutex );
	{
		assert( smpData == NULL );
		smpData = data;
		smpDataReady = qtrue;

		// after this, the renderer can continue through GLimp_RendererSleep
		pthread_cond_signal( &renderCommandsEvent );
	}
	pthread_mutex_unlock( &smpMutex );
}

#else

void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n");
  return qfalse;
}
void *GLimp_RendererSleep( void ) {
  return NULL;
}
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}

#endif

/*****************************************************************************/
/* MOUSE                                                                     */
/*****************************************************************************/

void IN_Init(void) {
	Com_Printf ("\n------- Input Initialization -------\n");
  // mouse variables
  in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	
	// turn on-off sub-frame timing of X events
	in_subframe = Cvar_Get ("in_subframe", "1", CVAR_ARCHIVE);
	
	// developer feature, allows to break without loosing mouse pointer
	in_nograb = Cvar_Get ("in_nograb", "0", 0);

  // bk001130 - from cvs.17 (mkv), joystick variables
  in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH);
  // bk001130 - changed this to match win32
  in_joystickDebug = Cvar_Get ("in_debugjoystick", "0", CVAR_TEMP);
  joy_threshold = Cvar_Get ("joy_threshold", "0.15", CVAR_ARCHIVE); // FIXME: in_joythreshold

  if (in_mouse->value)
    mouse_avail = qtrue;
  else
    mouse_avail = qfalse;

  IN_StartupJoystick( ); // bk001130 - from cvs1.17 (mkv)
	Com_Printf ("------------------------------------\n");
}

void IN_Shutdown(void)
{
  mouse_avail = qfalse;
}

void IN_Frame (void) {

  // bk001130 - from cvs 1.17 (mkv)
  IN_JoyMove(); // FIXME: disable if on desktop?

  if ( cls.keyCatchers & KEYCATCH_CONSOLE )
  {
    // temporarily deactivate if not in the game and
    // running on the desktop
    // voodoo always counts as full screen
    if (Cvar_VariableValue ("r_fullscreen") == 0
        && strcmp( Cvar_VariableString("r_glDriver"), _3DFX_DRIVER_NAME ) )
    {
      IN_DeactivateMouse ();
      return;
    }
  }

  IN_ActivateMouse();
}

void IN_Activate(void)
{
}

// bk001130 - cvs1.17 joystick code (mkv) was here, no linux_joystick.c

void Sys_SendKeyEvents (void) {
  // XEvent event; // bk001204 - unused

  if (!screen)
    return;
  HandleEvents();
}


// (moved this back in here from linux_joystick.c, so it's all in one place...
//   --ryan.

/* We translate axes movement into keypresses. */
static int joy_keys[16] = {
     K_LEFTARROW, K_RIGHTARROW,
     K_UPARROW, K_DOWNARROW,
     K_JOY16, K_JOY17,
     K_JOY18, K_JOY19,
     K_JOY20, K_JOY21,
     K_JOY22, K_JOY23,

     K_JOY24, K_JOY25,
     K_JOY26, K_JOY27
};


// bk001130 - from linux_glimp.c
extern cvar_t *  in_joystick;
extern cvar_t *  in_joystickDebug;
extern cvar_t *  joy_threshold;


/**********************************************/
/* Joystick routines.                         */
/**********************************************/
// bk001130 - from cvs1.17 (mkv), removed from linux_glimp.c
void IN_StartupJoystick( void )
{
  int i = 0;
  int total = 0;

  if (stick != NULL)
    SDL_JoystickClose(stick);

  stick = NULL;

  if( !in_joystick->integer ) {
    Com_Printf( "Joystick is not active.\n" );
    return;
  }

  if (!SDL_WasInit(SDL_INIT_JOYSTICK))
  {
      Com_Printf( PRINT_ALL, "Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
      if (SDL_Init(SDL_INIT_JOYSTICK) == -1)
      {
          Com_Printf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
          return;
      }
      Com_Printf( PRINT_ALL, "SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
  }

  total = SDL_NumJoysticks();
  Com_Printf("I see %d possible joysticks\n", total);
  for (i = 0; i < total; i++)
    Com_Printf("[%d] %s\n", i, SDL_JoystickName(i));

  // !!! FIXME: someone should add a way to select a specific stick.
  for( i = 0; i < total; i++ ) {
    stick = SDL_JoystickOpen(i);
    if (stick == NULL)
        continue;

    Com_Printf( "Joystick %d opened\n", i );
    Com_Printf( "Name:    %s\n", SDL_JoystickName(i) );
    Com_Printf( "Axes:    %d\n", SDL_JoystickNumAxes(stick) );
    Com_Printf( "Buttons: %d\n", SDL_JoystickNumButtons(stick) );
    Com_Printf( "Balls: %d\n", SDL_JoystickNumBalls(stick) );

    SDL_JoystickEventState(SDL_QUERY);

    /* Our work here is done. */
    return;
  }

  /* No soup for you. */
  if( stick == NULL ) {
    Com_Printf( "No joystick opened.\n" );
    return;
  }
}

void IN_JoyMove( void )
{
  if (!stick)
    return;

  SDL_JoystickUpdate();

#if 0   // !!! FIXME: convert this from the Linux codebase.
  /* Store instantaneous joystick state. Hack to get around
   * event model used in Linux joystick driver.
	 */
  static int axes_state[16];
  /* Old bits for Quake-style input compares. */
  static unsigned int old_axes = 0;
  /* Our current goodies. */
  unsigned int axes = 0;
  int i = 0;

  if( joy_fd == -1 ) {
    return;
  }

  /* Empty the queue, dispatching button presses immediately
	 * and updating the instantaneous state for the axes.
	 */
  do {
    int n = -1;
    struct js_event event;

    n = read( joy_fd, &event, sizeof( event ) );

    if( n == -1 ) {
      /* No error, we're non-blocking. */
      break;
    }

    if( event.type & JS_EVENT_BUTTON ) {
      Sys_QueEvent( 0, SE_KEY, K_JOY1 + event.number, event.value, 0, NULL );
    } else if( event.type & JS_EVENT_AXIS ) {

      if( event.number >= 16 ) {
	continue;
      }

      axes_state[event.number] = event.value;
    } else {
      Com_Printf( "Unknown joystick event type\n" );
    }

  } while( 1 );


  /* Translate our instantaneous state to bits. */
  for( i = 0; i < 16; i++ ) {
    float f = ( (float) axes_state[i] ) / 32767.0f;

    if( f < -joy_threshold->value ) {
      axes |= ( 1 << ( i * 2 ) );
    } else if( f > joy_threshold->value ) {
      axes |= ( 1 << ( ( i * 2 ) + 1 ) );
    }

  }

  /* Time to update axes state based on old vs. new. */
  for( i = 0; i < 16; i++ ) {

    if( ( axes & ( 1 << i ) ) && !( old_axes & ( 1 << i ) ) ) {
      Sys_QueEvent( 0, SE_KEY, joy_keys[i], qtrue, 0, NULL );
    }

    if( !( axes & ( 1 << i ) ) && ( old_axes & ( 1 << i ) ) ) {
      Sys_QueEvent( 0, SE_KEY, joy_keys[i], qfalse, 0, NULL );
    }
  }

  /* Save for future generations. */
  old_axes = axes;
#endif
}

#endif  // USE_SDL

// end of linux_glimp_sdl.c ...

