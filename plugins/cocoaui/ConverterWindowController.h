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

@interface ConverterWindowController : NSWindowController<NSWindowDelegate,NSTableViewDataSource,NSTableViewDelegate>

@property (unsafe_unretained) IBOutlet NSTextField *outputFolder;
@property (unsafe_unretained) IBOutlet NSButton *writeToSourceFolder;
@property (unsafe_unretained) IBOutlet NSButton *preserveFolderStructure;
@property (unsafe_unretained) IBOutlet NSButton *bypassSameFormat;
@property (weak) IBOutlet NSButton *retagAfterCopy;
@property (unsafe_unretained) IBOutlet NSTextField *outputFileName;
@property (unsafe_unretained) IBOutlet NSArrayController *filenamePreviewController;
@property (unsafe_unretained) IBOutlet NSPopUpButton *encoderPreset;
@property (unsafe_unretained) IBOutlet NSPopUpButton *outputFormat;
@property (unsafe_unretained) IBOutlet NSPopUpButton *fileExistsAction;
- (IBAction)cancelAction:(id)sender;
- (IBAction)okAction:(id)sender;

- (IBAction)openOutputFolderAction:(id)sender;
- (IBAction)editEncoderPresetsAction:(id)sender;

- (IBAction)outputFolderChanged:(id)sender;
- (IBAction)writeToSourceFolderChanged:(id)sender;
- (IBAction)preserveFolderStructureChanged:(id)sender;
- (IBAction)bypassSameFormatChanged:(id)sender;
- (IBAction)retagAfterCopyChanged:(id)sender;
- (IBAction)outputPathChanged:(id)sender;
- (IBAction)encoderPresetChanged:(id)sender;
- (IBAction)overwritePromptChanged:(id)sender;
- (IBAction)outputFormatChanged:(id)sender;



@property (strong) IBOutlet NSPanel *encoderPresetsPanel;
- (IBAction)closeEncoderPresetsAction:(id)sender;
- (IBAction)addEncoderPresetAction:(id)sender;
- (IBAction)removeEncoderPresetAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSTableView *encoderPresetsTableView;
@property (unsafe_unretained) IBOutlet NSTextField *encoderPresetOutputFileExtension;
@property (unsafe_unretained) IBOutlet NSTextField *encoderPresetCommandLine;
@property (unsafe_unretained) IBOutlet NSPopUpButton *encoderPresetExecutionMethod;
@property (unsafe_unretained) IBOutlet NSPopUpButton *encoderPresetID3v2TagVersion;
@property (unsafe_unretained) IBOutlet NSButton *encoderPresetApeTag;
@property (unsafe_unretained) IBOutlet NSButton *encoderPresetFlacTag;
@property (unsafe_unretained) IBOutlet NSButton *encoderPresetOggVorbisTag;
@property (unsafe_unretained) IBOutlet NSButton *encoderPresetID3v1Tag;
@property (unsafe_unretained) IBOutlet NSButton *encoderPresetID3v2Tag;
@property (unsafe_unretained) IBOutlet NSButton *encoderPresetMP4Tag;

- (IBAction)encoderPresetOutputFileExtensionChangedAction:(id)sender;
- (IBAction)encoderPresetCommandLineChangedAction:(id)sender;
- (IBAction)encoderPresetExecutionMethodChangedAction:(id)sender;
- (IBAction)encoderPresetID3v2TagVersionChangedAction:(id)sender;
- (IBAction)encoderPresetApeTagChangedAction:(id)sender;
- (IBAction)encoderPresetFlacTagChangedAction:(id)sender;
- (IBAction)encoderPresetOggVorbisTagChangedAction:(id)sender;
- (IBAction)encoderPresetID3v1TagChangedAction:(id)sender;
- (IBAction)encoderPresetID3v2TagChangedAction:(id)sender;
- (IBAction)encoderPresetMP4TagChangedAction:(id)sender;



@property (strong) IBOutlet NSPanel *progressPanel;
@property (unsafe_unretained) IBOutlet NSTextField *progressText;
@property (unsafe_unretained) IBOutlet NSTextField *progressOutText;
@property (unsafe_unretained) IBOutlet NSTextField *progressNumeric;

@property (unsafe_unretained) IBOutlet NSProgressIndicator *progressBar;
- (IBAction)progressCancelAction:(id)sender;

// ctx is one of the DDB_ACTION_CTX_ constants
- (void)run:(int)ctx;

+ (void)runConverter:(int)ctx;

+ (void)cleanup;

@end
