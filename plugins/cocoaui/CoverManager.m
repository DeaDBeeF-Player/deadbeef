#import "CoverManager.h"
#include "../../deadbeef.h"
#include "../../plugins/artwork/artwork.h"

extern DB_functions_t *deadbeef;

static CoverManager *g_DefaultCoverManager = nil;

#define CACHE_SIZE 20

@implementation CoverManager {
    ddb_artwork_plugin_t *_artwork_plugin;
    NSImage *_testCover;
    NSMutableDictionary *_cachedCovers[CACHE_SIZE];
    char *_name_tf;
}

- (void)dealloc {
    deadbeef->tf_free (_name_tf);
}

+ (CoverManager *)defaultCoverManager {
    if (!g_DefaultCoverManager) {
        g_DefaultCoverManager = [[CoverManager alloc] init];
    }
    return g_DefaultCoverManager;
}

- (CoverManager *)init {
    self = [super init];
    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    _testCover = [NSImage imageNamed:@"noartwork.png"];
    //_name_tf = deadbeef->tf_compile ("$if2(b:%album%-a:%artist%,$if2(b:%album%,$if2(a:%artist,f:%directoryname%/%filename%)))");
    _name_tf = deadbeef->tf_compile ("b:%album%-a:%artist%");
    return self;
}

- (NSImage *)getTestCover {
    return _testCover;
}

- (NSString *)hashForTrack:(DB_playItem_t *)track  {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        .it = track,
    };

    char buffer[PATH_MAX];
    deadbeef->tf_eval (&ctx, _name_tf, buffer, sizeof (buffer));
    return [NSString stringWithUTF8String:buffer];
}

typedef struct {
    void (*real_callback) (NSImage *img, void *user_data);
    void *real_user_data;
} cover_callback_info_t;

static void cover_loaded_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    if (!error) {
        NSImage *img = [[NSImage alloc] initWithContentsOfFile:[NSString stringWithUTF8String:cover->filename]];
        if (img) {
            CoverManager *cm = [CoverManager defaultCoverManager];
            [cm addCoverForTrack:query->track withImage:img];
            cover_callback_info_t *info = query->user_data;
            info->real_callback (img, info->real_user_data);
        }
    }

    deadbeef->pl_item_unref (query->track);
    free (query);
}

- (void)addCoverForTrack:(ddb_playItem_t *)track withImage:(NSImage *)img {
    NSString *hash = [self hashForTrack:track];

    NSNumber *t = [NSNumber numberWithLong:time (NULL)];
    int i_min = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {
        NSMutableDictionary *d = _cachedCovers[i];
        if (!d) {
            i_min = i;
            break;
        }
        NSNumber *ts = d[@"ts"];
        if ([ts isLessThan:t]) {
            i_min = i;
        }
    }

    NSMutableDictionary *d = [[NSMutableDictionary alloc] initWithObjectsAndKeys:hash, @"hash", t, @"ts", img, @"img", nil];
    _cachedCovers[i_min] = d;
}

- (NSImage *)getCoverForTrack:(DB_playItem_t *)track withCallbackWhenReady:(void (*) (NSImage *img, void *user_data))callback withUserDataForCallback:(void *)user_data {
    NSString *hash = [self hashForTrack:track];
    for (int i = 0; i < CACHE_SIZE; i++) {
        NSMutableDictionary *d = _cachedCovers[i];
        if (d) {
            NSString *hashVal = d[@"hash"];
            if ([hashVal isEqualToString:hash]) {
                d[@"ts"] = [NSNumber numberWithLong:time (NULL)];
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

    _artwork_plugin->cover_get (query, cover_loaded_callback);

    return nil;
}

@end
