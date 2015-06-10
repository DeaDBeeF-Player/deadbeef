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
#import "TrackPropertiesWindowController.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

#define MAX_GUI_FIELD_LEN 5000

// full metadata
static const char *types[] = {
    "artist", "Artist",
    "title", "Track Title",
    "album", "Album",
    "year", "Date",
    "track", "Track Number",
    "numtracks", "Total Tracks",
    "genre", "Genre",
    "composer", "Composer",
    "disc", "Disc Number",
    "comment", "Comment",
    NULL
};

static const char *hc_props[] = {
    ":URI", "Location",
    ":TRACKNUM", "Subtrack Index",
    ":DURATION", "Duration",
    ":TAGS", "Tag Type(s)",
    ":HAS_EMBEDDED_CUESHEET", "Embedded Cuesheet",
    ":DECODER", "Codec",
    NULL
};

@interface TrackPropertiesWindowController () {
    int _iter;
    DB_playItem_t **_tracks;
    int _numtracks;
    BOOL _modified;
    NSMutableArray *_store;
    NSMutableArray *_propstore;
    BOOL _progress_aborted;
    BOOL _close_after_writing;
}

@end

@implementation TrackPropertiesWindowController

- (void)freeTrackList {
    if (_tracks) {
        for (int i = 0; i < _numtracks; i++) {
            deadbeef->pl_item_unref (_tracks[i]);
        }
        free (_tracks);
        _tracks = NULL;
        _numtracks = 0;
    }
}

- (void)dealloc {
    [self freeTrackList];
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    [[self window] setDelegate:(id<NSWindowDelegate>)self];

    _store = [[NSMutableArray alloc] init];
    _propstore = [[NSMutableArray alloc] init];
    [self fill];
    [_metadataTableView setDataSource:(id<NSTableViewDataSource>)self];
    [_propertiesTableView setDataSource:(id<NSTableViewDataSource>)self];
    [_metadataTableView reloadData];
    [_propertiesTableView reloadData];
}

