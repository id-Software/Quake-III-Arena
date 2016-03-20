#import "ErrorWindow.h"

@implementation ErrorWindow

- (void)bitch:(NSString *)errorlog {
	NSLog(errorlog);

	NSNib *nib = [[NSNib alloc] initWithNibNamed:@"ErrorWindow.nib" bundle:[NSBundle mainBundle]];
	[nib instantiateNibWithOwner:self topLevelObjects:nil];
	
	[errorWindow makeKeyWindow];
	[errorTextField setFont:[NSFont userFixedPitchFontOfSize:12.0]];
	[errorTextField setString:@""];
	[[errorTextField textStorage] appendAttributedString:[[[NSAttributedString alloc] initWithString:errorlog] autorelease]];
    [errorTextField scrollRangeToVisible:NSMakeRange([[errorTextField string] length], 0)];
}

@end
