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
#import "MediaLibraryItem.h"
#import "TrackPropertiesWindowController.h"
#include <deadbeef/deadbeef.h>
#include "utf8.h"
#include "trkproperties_shared.h"
#import "TrackPropertiesSingleLineFormatter.h"
#import "TrackPropertiesNullFormatter.h"
#import "TrackPropertiesMultipleFieldsTableData.h"

extern DB_functions_t *deadbeef;

@interface TrackPropertiesWindowController ()

/// Do not remove: used via binding
@property (unsafe_unretained) BOOL singleValueSelected;

@property (nonatomic) DB_playItem_t **tracks;
@property (nonatomic) int numtracks;
@property (nonatomic) NSMutableArray *store;
@property (nonatomic) NSMutableArray *propstore;
@property (nonatomic) BOOL progress_aborted;
@property (nonatomic) BOOL close_after_writing;
@property (nonatomic) TrackPropertiesMultipleFieldsTableData *multipleFieldsTableData;

@end

@implementation TrackPropertiesWindowController

- (void)setPlaylist:(ddb_playlist_t *)plt {
    if (_playlist) {
        deadbeef->plt_unref (_playlist);
    }
    _playlist = plt;
    if (_playlist) {
        deadbeef->plt_ref (_playlist);
    }

    [self reloadContent];
}

- (void)setMediaLibraryItems:(NSArray<MediaLibraryItem *> *)mediaLibraryItems {
    _mediaLibraryItems = mediaLibraryItems;
    [self reloadContent];
}

- (void)freeTrackList {
    trkproperties_free_track_list (&_tracks, &_numtracks);
}

- (void)dealloc {
    [self freeTrackList];
    if (_playlist) {
        deadbeef->plt_unref (_playlist);
        _playlist = NULL;
    }
}

- (void)windowDidLoad {
    [super windowDidLoad];
    self.window.delegate = self;

    self.store = [NSMutableArray new];
    self.propstore = [NSMutableArray new];
    [self reloadContent];
    self.metadataTableView.dataSource = self;
    self.propertiesTableView.dataSource = self;
    self.metadataTableView.delegate = self;
    [self.metadataTableView reloadData];
    [self.propertiesTableView reloadData];
}

// NOTE: add_field gets called once for each unique key (e.g. Artist or Album),
// which means it will usually contain 10-20 fields
static void
add_field (NSMutableArray *store, const char *key, const char *title, int is_prop, DB_playItem_t **tracks, int numtracks) {

    // get all values for each key, convert from 0-separated to '; '-separated, and put into NSArray
    NSMutableArray<NSString *> *values = [NSMutableArray new];
    deadbeef->pl_lock ();
    for (int i = 0; i < numtracks; i++) {
        NSString *value = @"";
        DB_metaInfo_t *meta = deadbeef->pl_meta_for_key (tracks[i], key);
        if (meta && meta->valuesize == 1) {
            meta = NULL;
        }

        if (meta) {
            const char *p = meta->value;
            const char *end = p + meta->valuesize;

            while (p < end) {
                value = [value stringByAppendingString:@(p)];
                p += strlen (p) + 1;
                if (p < end) {
                    value = [value stringByAppendingString:p < end-1 ? @"; " : @";"];
                }
            }
        }
        [values addObject:value];
    }
    deadbeef->pl_unlock ();

    [store addObject:@{
        @"title": @(title),
        @"key": @(key),
        @"values": values
    }.mutableCopy];
}

static char *
_formatted_title_for_unknown_key(const char *key) {
    size_t l = strlen (key);
    char *title = malloc(l*4);
    title[0] = '<';
    char *t = title + 1;
    const char *p = key;
    while (*p) {
        int32_t size = 0;
        u8_nextchar (p, &size);
        int outsize = u8_toupper((const signed char *)p, size, t);
        t += outsize;
        p += size;
    }
    *t++ = '>';
    *t++ = 0;
    return title;
}

- (void)fillMeta {
    [self.store removeAllObjects];
    if (!self.tracks) {
        return;
    }

    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 0, self.tracks, self.numtracks);

    // add "standard" fields
    for (int i = 0; trkproperties_types[i]; i += 2) {
        add_field (self.store, trkproperties_types[i], trkproperties_types[i+1], 0, self.tracks, self.numtracks);
    }

    // add all other fields
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; trkproperties_types[i]; i += 2) {
            if (!strcasecmp (keys[k], trkproperties_types[i])) {
                break;
            }
        }
        if (trkproperties_types[i]) {
            continue;
        }

        char *title = _formatted_title_for_unknown_key(keys[k]);
        add_field (self.store, keys[k], title, 0, self.tracks, self.numtracks);
        free (title);
        title = NULL;
    }
    if (keys) {
        free (keys);
    }
}

