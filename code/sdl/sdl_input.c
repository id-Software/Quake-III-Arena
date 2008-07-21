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

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "../renderer/tr_local.h"
#include "../client/client.h"
#include "../sys/sys_local.h"

#ifdef MACOS_X
// Mouse acceleration needs to be disabled
#define MACOS_X_ACCELERATION_HACK
// Cursor needs hack to hide
#define MACOS_X_CURSOR_HACK
#endif

#ifdef MACOS_X_ACCELERATION_HACK
#include <IOKit/IOTypes.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>
#endif

//#define KBD_DBG

static SDL_Joystick *stick = NULL;

static qboolean mouseAvailable = qfalse;
static qboolean mouseActive = qfalse;
static qboolean keyRepeatEnabled = qfalse;

static cvar_t *in_mouse;
#ifdef MACOS_X_ACCELERATION_HACK
static cvar_t *in_disablemacosxmouseaccel;
static double originalMouseSpeed = -1.0;
#endif
static cvar_t *in_nograb;

static cvar_t *in_joystick          = NULL;
static cvar_t *in_joystickDebug     = NULL;
static cvar_t *in_joystickThreshold = NULL;

/*
===============
IN_TranslateSDLToQ3Key
===============
*/
static const char *IN_TranslateSDLToQ3Key(SDL_keysym *keysym, int *key)
{
	static char buf[2] = { '\0', '\0' };
	*buf = '\0';

	*key = 0;

	// these happen to match the ASCII chars.
	if ((keysym->sym >= ' ') && (keysym->sym <= '~'))
	{
		*key = (int) keysym->sym;
	}
	else
	{
		switch (keysym->sym)
		{
			case SDLK_PAGEUP:       *key = K_PGUP;          break;
			case SDLK_KP9:          *key = K_KP_PGUP;       break;
			case SDLK_PAGEDOWN:     *key = K_PGDN;          break;
			case SDLK_KP3:          *key = K_KP_PGDN;       break;
			case SDLK_KP7:          *key = K_KP_HOME;       break;
			case SDLK_HOME:         *key = K_HOME;          break;
			case SDLK_KP1:          *key = K_KP_END;        break;
			case SDLK_END:          *key = K_END;           break;
			case SDLK_KP4:          *key = K_KP_LEFTARROW;  break;
			case SDLK_LEFT:         *key = K_LEFTARROW;     break;
			case SDLK_KP6:          *key = K_KP_RIGHTARROW; break;
			case SDLK_RIGHT:        *key = K_RIGHTARROW;    break;
			case SDLK_KP2:          *key = K_KP_DOWNARROW;  break;
			case SDLK_DOWN:         *key = K_DOWNARROW;     break;
			case SDLK_KP8:          *key = K_KP_UPARROW;    break;
			case SDLK_UP:           *key = K_UPARROW;       break;
			case SDLK_ESCAPE:       *key = K_ESCAPE;        break;
			case SDLK_KP_ENTER:     *key = K_KP_ENTER;      break;
			case SDLK_RETURN:       *key = K_ENTER;         break;
			case SDLK_TAB:          *key = K_TAB;           break;
			case SDLK_F1:           *key = K_F1;            break;
			case SDLK_F2:           *key = K_F2;            break;
			case SDLK_F3:           *key = K_F3;            break;
			case SDLK_F4:           *key = K_F4;            break;
			case SDLK_F5:           *key = K_F5;            break;
			case SDLK_F6:           *key = K_F6;            break;
			case SDLK_F7:           *key = K_F7;            break;
			case SDLK_F8:           *key = K_F8;            break;
			case SDLK_F9:           *key = K_F9;            break;
			case SDLK_F10:          *key = K_F10;           break;
			case SDLK_F11:          *key = K_F11;           break;
			case SDLK_F12:          *key = K_F12;           break;
			case SDLK_F13:          *key = K_F13;           break;
			case SDLK_F14:          *key = K_F14;           break;
			case SDLK_F15:          *key = K_F15;           break;

			case SDLK_BACKSPACE:    *key = K_BACKSPACE;     break; // ctrl-h
			case SDLK_KP_PERIOD:    *key = K_KP_DEL;        break;
			case SDLK_DELETE:       *key = K_DEL;           break;
			case SDLK_PAUSE:        *key = K_PAUSE;         break;

			case SDLK_LSHIFT:
			case SDLK_RSHIFT:       *key = K_SHIFT;         break;

			case SDLK_LCTRL:
			case SDLK_RCTRL:        *key = K_CTRL;          break;

			case SDLK_RMETA:
			case SDLK_LMETA:        *key = K_COMMAND;       break;

			case SDLK_RALT:
			case SDLK_LALT:         *key = K_ALT;           break;

			case SDLK_LSUPER:
			case SDLK_RSUPER:       *key = K_SUPER;         break;

			case SDLK_KP5:          *key = K_KP_5;          break;
			case SDLK_INSERT:       *key = K_INS;           break;
			case SDLK_KP0:          *key = K_KP_INS;        break;
			case SDLK_KP_MULTIPLY:  *key = K_KP_STAR;       break;
			case SDLK_KP_PLUS:      *key = K_KP_PLUS;       break;
			case SDLK_KP_MINUS:     *key = K_KP_MINUS;      break;
			case SDLK_KP_DIVIDE:    *key = K_KP_SLASH;      break;

			case SDLK_MODE:         *key = K_MODE;          break;
			case SDLK_COMPOSE:      *key = K_COMPOSE;       break;
			case SDLK_HELP:         *key = K_HELP;          break;
			case SDLK_PRINT:        *key = K_PRINT;         break;
			case SDLK_SYSREQ:       *key = K_SYSREQ;        break;
			case SDLK_BREAK:        *key = K_BREAK;         break;
			case SDLK_MENU:         *key = K_MENU;          break;
			case SDLK_POWER:        *key = K_POWER;         break;
			case SDLK_EURO:         *key = K_EURO;          break;
			case SDLK_UNDO:         *key = K_UNDO;          break;
			case SDLK_SCROLLOCK:    *key = K_SCROLLOCK;     break;
			case SDLK_NUMLOCK:      *key = K_KP_NUMLOCK;    break;
			case SDLK_CAPSLOCK:     *key = K_CAPSLOCK;      break;

			default:
				if (keysym->sym >= SDLK_WORLD_0 && keysym->sym <= SDLK_WORLD_95)
					*key = (keysym->sym - SDLK_WORLD_0) + K_WORLD_0;
				break;
		}
	}

	if( keysym->unicode <= 127 )  // maps to ASCII?
	{
		char ch = (char) keysym->unicode;
		if (ch == '~')
			*key = '~'; // console HACK

		// translate K_BACKSPACE to ctrl-h for MACOS_X (others?)
		if (ch == K_BACKSPACE && keysym->sym != SDLK_DELETE)
		{
			*key = 'h' - 'a' + 1;
			buf[0] = *key;
		}
		else
			buf[0] = ch;
	}

	return buf;
}

