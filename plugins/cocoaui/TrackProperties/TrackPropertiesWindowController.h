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
#import <Cocoa/Cocoa.h>
#include <deadbeef/deadbeef.h>

@class MediaLibraryItem;
@class TrackPropertiesWindowController;

@protocol TrackPropertiesWindowControllerDelegate

- (void)trackPropertiesWindowControllerDidUpdateTracks:(TrackPropertiesWindowController *)windowController;

@end

@interface TrackPropertiesWindowController : NSWindowController<NSWindowDelegate,NSTableViewDelegate,NSTableViewDataSource>

@property (nonatomic,weak) id<TrackPropertiesWindowControllerDelegate> delegate;

@property (nonatomic) ddb_playlist_t *playlist;
@property (nonatomic) ddb_action_context_t context;
@property (nonatomic) NSArray<MediaLibraryItem *> *mediaLibraryItems;

@property (unsafe_unretained) BOOL isModified;

// trkproperties window
@property (unsafe_unretained) IBOutlet NSTableView *metadataTableView;
@property (unsafe_unretained) IBOutlet NSTableView *propertiesTableView;
@property (unsafe_unretained) IBOutlet NSTextField *filename;
- (IBAction)applyTrackPropertiesAction:(id)sender;
- (IBAction)configureTagWritingAction:(id)sender;
- (IBAction)reloadTrackPropertiesAction:(id)sender;
- (IBAction)cancelTrackPropertiesAction:(id)sender;
- (IBAction)okTrackPropertiesAction:(id)sender;

// edit value panel
@property (strong) IBOutlet NSPanel *editValuePanel;
@property (unsafe_unretained) IBOutlet NSTextField *fieldName;

- (IBAction)cancelEditValuePanelAction:(id)sender;
- (IBAction)okEditValuePanelAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSTextView *fieldValue;

// edit multiple values panel
@property (strong) IBOutlet NSPanel *editMultipleValuesPanel;
- (IBAction)cancelEditMultipleValuesPanel:(id)sender;
@property (weak) IBOutlet NSTextField *multiValueFieldName;
- (IBAction)okEditMultipleValuesAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSTextView *multiValueSingle;
@property (weak) IBOutlet NSTableView *multiValueTableView;
@property (weak) IBOutlet NSTabView *multiValueTabView;

// menu
- (IBAction)editValueAction:(id)sender;
- (IBAction)editInPlaceAction:(id)sender;
- (IBAction)editCropAction:(id)sender;
- (IBAction)editCapitalizeAction:(id)sender;
- (IBAction)addNewField:(id)sender;

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
