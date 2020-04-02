/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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

#import "DdbShared.h"
#import "ConverterWindowController.h"
#import "CoverManager.h"
#import "EditColumnWindowController.h"
#import "GroupByCustomWindowController.h"
#import "NSImage+Additions.h"
#import "PlaylistViewController.h"
#import "PlaylistView.h"
#import "ReplayGainScannerController.h"
#import "NSMenu+ActionItems.h"
#import "tftintutil.h"

#include "deadbeef.h"
#include "rg_scanner.h"
#include "utf8.h"

#define CELL_HPADDING 4
#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

extern DB_functions_t *deadbeef;

@interface PlaylistViewController()

@property (nonatomic, strong) NSTimer *playPosUpdateTimer;
@property (nonatomic, assign) DB_playItem_t *playPosUpdateTrack;
@property (nonatomic) EditColumnWindowController *editColumnWindowController;
@property (nonatomic) GroupByCustomWindowController *groupByCustomWindowController;
@property (nonatomic) int sortColumn;

@end

@implementation PlaylistViewController {
    char *_group_str;
    char *_group_bytecode;
    BOOL _pin_groups;
}

- (void)cleanup {
    [self clearGrouping];

    if (self.playPosUpdateTimer) {
        [self.playPosUpdateTimer invalidate];
        self.playPosUpdateTimer = nil;
    }

    if (self.playPosUpdateTrack) {
        deadbeef->pl_item_unref (self.playPosUpdateTrack);
        self.playPosUpdateTrack = NULL;
    }

    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView cleanup];

    // don't wait for an automatic autorelease,
    // this would cause deadbeef's track refcount checker to run before the objects are really released
    @autoreleasepool {
        _trkProperties = nil;
    }
}

- (void)menuAddColumn:(id)sender {
    if (!self.editColumnWindowController) {
        self.editColumnWindowController = [[EditColumnWindowController alloc] initWithWindowNibName:@"EditColumnPanel"];
    }

    [self.editColumnWindowController initAddColumnSheet];

    [self.view.window beginSheet:self.editColumnWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            int idx = [self insertColumn:_menuColumn];
            if (idx >= 0) {
                [self updateColumn:idx];
            }
        }
    }];
}

- (void)menuEditColumn:(id)sender {
    if (!self.editColumnWindowController) {
        self.editColumnWindowController = [[EditColumnWindowController alloc] initWithWindowNibName:@"EditColumnPanel"];
    }

    uint8_t *c = _columns[_menuColumn].text_color;
    NSColor *color = [NSColor colorWithDeviceRed:c[0]/255.f green:c[1]/255.f blue:c[2]/255.f alpha:c[3]/255.f];

    [self.editColumnWindowController initEditColumnSheetWithTitle:[NSString stringWithUTF8String:_columns[_menuColumn].title]
                                                             type:_columns[_menuColumn].type
                                                           format:[NSString stringWithUTF8String:_columns[_menuColumn].format]
                                                        alignment:_columns[_menuColumn].alignment
                                                     setTextColor:_columns[_menuColumn].set_text_color
                                                        textColor:color];


    [self.view.window beginSheet:self.editColumnWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            int idx = _menuColumn;
            if (idx >= 0) {
                [self updateColumn:idx];
            }
        }
    }];
}


- (void)menuRemoveColumn:(id)sender {
    if (_menuColumn >= 0) {
        [self removeColumnAtIndex:_menuColumn];
        [self columnsChanged];
    }
}

- (void)menuTogglePinGroups:(NSButton *)sender {
    _pin_groups = sender.state == NSOnState ? 0 : 1;
    sender.state = _pin_groups?NSOnState:NSOffState;
    deadbeef->conf_set_int ([self pinGroupsConfStr], _pin_groups);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView updatePinnedGroup];
}

- (void)clearGrouping {
    if (_group_str) {
        free (_group_str);
        _group_str = NULL;
    }
    if (_group_bytecode) {
        deadbeef->tf_free (_group_bytecode);
        _group_bytecode = NULL;
    }
}

- (void)menuGroupByNone:(id)sender {
    [self clearGrouping];
    deadbeef->conf_remove_items ([self groupByConfStr]);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView reloadData];
}

- (void)menuGroupByArtistDateAlbum:(id)sender {
    [self clearGrouping];
    _group_str = strdup ("%album artist% - ['['%year%']' ]%album%");
    deadbeef->conf_set_str ([self groupByConfStr], _group_str);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView reloadData];
}

- (void)menuGroupByArtist:(id)sender {
    [self clearGrouping];
    _group_str = strdup ("%artist%");
    deadbeef->conf_set_str ([self groupByConfStr], _group_str);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView reloadData];
}

- (void)menuGroupByCustom:(id)sender {
    if (!self.groupByCustomWindowController) {
        self.groupByCustomWindowController = [[GroupByCustomWindowController alloc] initWithWindowNibName:@"GroupByCustomWindow"];
    }

    char buf[1000];
    deadbeef->conf_get_str ([self groupByConfStr], "", buf, sizeof (buf));
    [self.groupByCustomWindowController initWithFormat:[NSString stringWithUTF8String:buf]];

    [self.view.window beginSheet:self.groupByCustomWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            [self clearGrouping];
            _group_str = strdup (self.groupByCustomWindowController.formatTextField.stringValue.UTF8String);
            deadbeef->conf_set_str([self groupByConfStr], _group_str);
            PlaylistView *lv = (PlaylistView *)self.view;
            [lv.contentView reloadData];
        }
    }];
}

- (void)columnsChanged {
    PlaylistView *lv = (PlaylistView *)self.view;
    lv.headerView.needsDisplay = YES;
    lv.contentView.needsDisplay = YES;

    NSMutableArray *columns = [[NSMutableArray alloc] initWithCapacity:_ncolumns];
    for (int i = 0; i < _ncolumns; i++) {
        uint8_t *col = _columns[i].text_color;
        NSDictionary *dict = [[NSDictionary alloc] initWithObjectsAndKeys:
                              [NSString stringWithUTF8String:_columns[i].title], @"title"
                              , [NSString stringWithFormat:@"%d", _columns[i].type], @"id"
                              , [NSString stringWithUTF8String:_columns[i].format], @"format"
                              , [NSString stringWithFormat:@"%d", _columns[i].size], @"size"
                              , [NSNumber numberWithInt:_columns[i].alignment], @"alignment"
                              , [NSNumber numberWithInt:_columns[i].set_text_color], @"set_text_color"
                              , [NSString stringWithFormat:@"#%02x%02x%02x%02x", col[3], col[0], col[1], col[2]], @"text_color"
                              , nil];
        [columns addObject:dict];
    }

    NSError *err = nil;
    NSData *dt = [NSJSONSerialization dataWithJSONObject:columns options:0 error:&err];

    [lv.contentView updateContentFrame];

    NSString *json = [[NSString alloc] initWithData:dt encoding:NSUTF8StringEncoding];
    [self writeColumnConfig:json];
    deadbeef->conf_save ();
}

