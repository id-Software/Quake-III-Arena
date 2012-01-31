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

#import "macosx_display.h"

#include "tr_local.h"
#import "macosx_local.h"

#import <Foundation/Foundation.h>
#import <IOKit/graphics/IOGraphicsTypes.h>  // for interpreting the kCGDisplayIOFlags element of the display mode


NSDictionary *Sys_GetMatchingDisplayMode(qboolean allowStretchedModes)
{
    NSArray *displayModes;
    NSDictionary *mode;
    unsigned int modeIndex, modeCount, bestModeIndex;
    int verbose;
    cvar_t *cMinFreq, *cMaxFreq;
    int minFreq, maxFreq;
    unsigned int colorDepth;
    
    verbose = r_verbose->integer;

    colorDepth = r_colorbits->integer;
    if (colorDepth < 16 || !r_fullscreen->integer)
        colorDepth = [[glw_state.desktopMode objectForKey: (id)kCGDisplayBitsPerPixel] intValue];

    cMinFreq = ri.Cvar_Get("r_minDisplayRefresh", "0", CVAR_ARCHIVE);
    cMaxFreq = ri.Cvar_Get("r_maxDisplayRefresh", "0", CVAR_ARCHIVE);

    if (cMinFreq && cMaxFreq && cMinFreq->integer && cMaxFreq->integer &&
        cMinFreq->integer > cMaxFreq->integer) {
        ri.Error(ERR_FATAL, "r_minDisplayRefresh must be less than or equal to r_maxDisplayRefresh");
    }

    minFreq = cMinFreq ? cMinFreq->integer : 0;
    maxFreq = cMaxFreq ? cMaxFreq->integer : 0;
    
    displayModes = (NSArray *)CGDisplayAvailableModes(glw_state.display);
    if (!displayModes) {
        ri.Error(ERR_FATAL, "CGDisplayAvailableModes returned NULL -- 0x%0x is an invalid display", glw_state.display);
    }
    
    modeCount = [displayModes count];
    if (verbose) {
        ri.Printf(PRINT_ALL, "%d modes avaliable\n", modeCount);
        ri.Printf(PRINT_ALL, "Current mode is %s\n", [[(id)CGDisplayCurrentMode(glw_state.display) description] cString]);
    }
    
    // Default to the current desktop mode
    bestModeIndex = 0xFFFFFFFF;
    
    for ( modeIndex = 0; modeIndex < modeCount; ++modeIndex ) {
        id object;
        int refresh;
        
        mode = [displayModes objectAtIndex: modeIndex];
        if (verbose) {
            ri.Printf(PRINT_ALL, " mode %d -- %s\n", modeIndex, [[mode description] cString]);
        }

        // Make sure we get the right size
        object = [mode objectForKey: (id)kCGDisplayWidth];

        if ([[mode objectForKey: (id)kCGDisplayWidth] intValue] != glConfig.vidWidth ||
            [[mode objectForKey: (id)kCGDisplayHeight] intValue] != glConfig.vidHeight) {
            if (verbose)
                ri.Printf(PRINT_ALL, " -- bad size\n");
            continue;
        }

        if (!allowStretchedModes) {
            if ([[mode objectForKey: (id)kCGDisplayIOFlags] intValue] & kDisplayModeStretchedFlag) {
                if (verbose)
                    ri.Printf(PRINT_ALL, " -- stretched modes disallowed\n");
                continue;
            }
        }

        // Make sure that our frequency restrictions are observed
        refresh = [[mode objectForKey: (id)kCGDisplayRefreshRate] intValue];
        if (minFreq &&  refresh < minFreq) {
            if (verbose)
                ri.Printf(PRINT_ALL, " -- refresh too low\n");
            continue;
        }

        if (maxFreq && refresh > maxFreq) {
            if (verbose)
                ri.Printf(PRINT_ALL, " -- refresh too high\n");
            continue;
        }

        if ([[mode objectForKey: (id)kCGDisplayBitsPerPixel] intValue] != colorDepth) {
            if (verbose)
                ri.Printf(PRINT_ALL, " -- bad depth\n");
            continue;
        }

        bestModeIndex = modeIndex;
        if (verbose)
            ri.Printf(PRINT_ALL, " -- OK\n", bestModeIndex);
    }

    if (verbose)
        ri.Printf(PRINT_ALL, " bestModeIndex = %d\n", bestModeIndex);

    if (bestModeIndex == 0xFFFFFFFF) {
        ri.Printf(PRINT_ALL, "No suitable display mode available.\n");
        return nil;
    }
    
    return [displayModes objectAtIndex: bestModeIndex];
}