- (void)fillProps {
    [self.propstore removeAllObjects];

    // hardcoded properties
    for (int i = 0; trkproperties_hc_props[i]; i += 2) {
        add_field (self.propstore, trkproperties_hc_props[i], trkproperties_hc_props[i+1], 1, self.tracks, self.numtracks);
    }
    // properties
    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 1, self.tracks, self.numtracks);
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; trkproperties_hc_props[i]; i += 2) {
            if (!strcasecmp (keys[k], trkproperties_hc_props[i])) {
                break;
            }
        }
        if (trkproperties_hc_props[i]) {
            continue;
        }
        char *title = _formatted_title_for_unknown_key(keys[k] + 1);
        add_field (self.propstore, keys[k], title, 1, self.tracks, self.numtracks);
        free (title);
        title = NULL;
    }
    if (keys) {
        free (keys);
    }
}

- (void)fillMetadata {
    self.isModified = NO;

    deadbeef->pl_lock ();

    [self fillMeta];
    [self fillProps];

    deadbeef->pl_unlock ();
}

- (void)reloadContent {
    if (!self.window) {
        return;
    }
    self.close_after_writing = NO;

    [self freeTrackList];

    if (self.playlist) {
        trkproperties_build_track_list_for_ctx (self.playlist, self.context, &_tracks, &_numtracks);
    }
    else if (self.mediaLibraryItems) {
        NSInteger count = self.mediaLibraryItems.count;
        _tracks = calloc (count, sizeof (DB_playItem_t *));
        _numtracks = 0;

        for (NSInteger i = 0; i < count; i++) {
            ddb_playItem_t *it = self.mediaLibraryItems[i].playItem;
            if (it) {
                deadbeef->pl_item_ref (it);
                _tracks[_numtracks++] = it;
            }
        }
    }

    NSString *fname;

    if (self.numtracks == 1) {
        deadbeef->pl_lock ();
        fname = @(deadbeef->pl_find_meta_raw (self.tracks[0], ":URI"));

        deadbeef->pl_unlock ();
    }
    else if (self.numtracks != 0) {
        fname = @"[Multiple values]";
    }
    else {
        fname = @"[Nothing selected]";
    }

    [self fillMetadata];

    if (self.filename) {
        self.filename.stringValue = fname;
        [self.metadataTableView reloadData];
        [self.propertiesTableView reloadData];
    }
}

- (NSMutableArray *)storeForTableView:(NSTableView *)aTableView {
    if (aTableView == self.metadataTableView) {
        return self.store;
    }
    else if (aTableView == self.propertiesTableView) {
        return self.propstore;
    }

    return nil;
}

- (NSString *)fieldValueForIndex:(NSInteger)rowIndex store:(NSMutableArray *)store isMult:(nullable BOOL *)isMult {
    NSMutableArray<NSString *> *values = store[rowIndex][@"values"];
    // get uniq values
    NSArray *uniq = [NSOrderedSet orderedSetWithArray:values].array;
    NSInteger n = uniq.count;

    NSString *val = n > 1 ? @"[Multiple Values] " : @"";
    for (NSUInteger i = 0; i < uniq.count; i++) {
        val = [val stringByAppendingString:uniq[i]];
        if (i < uniq.count - 1) {
            val = [val stringByAppendingString:@"; "];
        }
    }

    if (isMult != NULL) {
        *isMult = n > 1;
    }

    return val;
}

- (NSSet<NSString *> *)specialFields {
    static NSSet<NSString *> *specialFields;

    if (specialFields == nil) {
        specialFields = [[NSSet alloc] initWithArray:@[
            @"comment",
            @"lyrics",
        ]];
    }

    return specialFields;
}

