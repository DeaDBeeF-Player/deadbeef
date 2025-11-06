/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Oleksiy Yakovenko and other contributors

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
#import "TrackContextMenu.h"
#import "TrackPropertiesManager.h"
#import "tftintutil.h"
#import "DdbPlayItemPasteboardSerializer.h"
#import "Weakify.h"

#include <deadbeef/deadbeef.h>
#include "utf8.h"
#include "artwork.h"

#define CELL_HPADDING 4
#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

extern DB_functions_t *deadbeef;

@interface PlaylistViewController() <DdbListviewDelegate,TrackContextMenuDelegate,CoverManagerListener>

@property (nonatomic) NSImage *playTpl;
@property (nonatomic) NSImage *pauseTpl;
@property (nonatomic) NSImage *bufTpl;
@property (nonatomic) NSDictionary *cellTextAttrsDictionary;
@property (nonatomic) NSDictionary *cellSelectedTextAttrsDictionary;
@property (nonatomic) NSDictionary *groupTextAttrsDictionary;
@property (nonatomic) int menuColumn;

@property (nonatomic) char *groupBytecode;
@property (nonatomic) BOOL pinGroups;


@property (nonatomic, strong) NSTimer *playPosUpdateTimer;
@property (nonatomic, assign) DB_playItem_t *playPosUpdateTrack;
@property (nonatomic) EditColumnWindowController *editColumnWindowController;
@property (nonatomic) GroupByCustomWindowController *groupByCustomWindowController;
@property (nonatomic) int sortColumn;
@property (nonatomic,readonly) const char *groupByConfStr;
@property (nonatomic) NSString *groupStr;

@property (nonatomic,readwrite) plt_col_info_t *columns;
@property (nonatomic,readwrite) int ncolumns;
@property (nonatomic) int columnsAllocated;

@property (nonatomic) TrackContextMenu *trackContextMenu;

@property (nonatomic) PlaylistDataModel *dataModel;

@property (nonatomic) ddb_artwork_plugin_t *artwork_plugin;
@property (nonatomic) int64_t sourceId;

@end

@implementation PlaylistViewController

- (void)dealloc
{
    [CoverManager.shared removeListener:self];
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
}

#pragma mark - Responder Chain

- (IBAction)delete:(id)sender {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        int cursor = deadbeef->plt_delete_selected (plt);
        if (cursor != -1) {
            DB_playItem_t *it = deadbeef->plt_get_item_for_idx (plt, cursor, PL_MAIN);
            if (it) {
                deadbeef->pl_set_selected (it, 1);
                deadbeef->pl_item_unref (it);
            }
        }
        deadbeef->plt_save_config (plt);
        deadbeef->plt_unref (plt);
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }
}

- (IBAction)selectAll:(id)sender {
    deadbeef->pl_select_all ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

- (IBAction)deselectAll:(id)sender {
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
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

- (IBAction)invertSelectionAction:(id)sender {
    deadbeef->pl_lock ();
    DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
    while (it) {
        if (deadbeef->pl_is_selected (it)) {
            deadbeef->pl_set_selected (it, 0);
        }
        else {
            deadbeef->pl_set_selected (it, 1);
        }
        DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
        deadbeef->pl_item_unref (it);
        it = next;
    }
    deadbeef->pl_unlock ();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_SELECTION, 0);
}

#pragma mark -

static void
artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    PlaylistViewController *self = (__bridge PlaylistViewController *)user_data;
    ddb_playItem_t *it = (ddb_playItem_t *)p1;
    if (it != NULL) {
        deadbeef->pl_item_ref (it);
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        PlaylistView *listview = (PlaylistView *)self.view;
        [listview.contentView invalidateArtworkCacheForRow:(DdbListviewRow_t)it];
        if (it != NULL) {
            deadbeef->pl_item_unref (it);
        }
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
                listview.contentView.needsDisplay = YES;
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

    [self.editColumnWindowController initEditColumnSheetWithTitle:@(self.columns[self.menuColumn].title)
                                                             type:self.columns[self.menuColumn].type
                                                           format:@(self.columns[self.menuColumn].format ?: "")
                                                       sortFormat:@(self.columns[self.menuColumn].sortFormat ?: "")
                                                        alignment:self.columns[self.menuColumn].alignment
                                                     setTextColor:self.columns[self.menuColumn].set_text_color
                                                        textColor:color];


    [self.view.window beginSheet:self.editColumnWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            int idx = self.menuColumn;
            if (idx >= 0) {
                [self updateColumn:idx];
                PlaylistView *listview = (PlaylistView *)self.view;
                listview.contentView.needsDisplay = YES;
            }
        }
    }];
}


