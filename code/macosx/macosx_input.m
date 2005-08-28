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
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <ApplicationServices/ApplicationServices.h>

#import "../client/client.h"
#import "macosx_local.h"
#import "../renderer/tr_local.h"

#import "Q3Controller.h"
//#import "CGMouseDeltaFix.h"
#import "macosx_timers.h"
#import "macosx_display.h" // For Sys_SetScreenFade

#import <drivers/event_status_driver.h>
#import <sys/types.h>
#import <sys/time.h>
#import <unistd.h>


static qboolean inputActive;

static NSDate *distantPast;

static cvar_t *in_nomouse;
static cvar_t *in_showevents;
static cvar_t *in_mouseLowEndSlope;
static cvar_t *in_mouseHighEndCutoff;
static cvar_t *in_disableOSMouseScaling;

static void Sys_StartMouseInput();
static void Sys_StopMouseInput();
static qboolean mouseactive = qfalse;
static BOOL inputRectValid = NO;
static CGRect inputRect;
static NXMouseScaling originalScaling;

static unsigned int currentModifierFlags;



static void Sys_PreventMouseMovement(CGPoint point)
{
    CGEventErr err;

    //Com_Printf("**** Calling CGAssociateMouseAndMouseCursorPosition(false)\n");
    err = CGAssociateMouseAndMouseCursorPosition(false);
    if (err != CGEventNoErr) {
        Sys_Error("Could not disable mouse movement, CGAssociateMouseAndMouseCursorPosition returned %d\n", err);
    }

    // Put the mouse in the position we want to leave it at
    err = CGWarpMouseCursorPosition(point);
    if (err != CGEventNoErr) {
        Sys_Error("Could not disable mouse movement, CGWarpMouseCursorPosition returned %d\n", err);
    }
}

static void Sys_ReenableMouseMovement()
{
    CGEventErr err;
    
    //Com_Printf("**** Calling CGAssociateMouseAndMouseCursorPosition(true)\n");
    
    err = CGAssociateMouseAndMouseCursorPosition(true);
    if (err != CGEventNoErr) {
        Sys_Error("Could not reenable mouse movement, CGAssociateMouseAndMouseCursorPosition returned %d\n", err);
    }
    
    // Leave the mouse where it was -- don't warp here.
}


void Sys_InitInput(void)
{
    // no input with dedicated servers
    if ( com_dedicated->integer ) {
            return;
    }
	
    // The Cvars don't seem to work really early.
    [(Q3Controller *)[NSApp delegate] showBanner];

    Com_Printf( "------- Input Initialization -------\n" );

    if (!distantPast)
        distantPast = [[NSDate distantPast] retain];

    // For hide support.  If we don't do this, then the command key will get stuck on when we hide (since we won't get the flags changed event when it goes up).
    currentModifierFlags = 0;
    
    r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );
    in_nomouse = Cvar_Get( "in_nomouse", "0", 0 );
    in_showevents = Cvar_Get( "in_showevents", "0", 0 );

    // these defaults were arrived at via emprical testing between a Windows box and a Mac OS X box
#define ACT_LIKE_WINDOWS
#ifdef ACT_LIKE_WINDOWS
    in_mouseLowEndSlope = Cvar_Get("in_mouseLowEndSlope", "3.5", CVAR_ARCHIVE);
    if (in_mouseLowEndSlope->value < 1) {
        Cvar_Set("in_mouseLowEndSlope", "1");
    }
#else
    in_mouseLowEndSlope = Cvar_Get("in_mouseLowEndSlope", "1", CVAR_ARCHIVE);
    if (in_mouseLowEndSlope->value < 1) {
        Cvar_Set("in_mouseLowEndSlope", "1");
    }
#endif

    in_mouseHighEndCutoff = Cvar_Get("in_mouseHighEndCutoff", "20", CVAR_ARCHIVE);
    if (in_mouseLowEndSlope->value < 1) {
        Cvar_Set("in_mouseHighEndCutoff", "1");
    }
    in_disableOSMouseScaling = Cvar_Get("in_disableOSMouseScaling", "1", CVAR_ARCHIVE );
    
    glw_state.display = Sys_DisplayToUse();

    inputActive = qtrue;

    if ( in_nomouse->integer == 0 )
        Sys_StartMouseInput();
    else
        Com_Printf( "  in_nomouse is set, skipping.\n" );

    Com_Printf( "------------------------------------\n" );
}

