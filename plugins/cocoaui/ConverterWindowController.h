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
@property (unsafe_unretained) IBOutlet NSButton *editDSPPresetsAction;

@end
