/*
===========================================================================
SDL2 platform layer for 64-bit builds: window/GL context, input, gamma.
Replaces linux_glimp.c (Xlib/GLX/XFree86-DGA/VidMode), which cannot be used
on modern systems. Written for this source tree; GLW_InitExtensions is carried
over from linux_glimp.c.
===========================================================================
*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <SDL2/SDL.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "linux_local.h"
#include "unix_glw.h"

glwstate_t glw_state;

static SDL_Window *sdlWindow = NULL;
static SDL_GLContext sdlContext = NULL;

static qboolean mouse_avail = qfalse;
static qboolean mouse_active = qfalse;
static int mx = 0, my = 0;

cvar_t *in_mouse;
cvar_t *in_subframe;     // unused here, kept for unix_shared.c
cvar_t *in_nograb;
cvar_t *in_joystick = NULL;
cvar_t *in_joystickDebug = NULL;
cvar_t *joy_threshold = NULL;
cvar_t *r_allowSoftwareGL;
cvar_t *r_previousglDriver;
qboolean vidmode_ext = qfalse;

// full extension string (glConfig.extensions_string is only 1024 chars)
static char s_extensions[65536];

static void *SDLGetProc( const char *name )
{
	void *p = SDL_GL_GetProcAddress( name );
	if ( !p ) p = dlsym( RTLD_DEFAULT, name );
	return p;
}

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

static void GLW_InitExtensions( void )
{
  if ( !r_allowExtensions->integer )
  {
    ri.Printf( PRINT_ALL, "*** IGNORING OPENGL EXTENSIONS ***\n" );
    return;
  }

  ri.Printf( PRINT_ALL, "Initializing OpenGL extensions\n" );

  // GL_S3_s3tc
  if ( Q_stristr( s_extensions, "GL_S3_s3tc" ) )
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
  if ( Q_stristr( s_extensions, "EXT_texture_env_add" ) )
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
  if ( Q_stristr( s_extensions, "GL_ARB_multitexture" ) )
  {
    if ( r_ext_multitexture->value )
    {
      qglMultiTexCoord2fARB = ( PFNGLMULTITEXCOORD2FARBPROC ) SDLGetProc( "glMultiTexCoord2fARB" );
      qglActiveTextureARB = ( PFNGLACTIVETEXTUREARBPROC ) SDLGetProc( "glActiveTextureARB" );
      qglClientActiveTextureARB = ( PFNGLCLIENTACTIVETEXTUREARBPROC ) SDLGetProc( "glClientActiveTextureARB" );

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
  if ( Q_stristr( s_extensions, "GL_EXT_compiled_vertex_array" ) )
  {
    if ( r_ext_compiled_vertex_array->value )
    {
      ri.Printf( PRINT_ALL, "...using GL_EXT_compiled_vertex_array\n" );
      qglLockArraysEXT = ( void ( APIENTRY * )( int, int ) ) SDLGetProc( "glLockArraysEXT" );
      qglUnlockArraysEXT = ( void ( APIENTRY * )( void ) ) SDLGetProc( "glUnlockArraysEXT" );
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


typedef enum { RSERR_OK, RSERR_INVALID_MODE } rserr_t;

/*
** key translation
*/
static int SDLKeyToQ3( SDL_Keysym *ks )
{
	switch ( ks->sym ) {
	case SDLK_PAGEUP:     return K_PGUP;
	case SDLK_PAGEDOWN:   return K_PGDN;
	case SDLK_HOME:       return K_HOME;
	case SDLK_END:        return K_END;
	case SDLK_LEFT:       return K_LEFTARROW;
	case SDLK_RIGHT:      return K_RIGHTARROW;
	case SDLK_DOWN:       return K_DOWNARROW;
	case SDLK_UP:         return K_UPARROW;
	case SDLK_ESCAPE:     return K_ESCAPE;
	case SDLK_RETURN:     return K_ENTER;
	case SDLK_TAB:        return K_TAB;
	case SDLK_F1:  return K_F1;   case SDLK_F2:  return K_F2;
	case SDLK_F3:  return K_F3;   case SDLK_F4:  return K_F4;
	case SDLK_F5:  return K_F5;   case SDLK_F6:  return K_F6;
	case SDLK_F7:  return K_F7;   case SDLK_F8:  return K_F8;
	case SDLK_F9:  return K_F9;   case SDLK_F10: return K_F10;
	case SDLK_F11: return K_F11;  case SDLK_F12: return K_F12;
	case SDLK_BACKSPACE:  return K_BACKSPACE;
	case SDLK_DELETE:     return K_DEL;
	case SDLK_PAUSE:      return K_PAUSE;
	case SDLK_LSHIFT: case SDLK_RSHIFT: return K_SHIFT;
	case SDLK_LCTRL:  case SDLK_RCTRL:  return K_CTRL;
	case SDLK_LALT:   case SDLK_RALT:
	case SDLK_LGUI:   case SDLK_RGUI:   return K_ALT;
	case SDLK_CAPSLOCK:   return K_CAPSLOCK;
	case SDLK_INSERT:     return K_INS;
	case SDLK_KP_0:       return K_KP_INS;
	case SDLK_KP_1:       return K_KP_END;
	case SDLK_KP_2:       return K_KP_DOWNARROW;
	case SDLK_KP_3:       return K_KP_PGDN;
	case SDLK_KP_4:       return K_KP_LEFTARROW;
	case SDLK_KP_5:       return K_KP_5;
	case SDLK_KP_6:       return K_KP_RIGHTARROW;
	case SDLK_KP_7:       return K_KP_HOME;
	case SDLK_KP_8:       return K_KP_UPARROW;
	case SDLK_KP_9:       return K_KP_PGUP;
	case SDLK_KP_PERIOD:  return K_KP_DEL;
	case SDLK_KP_ENTER:   return K_KP_ENTER;
	case SDLK_KP_DIVIDE:  return K_KP_SLASH;
	case SDLK_KP_MINUS:   return K_KP_MINUS;
	case SDLK_KP_PLUS:    return K_KP_PLUS;
	case SDLK_KP_MULTIPLY:return K_KP_STAR;
	case SDLK_NUMLOCKCLEAR: return K_KP_NUMLOCK;
	default:
		// printable ASCII keycodes are the lower-case character, like Q3 expects
		if ( ks->sym >= 32 && ks->sym < 127 ) {
			return (int)ks->sym;
		}
		return 0;
	}
}