- (void)menuRemoveColumn:(id)sender {
    if (self.menuColumn >= 0) {
        [self removeColumnAtIndex:self.menuColumn];
        [self columnsDidChange];
        PlaylistView *listview = (PlaylistView *)self.view;
        listview.contentView.needsDisplay = YES;
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
    self.groupStr = @"%album artist% - ['['$year(%date%)']' ]%album%";
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
    [self.groupByCustomWindowController initWithFormat:@(buf)];

    [self.view.window beginSheet:self.groupByCustomWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK) {
            [self clearGrouping];
            self.groupStr = self.groupByCustomWindowController.formatTextField.stringValue;
            deadbeef->conf_set_str(self.groupByConfStr, self.groupStr.UTF8String);
            PlaylistView *lv = (PlaylistView *)self.view;
            [lv.contentView reloadData];
            deadbeef->conf_save ();
        }
    }];
}

- (void)columnsDidChange {
    PlaylistView *lv = (PlaylistView *)self.view;
    lv.headerView.needsDisplay = YES;
    lv.contentView.needsDisplay = YES;

    NSMutableArray *columns = [[NSMutableArray alloc] initWithCapacity:self.ncolumns];
    for (int i = 0; i < self.ncolumns; i++) {
        uint8_t *col = self.columns[i].text_color;
        NSDictionary *dict = @{@"title": @(self.columns[i].title)
                              , @"id": [NSString stringWithFormat:@"%d", self.columns[i].type]
                              , @"format": @(self.columns[i].format ?: "")
                              , @"sort_format": @(self.columns[i].sortFormat ?: "")
                              , @"size": [NSString stringWithFormat:@"%d", self.columns[i].size]
                              , @"alignment": @(self.columns[i].alignment)
                              , @"set_text_color": @(self.columns[i].set_text_color)
                              , @"text_color": [NSString stringWithFormat:@"#%02x%02x%02x%02x", col[3], col[0], col[1], col[2]]};
        [columns addObject:dict];
    }

    NSError *err = nil;
    NSData *dt = [NSJSONSerialization dataWithJSONObject:columns options:0 error:&err];

    [lv.contentView reloadData];

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

    [self initColumnAtIndex:idx
                      title:self.editColumnWindowController.titleTextField.stringValue.UTF8String
                 identifier:(int)type
                       size:self.columns[idx].size
                     format:self.editColumnWindowController.formatTextField.stringValue.UTF8String
                 sortFormat:self.editColumnWindowController.sortFormatTextField.stringValue.UTF8String
                  alignment:(int)self.editColumnWindowController.alignmentPopUpButton.indexOfSelectedItem shouldSetColor:(self.editColumnWindowController.setColorButton.state == NSControlStateValueOn)
                      color:rgba];
    [self columnsDidChange];
}

#define DEFAULT_COLUMNS "[{\"title\":\"Playing\", \"id\":\"1\", \"format\":\"%playstatus%\", \"size\":\"50\"}, {\"title\":\"Artist / Album\", \"format\":\"$if(%album artist%,%album artist%,Unknown Artist)[ - %album%]\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track number%\", \"size\":\"50\"}, {\"title\":\"Title / Track Artist\", \"format\":\"%title%[ // %track artist%]\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

- (NSString *)getColumnConfig {
    return conf_get_nsstr ("cocoaui.columns", DEFAULT_COLUMNS);
}

- (void)writeColumnConfig:(NSString *)config {
    deadbeef->conf_set_str ("cocoaui.columns", config.UTF8String);
}

