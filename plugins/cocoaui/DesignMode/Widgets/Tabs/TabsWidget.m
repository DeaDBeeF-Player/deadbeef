//
//  TabsWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "TabsWidget.h"
#import "PlaceholderWidget.h"
#import "RenameTabViewController.h"
#import "SegmentedTabView.h"

@interface TabsWidget() <NSMenuDelegate, NSTabViewDelegate, RenameTabViewControllerDelegate>

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic) SegmentedTabView *segmentedTabView;
@property (nonatomic) NSTabViewItem *clickedItem;
@property (nonatomic) NSPoint clickedPoint;
@property (nonatomic) NSMutableArray *labels;
@property (nonatomic) NSPopover *renameTabPopover;
@property (nonatomic) NSInteger initiallySelectedTabIndex;

@end

@implementation TabsWidget

+ (NSString *)widgetType {
    return @"Tabs";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.initiallySelectedTabIndex = -1;

    self.deps = deps;

    self.labels = [NSMutableArray new];

    self.segmentedTabView = [[SegmentedTabView alloc] initWithFrame:NSZeroRect];
    self.segmentedTabView.target = self;
    self.segmentedTabView.action = @selector(segmentedTabViewAction:);

    [self.topLevelView addSubview:self.segmentedTabView];

    NSMenu *menu = [NSMenu new];

    menu.delegate = self;

    NSMenuItem *renameTabItem = [menu addItemWithTitle:@"Rename Tab" action:@selector(renameTab:) keyEquivalent:@""];
    NSMenuItem *removeTabItem = [menu addItemWithTitle:@"Remove Tab" action:@selector(removeTab:) keyEquivalent:@""];
    NSMenuItem *addNewTabItem = [menu addItemWithTitle:@"Add New Tab" action:@selector(addNewTab:) keyEquivalent:@""];
    [menu addItem:NSMenuItem.separatorItem];
    NSMenuItem *moveTabLeftItem = [menu addItemWithTitle:@"Move Tab Left" action:@selector(moveTabLeft:) keyEquivalent:@""];
    NSMenuItem *moveTabRightItem = [menu addItemWithTitle:@"Move Tab Right" action:@selector(moveTabRight:) keyEquivalent:@""];

    renameTabItem.target = self;
    removeTabItem.target = self;
    addNewTabItem.target = self;
    moveTabLeftItem.target = self;
    moveTabRightItem.target = self;

    self.segmentedTabView.menu = menu;

    // constrain view
    self.segmentedTabView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.segmentedTabView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [self.segmentedTabView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [self.segmentedTabView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [self.segmentedTabView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)segmentedTabViewAction:(SegmentedTabView *)sender {
    [self configure];
    [self.deps.state layoutDidChange];
}

- (void)menuNeedsUpdate:(NSMenu *)menu {
    self.clickedItem = [self tabViewAtPoint:NSEvent.mouseLocation];
    NSPoint point = [self.segmentedTabView.window convertPointFromScreen:NSEvent.mouseLocation];
    self.clickedPoint = [self.segmentedTabView convertPoint:point fromView:nil];
}

- (NSTabViewItem *)tabViewAtPoint:(NSPoint)point {
    point = [self.segmentedTabView.window convertPointFromScreen:point];
    point = [self.segmentedTabView convertPoint:point fromView:nil];
    NSTabViewItem *item = [self.segmentedTabView tabViewItemAtPoint:point];
    return item;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (self.clickedItem == nil
        && (menuItem.action == @selector(renameTab:)
            || menuItem.action == @selector(removeTab:)
            || menuItem.action == @selector(moveTabLeft:)
            || menuItem.action == @selector(moveTabRight:))) {
        return NO;
    }

    NSInteger index = [self.segmentedTabView indexOfTabViewItem:self.clickedItem];

    if (menuItem.action == @selector(moveTabLeft:) && index < 1) {
        return NO;
    }

    if (menuItem.action == @selector(moveTabRight:) && index >= self.segmentedTabView.numberOfTabViewItems-1) {
        return NO;
    }

    return YES;
}

- (void)renameTab:(NSMenuItem *)sender {
    if (self.renameTabPopover != nil) {
        [self.renameTabPopover close];
        self.renameTabPopover = nil;
    }

    self.renameTabPopover = [NSPopover new];
    self.renameTabPopover.behavior = NSPopoverBehaviorTransient;

    RenameTabViewController *viewController = [RenameTabViewController new];
    viewController.name = self.clickedItem.label;
    viewController.popover = self.renameTabPopover;
    viewController.delegate = self;

    self.renameTabPopover.contentViewController = viewController;

    NSRect rect = NSMakeRect(self.clickedPoint.x, self.clickedPoint.y, 1, 1);
    [self.renameTabPopover showRelativeToRect:rect ofView:self.segmentedTabView preferredEdge:NSRectEdgeMaxY];
}

- (void)renameTabDone:(RenameTabViewController *)renameTabViewController withName:(NSString *)name {
    NSInteger index = [self.segmentedTabView indexOfTabViewItem:self.clickedItem];
    if (index == NSNotFound) {
        return;
    }

    [self.segmentedTabView setLabel:name forSegment:index];
    self.labels[index] = name;

    [self.deps.state layoutDidChange];
}

- (void)removeTabItemForChild:(id<WidgetProtocol>)child {
    NSUInteger index = [self.childWidgets indexOfObject:child];
    if (index == NSNotFound) {
        return;
    }
    NSTabViewItem *item = [self.segmentedTabView tabViewItemAtIndex:index];
    [child.view removeFromSuperview];
    [self.segmentedTabView removeTabViewItem:item];
    [self.labels removeObjectAtIndex:index];
    [super removeChild:child];
}

- (void)removeTab:(NSMenuItem *)sender {
    if (self.clickedItem == nil) {
        return;
    }


    NSInteger index = [self.segmentedTabView indexOfTabViewItem:self.clickedItem];
    if (index == NSNotFound) {
        return;
    }

    [self removeTabItemForChild:self.childWidgets[index]];

    [self.deps.state layoutDidChange];
}

- (void)addNewTab:(NSMenuItem *)sender {
    [self.labels addObject:[NSString stringWithFormat:@"New Tab %d", (int)self.labels.count]];
    id<WidgetProtocol> child = [self.deps.factory createWidgetWithType:PlaceholderWidget.widgetType];

    if (self.clickedItem != nil) {
        NSInteger index = [self.segmentedTabView indexOfTabViewItem:self.clickedItem];
        [self insertChild:child atIndex:index+1];
        [self.segmentedTabView selectTabViewItemAtIndex:index+1];
    }
    else {
        [self appendChild:child];
        [self.segmentedTabView selectTabViewItemAtIndex:self.childWidgets.count-1];
    }

    [self.deps.state layoutDidChange];
}

- (void)moveTabLeft:(NSMenuItem *)sender {
    if (self.clickedItem == nil) {
        return;
    }

    NSInteger index = [self.segmentedTabView indexOfTabViewItem:self.clickedItem];
    if (index == 0 || index == NSNotFound) {
        return;
    }

    BOOL wasSelected = [self.segmentedTabView tabViewItemAtIndex:index] == self.segmentedTabView.selectedTabViewItem;

    NSString *label = self.labels[index];
    id<WidgetProtocol> childWidget = self.childWidgets[index];
    [self removeTabItemForChild:self.childWidgets[index]];

    index -= 1;

    [self.labels insertObject:label atIndex:index];
    [self insertChild:childWidget atIndex:index];

    self.clickedItem = [self.segmentedTabView tabViewItemAtIndex:index];
    [self.segmentedTabView setLabel:self.clickedItem.label forSegment:index];

    if (wasSelected) {
        [self.segmentedTabView selectTabViewItemAtIndex:index];
    }

    [self.deps.state layoutDidChange];
}

- (void)moveTabRight:(NSMenuItem *)sender {
    if (self.clickedItem == nil) {
        return;
    }

    NSInteger index = [self.segmentedTabView indexOfTabViewItem:self.clickedItem];
    if (index == NSNotFound || index == self.segmentedTabView.numberOfTabViewItems-1) {
        return;
    }

    BOOL wasSelected = [self.segmentedTabView tabViewItemAtIndex:index] == self.segmentedTabView.selectedTabViewItem;

    NSString *label = self.labels[index];
    id<WidgetProtocol> childWidget = self.childWidgets[index];
    [self removeTabItemForChild:self.childWidgets[index]];

    index += 1;

    [self.labels insertObject:label atIndex:index];
    [self insertChild:childWidget atIndex:index];

    self.clickedItem = [self.segmentedTabView tabViewItemAtIndex:index];
    [self.segmentedTabView setLabel:self.clickedItem.label forSegment:index];

    if (wasSelected) {
        [self.segmentedTabView selectTabViewItemAtIndex:index];
    }

    [self.deps.state layoutDidChange];
}

- (void)appendChild:(id<WidgetProtocol>)child {
    [super appendChild:child];
    NSTabViewItem *item = [NSTabViewItem new];
    if (self.childWidgets.count-1 < self.labels.count) {
        item.label = self.labels[self.childWidgets.count-1];
    }
    else {
        item.label = @"<Missing Label>";
    }

    [item.view addSubview:child.view];

    child.view.translatesAutoresizingMaskIntoConstraints = NO;
    [item.view.leadingAnchor constraintEqualToAnchor:child.view.leadingAnchor].active = YES;
    [item.view.trailingAnchor constraintEqualToAnchor:child.view.trailingAnchor].active = YES;
    [item.view.topAnchor constraintEqualToAnchor:child.view.topAnchor].active = YES;
    [item.view.bottomAnchor constraintEqualToAnchor:child.view.bottomAnchor].active = YES;

    [self.segmentedTabView addTabViewItem:item];
}

- (void)removeChild:(id<WidgetProtocol>)child {
    // replace child with a placeholder
    id<WidgetProtocol> placeholder = [self.deps.factory createWidgetWithType:PlaceholderWidget.widgetType];
    [self replaceChild:child withChild:placeholder];
}

- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild {
    NSUInteger index = [self.childWidgets indexOfObject:child];
    if (index == NSNotFound) {
        return;
    }

    NSTabViewItem *item = [self.segmentedTabView tabViewItemAtIndex:index];

    [child.view removeFromSuperview];
    [item.view addSubview:newChild.view];

    self.childWidgets[index] = newChild;

    newChild.view.translatesAutoresizingMaskIntoConstraints = NO;
    [item.view.leadingAnchor constraintEqualToAnchor:newChild.view.leadingAnchor].active = YES;
    [item.view.trailingAnchor constraintEqualToAnchor:newChild.view.trailingAnchor].active = YES;
    [item.view.topAnchor constraintEqualToAnchor:newChild.view.topAnchor].active = YES;
    [item.view.bottomAnchor constraintEqualToAnchor:newChild.view.bottomAnchor].active = YES;

    child.parentWidget = nil;
    newChild.parentWidget = self;
    [item.view layoutSubtreeIfNeeded];

    [newChild configure];
}

- (void)insertChild:(id<WidgetProtocol>)child atIndex:(NSInteger)index {
    [super insertChild:child atIndex:index];
    NSTabViewItem *item = [NSTabViewItem new];
    item.label = self.labels[index];

    [item.view addSubview:child.view];

    child.view.translatesAutoresizingMaskIntoConstraints = NO;
    [item.view.leadingAnchor constraintEqualToAnchor:child.view.leadingAnchor].active = YES;
    [item.view.trailingAnchor constraintEqualToAnchor:child.view.trailingAnchor].active = YES;
    [item.view.topAnchor constraintEqualToAnchor:child.view.topAnchor].active = YES;
    [item.view.bottomAnchor constraintEqualToAnchor:child.view.bottomAnchor].active = YES;

    [self.segmentedTabView insertTabViewItem:item atIndex:index];
}

- (NSDictionary *)serializedSettingsDictionary {
    NSMutableArray *labels = [NSMutableArray new];
    for (NSTabViewItem *item in self.segmentedTabView.tabViewItems) {
        [labels addObject:item.label];
    }

    NSInteger selectedTabIndex = -1;
    NSTabViewItem *selectedItem = self.segmentedTabView.selectedTabViewItem;
    if (selectedItem != nil) {
        selectedTabIndex = [self.segmentedTabView indexOfTabViewItem:selectedItem];
    }
    return @{
        @"labels": labels.copy,
        @"selectedTab": @(selectedTabIndex),
    };
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    NSArray<NSString *> *labels = dictionary[@"labels"];
    if ([labels isKindOfClass:NSArray.class]) {
        self.labels = labels.mutableCopy;
    }
    else {
        self.labels = [NSMutableArray new];
    }

    NSNumber *selectedTabNumber = dictionary[@"selectedTab"];
    if ([selectedTabNumber isKindOfClass:NSNumber.class]) {
        self.initiallySelectedTabIndex = selectedTabNumber.integerValue;
    }

    return YES;
}

- (void)didFinishLoading {
    if (self.initiallySelectedTabIndex != -1) {
        [self.segmentedTabView selectTabViewItemAtIndex:self.initiallySelectedTabIndex];
    }
}

@end
