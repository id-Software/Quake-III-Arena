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

#import "Q3Controller.h"

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include "client.h"
#include "macosx_local.h"
//#include "GameRanger SDK/gameranger.h"
#ifdef OMNI_TIMER
#import "macosx_timers.h"
#endif

#define MAX_ARGC 1024

static qboolean Sys_IsProcessingTerminationRequest = qfalse;
static void Sys_CreatePathToFile(NSString *path, NSDictionary *attributes);

@interface Q3Controller (Private)
- (void)quakeMain;
@end

@implementation Q3Controller

#ifndef DEDICATED

- (void)applicationDidFinishLaunching:(NSNotification *)notification;
{
    NS_DURING {
        [self quakeMain];
    } NS_HANDLER {
        Sys_Error("%@", [localException reason]);
    } NS_ENDHANDLER;
    Sys_Quit();
}

- (void)applicationDidUnhide:(NSNotification *)notification;
{
    // Don't reactivate the game if we are asking whether to quit
    if (Sys_IsProcessingTerminationRequest)
        return;
        
    if (!Sys_Unhide())
        // Didn't work -- hide again so we should get another chance to unhide later
        [NSApp hide: nil];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;
{
    int choice;

    if (!Sys_IsHidden) {
        // We're terminating via -terminate:
        return NSTerminateNow;
    }
    
    // Avoid reactivating GL when we unhide due to this panel
    Sys_IsProcessingTerminationRequest = qtrue;
    choice = NSRunAlertPanel(nil, @"Quit without saving?", @"Don't Quit", @"Quit", nil);
    Sys_IsProcessingTerminationRequest = qfalse;

    if (choice == NSAlertAlternateReturn)
        return NSTerminateNow;
        
    // Make sure we get re-hidden
    [NSApp hide:nil];
    
    return NSTerminateCancel;
}

// Actions

- (IBAction)paste:(id)sender;
{
    int shiftWasDown, insertWasDown;
    unsigned int currentTime;

    currentTime = Sys_Milliseconds();
    // Save the original keyboard state
    shiftWasDown = keys[K_SHIFT].down;
    insertWasDown = keys[K_INS].down;
    // Fake a Shift-Insert keyboard event
    keys[K_SHIFT].down = qtrue;
    Sys_QueEvent(currentTime, SE_KEY, K_INS, qtrue, 0, NULL);
    Sys_QueEvent(currentTime, SE_KEY, K_INS, qfalse, 0, NULL);
    // Restore the original keyboard state
    keys[K_SHIFT].down = shiftWasDown;
    keys[K_INS].down = insertWasDown;
}

extern void CL_Quit_f(void);


- (IBAction)requestTerminate:(id)sender;
{
    Com_Quit_f();
    // UI_QuitMenu();
}

- (void)showBanner;
{
    static BOOL hasShownBanner = NO;

    if (!hasShownBanner) {
        cvar_t *showBanner;

        hasShownBanner = YES;
        showBanner = Cvar_Get("cl_showBanner", "1", 0);
        if (showBanner->integer != 0) {
            NSPanel *splashPanel;
            NSImage *bannerImage;
            NSRect bannerRect;
            NSImageView *bannerImageView;
            
            bannerImage = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForImageResource:@"banner.jpg"]];
            bannerRect = NSMakeRect(0.0, 0.0, [bannerImage size].width, [bannerImage size].height);
            
            splashPanel = [[NSPanel alloc] initWithContentRect:bannerRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
            
            bannerImageView = [[NSImageView alloc] initWithFrame:bannerRect];
            [bannerImageView setImage:bannerImage];
            [splashPanel setContentView:bannerImageView];
            [bannerImageView release];
            
            [splashPanel center];
            [splashPanel setHasShadow:YES];
            [splashPanel orderFront: nil];
            [NSThread sleepUntilDate:[NSDate dateWithTimeIntervalSinceNow:2.5]];
            [splashPanel close];
            
            [bannerImage release];
        }
    }
}

// Services

- (void)connectToServer:(NSPasteboard *)pasteboard userData:(NSString *)data error:(NSString **)error;
{
    NSArray *pasteboardTypes;

    pasteboardTypes = [pasteboard types];
    if ([pasteboardTypes containsObject:NSStringPboardType]) {
        NSString *requestedServer;

        requestedServer = [pasteboard stringForType:NSStringPboardType];
        if (requestedServer) {
            Cbuf_AddText(va("connect %s\n", [requestedServer cString]));
            return;
        }
    }
    *error = @"Unable to connect to server:  could not find string on pasteboard";
}

