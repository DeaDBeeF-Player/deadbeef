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

#import <Foundation/Foundation.h>
#import "DdbShared.h"
#import "deadbeef.h"

NSString * const ddbPlaylistItemsUTIType = @"org.deadbeef.playlistItems";
NSString * const ddbMedialibItemUTIType = @"org.deadbeef.medialibItem";

extern DB_functions_t *deadbeef;

#define _(x) x

int
cocoaui_add_new_playlist (void) {
    int cnt = deadbeef->plt_get_count ();
    int i;
    int idx = 0;
    for (;;) {
        char name[100];
        if (!idx) {
            strcpy (name, _("New Playlist"));
        }
        else {
            snprintf (name, sizeof (name), _("New Playlist (%d)"), idx);
        }
        deadbeef->pl_lock ();
        for (i = 0; i < cnt; i++) {
            char t[100];
            ddb_playlist_t *plt = deadbeef->plt_get_for_idx (i);
            deadbeef->plt_get_title (plt, t, sizeof (t));
            deadbeef->plt_unref (plt);
            if (!strcasecmp (t, name)) {
                break;
            }
        }
        deadbeef->pl_unlock ();
        if (i == cnt) {
            return deadbeef->plt_add (cnt, name);
        }
        idx++;
    }
    return -1;
}

void
cocoaui_playlist_set_curr (int playlist) {
    deadbeef->plt_set_curr_idx (playlist);
    deadbeef->conf_set_int ("playlist.current", playlist);
}

NSString *
conf_get_nsstr (const char *key, const char *def) {
    char value[4096];
    deadbeef->conf_get_str (key, def, value, sizeof (value));
    return [NSString stringWithUTF8String:value];
}

void
conf_set_nsstr (const char *key, NSString *value) {
    deadbeef->conf_set_str (key, value.UTF8String);
}

int
cocoaui_remove_playlist (int index) {
//    NSAlert *alert = [NSAlert new];
//
//    alert.messageText = @"Removing playlist";
//    alert.informativeText = [NSString stringWithFormat:@"Do you really want to remove the playlist '%@'?", plt_get_title_wrapper (_tab_clicked)];
//    [alert addButtonWithTitle:@"No"];
//    [alert addButtonWithTitle:@"Yes"];
//#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
//    if (@available(macOS 10.16, *)) {
//        alert.buttons[1].hasDestructiveAction = YES;
//    }
//#endif
//
//    self.playlistConfirmationAlertOpen = YES;
//
//    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
//        self.playlistConfirmationAlertOpen = NO;
//        if (returnCode == NSAlertFirstButtonReturn) {
//            self.needsDisplay = YES;
//            return;
//        }
//        deadbeef->plt_remove (self.tab_clicked);
//        int playlist = deadbeef->plt_get_curr_idx ();
//        deadbeef->conf_set_int ("playlist.current", playlist);
//        [self scrollToTab:playlist];
//        self.tab_clicked = -1;
//        self.needsDisplay = YES;
//    }];
    return 0;
}
