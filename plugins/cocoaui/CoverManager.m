/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>
#include "../../plugins/artwork/artwork.h"

#define DEBUG_COUNTER 0

extern DB_functions_t *deadbeef;

static CoverManager *g_DefaultCoverManager = nil;

#define CACHE_SIZE 50

@interface CachedCover : NSObject
@property (nullable, nonatomic) NSImage *image;
@end

@implementation CachedCover
@end

#pragma mark -

@interface CoverManager()

@property (nonnull, nonatomic) NSHashTable<id<CoverManagerListener>> *listeners;

/// A background serial queue for creating NSImages.
/// It has to be serial, to load the images in the same order they were requested.
@property (nonatomic) dispatch_queue_t loaderQueue;

@property (nonatomic) NSCache<NSString *,CachedCover *> *cachedCovers;
@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;
@property (nonatomic) NSString *defaultCoverPath;
@property (nonatomic,readwrite) NSImage *defaultCover;
@property (nonatomic) char *name_tf;
@property (nonatomic) int imageSize;

#if DEBUG_COUNTER
@property (atomic) int counter;
#endif

@end

@implementation CoverManager

+ (CoverManager *)shared {
    if (!g_DefaultCoverManager) {
        g_DefaultCoverManager = [CoverManager new];
    }
    return g_DefaultCoverManager;
}

+ (void)freeSharedInstance {
    g_DefaultCoverManager = nil;
}

- (void)dealloc {
    if (_artwork_plugin) {
        _artwork_plugin->remove_listener (_artwork_listener, (__bridge void *)(self));
    }
    deadbeef->tf_free (_name_tf);
    _name_tf = NULL;
}

static void
_artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    CoverManager *self = (__bridge CoverManager *)user_data;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (event == DDB_ARTWORK_SETTINGS_DID_CHANGE) {
            [self settingsDidChangeForTrack:(ddb_playItem_t *)p1];
        }
    });
}

- (CoverManager *)init {
    self = [super init];
    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    if (_artwork_plugin == NULL) {
        return self;
    }

    _listeners = [NSHashTable weakObjectsHashTable];

    self.imageSize = deadbeef->conf_get_int("artwork.image_size", 256);
    [self updateDefaultCover];

    _cachedCovers = [NSCache new];
    _cachedCovers.countLimit = CACHE_SIZE;

    // Each file may contain its own album art, therefore we can't really cache them easily by album/artist/title.
    // This however would duplicate the same image, for every track in every album, if the custom grouping is set per file.
    //_name_tf = deadbeef->tf_compile ("b:%album%-a:%artist%-t:%title%");
    _name_tf = deadbeef->tf_compile ("%_path_raw%");

    _loaderQueue = dispatch_queue_create("CoverManagerLoaderQueue", NULL);

    _artwork_plugin->add_listener(_artwork_listener, (__bridge void *)(self));

    return self;
}

- (void)addListener:(id<CoverManagerListener>)observer {
    [self.listeners addObject:observer];
}

- (void)removeListener:(id<CoverManagerListener>) observer {
    [self.listeners removeObject:observer];
}

- (void)settingsDidChangeForTrack:(ddb_playItem_t *)track {
    if (track == NULL) {
        self.imageSize = deadbeef->conf_get_int("artwork.image_size", 256);
        [self updateDefaultCover];
        [self resetCache];
    }
    else {
        NSString *hash = [self hashForTrack:track];
        [self.cachedCovers removeObjectForKey:hash];
    }
}

- (void)updateDefaultCover {
    if (_artwork_plugin == NULL) {
        return;
    }
    char path[PATH_MAX];
    _artwork_plugin->default_image_path(path, sizeof(path));

    NSString *defaultCoverPath = @(path);

    if (![self.defaultCoverPath isEqualToString:defaultCoverPath]) {
        self.defaultCoverPath = defaultCoverPath;
        self.defaultCover = [[NSImage alloc] initWithContentsOfFile:defaultCoverPath];
        if (self.defaultCover == nil) {
            self.defaultCover = [NSImage imageWithSize:NSMakeSize(1, 1) flipped:NO drawingHandler:^BOOL(NSRect dstRect) {
                [NSColor.windowBackgroundColor set];
                NSRectFill(NSMakeRect(0, 0, 1, 1));
                return YES;
            }];
        }
    }
}

- (NSString *)hashForTrack:(ddb_playItem_t *)track  {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        .it = track,
    };

    char buffer[PATH_MAX];
    deadbeef->tf_eval (&ctx, self.name_tf, buffer, sizeof (buffer));
    return @(buffer);
}

- (nullable NSImage *)loadImageFromCover:(nonnull ddb_cover_info_t *)cover {
    NSImage *img;

    if (cover && cover->image_filename) {
        img = [[NSImage alloc] initWithContentsOfFile:@(cover->image_filename)];
    }

    if (img != nil) {
        const int max_image_size = self.imageSize;

        CGSize size = img.size;
        if (size.width > max_image_size
            || size.height > max_image_size) {
            CGSize newSize = CGSizeMake(max_image_size, max_image_size);
            newSize = [self desiredSizeForImageSize:size availableSize:newSize];

            img = [self createScaledImage:img newSize:newSize];
        }
    }

    if (img == nil) {
        img = self.defaultCover;
    }
    return img;
}

