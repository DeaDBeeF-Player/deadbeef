/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#import "DdbSearchViewController.h"
#import "DdbSearchWidget.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbSearchViewController

#define DEFAULT_COLUMNS "[{\"title\":\"Artist - Album\", \"format\":\"%artist%[ - %album%]\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track number%\", \"size\":\"50\"}, {\"title\":\"Track Title\", \"format\":\"%title%\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

- (NSString *)getColumnConfig {
    return [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.search_columns", DEFAULT_COLUMNS)];
}

- (void)writeColumnConfig:(NSString *)config {
    deadbeef->conf_set_str ("cocoaui.search_columns", [config UTF8String]);
}

- (int)playlistIter {
    return PL_SEARCH;
}

- (void)awakeFromNib {
    DdbSearchWidget *view = (DdbSearchWidget *)[self view];
    [view setDelegate:(id<DdbListviewDelegate>)self];
    [self initContent];
}

- (const char *)groupByConfStr {
    return "cocoaui.search.group_by";
}

- (const char *)pinGroupsConfStr {
    return "cocoaui.search.pin_groups";
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    NSString *val = [textField stringValue];
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_search_process (plt, [val UTF8String]);
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION | DDB_PLAYLIST_CHANGE_SEARCHRESULT, 0);
        deadbeef->plt_unref (plt);
        DdbSearchWidget *pltWidget = (DdbSearchWidget *)[self view];
        deadbeef->sendmessage (DB_EV_FOCUS_SELECTION, (uintptr_t)[pltWidget listview], PL_MAIN, 0);
    }
}

- (void)selectionChanged:(DdbListviewRow_t)row {
    DdbSearchWidget *pltWidget = (DdbSearchWidget *)[self view];
    deadbeef->sendmessage (DB_EV_SELCHANGED, (uintptr_t)[pltWidget listview], deadbeef->plt_get_curr_idx (), [self playlistIter]);
    deadbeef->sendmessage (DB_EV_FOCUS_SELECTION, (uintptr_t)[pltWidget listview], PL_MAIN, 0);
}

- (void)reset {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_search_reset (plt);
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION | DDB_PLAYLIST_CHANGE_SEARCHRESULT, 0);
        deadbeef->plt_unref (plt);
    }
    [_entry setStringValue:@""];
    [_entry becomeFirstResponder];

}

- (void)sortColumn:(DdbListviewCol_t)column withOrder:(int)order {
    plt_col_info_t *c = &_columns[(int)column];
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_SEARCH, c->type, c->format, order-1);
    deadbeef->plt_unref (plt);
}

- (void)dealloc {
    [self cleanup];
}

@end
