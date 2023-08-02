//
//  SegmentedTabView.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 19/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "SegmentedTabView.h"

@interface SegmentedTabView()

@property (nonatomic,readwrite) NSSegmentedControl *segmentedControl;
@property (nonatomic,readwrite) NSTabView *tabView;

@end

@implementation SegmentedTabView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self == nil) {
        return nil;
    }

    self.tabView = [[NSTabView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
    self.tabView.tabPosition = NSTabPositionNone;
    self.tabView.tabViewBorderType = NSTabViewBorderTypeNone;
    self.tabView.allowsTruncatedLabels = YES;

    self.segmentedControl = [[NSSegmentedControl alloc] initWithFrame:NSZeroRect];
    self.segmentedControl.segmentStyle = NSSegmentStyleAutomatic;
    self.segmentedControl.segmentDistribution = NSSegmentDistributionFillEqually;

    self.segmentedControl.action = @selector(segmentedControlAction:);
    self.segmentedControl.target = self;

    [self addSubview:self.segmentedControl];
    [self addSubview:self.tabView];

    self.segmentedControl.translatesAutoresizingMaskIntoConstraints = NO;
    self.tabView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.segmentedControl.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
    [self.segmentedControl.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
    [self.segmentedControl.topAnchor constraintEqualToAnchor:self.topAnchor].active = YES;

    [self.tabView.topAnchor constraintEqualToAnchor:self.segmentedControl.bottomAnchor].active = YES;
    [self.tabView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
    [self.tabView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
    [self.tabView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor].active = YES;

    return self;
}

- (void)updateSegmentsFromTabItems {
    NSInteger count = (self.tabView).numberOfTabViewItems;
    self.segmentedControl.segmentCount = count;
    for (NSInteger index = 0; index < count; index++) {
        [self.segmentedControl setLabel:[self.tabView tabViewItemAtIndex:index].label forSegment:index];
    }
    NSTabViewItem *selectedItem = self.tabView.selectedTabViewItem;
    if (selectedItem != nil) {
        self.segmentedControl.selectedSegment = [self.tabView indexOfTabViewItem:selectedItem];
    }
    else {
        self.segmentedControl.selectedSegment = -1;
    }
}

- (void)setLabel:(NSString *)label forSegment:(NSInteger)index {
    [self.tabView tabViewItemAtIndex:index].label = label;
    [self.segmentedControl setLabel:[self.tabView tabViewItemAtIndex:index].label forSegment:index];
}


- (void)segmentedControlAction:(NSSegmentedControl *)sender {
    [self.tabView selectTabViewItemAtIndex:sender.indexOfSelectedItem];
    [self sendAction:self.action to:self.target];
}

- (void)removeTabViewItem:(NSTabViewItem *)tabViewItem {
    [self.tabView removeTabViewItem:tabViewItem];
    [self updateSegmentsFromTabItems];
}

- (void)addTabViewItem:(NSTabViewItem *)tabViewItem {
    [self.tabView addTabViewItem:tabViewItem];
    [self updateSegmentsFromTabItems];
}

- (void)insertTabViewItem:(NSTabViewItem *)tabViewItem atIndex:(NSInteger)index {
    [self.tabView insertTabViewItem:tabViewItem atIndex:index];
    [self updateSegmentsFromTabItems];
}

- (nullable NSTabViewItem *)tabViewItemAtPoint:(NSPoint)point {
    point = [self.segmentedControl convertPoint:point fromView:self];

    NSInteger count = self.segmentedControl.segmentCount;
    CGFloat x = 0;
    for (NSInteger index = 0; index < count; index++) {
        CGFloat width;
        if (self.segmentedControl.segmentDistribution == NSSegmentDistributionFillEqually) {
            width = self.segmentedControl.bounds.size.width / count;
        }
        else {
            // NOTE: this will return 0 for non-equal auto distributed segments
            width = [self.segmentedControl widthForSegment:index];
        }
        x += width;
        if (x > point.x) {
            return [self.tabView tabViewItemAtIndex:index];
        }
    }

    return nil;
}

- (NSInteger)indexOfTabViewItem:(NSTabViewItem *)tabViewItem {
    return [self.tabView indexOfTabViewItem:tabViewItem];
}

- (NSTabViewItem *)tabViewItemAtIndex:(NSInteger)index {
    return [self.tabView tabViewItemAtIndex:index];
}

- (NSInteger)numberOfTabViewItems {
    return self.tabView.numberOfTabViewItems;
}

- (NSArray<__kindof NSTabViewItem *> *)tabViewItems {
    return self.tabView.tabViewItems;
}

- (void)setTabViewItems:(NSArray<__kindof NSTabViewItem *> *)tabViewItems {
    self.tabView.tabViewItems = tabViewItems;
}

- (void)selectTabViewItemAtIndex:(NSInteger)index {
    [self.tabView selectTabViewItemAtIndex:index];
    [self updateSegmentsFromTabItems];
}

- (NSTabViewItem *)selectedTabViewItem {
    return self.tabView.selectedTabViewItem;
}

@end
