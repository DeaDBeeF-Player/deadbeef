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
#import "CoverManager.h"
#import "EditColumnWindowController.h"
#import "GroupByCustomWindowController.h"
#import "NSImage+Additions.h"
#import "PlaylistGroup.h"
#import "PlaylistViewController.h"
#import "PlaylistView.h"
#import "TrackPropertiesWindowController.h"
#import "TrackContextMenu.h"
#import "tftintutil.h"

#include "deadbeef.h"
#include "medialib.h"
#include "utf8.h"
#include "artwork.h"

#define CELL_HPADDING 4
#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

extern DB_functions_t *deadbeef;

@interface PlaylistViewController() <DdbListviewDelegate,TrackContextMenuDelegate>

@property (nonatomic) NSImage *playTpl;
@property (nonatomic) NSImage *pauseTpl;
@property (nonatomic) NSImage *bufTpl;
@property (nonatomic) NSDictionary *cellTextAttrsDictionary;
@property (nonatomic) NSDictionary *cellSelectedTextAttrsDictionary;
@property (nonatomic) NSDictionary *groupTextAttrsDictionary;
@property (nonatomic) TrackPropertiesWindowController *trkProperties;
@property (nonatomic) int menuColumn;

@property (nonatomic) char *groupBytecode;
@property (nonatomic) BOOL pinGroups;


@property (nonatomic, strong) NSTimer *playPosUpdateTimer;
@property (nonatomic, assign) DB_playItem_t *playPosUpdateTrack;
@property (nonatomic) EditColumnWindowController *editColumnWindowController;
@property (nonatomic) GroupByCustomWindowController *groupByCustomWindowController;
@property (nonatomic) int sortColumn;
@property (nonatomic) ddb_medialib_plugin_t *medialibPlugin;
@property (nonatomic,readonly) const char *groupByConfStr;
@property (nonatomic) NSString *groupStr;

@property (nonatomic,readwrite) plt_col_info_t *columns;
@property (nonatomic,readwrite) int ncolumns;
@property (nonatomic) int columnsAllocated;

@property (nonatomic) TrackContextMenu *trackContextMenu;

@property (nonatomic) PlaylistDataModel *dataModel;

@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;

@end

@implementation PlaylistViewController

- (void)dealloc
{
    if (_artwork_plugin != NULL) {
        _artwork_plugin->remove_listener (artwork_listener, (__bridge void *)self);
        _artwork_plugin = NULL;
    }
    self.trackContextMenu = nil;
    [self cleanup];
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

    [self freeColumns];

    // don't wait for an automatic autorelease,
    // this would cause deadbeef's track refcount checker to run before the objects are really released
    @autoreleasepool {
        self.trkProperties = nil;
    }
}

static void
artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    PlaylistViewController *self = (__bridge PlaylistViewController *)user_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [CoverManager.defaultCoverManager resetCache];
        PlaylistView *listview = (PlaylistView *)self.view;
        listview.contentView.needsDisplay = YES;
    });
}

- (void)menuAddColumn:(id)sender {
    if (!self.editColumnWindowController) {
        self.editColumnWindowController = [[EditColumnWindowController alloc] initWithWindowNibName:@"EditColumnPanel"];
    }

    [self.editColumnWindowController initAddColumnSheet];

    [self.view.window beginSheet:self.editColumnWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            int idx = [self insertColumn:self.menuColumn];
            if (idx >= 0) {
                [self updateColumn:idx];
                PlaylistView *listview = (PlaylistView *)self.view;
                [listview.contentView reloadData];
            }
        }
    }];
}

