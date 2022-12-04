//
/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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



#import "PlaylistUtil.h"

extern DB_functions_t *deadbeef;

@implementation PlaylistUtil

+ (PlaylistUtil *)shared {
    static PlaylistUtil *instance;
    if (instance == NULL) {
        instance = [PlaylistUtil new];
    }
    return instance;
}

- (void)moveItemsFromPlaylist:(ddb_playlist_t *)from toPlaylist:(ddb_playlist_t *)to afterItem:(ddb_playItem_t *)after {
    ddb_playItem_t *it = deadbeef->plt_get_head_item(from, PL_MAIN);
    while (it != NULL) {
        ddb_playItem_t *next = deadbeef->pl_get_next(it, PL_MAIN);

        deadbeef->plt_remove_item(from, it);
        deadbeef->plt_insert_item(to, after, it);
        after = it;

        deadbeef->pl_item_unref(it);
        it = next;
    }
}

@end