/*
===============
IN_PrintKey
===============
*/
static void IN_PrintKey(const SDL_Event* event)
{
#ifdef KBD_DBG
	fprintf( stderr, "key name: %s", SDL_GetKeyName (event->key.keysym.sym ) );
	if(event->key.keysym.unicode)
	{
		fprintf( stderr, " unicode: %hx", event->key.keysym.unicode );
		if( event->key.keysym.unicode >= '0' &&
				event->key.keysym.unicode <= '~')  // printable?
		{
			fprintf( stderr, " (%c)", (unsigned char)event->key.keysym.unicode );
		}
	}
	fflush( stderr );
#endif
}

#ifdef MACOS_X_ACCELERATION_HACK
/*
===============
IN_GetIOHandle
===============
*/
static io_connect_t IN_GetIOHandle(void) // mac os x mouse accel hack
{
	io_connect_t iohandle = MACH_PORT_NULL;
	kern_return_t status;
	io_service_t iohidsystem = MACH_PORT_NULL;
	mach_port_t masterport;

	status = IOMasterPort(MACH_PORT_NULL, &masterport);
	if(status != KERN_SUCCESS)
		return 0;

	iohidsystem = IORegistryEntryFromPath(masterport, kIOServicePlane ":/IOResources/IOHIDSystem");
	if(!iohidsystem)
		return 0;

	status = IOServiceOpen(iohidsystem, mach_task_self(), kIOHIDParamConnectType, &iohandle);
	IOObjectRelease(iohidsystem);

	return iohandle;
}
#endif