- (void)menuEditColumn:(id)sender {
    if (!self.editColumnWindowController) {
        self.editColumnWindowController = [[EditColumnWindowController alloc] initWithWindowNibName:@"EditColumnPanel"];
    }

    uint8_t *c = self.columns[self.menuColumn].text_color;
    NSColor *color = [NSColor colorWithDeviceRed:c[0]/255.f green:c[1]/255.f blue:c[2]/255.f alpha:c[3]/255.f];

    [self.editColumnWindowController initEditColumnSheetWithTitle:[NSString stringWithUTF8String:self.columns[self.menuColumn].title]
                                                             type:self.columns[self.menuColumn].type
                                                           format:[NSString stringWithUTF8String:self.columns[self.menuColumn].format]
                                                        alignment:self.columns[self.menuColumn].alignment
                                                     setTextColor:self.columns[self.menuColumn].set_text_color
                                                        textColor:color];


    [self.view.window beginSheet:self.editColumnWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            int idx = self.menuColumn;
            if (idx >= 0) {
                [self updateColumn:idx];
                PlaylistView *listview = (PlaylistView *)self.view;
                [listview.contentView reloadData];
            }
        }
    }];
}


- (void)menuRemoveColumn:(id)sender {
    if (self.menuColumn >= 0) {
        [self removeColumnAtIndex:self.menuColumn];
        [self columnsChanged];
        PlaylistView *listview = (PlaylistView *)self.view;
        [listview.contentView reloadData];
    }
}

- (void)menuTogglePinGroups:(NSButton *)sender {
    self.pinGroups = sender.state == NSControlStateValueOn ? 0 : 1;
    sender.state = self.pinGroups?NSControlStateValueOn:NSControlStateValueOff;
    deadbeef->conf_set_int ([self pinGroupsConfStr], self.pinGroups);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView updatePinnedGroup];
}

- (void)clearGrouping {
    self.groupStr = NULL;
    if (self.groupBytecode) {
        deadbeef->tf_free (self.groupBytecode);
        self.groupBytecode = NULL;
    }
}

- (void)menuGroupByNone:(id)sender {
    [self clearGrouping];
    deadbeef->conf_remove_items (self.groupByConfStr);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView reloadData];
    deadbeef->conf_save ();
}

- (void)menuGroupByArtistDateAlbum:(id)sender {
    [self clearGrouping];
    self.groupStr = @"%album artist% - ['['%year%']' ]%album%";
    deadbeef->conf_set_str (self.groupByConfStr, self.groupStr.UTF8String);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView reloadData];
    deadbeef->conf_save ();
}

- (void)menuGroupByArtist:(id)sender {
    [self clearGrouping];
    self.groupStr = @"%artist%";
    deadbeef->conf_set_str (self.groupByConfStr, self.groupStr.UTF8String);
    PlaylistView *lv = (PlaylistView *)self.view;
    [lv.contentView reloadData];
    deadbeef->conf_save ();
}

- (void)menuGroupByCustom:(id)sender {
    if (!self.groupByCustomWindowController) {
        self.groupByCustomWindowController = [[GroupByCustomWindowController alloc] initWithWindowNibName:@"GroupByCustomWindow"];
    }

    char buf[1000];
    deadbeef->conf_get_str (self.groupByConfStr, "", buf, sizeof (buf));
    [self.groupByCustomWindowController initWithFormat:[NSString stringWithUTF8String:buf]];

    [self.view.window beginSheet:self.groupByCustomWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            [self clearGrouping];
            self.groupStr = self.groupByCustomWindowController.formatTextField.stringValue;
            deadbeef->conf_set_str([self groupByConfStr], self.groupStr.UTF8String);
            PlaylistView *lv = (PlaylistView *)self.view;
            [lv.contentView reloadData];
            deadbeef->conf_save ();
        }
    }];
}

