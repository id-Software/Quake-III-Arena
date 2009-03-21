//
//  Controller.m
//  ioquake3fe
//
//  Created by Ben Wilber on 3/11/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "Controller.h"
#import "ErrorWindow.h"

#define IOQ3_BIN @"ioquake3.ub"

@implementation Controller

- (id)init
{
	[super init];
	quakeData = [[NSMutableData alloc] initWithCapacity:1.0];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(readPipe:) name:NSFileHandleReadCompletionNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(taskNote:) name:NSTaskDidTerminateNotification object:nil];
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[super dealloc];
}

- (IBAction)launch:(id)sender
{
	NSPipe *pipe = [NSPipe pipe];
	quakeOut = [pipe fileHandleForReading];
	[quakeOut readInBackgroundAndNotify];
	
	quakeTask = [NSTask new];
	[quakeTask setStandardOutput:pipe];
	[quakeTask setStandardError:pipe];

	NSString *args = [argsTextField stringValue];
	if ([args length])
		[quakeTask setArguments:[args componentsSeparatedByString:@" "]];
		// tiger sucks
		//[quakeTask setArguments:[args componentsSeparatedByCharactersInSet:[NSCharacterSet whitespaceCharacterSet]]];
			
	BOOL die = NO;
	
	@try {
		[quakeTask setLaunchPath:[[NSBundle mainBundle] pathForAuxiliaryExecutable:IOQ3_BIN]];
		[quakeTask launch];
	}
	@catch (NSException *e) {
		[[NSAlert
		  alertWithMessageText:NSLocalizedString(@"Launch Failed", @"launch failed")
		  defaultButton:NSLocalizedString(@"OK", @"OK")
		  alternateButton:nil
		  otherButton:nil
		  informativeTextWithFormat:NSLocalizedString(@"Something is probably wrong with the actual ioquake3 binary.", @"launch failed text")]
		 runModal];
		die = YES;
	}
	@finally {
		if (die)
			[NSApp terminate:self];
	}

	[[sender window] close];
	return;
}

- (void)readPipe:(NSNotification *)note
{
	if ([note object] == quakeOut) {
		NSData *outputData = [[note userInfo] objectForKey:NSFileHandleNotificationDataItem];
		if ([outputData length])
			[quakeData appendData:outputData];
		if (quakeTask)
			[quakeOut readInBackgroundAndNotify];
	}
}

- (void)taskNote:(NSNotification *)note
{
	if ([note object] == quakeTask) {
		if ([quakeTask isRunning] == NO) {
			if ([quakeTask terminationStatus] != 0) {
				ErrorWindow *ew = [[[ErrorWindow alloc] init] autorelease];
				[ew bitch:[[[NSString alloc] initWithData:quakeData encoding:NSUTF8StringEncoding] autorelease]];
			} else {
				[NSApp terminate:self];
			}
		}
	}
}

@end
