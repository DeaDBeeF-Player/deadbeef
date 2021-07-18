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
#import "artwork.h"
#import "AppDelegate.h"
#import "MediaLibrarySelectorCellView.h"
#import "MediaLibrarySearchCellView.h"
#import "MediaLibraryItem.h"
#import "MediaLibraryCoverQueryData.h"
#import "MediaLibraryOutlineViewController.h"
#import "MedialibItemDragDropHolder.h"
#import "TrackContextMenu.h"
#import "TrackPropertiesWindowController.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryOutlineViewController() <TrackContextMenuDelegate,MediaLibraryFilterSelectorCellViewDelegate,MediaLibrarySearchCellViewDelegate> {
    ddb_mediasource_list_selector_t *_selectors;
}

@property (nonatomic) NSString *selectorItem;
@property (nonatomic) NSString *searchItem;
@property (nonatomic) MediaLibraryItem *medialibRootItem;

@property (nullable, nonatomic) NSString *searchString;

@property (nonatomic) NSArray *topLevelItems;
@property (nonatomic) BOOL outlineViewInitialized;

@property (nonatomic) int listenerId;

@property (nonatomic) NSOutlineView *outlineView;

@property (atomic) ddb_medialib_plugin_t *medialibPlugin;
@property (atomic,readonly) ddb_mediasource_source_t medialibSource;
@property (atomic) ddb_artwork_plugin_t *artworkPlugin;

@property (nonatomic) ddb_medialib_item_t *medialibItemTree;

@property (nonatomic) NSInteger lastSelectedIndex;
@property (nonatomic) NSMutableArray<MediaLibraryItem *> *selectedItems;

@property (nonatomic) TrackContextMenu *trackContextMenu;
@property (nonatomic) TrackPropertiesWindowController *trkProperties;

@property (nonatomic) NSMutableDictionary<NSString *,NSImage *> *albumArtCache;

@end

@implementation MediaLibraryOutlineViewController

- (ddb_mediasource_source_t)medialibSource {
    AppDelegate *appDelegate = NSApplication.sharedApplication.delegate;
    return appDelegate.mediaLibraryManager.source;
}

- (instancetype)init {
    return [self initWithOutlineView:[NSOutlineView new]];
}

- (instancetype)initWithOutlineView:(NSOutlineView *)outlineView {
    self = [super init];
    if (!self) {
        return nil;
    }

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationWillQuit:) name:@"ApplicationWillQuit" object:nil];

    self.outlineView = outlineView;
    self.outlineView.dataSource = self;
    self.outlineView.delegate = self;
    [self.outlineView registerForDraggedTypes:@[ddbMedialibItemUTIType]];

    self.selectorItem = @"Popup";
    self.searchItem = @"Search Field";

    self.medialibPlugin = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    _selectors = self.medialibPlugin->plugin.get_selectors_list (self.medialibSource);
    self.artworkPlugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    self.listenerId = self.medialibPlugin->plugin.add_listener (self.medialibSource, _medialib_listener, (__bridge void *)self);

    self.trackContextMenu = [TrackContextMenu new];
    self.outlineView.menu = self.trackContextMenu;
    self.outlineView.menu.delegate = self;

    [self initializeTreeView:0];
    [self.outlineView expandItem:self.medialibRootItem];

    [self updateMedialibStatus];

    self.outlineView.doubleAction = @selector(outlineViewDoubleAction:);
    self.outlineView.target = self;

    self.albumArtCache = [NSMutableDictionary new];

    self.selectedItems = [NSMutableArray new];

    return self;
}

- (void)applicationWillQuit:(NSNotification *)notification {
    self.medialibPlugin = NULL;
    self.artworkPlugin = NULL;
}

- (void)dealloc {
    self.medialibPlugin->plugin.remove_listener (self.medialibSource, self.listenerId);
    self.medialibPlugin->plugin.free_selectors_list (self.medialibSource, _selectors);
    _selectors = NULL;
    self.listenerId = -1;
    self.medialibPlugin = NULL;
}

static void _medialib_listener (ddb_mediasource_event_type_t event, void *user_data) {
    MediaLibraryOutlineViewController *ctl = (__bridge MediaLibraryOutlineViewController *)user_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [ctl medialibEvent:event];
    });
}

