#import "AppDelegate.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation AppDelegate

@synthesize playlist;

DB_playItem_t *prev = NULL;
int prevIdx = -1;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
//    [playlist setDelegate:(id<NSTableViewDelegate>)self];
    [playlist setDataSource:(id<NSTableViewDataSource>)self];
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

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    int cnt = deadbeef->pl_getcount(PL_MAIN);
    return cnt;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
    if ([[aTableColumn identifier] isEqualToString:@"playing"]) {
        return NULL;
    }

    DB_playItem_t *it = NULL;
    
    if (prevIdx != -1) {
        if (prevIdx == rowIndex) {
            it = prev;
            deadbeef->pl_item_ref (it);
        }
        else if (prevIdx == rowIndex - 1) {
            it = deadbeef->pl_get_prev (prev, PL_MAIN);
        }
        else if (prevIdx == rowIndex + 1) {
            it = deadbeef->pl_get_next (prev, PL_MAIN);
        }
    
    }
    if (!it) {
        it = deadbeef->pl_get_for_idx (rowIndex);
    }
    
    if (prev) {
        deadbeef->pl_item_unref (prev);
    }
    prev = it;
    prevIdx = rowIndex;

    if ([[aTableColumn identifier] isEqualToString:@"albumartist"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%a - %b");
        return [NSString stringWithUTF8String:buf];
    }
    if ([[aTableColumn identifier] isEqualToString:@"trknum"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%n");
        return [NSString stringWithUTF8String:buf];
    }
    if ([[aTableColumn identifier] isEqualToString:@"title"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%t");
        return [NSString stringWithUTF8String:buf];
    }
    if ([[aTableColumn identifier] isEqualToString:@"duration"]) {
        char buf[1024];
        deadbeef->pl_format_title (it, rowIndex, buf, sizeof (buf), -1, "%l");
        return [NSString stringWithUTF8String:buf];
    }
    
    return @"Hello";
}

- (IBAction)openFilesAction:(id)sender {
    NSOpenPanel* openDlg = [NSOpenPanel openPanel];
    
    // Enable the selection of files in the dialog.
    [openDlg setCanChooseFiles:YES];
    [openDlg setAllowsMultipleSelection:YES];
    
    // Enable the selection of directories in the dialog.
    [openDlg setCanChooseDirectories:NO];
    
    // Display the dialog.  If the OK button was pressed,
    // process the files.
    if ( [openDlg runModalForDirectory:nil file:nil] == NSOKButton )
    {
        // Get an array containing the full filenames of all
        // files and directories selected.
        NSArray* files = [openDlg filenames];
        
        // Loop through all the files and process them.
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        deadbeef->plt_clear(plt);
        if (plt) {
            if (!deadbeef->plt_add_files_begin (plt, 0)) {
                for( int i = 0; i < [files count]; i++ )
                {
                    NSString* fileName = [files objectAtIndex:i];
                    deadbeef->plt_add_file2 (0, plt, [fileName UTF8String], NULL, NULL);
                }
                deadbeef->plt_add_files_end (plt, 0);
                deadbeef->plt_unref (plt);
            }
        }
    }
    [playlist reloadData];
    deadbeef->pl_save_current();
    deadbeef->conf_save();
}

- (IBAction)addFilesAction:(id)sender {
}

- (IBAction)addFoldersAction:(id)sender {
}

- (IBAction)clearAction:(id)sender {
}
@end
