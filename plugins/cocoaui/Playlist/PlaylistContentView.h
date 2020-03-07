//
//  PlaylistContentView.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbListviewDelegate.h"

NS_ASSUME_NONNULL_BEGIN

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

- (void)scrollChanged:(NSRect)visibleRect;
- (void)updatePinnedGroup;

@end


NS_ASSUME_NONNULL_END