- (void)updateColumn:(int)idx {
    CGFloat r, g, b, a;
    NSColor *color = [self.editColumnWindowController.colorWell.color colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
    [color getRed:&r green:&g blue:&b alpha:&a];

    uint8_t rgba[] = {
        (uint8_t)(r*255),(uint8_t)(g*255),(uint8_t)(b*255),(uint8_t)(a*255)
    };

    int type = -1;

    NSInteger typeIndex = self.editColumnWindowController.typePopUpButton.indexOfSelectedItem;
    switch (typeIndex) {
    case 0:
        type = DB_COLUMN_FILENUMBER;
        break;
    case 1:
        type = DB_COLUMN_PLAYING;
        break;
    case 2:
        type = DB_COLUMN_ALBUM_ART;
        break;
    case 3: // artist / album
        self.editColumnWindowController.formatTextField.stringValue = @"$if(%album artist%,%album artist%,Unknown Artist)[ - %album%]";
        break;
    case 4: // artist
        self.editColumnWindowController.formatTextField.stringValue = @"$if(%artist%,%artist%,Unknown Artist)";
        break;
    case 5: // album
        self.editColumnWindowController.formatTextField.stringValue = @"%album%";
        break;
    case 6: // title / track artist
        self.editColumnWindowController.formatTextField.stringValue = @"%title%[ // %track artist%]";
        break;
    case 7: // duration
        self.editColumnWindowController.formatTextField.stringValue = @"%length%";
        break;
    case 8: // track number
        self.editColumnWindowController.formatTextField.stringValue = @"%tracknumber%";
        break;
    case 9: // album artist
        self.editColumnWindowController.formatTextField.stringValue = @"$if(%album artist%,%album artist%,Unknown Artist)";
        break;
    }

    [self initColumn:idx
           withTitle:self.editColumnWindowController.titleTextField.stringValue.UTF8String
              withId:(int)type
            withSize:_columns[idx].size
          withFormat:self.editColumnWindowController.formatTextField.stringValue.UTF8String
       withAlignment:(int)self.editColumnWindowController.alignmentPopUpButton.indexOfSelectedItem withSetColor:(self.editColumnWindowController.setColorButton.state == NSOnState)
           withColor:rgba];
    [self columnsChanged];
}

#define DEFAULT_COLUMNS "[{\"title\":\"Playing\", \"id\":\"1\", \"format\":\"%playstatus%\", \"size\":\"50\"}, {\"title\":\"Artist / Album\", \"format\":\"$if(%album artist%,%album artist%,Unknown Artist)[ - %album%]\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track number%\", \"size\":\"50\"}, {\"title\":\"Title / Track Artist\", \"format\":\"%title%[ // %track artist%]\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

- (NSString *)getColumnConfig {
    return conf_get_nsstr ("cocoaui.columns", DEFAULT_COLUMNS);
}

- (void)writeColumnConfig:(NSString *)config {
    deadbeef->conf_set_str ("cocoaui.columns", [config UTF8String]);
}

- (int)playlistIter {
    return PL_MAIN;
}

- (void)initContent {
    NSString *cols = [self getColumnConfig];
    NSData *data = [cols dataUsingEncoding:NSUTF8StringEncoding];

    NSError *err = nil;
    NSArray *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

    if (!json) {
        NSLog (@"error parsing column config, error: %@\n", [err localizedDescription]);
    }
    else {
        [self loadColumns:json];
    }
    _playTpl = [[NSImage imageNamed:@"btnplayTemplate.pdf"] flippedImage];
    _pauseTpl = [[NSImage imageNamed:@"btnpauseTemplate.pdf"] flippedImage];
    _bufTpl = [[NSImage imageNamed:@"bufferingTemplate.pdf"] flippedImage];

    NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];

    textStyle.alignment = NSTextAlignmentLeft;
    textStyle.lineBreakMode = NSLineBreakByTruncatingTail;

    _colTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont controlContentFontOfSize:[NSFont smallSystemFontSize]], NSFontAttributeName
                               , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                               , NSColor.controlTextColor, NSForegroundColorAttributeName
                               , textStyle, NSParagraphStyleAttributeName
                               , nil];

    textStyle.alignment = NSTextAlignmentLeft;
    textStyle.lineBreakMode = NSLineBreakByTruncatingTail;


    int rowheight = 18;

    _groupTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                 , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                 , NSColor.controlTextColor, NSForegroundColorAttributeName
                                 , textStyle, NSParagraphStyleAttributeName
                                 , nil];

    _cellTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                , NSColor.controlTextColor, NSForegroundColorAttributeName
                                , textStyle, NSParagraphStyleAttributeName
                                , nil];

    _cellSelectedTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                        , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                        , NSColor.alternateSelectedControlTextColor, NSForegroundColorAttributeName
                                        , textStyle, NSParagraphStyleAttributeName
                                        , nil];


    // initialize group by str
    deadbeef->conf_lock ();
    const char *group_by = deadbeef->conf_get_str_fast ([self groupByConfStr], NULL);
    if (group_by) {
        _group_str = strdup (group_by);
    }
    deadbeef->conf_unlock ();

    _pin_groups = deadbeef->conf_get_int ([self pinGroupsConfStr], 0);
}

- (const char *)groupByConfStr {
    return "cocoaui.playlist.group_by";
}

- (const char *)pinGroupsConfStr {
    return "cocoaui.playlist.pin_groups";
}

- (void)awakeFromNib {
    [self setup];
}

- (void)setup {
    PlaylistView *lv = (PlaylistView *)self.view;
    lv.delegate = self;

    self.sortColumn = -1;

    [self initContent];
    [self setupPlaylist:lv];
}

- (void)freeColumns {
    for (int i = 0; i < _ncolumns; i++) {
        if (_columns[i].title) {
            free (_columns[i].title);
        }
        if (_columns[i].format) {
            free (_columns[i].format);
        }
        if (_columns[i].bytecode) {
            deadbeef->tf_free (_columns[i].bytecode);
        }
    }
    memset (_columns, 0, sizeof (_columns));
    _ncolumns = 0;
}

- (int)insertColumn:(int)beforeIdx {
    if (_ncolumns == PLT_MAX_COLUMNS) {
        return -1;
    }
    if (beforeIdx >= 0 && beforeIdx < _ncolumns) {
        memmove (&_columns[beforeIdx+1], &_columns[beforeIdx], (_ncolumns - beforeIdx) * sizeof (plt_col_info_t));
    }
    _ncolumns++;
    int idx = beforeIdx >= 0 ? beforeIdx : _ncolumns-1;
    _columns[idx].size = 100;
    return idx;
}

