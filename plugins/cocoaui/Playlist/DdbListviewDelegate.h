//
//  DdbListviewDelegate.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 2/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "PlaylistDataModel.h"

@class PlaylistGroup;

typedef intptr_t DdbListviewCol_t;

@protocol DdbListviewDelegate<NSObject>

@property (nonatomic,readonly) DdbListviewCol_t firstColumn;
- (DdbListviewCol_t)nextColumn:(DdbListviewCol_t)col;
@property (nonatomic,readonly) DdbListviewCol_t invalidColumn;
@property (nonatomic,readonly) int columnCount;
@property (readonly) int sortColumnIndex;

@property (nonatomic,readonly) BOOL pinGroups;

@property (nonatomic,readonly) BOOL hasDND;

- (int)columnWidth:(DdbListviewCol_t)col;
- (void)setColumnWidth:(int)width forColumn:(DdbListviewCol_t)col;

- (int)columnMinHeight:(DdbListviewCol_t)col;
- (int)columnGroupHeight:(DdbListviewCol_t)col;

- (void)moveColumn:(DdbListviewCol_t)col to:(DdbListviewCol_t)to;

- (void)columnsDidChange;

- (NSMenu *)contextMenuForColumn:(DdbListviewCol_t)col withEvent:(NSEvent*)theEvent forView:(NSView *)view;

- (BOOL)isAlbumArtColumn:(DdbListviewCol_t)col;

- (void)drawCell:(NSUInteger)rowIdx forRow:(DdbListviewRow_t)row forColumn:(DdbListviewCol_t)col inRect:(NSRect)rect focused:(BOOL)focused;
- (void)drawGroupTitle:(DdbListviewRow_t)row inRect:(NSRect)rect;
- (void)drawAlbumArtForGroup:(PlaylistGroup *)group inColumn:(DdbListviewCol_t)col isPinnedGroup:(BOOL)pinned nextGroupCoord:(int)grp_next_y xPos:(int)x yPos:(int)y viewportY:(CGFloat)viewportY width:(int)width height:(int)height;
- (void)selectionChanged:(DdbListviewRow_t)row;
- (NSMenu *)contextMenuForEvent:(NSEvent *)event forView:(NSView *)view;
- (void)sortColumn:(DdbListviewCol_t)column;
- (void)dropItems:(int)from_playlist before:(DdbListviewRow_t)before indices:(uint32_t *)indices count:(int)count copy:(BOOL)copy;
- (void)externalDropItems:(NSArray *)paths after:(DdbListviewRow_t)after completionBlock:(void (^) (void))completionBlock;
- (void)dropPlayItems:(DdbListviewRow_t *)items before:(DdbListviewRow_t)before count:(int)count;
- (void)scrollChanged:(CGFloat)scrollpos;

- (NSString *)columnTitleAtIndex:(NSUInteger)index;
- (enum ddb_sort_order_t)columnSortOrderAtIndex:(NSUInteger)index;

- (NSString *)rowGroupStr:(DdbListviewRow_t)row;
@end
