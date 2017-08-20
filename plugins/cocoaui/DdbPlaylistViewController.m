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

#import "DdbPlaylistViewController.h"
#import "DdbPlaylistWidget.h"
#import "DdbListview.h"
#import "ConverterWindowController.h"
#import "CoverManager.h"
#import "ReplayGainScannerController.h"
#include "../../deadbeef.h"
#include "rg_scanner.h"

#define CELL_HPADDING 4

extern DB_functions_t *deadbeef;

@interface DdbPlaylistViewController()
@end

@implementation DdbPlaylistViewController {
    char *_group_str;
    char *_group_bytecode;
    BOOL _pin_groups;
}

- (void)dealloc {
    [self cleanup];
}

- (void)cleanup {
    DdbPlaylistWidget *view = (DdbPlaylistWidget *)[self view];
    [[view listview] cleanup];
    [self clearGrouping];
}

- (void)menuAddColumn:(id)sender {
    [self initAddColumnSheet:-1];
    [NSApp beginSheet:self.addColumnPanel modalForWindow:[[self view] window]  modalDelegate:self didEndSelector:@selector(didEndAddColumn:returnCode:contextInfo:) contextInfo:nil];
}

- (void)menuEditColumn:(id)sender {
    [self initAddColumnSheet:_menuColumn];
    [NSApp beginSheet:self.addColumnPanel modalForWindow:[[self view] window]  modalDelegate:self didEndSelector:@selector(didEndEditColumn:returnCode:contextInfo:) contextInfo:nil];
}

- (void)initAddColumnSheet:(int)colIdx {
    if (colIdx == -1) {
        [_addColumnTitle setStringValue:@""];
        [_addColumnType selectItemAtIndex: 10];
        [_addColumnFormat setEnabled:YES];
        [_addColumnFormat setStringValue:@""];
        [_addColumnAlignment setIntValue:0];
        [_addColumnSetColor setState:NSOffState];
        [_addColumnColor setEnabled:NO];
        [_addColumnColor setColor:[NSColor blackColor]];
    }
    else {
        [_addColumnTitle setStringValue:[NSString stringWithUTF8String:_columns[colIdx].title]];
        int type = 10; // custom
        switch (_columns[colIdx].type) {
        case DB_COLUMN_FILENUMBER:
            type = 0;
            break;
        case DB_COLUMN_PLAYING:
            type = 1;
            break;
        case DB_COLUMN_ALBUM_ART:
            type = 2;
            break;
        }
        [_addColumnType selectItemAtIndex: type];
        [_addColumnFormat setEnabled:type == 10];
        [_addColumnFormat setStringValue:[NSString stringWithUTF8String:_columns[colIdx].format]];
        [_addColumnAlignment setIntValue:_columns[colIdx].alignment];
        [_addColumnSetColor setState:_columns[colIdx].set_text_color];
        [_addColumnColor setEnabled:_columns[colIdx].set_text_color];
        uint8_t *c = _columns[colIdx].text_color;
        [[NSColorPanel sharedColorPanel] setShowsAlpha:YES];
        [_addColumnColor setColor:[NSColor colorWithDeviceRed:c[0]/255.f green:c[1]/255.f blue:c[2]/255.f alpha:c[3]/255.f]];
    }
}

- (IBAction)addColumnTypeChanged:(id)sender {
    BOOL isCustom = [_addColumnType indexOfSelectedItem] == 10;
    [_addColumnFormat setEnabled: isCustom];
    [_addColumnTitle setStringValue:[[_addColumnType selectedItem] title]];
}

- (IBAction)addColumnSetColorChanged:(id)sender {
    [_addColumnColor setEnabled:[sender state] == NSOnState];
}

- (void)menuRemoveColumn:(id)sender {
    if (_menuColumn >= 0) {
        [self removeColumnAtIndex:_menuColumn];
        [[self view] setNeedsDisplay:YES];
        [self columnsChanged];
    }
}