- (void)buildTrackListForCtx:(int)ctx {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!plt) {
        return;
    }
    deadbeef->pl_lock ();

    int num = 0;
    if (ctx == DDB_ACTION_CTX_SELECTION) {
        num = deadbeef->plt_getselcount (plt);
    }
    else if (ctx == DDB_ACTION_CTX_PLAYLIST) {
        num = deadbeef->plt_get_item_count (plt, PL_MAIN);
    }
    else if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        num = 1;
    }
    if (num <= 0) {
        deadbeef->pl_unlock ();
        deadbeef->plt_unref (plt);
        return;
    }

    _tracks = malloc (sizeof (DB_playItem_t *) * num);
    if (!_tracks) {
        fprintf (stderr, "gtkui: failed to alloc %d bytes to store selected tracks\n", (int)(num * sizeof (void *)));
        deadbeef->pl_unlock ();
        deadbeef->plt_unref (plt);
        return;
    }

    if (ctx == DDB_ACTION_CTX_NOWPLAYING) {
        DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
        if (!it) {
            free (_tracks);
            _tracks = NULL;
            deadbeef->pl_unlock ();
            deadbeef->plt_unref (plt);
            return;
        }
        _tracks[0] = it;
    }
    else {
        int n = 0;
        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
        while (it) {
            if (ctx == DDB_ACTION_CTX_PLAYLIST || deadbeef->pl_is_selected (it)) {
                assert (n < num);
                deadbeef->pl_item_ref (it);
                _tracks[n++] = it;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    _numtracks = num;

    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);
}

int
build_key_list (const char ***pkeys, int props, DB_playItem_t **tracks, int numtracks) {
    int sz = 20;
    const char **keys = malloc (sizeof (const char *) * sz);
    if (!keys) {
        fprintf (stderr, "fatal: out of memory allocating key list\n");
        assert (0);
        return 0;
    }

    int n = 0;

    for (int i = 0; i < numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (tracks[i]);
        while (meta) {
            if (meta->key[0] != '!' && ((props && meta->key[0] == ':') || (!props && meta->key[0] != ':'))) {
                int k = 0;
                for (; k < n; k++) {
                    if (meta->key == keys[k]) {
                        break;
                    }
                }
                if (k == n) {
                    if (n >= sz) {
                        sz *= 2;
                        keys = realloc (keys, sizeof (const char *) * sz);
                        if (!keys) {
                            fprintf (stderr, "fatal: out of memory reallocating key list (%d keys)\n", sz);
                            assert (0);
                        }
                    }
                    keys[n++] = meta->key;
                }
            }
            meta = meta->next;
        }
    }

    *pkeys = keys;
    return n;
}

static int
equals_ptr (const char *a, const char *b) {
    return a == b;
}

#define min(x,y) ((x)<(y)?(x):(y))

#define isutf(c) (((c)&0xC0)!=0x80)

static void
u8_dec(const char *s, int32_t *i)
{
    (void)(isutf(s[--(*i)]) || isutf(s[--(*i)]) ||
           isutf(s[--(*i)]) || --(*i));
}

static int
get_field_value (char *out, int size, const char *key, const char *(*getter)(DB_playItem_t *it, const char *key), int (*equals)(const char *a, const char *b), DB_playItem_t **tracks, int numtracks) {
    char *out_start = out;

    int multiple = 0;
    *out = 0;
    if (numtracks == 0) {
        return 0;
    }
    char *p = out;
    deadbeef->pl_lock ();
    const char **prev = malloc (sizeof (const char *) * numtracks);
    memset (prev, 0, sizeof (const char *) * numtracks);
    for (int i = 0; i < numtracks; i++) {
        const char *val = getter (tracks[i], key);
        if (val && val[0] == 0) {
            val = NULL;
        }
        if (i > 0/* || (val && strlen (val) >= MAX_GUI_FIELD_LEN)*/) {
            int n = 0;
            for (; n < i; n++) {
                if (equals (prev[n], val)) {
                    break;
                }
            }
            if (n == i/* || (val && strlen (val) >= MAX_GUI_FIELD_LEN)*/) {
                multiple = 1;
                if (val) {
                    size_t l = snprintf (out, size, out == p ? "%s" : "; %s", val ? val : "");
                    l = min (l, size);
                    out += l;
                    size -= l;
                }
            }
        }
        else if (val) {
            size_t l = snprintf (out, size, "%s", val ? val : "");
            l = min (l, size);
            out += l;
            size -= l;
        }
        prev[i] = val;
        if (size <= 1) {
            break;
        }
    }
    deadbeef->pl_unlock ();
    if (size <= 1) {
        int idx = (int)(out - 4 - out_start);
        u8_dec (out_start , &idx);
        char *prev = out_start + idx;
        strcpy (prev, "...");
    }
    free (prev);
    return multiple;
}

void
add_field (NSMutableArray *store, const char *key, const char *title, int is_prop, DB_playItem_t **tracks, int numtracks) {
    // get value to edit
    const char *mult = is_prop ? "" : "[Multiple values] ";
    char val[MAX_GUI_FIELD_LEN];
    size_t ml = strlen (mult);
    memcpy (val, mult, ml+1);
    int n = get_field_value (val + ml, (int)(sizeof (val) - ml), key, deadbeef->pl_find_meta_raw, equals_ptr, tracks, numtracks);

    if (!is_prop) {
        if (n) {
            [store addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:title], @"title", [NSString stringWithUTF8String:val], @"value", [NSString stringWithUTF8String:key], @"key", [NSNumber numberWithInt: (n ? 1 : 0)], @"n", nil]];
        }
        else {
            deadbeef->pl_lock ();
            const char *val = deadbeef->pl_find_meta_raw (tracks[0], key);
            if (!val) {
                val = "";
            }
            [store addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:title], @"title", [NSString stringWithUTF8String:val], @"value", [NSString stringWithUTF8String:key], @"key", [NSNumber numberWithInt: (n ? 1 : 0)], @"n", nil]];
            deadbeef->pl_unlock ();
        }
    }
    else {
        [store addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:title], @"title", [NSString stringWithUTF8String:(n ? val : val + ml)], @"value", nil]];
    }
}

