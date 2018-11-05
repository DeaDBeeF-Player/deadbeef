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
#include "../../deadbeef.h"
#include "../../utf8.h"
#include "../../shared/trkproperties_shared.h"

// Max length of a string displayed in the TableView
// If a string is longer -- it gets clipped, and appended with " (…)", like with linebreaks
#define MAX_GUI_FIELD_LEN 500


extern DB_functions_t *deadbeef;

@interface SingleLineFormatter : NSFormatter
@end

@implementation SingleLineFormatter
- (NSString *)stringForObjectValue:(id)anObject {
    if ([anObject isKindOfClass:[NSString class]]) {
        NSString *str = anObject;
        NSRange range = [str rangeOfString:@"\n"];
        if ([str length] >= MAX_GUI_FIELD_LEN && (range.location == NSNotFound || range.location >= MAX_GUI_FIELD_LEN)) {
            range.location = MAX_GUI_FIELD_LEN;
        }
        if (range.location != NSNotFound ) {
            return [[str substringToIndex:range.location-1] stringByAppendingString:@" (…)"];
        }
        else {
            return str;
        }
    }
    return @"";
}

- (NSString *)editingStringForObjectValue:(id)anObject {
    return anObject;
}

- (BOOL)getObjectValue:(out id *)anObject
             forString:(NSString *)string
      errorDescription:(out NSString **)error {
    *anObject = string;
    return YES;
}
@end

@interface NullFormatter : NSFormatter
@end

@implementation NullFormatter

- (NSString *)stringForObjectValue:(id)anObject {
    return anObject;
}

- (NSString *)editingStringForObjectValue:(id)anObject {
    return @"";
}

- (BOOL)getObjectValue:(out id *)anObject
             forString:(NSString *)string
      errorDescription:(out NSString **)error {
    *anObject = string;
    return YES;
}
@end

@interface MultipleFieldsTableData : NSObject<NSTableViewDataSource, NSTableViewDelegate> {
    @public NSArray<NSString *> *_items;
    @public NSMutableArray<NSString *> *_fields;
}
@end

@implementation MultipleFieldsTableData
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [_fields count];
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSUserInterfaceItemIdentifier ident = [tableColumn identifier];
    NSTableCellView *view = [tableView makeViewWithIdentifier:ident owner:self];
    NSTextField *textView = [view textField];
    if ([ident isEqualToString:@"Index"]) {
        [textView setStringValue:[NSString stringWithFormat:@"%d", (int)row+1]];
    }
    else if ([ident isEqualToString:@"Item"]) {
        [textView setStringValue:_items[row]];
    }
    else if ([ident isEqualToString:@"Field"]) {
        [textView setFormatter:[[SingleLineFormatter alloc] init]];
        [textView setStringValue:_fields[row]];
        [textView setTarget:self];
        [textView setAction:@selector(fieldEditedAction:)];
        [textView setIdentifier:[NSString stringWithFormat:@"%d", (int)row]];
    }
    return view;
}

- (void)fieldEditedAction:(NSTextField *)sender {
    NSString *value = [sender stringValue];
    int row = [[sender identifier] intValue];
    _fields[row] = value;
    // FIXME: add modified flag
}
@end


@interface TrackPropertiesWindowController () {
    int _iter;
    ddb_playlist_t *_last_plt;
    DB_playItem_t **_tracks;
    int _numtracks;
    NSMutableArray *_store;
    NSMutableArray *_propstore;
    BOOL _progress_aborted;
    BOOL _close_after_writing;
    MultipleFieldsTableData *_multipleFieldsTableData;
}

@end

@implementation TrackPropertiesWindowController

- (void)setPlaylist:(ddb_playlist_t *)plt {
    if (_last_plt) {
        deadbeef->plt_unref (_last_plt);
    }
    _last_plt = plt;
    if (_last_plt) {
        deadbeef->plt_ref (_last_plt);
    }
}

- (void)freeTrackList {
    trkproperties_free_track_list (&_tracks, &_numtracks);
}

- (void)dealloc {
    [self freeTrackList];
    if (_last_plt) {
        deadbeef->plt_unref (_last_plt);
        _last_plt = NULL;
    }
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
    [_metadataTableView setDelegate:(id<NSTableViewDelegate>)self];
    [_metadataTableView reloadData];
    [_propertiesTableView reloadData];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    _singleValueSelected = [[_metadataTableView selectedRowIndexes] count] == 1;
}