/*
===============
IN_ActivateMouse
===============
*/
static void IN_ActivateMouse( void )
{
	if (!mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

#ifdef MACOS_X_ACCELERATION_HACK
	if (!mouseActive) // mac os x mouse accel hack
	{
		// Save the status of mouse acceleration
		originalMouseSpeed = -1.0; // in case of error
		if(in_disablemacosxmouseaccel->integer)
		{
			io_connect_t mouseDev = IN_GetIOHandle();
			if(mouseDev != 0)
			{
				if(IOHIDGetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), &originalMouseSpeed) == kIOReturnSuccess)
				{
					Com_Printf("previous mouse acceleration: %f\n", originalMouseSpeed);
					if(IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), -1.0) != kIOReturnSuccess)
					{
						Com_Printf("Could not disable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
						Cvar_Set ("in_disablemacosxmouseaccel", 0);
					}
				}
				else
				{
					Com_Printf("Could not disable mouse acceleration (failed at IOHIDGetAccelerationWithKey).\n");
					Cvar_Set ("in_disablemacosxmouseaccel", 0);
				}
				IOServiceClose(mouseDev);
			}
			else
			{
				Com_Printf("Could not disable mouse acceleration (failed at IO_GetIOHandle).\n");
				Cvar_Set ("in_disablemacosxmouseaccel", 0);
			}
		}
	}
#endif

	if( !mouseActive )
	{
		SDL_ShowCursor( 0 );
#ifdef MACOS_X_CURSOR_HACK
		// This is a bug in the current SDL/macosx...have to toggle it a few
		//  times to get the cursor to hide.
		SDL_ShowCursor( 1 );
		SDL_ShowCursor( 0 );
#endif
		SDL_WM_GrabInput( SDL_GRAB_ON );
	}

	// in_nograb makes no sense in fullscreen mode
	if( !r_fullscreen->integer )
	{
		if( in_nograb->modified || !mouseActive )
		{
			if( in_nograb->integer )
				SDL_WM_GrabInput( SDL_GRAB_OFF );
			else
				SDL_WM_GrabInput( SDL_GRAB_ON );

			in_nograb->modified = qfalse;
		}
	}

	mouseActive = qtrue;
}

/*
===============
IN_DeactivateMouse
===============
*/
static void IN_DeactivateMouse( void )
{
	if (!mouseAvailable || !SDL_WasInit( SDL_INIT_VIDEO ) )
		return;

#ifdef MACOS_X_ACCELERATION_HACK
	if (mouseActive) // mac os x mouse accel hack
	{
		if(originalMouseSpeed != -1.0)
		{
			io_connect_t mouseDev = IN_GetIOHandle();
			if(mouseDev != 0)
			{
				Com_Printf("restoring mouse acceleration to: %f\n", originalMouseSpeed);
				if(IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), originalMouseSpeed) != kIOReturnSuccess)
					Com_Printf("Could not re-enable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
				IOServiceClose(mouseDev);
			}
			else
				Com_Printf("Could not re-enable mouse acceleration (failed at IO_GetIOHandle).\n");
		}
	}
