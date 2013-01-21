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

/**
 *  \file SDL_gamecontroller.h
 *  
 *  Include file for SDL game controller event handling
 */

#ifndef _SDL_gamecontroller_h
#define _SDL_gamecontroller_h

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_joystick.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
extern "C" {
/* *INDENT-ON* */
#endif

/**
 *  \file SDL_gamecontroller.h
 *
 *  In order to use these functions, SDL_Init() must have been called
 *  with the ::SDL_INIT_JOYSTICK flag.  This causes SDL to scan the system
 *  for game controllers, and load appropriate drivers.
 */

/* The gamecontroller structure used to identify an SDL game controller */
struct _SDL_GameController;
typedef struct _SDL_GameController SDL_GameController;


typedef enum 
{
	SDL_CONTROLLER_BINDTYPE_NONE = 0,
	SDL_CONTROLLER_BINDTYPE_BUTTON,
	SDL_CONTROLLER_BINDTYPE_AXIS,
	SDL_CONTROLLER_BINDTYPE_HAT
} SDL_CONTROLLER_BINDTYPE;
/**
 *  get the sdl joystick layer binding for this controller button/axis mapping
 */
struct _SDL_GameControllerHatBind
{
	int hat;
	int hat_mask;
};

typedef struct _SDL_GameControllerButtonBind
{
	SDL_CONTROLLER_BINDTYPE m_eBindType;
	union
	{
		int button;
		int axis;
		struct _SDL_GameControllerHatBind hat;
	};

} SDL_GameControllerButtonBind;


/**
 *  To count the number of game controllers in the system for the following:
 *	int nJoysticks = SDL_NumJoysticks();
 *	int nGameControllers = 0;
 *	for ( int i = 0; i < nJoysticks; i++ ) {
 *		if ( SDL_IsGameController(i) ) {
 *			nGameControllers++;
 *		}
 *  }
 *
 *  Using the SDL_HINT_GAMECONTROLLERCONFIG hint you can add support for controllers SDL is unaware of or cause an existing controller to have a different binding. The format is:
 *	guid,name,mappings
 *
 *  Where GUID is the string value from SDL_JoystickGetGUIDString(), name is the human readable string for the device and mappings are controller mappings to joystick ones.
 *  Under Windows there is a reserved GUID of "xinput" that covers any XInput devices.
 *	The mapping format for joystick is: 
 *		bX - a joystick button, index X
 *		hX.Y - hat X with value Y
 *		aX - axis X of the joystick
 *  Buttons can be used as a controller axis and vice versa.
 *
 *  This string shows an example of a valid mapping for a controller
 * 	"341a3608000000000000504944564944,Aferglow PS3 Controller,a:b1,b:b2,y:b3,x:b0,start:b9,guide:b12,back:b8,dpup:h0.1,dpleft:h0.8,dpdown:h0.4,dpright:h0.2,leftshoulder:b4,rightshoulder:b5,leftstick:b10,rightstick:b11,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:b6,righttrigger:b7",
 *
 */


/**
 *  Is the joystick on this index supported by the game controller interface?
 *		returns 1 if supported, 0 otherwise.
 */
extern DECLSPEC int SDLCALL SDL_IsGameController(int joystick_index);


/**
 *  Get the implementation dependent name of a game controller.
 *  This can be called before any controllers are opened.
 *  If no name can be found, this function returns NULL.
 */
extern DECLSPEC const char *SDLCALL SDL_GameControllerNameForIndex(int joystick_index);

/**
 *  Open a game controller for use.  
 *  The index passed as an argument refers to the N'th game controller on the system.  
 *  This index is the value which will identify this controller in future controller
 *  events.
 *  
 *  \return A controller identifier, or NULL if an error occurred.
 */
extern DECLSPEC SDL_GameController *SDLCALL SDL_GameControllerOpen(int joystick_index);

/**
 *  Return the name for this currently opened controller
 */
extern DECLSPEC const char *SDLCALL SDL_GameControllerName(SDL_GameController * gamecontroller);
	
/**
 *  Returns 1 if the controller has been opened and currently connected, or 0 if it has not.
 */
extern DECLSPEC int SDLCALL SDL_GameControllerGetAttached(SDL_GameController * gamecontroller);

/**
 *  Get the underlying joystick object used by a controller
 */
extern DECLSPEC SDL_Joystick *SDLCALL SDL_GameControllerGetJoystick(SDL_GameController * gamecontroller);

/**
 *  Enable/disable controller event polling.
 *  
 *  If controller events are disabled, you must call SDL_GameControllerUpdate()
 *  yourself and check the state of the controller when you want controller
 *  information.
 *  
 *  The state can be one of ::SDL_QUERY, ::SDL_ENABLE or ::SDL_IGNORE.
 */
extern DECLSPEC int SDLCALL SDL_GameControllerEventState(int state);

/**
 *  The list of axii available from a controller
 */
typedef enum 
{
	SDL_CONTROLLER_AXIS_INVALID = -1,
	SDL_CONTROLLER_AXIS_LEFTX,
	SDL_CONTROLLER_AXIS_LEFTY,
	SDL_CONTROLLER_AXIS_RIGHTX,
	SDL_CONTROLLER_AXIS_RIGHTY,
	SDL_CONTROLLER_AXIS_TRIGGERLEFT,
	SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
	SDL_CONTROLLER_AXIS_MAX
} SDL_CONTROLLER_AXIS;

/**
 *  turn this string into a axis mapping
 */
extern DECLSPEC SDL_CONTROLLER_AXIS SDLCALL SDL_GameControllerGetAxisFromString(const char *pchString);

/**
 *  get the sdl joystick layer binding for this controller button mapping
 */
extern DECLSPEC SDL_GameControllerButtonBind SDLCALL SDL_GameControllerGetBindForAxis(SDL_GameController * gamecontroller, SDL_CONTROLLER_AXIS button);

/**
 *  Get the current state of an axis control on a game controller.
 *  
 *  The state is a value ranging from -32768 to 32767.
 *  
 *  The axis indices start at index 0.
 */
extern DECLSPEC Sint16 SDLCALL SDL_GameControllerGetAxis(SDL_GameController * gamecontroller,
                                                   SDL_CONTROLLER_AXIS axis);

/**
 *  The list of buttons available from a controller
 */
typedef enum
{
	SDL_CONTROLLER_BUTTON_INVALID = -1,
	SDL_CONTROLLER_BUTTON_A,
	SDL_CONTROLLER_BUTTON_B,
	SDL_CONTROLLER_BUTTON_X,
	SDL_CONTROLLER_BUTTON_Y,
	SDL_CONTROLLER_BUTTON_BACK,
	SDL_CONTROLLER_BUTTON_GUIDE,
	SDL_CONTROLLER_BUTTON_START,
	SDL_CONTROLLER_BUTTON_LEFTSTICK,
	SDL_CONTROLLER_BUTTON_RIGHTSTICK,
	SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
	SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
	SDL_CONTROLLER_BUTTON_DPAD_UP,
	SDL_CONTROLLER_BUTTON_DPAD_DOWN,
	SDL_CONTROLLER_BUTTON_DPAD_LEFT,
	SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
	SDL_CONTROLLER_BUTTON_MAX
} SDL_CONTROLLER_BUTTON;

/**
 *  turn this string into a button mapping
 */
extern DECLSPEC SDL_CONTROLLER_BUTTON SDLCALL SDL_GameControllerGetButtonFromString(const char *pchString);


/**
 *  get the sdl joystick layer binding for this controller button mapping
 */
extern DECLSPEC SDL_GameControllerButtonBind SDLCALL SDL_GameControllerGetBindForButton(SDL_GameController * gamecontroller, SDL_CONTROLLER_BUTTON button);


/**
 *  Get the current state of a button on a game controller.
 *  
 *  The button indices start at index 0.
 */
extern DECLSPEC Uint8 SDLCALL SDL_GameControllerGetButton(SDL_GameController * gamecontroller,
                                                    SDL_CONTROLLER_BUTTON button);

/**
 *  Close a controller previously opened with SDL_GameControllerOpen().
 */
extern DECLSPEC void SDLCALL SDL_GameControllerClose(SDL_GameController * gamecontrollerk);


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
/* *INDENT-OFF* */
}
/* *INDENT-ON* */
#endif
#include "close_code.h"

#endif /* _SDL_gamecontroller_h */

/* vi: set ts=4 sw=4 expandtab: */