- (void)initializeTreeView:(int)index {
    NSInteger itemIndex = NSNotFound;
    if (self.outlineViewInitialized) {
        itemIndex = [self.topLevelItems indexOfObject:self.medialibRootItem];
        [self.outlineView removeItemsAtIndexes:[NSIndexSet indexSetWithIndex:itemIndex] inParent:nil withAnimation:NSTableViewAnimationEffectNone];
        self.medialibRootItem = nil;
        self.topLevelItems = nil;
    }

    if (self.medialibItemTree) {
        self.medialibPlugin->plugin.free_item_tree (self.medialibSource, self.medialibItemTree);
        self.medialibItemTree = NULL;
    }
    self.medialibItemTree = self.medialibPlugin->plugin.create_item_tree (self.medialibSource, _selectors[index], self.searchString ? self.searchString.UTF8String : NULL);
    self.medialibRootItem = [[MediaLibraryItem alloc] initWithItem:self.medialibItemTree];

    self.topLevelItems = @[
        self.selectorItem,
        self.searchItem,
        self.medialibRootItem,
    ];

    if (self.outlineViewInitialized) {
        [self.outlineView insertItemsAtIndexes:[NSIndexSet indexSetWithIndex:itemIndex] inParent:nil withAnimation:NSTableViewAnimationEffectNone];
    }

    if (!self.outlineViewInitialized) {
        [self.outlineView reloadData];
    }

    self.outlineViewInitialized = YES;
}

- (void)updateMedialibStatusForView:(NSTableCellView *)view {
    int state = self.medialibPlugin->plugin.scanner_state (self.medialibSource);
    switch (state) {
    case DDB_MEDIASOURCE_STATE_LOADING:
        view.textField.stringValue = @"Loading...";
        break;
    case DDB_MEDIASOURCE_STATE_SCANNING:
        view.textField.stringValue = @"Scanning...";
        break;
    case DDB_MEDIASOURCE_STATE_INDEXING:
        view.textField.stringValue = @"Indexing...";
        break;
    case DDB_MEDIASOURCE_STATE_SAVING:
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

- (void)medialibEvent:(ddb_mediasource_event_type_t)event {
    if (self.medialibPlugin == NULL) {
        return;
    }
    if (event == DDB_MEDIASOURCE_EVENT_CONTENT_CHANGED || event == DDB_MEDIASOURCE_EVENT_SCAN_DID_COMPLETE) {
        [self filterChanged];
    }
    else if (event == DDB_MEDIASOURCE_EVENT_STATE_CHANGED) {
        int state = self.medialibPlugin->plugin.scanner_state (self.medialibSource);
        if (state != DDB_MEDIASOURCE_STATE_IDLE) {
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

    ddb_playItem_t *playItem = [self selectedPlayItem];

    if (playItem) {
        ddb_playItem_t *it = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy (it, playItem);
        deadbeef->plt_insert_item (curr_plt, NULL, it);
        deadbeef->pl_item_unref (it);
        deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, 0, 0);
    }
}

#pragma mark - NSOutlineViewDataSource

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if (item == nil) {
        return self.topLevelItems.count;
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
        return self.topLevelItems[index];
    }
    else if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        return [mlItem childAtIndex:index];
    }

    return [NSString stringWithFormat:@"Error %d", (int)index];
}

- (void)arrayOfPlayableItemsForItem:(MediaLibraryItem *)item outputArray:(out NSMutableArray<MediaLibraryItem *> *)items {
    if (item.playItem != NULL) {
        [items addObject:item];
    }

    for (MediaLibraryItem *child in item.children) {
        [self arrayOfPlayableItemsForItem:child outputArray:items];
    }
}

#pragma mark - NSOutlineViewDataSource - Drag and drop

- (id<NSPasteboardWriting>)outlineView:(NSOutlineView *)outlineView pasteboardWriterForItem:(MediaLibraryItem *)item {
    if (![item isKindOfClass:MediaLibraryItem.class]) {
        return nil;
    }

    NSMutableArray<MediaLibraryItem *> *items = [NSMutableArray new];
    [self arrayOfPlayableItemsForItem:item outputArray:items];

    ddb_playItem_t **playItems = calloc(items.count, sizeof (ddb_playItem_t *));
    NSInteger count = 0;

    for (MediaLibraryItem *playableItem in items) {
        playItems[count++] = playableItem.playItem;
    }

    return [[MedialibItemDragDropHolder alloc] initWithItems:playItems count:count];
}


- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id<NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)index {
    return NSDragOperationNone;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id<NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index {
    return NO;
}

#pragma mark - NSOutlineViewDelegate

static void cover_get_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    MediaLibraryCoverQueryData *data = (__bridge MediaLibraryCoverQueryData *)(query->user_data);
    [data.viewController coverGetCallbackWithQuery:query coverInfo:cover error:error];
}

