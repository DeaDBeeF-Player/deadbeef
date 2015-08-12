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