#define MAX_DISPLAYS 128

void Sys_GetGammaTable(glwgamma_t *table)
{
    CGTableCount tableSize = 512;
    CGDisplayErr err;
    
    table->tableSize = tableSize;
    if (table->red)
        free(table->red);
    table->red = malloc(tableSize * sizeof(*table->red));
    if (table->green)
        free(table->green);
    table->green = malloc(tableSize * sizeof(*table->green));
    if (table->blue)
        free(table->blue);
    table->blue = malloc(tableSize * sizeof(*table->blue));
    
    // TJW: We _could_ loop here if we get back the same size as our table, increasing the table size.
    err = CGGetDisplayTransferByTable(table->display, tableSize, table->red, table->green, table->blue,
&table->tableSize);
    if (err != CGDisplayNoErr) {
        Com_Printf("GLimp_Init: CGGetDisplayTransferByTable returned %d.\n", err);
        table->tableSize = 0;
    }
}

void Sys_SetGammaTable(glwgamma_t *table)
{
}


void Sys_StoreGammaTables()
{
    // Store the original gamma for all monitors so that we can fade and unfade them all
    CGDirectDisplayID displays[MAX_DISPLAYS];
    CGDisplayCount displayIndex;
    CGDisplayErr err;

    err = CGGetActiveDisplayList(MAX_DISPLAYS, displays, &glw_state.displayCount);
    if (err != CGDisplayNoErr)
        Sys_Error("Cannot get display list -- CGGetActiveDisplayList returned %d.\n", err);
    
    glw_state.originalDisplayGammaTables = calloc(glw_state.displayCount, sizeof(*glw_state.originalDisplayGammaTables));
    for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {
        glwgamma_t *table;

        table = &glw_state.originalDisplayGammaTables[displayIndex];
        table->display = displays[displayIndex];
        Sys_GetGammaTable(table);
    }
}


//  This isn't a mathematically correct fade, but we don't care that much.
void Sys_SetScreenFade(glwgamma_t *table, float fraction)
{
    CGTableCount tableSize;
    CGGammaValue *red, *blue, *green;
    CGTableCount gammaIndex;
    
    if (!glConfig.deviceSupportsGamma)
        return;

    if (!(tableSize = table->tableSize))
        // we couldn't get the table for this display for some reason
        return;
    
//    Com_Printf("0x%08x %f\n", table->display, fraction);
    
    red = glw_state.tempTable.red;
    green = glw_state.tempTable.green;
    blue = glw_state.tempTable.blue;
    if (glw_state.tempTable.tableSize < tableSize) {
        glw_state.tempTable.tableSize = tableSize;
        red = realloc(red, sizeof(*red) * tableSize);
        green = realloc(green, sizeof(*green) * tableSize);
        blue = realloc(blue, sizeof(*blue) * tableSize);
        glw_state.tempTable.red = red;
        glw_state.tempTable.green = green;
        glw_state.tempTable.blue = blue;
    }

    for (gammaIndex = 0; gammaIndex < table->tableSize; gammaIndex++) {
        red[gammaIndex] = table->red[gammaIndex] * fraction;
        blue[gammaIndex] = table->blue[gammaIndex] * fraction;
        green[gammaIndex] = table->green[gammaIndex] * fraction;
    }
    
    CGSetDisplayTransferByTable(table->display, table->tableSize, red, green, blue);
}