#endif

	if( mouseActive )
	{
		SDL_WM_GrabInput( SDL_GRAB_OFF );
		SDL_WarpMouse( glConfig.vidWidth / 2, glConfig.vidHeight / 2 );
		SDL_ShowCursor( 1 );

		mouseActive = qfalse;
	}
}

// We translate axes movement into keypresses
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

// translate hat events into keypresses
// the 4 highest buttons are used for the first hat ...
static int hat_keys[16] = {
	K_JOY29, K_JOY30,
	K_JOY31, K_JOY32,
	K_JOY25, K_JOY26,
	K_JOY27, K_JOY28,
	K_JOY21, K_JOY22,
	K_JOY23, K_JOY24,
	K_JOY17, K_JOY18,
	K_JOY19, K_JOY20
};


extern cvar_t *  in_joystick;
extern cvar_t *  in_joystickDebug;
extern cvar_t *  in_joystickThreshold;
cvar_t *in_joystickNo;

#define ARRAYLEN(x) (sizeof (x) / sizeof (x[0]))
struct
{
	qboolean buttons[16];  // !!! FIXME: these might be too many.
	unsigned int oldaxes;
	unsigned int oldhats;
} stick_state;


/*
===============
IN_StartupJoystick
===============
*/
static void IN_StartupJoystick( void )
{
	int i = 0;
	int total = 0;

	if (stick != NULL)
		SDL_JoystickClose(stick);

	stick = NULL;
	memset(&stick_state, '\0', sizeof (stick_state));

	if( !in_joystick->integer ) {
		Com_DPrintf( "Joystick is not active.\n" );
		return;
	}

	if (!SDL_WasInit(SDL_INIT_JOYSTICK))
	{
		Com_DPrintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");
		if (SDL_Init(SDL_INIT_JOYSTICK) == -1)
		{
			Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
			return;
		}
		Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
	}

	total = SDL_NumJoysticks();
	Com_DPrintf("%d possible joysticks\n", total);
	for (i = 0; i < total; i++)
		Com_DPrintf("[%d] %s\n", i, SDL_JoystickName(i));

	in_joystickNo = Cvar_Get( "in_joystickNo", "0", CVAR_ARCHIVE );
	if( in_joystickNo->integer < 0 || in_joystickNo->integer >= total )
		Cvar_Set( "in_joystickNo", "0" );

	stick = SDL_JoystickOpen( in_joystickNo->integer );

	if (stick == NULL) {
		Com_DPrintf( "No joystick opened.\n" );
		return;
	}

	Com_DPrintf( "Joystick %d opened\n", in_joystickNo->integer );
	Com_DPrintf( "Name:    %s\n", SDL_JoystickName(in_joystickNo->integer) );
	Com_DPrintf( "Axes:    %d\n", SDL_JoystickNumAxes(stick) );
	Com_DPrintf( "Hats:    %d\n", SDL_JoystickNumHats(stick) );
	Com_DPrintf( "Buttons: %d\n", SDL_JoystickNumButtons(stick) );
	Com_DPrintf( "Balls: %d\n", SDL_JoystickNumBalls(stick) );

	SDL_JoystickEventState(SDL_QUERY);

	return;
}