void Sys_ShutdownInput(void)
{
    // no input with dedicated servers
    if ( !com_dedicated || com_dedicated->integer ) {
            return;
    }

    Com_Printf( "------- Input Shutdown -------\n" );
    if ( !inputActive ) {
        return;
    }
    inputActive = qfalse;

    if (mouseactive)
        Sys_StopMouseInput();

    Com_Printf( "------------------------------\n" );
}

static void Sys_LockMouseInInputRect(CGRect rect)
{
    CGPoint center;
    
    center.x = rect.origin.x + rect.size.width / 2.0;
    center.y = rect.origin.y + rect.size.height / 2.0;

    // Now, put the mouse in the middle of the input rect (anywhere over it would do)
    // and don't allow it to move.  This means that the user won't be able to accidentally
    // select another application.
    Sys_PreventMouseMovement(center);
}

extern void Sys_UpdateWindowMouseInputRect(void);

static void Sys_StartMouseInput()
{
    NXEventHandle eventStatus;
    CGMouseDelta dx, dy;

    if (mouseactive) {
        //Com_Printf("**** Attempted to start mouse input while already started\n");
        return;
    }

    Com_Printf("Starting mouse input\n");

    mouseactive = qtrue;
    if (inputRectValid && !glConfig.isFullscreen)
        // Make sure that if window moved we don't hose the user...
        Sys_UpdateWindowMouseInputRect();

    Sys_LockMouseInInputRect(inputRect);

    // Grab any mouse delta information to reset the last delta buffer
    CGGetLastMouseDelta(&dx, &dy);
    
    // Turn off mouse scaling
    if (in_disableOSMouseScaling->integer==0 && (eventStatus = NXOpenEventStatus())) {
        NXMouseScaling newScaling;

        NXGetMouseScaling(eventStatus, &originalScaling);
        newScaling.numScaleLevels = 1;
        newScaling.scaleThresholds[0] = 1;
        newScaling.scaleFactors[0] = -1;
        NXSetMouseScaling(eventStatus, &newScaling);
        NXCloseEventStatus(eventStatus);
    }
    
    [NSCursor hide];
}

static void Sys_StopMouseInput()
{
    NXEventHandle eventStatus;
    if (!mouseactive) {
        //Com_Printf("**** Attempted to stop mouse input while already stopped\n");
        return;
    }
    
    Com_Printf("Stopping mouse input\n");
    
    // Restore mouse scaling
    if (in_disableOSMouseScaling->integer == 0 && (eventStatus = NXOpenEventStatus())) {
        NXSetMouseScaling(eventStatus, &originalScaling);
        NXCloseEventStatus(eventStatus);
    }

    mouseactive = qfalse;
    Sys_ReenableMouseMovement();

    [NSCursor unhide];
}

//===========================================================================

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

static char *Sys_ConsoleInput(void)
{
    extern qboolean stdin_active;
    static char text[256];
    int     len;
    fd_set	fdset;
    struct timeval timeout;

    if (!com_dedicated || !com_dedicated->integer)
        return NULL;
    
    if (!stdin_active)
        return NULL;
    
    FD_ZERO(&fdset);
    FD_SET(fileno(stdin), &fdset);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(fileno(stdin), &fdset))
        return NULL;

    len = read (fileno(stdin), text, sizeof(text));
    if (len == 0) { // eof!
        stdin_active = qfalse;
        return NULL;
    }

    if (len < 1)
        return NULL;
    text[len-1] = 0;    // rip off the /n and terminate

    return text;
}

//===========================================================================
// Mouse input
//===========================================================================

#define MAX_DISPLAYS 128