- (void)buildTrackListForCtx:(int)ctx forPlaylist:(ddb_playlist_t *)plt {
    trkproperties_build_track_list_for_ctx (plt, ctx, &_tracks, &_numtracks);
}


#define min(x,y) ((x)<(y)?(x):(y))

// NOTE: add_field gets called once for each unique key (e.g. Artist or Album),
// which means it will usually contain 10-20 fields
static void
add_field (NSMutableArray *store, const char *key, const char *title, int is_prop, DB_playItem_t **tracks, int numtracks) {

    // get all values for each key, convert from 0-separated to '; '-separated, and put into NSArray
    NSMutableArray<NSString *> *values = [[NSMutableArray alloc] init];
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
                value = [value stringByAppendingString:[NSString stringWithUTF8String:p]];
                p += strlen (p) + 1;
                if (p < end) {
                    value = [value stringByAppendingString:@";"];
                }
            }
        }
        [values addObject:value];
    }
    deadbeef->pl_unlock ();

    [store addObject:[NSMutableDictionary dictionaryWithObjectsAndKeys:[NSString stringWithUTF8String:title], @"title", [NSString stringWithUTF8String:key], @"key", values, @"values", nil]];
}


- (void)fillMeta {
    [_store removeAllObjects];
    if (!_tracks) {
        return;
    }

    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 0, _tracks, _numtracks);

    // add "standard" fields
    for (int i = 0; trkproperties_types[i]; i += 2) {
        add_field (_store, trkproperties_types[i], trkproperties_types[i+1], 0, _tracks, _numtracks);
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

        size_t l = strlen (keys[k]);
        char title[l + 3];
        snprintf (title, sizeof (title), "<%s>", keys[k]);
        add_field (_store, keys[k], title, 0, _tracks, _numtracks);
    }
    if (keys) {
        free (keys);
    }
}

- (void)fillMetadata {
    self.modified = NO;

    deadbeef->pl_lock ();

    [self fillMeta];
    [_propstore removeAllObjects];

    // hardcoded properties
    for (int i = 0; trkproperties_hc_props[i]; i += 2) {
        add_field (_propstore, trkproperties_hc_props[i], trkproperties_hc_props[i+1], 1, _tracks, _numtracks);
    }
    // properties
    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 1, _tracks, _numtracks);
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
        size_t l = strlen (keys[k]) + 2;
        char title[l];
        snprintf (title, sizeof (title), "<%s>", keys[k]+1);
        add_field (_propstore, keys[k], title, 1, _tracks, _numtracks);
    }
    if (keys) {
        free (keys);
    }

    deadbeef->pl_unlock ();
}

