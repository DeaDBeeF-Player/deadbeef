//
//  DdbPlaylistViewController.m
//  deadbeef
//
//  Created by waker on 03/10/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbPlaylistViewController.h"
#import "DdbPlaylistWidget.h"
#import "DdbListview.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation DdbPlaylistViewController


- (void)menuAddColumn:(id)sender {
    [self addColumn];
}

- (void)menuEditColumn:(id)sender {

}

- (void)menuRemoveColumn:(id)sender {

}

- (void)menuTogglePinGroups:(id)sender {

}

- (void)addColumn {
    [NSApp beginSheet:self.addColumnPanel modalForWindow:[[self view] window]  modalDelegate:self didEndSelector:@selector(didEndSheet:returnCode:contextInfo:) contextInfo:nil];

}

- (void)didEndSheet:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo
{
    [sheet orderOut:self];

    if (returnCode == NSOKButton) {
    }
}


- (IBAction)addColumnClose:(id)sender {
    [NSApp endSheet:self.addColumnPanel returnCode:([sender tag] == 1) ? NSOKButton : NSCancelButton];
}

#define DEFAULT_COLUMNS "[{\"title\":\"Playing\", \"id\":\"1\", \"format\":\"%playstatus%\", \"size\":\"50\"}, {\"title\":\"Artist - Album\", \"format\":\"%artist%[ - %album%]\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track%\", \"size\":\"50\"}, {\"title\":\"Track Title\", \"format\":\"%title%\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

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

}

- (DdbPlaylistViewController *)init {
    self = [super initWithNibName:@"Playlist" bundle:nil];

    if (self) {
        [self initContent];
        id view = [self view];
        [view setDelegate:(id<DdbListviewDelegate>)self];
    }
    return self;
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

- (void)initColumn:(int)idx withTitle:(const char *)title withId:(int)_id withSize:(int)size withFormat:(const char *)format {
    _columns[idx]._id = _id;
    _columns[idx].title = strdup (title);
    _columns[idx].format = format ? strdup (format) : NULL;
    _columns[idx].size = size;
    if (format) {
        char *bytecode;
        int res = deadbeef->tf_compile (format, &bytecode);
        if (res >= 0) {
            _columns[idx].bytecode = bytecode;
            _columns[idx].bytecode_len = res;
        }
    }
}

- (NSMenu *)getColumnMenu {
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"ColumnMenu"];
    [menu setDelegate:(id<NSMenuDelegate>)self];
    [menu setAutoenablesItems:NO];
    [[menu insertItemWithTitle:@"Add Column" action:@selector(menuAddColumn:) keyEquivalent:@"" atIndex:0] setTarget:self];
    [[menu insertItemWithTitle:@"Edit Column" action:@selector(menuEditColumn:) keyEquivalent:@"" atIndex:1] setTarget:self];
    [[menu insertItemWithTitle:@"Remove Column" action:@selector(menuRemoveColumn:) keyEquivalent:@"" atIndex:2] setTarget:self];
    [[menu insertItemWithTitle:@"Pin Groups When Scrolling" action:@selector(menuTogglePinGroups:) keyEquivalent:@"" atIndex:3] setTarget:self];

    [menu insertItem:[NSMenuItem separatorItem] atIndex:4];

    NSMenu *groupBy = [[NSMenu alloc] initWithTitle:@"Group By"];
    [groupBy setDelegate:(id<NSMenuDelegate>)self];
    [groupBy setAutoenablesItems:NO];

    [[groupBy insertItemWithTitle:@"None" action:@selector(menuGroupByNone) keyEquivalent:@"" atIndex:0] setTarget:self];
    [[groupBy insertItemWithTitle:@"Artist/Date/Album" action:@selector(menuGroupByArtistDateAlbum) keyEquivalent:@"" atIndex:1] setTarget:self];
    [[groupBy insertItemWithTitle:@"Artist" action:@selector(menuGroupByArtist) keyEquivalent:@"" atIndex:2] setTarget:self];
    [groupBy insertItemWithTitle:@"Custom" action:@selector(menuGroupByCustom) keyEquivalent:@"" atIndex:3];

    NSMenuItem *groupByItem = [[NSMenuItem alloc] initWithTitle:@"Group By" action:nil keyEquivalent:@""];
    [groupByItem setSubmenu:groupBy];
    [menu insertItem:groupByItem atIndex:5];

    return menu;
}