- (void)performCommand:(NSPasteboard *)pasteboard userData:(NSString *)data error:(NSString **)error;
{
    NSArray *pasteboardTypes;

    pasteboardTypes = [pasteboard types];
    if ([pasteboardTypes containsObject:NSStringPboardType]) {
        NSString *requestedCommand;

        requestedCommand = [pasteboard stringForType:NSStringPboardType];
        if (requestedCommand) {
            Cbuf_AddText(va("%s\n", [requestedCommand cString]));
            return;
        }
    }
    *error = @"Unable to perform command:  could not find string on pasteboard";
}

#endif

- (void)quakeMain;
{
    NSAutoreleasePool *pool;
    int argc = 0;
    const char *argv[MAX_ARGC];
    NSProcessInfo *processInfo;
    NSArray *arguments;
    unsigned int argumentIndex, argumentCount;
    NSFileManager *defaultManager;
    unsigned int commandLineLength;
    NSString *installationPathKey, *installationPath;
    char *cmdline;
    BOOL foundDirectory;
    NSString *appName, *demoAppName, *selectButton;
    int count = 0;
    pool = [[NSAutoreleasePool alloc] init];

    [NSApp setServicesProvider:self];

    processInfo = [NSProcessInfo processInfo];
    arguments = [processInfo arguments];
    argumentCount = [arguments count];
    for (argumentIndex = 0; argumentIndex < argumentCount; argumentIndex++) {
        NSString *arg;
        
        arg = [arguments objectAtIndex:argumentIndex];
        // Don't pass the Process Serial Number command line arg that the Window Server/Finder invokes us with
        if ([arg hasPrefix: @"-psn_"])
            continue;
            
        argv[argc++] = strdup([arg cString]);
    }
    
    // Figure out where the level data is stored.
    installationPathKey = @"RetailInstallationPath";

    installationPath = [[NSUserDefaults standardUserDefaults] objectForKey:installationPathKey];
    if (!installationPath) {
        // Default to the directory containing the executable (which is where most users will want to put it
        installationPath = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
    }

#if !defined(DEDICATED)    
    appName = [[[NSBundle mainBundle] infoDictionary] objectForKey: @"CFBundleName"];
#else
	// We are hard coding the app name here since the dedicated server is a tool, not a app bundle and does not have access to the Info.plist that the client app does.  Suck.
    appName = @"Quake3";
#endif
    demoAppName = appName;

    while (YES) {
        NSString *dataPath;
        NSOpenPanel *openPanel;
        int result;
        
        foundDirectory = NO;
        defaultManager = [NSFileManager defaultManager];
        //NSLog(@"Candidate installation path = %@", installationPath);
        dataPath = [installationPath stringByAppendingPathComponent: @"baseq3"];
        
        if ([defaultManager fileExistsAtPath: dataPath]) {
            // Check that the data directory contains at least one .pk3 file.  We don't know what it will be named, so don't hard code a name (for example it might be named 'french.pk3' for a French release
            NSArray *files;
            unsigned int fileIndex;
            
            files = [defaultManager directoryContentsAtPath: dataPath];
            fileIndex = [files count];
            while (fileIndex--) {
                if ([[files objectAtIndex: fileIndex] hasSuffix: @"pk3"]) {
                    //NSLog(@"Found %@.", [files objectAtIndex: fileIndex]);
                    foundDirectory = YES;
                    break;
                }
            }
        }
        
        if (foundDirectory)
            break;

#ifdef DEDICATED
            break;
#warning TJW: We are hard coding the app name and default domain here since the dedicated server is a tool, not a app bundle and does not have access to the Info.plist that the client app does.  Suck.
        NSLog(@"Unable to determine installation directory.  Please move the executable into the '%@' installation directory or add a '%@' key in the 'Q3DedicatedServer' defaults domain.", appName, installationPathKey, [[NSBundle mainBundle] bundleIdentifier]);
        Sys_Quit();
        exit(1);
#else
        selectButton = @"Select Retail Installation...";

        result = NSRunAlertPanel(demoAppName, @"You need to select the installation directory for %@ (not any directory inside of it -- the installation directory itself).", selectButton, @"Quit", nil, appName);
        switch (result) {
            case NSAlertDefaultReturn:
                break;
            default:
                Sys_Quit();
                break;
        }
        
        openPanel = [NSOpenPanel openPanel];
        [openPanel setAllowsMultipleSelection:NO];
        [openPanel setCanChooseDirectories:YES];
        [openPanel setCanChooseFiles:NO];
        result = [openPanel runModalForDirectory:nil file:nil];
        if (result == NSOKButton) {
            NSArray *filenames;

            filenames = [openPanel filenames];
            if ([filenames count] == 1) {
                installationPath = [filenames objectAtIndex:0];
                [[NSUserDefaults standardUserDefaults] setObject:installationPath forKey:installationPathKey];
                [[NSUserDefaults standardUserDefaults] synchronize];
            }
        }
#endif
    }
    
    // Create the application support directory if it doesn't exist already
    do {
        NSArray *results;
        NSString *libraryPath, *homePath, *filePath;
        NSDictionary *attributes;
        
        results = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
        if (![results count])
            break;
        
        libraryPath = [results objectAtIndex: 0];
        homePath = [libraryPath stringByAppendingPathComponent: @"Application Support"];
        homePath = [homePath stringByAppendingPathComponent: appName];
        filePath = [homePath stringByAppendingPathComponent: @"foo"];
        
        attributes = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithUnsignedInt: 0750], NSFilePosixPermissions, nil];
        NS_DURING {
            Sys_CreatePathToFile(filePath, attributes);
            Sys_SetDefaultHomePath([homePath fileSystemRepresentation]);
        } NS_HANDLER {
            NSLog(@"Exception: %@", localException);
#ifndef DEDICATED
            NSRunAlertPanel(nil, @"Unable to create '%@'.  Please make sure that you have permission to write to this folder and re-run the game.", @"OK", nil, nil, homePath);
#endif
            Sys_Quit();
        } NS_ENDHANDLER;
    } while(0);
    
    // Provoke the CD scanning code into looking up the CD.
    Sys_CheckCD();
    
    // Let the filesystem know where our local install is
    Sys_SetDefaultInstallPath([installationPath cString]);

    cmdline = NULL;