- (void)fill {
    _close_after_writing = NO;

    [self freeTrackList];

    [self buildTrackListForCtx:DDB_ACTION_CTX_SELECTION forPlaylist:_last_plt];

    NSString *fname;

    if (_numtracks == 1) {
        deadbeef->pl_lock ();
        fname = [NSString stringWithUTF8String:deadbeef->pl_find_meta_raw (_tracks[0], ":URI")];

        deadbeef->pl_unlock ();
    }
    else if (_numtracks != 0) {
        fname = @"[Multiple values]";
    }
    else {
        fname = @"[Nothing selected]";
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

// when editing the "multiple values" cells, turn them into ""
// this, unfortunately, is not undoable, so as soon as the user starts editing -- no way back
- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return;
    }

    if([[aTableColumn identifier] isEqualToString:@"value"]){
        [aCell setFormatter:[[SingleLineFormatter alloc] init]];
    }
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    NSMutableArray *store = [self storeForTableView:aTableView];
    if (!store) {
        return nil;
    }

    if ([[aTableColumn identifier] isEqualToString:@"name"]) {
        NSString *title = store[rowIndex][@"title"];
        return title;
    }
    else if ([[aTableColumn identifier] isEqualToString:@"value"]) {
        NSMutableArray<NSString *> *values = store[rowIndex][@"values"];
        // get uniq values
        NSArray *uniq = [[NSOrderedSet orderedSetWithArray:values] array];
        NSInteger n = [uniq count];

        NSString *val = n > 1 ? @"[Multiple Values] " : @"";
        for (int i = 0; i < [uniq count]; i++) {
            val = [val stringByAppendingString:uniq[i]];
            if (i < [uniq count] - 1) {
                val = [val stringByAppendingString:@"; "];
            }
        }

        return val;
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
    for (int i = 0; i < [values count]; i++) {
        if ([values[i] isNotEqualTo:anObject]) {
            values[i] = anObject;
            self.modified = YES;
        }
    }
}

- (void)setMetadataForSelectedTracks:(NSDictionary *)dict {
    const char *skey = [dict[@"key"] UTF8String];
    NSMutableArray<NSString *> *values = dict[@"values"];

    for (int i = 0; i < _numtracks; i++) {
        NSString *value = values[i];
        NSArray *components = [value componentsSeparatedByString:@";"];

        NSMutableArray *transformedValues = [[NSMutableArray alloc] init];
        for (NSString *val in components) {
            NSInteger i = 0;
            while ((i < [val length])
                   && [[NSCharacterSet whitespaceCharacterSet] characterIsMember:[val characterAtIndex:i]]) {
                i++;
            }
            // whitespace-only?
            if (i > 0 && i == [val length]-1) {
                continue;
            }
            [transformedValues addObject: (i == 0 ? val : [val substringFromIndex:i])];
        }

        deadbeef->pl_delete_meta (_tracks[i], skey);
        for (NSString *val in transformedValues) {
            if ([val length]) {
                deadbeef->pl_append_meta (_tracks[i], skey, [val UTF8String]);
            }
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
        [NSApp endSheet:_progressPanel];
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        self.modified = NO;
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
    if (!self.modified) {
        return;
    }
    deadbeef->pl_lock ();
    NSMutableArray *store = [self storeForTableView:_metadataTableView];

    // delete all metadata properties that are not in the listview
    for (int i = 0; i < _numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (_tracks[i]);
        while (meta) {
            DB_metaInfo_t *next = meta->next;
            if (meta->key[0] != ':' && meta->key[0] != '!' && meta->key[0] != '_') {
                NSDictionary *dict;
                for (dict in store) {
                    if (!strcasecmp ([dict[@"key"] UTF8String] , meta->key)) {
                        // field found, don't delete
                        break;
                    }
                }

                if (!dict) {
                    // field not found, delete
                    deadbeef->pl_delete_metadata (_tracks[i], meta);
                }
            }
            meta = next;
        }
    }
    // put all metainfo into track
    for (NSDictionary *dict in store) {
        [self setMetadataForSelectedTracks:dict];
    }
    deadbeef->pl_unlock ();

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

    _progress_aborted = NO;

    [NSApp beginSheet:_progressPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(progressPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];

    dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(aQueue, ^{
        [self writeMetaWorker];
    });
}

- (void)progressPanelDidEnd:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [sheet orderOut:self];
}

- (IBAction)cancelWritingAction:(id)sender {
    _progress_aborted = YES;
}

- (BOOL)windowShouldClose:(id)sender {
    if (self.modified) {
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
        self.modified = NO;
        [[self window] close];
    }
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

    [_mp3WriteID3v2 setState:write_id3v2];
    [_mp3WriteID3v1 setState:write_id3v1];
    [_mp3WriteAPEv2 setState:write_apev2];
    [_mp3StripID3v2 setState:strip_id3v2];
    [_mp3StripID3v1 setState:strip_id3v1];
    [_mp3StripAPEv2 setState:strip_apev2];
    [_mp3ID3v2Version selectItemAtIndex:id3v2_version-3];
    [_mp3ID3v1Charset setStringValue:[NSString stringWithUTF8String:id3v1_encoding]];
    [_apeWriteID3v2 setState:ape_write_id3v2];
    [_apeWriteAPEv2 setState:ape_write_apev2];
    [_apeStripID3v2 setState:ape_strip_id3v2];
    [_apeStripAPEv2 setState:ape_strip_apev2];
    [_wvWriteAPEv2 setState:wv_write_apev2];
    [_wvWriteID3v1 setState:wv_write_id3v1];
    [_wvStripAPEv2 setState:wv_strip_apev2];
    [_wvStripID3v1 setState:wv_strip_id3v1];

    [NSApp beginSheet:_tagWriterSettingsPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndTagWriterSettings:returnCode:contextInfo:) contextInfo:nil];
}

- (IBAction)reloadTrackPropertiesAction:(id)sender {
    trkproperties_reload_tags (_tracks, _numtracks);

    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    [self fill];
}

- (IBAction)cancelTrackPropertiesAction:(id)sender {
    if ([self windowShouldClose:sender]) {
        [self close];
    }
}

- (IBAction)okTrackPropertiesAction:(id)sender {
    [self applyTrackPropertiesAction:sender];
    [self close];
}


- (void)didEndTagWriterSettings:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_tagWriterSettingsPanel orderOut:self];
}