- (void)initColumn:(int)idx withTitle:(const char *)title withId:(int)_id withSize:(int)size withFormat:(const char *)format withAlignment:(int)alignment withSetColor:(BOOL)setColor withColor:(uint8_t *)color {
    _columns[idx].type = _id;
    _columns[idx].title = strdup (title);
    _columns[idx].format = format ? strdup (format) : NULL;
    _columns[idx].size = size;
    _columns[idx].alignment = alignment;
    if (format) {
        _columns[idx].bytecode = deadbeef->tf_compile (format);
    }
    _columns[idx].set_text_color = setColor;
    _columns[idx].text_color[0] = color[0];
    _columns[idx].text_color[1] = color[1];
    _columns[idx].text_color[2] = color[2];
    _columns[idx].text_color[3] = color[3];
}

- (void)removeColumnAtIndex:(int)idx {
    char *sortColumnTitle = NULL;
    if (self.sortColumn >= 0) {
        sortColumnTitle = _columns[self.sortColumn].title;

    }

    if (idx != _ncolumns-1) {
        memmove (&_columns[idx], &_columns[idx+1], (_ncolumns-idx) * sizeof (plt_col_info_t));
    }
    _ncolumns--;

    self.sortColumn = [self columnIndexForTitle:sortColumnTitle];
}

// pass col=-1 for "empty space", e.g. when appending new col
- (NSMenu *)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view {
    _menuColumn = (int)col;
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"ColumnMenu"];
    menu.delegate = self;
    menu.autoenablesItems = NO;
    [menu insertItemWithTitle:@"Add Column" action:@selector(menuAddColumn:) keyEquivalent:@"" atIndex:0].target = self;
    if (col != -1) {
        [menu insertItemWithTitle:@"Edit Column" action:@selector(menuEditColumn:) keyEquivalent:@"" atIndex:1].target = self;
        [menu insertItemWithTitle:@"Remove Column" action:@selector(menuRemoveColumn:) keyEquivalent:@"" atIndex:2].target = self;
        NSMenuItem *item = [menu insertItemWithTitle:@"Pin Groups When Scrolling" action:@selector(menuTogglePinGroups:) keyEquivalent:@"" atIndex:3];
        item.state = _pin_groups?NSOnState:NSOffState;
        item.target = self;

        [menu insertItem:[NSMenuItem separatorItem] atIndex:4];

        NSMenu *groupBy = [[NSMenu alloc] initWithTitle:@"Group By"];
        groupBy.delegate = self;
        groupBy.autoenablesItems = NO;

        [groupBy insertItemWithTitle:@"None" action:@selector(menuGroupByNone:) keyEquivalent:@"" atIndex:0].target = self;
        [groupBy insertItemWithTitle:@"Artist/Date/Album" action:@selector(menuGroupByArtistDateAlbum:) keyEquivalent:@"" atIndex:1].target = self;
        [groupBy insertItemWithTitle:@"Artist" action:@selector(menuGroupByArtist:) keyEquivalent:@"" atIndex:2].target = self;
        [groupBy insertItemWithTitle:@"Custom" action:@selector(menuGroupByCustom:) keyEquivalent:@"" atIndex:3];

        NSMenuItem *groupByItem = [[NSMenuItem alloc] initWithTitle:@"Group By" action:nil keyEquivalent:@""];
        groupByItem.submenu = groupBy;
        [menu insertItem:groupByItem atIndex:5];
    }

    return menu;
}

- (BOOL)isAlbumArtColumn:(DdbListviewCol_t)col {
    return _columns[col].type == DB_COLUMN_ALBUM_ART;
}

- (void)loadColumns:(NSArray *)cols {
    [self freeColumns];
    [cols enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        NSDictionary *dict = obj;
        NSString *title_s = [dict objectForKey:@"title"];
        NSString *id_s = [dict objectForKey:@"id"];
        NSString *format_s = [dict objectForKey:@"format"];
        NSString *size_s = [dict objectForKey:@"size"];
        NSNumber *alignment_n = [dict objectForKey:@"alignment"];
        NSNumber *setcolor_n = [dict objectForKey:@"set_text_color"];
        NSString *textcolor_s = [dict objectForKey:@"text_color"];

        const char *title = "";
        if (title_s) {
            title = [title_s UTF8String];
        }

        int _id = -1;
        if (id_s) {
            _id = (int)[id_s integerValue];
        }

        const char *fmt = NULL;
        if (format_s) {
            fmt = [format_s UTF8String];
        }

        int size = 80;
        if (size_s) {
            size = (int)[size_s integerValue];
        }

        int alignment = 0;
        if (alignment_n) {
            alignment = [alignment_n intValue];
        }

        BOOL setcolor = NO;
        if (setcolor_n) {
            setcolor = [setcolor_n intValue] ? YES : NO;
        }

        int r = 0, g = 0, b = 0, a = 0xff;
        if (textcolor_s) {
            sscanf ([textcolor_s UTF8String], "#%02x%02x%02x%02x", &a, &r, &g, &b);
        }

        uint8_t rgba[4] = {
            r, g, b, a
        };

        [self initColumn:_ncolumns withTitle:title withId:_id withSize:size withFormat:fmt withAlignment:alignment withSetColor:setcolor withColor:rgba];
        _ncolumns++;
    }];
}

- (void)lock {
    deadbeef->pl_lock ();
}

- (void)unlock {
    deadbeef->pl_unlock ();
}

- (int)columnCount {
    return _ncolumns;
}

- (int)rowCount {
    return deadbeef->pl_getcount ([self playlistIter]);
}

- (int)cursor {
    return deadbeef->pl_get_cursor([self playlistIter]);
}

- (void)setCursor:(int)cursor {
    deadbeef->pl_set_cursor ([self playlistIter], cursor);
}

- (void)activate:(int)idx {
    DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (idx, [self playlistIter]);
    if (it) {
        int i = deadbeef->pl_get_idx_of (it);
        if (i != -1) {
            deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, i, 0);
        }
        deadbeef->pl_item_unref (it);
    }
}

- (DdbListviewCol_t)firstColumn {
    return 0;
}

- (DdbListviewCol_t)nextColumn:(DdbListviewCol_t)col {
    return col >= [self columnCount] - 1 ? [self invalidColumn] : col+1;
}

- (DdbListviewCol_t)invalidColumn {
    return -1;
}

- (int)columnWidth:(DdbListviewCol_t)col {
    return _columns[col].size;
}

- (int)columnGroupHeight:(DdbListviewCol_t)col {
    return _columns[col].size - ART_PADDING_HORZ*2;
}

- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col {
    _columns[col].size = width;
}

- (int)columnMinHeight:(DdbListviewCol_t)col {
    return _columns[col].type == DB_COLUMN_ALBUM_ART;
}

- (int)columnIndexForTitle:(const char *)title {
    for (int i = 0; i < _ncolumns; i++) {
        if (_columns[i].title == title) {
            return i;
        }
    }
    return -1;
}

- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to {
    plt_col_info_t tmp;

    char *sortColumnTitle = NULL;
    if (self.sortColumn >= 0) {
        sortColumnTitle = _columns[self.sortColumn].title;

    }

    while (col < to) {
        memcpy (&tmp, &_columns[col], sizeof (plt_col_info_t));
        memmove (&_columns[col], &_columns[col+1], sizeof (plt_col_info_t));
        memcpy (&_columns[col+1], &tmp, sizeof (plt_col_info_t));
        col++;
    }
    while (col > to) {
        memcpy (&tmp, &_columns[col], sizeof (plt_col_info_t));
        memmove (&_columns[col], &_columns[col-1], sizeof (plt_col_info_t));
        memcpy (&_columns[col-1], &tmp, sizeof (plt_col_info_t));
        col--;
    }

    self.sortColumn = [self columnIndexForTitle:sortColumnTitle];
}

- (DdbListviewRow_t)firstRow {
    return (DdbListviewRow_t)deadbeef->pl_get_first([self playlistIter]);
}

- (DdbListviewRow_t)nextRow:(DdbListviewRow_t)row {
    return (DdbListviewRow_t)deadbeef->pl_get_next((DB_playItem_t *)row, [self playlistIter]);
}

- (DdbListviewRow_t)invalidRow {
    return 0;
}

- (DdbListviewRow_t)rowForIndex:(int)idx {
    return (DdbListviewRow_t)deadbeef->pl_get_for_idx_and_iter (idx, [self playlistIter]);
}

- (void)refRow:(DdbListviewRow_t)row {
    deadbeef->pl_item_ref ((DB_playItem_t *)row);
}

- (void)unrefRow:(DdbListviewRow_t)row {
    deadbeef->pl_item_unref ((DB_playItem_t *)row);
}

- (void)drawColumnHeader:(DdbListviewCol_t)col inRect:(NSRect)rect {
    if (col < self.columnCount) {
        CGFloat width = rect.size.width-6;
        if (col == self.sortColumn) {
            width -= 16;
        }
        if (width < 0) {
            width = 0;
        }
        [NSColor.controlTextColor set];
        [[NSString stringWithUTF8String:_columns[col].title] drawInRect:NSMakeRect(rect.origin.x+4, rect.origin.y-2, width, rect.size.height-2) withAttributes:_colTextAttrsDictionary];


        if (col == self.sortColumn) {
            [[NSColor.controlTextColor highlightWithLevel:0.5] set];
            NSBezierPath *path = [NSBezierPath new];
            path.lineWidth = 2;
            if (_columns[col].sort_order == DDB_SORT_ASCENDING) {
                [path moveToPoint:NSMakePoint(rect.origin.x+4+width+4, rect.origin.y+10)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+8, rect.origin.y+10+4)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+12, rect.origin.y+10)];
            }
            else if (_columns[col].sort_order == DDB_SORT_DESCENDING) {
                [path moveToPoint:NSMakePoint(rect.origin.x+4+width+4, rect.origin.y+10+4)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+8, rect.origin.y+10)];
                [path lineToPoint:NSMakePoint(rect.origin.x+4+width+12, rect.origin.y+10+4)];
            }
            [path stroke];
        }
    }
}

- (NSMutableAttributedString *)stringWithTintAttributesFromString:(const char *)inputString initialAttributes:(NSDictionary *)attributes foregroundColor:(NSColor *)foregroundColor backgroundColor:(NSColor *)backgroundColor {
    const int maxTintRanges = 100;
    int tintRanges[maxTintRanges];
    NSUInteger numTintRanges;
    char * plainString;

    numTintRanges = calculate_tint_ranges_from_string (inputString, tintRanges, maxTintRanges, &plainString);

    NSMutableAttributedString *str = [[NSMutableAttributedString alloc] initWithString:[NSString stringWithUTF8String:plainString] attributes:attributes];

    // add attributes
    for (NSUInteger i = 0; i < numTintRanges/2; i++) {
        int index0 = tintRanges[i*2+1];
        int tint = tintRanges[i*2+0];
        NSUInteger len = str.length - index0;

        CGFloat blend = 1.f + 0.1 * tint;
        if (blend < 0) {
            blend = 0;
        }

        NSColor *tinted = [backgroundColor blendedColorWithFraction:blend ofColor:foregroundColor];

        [str addAttributes:@{
            NSForegroundColorAttributeName:tinted
        } range:NSMakeRange(index0, len)];
    }

    free (plainString);

    return str;
}

