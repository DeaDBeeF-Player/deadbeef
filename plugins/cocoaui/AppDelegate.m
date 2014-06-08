#import "AppDelegate.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation AppDelegate

@synthesize arrayController;
@synthesize playlists;


- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Insert code here to initialize your application
    printf ("finished launching\n");
    NSMutableDictionary *value = [[NSMutableDictionary alloc] init];
    // Add some values to the dictionary
    // which match up to the NSTableView bindings
    [value setObject:[NSNumber numberWithInt:0] forKey:@"id"];
    [value setObject:[NSString stringWithFormat:@"test"] forKey:@"name"];
    
    [arrayController addObject:value];
    
    [playlists reloadData];
}

- (IBAction)tbClicked:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];
    
    switch (selectedSegment) {
        case 0:
            deadbeef->sendmessage(DB_EV_STOP, 0, 0, 0);
            break;
        case 1:
            deadbeef->sendmessage(DB_EV_PLAY_CURRENT, 0, 0, 0);
            break;
        case 2:
            deadbeef->sendmessage(DB_EV_TOGGLE_PAUSE, 0, 0, 0);
            break;
        case 3:
            deadbeef->sendmessage(DB_EV_PREV, 0, 0, 0);
            break;
        case 4:
            deadbeef->sendmessage(DB_EV_NEXT, 0, 0, 0);
            break;
    }
}
/*
- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return 1;
}

- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(int)rowIndex
{
//    NSString* myStr = @"Some value";
//   id objectAtRow = myStr;
//    NSString *columnKey = [aTableColumn identifier];
//    return  [objectAtRow valueForKey:columnKey];
}
*/
@end
