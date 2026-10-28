/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>

NSString * const ddbPlaylistItemsUTIType = @"org.deadbeef.playlistItems";
NSString * const ddbPlaylistDataUTIType = @"org.deadbeef.playlistData";

extern DB_functions_t *deadbeef;

#define _(x) x

int
cocoaui_add_new_playlist (void) {
    return deadbeef->plt_add2 (deadbeef->plt_get_count (), NULL);
}

NSString *
conf_get_nsstr (const char *key, const char *def) {
    char value[4096];
    deadbeef->conf_get_str (key, def, value, sizeof (value));
    return @(value);
}

void
conf_set_nsstr (const char *key, NSString *value) {
    deadbeef->conf_set_str (key, value.UTF8String);
}

NSString *
plt_get_title_wrapper (int plt) {
    if (plt == -1) {
        return @"";
    }
    ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);

    char buffer[1000];
    deadbeef->plt_get_title (p, buffer, sizeof (buffer));
    deadbeef->plt_unref (p);
    return @(buffer);
}