- (void)menuTogglePinGroups:(id)sender {
    _pin_groups = [sender state] == NSOnState ? 0 : 1;
    [sender setState:_pin_groups?NSOnState:NSOffState];
    deadbeef->conf_set_int ([self pinGroupsConfStr], _pin_groups);
    [[self view] setNeedsDisplay:YES];
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
    DdbPlaylistWidget *view = (DdbPlaylistWidget *)[self view];
    [[view listview] reloadData];
}

- (void)menuGroupByArtistDateAlbum:(id)sender {
    [self clearGrouping];
    _group_str = strdup ("%album artist% - ['['%year%']' ]%album%");
    deadbeef->conf_set_str ([self groupByConfStr], _group_str);
    DdbPlaylistWidget *view = (DdbPlaylistWidget *)[self view];
    [[view listview] reloadData];
}

- (void)menuGroupByArtist:(id)sender {
    [self clearGrouping];
    _group_str = strdup ("%artist%");
    deadbeef->conf_set_str ([self groupByConfStr], _group_str);
    DdbPlaylistWidget *view = (DdbPlaylistWidget *)[self view];
    [[view listview] reloadData];
}

- (void)menuGroupByCustom:(id)sender {
    // TODO
}

- (void)updateColumn:(int)idx {
    CGFloat r, g, b, a;
    NSColor *color = [[_addColumnColor color] colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace
]];
    [color getRed:&r green:&g blue:&b alpha:&a];

    uint8_t rgba[] = {
        r*255,g*255,b*255,a*255
    };

    int type = -1;

    switch ([_addColumnType indexOfSelectedItem]) {
    case 0:
        type = DB_COLUMN_FILENUMBER;
        break;
    case 1:
        type = DB_COLUMN_PLAYING;
        break;
    case 2:
        type = DB_COLUMN_ALBUM_ART;
        break;
    case 3: // artist - album
        [_addColumnFormat setStringValue:@"$if(%artist%,%artist%,Unknown Artist)[ - %album%]"];
        break;
    case 4: // artist
        [_addColumnFormat setStringValue:@"$if(%artist%,%artist%,Unknown Artist)"];
        break;
    case 5: // album
        [_addColumnFormat setStringValue:@"%album%"];
        break;
    case 6: // title
        [_addColumnFormat setStringValue:@"%title%"];
        break;
    case 7: // duration
        [_addColumnFormat setStringValue:@"%length%"];
        break;
    case 8: // track number
        [_addColumnFormat setStringValue:@"%track number%"];
        break;
    case 9: // album artist
        [_addColumnFormat setStringValue:@"$if(%album artist%,%album artist%,Unknown Artist)"];
        break;
    }

    [self initColumn:idx withTitle:[[_addColumnTitle stringValue] UTF8String]  withId:(int)type withSize:_columns[idx].size withFormat:[[_addColumnFormat stringValue] UTF8String] withAlignment:(int)[_addColumnAlignment indexOfSelectedItem] withSetColor:[_addColumnSetColor state] == NSOnState withColor:rgba];
    [[self view] setNeedsDisplay:YES];
    [self columnsChanged];
}

- (void)didEndAddColumn:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    [sheet orderOut:self];

    if (returnCode == NSOKButton) {
        int idx = [self insertColumn:_menuColumn];
        if (idx >= 0) {
            [self updateColumn:idx];
        }
    }
}

- (void)didEndEditColumn:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    [sheet orderOut:self];

    if (returnCode == NSOKButton) {
        int idx = _menuColumn;
        if (idx >= 0) {
            [self updateColumn:idx];
        }
    }
}

- (IBAction)addColumnCancel:(id)sender {
    [NSApp endSheet:self.addColumnPanel returnCode:NSCancelButton];
}

- (IBAction)addColumnOK:(id)sender {
    [NSApp endSheet:self.addColumnPanel returnCode:NSOKButton];
}

