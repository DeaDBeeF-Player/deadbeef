//
//  PlaylistContentView.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/1/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbListviewDelegate.h"

NS_ASSUME_NONNULL_BEGIN

@interface PlaylistContentView : NSView

@property (nonatomic, weak) id<DdbListviewDelegate> delegate;
@property (nonatomic, weak) id<DdbListviewDataModelProtocol> dataModel;
@property (nonatomic,readonly) int grouptitle_height;

- (void)cleanup;

- (void)drawRow:(int)idx;
- (void)drawGroup:(nullable PlaylistGroup *)group;

- (void)setCursor:(int)cursor noscroll:(BOOL)noscroll;

- (void)scrollToRowWithIndex:(int)idx;
- (void)scrollVerticalPosition:(CGFloat)verticalPosition;

- (void)reloadData;

- (void)scrollChanged:(NSRect)visibleRect;
- (void)updatePinnedGroup;

- (NSInteger)getScrollFocusGroupAndOffset:(CGFloat *)offset;
- (CGFloat)groupPositionAtIndex:(NSInteger)index;
- (nullable PlaylistGroup *)groupForIndex:(NSInteger)index;

- (void)invalidateArtworkCacheForRow:(DdbListviewRow_t)row;

- (void)configChanged;

@end


NS_ASSUME_NONNULL_END