- (NSString *)albumArtCacheKeyForTrack:(ddb_playItem_t *)track {
    const char *artist = deadbeef->pl_find_meta (track, "artist") ?: "Unknown Artist";
    const char *album = deadbeef->pl_find_meta (track, "album") ?: "Unknown Album";

    return [NSString stringWithFormat:@"artist:%s;album:%s", artist, album];
}

- (void)coverGetCallbackWithQuery:(ddb_cover_query_t *)query coverInfo:(ddb_cover_info_t *)cover error:(int)error {
    MediaLibraryCoverQueryData *data = (__bridge_transfer MediaLibraryCoverQueryData *)(query->user_data);
    if (!error) {
        NSImage *image;
        if (cover->image_filename) {
            image = [[NSImage alloc] initByReferencingFile:[NSString stringWithUTF8String:cover->image_filename]];
        }
        else if (cover->blob) {
            NSData *blobData = [NSData dataWithBytes:cover->blob length:cover->blob_size];
            image = [[NSImage alloc] initWithData:blobData];
        }

        if (image) {

            // resize
            CGFloat scale;
            NSSize size = image.size;
            if (size.width > size.height) {
                scale = 24/size.width;
            }
            else {
                scale = 24/size.height;
            }
            size.width *= scale;
            size.height *= scale;

            if (size.width >= 1 && size.height >= 1) {
                NSImage *smallImage = [[NSImage alloc] initWithSize:size];
                [smallImage lockFocus];
                [image setSize:size];
                NSGraphicsContext.currentContext.imageInterpolation = NSImageInterpolationHigh;
                [image drawAtPoint:NSZeroPoint fromRect:CGRectMake(0, 0, size.width, size.height) operation:NSCompositingOperationCopy fraction:1.0];
                [smallImage unlockFocus];
                image = smallImage;
                NSString *key = [self albumArtCacheKeyForTrack:query->track];
                self.albumArtCache[key] = image;
            }
            else {
                image = nil;
            }
        }

        // FIXME: this is a memory leak
        if (self.artworkPlugin != NULL) {
            self.artworkPlugin->cover_info_release (cover);
        }
        else {
            NSLog(@"ERROR: MediaLibraryOutlineViewController: Need to call cover_info_release but artworkPlugin is NULL");
        }
        cover = NULL;

        dispatch_async(dispatch_get_main_queue(), ^{
            data.item.coverImage = image;

            NSInteger row = [self.outlineView rowForItem:data.item];
            if (row >= 0) {
                NSTableCellView *cellView = [[self.outlineView rowViewAtRow:row makeIfNecessary:NO] viewAtColumn:0];
                if (cellView) {
                    cellView.imageView.image = image;
                }
            }
        });
    }

    deadbeef->pl_item_unref (query->track);
    free (query);
}

- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
    NSTableCellView *view;
    if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        ddb_playItem_t *it = NULL;
        if (mlItem.numberOfChildren) {
            it = [mlItem childAtIndex:0].playItem;
        }
        if (!it) {
            view = [outlineView makeViewWithIdentifier:@"TextCell" owner:self];
        }
        else {
            view = [outlineView makeViewWithIdentifier:@"ImageTextCell" owner:self];
        }
        if (item == self.medialibRootItem) {
            [self updateMedialibStatusForView:view];
        }
        else {
            view.textField.stringValue = mlItem.stringValue;

            if (it) {
                if (mlItem.coverImage) {
                    view.imageView.image = mlItem.coverImage;
                }
                else {
                    NSString *key = [self albumArtCacheKeyForTrack:it];
                    NSImage *image = self.albumArtCache[key];
                    if (image) {
                        view.imageView.image = image;
                    }
                    else {
                        view.imageView.image = nil;
                        if (!mlItem.coverObtained) {
                            ddb_cover_query_t *query = calloc (sizeof (ddb_cover_query_t), 1);
                            query->_size = sizeof (ddb_cover_query_t);
                            MediaLibraryCoverQueryData *data = [MediaLibraryCoverQueryData new];
                            data.item = mlItem;
                            data.viewController = self;
                            query->user_data = (__bridge_retained void *)(data);
                            query->flags = DDB_ARTWORK_FLAG_NO_CACHE|DDB_ARTWORK_FLAG_LOAD_BLOB;
                            query->track = it;
                            deadbeef->pl_item_ref (it);
                            self.artworkPlugin->cover_get(query, cover_get_callback);
                            mlItem.coverObtained = YES;
                        }
                    }
                }
            }
        }
    }
    else if (item == self.selectorItem) {
        view = [outlineView makeViewWithIdentifier:@"FilterSelectorCell" owner:self];
        MediaLibrarySelectorCellView *selectorCellView = (MediaLibrarySelectorCellView *)view;
        selectorCellView.delegate = self;

        [selectorCellView.popupButton removeAllItems];

        // populate the selector popup
        for (int i = 0; _selectors[i]; i++) {
            const char *name = self.medialibPlugin->plugin.selector_name (self.medialibSource, _selectors[i]);
            [selectorCellView.popupButton addItemWithTitle:[NSString stringWithUTF8String:name]];
        }

        [selectorCellView.popupButton selectItemAtIndex:self.lastSelectedIndex];
        if (view.textField) {
            view.textField.stringValue = item;
        }
    }
    else if (item == self.searchItem) {
        view = [outlineView makeViewWithIdentifier:@"SearchCell" owner:self];
        MediaLibrarySearchCellView *searchCellView = (MediaLibrarySearchCellView *)view;
        searchCellView.delegate = self;
    }
    return view;
}

