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

#define PLT_MAX_COLUMNS 100

typedef struct {
    char *title;
    int _id; // predefined col type
    char *format;
    int size;
    char *bytecode;
    int bytecode_len;
} plt_col_info_t;

@interface DdbPlaylistViewController : NSViewController {
    plt_col_info_t _columns[PLT_MAX_COLUMNS];
    int _ncolumns;
    NSImage *_playTpl;
    NSImage *_pauseTpl;
    NSImage *_bufTpl;
    NSDictionary *_colTextAttrsDictionary;
    NSDictionary *_cellTextAttrsDictionary;
    NSDictionary *_cellSelectedTextAttrsDictionary;
    NSDictionary *_groupTextAttrsDictionary;
    TrackPropertiesWindowController *_trkProperties;
}

- (const char *)defaultColumnConfig;
- (void)initContent;
- (int)playlistIter;

// playlist columns
@property (unsafe_unretained) IBOutlet NSPanel *addColumnPanel;
- (IBAction)addColumnClose:(id)sender;
- (int)handleListviewMessage:(DdbListview *)listview id:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@end