- (void)drawCell:(int)idx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused {
    int sel = deadbeef->pl_is_selected((DB_playItem_t *)row);
    NSColor *background = NSColor.controlBackgroundColor;
    if (sel) {
        if (focused) {
            [NSColor.alternateSelectedControlColor set];
            background = NSColor.alternateSelectedControlColor;
            [NSBezierPath fillRect:rect];
        }
        else {
            [NSColor.controlShadowColor set];
            background = NSColor.controlShadowColor;
            [NSBezierPath fillRect:rect];
        }
    }

    if (col == [self invalidColumn]) {
        return;
    }

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();

    if (_columns[col].type == DB_COLUMN_PLAYING && playing_track && (DB_playItem_t *)row == playing_track) {
        NSImage *img = NULL;
        int paused = deadbeef->get_output ()->state () == DDB_PLAYBACK_STATE_PAUSED;
        int buffering = !deadbeef->streamer_ok_to_read (-1);
        if (paused) {
            img = _pauseTpl;
        }
        else if (!buffering) {
            img = _playTpl;
        }
        else {
            img = _bufTpl;
        }

        NSColor *imgColor = sel ? NSColor.alternateSelectedControlTextColor : NSColor.controlTextColor;

        CGContextRef c = [[NSGraphicsContext currentContext] graphicsPort];
        CGContextSaveGState(c);

        NSRect maskRect = rect;
        if (maskRect.size.width > maskRect.size.height) {
            maskRect.size.width = maskRect.size.height;
        }
        else {
            maskRect.size.height = maskRect.size.width;
        }
        maskRect.origin = NSMakePoint(rect.origin.x + rect.size.width/2 - maskRect.size.width/2, rect.origin.y + rect.size.height/2 - maskRect.size.height/2);

        CGImageRef maskImage = [img CGImageForProposedRect:&maskRect context:[NSGraphicsContext currentContext] hints:nil];

        CGContextClipToMask(c, NSRectToCGRect(maskRect), maskImage);
        [imgColor set];
        [NSBezierPath fillRect:maskRect];
        CGContextRestoreGState(c);
    }

    if (playing_track) {
        deadbeef->pl_item_unref (playing_track);
    }

    if (_columns[col].bytecode) {
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = (DB_playItem_t *)row,
            .plt = deadbeef->plt_get_curr (),
            .id = _columns[col].type,
            .idx = idx,
            .flags = DDB_TF_CONTEXT_HAS_ID|DDB_TF_CONTEXT_HAS_INDEX|DDB_TF_CONTEXT_TEXT_DIM,
        };

        char text[1024] = "";
        deadbeef->tf_eval (&ctx, _columns[col].bytecode, text, sizeof (text));

        rect.origin.x += CELL_HPADDING;
        rect.size.width -= CELL_HPADDING;

        if (text[0]) {
            NSDictionary *attributes = sel?_cellSelectedTextAttrsDictionary:_cellTextAttrsDictionary;
            NSColor *foreground = attributes[NSForegroundColorAttributeName];

            NSMutableAttributedString *attrString = [self stringWithTintAttributesFromString:text initialAttributes:attributes                                                     foregroundColor:foreground backgroundColor:background];
            [attrString drawInRect:rect];
        }

        if (ctx.update > 0) {
            if (self.playPosUpdateTimer) {
                [self.playPosUpdateTimer invalidate];
            }
            if (self.playPosUpdateTrack) {
                deadbeef->pl_item_unref (self.playPosUpdateTrack);
                self.playPosUpdateTrack = NULL;
            }
            self.playPosUpdateTrack = deadbeef->pl_get_for_idx_and_iter (ctx.idx, [self playlistIter]);

            self.playPosUpdateTimer = [NSTimer scheduledTimerWithTimeInterval:ctx.update/1000.0 repeats:NO block:^(NSTimer * _Nonnull timer) {
                ddb_playlist_t *curr = deadbeef->plt_get_curr ();
                DB_playItem_t *trk = deadbeef->pl_get_for_idx_and_iter (ctx.idx, [self playlistIter]);

                if (ctx.plt == curr && trk == self.playPosUpdateTrack) {
                    PlaylistView *lv = (PlaylistView *)self.view;
                    [lv.contentView drawRow:idx];
                }
                if (trk) {
                    deadbeef->pl_item_unref (trk);
                }
                if (curr) {
                    deadbeef->plt_unref (curr);
                }
            }];
        }
        if (ctx.plt) {
            deadbeef->plt_unref (ctx.plt);
        }
    }
}


- (void)drawGroupTitle:(DdbListviewRow_t)row inRect:(NSRect)rect {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = (DB_playItem_t *)row,
        .plt = deadbeef->plt_get_curr (),
        .flags = DDB_TF_CONTEXT_TEXT_DIM | DDB_TF_CONTEXT_NO_DYNAMIC,
    };

    char text[1024] = "";
    deadbeef->tf_eval (&ctx, _group_bytecode, text, sizeof (text));

    NSMutableAttributedString *attrString = [self stringWithTintAttributesFromString:text initialAttributes:_groupTextAttrsDictionary foregroundColor:_groupTextAttrsDictionary[NSForegroundColorAttributeName] backgroundColor:NSColor.controlBackgroundColor];

    NSSize size = [attrString size];

    NSRect strRect = rect;
    strRect.origin.x += 5;
    strRect.origin.y = strRect.origin.y + strRect.size.height / 2 - size.height / 2;
    strRect.size.height = size.height;
    [attrString drawInRect:strRect];

    if (ctx.plt) {
        deadbeef->plt_unref (ctx.plt);
    }

    if (size.width < rect.size.width - 15) {
        [NSBezierPath fillRect:NSMakeRect(size.width + 10, rect.origin.y + rect.size.height/2, rect.size.width - size.width - 15, 1)];
    }
}

typedef struct {
    void *ctl; // DdbPlaylistViewController ptr (retain)
    int grp;
} cover_avail_info_t;

static void coverAvailCallback (NSImage *__strong img, void *user_data) {
    cover_avail_info_t *info = user_data;
    PlaylistViewController *ctl = (__bridge_transfer PlaylistViewController *)info->ctl;
    PlaylistView *lv = (PlaylistView *)ctl.view;
    [lv.contentView drawGroup:info->grp];
    free (info);
}

- (void)drawAlbumArtForGroup:(DdbListviewGroup_t *)grp
                  groupIndex:(int)groupIndex
                  inColumn:(DdbListviewCol_t)col
             isPinnedGroup:(BOOL)pinned
            nextGroupCoord:(int)grp_next_y
                      xPos:(int)x
                      yPos:(int)y
                 viewportY:(CGFloat)viewportY
                     width:(int)width
                    height:(int)height {
    PlaylistView *lv = (PlaylistView *)self.view;
    DB_playItem_t *it = (DB_playItem_t *)grp->head;
    cover_avail_info_t *inf = calloc (sizeof (cover_avail_info_t), 1);
    inf->ctl = (__bridge_retained void *)self;
    inf->grp = groupIndex;
    NSImage *image = [[CoverManager defaultCoverManager] getCoverForTrack:it withCallbackWhenReady:coverAvailCallback withUserDataForCallback:inf];
    if (!image) {
        // FIXME: the problem here is that if the cover is not found (yet) -- it won't draw anything, but the rect is already invalidated, and will come out as background color
        return;
    }

    int art_width = width - ART_PADDING_HORZ * 2;
    int art_height = height - ART_PADDING_VERT * 2 - lv.contentView.grouptitle_height;

    if (art_width < 8 || art_height < 8 || !it) {
        return;
    }

    int art_x = x + ART_PADDING_HORZ;
    CGFloat min_y = (pinned ? viewportY+lv.contentView.grouptitle_height : y) + ART_PADDING_VERT;
    CGFloat max_y = grp_next_y;

    CGFloat ypos = min_y;
    if (min_y + art_width + ART_PADDING_VERT >= max_y) {
        ypos = max_y - art_width - ART_PADDING_VERT;
    }

    NSSize size = [image size];
    if (size.width >= size.height) {
        CGFloat h = size.height / (size.width / art_width);
//        ypos += art_height/2 - h/2;
        [image drawInRect:NSMakeRect(art_x, ypos, art_width, h)];
    }
    else {
        plt_col_info_t *c = &_columns[(int)col];
        CGFloat h = art_width;
        CGFloat w = size.width / (size.height / h);
        if (c->alignment == 1) {
            art_x += art_width/2 - w/2;
        }
        else if (c->alignment == 2) {
            art_x += art_width-w;
        }
        [image drawInRect:NSMakeRect(art_x, ypos, w, h)];
    }
}

