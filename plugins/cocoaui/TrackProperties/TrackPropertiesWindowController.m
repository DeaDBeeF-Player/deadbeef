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

#import "AddNewFieldWindowController.h"
#import "EditSingleValueWindowController.h"
#import "EditMultipleValuesWindowController.h"
#import "MediaLibraryItem.h"
#import "TrackPropertiesListViewController.h"
#import "TrackPropertiesMultipleFieldsTableData.h"
#import "TrackPropertiesNullFormatter.h"
#import "TrackPropertiesSingleLineFormatter.h"
#import "TrackPropertiesWindowController.h"
#include <deadbeef/deadbeef.h>
#include "utf8.h"
#include "trkproperties_shared.h"

extern DB_functions_t *deadbeef;

@interface TrackPropertiesWindowController ()

@property (nonatomic) BOOL progress_aborted;
@property (nonatomic) BOOL close_after_writing;

@property (nonatomic) TrackPropertiesListViewController *metadataListViewController;
@property (nonatomic) TrackPropertiesListViewController *propertiesListViewController;

@property (weak) IBOutlet NSView *metadataTableContainerView;
@property (weak) IBOutlet NSView *propertiesTableContainerView;

@property (nonatomic) ddb_playlist_t *playlist;
@property (nonatomic) ddb_action_context_t context;
@property (nonatomic) NSArray<MediaLibraryItem *> *mediaLibraryItems;

// trkproperties window
@property (unsafe_unretained) IBOutlet NSTextField *filename;
- (IBAction)applyTrackPropertiesAction:(id)sender;
- (IBAction)configureTagWritingAction:(id)sender;
- (IBAction)reloadTrackPropertiesAction:(id)sender;
- (IBAction)cancelTrackPropertiesAction:(id)sender;
- (IBAction)okTrackPropertiesAction:(id)sender;

// metadata writing progress dialog
@property (strong) IBOutlet NSPanel *progressPanel;
@property (unsafe_unretained) IBOutlet NSTextField *currentTrackPath;
- (IBAction)cancelWritingAction:(id)sender;

// tag writer settings
@property (strong) IBOutlet NSPanel *tagWriterSettingsPanel;
- (IBAction)tagWriterSettingsCloseAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSButton *mp3WriteID3v2;
@property (unsafe_unretained) IBOutlet NSButton *mp3WriteID3v1;
@property (unsafe_unretained) IBOutlet NSButton *mp3WriteAPEv2;
@property (unsafe_unretained) IBOutlet NSButton *mp3StripID3v2;
@property (unsafe_unretained) IBOutlet NSButton *mp3StripID3v1;
@property (unsafe_unretained) IBOutlet NSButton *mp3StripAPEv2;
@property (unsafe_unretained) IBOutlet NSPopUpButton *mp3ID3v2Version;
@property (unsafe_unretained) IBOutlet NSTextField *mp3ID3v1Charset;

- (IBAction)mp3WriteID3v2Action:(id)sender;
- (IBAction)mp3WriteID3v1Action:(id)sender;
- (IBAction)mp3WriteAPEv2Action:(id)sender;
- (IBAction)mp3StripID3v2Action:(id)sender;
- (IBAction)mp3StripID3v1Action:(id)sender;
- (IBAction)mp3StripAPEv2Action:(id)sender;
- (IBAction)mp3ID3v2VersionChangeAction:(id)sender;
- (IBAction)mp3ID3v1CharsetChangeAction:(id)sender;


@property (unsafe_unretained) IBOutlet NSButton *apeWriteID3v2;
@property (unsafe_unretained) IBOutlet NSButton *apeWriteAPEv2;
@property (unsafe_unretained) IBOutlet NSButton *apeStripID3v2;
@property (unsafe_unretained) IBOutlet NSButton *apeStripAPEv2;

- (IBAction)apeWriteID3v2Action:(id)sender;
- (IBAction)apeWriteAPEv2Action:(id)sender;
- (IBAction)apeStripID3v2Action:(id)sender;
- (IBAction)apeStripAPEv2Action:(id)sender;

@property (unsafe_unretained) IBOutlet NSButton *wvWriteAPEv2;
@property (unsafe_unretained) IBOutlet NSButton *wvWriteID3v1;
@property (unsafe_unretained) IBOutlet NSButton *wvStripAPEv2;
@property (unsafe_unretained) IBOutlet NSButton *wvStripID3v1;

- (IBAction)wvWriteAPEv2Action:(id)sender;
- (IBAction)wvWriteID3v1Action:(id)sender;
- (IBAction)wvStripAPEv2Action:(id)sender;
- (IBAction)wvStripID3v1Action:(id)sender;

@end

@implementation TrackPropertiesWindowController

- (void)setPlaylist:(ddb_playlist_t *)plt context:(ddb_action_context_t)context{
    if (_playlist) {
        deadbeef->plt_unref (_playlist);
    }
    _playlist = plt;
    if (_playlist) {
        deadbeef->plt_ref (_playlist);
    }
    _context = context;

    [self reloadContent];
}

- (void)setMediaLibraryItems:(NSArray<MediaLibraryItem *> *)mediaLibraryItems {
    _mediaLibraryItems = mediaLibraryItems;
    [self reloadContent];
}