- (void)fillMeta {
    [_store removeAllObjects];
    if (!_tracks) {
        return;
    }

    const char **keys = NULL;
    int nkeys = build_key_list (&keys, 0, _tracks, _numtracks);

    // add "standard" fields
    for (int i = 0; types[i]; i += 2) {
        add_field (_store, types[i], types[i+1], 0, _tracks, _numtracks);
    }

    // add all other fields
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; types[i]; i += 2) {
            if (!strcasecmp (keys[k], types[i])) {
                break;
            }
        }
        if (types[i]) {
            continue;
        }

        char title[MAX_GUI_FIELD_LEN];
        if (!types[i]) {
            snprintf (title, sizeof (title), "<%s>", keys[k]);
        }
        add_field (_store, keys[k], title, 0, _tracks, _numtracks);
    }
    if (keys) {
        free (keys);
    }
}

- (void)fillMetadata {
    _modified = NO;

    deadbeef->pl_lock ();

    [self fillMeta];
    [_propstore removeAllObjects];

    // hardcoded properties
    for (int i = 0; hc_props[i]; i += 2) {
        add_field (_propstore, hc_props[i], hc_props[i+1], 1, _tracks, _numtracks);
    }
    // properties
    const char **keys = NULL;
    int nkeys = build_key_list (&keys, 1, _tracks, _numtracks);
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; hc_props[i]; i += 2) {
            if (!strcasecmp (keys[k], hc_props[i])) {
                break;
            }
        }
        if (hc_props[i]) {
            continue;
        }
        char title[MAX_GUI_FIELD_LEN];
        snprintf (title, sizeof (title), "<%s>", keys[k]+1);
        add_field (_propstore, keys[k], title, 1, _tracks, _numtracks);
    }
    if (keys) {
        free (keys);
    }

    deadbeef->pl_unlock ();

#if 0
    NSLog (@"%@\n", _propstore);
    NSLog (@"%@\n", _store);
#endif
}

- (void)fill {
    _close_after_writing = NO;

    [self freeTrackList];

    [self buildTrackListForCtx:DDB_ACTION_CTX_SELECTION];

    NSString *fname;

    if (_numtracks == 1) {
        deadbeef->pl_lock ();
        fname = [NSString stringWithUTF8String:deadbeef->pl_find_meta_raw (_tracks[0], ":URI")];

        deadbeef->pl_unlock ();
    }
    else {
        fname = @"[Multiple values]";
    }

    [self fillMetadata];

    if (_filename) {
        [_filename setStringValue:fname];
        [_metadataTableView reloadData];
        [_propertiesTableView reloadData];
    }
}

// NSTableView delegate
- (NSMutableArray *)storeForTableView:(NSTableView *)aTableView {
    if (aTableView == _metadataTableView) {
        return _store;
    }
    else if (aTableView == _propertiesTableView) {
        return _propstore;
    }

    return nil;
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return 0;
    }

    return (int)[store count];
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return nil;
    }

    if ([[aTableColumn identifier] isEqualToString:@"name"]) {
        NSString *title = [store objectAtIndex:rowIndex][@"title"];
        return title;
    }
    else if ([[aTableColumn identifier] isEqualToString:@"value"]) {
        return [store objectAtIndex:rowIndex][@"value"];
    }
    return nil;
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return;
    }

    NSMutableDictionary *dict = [store objectAtIndex:rowIndex];
    if ([[dict objectForKey:@"value"] isNotEqualTo:anObject]) {
        [dict setObject:anObject forKey:@"value"];
        if ([dict objectForKey:@"n"])
        {
            [dict setObject:[NSNumber numberWithInt:0] forKey:@"n"];
        }
        _modified = YES;
    }
}

- (void)setMetadataForSelectedTracks:(NSDictionary *)iter {
    // skip "multiple values"
    NSString *n = [iter valueForKey:@"n"];
    if (n && [n intValue] != 0)
        return;

    const char *skey = [[iter valueForKey:@"key"] UTF8String];
    const char *svalue = [[iter valueForKey:@"value"] UTF8String];

    for (int i = 0; i < _numtracks; i++) {
        const char *oldvalue= deadbeef->pl_find_meta_raw (_tracks[i], skey);
        if (oldvalue && strlen (oldvalue) > MAX_GUI_FIELD_LEN) {
            fprintf (stderr, "trkproperties: value is too long, ignored\n");
            continue;
        }

        if (*svalue) {
            deadbeef->pl_replace_meta (_tracks[i], skey, svalue);
        }
        else {
            deadbeef->pl_delete_meta (_tracks[i], skey);
        }
    }
}

