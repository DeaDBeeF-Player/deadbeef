//
//  PlaylistContentView.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

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
- (void)drawAlbumArtForGroup:(DdbListviewGroup_t *)group groupIndex:(int)groupIndex inColumn:(DdbListviewCol_t)col isPinnedGroup:(BOOL)pinned nextGroupCoord:(int)grp_next_y xPos:(int)x yPos:(int)y viewportY:(CGFloat)viewportY width:(int)width height:(int)height;
- (int)modificationIdx;
- (void)selectionChanged:(DdbListviewRow_t)row;
- (int)selectedCount;
- (BOOL)hasDND;
- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view;
- (void)sortColumn:(DdbListviewCol_t)column withOrder:(int)order;
- (void)dropItems:(int)from_playlist before:(DdbListviewRow_t)before indices:(uint32_t *)indices count:(int)count copy:(BOOL)copy;
- (void)externalDropItems:(NSArray *)paths after:(DdbListviewRow_t)after;
- (void)scrollChanged:(CGFloat)scrollpos;
@end

@interface PlaylistContentView : NSView

@property (nonatomic, weak) id<DdbListviewDelegate> delegate;
@property (nonatomic,readonly) int grouptitle_height;

- (void)cleanup;

- (void)drawRow:(int)idx;
- (void)drawGroup:(int)idx;

- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll;

- (void)scrollToRowWithIndex:(int)idx;
- (void)scrollVerticalPosition:(CGFloat)verticalPosition;

- (void)updateContentFrame;
- (void)reloadData;

@end


NS_ASSUME_NONNULL_END
