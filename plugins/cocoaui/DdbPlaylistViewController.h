//
//  DdbPlaylistViewController.h
//  deadbeef
//
//  Created by waker on 03/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbListview.h"
#import "TrackPropertiesWindowController.h"
#import "ConverterWindowController.h"

#define PLT_MAX_COLUMNS 100

typedef struct {
    char *title;
    int type; // predefined col type
    char *format;
    int size;
    int alignment;
    int set_text_color;
    uint8_t text_color[4];
    char *bytecode;
} plt_col_info_t;

@interface DdbPlaylistViewController : NSViewController {
    plt_col_info_t _columns[PLT_MAX_COLUMNS];
    int _ncolumns;
    int _menuColumn;
    NSImage *_playTpl;
    NSImage *_pauseTpl;
    NSImage *_bufTpl;
    NSDictionary *_colTextAttrsDictionary;
    NSDictionary *_cellTextAttrsDictionary;
    NSDictionary *_cellSelectedTextAttrsDictionary;
    NSDictionary *_groupTextAttrsDictionary;
    TrackPropertiesWindowController *_trkProperties;
    ConverterWindowController *_converter;
}

- (void)initContent;
- (int)playlistIter;

// playlist columns
@property (unsafe_unretained) IBOutlet NSPanel *addColumnPanel;
- (IBAction)addColumnCancel:(id)sender;
- (IBAction)addColumnOK:(id)sender;

@property (unsafe_unretained) IBOutlet NSTextField *addColumnTitle;
@property (unsafe_unretained) IBOutlet NSPopUpButton *addColumnType;
@property (unsafe_unretained) IBOutlet NSTextField *addColumnFormat;
@property (unsafe_unretained) IBOutlet NSPopUpButton *addColumnAlignment;
@property (unsafe_unretained) IBOutlet NSButton *addColumnSetColor;
@property (unsafe_unretained) IBOutlet NSColorWell *addColumnColor;
- (IBAction)addColumnTypeChanged:(id)sender;
- (IBAction)addColumnSetColorChanged:(id)sender;

- (int)handleListviewMessage:(DdbListview *)listview id:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@end