/*
===============
IN_JoyMove
===============
*/
static void IN_JoyMove( void )
{
	qboolean joy_pressed[ARRAYLEN(joy_keys)];
	unsigned int axes = 0;
	unsigned int hats = 0;
	int total = 0;
	int i = 0;

	if (!stick)
		return;

	SDL_JoystickUpdate();

	memset(joy_pressed, '\0', sizeof (joy_pressed));

	// update the ball state.
	total = SDL_JoystickNumBalls(stick);
	if (total > 0)
	{
		int balldx = 0;
		int balldy = 0;
		for (i = 0; i < total; i++)
		{
			int dx = 0;
			int dy = 0;
			SDL_JoystickGetBall(stick, i, &dx, &dy);
			balldx += dx;
			balldy += dy;
		}
		if (balldx || balldy)
		{
			// !!! FIXME: is this good for stick balls, or just mice?
			// Scale like the mouse input...
			if (abs(balldx) > 1)
				balldx *= 2;
			if (abs(balldy) > 1)
				balldy *= 2;
			Com_QueueEvent( 0, SE_MOUSE, balldx, balldy, 0, NULL );
		}
	}

	// now query the stick buttons...
	total = SDL_JoystickNumButtons(stick);
	if (total > 0)
	{
		if (total > ARRAYLEN(stick_state.buttons))
			total = ARRAYLEN(stick_state.buttons);
		for (i = 0; i < total; i++)
		{
			qboolean pressed = (SDL_JoystickGetButton(stick, i) != 0);
			if (pressed != stick_state.buttons[i])
			{
				Com_QueueEvent( 0, SE_KEY, K_JOY1 + i, pressed, 0, NULL );
				stick_state.buttons[i] = pressed;
			}
		}
	}

	// look at the hats...
	total = SDL_JoystickNumHats(stick);
	if (total > 0)
	{
		if (total > 4) total = 4;
		for (i = 0; i < total; i++)
		{
			((Uint8 *)&hats)[i] = SDL_JoystickGetHat(stick, i);
		}
	}

	// update hat state
	if (hats != stick_state.oldhats)
	{
		for( i = 0; i < 4; i++ ) {
			if( ((Uint8 *)&hats)[i] != ((Uint8 *)&stick_state.oldhats)[i] ) {
				// release event
				switch( ((Uint8 *)&stick_state.oldhats)[i] ) {
					case SDL_HAT_UP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 1], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 0], qfalse, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 2], qfalse, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 3], qfalse, 0, NULL );
						break;
					default:
						break;
				}
				// press event
				switch( ((Uint8 *)&hats)[i] ) {
					case SDL_HAT_UP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_DOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFT:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_RIGHTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 1], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTUP:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 0], qtrue, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					case SDL_HAT_LEFTDOWN:
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 2], qtrue, 0, NULL );
						Com_QueueEvent( 0, SE_KEY, hat_keys[4*i + 3], qtrue, 0, NULL );
						break;
					default:
						break;
				}
			}
		}
	}

	// save hat state
	stick_state.oldhats = hats;

	// finally, look at the axes...
	total = SDL_JoystickNumAxes(stick);
	if (total > 0)
	{
		if (total > 16) total = 16;
		for (i = 0; i < total; i++)
		{
			Sint16 axis = SDL_JoystickGetAxis(stick, i);
			float f = ( (float) axis ) / 32767.0f;
			if( f < -in_joystickThreshold->value ) {
				axes |= ( 1 << ( i * 2 ) );
			} else if( f > in_joystickThreshold->value ) {
				axes |= ( 1 << ( ( i * 2 ) + 1 ) );
			}
		}
	}

	/* Time to update axes state based on old vs. new. */
	if (axes != stick_state.oldaxes)
	{
		for( i = 0; i < 16; i++ ) {
			if( ( axes & ( 1 << i ) ) && !( stick_state.oldaxes & ( 1 << i ) ) ) {
				Com_QueueEvent( 0, SE_KEY, joy_keys[i], qtrue, 0, NULL );
			}

			if( !( axes & ( 1 << i ) ) && ( stick_state.oldaxes & ( 1 << i ) ) ) {
				Com_QueueEvent( 0, SE_KEY, joy_keys[i], qfalse, 0, NULL );
			}
		}
	}

	/* Save for future generations. */
	stick_state.oldaxes = axes;
}

