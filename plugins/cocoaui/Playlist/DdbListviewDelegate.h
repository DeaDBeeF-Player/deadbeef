//
//  DdbListviewDelegate.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#ifndef DdbListviewDelegate_h
#define DdbListviewDelegate_h

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

@property (nonatomic,readonly) int columnCount;
@property (nonatomic,readonly) int rowCount;
@property (nonatomic) int cursor;
@property (readonly) int sortColumnIndex;

@property (nonatomic,readonly) DdbListviewCol_t firstColumn;
- (DdbListviewCol_t)nextColumn:(DdbListviewCol_t)col;

@property (nonatomic,readonly) DdbListviewRow_t firstRow;
- (DdbListviewRow_t)nextRow:(DdbListviewRow_t)row;

@property (nonatomic,readonly) DdbListviewCol_t invalidColumn;
@property (nonatomic,readonly) DdbListviewRow_t invalidRow;

@property (nonatomic,readonly) BOOL pinGroups;

@property (nonatomic,readonly) int modificationIdx;
@property (nonatomic,readonly) int selectedCount;
@property (nonatomic,readonly) BOOL hasDND;

- (void)lock;
- (void)unlock;

- (void)activate:(int)idx;

- (int)columnWidth:(DdbListviewCol_t)col;
- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col;

- (int)columnMinHeight:(DdbListviewCol_t)col;
- (int)columnGroupHeight:(DdbListviewCol_t)col;

- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to;

- (void)columnsChanged;

- (NSMenu *)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view;

- (BOOL)isAlbumArtColumn:(DdbListviewCol_t)col;

- (DdbListviewRow_t)rowForIndex:(int)idx;
- (void)refRow:(DdbListviewRow_t)row;
- (void)unrefRow:(DdbListviewRow_t)row;
- (void)selectRow:(DdbListviewRow_t)row withState:(BOOL)state;
- (BOOL)rowSelected:(DdbListviewRow_t)row;
- (void)deselectAll;
- (NSString *)rowGroupStr:(DdbListviewRow_t)row;
- (void)drawCell:(int)rowIdx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused;
- (void)drawGroupTitle:(DdbListviewRow_t)row inRect:(NSRect)rect;
- (void)drawAlbumArtForGroup:(DdbListviewGroup_t *)group groupIndex:(int)groupIndex inColumn:(DdbListviewCol_t)col isPinnedGroup:(BOOL)pinned nextGroupCoord:(int)grp_next_y xPos:(int)x yPos:(int)y viewportY:(CGFloat)viewportY width:(int)width height:(int)height;
- (void)selectionChanged:(DdbListviewRow_t)row;
- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view;
- (void)sortColumn:(DdbListviewCol_t)column;
- (void)dropItems:(int)from_playlist before:(DdbListviewRow_t)before indices:(uint32_t *)indices count:(int)count copy:(BOOL)copy;
- (void)externalDropItems:(NSArray *)paths after:(DdbListviewRow_t)after;
- (void)scrollChanged:(CGFloat)scrollpos;

- (NSString *)columnTitleAtIndex:(NSUInteger)index;
- (enum ddb_sort_order_t)columnSortOrderAtIndex:(NSUInteger)index;

@end

#endif /* DdbListviewDelegate_h */
