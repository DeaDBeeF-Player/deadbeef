/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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

#import "CoverManager.h"
#include "../../deadbeef.h"
#include "../../plugins/artwork/artwork.h"

extern DB_functions_t *deadbeef;

static CoverManager *g_DefaultCoverManager = nil;

#define CACHE_SIZE 20

@interface CoverManager() {
    NSMutableDictionary *_cachedCovers[CACHE_SIZE];
}

@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;
@property (nonatomic,readwrite) NSImage *defaultCover;
@property (nonatomic) char *name_tf;

@end

@implementation CoverManager

- (void)dealloc {
    deadbeef->tf_free (_name_tf);
    _name_tf = NULL;
    for (int i = 0; i < CACHE_SIZE; i++) {
        _cachedCovers[i] = nil;
    }
}

+ (CoverManager *)defaultCoverManager {
    if (!g_DefaultCoverManager) {
        g_DefaultCoverManager = [CoverManager new];
    }
    return g_DefaultCoverManager;
}

- (CoverManager *)init {
    self = [super init];
    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    _defaultCover = [NSImage imageNamed:@"noartwork.png"];

    // Each file may contain its own album art, therefore we can't really cache them easily by album/artist/title.
    // This however would duplicate the same image, for every track in every album, if the custom grouping is set per file.
    //_name_tf = deadbeef->tf_compile ("b:%album%-a:%artist%-t:%title%");
    _name_tf = deadbeef->tf_compile ("%_path_raw%");
    return self;
}

- (NSString *)hashForTrack:(DB_playItem_t *)track  {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        .it = track,
    };

    char buffer[PATH_MAX];
    deadbeef->tf_eval (&ctx, self.name_tf, buffer, sizeof (buffer));
    return [NSString stringWithUTF8String:buffer];
}

typedef struct {
    void (*real_callback) (NSImage *img, void *user_data);
    void *real_user_data;
} cover_callback_info_t;

static void cover_loaded_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        // We want to load the images in background, to keep UI responsive
        CoverManager *cm = [CoverManager defaultCoverManager];
        NSImage *img = nil;
        if (!img && cover && cover->blob) {
            NSData *data = [NSData dataWithBytesNoCopy:cover->blob + cover->blob_image_offset length:cover->blob_image_size freeWhenDone:NO];
            img = [[NSImage alloc] initWithData:data];
            data = nil;
        }
        if (!img && cover && cover->image_filename) {
            img = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:cover->image_filename]];
        }
        if (!img) {
            img = [cm defaultCover];
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            if (img) {
                [cm addCoverForTrack:query->track withImage:img];
                cover_callback_info_t *info = query->user_data;
                info->real_callback (img, info->real_user_data);
            }

            deadbeef->pl_item_unref (query->track);

            if (cover) {
                cm->_artwork_plugin->cover_info_free (cover);
            }

            free (query);
        });
    });
}

- (void)addCoverForTrack:(ddb_playItem_t *)track withImage:(NSImage *)img {
    NSString *hash = [self hashForTrack:track];

    NSNumber *t = [NSNumber numberWithLong:time (NULL)];
    int i_min = 0;
    BOOL foundEmpty = NO;
    for (int i = 0; i < CACHE_SIZE; i++) {
        NSMutableDictionary *d = _cachedCovers[i];
        if (!d && !foundEmpty) {
            i_min = i;
            foundEmpty = YES;
            continue;
        }

        // same image
        if ([d[@"hash"] isEqualToString:hash]) {
            i_min = i;
            break;
        }

        // lowest timestamp
        NSNumber *ts = d[@"ts"];
        if (!foundEmpty && [ts isLessThan:t]) {
            i_min = i;
        }
    }

//    NSLog (@"+ %@", hash);
    NSMutableDictionary *d = [[NSMutableDictionary alloc] initWithObjectsAndKeys:hash, @"hash", t, @"ts", nil];
    if (img != nil) {
        d[@"img"] = img;
    }
    _cachedCovers[i_min] = d;
}

- (void)dumpCache {
    for (int i = 0; i < CACHE_SIZE; i++) {
        NSLog(@"%@", _cachedCovers[i]);
    }
}

- (NSDictionary *)findInCache:(NSString *)key {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if ([_cachedCovers[i][@"hash"] isEqualToString:key]) {
            return _cachedCovers[i];
        }
    }
    return nil;
}

- (NSImage *)getCoverForTrack:(DB_playItem_t *)track withCallbackWhenReady:(void (*) (NSImage *img, void *user_data))callback withUserDataForCallback:(void *)user_data {
    if (!self.artwork_plugin) {
        callback (nil, user_data);
        return nil;
    }

    NSString *hash = [self hashForTrack:track];
    for (int i = 0; i < CACHE_SIZE; i++) {
        NSMutableDictionary *d = _cachedCovers[i];
        if (d) {
            NSString *hashVal = d[@"hash"];
            if ([hashVal isEqualToString:hash]) {
                d[@"ts"] = [NSNumber numberWithLong:time (NULL)];
                callback (nil, user_data);
                return d[@"img"];
            }
        }
    }

    ddb_cover_query_t *query = calloc (sizeof (ddb_cover_query_t), 1);
    query->_size = sizeof (ddb_cover_query_t);
    query->track = track;
    deadbeef->pl_item_ref (track);

    cover_callback_info_t *info = calloc (sizeof (cover_callback_info_t), 1);
    info->real_callback = callback;
    info->real_user_data = user_data;
    query->user_data = info;

    [self addCoverForTrack:query->track withImage:nil];
    self.artwork_plugin->cover_get (query, cover_loaded_callback);

    return nil;
}

@end
