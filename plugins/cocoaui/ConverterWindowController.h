//
//  ConverterWindowController.h
//  deadbeef
//
//  Created by waker on 16/06/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ConverterWindowController : NSWindowController
@property (unsafe_unretained) IBOutlet NSTextField *outputFolder;
@property (unsafe_unretained) IBOutlet NSButton *writeToSourceFolder;
@property (unsafe_unretained) IBOutlet NSButton *preserveFolderStructure;
@property (unsafe_unretained) IBOutlet NSTextField *outputFileName;
@property (unsafe_unretained) IBOutlet NSPopUpButton *encoderPreset;
@property (unsafe_unretained) IBOutlet NSPopUpButton *dspPreset;
@property (unsafe_unretained) IBOutlet NSPopUpButton *outputFormat;
@property (unsafe_unretained) IBOutlet NSPopUpButton *fileExistsAction;
- (IBAction)cancelAction:(id)sender;
- (IBAction)okAction:(id)sender;
- (IBAction)openOutputFolderAction:(id)sender;
- (IBAction)editEncoderPresetsAction:(id)sender;
- (IBAction)editDSPPresetsAction:(id)sender;

- (IBAction)outputFolderChanged:(id)sender;
- (IBAction)writeToSourceFolderChanged:(id)sender;
- (IBAction)preserveFolderStructureChanged:(id)sender;
- (IBAction)outputPathChanged:(id)sender;
- (IBAction)encoderPresetChanged:(id)sender;
- (IBAction)dspPresetChanged:(id)sender;
- (IBAction)overwritePromptChanged:(id)sender;



@property (strong) IBOutlet NSPanel *encoderPresetsPanel;
- (IBAction)closeEncoderPresetsAction:(id)sender;
@property (unsafe_unretained) IBOutlet NSTableView *encoderPresetsTableView;
- (IBAction)addEncoderPresetAction:(id)sender;
- (IBAction)removeEncoderPresetAction:(id)sender;


@property (strong) IBOutlet NSPanel *dspPresetsPanel;
- (IBAction)closeDSPPresetsAction:(id)sender;

@property (strong) IBOutlet NSPanel *progressPanel;
@property (unsafe_unretained) IBOutlet NSTextField *progressText;
@property (unsafe_unretained) IBOutlet NSProgressIndicator *progressBar;
- (IBAction)progressCancelAction:(id)sender;

// ctx is one of the DDB_ACTION_CTX_ constants
- (void)run:(int)ctx;

@end