#define DEFAULT_COLUMNS "[{\"title\":\"Playing\", \"id\":\"1\", \"format\":\"%playstatus%\", \"size\":\"50\"}, {\"title\":\"Artist - Album\", \"format\":\"%artist%[ - %album%]\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track number%\", \"size\":\"50\"}, {\"title\":\"Track Title\", \"format\":\"%title%\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

- (NSString *)getColumnConfig {
    return [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.columns", DEFAULT_COLUMNS)];
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
    _playTpl = [NSImage imageNamed:@"btnplayTemplate.pdf"];
    [_playTpl setFlipped:YES];
    _pauseTpl = [NSImage imageNamed:@"btnpauseTemplate.pdf"];
    [_pauseTpl setFlipped:YES];
    _bufTpl = [NSImage imageNamed:@"bufferingTemplate.pdf"];
    [_bufTpl setFlipped:YES];


    NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];

    [textStyle setAlignment:NSLeftTextAlignment];
    [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];

    _colTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont controlContentFontOfSize:[NSFont smallSystemFontSize]], NSFontAttributeName
                               , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                               , [NSColor controlTextColor], NSForegroundColorAttributeName
                               , textStyle, NSParagraphStyleAttributeName
                               , nil];

    [textStyle setAlignment:NSLeftTextAlignment];
    [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];


    int rowheight = 18;

    _groupTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                 , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                 , [NSColor controlTextColor], NSForegroundColorAttributeName
                                 , textStyle, NSParagraphStyleAttributeName
                                 , nil];

    _cellTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
                                [NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                , [NSColor controlTextColor], NSForegroundColorAttributeName
                                , textStyle, NSParagraphStyleAttributeName
                                , nil];

    _cellSelectedTextAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:[NSFont controlContentFontOfSize:[NSFont systemFontSizeForControlSize:rowheight]], NSFontAttributeName
                                        , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                        , [NSColor alternateSelectedControlTextColor], NSForegroundColorAttributeName
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
    DdbPlaylistWidget *view = (DdbPlaylistWidget *)[self view];
    [view setDelegate:(id<DdbListviewDelegate>)self];
    [self initContent];

    [self setupPlaylist:[view listview]];
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
    if (idx != _ncolumns-1) {
        memmove (&_columns[idx], &_columns[idx+1], (_ncolumns-idx) * sizeof (plt_col_info_t));
    }
    _ncolumns--;
}

// pass col=-1 for "empty space", e.g. when appending new col
- (NSMenu *)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view {
    _menuColumn = (int)col;
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"ColumnMenu"];
    [menu setDelegate:(id<NSMenuDelegate>)self];
    [menu setAutoenablesItems:NO];
    [[menu insertItemWithTitle:@"Add Column" action:@selector(menuAddColumn:) keyEquivalent:@"" atIndex:0] setTarget:self];
    if (col != -1) {
        [[menu insertItemWithTitle:@"Edit Column" action:@selector(menuEditColumn:) keyEquivalent:@"" atIndex:1] setTarget:self];
        [[menu insertItemWithTitle:@"Remove Column" action:@selector(menuRemoveColumn:) keyEquivalent:@"" atIndex:2] setTarget:self];
        NSMenuItem *item = [menu insertItemWithTitle:@"Pin Groups When Scrolling" action:@selector(menuTogglePinGroups:) keyEquivalent:@"" atIndex:3];
        [item setState:_pin_groups?NSOnState:NSOffState];
        [item setTarget:self];

        [menu insertItem:[NSMenuItem separatorItem] atIndex:4];

        NSMenu *groupBy = [[NSMenu alloc] initWithTitle:@"Group By"];
        [groupBy setDelegate:(id<NSMenuDelegate>)self];
        [groupBy setAutoenablesItems:NO];

        [[groupBy insertItemWithTitle:@"None" action:@selector(menuGroupByNone:) keyEquivalent:@"" atIndex:0] setTarget:self];
        [[groupBy insertItemWithTitle:@"Artist/Date/Album" action:@selector(menuGroupByArtistDateAlbum:) keyEquivalent:@"" atIndex:1] setTarget:self];
        [[groupBy insertItemWithTitle:@"Artist" action:@selector(menuGroupByArtist:) keyEquivalent:@"" atIndex:2] setTarget:self];
        [groupBy insertItemWithTitle:@"Custom" action:@selector(menuGroupByCustom:) keyEquivalent:@"" atIndex:3];

        NSMenuItem *groupByItem = [[NSMenuItem alloc] initWithTitle:@"Group By" action:nil keyEquivalent:@""];
        [groupByItem setSubmenu:groupBy];
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
    return col == [self columnCount] - 1 ? [self invalidColumn] : col+1;
}

