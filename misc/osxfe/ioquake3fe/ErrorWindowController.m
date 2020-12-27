//
//  ErrorWindowController.m
//  ioquake3fe
//
//  Created by Ben Wilber on 3/11/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "ErrorWindowController.h"

@implementation ErrorWindowController

// yes, a whole class just so the fucking app will quit

- (BOOL)windowShouldClose:(id)sender {
	[NSApp terminate:self];
	return YES;
}

@end