/*
===============
IN_ProcessEvents
===============
*/
static void IN_ProcessEvents( void )
{
	SDL_Event e;
	const char *p = NULL;
	int key = 0;

	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
			return;

	if( Key_GetCatcher( ) == 0 && keyRepeatEnabled )
	{
		SDL_EnableKeyRepeat( 0, 0 );
		keyRepeatEnabled = qfalse;
	}
	else if( !keyRepeatEnabled )
	{
		SDL_EnableKeyRepeat( SDL_DEFAULT_REPEAT_DELAY,
			SDL_DEFAULT_REPEAT_INTERVAL );
		keyRepeatEnabled = qtrue;
	}

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_KEYDOWN:
				IN_PrintKey(&e);
				p = IN_TranslateSDLToQ3Key(&e.key.keysym, &key);
				if( key )
					Com_QueueEvent( 0, SE_KEY, key, qtrue, 0, NULL );

				if( p )
				{
					while( *p )
						Com_QueueEvent( 0, SE_CHAR, *p++, 0, 0, NULL );
				}
				break;

			case SDL_KEYUP:
				IN_TranslateSDLToQ3Key(&e.key.keysym, &key);
				Com_QueueEvent( 0, SE_KEY, key, qfalse, 0, NULL );
				break;

			case SDL_MOUSEMOTION:
				if (mouseActive)
					Com_QueueEvent( 0, SE_MOUSE, e.motion.xrel, e.motion.yrel, 0, NULL );
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				{
					unsigned char b;
					switch (e.button.button)
					{
						case 1:   b = K_MOUSE1;     break;
						case 2:   b = K_MOUSE3;     break;
						case 3:   b = K_MOUSE2;     break;
						case 4:   b = K_MWHEELUP;   break;
						case 5:   b = K_MWHEELDOWN; break;
						case 6:   b = K_MOUSE4;     break;
						case 7:   b = K_MOUSE5;     break;
						default:  b = K_AUX1 + (e.button.button - 8)%16; break;
					}
					Com_QueueEvent( 0, SE_KEY, b,
						( e.type == SDL_MOUSEBUTTONDOWN ? qtrue : qfalse ), 0, NULL );
				}
				break;

			case SDL_QUIT:
				Sys_Quit();
				break;

			default:
				break;
		}
	}
}

/*
===============
IN_Frame
===============
*/
void IN_Frame (void)
{
	IN_JoyMove( );

	// Release the mouse if the console is down in windowed mode
	// or if the window loses focus due to task switching
	if( ( ( Key_GetCatcher( ) & KEYCATCH_CONSOLE ) && !r_fullscreen->integer ) ||
	    !( SDL_GetAppState() & SDL_APPINPUTFOCUS ) )
		IN_DeactivateMouse( );
	else
		IN_ActivateMouse( );

	IN_ProcessEvents( );
}

/*
===============
IN_Init
===============
*/
void IN_Init(void)
{
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Error( ERR_FATAL, "IN_Init called before SDL_Init( SDL_INIT_VIDEO )\n" );
		return;
	}

	Com_DPrintf ("\n------- Input Initialization -------\n");

	// mouse variables
	in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	in_nograb = Cvar_Get ("in_nograb", "0", CVAR_ARCHIVE);

	in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE|CVAR_LATCH);
	in_joystickDebug = Cvar_Get ("in_debugjoystick", "0", CVAR_TEMP);
	in_joystickThreshold = Cvar_Get ("in_joystickThreshold", "0.15", CVAR_ARCHIVE);

#ifdef MACOS_X_ACCELERATION_HACK
	in_disablemacosxmouseaccel = Cvar_Get ("in_disablemacosxmouseaccel", "1", CVAR_ARCHIVE);
#endif

	Cvar_Set( "cl_platformSensitivity", "1.0" );

	SDL_EnableUNICODE(1);
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	keyRepeatEnabled = qtrue;

	if (in_mouse->value)
		mouseAvailable = qtrue;
	else
		mouseAvailable = qfalse;

	IN_StartupJoystick( );
	Com_DPrintf ("------------------------------------\n");
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown(void)
{
	IN_DeactivateMouse();

	mouseAvailable = qfalse;

	if (stick)
	{
		SDL_JoystickClose(stick);
		stick = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}