- (void)filterChanged {
    [self initializeTreeView:(int)self.lastSelectedIndex];
    [self.outlineView expandItem:self.medialibRootItem expandChildren:self.searchString!=nil];
}

#pragma mark - TrackContextMenuDelegate

- (void)trackProperties {
    if (!self.trkProperties) {
        self.trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
    }
    self.trkProperties.mediaLibraryItems = self.selectedItems;
    [self.trkProperties showWindow:self];
}

- (void)playlistChanged {
    // FIXME
}

- (ddb_playItem_t *)selectedPlayItem {
    NSInteger row = -1;
    MediaLibraryItem *item;

    row = self.outlineView.clickedRow;
    if (row == -1) {
        return NULL;
    }

    item = [self.outlineView itemAtRow:row];
    if (!item || ![item isKindOfClass:MediaLibraryItem.class]) {
        return NULL;
    }

    return item.playItem;
}

- (void)addSelectedItemsRecursively:(MediaLibraryItem *)item {
    if (![item isKindOfClass:MediaLibraryItem.class]) {
        return;
    }

    ddb_playItem_t *it = item.playItem;
    if (it) {
        [self.selectedItems addObject:item];
    }

    for (NSUInteger i = 0; i < item.numberOfChildren; i++) {
        [self addSelectedItemsRecursively:item.children[i]];
    }
}

- (void)menuNeedsUpdate:(TrackContextMenu *)menu {
    [self.selectedItems removeAllObjects];
    NSInteger clickedRow = self.outlineView.clickedRow;
    if (clickedRow != -1 && [self.outlineView isRowSelected:clickedRow]) {
        [self.outlineView.selectedRowIndexes enumerateIndexesUsingBlock:^(NSUInteger row, BOOL * _Nonnull stop) {
            [self addSelectedItemsRecursively:[self.outlineView itemAtRow:row]];
        }];
    }
    else if (clickedRow != -1) {
        [self addSelectedItemsRecursively:[self.outlineView itemAtRow:clickedRow]];
    }


    ddb_playItem_t **tracks = NULL;
    NSInteger count = 0;

    if (self.selectedItems.count) {
        tracks = calloc (sizeof (ddb_playItem_t *), self.selectedItems.count);
        for (MediaLibraryItem *item in self.selectedItems) {
            tracks[count++] = item.playItem;
        }
    }

    [self.trackContextMenu updateWithTrackList:tracks count:count playlist:NULL currentTrack:NULL currentTrackIdx:-1];

    free (tracks);
}

- (NSIndexSet *)outlineView:(NSOutlineView *)outlineView selectionIndexesForProposedSelection:(NSIndexSet *)proposedSelectionIndexes {
    NSMutableIndexSet *selectionIndexes = [NSMutableIndexSet new];

    // prevent selecting filter items
    [proposedSelectionIndexes enumerateIndexesUsingBlock:^(NSUInteger row, BOOL * _Nonnull stop) {
        id item = [self.outlineView itemAtRow:row];
        if (item != self.selectorItem
            && item != self.medialibRootItem
            && item != self.searchItem) {
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

#pragma mark - MediaLibrarySearchCellViewDelegate
- (void)mediaLibrarySearchCellViewTextChanged:(nonnull NSString *)text {
    if ([text isEqualToString:@""]) {
        self.searchString = nil;
    }
    else {
        self.searchString = text;
    }
    [self filterChanged];
}

@end