- (void)columnsChanged {
    PlaylistView *lv = (PlaylistView *)self.view;
    lv.headerView.needsDisplay = YES;
    lv.contentView.needsDisplay = YES;

    NSMutableArray *columns = [[NSMutableArray alloc] initWithCapacity:self.ncolumns];
    for (int i = 0; i < self.ncolumns; i++) {
        uint8_t *col = self.columns[i].text_color;
        NSDictionary *dict = [[NSDictionary alloc] initWithObjectsAndKeys:
                              [NSString stringWithUTF8String:self.columns[i].title], @"title"
                              , [NSString stringWithFormat:@"%d", self.columns[i].type], @"id"
                              , [NSString stringWithUTF8String:self.columns[i].format], @"format"
                              , [NSString stringWithFormat:@"%d", self.columns[i].size], @"size"
                              , [NSNumber numberWithInt:self.columns[i].alignment], @"alignment"
                              , [NSNumber numberWithInt:self.columns[i].set_text_color], @"set_text_color"
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
            withSize:self.columns[idx].size
          withFormat:self.editColumnWindowController.formatTextField.stringValue.UTF8String
       withAlignment:(int)self.editColumnWindowController.alignmentPopUpButton.indexOfSelectedItem withSetColor:(self.editColumnWindowController.setColorButton.state == NSControlStateValueOn)
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
    self.playTpl = [[NSImage imageNamed:@"btnplayTemplate.pdf"] flippedImage];
    self.pauseTpl = [[NSImage imageNamed:@"btnpauseTemplate.pdf"] flippedImage];
    self.bufTpl = [[NSImage imageNamed:@"bufferingTemplate.pdf"] flippedImage];

    NSMutableParagraphStyle *textStyle = [NSParagraphStyle.defaultParagraphStyle mutableCopy];
    textStyle.alignment = NSTextAlignmentLeft;
    textStyle.lineBreakMode = NSLineBreakByTruncatingTail;


    int rowheight = 18;

    self.groupTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                 , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                 , NSColor.controlTextColor, NSForegroundColorAttributeName
                                 , textStyle, NSParagraphStyleAttributeName
                                 , nil];

    self.cellTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                , NSColor.controlTextColor, NSForegroundColorAttributeName
                                , textStyle, NSParagraphStyleAttributeName
                                , nil];

    self.cellSelectedTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                        , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                        , NSColor.alternateSelectedControlTextColor, NSForegroundColorAttributeName
                                        , textStyle, NSParagraphStyleAttributeName
                                        , nil];


    // initialize group by str
    deadbeef->conf_lock ();
    const char *group_by = deadbeef->conf_get_str_fast (self.groupByConfStr, NULL);
    if (group_by) {
        self.groupStr = [NSString stringWithUTF8String:group_by];
    }
    deadbeef->conf_unlock ();

    self.pinGroups = deadbeef->conf_get_int ([self pinGroupsConfStr], 0);
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
    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    _artwork_plugin->add_listener (artwork_listener, (__bridge void *)self);

    PlaylistView *lv = (PlaylistView *)self.view;
    lv.delegate = self;
    self.dataModel = [[PlaylistDataModel alloc] initWithIter:self.playlistIter];
    lv.dataModel = self.dataModel;

    self.trackContextMenu = [TrackContextMenu new];
    self.trackContextMenu.view = self.view;
    self.trackContextMenu.delegate = self;

    self.sortColumn = -1;

    [self initContent];
    [self setupPlaylist:lv];
}

- (void)freeColumns {
    for (int i = 0; i < self.ncolumns; i++) {
        if (self.columns[i].title) {
            free (self.columns[i].title);
        }
        if (self.columns[i].format) {
            free (self.columns[i].format);
        }
        if (self.columns[i].bytecode) {
            deadbeef->tf_free (self.columns[i].bytecode);
        }
    }
    free (self.columns);
    self.columns = NULL;
    self.ncolumns = 0;
    self.columnsAllocated = 0;
}

