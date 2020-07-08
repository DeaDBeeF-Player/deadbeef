//
//  MainWindowSidebarViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/8/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MainWindowSidebarViewController.h"

@interface MainWindowSidebarViewController () <NSOutlineViewDelegate,NSOutlineViewDataSource>

@property (nonatomic) NSArray<NSString *> *groups;
@property (nonatomic) NSArray<NSString *> *groupItems;
@property (weak) IBOutlet NSOutlineView *outlineView;

@end

@implementation MainWindowSidebarViewController

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    self.groups = @[
        @"Library",
    ];

    self.groupItems = @[
        @"Artists",
        @"Albums",
        @"Genres",
        @"Songs",
        @"Folders",
        @"Playlists",
    ];

    return self;
}

- (void)viewDidLoad {
    for (NSString *group in self.groups) {
        [self.outlineView expandItem:group];
    }

}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if (item == nil) {
        return self.groups.count;
    }
    else if ([self.groups containsObject:item]) {
        return self.groupItems.count;
    }
    return 0;
}


- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    if (item == nil || [self.groups containsObject:item]) {
        return YES;
    }
    return NO;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldShowOutlineCellForItem:(id)item {
    return NO;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldSelectItem:(id)item {
    if ([self.groups containsObject:item]) {
        return NO;
    }
    return YES;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isGroupItem:(id)item {
    return [self.groups containsObject:item];
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    if (item == nil) {
        return self.groups[index];
    }

    if (item == self.groups[0]) {
        return self.groupItems[index];
    }

    return [NSString stringWithFormat:@"Playlist %d", (int)index];
}

#pragma mark - NSOutlineViewDelegate

- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
    NSTableCellView *view;
    if ([self.groups containsObject:item]) {
        view = [outlineView makeViewWithIdentifier:@"groupCellId" owner:self];
    }
    else {
        view = [outlineView makeViewWithIdentifier:@"cellId" owner:self];
        // FIXME: 11.0 availability check doesn't work in BigSur beta
        if (@available(macOS 10.16, *)) {
            view.imageView.image = [NSImage imageWithSystemSymbolName:@"play.circle" accessibilityDescription:nil];
        }
    }
    view.textField.stringValue = item;
    return view;
}

@end