#if 0
    if (GRCheckFileForCmd()) {
	GRGetWaitingCmd();
	if (GRHasProperty( 'Exec' )) {
            NSString *cfgPath, *grCfg;
            cfgPath = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
            cfgPath = [cfgPath stringByAppendingPathComponent: [NSString stringWithCString: GRGetPropertyStr( 'Exec' )]];
            grCfg = [NSString stringWithContentsOfFile: cfgPath];
            cmdline = malloc(strlen([grCfg cString])+1);
            [grCfg getCString: cmdline];
	}
    }
#endif
    if (!cmdline) {
        // merge the command line, this is kinda silly	
        for (commandLineLength = 1, argumentIndex = 1; argumentIndex < argc; argumentIndex++)
            commandLineLength += strlen(argv[argumentIndex]) + 1;
        cmdline = malloc(commandLineLength);
        *cmdline = '\0';
        for (argumentIndex = 1; argumentIndex < argc; argumentIndex++) {
            if (argumentIndex > 1)
                strcat(cmdline, " ");
            strcat(cmdline, argv[argumentIndex]);
        }
    }
    Com_Printf("command line: %s\n", cmdline);
    
    Com_Init(cmdline);

#ifndef DEDICATED
    [NSApp activateIgnoringOtherApps:YES];
#endif

    while (1) {
        Com_Frame();

        if ((count & 15)==0) {
            // We should think about doing this less frequently than every frame
            [pool release];
            pool = [[NSAutoreleasePool alloc] init];
        }
    }

    [pool release];
}

@end



// Creates any directories needed to be able to create a file at the specified path.  Raises an exception on failure.
static void Sys_CreatePathToFile(NSString *path, NSDictionary *attributes)
{
    NSArray *pathComponents;
    unsigned int dirIndex, dirCount;
    unsigned int startingIndex;
    NSFileManager *manager;
    
    manager = [NSFileManager defaultManager];
    pathComponents = [path pathComponents];
    dirCount = [pathComponents count] - 1;

    startingIndex = 0;
    for (dirIndex = startingIndex; dirIndex < dirCount; dirIndex++) {
        NSString *partialPath;
        BOOL fileExists;

        partialPath = [NSString pathWithComponents:[pathComponents subarrayWithRange:NSMakeRange(0, dirIndex + 1)]];
        
        // Don't use the 'fileExistsAtPath:isDirectory:' version since it doesn't traverse symlinks
        fileExists = [manager fileExistsAtPath:partialPath];
        if (!fileExists) {
            if (![manager createDirectoryAtPath:partialPath attributes:attributes]) {
                [NSException raise:NSGenericException format:@"Unable to create a directory at path: %@", partialPath];
            }
        } else {
            NSDictionary *attributes;

            attributes = [manager fileAttributesAtPath:partialPath traverseLink:YES];
            if (![[attributes objectForKey:NSFileType] isEqualToString: NSFileTypeDirectory]) {
                [NSException raise:NSGenericException format:@"Unable to write to path \"%@\" because \"%@\" is not a directory",
                    path, partialPath];
            }
        }
    }
}

#ifdef DEDICATED
void S_ClearSoundBuffer( void ) {
}
#endif