CGDirectDisplayID Sys_DisplayToUse(void)
{
    static BOOL gotDisplay =  NO;
    static CGDirectDisplayID displayToUse;
    
    cvar_t   *vid_screen;
    CGDisplayErr err;
    CGDirectDisplayID displays[MAX_DISPLAYS];
    CGDisplayCount displayCount;
    int displayIndex;
    
    if (gotDisplay)
        return displayToUse;
    gotDisplay = YES;    
    
    err = CGGetActiveDisplayList(MAX_DISPLAYS, displays, &displayCount);
    if (err != CGDisplayNoErr)
        Sys_Error("Cannot get display list -- CGGetActiveDisplayList returned %d.\n", err);

    // -1, the default, means to use the main screen
    if ((vid_screen = Cvar_Get("vid_screen", "-1", CVAR_ARCHIVE)))
        displayIndex = vid_screen->integer;
    else
        displayIndex = -1;
        
    if (displayIndex < 0 || displayIndex >= displayCount)
        // This is documented (in CGDirectDisplay.h) to be the main display.  We want to
        // return this instead of kCGDirectMainDisplay since this will allow us to compare
        // display IDs.
        displayToUse = displays[0];
    else
        displayToUse = displays[displayIndex];

    return displayToUse;
}

void Sys_SetMouseInputRect(CGRect newRect)
{
    inputRectValid = YES;
    inputRect = newRect;
    //Com_Printf("**** inputRect = (%f, %f, %f, %f)\n", newRect.origin.x, newRect.origin.y, newRect.size.width, newRect.size.height);
    
    if (mouseactive)
        Sys_LockMouseInInputRect(inputRect);
}


static void Sys_ProcessMouseMovedEvent(NSEvent *mouseMovedEvent, int currentTime)
{
    float dx, dy;
    
    if (!mouseactive)
        return;
        
    dx = [mouseMovedEvent deltaX];
    dy = [mouseMovedEvent deltaY];
    
    if (in_showevents->integer)
        Com_Printf("MOUSE MOVED: %d, %d\n", dx, dy);

    Sys_QueEvent(currentTime, SE_MOUSE, dx, dy, 0, NULL );
}

// If we are 'paused' (i.e., in any state that our normal key bindings aren't in effect), then interpret cmd-h and cmd-tab as hiding the application.
static qboolean maybeHide()
{
    if ((currentModifierFlags & NSCommandKeyMask) == 0)
        return qfalse;

    return Sys_Hide();
}

