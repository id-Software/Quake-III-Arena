#import <Cocoa/Cocoa.h>

@interface ErrorWindow : NSObject {
	IBOutlet id		errorWindow;
    IBOutlet id		errorTextField;
}

- (void)bitch:(NSString *)errorlog;

@end
