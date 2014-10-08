//
//  DdbSearchViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 08/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbSearchViewController.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbSearchViewController

#define DEFAULT_COLUMNS "[{\"title\":\"Artist - Album\", \"format\":\"%artist%[ - %album%]\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track%\", \"size\":\"50\"}, {\"title\":\"Track Title\", \"format\":\"%title%\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

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
    [_listview setDelegate:(id<DdbListviewDelegate>)self];
}

- (id)init {
    self = [super initWithNibName:@"Search" bundle:nil];
    if (self) {
        [self initContent];
    }
    return self;
}

- (NSString *)rowGroupStr:(DdbListviewRow_t)row {
    return NULL;
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    NSString *val = [textField stringValue];
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_search_process (plt, [val UTF8String]);
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, 0, 0);
        deadbeef->plt_unref (plt);
    }
}

- (void)reset {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_search_reset (plt);
        deadbeef->plt_unref (plt);
    }
    [_entry setStringValue:@""];
}
@end