- (IBAction)tagWriterSettingsCloseAction:(id)sender {
    [NSApp endSheet:_tagWriterSettingsPanel returnCode:NSOKButton];
}

- (IBAction)mp3WriteID3v2Action:(id)sender {
    deadbeef->conf_set_int ("mp3.write_id3v2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)mp3WriteID3v1Action:(id)sender {
    deadbeef->conf_set_int ("mp3.write_id3v1", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)mp3WriteAPEv2Action:(id)sender {
    deadbeef->conf_set_int ("mp3.write_apev2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)mp3StripID3v2Action:(id)sender {
    deadbeef->conf_set_int ("mp3.strip_id3v2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)mp3StripID3v1Action:(id)sender {
    deadbeef->conf_set_int ("mp3.strip_id3v1", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)mp3StripAPEv2Action:(id)sender {
    deadbeef->conf_set_int ("mp3.strip_apev2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)mp3ID3v2VersionChangeAction:(id)sender {
    int ver = (int)[sender indexOfSelectedItem]+3;
    deadbeef->conf_set_int ("mp3.id3v2_version", ver);
    deadbeef->conf_save ();
}

- (IBAction)mp3ID3v1CharsetChangeAction:(id)sender {
    deadbeef->conf_set_str ("mp3.id3v1_encoding", [[sender stringValue] UTF8String]);
    deadbeef->conf_save ();
}

- (IBAction)apeWriteID3v2Action:(id)sender {
    deadbeef->conf_set_int ("ape.write_id3v2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)apeWriteAPEv2Action:(id)sender {
    deadbeef->conf_set_int ("ape.write_apev2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)apeStripID3v2Action:(id)sender {
    deadbeef->conf_set_int ("ape.strip_id3v2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)apeStripAPEv2Action:(id)sender {
    deadbeef->conf_set_int ("ape.strip_apev2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)wvWriteAPEv2Action:(id)sender {
    deadbeef->conf_set_int ("wv.write_apev2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)wvWriteID3v1Action:(id)sender {
    deadbeef->conf_set_int ("wv.write_id3v1", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)wvStripAPEv2Action:(id)sender {
    deadbeef->conf_set_int ("wv.strip_apev2", [sender state]);
    deadbeef->conf_save ();
}

- (IBAction)wvStripID3v1Action:(id)sender {
    deadbeef->conf_set_int ("wv.strip_id3v1", [sender state]);
    deadbeef->conf_save ();
}

- (void)didEndEditValuePanel:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    self.modified = YES;
    [_editValuePanel orderOut:self];
}

- (IBAction)editValueAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];
    if ([ind count] != 1) {
        return; // multiple fields can't be edited at the same time
    }

    NSInteger idx = [ind firstIndex];

    if (_numtracks != 1) {
        NSString *key = _store[idx][@"key"];
        [_multiValueFieldName setStringValue: [key uppercaseString]];

        NSMutableArray<NSString *> *fields = [[NSMutableArray alloc] init];
        NSMutableArray<NSString *> *items = [[NSMutableArray alloc] init];

        deadbeef->pl_lock ();

        char *item_tf = deadbeef->tf_compile ("%title%[ // %track artist%]");

        ddb_tf_context_t ctx;
        memset (&ctx, 0, sizeof (ctx));

        ctx._size = sizeof (ctx);
        ctx.plt = NULL;
        ctx.idx = -1;
        ctx.id = -1;

        fields = _store[idx][@"values"];

        for (int i = 0; i < _numtracks; i++) {
            char item[1000];
            ctx.it = _tracks[i];
            deadbeef->tf_eval(&ctx, item_tf, item, sizeof (item));
            [items addObject:[NSString stringWithUTF8String:item]];
        }
        deadbeef->pl_unlock ();
        deadbeef->tf_free (item_tf);

        _multipleFieldsTableData = [[MultipleFieldsTableData alloc] init];
        _multipleFieldsTableData->_fields = [[NSMutableArray alloc] initWithArray:fields copyItems:NO];
        _multipleFieldsTableData->_items = items;
        [_multiValueTableView setDelegate:_multipleFieldsTableData];
        [_multiValueTableView setDataSource:_multipleFieldsTableData];
        [self.window beginSheet:_editMultipleValuesPanel completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSModalResponseOK) {
                if ([[[_multiValueTabView selectedTabViewItem] identifier] isEqualToString:@"singleValue"]) {
                    [self setSameValuesForIndex:(int)idx value:[[_multiValueSingle textStorage] string]];
                }
                else {
                    for (int i = 0; i < _numtracks; i++) {
                        _store[idx][@"values"] = [[NSMutableArray alloc] initWithArray:_multipleFieldsTableData->_fields copyItems:NO];
                    }
                }
                self.modified = YES;
            }
        }];
        return;
    }

    [_fieldName setStringValue: [_store[idx][@"key"] uppercaseString]];
    [_fieldValue setString: _store[idx][@"values"][0]];

    [NSApp beginSheet:_editValuePanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndEditValuePanel:returnCode:contextInfo:) contextInfo:nil];
}

- (IBAction)editInPlaceAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];
    if ([ind count] != 1) {
        return; // multiple fields can't be edited at the same time
    }

    NSInteger idx = [ind firstIndex];

    [_metadataTableView editColumn:1 row:idx withEvent:nil select:YES];
}

- (IBAction)cancelEditValuePanelAction:(id)sender {
    [NSApp endSheet:_editValuePanel];
}

- (IBAction)okEditValuePanelAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];
    NSInteger idx = [ind firstIndex];
    if (![_store[idx][@"values"][0] isEqualToString:[_fieldValue string]]) {
        _store[idx][@"values"][0] = [_fieldValue string];
        [_metadataTableView reloadData];
        self.modified = YES;
    }

    [NSApp endSheet:_editValuePanel];
}

- (void)setSameValuesForIndex:(int)idx value:(NSString *)value {
    NSMutableArray<NSString *> *values = _store[idx][@"values"];
    for (int i = 0; i < [values count]; i++) {
        values[i] = value;
    }
}

- (IBAction)editRemoveAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];

    [ind enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        [self setSameValuesForIndex:(int)idx value:@""];
        self.modified = YES;
    }];

    if (self.modified) {
        [_metadataTableView reloadData];
    }
}

