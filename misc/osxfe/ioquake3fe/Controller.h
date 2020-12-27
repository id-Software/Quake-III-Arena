//
//  Controller.h
//  ioquake3fe
//
//  Created by Ben Wilber on 3/11/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface Controller : NSObject {
	IBOutlet id		argsTextField;
	NSTask			*quakeTask;
	NSFileHandle	*quakeOut;
	NSMutableData	*quakeData;
}

- (IBAction)launch:(id)sender;
- (void)readPipe:(NSNotification *)note;
- (void)taskNote:(NSNotification *)note;

@end