- (void)setMetadataForSelectedTracks:(NSDictionary *)dict {
    NSString *key = dict[@"key"];
    const char *skey = key.UTF8String;
    NSMutableArray<NSString *> *values = dict[@"values"];

    for (int i = 0; i < self.numtracks; i++) {
        NSString *value = values[i];
        NSArray *components;

        // Don't split up special fields, which are not supposed to be multi-value
        if (![[self specialFields] containsObject:key.lowercaseString]) {
            components = [value componentsSeparatedByString:@";"];
        }
        else {
            components = @[value];
        }

        NSMutableArray *transformedValues = [NSMutableArray new];
        for (NSString *val in components) {
            NSUInteger j = 0;
            while ((j < val.length)
                   && [[NSCharacterSet whitespaceCharacterSet] characterIsMember:[val characterAtIndex:j]]) {
                j++;
            }
            // whitespace-only?
            if (j > 0 && j == val.length-1) {
                continue;
            }
            [transformedValues addObject: (j == 0 ? val : [val substringFromIndex:j])];
        }

        deadbeef->pl_delete_meta (self.tracks[i], skey);
        for (NSString *val in transformedValues) {
            if (val.length) {
                deadbeef->pl_append_meta (self.tracks[i], skey, val.UTF8String);
            }
        }
    }
}

- (void)writeMetaWorker {
    NSMutableSet *fileset = [NSMutableSet new];
    for (int t = 0; t < self.numtracks; t++) {
        if (self.progress_aborted) {
            break;
        }
        DB_playItem_t *track = self.tracks[t];
        deadbeef->pl_lock ();
        const char *dec = deadbeef->pl_find_meta_raw (track, ":DECODER");
        char decoder_id[100];
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = track && dec;
        NSString *uri = @(deadbeef->pl_find_meta (track, ":URI"));
        deadbeef->pl_unlock ();
        if (match) {
            int is_subtrack = deadbeef->pl_get_item_flags (track) & DDB_IS_SUBTRACK;
            if (is_subtrack) {
                if ([fileset containsObject:uri]) {
                    continue;
                }
                [fileset addObject:uri];
            }
            // update progress
            deadbeef->pl_item_ref (track);
            dispatch_async(dispatch_get_main_queue(), ^{
                self.currentTrackPath.stringValue = uri;
                deadbeef->pl_item_unref (track);
            });
            // find decoder
            DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                    DB_decoder_t *d = decoders[i];
                    if (d->write_metadata) {
                        d->write_metadata (track);
                    }
                    break;
                }
            }
        }
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp endSheet:self.progressPanel];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        self.isModified = NO;
        if (self.close_after_writing) {
            [self.window close];
        }

        [self.delegate trackPropertiesWindowControllerDidUpdateTracks:self];
    });
}

- (IBAction)applyTrackPropertiesAction:(id)sender {
    if (!self.isModified) {
        if (self.close_after_writing) {
            [self close];
        }
        return;
    }
    deadbeef->pl_lock ();
    NSMutableArray *store = [self storeForTableView:self.metadataTableView];

    // delete all metadata properties that are not in the listview
    for (int i = 0; i < self.numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (self.tracks[i]);
        while (meta) {
            DB_metaInfo_t *next = meta->next;
            if (meta->key[0] != ':' && meta->key[0] != '!' && meta->key[0] != '_') {
                NSDictionary *dict;
                for (dict in store) {
                    if (!strcasecmp (((NSString *)dict[@"key"]).UTF8String, meta->key)) {
                        // field found, don't delete
                        break;
                    }
                }

                if (!dict) {
                    // field not found, delete
                    deadbeef->pl_delete_metadata (self.tracks[i], meta);
                }
            }
            meta = next;
        }
    }
    // put all metainfo into track
    for (NSDictionary *dict in store) {
        self.metadataForSelectedTracks = dict;
    }
    deadbeef->pl_unlock ();

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

    self.progress_aborted = NO;

    [self.window beginSheet:self.progressPanel completionHandler:^(NSModalResponse returnCode) {
    }];

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        [self writeMetaWorker];
    });
}

- (IBAction)cancelWritingAction:(id)sender {
    self.progress_aborted = YES;
}

- (BOOL)windowShouldClose:(id)sender {
    if (self.isModified) {
        NSAlert *alert = [NSAlert new];
        [alert addButtonWithTitle:@"Yes"];
        [alert addButtonWithTitle:@"No"];
        [alert addButtonWithTitle:@"Cancel"];
        alert.messageText = @"Save changes?";
        alert.alertStyle = NSAlertStyleWarning;

        [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSAlertFirstButtonReturn) {
                self.close_after_writing = YES;
                [self applyTrackPropertiesAction:alert];
            }
            else if (returnCode == NSAlertSecondButtonReturn){
                self.isModified = NO;
                [self.window close];
            }
        }];

        return NO;
    }
    return YES;
}