- (void)initContent {
    NSString *cols = [self getColumnConfig];
    NSData *data = [cols dataUsingEncoding:NSUTF8StringEncoding];

    NSError *err = nil;
    NSArray *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

    if (!json) {
        NSLog (@"error parsing column config, error: %@\n", err.localizedDescription);
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

    self.groupTextAttrsDictionary = @{NSFontAttributeName: [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]]
                                 , NSBaselineOffsetAttributeName: @0.0f
                                 , NSForegroundColorAttributeName: NSColor.controlTextColor
                                 , NSParagraphStyleAttributeName: textStyle};

    self.cellTextAttrsDictionary = @{NSFontAttributeName: [NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]]
                                , NSBaselineOffsetAttributeName: @0.0f
                                , NSForegroundColorAttributeName: NSColor.controlTextColor
                                , NSParagraphStyleAttributeName: textStyle};

    self.cellSelectedTextAttrsDictionary = @{NSFontAttributeName: [NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]]
                                        , NSBaselineOffsetAttributeName: @0.0f
                                        , NSForegroundColorAttributeName: NSColor.alternateSelectedControlTextColor
                                        , NSParagraphStyleAttributeName: textStyle};


    // initialize group by str
    deadbeef->conf_lock ();
    const char *group_by = deadbeef->conf_get_str_fast (self.groupByConfStr, NULL);
    if (group_by) {
        self.groupStr = @(group_by);
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

- (void)loadView {
    self.view = [PlaylistView new];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    _artwork_plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    _artwork_plugin->add_listener (artwork_listener, (__bridge void *)self);

    self.sourceId = _artwork_plugin->allocate_source_id();

    PlaylistView *lv = (PlaylistView *)self.view;
    lv.delegate = self;
    self.dataModel = [[PlaylistDataModel alloc] initWithIter:self.playlistIter];
    lv.dataModel = self.dataModel;

    self.sortColumn = -1;

    [self initContent];
    [self setupPlaylist:lv];

    [CoverManager.shared addListener:self];
}

- (void)freeColumns {
    for (int i = 0; i < self.ncolumns; i++) {
        [self freeColumnDataAtIndex:i];
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
    memset (&self.columns[idx], 0, sizeof(plt_col_info_t));
    self.columns[idx].size = 100;
    return idx;
}

- (void)freeColumnDataAtIndex:(int)idx {
    free(self.columns[idx].title);
    self.columns[idx].title = NULL;
    free(self.columns[idx].format);
    self.columns[idx].format = NULL;
    free(self.columns[idx].sortFormat);
    self.columns[idx].sortFormat = NULL;
    if (self.columns[idx].bytecode) {
        deadbeef->tf_free (self.columns[idx].bytecode);
        self.columns[idx].bytecode = NULL;
    }
}

- (void)initColumnAtIndex:(int)idx title:(const char *)title identifier:(int)_id size:(int)size format:(const char *)format sortFormat:(const char *)sortFormat alignment:(PlaylistColumnAlignment)alignment shouldSetColor:(BOOL)shouldSetColor color:(uint8_t *)color {
    [self freeColumnDataAtIndex:idx];

    self.columns[idx].type = _id;
    self.columns[idx].title = strdup (title);
    self.columns[idx].format = format ? strdup (format) : NULL;
    self.columns[idx].sortFormat = (sortFormat && sortFormat[0]) ? strdup (sortFormat) : NULL;
    self.columns[idx].size = size;
    self.columns[idx].alignment = alignment;
    if (format) {
        self.columns[idx].bytecode = deadbeef->tf_compile (format);
    }
    self.columns[idx].set_text_color = shouldSetColor;
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
        NSString *title_s = dict[@"title"];
        NSString *id_s = dict[@"id"];
        NSString *format_s = dict[@"format"];
        NSString *sortFormat_s = dict[@"sort_format"];
        NSString *size_s = dict[@"size"];
        NSNumber *alignment_n = dict[@"alignment"];
        NSNumber *setcolor_n = dict[@"set_text_color"];
        NSString *textcolor_s = dict[@"text_color"];

        const char *title = "";
        if (title_s) {
            title = title_s.UTF8String;
        }

        int colId = -1;
        if (id_s) {
            colId = (int)id_s.integerValue;
        }

        const char *fmt = NULL;
        if (format_s) {
            fmt = format_s.UTF8String;
        }

        const char *sortfmt = NULL;
        if (sortFormat_s) {
            sortfmt = sortFormat_s.UTF8String;
        }

        int size = 80;
        if (size_s) {
            size = (int)size_s.integerValue;
        }

        PlaylistColumnAlignment alignment = ColumnAlignmentLeft;
        if (alignment_n) {
            alignment = (PlaylistColumnAlignment)alignment_n.intValue;
            
            if (alignment != ColumnAlignmentLeft && alignment != ColumnAlignmentCenter && alignment != ColumnAlignmentRight) {
                alignment = ColumnAlignmentLeft;
            }
        }
        

        BOOL setcolor = NO;
        if (setcolor_n) {
            setcolor = setcolor_n.intValue ? YES : NO;
        }

        int r = 0, g = 0, b = 0, a = 0xff;
        if (textcolor_s) {
            sscanf (textcolor_s.UTF8String, "#%02x%02x%02x%02x", &a, &r, &g, &b);
        }

        uint8_t rgba[4] = {
            r, g, b, a
        };

        int colIdx = self.ncolumns;
        [self insertColumn:self.ncolumns];
        [self initColumnAtIndex:colIdx
                          title:title
                     identifier:colId
                           size:size
                         format:fmt
                         sortFormat:sortfmt
                      alignment:alignment
                 shouldSetColor:setcolor
                          color:rgba];
    }];
}

