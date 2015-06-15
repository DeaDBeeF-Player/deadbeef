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
#import <Cocoa/Cocoa.h>

@interface TrackPropertiesWindowController : NSWindowController

- (void)fill;

// trkproperties window
@property (unsafe_unretained) IBOutlet NSTableView *metadataTableView;
@property (unsafe_unretained) IBOutlet NSTableView *propertiesTableView;
@property (unsafe_unretained) IBOutlet NSTextField *filename;
- (IBAction)applyTrackPropertiesAction:(id)sender;
- (IBAction)configureTagWritingAction:(id)sender;

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