- (DdbListviewCol_t)invalidColumn {
    return -1;
}

- (int)columnWidth:(DdbListviewCol_t)col {
    return _columns[col].size;
}

- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col {
    _columns[col].size = width;
}

- (int)columnMinHeight:(DdbListviewCol_t)col {
    return _columns[col].type == DB_COLUMN_ALBUM_ART;
}

- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to {
    plt_col_info_t tmp;

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
}

- (void)columnsChanged {
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

    NSString *json = [[NSString alloc] initWithData:dt encoding:NSUTF8StringEncoding];
    [self writeColumnConfig:json];
    deadbeef->conf_save ();
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
    [[NSColor colorWithCalibratedWhite:0.3f alpha:0.3f] set];
    [NSBezierPath fillRect:NSMakeRect(rect.origin.x + rect.size.width - 1, rect.origin.y+3,1,rect.size.height-6)];

    [[NSString stringWithUTF8String:_columns[col].title] drawInRect:NSMakeRect(rect.origin.x+4, rect.origin.y+1, rect.size.width-6, rect.size.height-2) withAttributes:_colTextAttrsDictionary];
}

- (void)drawCell:(int)idx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused {
    int sel = deadbeef->pl_is_selected((DB_playItem_t *)row);
    if (sel) {
        if (focused) {
            [[NSColor alternateSelectedControlColor] set];
            [NSBezierPath fillRect:rect];
        }
        else {
            [[NSColor controlShadowColor] set];
            [NSBezierPath fillRect:rect];
        }
    }

    if (col == [self invalidColumn]) {
        return;
    }

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();

    if (_columns[col].type == DB_COLUMN_PLAYING && playing_track && (DB_playItem_t *)row == playing_track) {
        NSImage *img = NULL;
        int paused = deadbeef->get_output ()->state () == OUTPUT_STATE_PAUSED;
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

        NSColor *imgColor = sel ? [NSColor alternateSelectedControlTextColor] : [NSColor controlTextColor];

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
            .flags = DDB_TF_CONTEXT_HAS_ID|DDB_TF_CONTEXT_HAS_INDEX,
        };

        char text[1024] = "";
        deadbeef->tf_eval (&ctx, _columns[col].bytecode, text, sizeof (text));

        rect.origin.x += CELL_HPADDING;
        rect.size.width -= CELL_HPADDING;

        [[NSString stringWithUTF8String:text] drawInRect:rect withAttributes:sel?_cellSelectedTextAttrsDictionary:_cellTextAttrsDictionary];

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
    };

    char text[1024] = "";
    deadbeef->tf_eval (&ctx, _group_bytecode, text, sizeof (text));

    NSString *title = [NSString stringWithUTF8String:text];

    NSSize size = [title sizeWithAttributes:_groupTextAttrsDictionary];


    NSRect strRect = rect;
    strRect.origin.x += 5;
    strRect.origin.y = strRect.origin.y + strRect.size.height / 2 - size.height / 2;
    strRect.size.height = size.height;
    [title drawInRect:strRect withAttributes:_groupTextAttrsDictionary];

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
    DdbPlaylistViewController *ctl = (__bridge_transfer DdbPlaylistViewController *)info->ctl;
    DdbPlaylistWidget *pltWidget = (DdbPlaylistWidget *)[ctl view];
    DdbListview *listview = [pltWidget listview];
    [listview drawGroup:info->grp];
    free (info);
}