- (int)columnCount {
    return self.ncolumns;
}

- (DdbListviewCol_t)firstColumn {
    return 0;
}

- (DdbListviewCol_t)nextColumn:(DdbListviewCol_t)col {
    return col >= self.columnCount - 1 ? self.invalidColumn : col+1;
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

    NSMutableAttributedString *str = [[NSMutableAttributedString alloc] initWithString:@(plainString) attributes:attributes];

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

    if (col == self.invalidColumn) {
        return;
    }

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track_safe ();

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

        CGContextRef c = [NSGraphicsContext currentContext].CGContext;
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
        rect.size.width -= CELL_HPADDING*2;

        if (text[0]) {
            NSDictionary *attributes = sel?self.cellSelectedTextAttrsDictionary:self.cellTextAttrsDictionary;

            // set text alignment for this specific column
            NSMutableParagraphStyle *textStyle = [attributes valueForKey:NSParagraphStyleAttributeName];
            switch (self.columns[col].alignment) {
                case ColumnAlignmentLeft:
                    textStyle.alignment = NSTextAlignmentLeft;
                    break;
                case ColumnAlignmentCenter:
                    textStyle.alignment = NSTextAlignmentCenter;
                    break;
                case ColumnAlignmentRight:
                    textStyle.alignment = NSTextAlignmentRight;
                    break;
                default:
                    textStyle.alignment = NSTextAlignmentLeft;
                    break;
            }

            NSAttributedString *attrString;
            if (ctx.dimmed) {
                NSColor *foreground = attributes[NSForegroundColorAttributeName];

                attrString = [self stringWithTintAttributesFromString:text initialAttributes:attributes                                                     foregroundColor:foreground backgroundColor:background];
            }
            else {
                attrString = [[NSAttributedString alloc] initWithString:@(text) attributes:attributes];
            }
            [attrString drawInRect:rect];
            textStyle.alignment = NSTextAlignmentLeft;
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

            if (self.view.window.isVisible
                && deadbeef->get_output()->state() == DDB_PLAYBACK_STATE_PLAYING) {
                weakify(self);
                self.playPosUpdateTimer = [NSTimer scheduledTimerWithTimeInterval:ctx.update/1000.0 repeats:NO block:^(NSTimer * _Nonnull timer) {
                    strongify(self);
                    if (self == nil) {
                        return;
                    }
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

    CGSize availableSize = [self.view convertSizeToBacking:CGSizeMake(art_width, art_height)];
    if (grp->hasCachedImage) {
        image = grp->cachedImage;
    }
    else {
        image = [CoverManager.shared coverForTrack:it sourceId:self.sourceId completionBlock:^(NSImage *img) {
            if (grp != nil) {
                if (img != nil) {
                    NSSize desiredSize = [CoverManager.shared desiredSizeForImageSize:img.size availableSize:availableSize];
                    grp->cachedImage = [CoverManager.shared createScaledImage:img newSize:desiredSize];
                }
                else {
                    grp->cachedImage = nil;
                }
                grp->hasCachedImage = YES;
            }

            [lv.contentView drawGroup:grp];
        }];
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
    NSSize desiredSize = [CoverManager.shared desiredSizeForImageSize:size availableSize:availableSize];
    CGSize drawSize = [self.view convertSizeFromBacking:desiredSize];
    
    if (size.width < size.height) {
        plt_col_info_t *c = &self.columns[(int)col];
        if (c->alignment == ColumnAlignmentCenter) {
            art_x += art_width/2 - drawSize.width/2;
        }
        else if (c->alignment == ColumnAlignmentRight) {
            art_x += art_width-drawSize.width;
        }
    }
    drawRect = NSMakeRect(art_x, ypos, drawSize.width, drawSize.height);

    if (!grp->cachedImage) {
        grp->cachedImage = [CoverManager.shared createScaledImage:image newSize:desiredSize];
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

        // there's a delay in scrollview layout
        dispatch_after((dispatch_time_t)(0.01 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            [listview.contentView scrollVerticalPosition:scroll];
        });
    }
}

- (void)configChanged {
    PlaylistView *listview = (PlaylistView *)self.view;
    [listview.contentView configChanged];
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
                DB_playItem_t *curr = deadbeef->streamer_get_playing_track_safe ();
                if (curr) {
                    int idx = deadbeef->pl_get_idx_of (curr);
                    [listview.contentView drawRow:idx];
                    deadbeef->pl_item_unref (curr);
                }
            });
        }
            break;
        case DB_EV_PLAYLISTCHANGED: {
            if (p1 == 0) {
                // a change requiring full reload -- such as adding/removing a track
                dispatch_async(dispatch_get_main_queue(), ^{
                    PlaylistView *listview = (PlaylistView *)self.view;
                    [listview.contentView reloadData];
                });
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_SEARCHRESULT) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    PlaylistView *listview = (PlaylistView *)self.view;
                    if ([self playlistIter] == PL_SEARCH) {
                        [listview.contentView reloadData];
                    }
                });
            }
            else if (p1 == DDB_PLAYLIST_CHANGE_SELECTION) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    PlaylistView *listview = (PlaylistView *)self.view;
                    if (ctx != (uintptr_t)listview) {
                        listview.contentView.needsDisplay = YES;
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
                DB_playItem_t *it = deadbeef->streamer_get_playing_track_safe ();
                if (it) {
                    deadbeef->pl_lock ();
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
                    }

                    int idx = deadbeef->plt_get_item_idx (plt, it, [self playlistIter]);
                    if (idx != -1) {
                        // there's a delay in scrollview layout
                        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.01 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                            [listview.contentView setCursor:idx noscroll:YES];
                            [listview.contentView scrollToRowWithIndex:idx];
                        });
                    }
                    deadbeef->plt_unref (plt);
                    deadbeef->plt_unref (prev_plt);
                    deadbeef->pl_unlock ();
                    deadbeef->pl_item_unref (it);
                }
            });
        }
            break;
        case DB_EV_CONFIGCHANGED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                [self configChanged];
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

    deadbeef->plt_sort_v2 (plt, PL_MAIN, c->type, (c->sortFormat && c->sortFormat[0]) ? c->sortFormat : c->format, self.columns[column].sort_order);
    deadbeef->plt_unref (plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

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
    deadbeef->pl_save_all();
}

