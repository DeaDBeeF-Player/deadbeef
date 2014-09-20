//
//  PlaylistDelegate.m
//  deadbeef
//
//  Created by waker on 14/09/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "PlaylistDelegate.h"
#import "DdbListview.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

@implementation PlaylistDelegate

#define DEFAULT_COLUMNS "[{\"title\":\"Playing\", \"id\":\"1\", \"format\":\"%playstatus%\", \"size\":\"50\"}, {\"title\":\"Artist - Album\", \"format\":\"%artist% - %album%\", \"size\":\"150\"}, {\"title\":\"Track Nr\", \"format\":\"%track%\", \"size\":\"50\"}, {\"title\":\"Track Title\", \"format\":\"%title%\", \"size\":\"150\"}, {\"title\":\"Length\", \"format\":\"%length%\", \"size\":\"50\"}]"

- (PlaylistDelegate *)init {
    self = [super init];

    if (self) {
        NSString *cols = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.columns", DEFAULT_COLUMNS)];
        NSData *data = [cols dataUsingEncoding:NSUTF8StringEncoding];

        NSError *err = nil;
        id json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

        if (!json) {
            NSLog (@"error parsing column config, error: %@\n", [err localizedDescription]);
        }
        else {
            [self loadColumns:json];
        }
    }

    return self;
}

- (void)freeColumns {
    for (int i = 0; i < ncolumns; i++) {
        if (columns[i].title) {
            free (columns[i].title);
        }
        if (columns[i].format) {
            free (columns[i].format);
        }
        if (columns[i].bytecode) {
            deadbeef->tf_free (columns[i].bytecode);
        }
    }
    memset (columns, 0, sizeof (columns));
    ncolumns = 0;
}

- (void)initColumn:(int)idx withTitle:(const char *)title withId:(int)_id withSize:(int)size withFormat:(const char *)format {
    columns[idx]._id = _id;
    columns[idx].title = strdup (title);
    columns[idx].format = strdup (format);
    columns[idx].size = size;
    if (format) {
        char *bytecode;
        int res = deadbeef->tf_compile (format, &bytecode);
        if (res >= 0) {
            columns[idx].bytecode = bytecode;
            columns[idx].bytecode_len = res;
        }
    }
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

        [self initColumn:ncolumns withTitle:title withId:_id withSize:size withFormat:fmt];
        ncolumns++;
    }];
}

- (void)lock {
    deadbeef->pl_lock ();
}

- (void)unlock {
    deadbeef->pl_unlock ();
}

- (int)columnCount {
    return ncolumns;
}

- (int)rowCount {
    return deadbeef->pl_getcount (PL_MAIN);
}

- (int)cursor {
    return deadbeef->pl_get_cursor(PL_MAIN);
}

- (void)setCursor:(int)cursor {
    deadbeef->pl_set_cursor (PL_MAIN, cursor);
}

- (void)activate:(int)idx {
    deadbeef->sendmessage (DB_EV_PLAY_NUM, 0, idx, 0);
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
    return columns[col].size;
}

- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col {

}

- (int)columnMinHeight:(DdbListviewCol_t)col {
    return columns[col]._id == DB_COLUMN_ALBUM_ART;
}

- (DdbListviewRow_t)firstRow {
    return (DdbListviewRow_t)deadbeef->pl_get_first(PL_MAIN);
}

- (DdbListviewRow_t)nextRow:(DdbListviewRow_t)row {
    return (DdbListviewRow_t)deadbeef->pl_get_next((DB_playItem_t *)row, PL_MAIN);
}

- (DdbListviewRow_t)invalidRow {
    return 0;
}

- (DdbListviewRow_t)rowForIndex:(int)idx {
    return (DdbListviewRow_t)deadbeef->pl_get_for_idx_and_iter (idx, PL_MAIN);
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

    NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];

    CGFloat fontsize = [NSFont smallSystemFontSize];

    NSFont *textFont = [NSFont controlContentFontOfSize:fontsize];

    [textStyle setAlignment:NSLeftTextAlignment];
    [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];
    NSDictionary *textAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:textFont, NSFontAttributeName
                                         , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                         , [NSColor controlTextColor], NSForegroundColorAttributeName
                                         , textStyle, NSParagraphStyleAttributeName
                                         , nil];

    [[NSString stringWithUTF8String:columns[col].title] drawInRect:NSMakeRect(rect.origin.x+4, rect.origin.y+1, rect.size.width-6, rect.size.height-2) withAttributes:textAttrsDictionary];
}

- (void)drawRowBackground:(DdbListviewRow_t)row inRect:(NSRect)rect {
    if (row%2) {
        [[NSColor selectedTextBackgroundColor] set];
        [NSBezierPath fillRect:rect];
    }
}

- (void)drawCell:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused {
    ddb_tf_context_t ctx = {
        ._size = sizeof (ddb_tf_context_t),
        .it = (DB_playItem_t *)row,
        .plt = deadbeef->plt_get_curr (),
        .idx = -1,
        .id = columns[col]._id
    };

    NSColor *textColor;

    if (deadbeef->pl_is_selected((DB_playItem_t *)row)) {
        if (focused) {
            [[NSColor alternateSelectedControlColor] set];
            [NSBezierPath fillRect:rect];
            textColor = [NSColor alternateSelectedControlTextColor];
        }
        else {
            [[NSColor selectedControlColor] set];
            [NSBezierPath fillRect:rect];
            textColor = [NSColor selectedControlTextColor];
        }
    }
    else {
        textColor = [NSColor controlTextColor];
    }


    if (columns[col].bytecode) {
        char text[1024] = "";
        deadbeef->tf_eval (&ctx, columns[col].bytecode, columns[col].bytecode_len, text, sizeof (text));

        NSMutableParagraphStyle *textStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];

        CGFloat fontsize = [NSFont systemFontSizeForControlSize:rect.size.height];

        NSFont *textFont = [NSFont controlContentFontOfSize:fontsize];

        [textStyle setAlignment:NSLeftTextAlignment];
        [textStyle setLineBreakMode:NSLineBreakByTruncatingTail];
        NSDictionary *textAttrsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:textFont, NSFontAttributeName
                                             , [NSNumber numberWithFloat:0], NSBaselineOffsetAttributeName
                                             , textColor, NSForegroundColorAttributeName
                                             , textStyle, NSParagraphStyleAttributeName
                                             , nil];

        [[NSString stringWithUTF8String:text] drawInRect:rect withAttributes:textAttrsDictionary];
    }
}

- (void)selectRow:(DdbListviewRow_t)row withState:(BOOL)state {
    deadbeef->pl_set_selected ((DB_playItem_t *)row, state);
}

- (BOOL)rowSelected:(DdbListviewRow_t)row {
    return deadbeef->pl_is_selected ((DB_playItem_t *)row);
}

- (NSString *)rowGroupStr:(DdbListviewRow_t)row {
    // FIXME: this should return the tf output for the current row with current grouping
    return nil;
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
    deadbeef->sendmessage (DB_EV_SELCHANGED, 0/*should be DdbListview ptr*/, deadbeef->plt_get_curr_idx (), PL_MAIN);
}

- (BOOL)hasDND {
    return NO;
}

@end