- (void)cleanupQuery:(ddb_cover_query_t *)query {
    // just clean the completion blocks and queries -- the artwork plugin will take care of the covers
    __unused void (^completionBlock)(NSImage *) = (void (^)(NSImage *__strong))CFBridgingRelease(query->user_data);
    deadbeef->pl_item_unref (query->track);
    free (query);
#if DEBUG_COUNTER
    self.counter -= 1;
    NSLog (@"*** counter: %d\n", self.counter);
#endif
}

static void
cover_loaded_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    CoverManager *self = CoverManager.shared;

    if (self.isTerminating) {
        [self cleanupQuery:query];
        return;
    }

    // Load the image on background queue
    dispatch_async(self.loaderQueue, ^{
        if (self.isTerminating) {
            [self cleanupQuery:query];
            return;
        }
        NSImage *img;

        if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
            img = [self loadImageFromCover:cover];
        }

        // Update the UI on main queue
        dispatch_async(dispatch_get_main_queue(), ^{
            if (self.isTerminating) {
                [self cleanupQuery:query];
                return;
            }
            if (!(query->flags & DDB_ARTWORK_FLAG_CANCELLED)) {
                [self addCoverForTrack:query->track withImage:img];
            }
            void (^completionBlock)(NSImage *) = (void (^)(NSImage *__strong))CFBridgingRelease(query->user_data);
            completionBlock(img);

            // Free the query -- it's fast, so it's OK to free it on main queue
            deadbeef->pl_item_unref (query->track);
            free (query);

#if DEBUG_COUNTER
            self.counter -= 1;
            NSLog (@"*** counter: %d\n", self.counter);
#endif

            // Release the cover on background queue
            if (cover != NULL) {
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                    self->_artwork_plugin->cover_info_release (cover);
                });
            }
        });
    });
}

- (void)addCoverForTrack:(ddb_playItem_t *)track withImage:(NSImage *)img {
    NSString *hash = [self hashForTrack:track];

    CachedCover *cover = [CachedCover new];
    cover.image = img;

    [self.cachedCovers setObject:cover forKey:hash];
}

- (nullable NSImage *)coverForTrack:(nonnull ddb_playItem_t *)track sourceId:(int64_t)sourceId completionBlock:(nonnull void (^) (NSImage * img))completionBlock {
    if (!self.artwork_plugin) {
        completionBlock(nil);
        return nil;
    }

    if (self.isTerminating) {
        completionBlock(nil);
        return nil;
    }

    NSString *hash = [self hashForTrack:track];

    CachedCover *cover = [self.cachedCovers objectForKey:hash];
    if (cover != nil) {
        if (cover.image == nil) {
            completionBlock(nil);
        }
        // Callback is not executed if the image is non-nil, to avoid double drawing.
        // The caller must release user data if the returned image is not nil.
        return cover.image;
    }

    ddb_cover_query_t *query = calloc (1, sizeof (ddb_cover_query_t));
    query->_size = sizeof (ddb_cover_query_t);
    query->track = track;
    deadbeef->pl_item_ref (track);
    query->source_id = sourceId;

    query->user_data = (void *)CFBridgingRetain(completionBlock);

#if DEBUG_COUNTER
    self.counter += 1;
    NSLog (@"*** counter: %d\n", self.counter);
#endif

    self.artwork_plugin->cover_get (query, cover_loaded_callback);

    return nil;
}

- (nullable NSImage *)coverForTrack:(nonnull ddb_playItem_t *)track completionBlock:(nonnull void (^) (NSImage * img))completionBlock {
    return [self coverForTrack:track sourceId:0 completionBlock:completionBlock];
}

- (void)resetCache {
    [self.cachedCovers removeAllObjects];
    for (id<CoverManagerListener> listener in self.listeners) {
        if (listener != nil) {
            [listener coverManagerDidReset];
        }
    }
}

- (NSImage *)createScaledImage:(NSImage *)image newSize:(CGSize)newSize {
    CGSize originalSize = image.size;
    if (originalSize.width <= newSize.width && originalSize.height <= newSize.height) {
        return image;
    }
    NSImage *cachedImage = [[NSImage alloc] initWithSize:newSize];
    [cachedImage lockFocus];
    cachedImage.size = newSize;
    NSGraphicsContext.currentContext.imageInterpolation = NSImageInterpolationHigh;
    [image drawInRect:NSMakeRect(0, 0, newSize.width, newSize.height) fromRect:CGRectMake(0, 0, originalSize.width, originalSize.height) operation:NSCompositingOperationCopy fraction:1.0];
    [cachedImage unlockFocus];
    return cachedImage;
}

- (CGSize)desiredSizeForImageSize:(CGSize)imageSize availableSize:(CGSize)availableSize {
    CGFloat scale = MIN(availableSize.width/imageSize.width, availableSize.height/imageSize.height);

    return CGSizeMake(imageSize.width * scale, imageSize.height * scale);
}

@end
