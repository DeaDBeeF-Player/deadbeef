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

typedef struct DdbListviewGroup_s {
    DdbListviewRow_t head;
    int head_idx;
    int32_t height;
    int32_t min_height;
    int32_t num_items;
    struct DdbListviewGroup_s *next;
} DdbListviewGroup_t;

@protocol DdbListviewDelegate<NSObject>
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
- (int)columnGroupHeight:(DdbListviewCol_t)col;
- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to;
- (void)columnsChanged;
- (NSMenu *)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view;
- (BOOL)isAlbumArtColumn:(DdbListviewCol_t)col;
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
- (BOOL)pinGroups;
- (void)drawColumnHeader:(DdbListviewCol_t)col inRect:(NSRect)rect;
- (void)drawCell:(int)rowIdx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused;
- (void)drawGroupTitle:(DdbListviewRow_t)row inRect:(NSRect)rect;
- (void)drawAlbumArtForGroup:(DdbListviewGroup_t *)group groupIndex:(int)groupIndex inColumn:(DdbListviewCol_t)col isPinnedGroup:(BOOL)pinned nextGroupCoord:(int)grp_next_y xPos:(int)x yPos:(int)y viewportY:(int)viewportY width:(int)width height:(int)height;
- (int)modificationIdx;
- (void)selectionChanged:(DdbListviewRow_t)row;
- (int)selectedCount;
- (BOOL)hasDND;
- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view;
- (void)sortColumn:(DdbListviewCol_t)column withOrder:(int)order;
- (void)dropItems:(int)from_playlist before:(DdbListviewRow_t)before indices:(uint32_t *)indices count:(int)count copy:(BOOL)copy;
- (void)externalDropItems:(NSArray *)paths after:(DdbListviewRow_t)after;
- (void)scrollChanged:(int)scrollpos;
@end

@interface DdbListview : NSView

@property (readonly) NSView *headerView;
@property (readonly) NSView *contentView;
@property (readonly) DdbListviewGroup_t *groups;
@property (readonly) int grouptitle_height;
@property (readonly) int fullheight;
@property (readwrite) NSPoint lastpos;
@property (readwrite) int shift_sel_anchor;
@property (weak,nonatomic) id<DdbListviewDelegate> delegate;

- (void)reloadData;
- (void)groupCheck;
- (int)pickPoint:(int)y group:(DdbListviewGroup_t **)group groupIndex:(int *)group_idx index:(int *)global_idx;
- (void)drawRow:(int)idx;
- (void)drawGroup:(int)idx;
- (void)clickSelection:(NSPoint)pt grp:(DdbListviewGroup_t *)grp grp_index:(int)grp_index sel:(int)sel dnd:(BOOL)dnd button:(int)button;
- (void)listMouseUp:(NSEvent *)event;
- (void)listMouseDragged:(NSEvent *)event;
- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll;
- (void)scrollToRowWithIndex:(int)idx;
- (void)setVScroll:(int)scroll;
- (void)updateContentFrame;
- (void)deselectAll;
- (void)cleanup;
@end