-(void)externalDropItems:(NSArray *)paths after:(DdbListviewRow_t)_after  completionBlock:(nonnull void (^) (void))completionBlock {
    ddb_playlist_t *plt = deadbeef->plt_alloc("drag-drop-playlist");
    ddb_playlist_t *plt_curr = deadbeef->plt_get_curr ();
    if (!deadbeef->plt_add_files_begin (plt_curr, 0)) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            DB_playItem_t *first = NULL;
            DB_playItem_t *after = NULL;
            int abort = 0;
            for(NSUInteger i = 0; i < paths.count; i++ )
            {
                NSString* path = paths[i];
                if (path) {
                    const char *fname = path.UTF8String;
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
                    }

                    if (abort) {
                        break;
                    }
                }
            }
            if (after) {
                deadbeef->pl_item_unref (after);
            }
            deadbeef->plt_add_files_end (plt_curr, 0);
            if (abort) {
                deadbeef->plt_unref (plt);
                deadbeef->plt_unref (plt_curr);
            }
            else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    // move items to UI playlist
                    deadbeef->plt_move_all_items(plt_curr, plt, (ddb_playItem_t *)_after);
                    // TODO: set cursor to the first dropped item

                    deadbeef->plt_unref (plt);
                    deadbeef->plt_unref (plt_curr);
                    deadbeef->pl_save_current();
                    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);

                    completionBlock();
                });
            }
        });
    }
    else {
        if (_after) {
            deadbeef->pl_item_unref ((DB_playItem_t *)_after);
        }
        deadbeef->plt_unref (plt);
    }
    deadbeef->pl_save_all();
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
    deadbeef->pl_save_all();
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
    return @(self.columns[index].title);
}

- (enum ddb_sort_order_t)columnSortOrderAtIndex:(NSUInteger)index {
    return self.columns[index].sort_order;
}


- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view {
    ddb_playItem_t **tracks = NULL;

    ddb_playlist_t *plt = deadbeef->plt_get_curr();

    ddb_playItem_t *current = deadbeef->streamer_get_playing_track_safe ();

    deadbeef->pl_lock ();

    int current_idx = -1;

    NSInteger count = deadbeef->plt_getselcount(plt);
    int all_idx = 0;
    if (count) {
        NSInteger idx = 0;
        tracks = calloc (count, sizeof (ddb_playItem_t *));

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

    self.trackContextMenu = [[TrackContextMenu alloc] initWithView:self.view];
    self.trackContextMenu.delegate = self;

    [self.trackContextMenu updateWithTrackList:tracks count:count playlist:plt currentTrack:current currentTrackIdx:current_idx];
    [self.trackContextMenu update:plt actionContext:DDB_ACTION_CTX_SELECTION];

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

#pragma mark - Copy & paste

- (void)copyAndDeleteSelected:(BOOL)deleteSelected {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();

    ddb_playItem_t **items = NULL;
    ssize_t count = deadbeef->plt_get_selected_items (plt, &items);
    if (count == 0) {
        deadbeef->plt_unref (plt);
        return;
    }

    DdbPlayItemPasteboardSerializer *holder = [[DdbPlayItemPasteboardSerializer alloc] initWithItems:items count:count];

    for (ssize_t index = 0; index < count; index++) {
        deadbeef->pl_item_unref (items[index]);
    }
    free (items);

    NSPasteboard *pasteboard = NSPasteboard.generalPasteboard;
    [pasteboard clearContents];
    [pasteboard writeObjects:@[holder]];

    if (deleteSelected) {
        deadbeef->plt_delete_selected (plt);
    }

    deadbeef->plt_unref (plt);

    if (deleteSelected) {
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }
}

- (void)cut:(id)sender {
    [self copyAndDeleteSelected:YES];
}

- (void)copy:(id)sender {
    [self copyAndDeleteSelected:NO];
}

- (void)paste:(id)sender {
    NSPasteboard *pasteboard = NSPasteboard.generalPasteboard;
    NSArray *copiedItems = [pasteboard readObjectsForClasses:@[DdbPlayItemPasteboardSerializer.class] options:@{}];
    if (copiedItems != nil) {
        DdbPlayItemPasteboardSerializer *holder = copiedItems.firstObject;
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        int cursor = deadbeef->plt_get_cursor (plt, PL_MAIN);
        if (cursor == -1) {
            cursor = 0;
        }
        ddb_playItem_t *before = deadbeef->plt_get_item_for_idx (plt, cursor, PL_MAIN);

        ssize_t count = 0;
        if (holder.plt != NULL) {
            ddb_playItem_t **items;
            count = deadbeef->plt_get_items(holder.plt, &items);

            [self dropPlayItems:(DdbListviewRow_t *)items before:(DdbListviewRow_t)before count:(int)count];

            for (ssize_t i = 0; i < count; i++) {
                deadbeef->pl_item_unref(items[i]);
            }
            free (items);
        }

        if (before != NULL) {
            deadbeef->pl_item_unref (before);
        }

        deadbeef->plt_deselect_all (plt);
        deadbeef->plt_set_cursor (plt, PL_MAIN, (int)(cursor + count));
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, (uintptr_t)self.view, DDB_PLAYLIST_CHANGE_SELECTION, 0);

        // TODO: scroll to cursor

        deadbeef->plt_unref (plt);
    }
}

#pragma mark - TrackContextMenuDelegate

- (void)trackContextMenuShowTrackProperties:(TrackContextMenu *)trackContextMenu {
    [TrackPropertiesManager.shared displayTrackProperties];
}

- (void)trackContextMenuDidReloadMetadata:(TrackContextMenu *)trackContextMenu {
    deadbeef->pl_save_current();
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

- (void)trackContextMenuDidDeleteFiles:(TrackContextMenu *)trackContextMenu cancelled:(BOOL)cancelled {
    if (!cancelled) {
        deadbeef->pl_save_all ();
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }
}

#pragma mark -

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
        ret = @(buf);
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

#pragma mark - CoverManagerListener

- (void)coverManagerDidReset {
    PlaylistView *listview = (PlaylistView *)self.view;
    [listview.contentView coverManagerDidReset];
}

@end
