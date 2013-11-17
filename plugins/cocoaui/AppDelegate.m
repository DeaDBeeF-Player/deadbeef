#import "AppDelegate.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Insert code here to initialize your application
}
- (IBAction)playClicked:(id)sender {
    deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
}
- (IBAction)pauseClicked:(id)sender {
    deadbeef->sendmessage(DB_EV_TOGGLE_PAUSE, 0, 0, 0);
}
- (IBAction)prevClicked:(id)sender {
    deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
}
- (IBAction)nextClicked:(id)sender {
    deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
}
- (IBAction)stopClicked:(id)sender {
    deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
}

@end