static inline void sendEventForCharacter(NSEvent *event, unichar character, qboolean keyDownFlag, int currentTime)
{
    if (in_showevents->integer)
        Com_Printf("CHARACTER: 0x%02x down=%d\n", character, keyDownFlag);
        
#ifdef OMNI_TIMER
    if (character == NSF9FunctionKey && !keyDownFlag) {
        // Log and reset the root timer.  We should currently only have the root on the stack.
        OTStackPopRoot();
        OTStackReportResults(NULL);
        OTStackReset();
        OTStackPushRoot(rootNode);
    }
#endif

    switch (character) {
        case 0x03:
            Sys_QueEvent(currentTime, SE_KEY, K_KP_ENTER, keyDownFlag, 0, NULL);
            break;
        case '\b':
        case '\177':
            Sys_QueEvent(currentTime, SE_KEY, K_BACKSPACE, keyDownFlag, 0, NULL);
            if (keyDownFlag) {
               Sys_QueEvent(currentTime, SE_CHAR, '\b', 0, 0, NULL);
            }
            break;
        case '\t':
            if (maybeHide())
                return;
            Sys_QueEvent(currentTime, SE_KEY, K_TAB, keyDownFlag, 0, NULL);
            if (keyDownFlag) {
                Sys_QueEvent(currentTime, SE_CHAR, '\t', 0, 0, NULL);
            }
            break;
        case '\r':
        case '\n':
            Sys_QueEvent(currentTime, SE_KEY, K_ENTER, keyDownFlag, 0, NULL);
            if (keyDownFlag) {
                Sys_QueEvent(currentTime, SE_CHAR, '\r', 0, 0, NULL);
            }
            break;
        case '\033':
            Sys_QueEvent(currentTime, SE_KEY, K_ESCAPE, keyDownFlag, 0, NULL);
            break;
        case ' ':
            Sys_QueEvent(currentTime, SE_KEY, K_SPACE, keyDownFlag, 0, NULL);
            if (keyDownFlag) {
                Sys_QueEvent(currentTime, SE_CHAR, ' ', 0, 0, NULL);
            }
            break;
        case NSUpArrowFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_UPARROW, keyDownFlag, 0, NULL);
            break;
        case NSDownArrowFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_DOWNARROW, keyDownFlag, 0, NULL);
            break;
        case NSLeftArrowFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_LEFTARROW, keyDownFlag, 0, NULL);
            break;
        case NSRightArrowFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_RIGHTARROW, keyDownFlag, 0, NULL);
            break;
        case NSF1FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F1, keyDownFlag, 0, NULL);
            break;
        case NSF2FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F2, keyDownFlag, 0, NULL);
            break;
        case NSF3FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F3, keyDownFlag, 0, NULL);
            break;
        case NSF4FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F4, keyDownFlag, 0, NULL);
            break;
        case NSF5FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F5, keyDownFlag, 0, NULL);
            break;
        case NSF6FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F6, keyDownFlag, 0, NULL);
            break;
        case NSF7FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F7, keyDownFlag, 0, NULL);
            break;
        case NSF8FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F8, keyDownFlag, 0, NULL);
            break;
        case NSF9FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F9, keyDownFlag, 0, NULL);
            break;
        case NSF10FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F10, keyDownFlag, 0, NULL);
            break;
        case NSF11FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F11, keyDownFlag, 0, NULL);
            break;
        case NSF12FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_F12, keyDownFlag, 0, NULL);
            break;
        case NSF13FunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, '`', keyDownFlag, 0, NULL);
            if (keyDownFlag) {
                Sys_QueEvent(currentTime, SE_CHAR, '`', 0, 0, NULL);
            }
            break;
        case NSInsertFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_INS, keyDownFlag, 0, NULL);
            break;
        case NSDeleteFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_DEL, keyDownFlag, 0, NULL);
            break;
        case NSPageDownFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_PGDN, keyDownFlag, 0, NULL);
            break;
        case NSPageUpFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_PGUP, keyDownFlag, 0, NULL);
            break;
        case NSHomeFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_HOME, keyDownFlag, 0, NULL);
            break;
        case NSEndFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_END, keyDownFlag, 0, NULL);
            break;
        case NSPauseFunctionKey:
            Sys_QueEvent(currentTime, SE_KEY, K_PAUSE, keyDownFlag, 0, NULL);
            break;
        default:
            if ([event modifierFlags] & NSNumericPadKeyMask) {
                switch (character) {
                    case '0':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_INS, keyDownFlag, 0, NULL);
                        break;
                    case '1':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_END, keyDownFlag, 0, NULL);
                        break;
                    case '2':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_DOWNARROW, keyDownFlag, 0, NULL);
                        break;
                    case '3':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_PGDN, keyDownFlag, 0, NULL);
                        break;
                    case '4':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_LEFTARROW, keyDownFlag, 0, NULL);
                        break;
                    case '5':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_5, keyDownFlag, 0, NULL);
                        break;
                    case '6':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_RIGHTARROW, keyDownFlag, 0, NULL);
                        break;
                    case '7':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_HOME, keyDownFlag, 0, NULL);
                        break;
                    case '8':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_UPARROW, keyDownFlag, 0, NULL);
                        break;
                    case '9':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_PGUP, keyDownFlag, 0, NULL);
                        break;
                    case '.':
                    case ',':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_DEL, keyDownFlag, 0, NULL);
                        break;
                    case '+':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_PLUS, keyDownFlag, 0, NULL);
                        break;
                    case '-':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_MINUS, keyDownFlag, 0, NULL);
                        break;
                    case '*':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_STAR, keyDownFlag, 0, NULL);
                        break;
                    case '/':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_SLASH, keyDownFlag, 0, NULL);
                        break;
                    case '=':
                        Sys_QueEvent(currentTime, SE_KEY, K_KP_EQUALS, keyDownFlag, 0, NULL);
                        break;
                    default:
                        //NSLog(@"TODO: Implement character %d", (int)character);
                        break;
                 }       
            } else if (character >= 'a' && character <= 'z') {
                if (character == 'h') {
                    if (maybeHide())
                        return;
                }
                Sys_QueEvent(currentTime, SE_KEY, character, keyDownFlag, 0, NULL);
                if (keyDownFlag) {
                    Sys_QueEvent(currentTime, SE_CHAR, (char)character, 0, 0, NULL);
                }
            } else if (character >= 'A' && character <= 'Z') {
                Sys_QueEvent(currentTime, SE_KEY, 'a' + (character - 'A'), keyDownFlag, 0, NULL);
                if (keyDownFlag) {
                    Sys_QueEvent(currentTime, SE_CHAR, character, 0, 0, NULL);
                }
            } else if (character >= 32 && character < 127) {
                Sys_QueEvent(currentTime, SE_KEY, character, keyDownFlag, 0, NULL);
                if (keyDownFlag) {
                    Sys_QueEvent(currentTime, SE_CHAR, (char)character, 0, 0, NULL);
                }
            } else {
                //NSLog(@"TODO: Implement character %d", (int)character);
            }
            break;
    }
}