#define ART_PADDING_HORZ 8
#define ART_PADDING_VERT 0

- (void)drawAlbumArtForGroup:(DdbListviewGroup_t *)grp
                  groupIndex:(int)groupIndex
                  inColumn:(DdbListviewCol_t)col
             isPinnedGroup:(BOOL)pinned
            nextGroupCoord:(int)grp_next_y
                      xPos:(int)x
                      yPos:(int)y
                 viewportY:(int)viewportY
                     width:(int)width
                    height:(int)height {
    DdbPlaylistWidget *pltWidget = (DdbPlaylistWidget *)[self view];
    DdbListview *listview = [pltWidget listview];
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
    int art_height = height - ART_PADDING_VERT * 2;

    if (art_width < 8 || art_height < 8 || !it) {
        return;
    }

    int art_x = x + ART_PADDING_HORZ;
    int min_y = (pinned ? viewportY+listview.grouptitle_height : y) + ART_PADDING_VERT;
    int max_y = grp_next_y;

    int ypos = min_y;
    if (min_y + art_width + ART_PADDING_VERT >= max_y) {
        ypos = max_y - art_width - ART_PADDING_VERT;
    }

    NSSize size = [image size];

    [image drawInRect:NSMakeRect(art_x, ypos, art_width, size.height / (size.width / art_width))];
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
    DdbPlaylistWidget *pltWidget = (DdbPlaylistWidget *)[self view];
    deadbeef->sendmessage (DB_EV_SELCHANGED, (uintptr_t)[pltWidget listview], deadbeef->plt_get_curr_idx (), [self playlistIter]);
}

- (int)selectedCount {
    return deadbeef->pl_getselcount();
}

- (BOOL)hasDND {
    return YES;
}

- (void)songChanged:(DdbListview *)listview from:(DB_playItem_t*)from to:(DB_playItem_t*)to {
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
            to_idx = deadbeef->pl_get_idx_of (to);
            if (to_idx != -1) {
                if (cursor_follows_playback) {
                    [listview setCursor:to_idx noscroll:YES];
                }
                if (scroll_follows_playback && plt == deadbeef->plt_get_curr_idx ()) {
                    [listview scrollToRowWithIndex: to_idx];
                }
            }
        }
    }

    if (from) {
        int idx = deadbeef->pl_get_idx_of (from);
        if (idx != -1) {
            [listview drawRow:idx];
        }
    }
    if (to && to_idx != -1) {
        [listview drawRow:to_idx];
    }
}

- (void)setupPlaylist:(DdbListview *)listview {
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

        [listview reloadData];
        deadbeef->pl_unlock ();
        [listview setVScroll:scroll];
    }
}