static void SDL_ActivateMouse( qboolean on )
{
	if ( !sdlWindow ) return;
	if ( on == mouse_active ) return;
	if ( on && !in_nograb->value ) {
		SDL_SetRelativeMouseMode( SDL_TRUE );
		SDL_ShowCursor( SDL_DISABLE );
	} else {
		SDL_SetRelativeMouseMode( SDL_FALSE );
		SDL_ShowCursor( SDL_ENABLE );
	}
	mouse_active = on;
	mx = my = 0;
}

void IN_ActivateMouse( void )   { if ( mouse_avail ) SDL_ActivateMouse( qtrue ); }
void IN_DeactivateMouse( void ) { SDL_ActivateMouse( qfalse ); }

static void HandleEvents( void )
{
	SDL_Event e;
	int t = Sys_Milliseconds();
	int key, b;
	qboolean gotMotion = qfalse;

	while ( SDL_PollEvent( &e ) ) {
		switch ( e.type ) {
		case SDL_KEYDOWN:
			if ( e.key.repeat ) {
				// Q3 generates its own repeats from SE_KEY only once; but keep
				// repeating for backspace/arrows in text fields via SE_CHAR below
			} else {
				key = SDLKeyToQ3( &e.key.keysym );
				if ( key ) Sys_QueEvent( t, SE_KEY, key, qtrue, 0, NULL );
			}
			// control characters the text input event does not report
			if ( e.key.keysym.sym == SDLK_BACKSPACE ) {
				Sys_QueEvent( t, SE_CHAR, 8, 0, 0, NULL );
			} else if ( e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER ) {
				Sys_QueEvent( t, SE_CHAR, 13, 0, 0, NULL );
			}
			break;
		case SDL_KEYUP:
			key = SDLKeyToQ3( &e.key.keysym );
			if ( key ) Sys_QueEvent( t, SE_KEY, key, qfalse, 0, NULL );
			break;
		case SDL_TEXTINPUT: {
			const unsigned char *p = (const unsigned char *)e.text.text;
			while ( *p ) {
				if ( *p >= 32 && *p < 127 ) Sys_QueEvent( t, SE_CHAR, *p, 0, 0, NULL );
				p++;
			}
			break;
		}
		case SDL_MOUSEMOTION:
			if ( mouse_active ) {
				mx += e.motion.xrel;
				my += e.motion.yrel;
				gotMotion = qtrue;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			b = -1;
			if ( e.button.button == SDL_BUTTON_LEFT )   b = 0;
			else if ( e.button.button == SDL_BUTTON_RIGHT )  b = 1;
			else if ( e.button.button == SDL_BUTTON_MIDDLE ) b = 2;
			else if ( e.button.button == SDL_BUTTON_X1 ) b = 3;
			else if ( e.button.button == SDL_BUTTON_X2 ) b = 4;
			if ( b >= 0 ) Sys_QueEvent( t, SE_KEY, K_MOUSE1 + b, e.type == SDL_MOUSEBUTTONDOWN, 0, NULL );
			break;
		case SDL_MOUSEWHEEL:
			if ( e.wheel.y > 0 ) {
				Sys_QueEvent( t, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
				Sys_QueEvent( t, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
			} else if ( e.wheel.y < 0 ) {
				Sys_QueEvent( t, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
				Sys_QueEvent( t, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
			}
			break;
		case SDL_WINDOWEVENT:
			if ( e.window.event == SDL_WINDOWEVENT_FOCUS_LOST ) {
				IN_DeactivateMouse();
			}
			break;
		case SDL_QUIT:
			Cbuf_ExecuteText( EXEC_APPEND, "quit\n" );
			break;
		}
	}

	if ( gotMotion && ( mx || my ) ) {
		Sys_QueEvent( t, SE_MOUSE, mx, my, 0, NULL );
		mx = my = 0;
	}
}

void Sys_SendKeyEvents( void )
{
	if ( !sdlWindow ) return;
	HandleEvents();
}

void KBD_Init( void ) {}
void KBD_Close( void ) {}

/*
** gamma
*/
void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
	Uint16 r[256], g[256], b[256];
	int i;
	if ( !sdlWindow || !glConfig.deviceSupportsGamma ) return;
	for ( i = 0; i < 256; i++ ) {
		r[i] = (Uint16)( red[i] << 8 | red[i] );
		g[i] = (Uint16)( green[i] << 8 | green[i] );
		b[i] = (Uint16)( blue[i] << 8 | blue[i] );
	}
	SDL_SetWindowGammaRamp( sdlWindow, r, g, b );
}

void GLimp_Shutdown( void )
{
	IN_DeactivateMouse();
	if ( sdlContext ) { SDL_GL_DeleteContext( sdlContext ); sdlContext = NULL; }
	if ( sdlWindow )  { SDL_DestroyWindow( sdlWindow ); sdlWindow = NULL; }
	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	memset( &glConfig, 0, sizeof( glConfig ) );
	memset( &glState, 0, sizeof( glState ) );

	QGL_Shutdown();
}

void GLimp_LogComment( char *comment )
{
	if ( glw_state.log_fp ) {
		fprintf( glw_state.log_fp, "%s", comment );
	}
}

/*
** GLW_SetMode
*/
static int GLW_SetMode( int mode, qboolean fullscreen )
{
	int colorbits, depthbits, stencilbits;
	int tcolorbits, tdepthbits, tstencilbits;
	int i;
	Uint32 flags;
	const char *glstring;

	ri.Printf( PRINT_ALL, "Initializing OpenGL display (SDL2)\n" );
	ri.Printf( PRINT_ALL, "...setting mode %d:", mode );
	if ( !R_GetModeInfo( &glConfig.vidWidth, &glConfig.vidHeight, &glConfig.windowAspect, mode ) ) {
		ri.Printf( PRINT_ALL, " invalid mode\n" );
		return RSERR_INVALID_MODE;
	}
	ri.Printf( PRINT_ALL, " %d %d\n", glConfig.vidWidth, glConfig.vidHeight );

	if ( fullscreen ) {
		ri.Printf( PRINT_ALL, "...setting fullscreen\n" );
	}
	glConfig.isFullscreen = fullscreen;

	if ( r_colorbits->integer == 16 ) colorbits = 16; else colorbits = 24;
	depthbits = r_depthbits->integer ? r_depthbits->integer : 24;
	stencilbits = r_stencilbits->integer;
	if ( !r_depthbits->integer && colorbits == 16 ) depthbits = 16;

	for ( i = 0; i < 16; i++ ) {
		tcolorbits = colorbits;
		tdepthbits = depthbits;
		tstencilbits = stencilbits;
		if ( ( i % 4 ) == 3 && tcolorbits == 24 ) tcolorbits = 16;
		if ( ( i % 4 ) == 2 ) {
			if ( tdepthbits == 24 ) tdepthbits = 16;
			else if ( tdepthbits == 16 ) tdepthbits = 8;
		}
		if ( ( i % 4 ) == 1 ) {
			if ( tstencilbits > 0 ) tstencilbits = ( tstencilbits > 8 ) ? 8 : 0;
		}

		SDL_GL_ResetAttributes();
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   tcolorbits == 24 ? 8 : 4 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, tcolorbits == 24 ? 8 : 4 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  tcolorbits == 24 ? 8 : 4 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, tdepthbits );
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, tstencilbits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );

		flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
		if ( fullscreen ) flags |= SDL_WINDOW_FULLSCREEN;

		sdlWindow = SDL_CreateWindow( "Quake III Arena", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		                              glConfig.vidWidth, glConfig.vidHeight, flags );
		if ( !sdlWindow ) {
			ri.Printf( PRINT_ALL, "SDL_CreateWindow failed: %s\n", SDL_GetError() );
			continue;
		}
		sdlContext = SDL_GL_CreateContext( sdlWindow );
		if ( !sdlContext ) {
			ri.Printf( PRINT_ALL, "SDL_GL_CreateContext failed: %s\n", SDL_GetError() );
			SDL_DestroyWindow( sdlWindow );
			sdlWindow = NULL;
			continue;
		}

		ri.Printf( PRINT_ALL, "Using %d color bits, %d depth, %d stencil display.\n",
		           tcolorbits, tdepthbits, tstencilbits );
		glConfig.colorBits = tcolorbits;
		glConfig.depthBits = tdepthbits;
		glConfig.stencilBits = tstencilbits;
		break;
	}

	if ( !sdlContext ) {
		ri.Printf( PRINT_ALL, "Couldn't get a visual\n" );
		return RSERR_INVALID_MODE;
	}

	glstring = (const char *)qglGetString( GL_RENDERER );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glstring ? glstring : "(null)" );
	return RSERR_OK;
}

#define Q_stristr_local Q_stristr_sdl

static void GLW_InitGamma( void )
{
	Uint16 ramp[256];
	int i;
	glConfig.deviceSupportsGamma = qfalse;
	for ( i = 0; i < 256; i++ ) ramp[i] = (Uint16)( i << 8 | i );
	if ( SDL_SetWindowGammaRamp( sdlWindow, ramp, ramp, ramp ) == 0 ) {
		glConfig.deviceSupportsGamma = qtrue;
		ri.Printf( PRINT_ALL, "SDL gamma ramp supported\n" );
	} else {
		ri.Printf( PRINT_ALL, "SDL gamma ramp not supported (%s); using overbright fallback\n", SDL_GetError() );
	}
}

void GLimp_Init( void )
{
	char buf[1024];
	cvar_t *lastValidRenderer = ri.Cvar_Get( "r_lastValidRenderer", "(uninitialized)", CVAR_ARCHIVE );
	const char *s;

	r_allowSoftwareGL = ri.Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH );
	r_previousglDriver = ri.Cvar_Get( "r_previousglDriver", "", CVAR_ROM );
	InitSig();

	if ( *r_previousglDriver->string ) {
		ri.Cvar_Set( "r_glDriver", r_previousglDriver->string );
	}

	if ( !QGL_Init( r_glDriver->string ) ) {
		ri.Error( ERR_FATAL, "GLimp_Init() - could not load OpenGL subsystem\n" );
	}

	if ( GLW_SetMode( r_mode->integer, r_fullscreen->integer ) != RSERR_OK ) {
		// retry in windowed 640x480
		if ( GLW_SetMode( 3, qfalse ) != RSERR_OK ) {
			ri.Error( ERR_FATAL, "GLimp_Init() - could not create an OpenGL window\n" );
		}
	}

	ri.Cvar_Set( "r_previousglDriver", r_glDriver->string );

	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	Q_strncpyz( glConfig.vendor_string, (const char *)qglGetString( GL_VENDOR ), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (const char *)qglGetString( GL_RENDERER ), sizeof( glConfig.renderer_string ) );
	if ( *glConfig.renderer_string && glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] == '\n' )
		glConfig.renderer_string[strlen( glConfig.renderer_string ) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (const char *)qglGetString( GL_VERSION ), sizeof( glConfig.version_string ) );
	s = (const char *)qglGetString( GL_EXTENSIONS );
	Q_strncpyz( s_extensions, s ? s : "", sizeof( s_extensions ) );
	Q_strncpyz( glConfig.extensions_string, s_extensions, sizeof( glConfig.extensions_string ) );

	strcpy( buf, glConfig.renderer_string );
	strlwr( buf );

	if ( Q_stricmp( lastValidRenderer->string, glConfig.renderer_string ) ) {
		glConfig.hardwareType = GLHW_GENERIC;
		ri.Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
		ri.Cvar_Set( "r_picmip", "1" );
	}

	ri.Cvar_Set( "r_lastValidRenderer", glConfig.renderer_string );

	SDL_GL_SetSwapInterval( r_swapInterval->integer );

	GLW_InitExtensions();
	GLW_InitGamma();

	SDL_StartTextInput();
	InitSig();
}

