//
//  MainWindowSidebarViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/8/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MainWindowSidebarViewController.h"
#import "deadbeef.h"
#import "medialib.h"
#import "MediaLibraryItem.h"

extern DB_functions_t *deadbeef;

@interface MainWindowSidebarViewController () <NSOutlineViewDelegate,NSOutlineViewDataSource>

@property (nonatomic) NSString *libraryItem;
@property (nonatomic) MediaLibraryItem *medialibRootItem;

@property (nonatomic) NSArray<NSString *> *groupItems;
@property (weak) IBOutlet NSOutlineView *outlineView;

@property (nonatomic) ddb_medialib_plugin_t *medialibPlugin;
@property (nonatomic) ddb_medialib_item_t *medialibItemTree;

@property (nonatomic) NSInteger lastSelectedIndex;

@end

@implementation MainWindowSidebarViewController

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    self.libraryItem = @"Library";

    self.groupItems = @[
        @"Genres",
        @"Albums",
        @"Artists",
        @"Folders",
    ];

    return self;
}

static void _medialib_listener (int event, void *user_data) {
    MainWindowSidebarViewController *ctl = (__bridge MainWindowSidebarViewController *)user_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [ctl medialibEvent:event];
    });
}

- (void)viewDidLoad {
    self.medialibPlugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    self.medialibPlugin->add_listener (_medialib_listener, (__bridge void *)self);

    [self.outlineView expandItem:self.libraryItem];
    [self.outlineView expandItem:self.medialibRootItem];
    [self initializeTreeView:0];
    [self updateMedialibStatus];
}

- (void)initializeTreeView:(int)index {
    static const char *indexNames[] = {
        "genre",
        "album",
        "artist",
        "folder",
    };
    self.medialibRootItem = nil;
    if (self.medialibItemTree) {
        self.medialibPlugin->free_list (self.medialibItemTree);
        self.medialibItemTree = NULL;
    }
    self.medialibItemTree = self.medialibPlugin->get_list (indexNames[index]);
    self.medialibRootItem = [[MediaLibraryItem alloc] initWithItem:self.medialibItemTree];
}

- (void)updateMedialibStatusForView:(NSTableCellView *)view {
    int state = self.medialibPlugin->scanner_state ();
    switch (state) {
    case DDB_MEDIALIB_STATE_LOADING:
        view.textField.stringValue = @"Loading...";
        break;
    case DDB_MEDIALIB_STATE_SCANNING:
        view.textField.stringValue = @"Scanning...";
        break;
    case DDB_MEDIALIB_STATE_INDEXING:
        view.textField.stringValue = @"Indexing...";
        break;
    case DDB_MEDIALIB_STATE_SAVING:
        view.textField.stringValue = @"Saving...";
        break;
    default:
        view.textField.stringValue = self.medialibRootItem.stringValue;
        break;
    }
}

- (void)updateMedialibStatus {
    NSInteger row = [self.outlineView rowForItem:self.medialibRootItem];
    if (row < 0) {
        return;
    }
    NSTableCellView *view = [[self.outlineView rowViewAtRow:row makeIfNecessary:NO]  viewAtColumn:0];


    [self updateMedialibStatusForView:view];
}

- (void)medialibEvent:(int)event {
    if (event == DDB_MEDIALIB_EVENT_CHANGED) {
        [self filterChanged];
    }
    else if (event == DDB_MEDIALIB_EVENT_SCANNER) {
        int state = self.medialibPlugin->scanner_state ();
        if (state != DDB_MEDIALIB_STATE_IDLE) {
//            [_scannerActiveIndicator startAnimation:self];

            [self updateMedialibStatus];

//            [_scannerActiveState setHidden:NO];
        }
        else {
//            [_scannerActiveIndicator stopAnimation:self];
//            [_scannerActiveState setHidden:YES];
        }
    }
}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if (item == nil) {
        return 2;
    }
    else if (item == self.libraryItem) {
        return self.groupItems.count;
    }
    else if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        return mlItem.numberOfChildren;
    }
    return 0;
}


- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        return mlItem.numberOfChildren > 0;
    }
    else if (item == nil || item == self.libraryItem || item == self.medialibRootItem) {
        return YES;
    }
    return NO;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldShowOutlineCellForItem:(id)item {
    if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        return mlItem.numberOfChildren > 0;
    }
    return NO;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldSelectItem:(id)item {
    if (item == self.libraryItem || item == self.medialibRootItem) {
        return NO;
    }
    return YES;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isGroupItem:(id)item {
    return item == self.libraryItem || item == self.medialibRootItem;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    if (item == nil) {
        switch (index) {
        case 0:
            return self.libraryItem;
        case 1:
            return self.medialibRootItem;
        }
    }
    else if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        return [mlItem childAtIndex:index];
    }
    else if (item == self.libraryItem) {
        return self.groupItems[index];
    }

    return [NSString stringWithFormat:@"Error %d", (int)index];
}

#pragma mark - NSOutlineViewDelegate

- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
    NSTableCellView *view;
    if (item == self.libraryItem || [item isKindOfClass:MediaLibraryItem.class]) {
        view = [outlineView makeViewWithIdentifier:@"TextCell" owner:self];
    }
    else {
        view = [outlineView makeViewWithIdentifier:@"ImageTextCell" owner:self];

#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
        if (@available(macOS 10.16, *)) {
            NSString *name;
            if (item == self.groupItems[0]) {
                name = @"guitars";
            }
            else if (item == self.groupItems[1]) {
                name = @"music.note.list";

            }
            else if (item == self.groupItems[2]) {
                name = @"music.mic";
            }
            else if (item == self.groupItems[3]) {
                name = @"folder";
            }

            if (name) {
                view.imageView.image = [NSImage imageWithSystemSymbolName:name accessibilityDescription:nil];
            }
        }
#else
            // FIXME: image fallback for previous macOS versions
#endif
    }
    if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        if (item == self.medialibRootItem) {
            [self updateMedialibStatusForView:view];
        }
        else {
            view.textField.stringValue = mlItem.stringValue;
        }
    }
    else {
        view.textField.stringValue = item;
    }
    return view;
}

- (void)filterChanged {
    [self.outlineView beginUpdates];
    [self.outlineView removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:1] inParent:nil withAnimation:NSTableViewAnimationEffectNone];
    [self initializeTreeView:(int)self.lastSelectedIndex];
    [self.outlineView insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:1] inParent:nil withAnimation:NSTableViewAnimationEffectNone];
    [self.outlineView expandItem:self.medialibRootItem];
    [self.outlineView endUpdates];
}

- (void)outlineViewSelectionDidChange:(NSNotification *)notification {
    id item = [self.outlineView itemAtRow:self.outlineView.selectedRow];

    NSUInteger idx = [self.groupItems indexOfObject:item];
    if (idx != NSNotFound) {
        self.lastSelectedIndex = idx;
        [self filterChanged];
    }
}


@end