// Fades all the active displays at the same time.

#define FADE_DURATION 0.5
void Sys_FadeScreens()
{
    CGDisplayCount displayIndex;
    int stepIndex;
    glwgamma_t *table;
    NSTimeInterval start, current;
    float time;
    
    if (!glConfig.deviceSupportsGamma)
        return;

    Com_Printf("Fading all displays\n");
    
    start = [NSDate timeIntervalSinceReferenceDate];
    time = 0.0;
    while (time != FADE_DURATION) {
        current = [NSDate timeIntervalSinceReferenceDate];
        time = current - start;
        if (time > FADE_DURATION)
            time = FADE_DURATION;
            
        for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {            
            table = &glw_state.originalDisplayGammaTables[displayIndex];
            Sys_SetScreenFade(table, 1.0 - time / FADE_DURATION);
        }
    }
}

void Sys_FadeScreen(CGDirectDisplayID display)
{
    CGDisplayCount displayIndex;
    glwgamma_t *table;
    int stepIndex;
    
    if (!glConfig.deviceSupportsGamma)
        return;

    Com_Printf("Fading display 0x%08x\n", display);

    for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {
        if (display == glw_state.originalDisplayGammaTables[displayIndex].display) {
            NSTimeInterval start, current;
            float time;
            
            start = [NSDate timeIntervalSinceReferenceDate];
            time = 0.0;

            table = &glw_state.originalDisplayGammaTables[displayIndex];
            while (time != FADE_DURATION) {
                current = [NSDate timeIntervalSinceReferenceDate];
                time = current - start;
                if (time > FADE_DURATION)
                    time = FADE_DURATION;

                Sys_SetScreenFade(table, 1.0 - time / FADE_DURATION);
            }
            return;
        }
    }

    Com_Printf("Unable to find display to fade it\n");
}

void Sys_UnfadeScreens()
{
    CGDisplayCount displayIndex;
    int stepIndex;
    glwgamma_t *table;
    NSTimeInterval start, current;
    float time;
    
    if (!glConfig.deviceSupportsGamma)
        return;
        
    Com_Printf("Unfading all displays\n");

    start = [NSDate timeIntervalSinceReferenceDate];
    time = 0.0;
    while (time != FADE_DURATION) {
        current = [NSDate timeIntervalSinceReferenceDate];
        time = current - start;
        if (time > FADE_DURATION)
            time = FADE_DURATION;
            
        for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {            
            table = &glw_state.originalDisplayGammaTables[displayIndex];
            Sys_SetScreenFade(table, time / FADE_DURATION);
        }
    }
}

void Sys_UnfadeScreen(CGDirectDisplayID display, glwgamma_t *table)
{
    CGDisplayCount displayIndex;
    int stepIndex;
    
    if (!glConfig.deviceSupportsGamma)
        return;
    
    Com_Printf("Unfading display 0x%08x\n", display);

    if (table) {
        CGTableCount i;
        
        Com_Printf("Given table:\n");
        for (i = 0; i < table->tableSize; i++) {
            Com_Printf("  %f %f %f\n", table->red[i], table->blue[i], table->green[i]);
        }
    }
    
    // Search for the original gamma table for the display
    if (!table) {
        for (displayIndex = 0; displayIndex < glw_state.displayCount; displayIndex++) {
            if (display == glw_state.originalDisplayGammaTables[displayIndex].display) {
                table = &glw_state.originalDisplayGammaTables[displayIndex];
                break;
            }
        }
    }
    
    if (table) {
        NSTimeInterval start, current;
        float time;
        
        start = [NSDate timeIntervalSinceReferenceDate];
        time = 0.0;

        while (time != FADE_DURATION) {
            current = [NSDate timeIntervalSinceReferenceDate];
            time = current - start;
            if (time > FADE_DURATION)
                time = FADE_DURATION;
            Sys_SetScreenFade(table, time / FADE_DURATION);
        }
        return;
    }
    
    Com_Printf("Unable to find display to unfade it\n");
}