// FIXME: move to its own windowcontroller
- (IBAction)configureTagWritingAction:(id)sender {
    // init values in the tag writer settings
    // tag writer
    int strip_id3v2 = deadbeef->conf_get_int ("mp3.strip_id3v2", 0);
    int strip_id3v1 = deadbeef->conf_get_int ("mp3.strip_id3v1", 0);
    int strip_apev2 = deadbeef->conf_get_int ("mp3.strip_apev2", 0);
    int write_id3v2 = deadbeef->conf_get_int ("mp3.write_id3v2", 1);
    int write_id3v1 = deadbeef->conf_get_int ("mp3.write_id3v1", 1);
    int write_apev2 = deadbeef->conf_get_int ("mp3.write_apev2", 0);
    int id3v2_version = deadbeef->conf_get_int ("mp3.id3v2_version", 3);
    if (id3v2_version < 3 || id3v2_version > 4) {
        id3v2_version = 3;
    }
    char id3v1_encoding[50];
    deadbeef->conf_get_str ("mp3.id3v1_encoding", "iso8859-1", id3v1_encoding, sizeof (id3v1_encoding));
    int ape_strip_id3v2 = deadbeef->conf_get_int ("ape.strip_id3v2", 0);
    int ape_strip_apev2 = deadbeef->conf_get_int ("ape.strip_apev2", 0);
    int ape_write_id3v2 = deadbeef->conf_get_int ("ape.write_id3v2", 0);
    int ape_write_apev2 = deadbeef->conf_get_int ("ape.write_apev2", 1);
    int wv_strip_apev2 = deadbeef->conf_get_int ("wv.strip_apev2", 0);
    int wv_strip_id3v1 = deadbeef->conf_get_int ("wv.strip_id3v1", 0);
    int wv_write_apev2 = deadbeef->conf_get_int ("wv.write_apev2", 1);
    int wv_write_id3v1 = deadbeef->conf_get_int ("wv.write_id3v1", 0);

    self.mp3WriteID3v2.state = write_id3v2;
    self.mp3WriteID3v1.state = write_id3v1;
    self.mp3WriteAPEv2.state = write_apev2;
    self.mp3StripID3v2.state = strip_id3v2;
    self.mp3StripID3v1.state = strip_id3v1;
    self.mp3StripAPEv2.state = strip_apev2;
    [self.mp3ID3v2Version selectItemAtIndex:id3v2_version-3];
    self.mp3ID3v1Charset.stringValue = @(id3v1_encoding);
    self.apeWriteID3v2.state = ape_write_id3v2;
    self.apeWriteAPEv2.state = ape_write_apev2;
    self.apeStripID3v2.state = ape_strip_id3v2;
    self.apeStripAPEv2.state = ape_strip_apev2;
    self.wvWriteAPEv2.state = wv_write_apev2;
    self.wvWriteID3v1.state = wv_write_id3v1;
    self.wvStripAPEv2.state = wv_strip_apev2;
    self.wvStripID3v1.state = wv_strip_id3v1;

    [self.window beginSheet:self.tagWriterSettingsPanel completionHandler:^(NSModalResponse returnCode) {
    }];
}

- (IBAction)reloadTrackPropertiesAction:(id)sender {
    trkproperties_reload_tags (self.tracks, self.numtracks);

    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    [self reloadContent];
}

- (IBAction)cancelTrackPropertiesAction:(id)sender {
    if ([self windowShouldClose:sender]) {
        [self close];
    }
}

- (IBAction)okTrackPropertiesAction:(id)sender {
    self.close_after_writing = YES;
    [self applyTrackPropertiesAction:sender];
}


- (IBAction)tagWriterSettingsCloseAction:(id)sender {
    [NSApp endSheet:self.tagWriterSettingsPanel returnCode:NSModalResponseOK];
}