- (int)handleListviewMessage:(DdbListview *)listview id:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
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
                        [listview drawRow:idx];
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
                    int buffering = !deadbeef->streamer_ok_to_read (-1);
                    if (buffering) {
                        DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();
                        if (playing_track) {
                            if (playing_track == track) {
                                [self songChanged:listview from:NULL to:playing_track];
                                draw = NO;
                            }
                            deadbeef->pl_item_unref (playing_track);
                        }
                    }
                    if (draw) {
                        [listview drawRow:deadbeef->pl_get_idx_of (track)];
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
                    [listview drawRow:idx];
                    deadbeef->pl_item_unref (curr);
                }
            });
        }
            break;
        case DB_EV_PLAYLISTCHANGED: {
            if (!p1 || (p1 & DDB_PLAYLIST_CHANGE_SEARCHRESULT)) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [listview reloadData];
                });
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
                    ddb_playlist_t *prev_plt = deadbeef->plt_get_curr ();

                    if (prev_plt != plt) {
                        // force group rebuild
                        deadbeef->plt_set_curr (plt);
                        [listview reloadData];
                    }

                    int idx = deadbeef->pl_get_idx_of_iter (it, [self playlistIter]);
                    if (idx != -1) {
                        [listview setCursor:idx noscroll:YES];
                        [listview scrollToRowWithIndex:idx];
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
                [listview reloadData];
            });
        }
            break;
        case DB_EV_SELCHANGED: {
            if (ctx != (uintptr_t)listview) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [listview reloadData];
                });
            }
        }
            break;
        case DB_EV_FOCUS_SELECTION: {
            if ([self playlistIter] != p1) {
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
                            [listview setCursor:idx noscroll:YES];
                            [listview scrollToRowWithIndex:idx];
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

- (void)convertSelection {
    [ConverterWindowController runConverter:DDB_ACTION_CTX_SELECTION];
}

- (void)trackProperties {
    if (!_trkProperties) {
        _trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
    }
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    [_trkProperties setPlaylist: plt];
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

- (void)sortColumn:(DdbListviewCol_t)column withOrder:(int)order {
    plt_col_info_t *c = &_columns[(int)column];
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    deadbeef->plt_sort_v2 (plt, PL_MAIN, c->type, c->format, order-1);
    deadbeef->plt_unref (plt);
}

- (void)scrollChanged:(int)pos {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_set_scroll (plt, pos);
        deadbeef->plt_unref (plt);
    }
}

- (void)rgRemove:(id)sender {
    int count;
    DB_playItem_t **tracks = [self getSelectedTracksForRg:&count withRgTags:YES];
    if (tracks) {
        [ReplayGainScannerController removeRgTagsFromTracks:tracks count:count];
    }
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
    if (tracks) {
        [ReplayGainScannerController runScanner:mode forTracks:tracks count:count];
    }
}

- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view {
    NSMenu *theMenu = [[NSMenu alloc] initWithTitle:@"Playlist Context Menu"];
    BOOL enabled = [self selectedCount] != 0;

    [[theMenu insertItemWithTitle:@"Track Properties" action:@selector(trackProperties) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

    NSMenu *rgMenu = [[NSMenu alloc] initWithTitle:@"ReplayGain"];
    [rgMenu setDelegate:(id<NSMenuDelegate>)self];
    [rgMenu setAutoenablesItems:NO];

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

    [[rgMenu insertItemWithTitle:@"Scan Per-file Track Gain" action:@selector(rgScanTracks:) keyEquivalent:@"" atIndex:0]  setEnabled:can_be_rg_scanned];
    [[rgMenu insertItemWithTitle:@"Scan Selection As Single Album" action:@selector(rgScanAlbum:) keyEquivalent:@"" atIndex:1] setEnabled:can_be_rg_scanned];
    [[rgMenu insertItemWithTitle:@"Scan Selection As Albums (By Tags)" action:@selector(rgScanAlbumsAuto:) keyEquivalent:@"" atIndex:2] setEnabled:can_be_rg_scanned];
    [[rgMenu insertItemWithTitle:@"Remove ReplayGain Information" action:@selector(rgRemove:) keyEquivalent:@"" atIndex:3] setEnabled:has_rg_info];

    NSMenuItem *rgMenuItem = [[NSMenuItem alloc] initWithTitle:@"ReplayGain" action:nil keyEquivalent:@""];
    [rgMenuItem setEnabled:enabled];
    [rgMenuItem setSubmenu:rgMenu];
    [theMenu insertItem:rgMenuItem atIndex:0];

    [[theMenu insertItemWithTitle:@"Reload metadata" action:@selector(reloadMetadata) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

    // FIXME: should be added via plugin action
    [[theMenu insertItemWithTitle:@"Convert" action:@selector(convertSelection) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

    [theMenu insertItem:[NSMenuItem separatorItem] atIndex:0];

    [[theMenu insertItemWithTitle:@"Remove From Playback Queue" action:@selector(removeFromPlaybackQueue) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

    [[theMenu insertItemWithTitle:@"Add To Playback Queue" action:@selector(addToPlaybackQueue) keyEquivalent:@"" atIndex:0] setEnabled:enabled];

    [theMenu setAutoenablesItems:NO];

    return theMenu;
}
@end