- (IBAction)editCropAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];

    for (int i = 0; i < [_store count]; i++) {
        if (![ind containsIndex:i]) {
            [self setSameValuesForIndex:i value:@""];
            self.modified = YES;
        }
    }

    if (self.modified) {
        [_metadataTableView reloadData];
    }
}

- (IBAction)editCapitalizeAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];

    for (int i = 0; i < [_store count]; i++) {
        if ([ind containsIndex:i]) {
            NSMutableArray<NSString *> *values = _store[i][@"values"];
            for (int n = 0; n < [values count]; n++) {
                values[n] =  [values[n] uppercaseString];
            }
            self.modified = YES;
        }
    }

    if (self.modified) {
        [_metadataTableView reloadData];
    }
}

- (IBAction)addNewField:(id)sender {
    [_addFieldName setStringValue: @""];
    [_addFieldAlreadyExists setHidden: YES];

    [NSApp beginSheet:_addFieldPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndCreateFieldPanel:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndCreateFieldPanel:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    [_addFieldPanel orderOut:self];
}

- (IBAction)cancelAddFieldPanelAction:(id)sender {
    [NSApp endSheet:_addFieldPanel];
}

- (IBAction)okAddFieldPanelAction:(id)sender {
    const char *key = [[_addFieldName stringValue] UTF8String];
    for (int i = 0; i < [_store count]; i++) {
        if (!strcasecmp(key, [_store [i][@"key"] UTF8String])) {
            [_addFieldAlreadyExists setHidden: NO];
            return;
        }
    }

    char title[strlen(key)+3];
    snprintf (title, sizeof (title), "<%s>", key);
    add_field (_store, key, title, 0, _tracks, _numtracks);
    self.modified = YES;
    [_metadataTableView reloadData];
    [NSApp endSheet:_addFieldPanel];

}

- (IBAction)cancelEditMultipleValuesPanel:(id)sender {
    [self.window endSheet:_editMultipleValuesPanel returnCode:NSModalResponseCancel];
}

- (IBAction)okEditMultipleValuesAction:(id)sender {
    NSIndexSet *ind = [_metadataTableView selectedRowIndexes];
    NSInteger idx = [ind firstIndex];

    _modified = YES;

    _store[idx][@"values"] = [[NSMutableArray alloc] initWithArray: _multipleFieldsTableData->_fields copyItems:NO];

    [_metadataTableView reloadData];

    [self.window endSheet:_editMultipleValuesPanel returnCode:NSModalResponseOK];
}

@end