- (IBAction)mp3WriteID3v2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("mp3.write_id3v2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)mp3WriteID3v1Action:(NSButton *)sender {
    deadbeef->conf_set_int ("mp3.write_id3v1", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)mp3WriteAPEv2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("mp3.write_apev2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)mp3StripID3v2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("mp3.strip_id3v2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)mp3StripID3v1Action:(NSButton *)sender {
    deadbeef->conf_set_int ("mp3.strip_id3v1", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)mp3StripAPEv2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("mp3.strip_apev2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)mp3ID3v2VersionChangeAction:(NSPopUpButton *)sender {
    int ver = (int)sender.indexOfSelectedItem+3;
    deadbeef->conf_set_int ("mp3.id3v2_version", ver);
    deadbeef->conf_save ();
}

- (IBAction)mp3ID3v1CharsetChangeAction:(NSTextField *)sender {
    deadbeef->conf_set_str ("mp3.id3v1_encoding", sender.stringValue.UTF8String);
    deadbeef->conf_save ();
}

- (IBAction)apeWriteID3v2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("ape.write_id3v2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)apeWriteAPEv2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("ape.write_apev2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)apeStripID3v2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("ape.strip_id3v2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)apeStripAPEv2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("ape.strip_apev2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)wvWriteAPEv2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("wv.write_apev2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)wvWriteID3v1Action:(NSButton *)sender {
    deadbeef->conf_set_int ("wv.write_id3v1", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)wvStripAPEv2Action:(NSButton *)sender {
    deadbeef->conf_set_int ("wv.strip_apev2", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)wvStripID3v1Action:(NSButton *)sender {
    deadbeef->conf_set_int ("wv.strip_id3v1", sender.state == NSControlStateValueOn);
    deadbeef->conf_save ();
}

- (IBAction)editValueAction:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;
    if (ind.count != 1) {
        return; // multiple fields can't be edited at the same time
    }

    NSInteger idx = ind.firstIndex;

    if (self.numtracks != 1) {
        // Allow editing the previous value, if all tracks have the same
        BOOL isMult;
        NSString *value = [self fieldValueForIndex:idx store:self.store isMult:&isMult];
        if (!isMult) {
            self.multiValueSingle.string = value;
        }
        else {
            self.multiValueSingle.string = @"";
        }

        NSString *key = self.store[idx][@"key"];
        self.multiValueFieldName.stringValue =  key.uppercaseString;

        NSMutableArray<NSString *> *fields = [NSMutableArray new];
        NSMutableArray<NSString *> *items = [NSMutableArray new];

        deadbeef->pl_lock ();

        char *item_tf = deadbeef->tf_compile ("%title%[ // %track artist%]");

        ddb_tf_context_t ctx;
        memset (&ctx, 0, sizeof (ctx));

        ctx._size = sizeof (ctx);
        ctx.plt = NULL;
        ctx.idx = -1;
        ctx.id = -1;

        fields = self.store[idx][@"values"];

        for (int i = 0; i < self.numtracks; i++) {
            char item[1000];
            ctx.it = self.tracks[i];
            deadbeef->tf_eval(&ctx, item_tf, item, sizeof (item));
            [items addObject:@(item)];
        }
        deadbeef->pl_unlock ();
        deadbeef->tf_free (item_tf);

        self.multipleFieldsTableData = [TrackPropertiesMultipleFieldsTableData new];
        self.multipleFieldsTableData.fields = [[NSMutableArray alloc] initWithArray:fields copyItems:NO];
        self.multipleFieldsTableData.items = items;
        self.multiValueTableView.delegate = self.multipleFieldsTableData;
        self.multiValueTableView.dataSource = self.multipleFieldsTableData;
        [self.window beginSheet:self.editMultipleValuesPanel completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSModalResponseOK) {
                if ([(self.multiValueTabView).selectedTabViewItem.identifier isEqualToString:@"singleValue"]) {
                    [self setSameValuesForIndex:(int)idx value:(self.multiValueSingle).textStorage.string];
                }
                else {
                    for (int i = 0; i < self.numtracks; i++) {
                        self.store[idx][@"values"] = [[NSMutableArray alloc] initWithArray:self.multipleFieldsTableData.fields copyItems:NO];
                    }
                }
                self.isModified = YES;
            }
        }];
        return;
    }

    self.fieldName.stringValue =  ((NSString *)self.store[idx][@"key"]).uppercaseString;
    self.fieldValue.string =  self.store[idx][@"values"][0];

    [self.window beginSheet:self.editValuePanel completionHandler:^(NSModalResponse returnCode) {
        self.isModified = YES;
    }];
}

- (IBAction)editInPlaceAction:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;
    if (ind.count != 1) {
        return; // multiple fields can't be edited at the same time
    }

    NSInteger idx = ind.firstIndex;

    [self.metadataTableView editColumn:1 row:idx withEvent:nil select:YES];
}

- (IBAction)cancelEditValuePanelAction:(id)sender {
    [NSApp endSheet:self.editValuePanel];
}

- (IBAction)okEditValuePanelAction:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;
    NSInteger idx = ind.firstIndex;
    if (![self.store[idx][@"values"][0] isEqualToString:(self.fieldValue).string]) {
        self.store[idx][@"values"][0] = (self.fieldValue).string;
        [self.metadataTableView reloadData];
        self.isModified = YES;
    }

    [NSApp endSheet:self.editValuePanel];
}

