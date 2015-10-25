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

#import "DdbWidget.h"

typedef intptr_t DdbListviewRow_t;
typedef intptr_t DdbListviewCol_t;

@protocol DdbListviewDelegate
- (void)lock;
- (void)unlock;
- (int)columnCount;
- (int)rowCount;
- (int)cursor;
- (void)setCursor:(int)cursor;
- (void)activate:(int)idx;
- (DdbListviewCol_t)firstColumn;
- (DdbListviewCol_t)nextColumn:(DdbListviewCol_t)col;
- (DdbListviewCol_t)invalidColumn;
- (int)columnWidth:(DdbListviewCol_t)col;
- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col;
- (int)columnMinHeight:(DdbListviewCol_t)col;
- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to;
- (void)columnsChanged;
- (void)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view;
- (DdbListviewRow_t)firstRow;
- (DdbListviewRow_t)nextRow:(DdbListviewRow_t)row;
- (DdbListviewRow_t)invalidRow;
- (DdbListviewRow_t)rowForIndex:(int)idx;
- (void)refRow:(DdbListviewRow_t)row;
- (void)unrefRow:(DdbListviewRow_t)row;
- (void)selectRow:(DdbListviewRow_t)row withState:(BOOL)state;
- (BOOL)rowSelected:(DdbListviewRow_t)row;
- (void)deselectAll;
- (NSString *)rowGroupStr:(DdbListviewRow_t)row;
- (void)drawColumnHeader:(DdbListviewCol_t)col inRect:(NSRect)rect;
- (void)drawRowBackground:(DdbListviewRow_t)row inRect:(NSRect)rect;
- (void)drawCell:(int)rowIdx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused;
- (void)drawGroupTitle:(DdbListviewRow_t)row inRect:(NSRect)rect;
- (int)modificationIdx;
- (void)selectionChanged:(DdbListviewRow_t)row;
- (int)selectedCount;
- (BOOL)hasDND;
- (void)trackProperties;
- (void)reloadMetadata;
- (void)convertSelection;
- (void)sortColumn:(DdbListviewCol_t)column withOrder:(int)order;
@end

typedef struct DdbListviewGroup_s {
    DdbListviewRow_t head;
    int head_idx;
    int32_t height;
    int32_t min_height;
    int32_t num_items;
    int pinned;
    struct DdbListviewGroup_s *next;
} DdbListviewGroup_t;

@interface DdbListview : NSView {
    id<DdbListviewDelegate> _delegate;
    DdbListviewGroup_t *_groups;
    int _grouptitle_height;
    BOOL _groups_pinned;
    int groups_build_idx;
    int _fullwidth;
    int _fullheight;
    BOOL _areaselect;
    int _areaselect_y;
    int _area_selection_start;
    int _area_selection_end;
    int _shift_sel_anchor;
    BOOL _dragwait;
    int _scroll_direction;
    int _scroll_pointer_y;
    NSPoint _lastpos;
}

@property (readonly) NSView *headerView;
@property (readonly) NSView *contentView;
@property (readonly) DdbListviewGroup_t *groups;
@property (readonly) int grouptitle_height;
@property (readonly) BOOL groups_pinned;
@property (readonly) int fullheight;
@property (readwrite) NSPoint lastpos;
@property (readwrite) int shift_sel_anchor;

- (id<DdbListviewDelegate>)delegate;
- (void)setDelegate:(id<DdbListviewDelegate>)delegate;
- (void)reloadData;
- (void)groupCheck;
- (int)pickPoint:(int)y group:(DdbListviewGroup_t **)group groupIndex:(int *)group_idx index:(int *)global_idx;
- (void)drawRow:(int)idx;
- (void)clickSelection:(NSPoint)pt grp:(DdbListviewGroup_t *)grp grp_index:(int)grp_index sel:(int)sel dnd:(BOOL)dnd button:(int)button;
- (void)listMouseUp:(NSEvent *)event;
- (void)listMouseDragged:(NSEvent *)event;
- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll;
- (void)scrollToRowWithIndex:(int)idx;
- (void)setVScroll:(int)scroll;
- (void)updateContentFrame;
- (void)deselectAll;
@end