void GLimp_EndFrame( void )
{
	if ( r_swapInterval->modified ) {
		r_swapInterval->modified = qfalse;
		SDL_GL_SetSwapInterval( r_swapInterval->integer );
	}
	if ( stricmp( r_drawBuffer->string, "GL_FRONT" ) != 0 ) {
		SDL_GL_SwapWindow( sdlWindow );
	}
	QGL_EnableLogging( (qboolean)r_logFile->integer );
}

// no SMP renderer on this platform
void GLimp_RenderThreadWrapper( void *stub ) {}
qboolean GLimp_SpawnRenderThread( void (*function)( void ) ) {
	ri.Printf( PRINT_WARNING, "ERROR: SMP support was disabled at compile time\n" );
	return qfalse;
}
void *GLimp_RendererSleep( void ) { return NULL; }
void GLimp_FrontEndSleep( void ) {}
void GLimp_WakeRenderer( void *data ) {}

/*
** input
*/
void IN_Init( void )
{
	Com_Printf( "\n------- Input Initialization -------\n" );
	in_mouse = Cvar_Get( "in_mouse", "1", CVAR_ARCHIVE );
	in_subframe = Cvar_Get( "in_subframe", "1", CVAR_ARCHIVE );
	in_nograb = Cvar_Get( "in_nograb", "0", 0 );
	in_joystick = Cvar_Get( "in_joystick", "0", CVAR_ARCHIVE | CVAR_LATCH );
	in_joystickDebug = Cvar_Get( "in_debugjoystick", "0", CVAR_TEMP );
	joy_threshold = Cvar_Get( "joy_threshold", "0.15", CVAR_ARCHIVE );

	mouse_avail = in_mouse->value ? qtrue : qfalse;

	IN_StartupJoystick();
	Com_Printf( "------------------------------------\n" );
}

void IN_Shutdown( void )
{
	IN_DeactivateMouse();
	mouse_avail = qfalse;
}

void IN_Frame( void )
{
	IN_JoyMove();

	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		// don't grab the mouse in the console when windowed
		if ( Cvar_VariableValue( "r_fullscreen" ) == 0 ) {
			IN_DeactivateMouse();
			return;
		}
	}
	IN_ActivateMouse();
}

void IN_Activate( void ) {}
