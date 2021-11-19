//
//  SegmentedTabView.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 19/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface SegmentedTabView : NSView

@property (nonatomic,readonly) NSSegmentedControl *segmentedControl;
@property (nonatomic,readonly) NSTabView *tabView;

- (void)updateSegmentsFromTabItems;

- (void)removeTabViewItem:(NSTabViewItem *)tabViewItem;
- (void)addTabViewItem:(NSTabViewItem *)tabViewItem;
- (void)insertTabViewItem:(NSTabViewItem *)tabViewItem atIndex:(NSInteger)index;
- (nullable NSTabViewItem *)tabViewItemAtPoint:(NSPoint)point;

- (void)setLabel:(NSString *)label forSegment:(NSInteger)index;

@end

NS_ASSUME_NONNULL_END
