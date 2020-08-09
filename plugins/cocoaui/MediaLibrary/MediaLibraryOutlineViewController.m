//
//  MediaLibraryOutlineViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "deadbeef.h"
#import "DdbShared.h"
#import "medialib.h"
#import "MediaLibrarySelectorCellView.h"
#import "MediaLibraryItem.h"
#import "MediaLibraryOutlineViewController.h"
#import "PlaylistLocalDragDropHolder.h"
#import "TrackContextMenu.h"
#import "TrackPropertiesWindowController.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryOutlineViewController() <TrackContextMenuDelegate,MediaLibraryFilterSelectorCellViewDelegate>

@property (nonatomic) NSString *libraryPopupItem;

@property (nonatomic) int listenerId;

@property (nonatomic) MediaLibraryItem *medialibRootItem;

@property (nonatomic) NSOutlineView *outlineView;

@property (nonatomic) ddb_medialib_plugin_t *medialibPlugin;
@property (nonatomic) ddb_medialib_item_t *medialibItemTree;

@property (nonatomic) NSInteger lastSelectedIndex;

@property (nonatomic) TrackPropertiesWindowController *trkProperties;

@end

@implementation MediaLibraryOutlineViewController

- (instancetype)init {
    return [self initWithOutlineView:[NSOutlineView new]];
}

- (instancetype)initWithOutlineView:(NSOutlineView *)outlineView {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.outlineView = outlineView;
    self.outlineView.dataSource = self;
    self.outlineView.delegate = self;
    [self.outlineView registerForDraggedTypes:@[ddbPlaylistItemsUTIType]];

    self.libraryPopupItem = @"Popup";

    self.medialibPlugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    self.listenerId = self.medialibPlugin->add_listener (_medialib_listener, (__bridge void *)self);

    self.outlineView.menu = [TrackContextMenu new];
    self.outlineView.menu.delegate = self;

    [self.outlineView expandItem:self.medialibRootItem];
    [self initializeTreeView:0];
    [self updateMedialibStatus];

    self.outlineView.doubleAction = @selector(outlineViewDoubleAction:);
    self.outlineView.target = self;

    return self;
}

- (void)dealloc {
    self.medialibPlugin->remove_listener (self.listenerId);

}

static void _medialib_listener (int event, void *user_data) {
    MediaLibraryOutlineViewController *ctl = (__bridge MediaLibraryOutlineViewController *)user_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [ctl medialibEvent:event];
    });
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

- (void)outlineViewDoubleAction:(NSOutlineView *)sender {
    ddb_playlist_t *curr_plt = NULL;
    if (deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1)) {
        char str[200];
        deadbeef->conf_get_str ("cli_add_playlist_name", "Default", str, sizeof (str));
        curr_plt = deadbeef->plt_find_by_name (str);
        if (!curr_plt) {
            curr_plt = deadbeef->plt_append (str);
        }
    }
    if (!curr_plt) {
        return;
    }

    deadbeef->plt_set_curr (curr_plt);
    deadbeef->plt_clear(curr_plt);

    ddb_playItem_t *playItem;
    ddb_playlist_t *plt;
    [self selectedPlayItem:&playItem plt:&plt];

    if (playItem) {
        ddb_playItem_t *it = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy (it, playItem);
        deadbeef->plt_insert_item (curr_plt, NULL, it);
        deadbeef->pl_item_unref (it);
        deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, 0, 0);
    }

    if (plt) {
        deadbeef->plt_unref (plt);
    }
}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if (item == nil) {
        return 2;
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
    else if (item == nil || item == self.medialibRootItem) {
        return YES;
    }
    return NO;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isGroupItem:(id)item {
    return item == self.medialibRootItem;
}

- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    if (item == nil) {
        switch (index) {
        case 0:
            return self.libraryPopupItem;
        case 1:
            return self.medialibRootItem;
        }
    }
    else if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        return [mlItem childAtIndex:index];
    }

    return [NSString stringWithFormat:@"Error %d", (int)index];
}


#pragma mark - NSOutlineViewDataSource - Drag and drop

- (id<NSPasteboardWriting>)outlineView:(NSOutlineView *)outlineView pasteboardWriterForItem:(MediaLibraryItem *)item {
    if (![item isKindOfClass:MediaLibraryItem.class]) {
        return nil;
    }
    ddb_playlist_t *plt = self.medialibPlugin->playlist();
    int idx = deadbeef->plt_get_item_idx (plt, item.playItem, PL_MAIN);
    deadbeef->plt_unref (plt);
    if (idx < 0) {
        return nil;
    }
    return [[PlaylistLocalDragDropHolder alloc] initWithMedialibItemIndex:idx];
}


- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id<NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)index {
    return NSDragOperationNone;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id<NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index {
    return NO;
}

#pragma mark - NSOutlineViewDelegate

- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
    NSTableCellView *view;
    if ([item isKindOfClass:MediaLibraryItem.class]) {
        view = [outlineView makeViewWithIdentifier:@"TextCell" owner:self];
        MediaLibraryItem *mlItem = item;
        if (item == self.medialibRootItem) {
            [self updateMedialibStatusForView:view];
        }
        else {
            view.textField.stringValue = mlItem.stringValue;
        }
    }
    else if (item == self.libraryPopupItem) {
        view = [outlineView makeViewWithIdentifier:@"FilterSelectorCell" owner:self];
        MediaLibrarySelectorCellView *selectorCellView = (MediaLibrarySelectorCellView *)view;
        selectorCellView.delegate = self;
        [selectorCellView.popupButton selectItemAtIndex:self.lastSelectedIndex];
        if (view.textField) {
            view.textField.stringValue = item;
        }
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

#pragma mark - TrackContextMenuDelegate

- (void)trackProperties {
    if (!self.trkProperties) {
        self.trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
    }
    ddb_playlist_t *plt = self.medialibPlugin->playlist();
    self.trkProperties.playlist =  plt;
    deadbeef->plt_unref (plt);
    [self.trkProperties fill];
    [self.trkProperties showWindow:self];
}

- (void)playlistChanged {
    // FIXME
}

- (void)selectedPlayItem:(ddb_playItem_t **)playItem plt:(ddb_playlist_t **)plt {
    *playItem = NULL;
    *plt = NULL;

    NSInteger row = -1;
    *plt = self.medialibPlugin->playlist();
    MediaLibraryItem *item;
    *playItem = NULL;

    if (!*plt) {
        return;
    }

    deadbeef->plt_deselect_all (*plt);

    row = self.outlineView.clickedRow;
    if (row == -1) {
        deadbeef->plt_unref (*plt);
        *plt = NULL;
        return;
    }

    item = [self.outlineView itemAtRow:row];
    if (!item || ![item isKindOfClass:MediaLibraryItem.class]) {
        deadbeef->plt_unref (*plt);
        *plt = NULL;
        return;
    }

    *playItem = item.playItem;
}

- (void)selectItemRecursively:(MediaLibraryItem *)item {
    if (![item isKindOfClass:MediaLibraryItem.class]) {
        return;
    }
    ddb_playItem_t *it = item.playItem;
    if (it) {
        deadbeef->pl_set_selected (it, 1);
    }

    for (NSUInteger i = 0; i < item.numberOfChildren; i++) {
        [self selectItemRecursively:item.children[i]];
    }
}

- (void)selectClickedRows {
    ddb_playlist_t *plt = self.medialibPlugin->playlist();
    if (!plt) {
        return;
    }

    deadbeef->plt_deselect_all (plt);

    [self.outlineView.selectedRowIndexes enumerateIndexesUsingBlock:^(NSUInteger row, BOOL * _Nonnull stop) {
        [self selectItemRecursively:[self.outlineView itemAtRow:row]];
    }];

    // add clicked row
    NSInteger clickedRow = self.outlineView.clickedRow;
    if (clickedRow != -1) {
        [self selectItemRecursively:[self.outlineView itemAtRow:clickedRow]];
    }


    deadbeef->plt_unref (plt);
}

- (void)menuNeedsUpdate:(TrackContextMenu *)menu {
    ddb_playlist_t *plt = self.medialibPlugin->playlist();
    if (!plt) {
        return;
    }

    [self selectClickedRows];

    [((TrackContextMenu *)self.outlineView.menu) update:self.medialibPlugin->playlist() iter:PL_MAIN];
    deadbeef->plt_unref (plt);

    // FIXME: the menu operates on the specified playlist, with its own selection, which can change while the menu is open
    // Therefore, it's better to create a separate list of playItems for the menu consumption.
}

- (NSIndexSet *)outlineView:(NSOutlineView *)outlineView selectionIndexesForProposedSelection:(NSIndexSet *)proposedSelectionIndexes {
    NSMutableIndexSet *selectionIndexes = [NSMutableIndexSet new];

    // prevent selecting filter items
    [proposedSelectionIndexes enumerateIndexesUsingBlock:^(NSUInteger row, BOOL * _Nonnull stop) {
        id item = [self.outlineView itemAtRow:row];
        if (item != self.libraryPopupItem
            && item != self.medialibRootItem) {
            [selectionIndexes addIndex:row];
        }
    }];
    return selectionIndexes;
}

#pragma mark - MediaLibraryFilterSelectorCellViewDelegate

- (void)filterSelectorChanged:(MediaLibrarySelectorCellView *)cellView {
    self.lastSelectedIndex = cellView.popupButton.indexOfSelectedItem;
    [self filterChanged];
}




@end