- (int)insertColumn:(int)beforeIdx {
    if (self.ncolumns >= self.columnsAllocated) {
        self.columnsAllocated += 10;
        self.columns = realloc(self.columns, sizeof (plt_col_info_t) * self.columnsAllocated);
    }

    if (beforeIdx >= 0 && beforeIdx < self.ncolumns) {
        memmove (&self.columns[beforeIdx+1], &self.columns[beforeIdx], (self.ncolumns - beforeIdx) * sizeof (plt_col_info_t));
    }
    self.ncolumns++;
    int idx = beforeIdx >= 0 ? beforeIdx : self.ncolumns-1;
    self.columns[idx].size = 100;
    return idx;
}

- (void)initColumn:(int)idx withTitle:(const char *)title withId:(int)_id withSize:(int)size withFormat:(const char *)format withAlignment:(int)alignment withSetColor:(BOOL)setColor withColor:(uint8_t *)color {
    self.columns[idx].type = _id;
    self.columns[idx].title = strdup (title);
    self.columns[idx].format = format ? strdup (format) : NULL;
    self.columns[idx].size = size;
    self.columns[idx].alignment = alignment;
    if (format) {
        self.columns[idx].bytecode = deadbeef->tf_compile (format);
    }
    self.columns[idx].set_text_color = setColor;
    self.columns[idx].text_color[0] = color[0];
    self.columns[idx].text_color[1] = color[1];
    self.columns[idx].text_color[2] = color[2];
    self.columns[idx].text_color[3] = color[3];
}

- (void)removeColumnAtIndex:(int)idx {
    char *sortColumnTitle = NULL;
    if (self.sortColumn >= 0) {
        sortColumnTitle = self.columns[self.sortColumn].title;

    }

    if (idx != self.ncolumns-1) {
        memmove (&self.columns[idx], &self.columns[idx+1], (self.ncolumns-idx) * sizeof (plt_col_info_t));
    }
    self.ncolumns--;

    self.sortColumn = [self columnIndexForTitle:sortColumnTitle];
}

// pass col=-1 for "empty space", e.g. when appending new col
- (NSMenu *)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view {
    self.menuColumn = (int)col;
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"ColumnMenu"];
    menu.delegate = self;
    menu.autoenablesItems = NO;
    [menu insertItemWithTitle:@"Add Column" action:@selector(menuAddColumn:) keyEquivalent:@"" atIndex:0].target = self;
    if (col != -1) {
        [menu insertItemWithTitle:@"Edit Column" action:@selector(menuEditColumn:) keyEquivalent:@"" atIndex:1].target = self;
        [menu insertItemWithTitle:@"Remove Column" action:@selector(menuRemoveColumn:) keyEquivalent:@"" atIndex:2].target = self;
        NSMenuItem *item = [menu insertItemWithTitle:@"Pin Groups When Scrolling" action:@selector(menuTogglePinGroups:) keyEquivalent:@"" atIndex:3];
        item.state = self.pinGroups?NSControlStateValueOn:NSControlStateValueOff;
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
    return self.columns[col].type == DB_COLUMN_ALBUM_ART;
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

        int colId = -1;
        if (id_s) {
            colId = (int)[id_s integerValue];
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

        int colIdx = self.ncolumns;
        [self insertColumn:self.ncolumns];
        [self initColumn:colIdx withTitle:title withId:colId withSize:size withFormat:fmt withAlignment:alignment withSetColor:setcolor withColor:rgba];
    }];
}

- (int)columnCount {
    return self.ncolumns;
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
    return self.columns[col].size;
}

- (int)columnGroupHeight:(DdbListviewCol_t)col {
    return self.columns[col].size - ART_PADDING_HORZ*2;
}

- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col {
    self.columns[col].size = width;
}

- (int)columnMinHeight:(DdbListviewCol_t)col {
    return self.columns[col].type == DB_COLUMN_ALBUM_ART;
}

- (int)columnIndexForTitle:(const char *)title {
    for (int i = 0; i < self.ncolumns; i++) {
        if (self.columns[i].title == title) {
            return i;
        }
    }
    return -1;
}

- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to {
    plt_col_info_t tmp;

    char *sortColumnTitle = NULL;
    if (self.sortColumn >= 0) {
        sortColumnTitle = self.columns[self.sortColumn].title;

    }

    while (col < to) {
        memcpy (&tmp, &self.columns[col], sizeof (plt_col_info_t));
        memmove (&self.columns[col], &self.columns[col+1], sizeof (plt_col_info_t));
        memcpy (&self.columns[col+1], &tmp, sizeof (plt_col_info_t));
        col++;
    }
    while (col > to) {
        memcpy (&tmp, &self.columns[col], sizeof (plt_col_info_t));
        memmove (&self.columns[col], &self.columns[col-1], sizeof (plt_col_info_t));
        memcpy (&self.columns[col-1], &tmp, sizeof (plt_col_info_t));
        col--;
    }

    self.sortColumn = [self columnIndexForTitle:sortColumnTitle];
}

- (NSMutableAttributedString *)stringWithTintAttributesFromString:(const char *)inputString initialAttributes:(NSDictionary *)attributes foregroundColor:(NSColor *)foregroundColor backgroundColor:(NSColor *)backgroundColor {
    const int maxTintStops = 100;
    tint_stop_t tintStops[maxTintStops];
    NSUInteger numTintStops;
    char * plainString;

    numTintStops = calculate_tint_stops_from_string (inputString, tintStops, maxTintStops, &plainString);

    NSMutableAttributedString *str = [[NSMutableAttributedString alloc] initWithString:[NSString stringWithUTF8String:plainString] attributes:attributes];

    NSColor *highlightColor = NSColor.alternateSelectedControlColor;

    // add attributes
    for (NSUInteger i = 0; i < numTintStops; i++) {
        int index0 = tintStops[i].index;
        NSUInteger len = str.length - index0;

        NSColor *finalColor = foregroundColor;
        if (tintStops[i].has_rgb) {
            finalColor = [NSColor colorWithRed:tintStops[i].r/255.0 green:tintStops[i].g/255.0 blue:tintStops[i].b/255.0 alpha:1];
        }

        int tint = tintStops[i].tint;
        if (tint < 0) {
            tint = MAX(tint, -3);
            const CGFloat factors[] = {.30f, .60f, .80f};
            CGFloat blend = factors[3+tint];
            finalColor = [backgroundColor blendedColorWithFraction:blend ofColor:finalColor];
        }
        else if (tint > 0) {
            tint = MIN (tint, 3);
            const CGFloat factors[] = {0, .25f, .50f};
            CGFloat blend = factors[tint-1];
            finalColor = [finalColor blendedColorWithFraction:blend ofColor:highlightColor];
        }

        [str addAttributes:@{
            NSForegroundColorAttributeName:finalColor
        } range:NSMakeRange(index0, len)];
    }

    free (plainString);

    return str;
}