- (void)writeMetaWorker {
    for (int t = 0; t < _numtracks; t++) {
        if (_progress_aborted) {
            break;
        }
        DB_playItem_t *track = _tracks[t];
        deadbeef->pl_lock ();
        const char *dec = deadbeef->pl_find_meta_raw (track, ":DECODER");
        char decoder_id[100];
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = track && dec;
        deadbeef->pl_unlock ();
        if (match) {
            int is_subtrack = deadbeef->pl_get_item_flags (track) & DDB_IS_SUBTRACK;
            if (is_subtrack) {
                continue;
            }
            // update progress
            deadbeef->pl_item_ref (track);
            dispatch_async(dispatch_get_main_queue(), ^{
                deadbeef->pl_lock ();
                NSString *path = [NSString stringWithUTF8String:deadbeef->pl_find_meta_raw (track, ":URI")];
                deadbeef->pl_unlock ();
                [_currentTrackPath setStringValue:path];
                deadbeef->pl_item_unref (track);
            });
            // find decoder
            DB_decoder_t *dec = NULL;
            DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                    dec = decoders[i];
                    if (dec->write_metadata) {
                        dec->write_metadata (track);
                    }
                    break;
                }
            }
        }
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        [_progressPanel orderOut:self];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        _modified = NO;
// FIXME: update playlist/search/...
#if 0
        main_refresh ();
        search_refresh ();
        show_track_properties_dlg (last_ctx);
#endif
        if (_close_after_writing) {
            [[self window] close];
        }
    });
}

- (IBAction)applyTrackPropertiesAction:(id)sender {
    deadbeef->pl_lock ();
    NSMutableArray *store = [self storeForTableView:_metadataTableView];

    // delete all metadata properties that are not in the listview
    for (int i = 0; i < _numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (_tracks[i]);
        while (meta) {
            DB_metaInfo_t *next = meta->next;
            if (meta->key[0] != ':' && meta->key[0] != '!' && meta->key[0] != '_') {
                NSDictionary *iter;
                for (iter in store) {
                    if (!strcasecmp ([[iter valueForKey:@"key"] UTF8String] , meta->key)) {
                        // field found, don't delete
                        break;
                    }
                }

                if (!iter) {
                    // field not found, delete
                    deadbeef->pl_delete_metadata (_tracks[i], meta);
                }
            }
            meta = next;
        }
    }
    // put all metainfo into track
    for (NSDictionary *iter in store) {
        [self setMetadataForSelectedTracks:iter];
    }
    deadbeef->pl_unlock ();

    for (int i = 0; i < _numtracks; i++) {
        ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
        ev->track = _tracks[i];
        deadbeef->pl_item_ref (ev->track);
        deadbeef->event_send ((ddb_event_t*)ev, 0, 0);
    }

    _progress_aborted = NO;

    [NSApp beginSheet:_progressPanel modalForWindow:[self window] modalDelegate:nil didEndSelector:nil contextInfo:nil];

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        [self writeMetaWorker];
    });
}

- (IBAction)cancelWritingAction:(id)sender {
    _progress_aborted = YES;
}

- (BOOL)windowShouldClose:(id)sender {
    if (_modified) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Cancel"];
        [alert setMessageText:@"Save changes?"];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert beginSheetModalForWindow:[self window] modalDelegate:self didEndSelector:@selector(alertDidEnd:returnCode:contextInfo:) contextInfo:nil];
        return NO;
    }
    return YES;
}

- (void)alertDidEnd:(NSAlert *)alert returnCode:(NSInteger)returnCode
        contextInfo:(void *)contextInfo {
    if (returnCode == NSAlertFirstButtonReturn) {
        _close_after_writing = YES;
        [self applyTrackPropertiesAction:alert];
    }
    else if (returnCode == NSAlertSecondButtonReturn){
        _modified = NO;
        [[self window] close];
    }
}

@end