- (void)windowDidLoad {
    [super windowDidLoad];
    self.window.delegate = self;

    self.metadataListViewController = [TrackPropertiesListViewController new];
    self.propertiesListViewController = [TrackPropertiesListViewController new];

    self.metadataListViewController.view.translatesAutoresizingMaskIntoConstraints = NO;
    self.propertiesListViewController.view.translatesAutoresizingMaskIntoConstraints = NO;

    [self.metadataTableContainerView addSubview:self.metadataListViewController.view];
    [self.propertiesTableContainerView addSubview:self.propertiesListViewController.view];

    [NSLayoutConstraint activateConstraints:@[
        [self.metadataListViewController.view.leadingAnchor constraintEqualToAnchor:self.metadataTableContainerView.leadingAnchor],
        [self.metadataListViewController.view.trailingAnchor constraintEqualToAnchor:self.metadataTableContainerView.trailingAnchor],
        [self.metadataListViewController.view.topAnchor constraintEqualToAnchor:self.metadataTableContainerView.topAnchor],
        [self.metadataListViewController.view.bottomAnchor constraintEqualToAnchor:self.metadataTableContainerView.bottomAnchor],
        [self.propertiesListViewController.view.leadingAnchor constraintEqualToAnchor:self.propertiesTableContainerView.leadingAnchor],
        [self.propertiesListViewController.view.trailingAnchor constraintEqualToAnchor:self.propertiesTableContainerView.trailingAnchor],
        [self.propertiesListViewController.view.topAnchor constraintEqualToAnchor:self.propertiesTableContainerView.topAnchor],
        [self.propertiesListViewController.view.bottomAnchor constraintEqualToAnchor:self.propertiesTableContainerView.bottomAnchor],
    ]];
}

- (void)reloadContent {
    if (!self.window) {
        return;
    }
    self.close_after_writing = NO;

    if (self.playlist) {
        [self.metadataListViewController loadFromPlaylist:self.playlist context:self.context flags:TrackPropertiesListFlagMetadata];
        [self.propertiesListViewController loadFromPlaylist:self.playlist context:self.context flags:TrackPropertiesListFlagProperties];
    }
    else if (self.mediaLibraryItems) {
        [self.metadataListViewController loadFromMediaLibraryItems:self.mediaLibraryItems flags:TrackPropertiesListFlagMetadata];
        [self.propertiesListViewController loadFromMediaLibraryItems:self.mediaLibraryItems flags:TrackPropertiesListFlagProperties];
    }

    NSString *fname;

    if (self.metadataListViewController.numtracks == 1) {
        deadbeef->pl_lock ();
        fname = @(deadbeef->pl_find_meta_raw (self.metadataListViewController.tracks[0], ":URI"));

        deadbeef->pl_unlock ();
    }
    else if (self.metadataListViewController.numtracks != 0) {
        fname = @"[Multiple values]";
    }
    else {
        fname = @"[Nothing selected]";
    }

    if (self.filename) {
        self.filename.stringValue = fname;
    }
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

- (void)setMetadataForSelectedTracks:(TrackPropertiesListItem *)dict {
    NSString *key = dict.key;
    const char *skey = key.UTF8String;
    NSMutableArray<NSString *> *values = dict.values;

    for (int i = 0; i < self.metadataListViewController.numtracks; i++) {
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

        deadbeef->pl_delete_meta (self.metadataListViewController.tracks[i], skey);
        for (NSString *val in transformedValues) {
            if (val.length) {
                deadbeef->pl_append_meta (self.metadataListViewController.tracks[i], skey, val.UTF8String);
            }
        }
    }
}

- (void)writeMetaWorker {
    NSMutableSet *fileset = [NSMutableSet new];
    for (int t = 0; t < self.metadataListViewController.numtracks; t++) {
        if (self.progress_aborted) {
            break;
        }
        DB_playItem_t *track = self.metadataListViewController.tracks[t];
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
        self.metadataListViewController.isModified = NO;
        if (self.close_after_writing) {
            [self.window close];
        }

        [self.delegate trackPropertiesWindowControllerDidUpdateTracks:self];
    });
}

- (IBAction)applyTrackPropertiesAction:(id)sender {
    if (!self.metadataListViewController.isModified) {
        if (self.close_after_writing) {
            [self close];
        }
        return;
    }
    deadbeef->pl_lock ();
    NSMutableArray<TrackPropertiesListItem *> *store = self.metadataListViewController.store;

    // delete all metadata properties that are not in the listview
    for (int i = 0; i < self.metadataListViewController.numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (self.metadataListViewController.tracks[i]);
        while (meta) {
            DB_metaInfo_t *next = meta->next;
            if (meta->key[0] != ':' && meta->key[0] != '!' && meta->key[0] != '_') {
                TrackPropertiesListItem *item;
                for (item in store) {
                    if (!strcasecmp (item.key.UTF8String, meta->key)) {
                        // field found, don't delete
                        break;
                    }
                }

                if (item == NULL) {
                    // field not found, delete
                    deadbeef->pl_delete_metadata (self.metadataListViewController.tracks[i], meta);
                }
            }
            meta = next;
        }
    }
    // put all metainfo into track
    for (TrackPropertiesListItem *item in store) {
        [self setMetadataForSelectedTracks:item];
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
    if (self.metadataListViewController.isModified) {
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
                self.metadataListViewController.isModified = NO;
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
    trkproperties_reload_tags (self.metadataListViewController.tracks, self.metadataListViewController.numtracks);

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

@end