- (void)drawCell:(NSUInteger)idx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused {
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

    if (self.columns[col].type == DB_COLUMN_PLAYING && playing_track && (DB_playItem_t *)row == playing_track) {
        NSImage *img = NULL;
        int paused = deadbeef->get_output ()->state () == DDB_PLAYBACK_STATE_PAUSED;
        int buffering = !deadbeef->streamer_ok_to_read (-1);
        if (paused) {
            img = self.pauseTpl;
        }
        else if (!buffering) {
            img = self.playTpl;
        }
        else {
            img = self.bufTpl;
        }

        NSColor *imgColor = sel ? NSColor.alternateSelectedControlTextColor : NSColor.controlTextColor;

        CGContextRef c = [[NSGraphicsContext currentContext] CGContext];
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

    if (self.columns[col].bytecode) {
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = (DB_playItem_t *)row,
            .plt = deadbeef->plt_get_curr (),
            .id = self.columns[col].type,
            .idx = (int)idx,
            .flags = DDB_TF_CONTEXT_HAS_ID|DDB_TF_CONTEXT_HAS_INDEX,
        };
        if (!sel) {
            ctx.flags |= DDB_TF_CONTEXT_TEXT_DIM;
        }

        char text[1024] = "";
        deadbeef->tf_eval (&ctx, self.columns[col].bytecode, text, sizeof (text));

        rect.origin.x += CELL_HPADDING;
        rect.size.width -= CELL_HPADDING;

        if (text[0]) {
            NSDictionary *attributes = sel?self.cellSelectedTextAttrsDictionary:self.cellTextAttrsDictionary;
            NSAttributedString *attrString;
            if (ctx.dimmed) {
                NSColor *foreground = attributes[NSForegroundColorAttributeName];

                attrString = [self stringWithTintAttributesFromString:text initialAttributes:attributes                                                     foregroundColor:foreground backgroundColor:background];
            }
            else {
                attrString = [[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:text] attributes:attributes];
            }
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
                    [lv.contentView drawRow:(int)idx];
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
    deadbeef->tf_eval (&ctx, self.groupBytecode, text, sizeof (text));

    NSMutableAttributedString *attrString = [self stringWithTintAttributesFromString:text initialAttributes:self.groupTextAttrsDictionary foregroundColor:self.groupTextAttrsDictionary[NSForegroundColorAttributeName] backgroundColor:NSColor.controlBackgroundColor];

    NSSize size = [attrString size];

    NSRect strRect = rect;
    strRect.origin.x += 5;
    strRect.origin.y = strRect.origin.y + strRect.size.height / 2 - size.height / 2;
    strRect.size.height = size.height;
    strRect.size.width -= 10;
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
    void *grp;
    CGFloat albumArtSpaceWidth;
} cover_avail_info_t;

static void coverAvailCallback (NSImage *img, void *user_data) {
    cover_avail_info_t *info = user_data;
    PlaylistViewController *ctl = (__bridge_transfer PlaylistViewController *)info->ctl;
    PlaylistView *lv = (PlaylistView *)ctl.view;

    PlaylistGroup *grp = (__bridge_transfer PlaylistGroup *)info->grp;
    if (grp != nil) {
        if (img != nil) {
            NSSize desiredSize = [CoverManager.defaultCoverManager artworkDesiredSizeForImageSize:img.size albumArtSpaceWidth:info->albumArtSpaceWidth];
            grp->cachedImage = [CoverManager.defaultCoverManager createCachedImage:img size:desiredSize];
        }
        else {
            grp->cachedImage = nil;
        }
        grp->hasCachedImage = YES;
    }

    [lv.contentView drawGroup:grp];
    free (info);
}


- (void)drawAlbumArtForGroup:(PlaylistGroup *)grp
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
    int art_height = height - ART_PADDING_VERT * 2 - lv.contentView.grouptitle_height;
    int art_width = width - ART_PADDING_HORZ * 2;

    if (art_width < 8 || art_height < 8 || !it) {
        return;
    }

    NSImage *image;

    if (grp->hasCachedImage) {
        image = grp->cachedImage;
    }
    else {
        cover_avail_info_t *info = calloc (sizeof (cover_avail_info_t), 1);
        info->ctl = (__bridge_retained void *)self;
        info->grp = (__bridge_retained void *)grp;

        info->albumArtSpaceWidth = art_width;

        image = [CoverManager.defaultCoverManager getCoverForTrack:it withCallbackWhenReady:coverAvailCallback withUserDataForCallback:info];

        if (image != nil) {
            // callback will not be called, release and free user data
            __unused PlaylistViewController *_ctl = (__bridge_transfer PlaylistViewController *)info->ctl;
            __unused PlaylistGroup *_grp = (__bridge_transfer PlaylistGroup *)info->grp;
            free (info);
        }
    }
    if (!image) {
        // FIXME: the problem here is that if the cover is not found (yet) -- it won't draw anything, but the rect is already invalidated, and will come out as background color
        return;
    }

    NSRect drawRect;

    int art_x = x + ART_PADDING_HORZ;
    CGFloat min_y = (pinned ? viewportY+lv.contentView.grouptitle_height : y) + ART_PADDING_VERT;
    CGFloat max_y = grp_next_y;

    CGFloat ypos = min_y;
    if (min_y + art_width + ART_PADDING_VERT >= max_y) {
        ypos = max_y - art_width - ART_PADDING_VERT;
    }

    NSSize size = image.size;
    NSSize desiredSize = [CoverManager.defaultCoverManager artworkDesiredSizeForImageSize:size albumArtSpaceWidth:art_width];

    if (size.width >= size.height) {
        drawRect = NSMakeRect(art_x, ypos, desiredSize.width, desiredSize.height);
    }
    else {
        plt_col_info_t *c = &self.columns[(int)col];
        if (c->alignment == 1) {
            art_x += art_width/2 - desiredSize.width/2;
        }
        else if (c->alignment == 2) {
            art_x += art_width-desiredSize.width;
        }
        drawRect = NSMakeRect(art_x, ypos, desiredSize.width, desiredSize.height);
    }

    if (!grp->cachedImage) {
        grp->cachedImage = [CoverManager.defaultCoverManager createCachedImage:image size:desiredSize];
        grp->hasCachedImage = YES;
    }

    [image drawInRect:drawRect];
}

- (void)selectionChanged:(DdbListviewRow_t)row {
    PlaylistView *lv = (PlaylistView *)self.view;
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, (uintptr_t)lv, DDB_PLAYLIST_CHANGE_SELECTION, 0);
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
                PlaylistView *listview = (PlaylistView *)self.view;
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
                    PlaylistView *listview = (PlaylistView *)self.view;
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
                PlaylistView *listview = (PlaylistView *)self.view;
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
                    PlaylistView *listview = (PlaylistView *)self.view;
                    [listview.contentView reloadData];
                });
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_SELECTION) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    PlaylistView *listview = (PlaylistView *)self.view;
                    if (ctx != (uintptr_t)listview) {
                        [listview.contentView reloadData];
                    }
                });
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_PLAYQUEUE) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    PlaylistView *listview = (PlaylistView *)self.view;
                    if (ctx != (uintptr_t)listview) {
                        listview.contentView.needsDisplay = YES;
                    }
                });
            }
        }
            break;
        case DB_EV_PLAYLISTSWITCHED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                PlaylistView *listview = (PlaylistView *)self.view;
                [self setupPlaylist:listview];
            });
        }
            break;
        case DB_EV_TRACKFOCUSCURRENT: {
            dispatch_async(dispatch_get_main_queue(), ^{
                PlaylistView *listview = (PlaylistView *)self.view;
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
                PlaylistView *listview = (PlaylistView *)self.view;
                [listview.contentView reloadData];
            });
        }
            break;
        case DB_EV_FOCUS_SELECTION: {
            if ([self playlistIter] != (int)p1) {
                break;
            }

            dispatch_async(dispatch_get_main_queue(), ^{
                PlaylistView *listview = (PlaylistView *)self.view;
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
    return 0;
}


- (void)sortColumn:(DdbListviewCol_t)column {
    plt_col_info_t *c = &self.columns[(int)column];
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();

    if (self.sortColumn != column) {
        self.columns[column].sort_order = 0;
    }
    self.sortColumn = (int)column;
    self.columns[column].sort_order = 1 - self.columns[column].sort_order;

    deadbeef->plt_sort_v2 (plt, PL_MAIN, c->type, c->format, self.columns[column].sort_order);
    deadbeef->plt_unref (plt);

    PlaylistView *lv = (PlaylistView *)self.view;
    lv.headerView.needsDisplay = YES;
}

- (void)dropItems:(int)from_playlist before:(DdbListviewRow_t)before indices:(uint32_t *)indices count:(int)count copy:(BOOL)copy {

    deadbeef->pl_lock ();
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    ddb_playlist_t *from = NULL;

    from = deadbeef->plt_get_for_idx(from_playlist);

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

                    if (abort) {
                        break;
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

- (void)dropPlayItems:(DdbListviewRow_t *)items before:(DdbListviewRow_t)before count:(int)count {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();

    deadbeef->pl_lock ();

    ddb_playItem_t *after = before ? deadbeef->pl_get_prev((ddb_playItem_t *)before, PL_MAIN) : deadbeef->plt_get_tail_item (plt, PL_MAIN);

    for (int i = 0; i < count; i++) {
        ddb_playItem_t *it = (ddb_playItem_t *)items[i];

        ddb_playItem_t *copy = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy (copy, it);
        deadbeef->plt_insert_item (plt, after, copy);
        deadbeef->pl_item_unref (copy);
        after = copy;
    }

    deadbeef->pl_unlock ();

    deadbeef->plt_save_config (plt);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (void)scrollChanged:(CGFloat)pos {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_set_scroll (plt, (int)pos);
        deadbeef->plt_unref (plt);
    }
}

- (int)sortColumnIndex {
    return self.sortColumn;
}

- (NSString *)columnTitleAtIndex:(NSUInteger)index {
    return [NSString stringWithUTF8String:self.columns[index].title];
}

- (enum ddb_sort_order_t)columnSortOrderAtIndex:(NSUInteger)index {
    return self.columns[index].sort_order;
}


- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view {
    ddb_playItem_t **tracks = NULL;

    ddb_playlist_t *plt = deadbeef->plt_get_curr();

    deadbeef->pl_lock ();

    ddb_playItem_t *current = deadbeef->streamer_get_playing_track ();
    int current_idx = -1;

    NSInteger count = deadbeef->plt_getselcount(plt);
    int all_idx = 0;
    if (count) {
        NSInteger idx = 0;
        tracks = calloc (sizeof (ddb_playItem_t *), count);

        ddb_playItem_t *it = deadbeef->plt_get_first (plt, self.playlistIter);
        while (it) {
            ddb_playItem_t *next = deadbeef->pl_get_next (it, self.playlistIter);
            if (current != NULL && it == current) {
                current_idx = all_idx;
            }
            if (deadbeef->pl_is_selected (it)) {
                tracks[idx++] = it;
            }
            else {
                deadbeef->pl_item_unref (it);
            }
            it = next;
            all_idx++;
        }
    }

    deadbeef->pl_unlock ();

    [self.trackContextMenu updateWithTrackList:tracks count:count playlist:plt currentTrack:current currentTrackIdx:current_idx];
    [self.trackContextMenu update:plt];

    if (current) {
        deadbeef->pl_item_unref (current);
        current = NULL;
    }

    if (plt) {
        deadbeef->plt_unref (plt);
        plt = NULL;
    }

    if (tracks) {
        for (NSInteger i = 0; i < count; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }
        free (tracks);
    }

    return self.trackContextMenu;
}

#pragma mark - TrackContextMenuDelegate

- (void)trackProperties {
    if (!self.trkProperties) {
        self.trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    self.trkProperties.playlist =  plt;
    deadbeef->plt_unref (plt);
    [self.trkProperties showWindow:self];
}

- (void)playlistChanged {
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (NSString *)rowGroupStr:(DdbListviewRow_t)row {
    if (!self.groupStr) {
        return nil;
    }
    if (!self.groupBytecode) {
        self.groupBytecode = deadbeef->tf_compile (self.groupStr.UTF8String);
    }

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = (DB_playItem_t *)row,
        .plt = deadbeef->plt_get_curr(),
    };
    char buf[1024];
    NSString *ret = @"";
    if (deadbeef->tf_eval (&ctx, self.groupBytecode, buf, sizeof (buf)) > 0) {
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

- (int)playlistIter {
    return PL_MAIN;
}

@end