- (void)loadColumns:(NSArray *)cols {
    [self freeColumns];
    [cols enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
        NSDictionary *dict = obj;
        NSString *title_s = [dict objectForKey:@"title"];
        NSString *id_s = [dict objectForKey:@"id"];
        NSString *format_s = [dict objectForKey:@"format"];
        NSString *size_s = [dict objectForKey:@"size"];

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

        [self initColumn:_ncolumns withTitle:title withId:_id withSize:size withFormat:fmt];
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
    return _columns[col]._id == DB_COLUMN_ALBUM_ART;
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
        NSDictionary *dict = [[NSDictionary alloc] initWithObjectsAndKeys:
                              [NSString stringWithUTF8String:_columns[i].title], @"title"
                              , [NSString stringWithFormat:@"%d", _columns[i]._id], @"id"
                              , [NSString stringWithUTF8String:_columns[i].format], @"format"
                              , [NSString stringWithFormat:@"%d", _columns[i].size], @"size"
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

- (void)drawRowBackground:(DdbListviewRow_t)row inRect:(NSRect)rect {
    if (row%2) {
        [[NSColor selectedTextBackgroundColor] set];
        [NSBezierPath fillRect:rect];
    }
}

- (void)drawCell:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused {
    int sel = deadbeef->pl_is_selected((DB_playItem_t *)row);
    if (sel) {
        if (focused) {
            [[NSColor alternateSelectedControlColor] set];
            [NSBezierPath fillRect:rect];
        }
        else {
            [[NSColor selectedControlColor] set];
            [NSBezierPath fillRect:rect];
        }
    }

    if (col == [self invalidColumn]) {
        return;
    }

    DB_playItem_t *playing_track = deadbeef->streamer_get_playing_track ();

    if (_columns[col]._id == DB_COLUMN_PLAYING && playing_track && (DB_playItem_t *)row == playing_track) {
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
            .idx = -1,
            .id = _columns[col]._id
        };

        char text[1024] = "";
        deadbeef->tf_eval (&ctx, _columns[col].bytecode, _columns[col].bytecode_len, text, sizeof (text));

        [[NSString stringWithUTF8String:text] drawInRect:rect withAttributes:sel?_cellSelectedTextAttrsDictionary:_cellTextAttrsDictionary];

        if (ctx.plt) {
            deadbeef->plt_unref (ctx.plt);
        }
    }
}

const char *group_str = "%artist%[ - %year%][ - %album%]";
char *group_bytecode = NULL;
int group_bytecode_size = 0;

- (void)drawGroupTitle:(DdbListviewRow_t)row inRect:(NSRect)rect {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = (DB_playItem_t *)row,
        .plt = deadbeef->plt_get_curr (),
        .idx = -1,
        .id = -1
    };

    char text[1024] = "";
    deadbeef->tf_eval (&ctx, group_bytecode, group_bytecode_size, text, sizeof (text));

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
    return nil;
    if (!group_bytecode) {
        group_bytecode_size = deadbeef->tf_compile (group_str, &group_bytecode);
    }

    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = (DB_playItem_t *)row,
        .plt = deadbeef->plt_get_curr(),
        .idx = -1,
        .id = -1
    };
    char buf[1024];
    NSString *ret = @"";
    if (deadbeef->tf_eval (&ctx, group_bytecode, group_bytecode_size, buf, sizeof (buf)) > 0) {
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
                    if (deadbeef->pl_playqueue_test (it) != -1) {
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
                    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                    if (plt) {
                        int idx = deadbeef->plt_get_item_idx (plt, track, PL_MAIN);
                        if (idx != -1) {
                            [listview drawRow:deadbeef->pl_get_idx_of (track)];
                        }
                        deadbeef->plt_unref (plt);
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
            dispatch_async(dispatch_get_main_queue(), ^{
                [listview reloadData];
            });
        }
            break;
        case DB_EV_PLAYLISTSWITCHED: {
            dispatch_async(dispatch_get_main_queue(), ^{
                ddb_playlist_t *plt = deadbeef->plt_get_curr ();
                if (plt) {
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
                    [listview setVScroll:scroll];
                }
            });
        }
            break;
        case DB_EV_TRACKFOCUSCURRENT: {
            dispatch_async(dispatch_get_main_queue(), ^{
                deadbeef->pl_lock ();
                DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
                if (it) {
                    ddb_playlist_t *plt = deadbeef->pl_get_playlist (it);
                    if (plt) {
                        deadbeef->plt_set_curr (plt);
                        int idx = deadbeef->pl_get_idx_of_iter (it, [self playlistIter]);
                        if (idx != -1) {
                            [listview setCursor:idx noscroll:YES];
                            [listview scrollToRowWithIndex:idx];
                        }
                        deadbeef->plt_unref (plt);
                    }
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

@end