- (void)setSameValuesForIndex:(NSUInteger)idx value:(NSString *)value {
    NSMutableArray<NSString *> *values = self.store[idx][@"values"];
    for (NSUInteger i = 0; i < values.count; i++) {
        values[i] = value;
    }
}

- (IBAction)delete:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;

    [ind enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        [self setSameValuesForIndex:(int)idx value:@""];
        self.isModified = YES;
    }];

    if (self.isModified) {
        [self.metadataTableView reloadData];
    }
}

- (IBAction)editCropAction:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;

    for (NSUInteger i = 0; i < self.store.count; i++) {
        if (![ind containsIndex:i]) {
            [self setSameValuesForIndex:i value:@""];
            self.isModified = YES;
        }
    }

    if (self.isModified) {
        [self.metadataTableView reloadData];
    }
}

- (IBAction)editCapitalizeAction:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;

    for (NSUInteger i = 0; i < self.store.count; i++) {
        if ([ind containsIndex:i]) {
            NSMutableArray<NSString *> *values = self.store[i][@"values"];
            for (NSUInteger n = 0; n < values.count; n++) {
                values[n] =  values[n].uppercaseString;
            }
            self.isModified = YES;
        }
    }

    if (self.isModified) {
        [self.metadataTableView reloadData];
    }
}

- (IBAction)addNewField:(id)sender {
    self.addFieldName.stringValue =  @"";
    self.addFieldAlreadyExists.hidden =  YES;

    [self.window beginSheet:self.addFieldPanel completionHandler:^(NSModalResponse returnCode) {
        if (returnCode != NSModalResponseOK) {
            return;
        }
        NSString *key = self.addFieldName.stringValue;
        for (NSUInteger i = 0; i < self.store.count; i++) {
            if (NSOrderedSame == [key caseInsensitiveCompare:self.store[i][@"key"]]) {
                self.addFieldAlreadyExists.hidden =  NO;
                return;
            }
        }

        char *title = _formatted_title_for_unknown_key(key.UTF8String);
        add_field (self.store, key.UTF8String, title, 0, self.tracks, self.numtracks);
        free (title);
        title = NULL;
        self.isModified = YES;
        [self.metadataTableView reloadData];
    }];
}

- (IBAction)cancelAddFieldPanelAction:(id)sender {
    [self.window endSheet:self.addFieldPanel returnCode:NSModalResponseCancel];
}

- (IBAction)okAddFieldPanelAction:(id)sender {
    [self.window endSheet:self.addFieldPanel returnCode:NSModalResponseOK];
}

- (IBAction)cancelEditMultipleValuesPanel:(id)sender {
    [self.window endSheet:self.editMultipleValuesPanel returnCode:NSModalResponseCancel];
}

- (IBAction)okEditMultipleValuesAction:(id)sender {
    NSIndexSet *ind = self.metadataTableView.selectedRowIndexes;
    NSInteger idx = ind.firstIndex;

    self.isModified = YES;

    self.store[idx][@"values"] = [[NSMutableArray alloc] initWithArray: self.multipleFieldsTableData.fields copyItems:NO];

    [self.metadataTableView reloadData];

    [self.window endSheet:self.editMultipleValuesPanel returnCode:NSModalResponseOK];
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return 0;
    }

    return (NSInteger)store.count;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return nil;
    }

    if ([aTableColumn.identifier isEqualToString:@"name"]) {
        NSString *title = store[rowIndex][@"title"];
        return title;
    }
    else if ([aTableColumn.identifier isEqualToString:@"value"]) {
        return [self fieldValueForIndex:rowIndex store:store isMult:NULL];
    }
    return nil;
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return;
    }

    NSMutableDictionary *dict = store[rowIndex];

    NSMutableArray<NSString *> *values = dict[@"values"];
    for (NSUInteger i = 0; i < values.count; i++) {
        if ([values[i] isNotEqualTo:anObject]) {
            values[i] = anObject;
            self.isModified = YES;
        }
    }
}

#pragma mark - NSTableViewDelegate

// when editing the "multiple values" cells, turn them into ""
// this, unfortunately, is not undoable, so as soon as the user starts editing -- no way back
- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return;
    }

    if([aTableColumn.identifier isEqualToString:@"value"]){
        ((NSTextFieldCell *)aCell).formatter = [TrackPropertiesSingleLineFormatter new];
    }
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    self.singleValueSelected = self.metadataTableView.selectedRowIndexes.count == 1;
}

@end