- (void)selectRow:(DdbListviewRow_t)row withState:(BOOL)state {
    deadbeef->pl_set_selected ((DB_playItem_t *)row, state);
}

- (BOOL)rowSelected:(DdbListviewRow_t)row {
    return deadbeef->pl_is_selected ((DB_playItem_t *)row);
}

- (void)deselectAll {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
}

- (NSString *)rowGroupStr:(DdbListviewRow_t)row {
    if (!_group_str) {
        return nil;
    }
    if (!_group_bytecode) {
        _group_bytecode = deadbeef->tf_compile (_group_str);
    }

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = (DB_playItem_t *)row,
        .plt = deadbeef->plt_get_curr(),
    };
    char buf[1024];
    NSString *ret = @"";
    if (deadbeef->tf_eval (&ctx, _group_bytecode, buf, sizeof (buf)) > 0) {
        ret = [NSString stringWithUTF8String:buf];
        if (!ret) {
            ret = @"";
        }
    }
    if (ctx.plt) {
        deadbeef->plt_unref (ctx.plt);
    }
    return ret;
}

- (BOOL)pinGroups {
    return _pin_groups;
}

- (int)modificationIdx {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    int res = plt ? deadbeef->plt_get_modification_idx (plt) : 0;
    if (plt) {
        deadbeef->plt_unref (plt);
    }
    return res;
}

- (void)selectionChanged:(DdbListviewRow_t)row {
    PlaylistView *lv = (PlaylistView *)self.view;
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, (uintptr_t)lv, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

- (int)selectedCount {
    return deadbeef->pl_getselcount();
}

- (BOOL)hasDND {
    return YES;
}

- (void)songChanged:(PlaylistView *)listview from:(DB_playItem_t*)from to:(DB_playItem_t*)to {
    int to_idx = -1;
    if (to) {
        int cursor_follows_playback = deadbeef->conf_get_int ("playlist.scroll.cursorfollowplayback", 1);
        int scroll_follows_playback = deadbeef->conf_get_int ("playlist.scroll.followplayback", 1);
        int plt = deadbeef->streamer_get_current_playlist ();
        if (plt != -1) {
            if (plt != deadbeef->plt_get_curr_idx ()) {
                ddb_playlist_t *p = deadbeef->plt_get_for_idx (plt);
                if (p) {
                    to_idx = deadbeef->plt_get_item_idx (p, to, PL_MAIN);
                    if (cursor_follows_playback) {
                        deadbeef->plt_deselect_all (p);
                        deadbeef->pl_set_selected (to, 1);
                        deadbeef->plt_set_cursor (p, PL_MAIN, to_idx);
                    }
                    deadbeef->plt_unref (p);
                }
                return;
            }
            to_idx = deadbeef->pl_get_idx_of_iter (to, [self playlistIter]);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    [listview.contentView setCursor:to_idx noscroll:YES];
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr_idx ()) {
                    [listview.contentView scrollToRowWithIndex: to_idx];
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            [listview.contentView drawRow:idx];
        }
    }
    if (to && to_idx != -1) {
        [listview.contentView drawRow:to_idx];
    }
}

- (void)setupPlaylist:(PlaylistView *)listview {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->pl_lock ();
        int cursor = deadbeef->plt_get_cursor (plt, PL_MAIN);
        int scroll = deadbeef->plt_get_scroll (plt);
        if (cursor != -1) {
            DB_playItem_t *it = deadbeef->pl_get_for_idx_and_iter (cursor, PL_MAIN);
            if (it) {
                deadbeef->pl_set_selected (it, 1);
                deadbeef->pl_item_unref (it);
            }
        }
        deadbeef->plt_unref (plt);

        [listview.contentView reloadData];
        deadbeef->pl_unlock ();
        [listview.contentView scrollVerticalPosition:scroll];
    }
}

- (int)sendMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    PlaylistView *listview = (PlaylistView *)self.view;
    switch (_id) {
        case DB_EV_SONGCHANGED: {
            if ([self playlistIter] != PL_MAIN) {
                break;
            }
            ddb_event_trackchange_t *ev = (ddb_event_trackchange_t *)ctx;
            DB_playItem_t *from = ev->from;
            DB_playItem_t *to = ev->to;
            if (from)
                deadbeef->pl_item_ref (from);
            if (to)
                deadbeef->pl_item_ref (to);
            dispatch_async(dispatch_get_main_queue(), ^{
                DB_playItem_t *it;
                int idx = 0;
                deadbeef->pl_lock ();
                for (it = deadbeef->pl_get_first (PL_MAIN); it; idx++) {
                    if (deadbeef->playqueue_test (it) != -1) {
                        [listview.contentView drawRow:idx];
                    }
                    DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                    deadbeef->pl_item_unref (it);
                    it = next;
                }
                [self songChanged:listview from:from to:to];
                if (from)
                    deadbeef->pl_item_unref (from);
                if (to)
                    deadbeef->pl_item_unref (to);
                deadbeef->pl_unlock ();
            });
        }
            break;
        case DB_EV_TRACKINFOCHANGED: {
            ddb_event_track_t *ev = (ddb_event_track_t *)ctx;
            DB_playItem_t *track = ev->track;
            if (track) {
                deadbeef->pl_item_ref (track);
                dispatch_async(dispatch_get_main_queue(), ^{
                    BOOL draw = NO;
                    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                    if (plt) {
                        int idx = deadbeef->plt_get_item_idx (plt, track, PL_MAIN);
                        if (idx != -1) {
                            draw = YES;
                        }
                        deadbeef->plt_unref (plt);
                    }
                    if (draw) {
                        [listview.contentView drawRow:deadbeef->pl_get_idx_of (track)];
                    }
                    deadbeef->pl_item_unref (track);
                });
            }
        }
            break;
        case DB_EV_PAUSED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                DB_playItem_t *curr = deadbeef->streamer_get_playing_track ();
                if (curr) {
                    int idx = deadbeef->pl_get_idx_of (curr);
                    [listview.contentView drawRow:idx];
                    deadbeef->pl_item_unref (curr);
                }
            });
        }
            break;
        case DB_EV_PLAYLISTCHANGED: {
            if (!p1 || (p1 == DDB_PLAYLIST_CHANGE_SEARCHRESULT)) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [listview.contentView reloadData];
                });
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_SELECTION) {
                if (ctx != (uintptr_t)listview) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [listview.contentView reloadData];
                    });
                }
            }
        }
            break;
        case DB_EV_PLAYLISTSWITCHED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                [self setupPlaylist:listview];
            });
        }
            break;
        case DB_EV_TRACKFOCUSCURRENT: {
            dispatch_async(dispatch_get_main_queue(), ^{
                deadbeef->pl_lock ();
                DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
                if (it) {
                    ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);

                    if (!plt) {
                        deadbeef->pl_item_unref (it);
                        deadbeef->pl_unlock ();
                        return;
                    }

                    ddb_playlist_t *prev_plt = deadbeef->plt_get_curr ();

                    if (prev_plt != plt) {
                        // force group rebuild
                        deadbeef->plt_set_curr (plt);
                        [listview.contentView reloadData];
                    }

                    int idx = deadbeef->pl_get_idx_of_iter (it, [self playlistIter]);
                    if (idx != -1) {
                        [listview.contentView setCursor:idx noscroll:YES];
                        [listview.contentView scrollToRowWithIndex:idx];
                    }
                    deadbeef->plt_unref (plt);
                    deadbeef->plt_unref (prev_plt);
                    deadbeef->pl_item_unref (it);
                }
                deadbeef->pl_unlock ();
            });
        }
            break;
        case DB_EV_CONFIGCHANGED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                [listview.contentView reloadData];
            });
        }
            break;
        case DB_EV_FOCUS_SELECTION: {
            if ([self playlistIter] != (int)p1) {
                break;
            }

            dispatch_async(dispatch_get_main_queue(), ^{
                deadbeef->pl_lock ();
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
                    DB_playItem_t *it = deadbeef->plt_get_first (plt, PL_MAIN);
                    while (it) {
                        if (deadbeef->pl_is_selected (it)) {
                            break;
                        }
                        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                        deadbeef->pl_item_unref (it);
                        it = next;
                    }
                    if (it) {
                        int idx = deadbeef->pl_get_idx_of (it);
                        if (idx != -1) {
                            [listview.contentView setCursor:idx noscroll:YES];
                            [listview.contentView scrollToRowWithIndex:idx];
                        }
                        deadbeef->pl_item_unref (it);
                    }

                    deadbeef->plt_unref (plt);
                }
                deadbeef->pl_unlock ();
            });
        }
    }
    return [super sendMessage:_id ctx:ctx p1:p1 p2:p2];
}