static inline void processKeyEvent(NSEvent *keyEvent, qboolean keyDownFlag, int currentTime)
{
    NSEventType eventType;
    NSString *characters;
    unsigned int characterIndex, characterCount;

    eventType = [keyEvent type];
    characters = [keyEvent charactersIgnoringModifiers];
    characterCount = [characters length];

    for (characterIndex = 0; characterIndex < characterCount; characterIndex++) {
        sendEventForCharacter(keyEvent, [characters characterAtIndex:characterIndex], keyDownFlag, currentTime);
    }
}

static inline void sendEventForMaskChangeInFlags(int quakeKey, unsigned int modifierMask, unsigned int newModifierFlags, int currentTime)
{
    BOOL oldHadModifier, newHasModifier;

    oldHadModifier = (currentModifierFlags & modifierMask) != 0;
    newHasModifier = (newModifierFlags & modifierMask) != 0;
    if (oldHadModifier != newHasModifier) {
        // NSLog(@"Key %d posted for modifier mask modifierMask", quakeKey);
        Sys_QueEvent(currentTime, SE_KEY, quakeKey, newHasModifier, 0, NULL);
    }
}

static inline void processFlagsChangedEvent(NSEvent *flagsChangedEvent, int currentTime)
{
    int newModifierFlags;

    newModifierFlags = [flagsChangedEvent modifierFlags];
    sendEventForMaskChangeInFlags(K_COMMAND, NSCommandKeyMask, newModifierFlags, currentTime);
    sendEventForMaskChangeInFlags(K_CAPSLOCK, NSAlphaShiftKeyMask, newModifierFlags, currentTime);
    sendEventForMaskChangeInFlags(K_ALT, NSAlternateKeyMask, newModifierFlags, currentTime);
    sendEventForMaskChangeInFlags(K_CTRL, NSControlKeyMask, newModifierFlags, currentTime);
    sendEventForMaskChangeInFlags(K_SHIFT, NSShiftKeyMask, newModifierFlags, currentTime);
    currentModifierFlags = newModifierFlags;
}

static inline void processSystemDefinedEvent(NSEvent *systemDefinedEvent, int currentTime)
{
    static int oldButtons = 0;
    int buttonsDelta;
    int buttons;
    int isDown;
    
    if ([systemDefinedEvent subtype] == 7) {

        if (!mouseactive)
            return;
        
    
	buttons = [systemDefinedEvent data2];
        buttonsDelta = oldButtons ^ buttons;
        
        //Com_Printf("uberbuttons: %08lx %08lx\n",buttonsDelta,buttons);


	if (buttonsDelta & 1) {
            isDown = buttons & 1;
            Sys_QueEvent(currentTime, SE_KEY, K_MOUSE1, isDown, 0, NULL);
            if (in_showevents->integer) {
                Com_Printf("MOUSE2: %s\n", isDown ? "down" : "up");
            }
	}

	if (buttonsDelta & 2) {
            isDown = buttons & 2;
            Sys_QueEvent(currentTime, SE_KEY, K_MOUSE2, isDown, 0, NULL);
            if (in_showevents->integer) {
                Com_Printf("MOUSE3: %s\n", isDown ? "down" : "up");
            }
	}

	if (buttonsDelta & 4) {
            isDown = buttons & 4;
            Sys_QueEvent(currentTime, SE_KEY, K_MOUSE3, isDown, 0, NULL);
            if (in_showevents->integer) {
                Com_Printf("MOUSE1: %s\n", isDown ? "down" : "up");
            }
	}

	if (buttonsDelta & 8) {
            isDown = buttons & 8;
            Sys_QueEvent(currentTime, SE_KEY, K_MOUSE4, isDown, 0, NULL);
            if (in_showevents->integer) {
                Com_Printf("MOUSE4: %s\n", isDown ? "down" : "up");
            }
        }
        
	if (buttonsDelta & 16) {
            isDown = buttons & 16;
            Sys_QueEvent(currentTime, SE_KEY, K_MOUSE5, isDown, 0, NULL);
            if (in_showevents->integer) {
                Com_Printf("MOUSE5: %s\n", isDown ? "down" : "up");
            }
	}
        
        oldButtons = buttons;
    }
}