- (void)convertSelection {
    [ConverterWindowController runConverter:DDB_ACTION_CTX_SELECTION];
}

- (void)trackProperties {
    if (!_trkProperties) {
        _trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    _trkProperties.playlist =  plt;
    deadbeef->plt_unref (plt);
    [_trkProperties fill];
    [_trkProperties showWindow:self];
}

- (void)addToPlaybackQueue {
    int iter = [self playlistIter];
    DB_playItem_t *it = deadbeef->pl_get_first(iter);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_push (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, iter);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

- (void)removeFromPlaybackQueue {
    int iter = [self playlistIter];
    DB_playItem_t *it = deadbeef->pl_get_first(iter);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->playqueue_remove (it);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, iter);
        deadbeef->pl_item_unref (it);
        it = next;
    }
}

- (void)forEachTrack:(BOOL (^)(DB_playItem_t *it))block forIter:(int)iter {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (iter);
    while (it) {
        BOOL res = block (it);
        if (!res) {
            deadbeef->pl_item_unref (it);
            break;
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, iter);
        deadbeef->pl_item_unref (it);
        it = next;
    }

    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);
}

- (void)reloadMetadata {
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        deadbeef->pl_lock ();
        char decoder_id[100];
        const char *dec = deadbeef->pl_find_meta (it, ":DECODER");
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = deadbeef->pl_is_selected (it) && deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI")) && dec;
        deadbeef->pl_unlock ();

        if (match) {
            uint32_t f = deadbeef->pl_get_item_flags (it);
            if (!(f & DDB_IS_SUBTRACK)) {
                f &= ~DDB_TAG_MASK;
                deadbeef->pl_set_item_flags (it, f);
                DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
                for (int i = 0; decoders[i]; i++) {
                    if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                        if (decoders[i]->read_metadata) {
                            decoders[i]->read_metadata (it);
                        }
                        break;
                    }
                }
            }
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (void)sortColumn:(DdbListviewCol_t)column {
    plt_col_info_t *c = &_columns[(int)column];
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();

    if (self.sortColumn != column) {
        _columns[column].sort_order = 0;
    }
    self.sortColumn = (int)column;
    _columns[column].sort_order = 1 - _columns[column].sort_order;

    deadbeef->plt_sort_v2 (plt, PL_MAIN, c->type, c->format, _columns[column].sort_order);
    deadbeef->plt_unref (plt);

    PlaylistView *lv = (PlaylistView *)self.view;
    lv.headerView.needsDisplay = YES;
}

- (void)dropItems:(int)from_playlist before:(DdbListviewRow_t)before indices:(uint32_t *)indices count:(int)count copy:(BOOL)copy {

    deadbeef->pl_lock ();
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    ddb_playlist_t *from = deadbeef->plt_get_for_idx(from_playlist);

    if (copy) {
        deadbeef->plt_copy_items (plt, PL_MAIN, from, (DB_playItem_t *)before, indices, count);
    }
    else {
        deadbeef->plt_move_items (plt, PL_MAIN, from, (DB_playItem_t *)before, indices, count);
    }

    deadbeef->plt_save_config (plt);
    deadbeef->plt_save_config (from);
    deadbeef->plt_unref (plt);
    deadbeef->plt_unref (from);
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

-(void)externalDropItems:(NSArray *)paths after:(DdbListviewRow_t)_after {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (!deadbeef->plt_add_files_begin (plt, 0)) {
        dispatch_queue_t aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
        dispatch_async(aQueue, ^{
            DB_playItem_t *first = NULL;
            DB_playItem_t *after = (DB_playItem_t *)_after;
            for(NSUInteger i = 0; i < paths.count; i++ )
            {
                NSString* path = [paths objectAtIndex:i];
                if (path) {
                    int abort = 0;
                    const char *fname = [path UTF8String];
                    DB_playItem_t *inserted = deadbeef->plt_insert_dir2 (0, plt, after, fname, &abort, NULL, NULL);
                    if (!inserted && !abort) {
                        inserted = deadbeef->plt_insert_file2 (0, plt, after, fname, &abort, NULL, NULL);
                        if (!inserted && !abort) {
                            inserted = deadbeef->plt_load2 (0, plt, after, fname, &abort, NULL, NULL);
                        }
                    }

                    if (inserted) {
                        if (!first) {
                            first = inserted;
                        }
                        if (after) {
                            deadbeef->pl_item_unref (after);
                        }
                        after = inserted;
                        deadbeef->pl_item_ref (after);

                        // TODO: set cursor to the first dropped item
                    }
                }
            }
            if (after) {
                deadbeef->pl_item_unref (after);
            }
            deadbeef->plt_add_files_end (plt, 0);
            deadbeef->plt_unref (plt);
            deadbeef->pl_save_current();
            deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
        });
    }
    else {
        if (_after) {
            deadbeef->pl_item_unref ((DB_playItem_t *)_after);
        }
        deadbeef->plt_unref (plt);
    }
}

- (void)scrollChanged:(CGFloat)pos {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_set_scroll (plt, (int)pos);
        deadbeef->plt_unref (plt);
    }
}

- (void)rgRemove:(id)sender {
    int count;
    DB_playItem_t **tracks = [self getSelectedTracksForRg:&count withRgTags:YES];
    if (!tracks) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    [ReplayGainScannerController removeRgTagsFromTracks:tracks count:count];
}

- (void)rgScanAlbum:(id)sender {
    [self rgScan:DDB_RG_SCAN_MODE_SINGLE_ALBUM];
}

- (void)rgScanAlbumsAuto:(id)sender {
    [self rgScan:DDB_RG_SCAN_MODE_ALBUMS_FROM_TAGS];
}

- (void)rgScanTracks:(id)sender {
    [self rgScan:DDB_RG_SCAN_MODE_TRACK];
}

- (DB_playItem_t **)getSelectedTracksForRg:(int *)pcount withRgTags:(BOOL)withRgTags {
   ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->pl_lock ();
    DB_playItem_t __block **tracks = NULL;
    int numtracks = deadbeef->plt_getselcount (plt);
    if (!numtracks) {
        deadbeef->pl_unlock ();
        return NULL;
    }

    ddb_replaygain_settings_t __block s;
    s._size = sizeof (ddb_replaygain_settings_t);

    tracks = calloc (numtracks, sizeof (DB_playItem_t *));
    int __block n = 0;
    [self forEachTrack:^(DB_playItem_t *it) {
        if (deadbeef->pl_is_selected (it)) {
            assert (n < numtracks);
            BOOL hasRgTags = NO;
            if (withRgTags) {
                deadbeef->replaygain_init_settings (&s, it);
                if (s.has_album_gain || s.has_track_gain) {
                    hasRgTags = YES;
                }
            }
            if (!withRgTags || hasRgTags) {
                deadbeef->pl_item_ref (it);
                tracks[n++] = it;
            }
        }
        return YES;
    }  forIter:PL_MAIN];
    deadbeef->pl_unlock ();
    deadbeef->plt_unref (plt);

    if (!n) {
        free (tracks);
        return NULL;
    }
    *pcount = n;
    return tracks;
}

- (void)rgScan:(int)mode {
    int count;
    DB_playItem_t **tracks = [self getSelectedTracksForRg:&count withRgTags:NO];
    if (!tracks) {
        return;
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    [ReplayGainScannerController runScanner:mode forTracks:tracks count:count];
}

- (void)addPluginActions:(NSMenu *)theMenu {
    DB_playItem_t *track = NULL;
    int selcount = self.selectedCount;

    if (selcount == 1) {
        DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                break;
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);

            deadbeef->pl_item_unref (it);
            it = next;
        }
        track = it;
    }

    [theMenu addActionItemsForContext:DDB_ACTION_CTX_SELECTION track:track filter:^BOOL(DB_plugin_action_t * _Nonnull action) {

        return (selcount==1 && (action->flags&DB_ACTION_SINGLE_TRACK)) || (selcount > 1 && (action->flags&DB_ACTION_MULTIPLE_TRACKS));
    }];
}

- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view {
    NSMenu *theMenu = [[NSMenu alloc] initWithTitle:@"Playlist Context Menu"];
    BOOL enabled = [self selectedCount] != 0;

    [theMenu insertItemWithTitle:@"Reload metadata" action:@selector(reloadMetadata) keyEquivalent:@"" atIndex:0].enabled = enabled;

    NSMenu *rgMenu = [[NSMenu alloc] initWithTitle:@"ReplayGain"];
    rgMenu.delegate = self;
    rgMenu.autoenablesItems = NO;

    BOOL __block has_rg_info = NO;
    BOOL __block can_be_rg_scanned = NO;
    if (enabled) {
        ddb_replaygain_settings_t __block s;
        s._size = sizeof (ddb_replaygain_settings_t);

        [self forEachTrack:^(DB_playItem_t *it){
            if (deadbeef->pl_is_selected (it)) {
                if (deadbeef->is_local_file (deadbeef->pl_find_meta (it, ":URI"))) {
                    if (deadbeef->pl_get_item_duration (it) > 0) {
                        can_be_rg_scanned = YES;
                    }
                    deadbeef->replaygain_init_settings (&s, it);
                    if (s.has_album_gain || s.has_track_gain) {
                        has_rg_info = YES;
                        return NO;
                    }
                }
            }
            return YES;
        } forIter:PL_MAIN];
    }

    [rgMenu addItemWithTitle:@"Scan Per-file Track Gain" action:@selector(rgScanTracks:) keyEquivalent:@""].enabled = can_be_rg_scanned;
    [rgMenu addItemWithTitle:@"Scan Selection As Single Album" action:@selector(rgScanAlbum:) keyEquivalent:@""].enabled = can_be_rg_scanned;
    [rgMenu addItemWithTitle:@"Scan Selection As Albums (By Tags)" action:@selector(rgScanAlbumsAuto:) keyEquivalent:@""].enabled = can_be_rg_scanned;
    [rgMenu addItemWithTitle:@"Remove ReplayGain Information" action:@selector(rgRemove:) keyEquivalent:@""].enabled = has_rg_info;

    NSMenuItem *rgMenuItem = [[NSMenuItem alloc] initWithTitle:@"ReplayGain" action:nil keyEquivalent:@""];
    rgMenuItem.enabled = enabled;
    rgMenuItem.submenu = rgMenu;
    [theMenu addItem:rgMenuItem];

    [theMenu addItemWithTitle:@"Add To Playback Queue" action:@selector(addToPlaybackQueue) keyEquivalent:@""].enabled = enabled;

    [theMenu addItemWithTitle:@"Remove From Playback Queue" action:@selector(removeFromPlaybackQueue) keyEquivalent:@""].enabled = enabled;

    [theMenu addItem:NSMenuItem.separatorItem];

    [theMenu addItem:NSMenuItem.separatorItem];

    [theMenu addItemWithTitle:@"Convert" action:@selector(convertSelection) keyEquivalent:@""].enabled = enabled;

    [self addPluginActions:theMenu];

    [theMenu addItem:NSMenuItem.separatorItem];

    [theMenu addItemWithTitle:@"Track Properties" action:@selector(trackProperties) keyEquivalent:@""].enabled = enabled;

    theMenu.autoenablesItems = NO;

    return theMenu;
}

@end