static inline void processEvent(NSEvent *event, int currentTime)
{
    NSEventType eventType;

    if (!inputActive)
        return;

    eventType = [event type];

    if (in_showevents->integer)
        NSLog(@"event = %@", event);
    
    switch (eventType) {
        // These six event types are ignored since we do all of our mouse down/up process via the uber-mouse system defined event.  We have to accept these events however since they get enqueued and the queue will fill up if we don't.
        case NSLeftMouseDown:
            //Sys_QueEvent(currentTime, SE_KEY, K_MOUSE1, qtrue, 0, NULL);
            return;
        case NSLeftMouseUp:
            //Sys_QueEvent(currentTime, SE_KEY, K_MOUSE1, qfalse, 0, NULL);
            return;
        case NSRightMouseDown:
            //Sys_QueEvent(currentTime, SE_KEY, K_MOUSE2, qtrue, 0, NULL);
            return;
        case NSRightMouseUp:
            //Sys_QueEvent(currentTime, SE_KEY, K_MOUSE2, qfalse, 0, NULL);
            return;
        case 25: // other mouse down
            return;
        case 26: // other mouse up
            return;
            
        case NSMouseMoved:
        case NSLeftMouseDragged:
        case NSRightMouseDragged:
        case 27: // other mouse dragged
            Sys_ProcessMouseMovedEvent(event, currentTime);
            return;
        case NSKeyDown:
        case NSKeyUp:
            processKeyEvent(event, eventType == NSKeyDown, currentTime);
            return;
        case NSFlagsChanged:
            processFlagsChangedEvent(event, currentTime);
            return;
	case NSSystemDefined:
	    processSystemDefinedEvent(event, currentTime);
	    return;
        case NSScrollWheel:
            if ([event deltaY] < 0.0) {
                Sys_QueEvent(currentTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
                Sys_QueEvent(currentTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
            } else {
                Sys_QueEvent(currentTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
                Sys_QueEvent(currentTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
            }
            return;
        default:
            break;
    }
    [NSApp sendEvent:event];
}

static void Sys_SendKeyEvents(int currentTime)
{
#ifndef DEDICATED
    NSEvent *event;
    NSDate *timeout;
    extern float SNDDMA_GetBufferDuration();
    
    timeout = distantPast;
    if (Sys_IsHidden)
        timeout = [NSDate dateWithTimeIntervalSinceNow: 0.25 * SNDDMA_GetBufferDuration()];
    
    // This gets call regardless of whether inputActive is true or not.  This is important since we need to be poking the event queue in order for the unhide event to make its way through the system.  This means that when we hide, we can just shut down the input system and reeanbled it when we unhide.
    while ((event = [NSApp nextEventMatchingMask: NSAnyEventMask
                                       untilDate: timeout
                                          inMode: NSDefaultRunLoopMode
                                         dequeue:YES])) {
            if (Sys_IsHidden) {
                // Just let NSApp handle events so that we'll get the app activation event
                [NSApp sendEvent: event];
                timeout = [NSDate dateWithTimeIntervalSinceNow: 0.1];
            } else {
                static int lastEventTime = 0;
                static BOOL lastEventTimeValid = NO;

                // Mac OS X 10.0.3 has a bug where the if the monitor goes to sleep in fullscreen GL mode, the gamma won't be restored.  We'll restore the gamma if there is a pause while in the game of more than 10 seconds.  We don't do this on the 'Sys_IsHidden' branch since unhiding will restore the monitor gamma.
                if ((currentTime - lastEventTime > 1 * 1000) && lastEventTimeValid) {
                    //Com_Printf("Restoring monitor gamma after being idle for %f seconds.\n", (currentTime - lastEventTime) / 1000.0);
                    [NSCursor hide];
                    Sys_SetScreenFade(&glw_state.inGameTable, 1.0);
                }
                lastEventTime = [event timestamp] * 1000.0;	//currentTime;
                lastEventTimeValid = YES;
                
                processEvent(event, lastEventTime);
            }
    }
#endif
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

extern qboolean	Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );

#define	MAX_QUED_EVENTS		256
#define	MASK_QUED_EVENTS	( MAX_QUED_EVENTS - 1 )

static sysEvent_t	eventQue[MAX_QUED_EVENTS];
static int              eventHead, eventTail;
static byte		sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr ) {
	sysEvent_t	*ev;
        int	i,j;
#ifndef DEDICATED
    if (in_showevents->integer)
        NSLog(@"EVENT ENQUEUE:  time=%d type=%d value=0x%08x value2=0x%08x\n", time, type, value, value2);
#endif

	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf("Sys_QueEvent: overflow\n");
	}

	if ( !time ) {
		time = Sys_Milliseconds();
	}

	// insert it by time
	for ( i = eventTail ; i < eventHead ; i++ ) {
		ev = &eventQue[ i & MASK_QUED_EVENTS ];
		if ( ev->evTime > time ) {
			break;
		}
	}

	// insert before i
	for ( j = eventHead ; j > i ; j-- ) {
		eventQue[ j & MASK_QUED_EVENTS ] = eventQue[ (j-1) & MASK_QUED_EVENTS ];
	}
	ev = &eventQue[ i & MASK_QUED_EVENTS ];

	eventHead++;

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent

================
*/
sysEvent_t Sys_GetEvent( void )
{
    sysEvent_t	ev;
    char       *s;
    msg_t       netmsg;
    netadr_t	adr;
    int         currentTime;
    
    // return if we have data
    if (eventHead > eventTail) {
        eventTail++;
        return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
    }

    // The queue must be empty.  Check all of the event sources.  If the events are
    // already in the queue, we can't imply any real ordering, so we'll avoid extra
    // system calls and give them all the same time.
    currentTime = Sys_Milliseconds();

    // Check for mouse and keyboard events
    Sys_SendKeyEvents(currentTime);

    // check for console commands
    s = Sys_ConsoleInput();
    if ( s ) {
        char	*b;
        int		len;
    
        len = strlen( s ) + 1;
        b = Z_Malloc( len );
        strcpy( b, s );
        Sys_QueEvent( currentTime, SE_CONSOLE, 0, 0, len, b );
    }
    

    // During debugging it is sometimes usefull to be able to start/stop mouse input.
    // Don't turn on the input when we've disabled it because we're hidden, however.
    if (!com_dedicated->integer) {
        if (in_nomouse->integer == mouseactive && !Sys_IsHidden) {
            if (in_nomouse->integer)
                Sys_StopMouseInput();
            else
                Sys_StartMouseInput();
        }
    }
    
    // check for network packets
    MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
    if ( Sys_GetPacket ( &adr, &netmsg ) ) {
        netadr_t		*buf;
        int				len;
    
        // copy out to a seperate buffer for qeueing
        len = sizeof( netadr_t ) + netmsg.cursize;
        buf = Z_Malloc( len );
        *buf = adr;
        memcpy( buf+1, netmsg.data, netmsg.cursize );
        Sys_QueEvent( currentTime, SE_PACKET, 0, 0, len, buf );
    }

    // If we got an event, return it
    if (eventHead > eventTail) {
        eventTail++;
        return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
    }
    
    // Otherwise, return an empty event to indicate that there are no events pending.
    memset( &ev, 0, sizeof( ev ) );
    ev.evTime = currentTime;

    return ev;
}



