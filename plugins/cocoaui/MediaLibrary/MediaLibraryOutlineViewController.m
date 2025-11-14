//
//  MediaLibraryOutlineViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#include "scriptable/scriptable.h"
#import "DdbShared.h"
#import "medialib.h"
#import "artwork.h"
#import "AppDelegate.h"
#import "MediaLibraryOutlineView.h"
#import "MediaLibraryItem.h"
#import "MediaLibraryOutlineViewController.h"
#import "DdbPlayItemPasteboardSerializer.h"
#import "TrackContextMenu.h"
#import "TrackPropertiesWindowController.h"
#import "UndoIntegration.h"
#import "Weakify.h"

extern DB_functions_t *deadbeef;

static void *kPresetCtx = &kPresetCtx;

@interface MediaLibraryOutlineViewController() <NSOutlineViewDataSource,MediaLibraryOutlineViewDelegate,TrackContextMenuDelegate,TrackPropertiesWindowControllerDelegate> {
    int64_t _modelListenerId;
}

@property (nonatomic) MediaLibraryItem *medialibRootItem;

@property (nullable, nonatomic) NSString *searchString;

@property (nonatomic) NSArray *topLevelItems;

@property (nonatomic) int artworkListenerId;
@property (nonatomic) int medialibListenerId;

@property (nonatomic) NSOutlineView *outlineView;
@property (nonatomic) NSSearchField *searchField;

@property (atomic) DB_mediasource_t *medialibPlugin;
@property (atomic,readonly) ddb_mediasource_source_t *medialibSource;
@property (atomic) ddb_artwork_plugin_t *artworkPlugin;

@property (nonatomic) ddb_medialib_item_t *medialibItemTree;

@property (nonatomic) NSMutableArray<MediaLibraryItem *> *selectedItems;

@property (nonatomic) TrackContextMenu *trackContextMenu;
@property (nonatomic) TrackPropertiesWindowController *trkProperties;

@property (nonatomic) NSMutableDictionary<NSString *,NSImage *> *albumArtCache;

@property (nonatomic) NSImage *folderImage;

@property (nonatomic) BOOL hasChangedSelection;

@end

@implementation MediaLibraryOutlineViewController

- (MediaLibraryManager *)mediaLibraryManager {
    AppDelegate *appDelegate = NSApplication.sharedApplication.delegate;
    return appDelegate.mediaLibraryManager;
}

- (ddb_mediasource_source_t *)medialibSource {
    return self.mediaLibraryManager.source;
}

- (instancetype)init {
    return [self initWithOutlineView:[NSOutlineView new] searchField:[NSSearchField new]];
}

static void
_model_listener (struct scriptableModel_t *model, void *user_data) {
    MediaLibraryOutlineViewController *self = (__bridge MediaLibraryOutlineViewController *)user_data;
    [self modelListener];
}

- (void)modelListener {
    [self initializeTreeView];
}

- (instancetype)initWithOutlineView:(NSOutlineView *)outlineView searchField:(NSSearchField *)searchField {
    self = [super init];
    if (!self) {
        return nil;
    }

    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(applicationWillQuit:) name:@"ApplicationWillQuit" object:nil];

    [self.mediaLibraryManager addObserver:self forKeyPath:@"preset" options:0 context:kPresetCtx];

    self.outlineView = outlineView;
    self.outlineView.dataSource = self;
    self.outlineView.delegate = self;
    [self.outlineView registerForDraggedTypes:@[ddbPlaylistDataUTIType]];

    self.searchField = searchField;
    self.searchField.target = self;
    self.searchField.action = @selector(searchFieldAction:);

    self.medialibPlugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    self.artworkPlugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");

    if (self->_artworkPlugin != NULL) {
        self.artworkPlugin->add_listener(_artwork_listener, (__bridge void *)self);
    }
    self.medialibListenerId = self.medialibPlugin->add_listener (self.medialibSource, _medialib_listener, (__bridge void *)self);

    self.trackContextMenu = [[TrackContextMenu alloc] initWithView:self.outlineView];
    self.outlineView.menu = self.trackContextMenu;
    self.outlineView.menu.delegate = self;

    [self initializeTreeView];

    scriptableModel_t *model = self.mediaLibraryManager.model;
    _modelListenerId = scriptableModelGetAPI(model)->add_listener (model, _model_listener, (__bridge void *)self);

    [self.outlineView expandItem:self.medialibRootItem];

    [self updateMedialibStatus];

    self.outlineView.doubleAction = @selector(outlineViewDoubleAction:);
    self.outlineView.target = self;

    self.albumArtCache = [NSMutableDictionary new];

    self.selectedItems = [NSMutableArray new];

    self.folderImage = [NSWorkspace.sharedWorkspace iconForFileType:NSFileTypeForHFSTypeCode(kGenericFolderIcon)];

    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == kPresetCtx) {
        weakify(self);
        dispatch_async(dispatch_get_main_queue(), ^{
            strongify(self);
            // NOTE: don't add a check for whether user changed to another preset.
            // This would break a refresh if the current preset changes settings.
            [self filterChanged];
        });
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)disconnect {
    scriptableModel_t *model = self.mediaLibraryManager.model;
    scriptableModelGetAPI(model)->remove_listener(model, _modelListenerId);
    _modelListenerId = 0;

    if (self.medialibPlugin == NULL) {
        return;
    }
    if (self.medialibListenerId != -1) {
        self.medialibPlugin->remove_listener (self.medialibSource, self.medialibListenerId);
        self.medialibListenerId = -1;
    }
    self.medialibPlugin = NULL;
    self.artworkPlugin = NULL;
}

- (void)applicationWillQuit:(NSNotification *)notification {
    [self disconnect];
    NSLog(@"MediaLibraryOutlineViewController: received applicationWillQuit notification");
}

- (void)dealloc {
    if (self.artworkPlugin != NULL) {
        self.artworkPlugin->remove_listener(_artwork_listener, (__bridge void *)self);
    }
    [self.mediaLibraryManager removeObserver:self forKeyPath:@"preset" context:kPresetCtx];
    [self disconnect];
    [NSNotificationCenter.defaultCenter removeObserver:self];
}

static void
_artwork_listener (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2) {
    MediaLibraryOutlineViewController *ctl = (__bridge MediaLibraryOutlineViewController *)user_data;
    weakify(ctl);
    dispatch_async(dispatch_get_main_queue(), ^{
        strongify(ctl);
        [ctl artworkEvent:event];
    });
}

static void
_medialib_listener (ddb_mediasource_event_type_t event, void *user_data) {
    MediaLibraryOutlineViewController *ctl = (__bridge MediaLibraryOutlineViewController *)user_data;
    weakify(ctl);
    dispatch_async(dispatch_get_main_queue(), ^{
        strongify(ctl);
        [ctl medialibEvent:event];
    });
}

- (void)initializeTreeView {
    if (self.medialibItemTree) {
        self.medialibPlugin->free_item_tree (self.medialibSource, self.medialibItemTree);
        self.medialibItemTree = NULL;
    }

    scriptableItem_t *tfQueryRoot =  self.medialibPlugin->get_queries_scriptable(self.medialibSource);

    scriptableModel_t *model = self.mediaLibraryManager.model;
    char *cPresetName = scriptableModelGetAPI(model)->get_active_name(model);
    NSString *presetName = @(cPresetName);
    free (cPresetName);
    scriptableItem_t *preset = NULL;
    if (presetName != nil) {
        preset = scriptableItemSubItemForName(tfQueryRoot, presetName.UTF8String);
    }
    if (preset == NULL) {
        preset = scriptableItemChildren(tfQueryRoot);
    }

    self.medialibItemTree = self.medialibPlugin->create_item_tree (self.medialibSource, preset, self.searchString ? self.searchString.UTF8String : NULL);
    self.medialibRootItem = [[MediaLibraryItem alloc] initWithItem:self.medialibItemTree];

    self.topLevelItems = @[
        self.medialibRootItem,
    ];

    [self.outlineView reloadData];

    // Restore selected/expanded state
    // Defer one frame, since the row indexes are unavailable immediately.
    weakify(self);
    dispatch_async(dispatch_get_main_queue(), ^{
        strongify(self);
        if (self == nil || self.medialibPlugin == NULL) {
            return;
        }
        [self applyStoredState];
    });
}

- (void)applyStoredState {
    NSMutableIndexSet *selectedRowIndexes = [NSMutableIndexSet new];
    [self.outlineView beginUpdates];
    [self restoreSelectedExpandedStateForItem:self.medialibRootItem selectedRows:selectedRowIndexes];
    [self.outlineView selectRowIndexes:selectedRowIndexes byExtendingSelection:NO];
    [self.outlineView endUpdates];
}

- (void)saveSelectionStateWithItem:(MediaLibraryItem *)item rowIndex:(NSInteger)rowIndex {
    const ddb_medialib_item_t *medialibItem = item.medialibItem;
    if (medialibItem == NULL) {
        return;
    }

    BOOL selected = [self.outlineView isRowSelected:rowIndex];
    BOOL expanded = [self.outlineView isItemExpanded:item];
    self.medialibPlugin->set_tree_item_selected (self.medialibSource, medialibItem, selected ? 1 : 0);
    self.medialibPlugin->set_tree_item_expanded (self.medialibSource, medialibItem, expanded ? 1 : 0);

    for (NSUInteger i = 0; i < item.numberOfChildren; i++) {
        [self saveSelectionStateWithItem:item.children[i] rowIndex:i];
    }
}

- (void)restoreSelectedExpandedStateForItem:(MediaLibraryItem *)item selectedRows:(NSMutableIndexSet *)selectedRows {
    const ddb_medialib_item_t *medialibItem = item.medialibItem;
    if (medialibItem == NULL) {
        return;
    }

    int selected = self.medialibPlugin->is_tree_item_selected (self.medialibSource, medialibItem);
    int expanded = self.medialibPlugin->is_tree_item_expanded (self.medialibSource, medialibItem);

    if (expanded) {
        [self.outlineView expandItem:item expandChildren:NO];
    }
    else {
        [self.outlineView collapseItem:item collapseChildren:NO];
    }

    if (selected) {
        NSInteger rowIndex = [self.outlineView rowForItem:item];
        if (rowIndex != -1) {
            [selectedRows addIndex:rowIndex];
        }
    }

    for (NSUInteger i = 0; i < item.numberOfChildren; i++) {
        [self restoreSelectedExpandedStateForItem:item.children[i] selectedRows:selectedRows];
    }
}

- (void)updateMedialibStatusForView:(NSTableCellView *)view {
    ddb_mediasource_state_t state = self.medialibPlugin->scanner_state (self.medialibSource);
    int enabled = self.medialibPlugin->is_source_enabled (self.medialibSource);
    switch (state) {
    case DDB_MEDIASOURCE_STATE_IDLE:
        view.textField.stringValue = enabled ? @"All Music" : @"Media library is disabled";
        break;
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

- (void)resetArtworkForItem:(MediaLibraryItem *)item {
    item.coverImage = nil;
    item.coverObtained = NO;
    for (MediaLibraryItem *child in item.children) {
        [self resetArtworkForItem:child];
    }
}

- (void)artworkEvent:(ddb_artwork_listener_event_t)event {
    if (event == DDB_ARTWORK_SETTINGS_DID_CHANGE) {
        [self resetArtworkForItem:self.medialibRootItem];
        [self.albumArtCache removeAllObjects];
        [self.outlineView reloadData];
    }
}

- (void)medialibEvent:(ddb_mediasource_event_type_t)event {
    if (self.medialibPlugin == NULL) {
        return;
    }
    switch (event) {
    case DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE:
        [self filterChanged];
        break;
    case DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE:
    case DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE:
        [self updateMedialibStatus];
        break;
    case DDB_MEDIASOURCE_EVENT_OUT_OF_SYNC:
        [self refreshMediasource];
        break;
    }
}

- (ddb_playlist_t *)getDestPlaylist {
    ddb_playlist_t *curr_plt = NULL;
    if (deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1)) {
        char str[200];
        deadbeef->conf_get_str ("cli_add_playlist_name", "Default", str, sizeof (str));
        curr_plt = deadbeef->plt_find_by_name (str);
        if (!curr_plt) {
            curr_plt = deadbeef->plt_append (str);
        }
    }
    return curr_plt;
}

- (int)addSelectionToPlaylist:(ddb_playlist_t *)plt {
    MediaLibraryItem *item = [self selectedItem];
    NSMutableArray<MediaLibraryItem *> *items = [NSMutableArray new];
    [self arrayOfPlayableItemsForItem:item outputArray:items];

    int count = 0;

    ddb_undo->group_begin();

    ddb_playItem_t *prev = deadbeef->plt_get_last(plt, PL_MAIN);
    for (item in items) {
        ddb_playItem_t *playItem = item.playItem;
        if (playItem == NULL) {
            continue;
        }
        ddb_playItem_t *it = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy (it, playItem);
        deadbeef->plt_insert_item (plt, prev, it);
        if (prev != NULL) {
            deadbeef->pl_item_unref (prev);
        }
        prev = it;
        count += 1;
    }

    ddb_undo->group_end ();

    if (prev != NULL) {
        deadbeef->pl_item_unref (prev);
    }
    prev = NULL;

    deadbeef->plt_save_config (plt);

    return count;
}

- (void)outlineViewDoubleAction:(NSOutlineView *)sender {
    NSInteger row = self.outlineView.selectedRow;
    if (row == -1) {
        return;
    }

    ddb_playlist_t * curr_plt = [self getDestPlaylist];
    if (!curr_plt) {
        return;
    }

    deadbeef->plt_set_curr (curr_plt);
    deadbeef->plt_clear(curr_plt);

    int count = [self addSelectionToPlaylist:curr_plt];

    deadbeef->plt_unref (curr_plt);

    ddb_undo->set_action_name ("Add Files");

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, DDB_PLAYLIST_CHANGE_CONTENT, 0, 0);
    if (count > 0) {
        deadbeef->sendmessage(DB_EV_PLAY_NUM, 0, 0, 0);
    }
}

- (void)filterChanged {
    [self initializeTreeView];
    [self.outlineView expandItem:self.medialibRootItem expandChildren:self.searchString!=nil];
}

- (void)arrayOfPlayableItemsForItem:(MediaLibraryItem *)item outputArray:(out NSMutableArray<MediaLibraryItem *> *)items {
    if (item.playItem != NULL) {
        [items addObject:item];
    }

    for (MediaLibraryItem *child in item.children) {
        [self arrayOfPlayableItemsForItem:child outputArray:items];
    }
}

- (int)widgetMessage:(int)_id ctx:(uint64_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    return 0;
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

    DdbPlayItemPasteboardSerializer *writer = [[DdbPlayItemPasteboardSerializer alloc] initWithItems:playItems count:count];
    free (playItems);
    return writer;
}


- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id<NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)index {
    return NSDragOperationNone;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id<NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index {
    return NO;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldSelectItem:(id)item {
    return [item isKindOfClass:MediaLibraryItem.class];
}

#pragma mark - NSOutlineViewDelegate

static void cover_get_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    void (^completionBlock)(ddb_cover_query_t *query, ddb_cover_info_t *cover, int error) = (void (^)(ddb_cover_query_t *query, ddb_cover_info_t *cover, int error))CFBridgingRelease(query->user_data);
    completionBlock(query, cover, error);
}

- (NSString *)albumArtCacheKeyForTrack:(ddb_playItem_t *)track {
    const char *artist = deadbeef->pl_find_meta (track, "artist");
    const char *album = deadbeef->pl_find_meta (track, "album");

    if (artist != NULL && album != NULL) {
        return [NSString stringWithFormat:@"artist:%s;album:%s", artist, album];
    }
    else {
        const char *path = deadbeef->pl_find_meta(track, ":URI");
        return [NSString stringWithFormat:@"file:%s", path];
    }
}

// NOTE: this is running on background thread
- (NSImage *)getImage:(ddb_cover_query_t *)query coverInfo:(ddb_cover_info_t *)cover error:(int)error {
    if (error) {
        return nil;
    }

    NSImage *image;
    if (cover->image_filename) {
        image = [[NSImage alloc] initByReferencingFile:@(cover->image_filename)];
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
            image.size = size;
            NSGraphicsContext.currentContext.imageInterpolation = NSImageInterpolationHigh;
            [image drawAtPoint:NSZeroPoint fromRect:CGRectMake(0, 0, size.width, size.height) operation:NSCompositingOperationCopy fraction:1.0];
            [smallImage unlockFocus];
            image = smallImage;
        }
        else {
            image = nil;
        }
    }

    // NOTE: this would not cause a memory leak, since the artwork plugin keeps track of the covers, and will free them at exit
    if (self.artworkPlugin != NULL) {
        self.artworkPlugin->cover_info_release (cover);
    }
    cover = NULL;

    return image;
}

- (void)updateCoverForItem:(MediaLibraryItem *)item track:(ddb_playItem_t *)track {
    void (^completionBlock)(ddb_cover_query_t *query, ddb_cover_info_t *cover, int error) = ^(ddb_cover_query_t *query, ddb_cover_info_t *cover, int error) {
        NSImage *image = [self getImage:query coverInfo:cover error:error];

        weakify(self);
        dispatch_async(dispatch_get_main_queue(), ^{
            strongify(self);
            if (image != nil) {
                NSString *key = [self albumArtCacheKeyForTrack:query->track];
                self.albumArtCache[key] = image;
            }
            deadbeef->pl_item_unref (query->track);
            free (query);
            NSInteger row = [self.outlineView rowForItem:item];
            if (row == -1) {
                return;
            }

            if (image == nil && item.children.count != 0) {
                item.coverImage = self.folderImage;
            }
            else {
                item.coverImage = image;
            }

            NSTableRowView *rowView = [self.outlineView rowViewAtRow:row makeIfNecessary:NO];
            NSTableCellView *cellView = [rowView viewAtColumn:0];
            cellView.imageView.image = item.coverImage;
        });
    };
    ddb_cover_query_t *query = calloc (1, sizeof (ddb_cover_query_t));
    query->_size = sizeof (ddb_cover_query_t);
    query->user_data = (void *)CFBridgingRetain(completionBlock);
    query->track = track;
    deadbeef->pl_item_ref (track);
    self.artworkPlugin->cover_get(query, cover_get_callback);

}

- (nullable NSView *)outlineView:(NSOutlineView *)outlineView viewForTableColumn:(nullable NSTableColumn *)tableColumn item:(id)item {
    NSTableCellView *view;
    if ([item isKindOfClass:MediaLibraryItem.class]) {
        MediaLibraryItem *mlItem = item;
        ddb_playItem_t *it = NULL;
        if (mlItem.numberOfChildren) {
            it = [mlItem childAtIndex:0].playItem;
        }
        if (item == self.medialibRootItem) {
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
            view.imageView.image = nil;

            if (mlItem.coverImage == nil && mlItem.children.count != 0) {
                view.imageView.image = self.folderImage;
            }

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
                    else if (self.artworkPlugin != NULL) {
                        view.imageView.image = nil;
                        if (!mlItem.coverObtained) {
                            NSInteger row = [self.outlineView rowForItem:mlItem];
                            if (row >= 0) {
                                [self updateCoverForItem:mlItem track:it];
                            }
                            mlItem.coverObtained = YES;
                        }
                    }
                }
            }
        }
    }
    return view;
}

- (void)refreshMediasource {
    if (self.hasChangedSelection) {
        self.hasChangedSelection = NO;
        [self saveSelectionStateWithItem:self.medialibRootItem rowIndex: 0];
    }
    self.medialibPlugin->refresh(self.medialibSource);
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification {
    NSObject *object = notification.userInfo[@"NSObject"];
    if (![object isKindOfClass:MediaLibraryItem.class]) {
        return;
    }

    MediaLibraryItem *item = (MediaLibraryItem *)object;

    const ddb_medialib_item_t *medialibItem = item.medialibItem;
    if (medialibItem != NULL) {
        self.medialibPlugin->set_tree_item_expanded (self.medialibSource, medialibItem, 1);
    }
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification {
    NSObject *object = notification.userInfo[@"NSObject"];
    if (![object isKindOfClass:MediaLibraryItem.class]) {
        return;
    }

    MediaLibraryItem *item = (MediaLibraryItem *)object;

    const ddb_medialib_item_t *medialibItem = item.medialibItem;
    if (medialibItem != NULL) {
        self.medialibPlugin->set_tree_item_expanded (self.medialibSource, medialibItem, 0);
    }
}

- (void)outlineViewSelectionDidChange:(NSNotification *)notification {
    self.hasChangedSelection = YES;
}

#pragma mark - MediaLibraryOutlineViewDelegate

- (void)mediaLibraryOutlineViewDidActivateAlternative:(MediaLibraryOutlineView *)outlineView {
    ddb_playlist_t * curr_plt = [self getDestPlaylist];
    if (!curr_plt) {
        return;
    }

    deadbeef->plt_set_curr (curr_plt);

    [self addSelectionToPlaylist:curr_plt];

    deadbeef->plt_unref (curr_plt);

    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, DDB_PLAYLIST_CHANGE_CONTENT, 0, 0);
}

- (BOOL)mediaLibraryOutlineView:(MediaLibraryOutlineView *)outlineView shouldDisplayMenuForRow:(NSInteger)row {
    id item = [self.outlineView itemAtRow:row];
    return [item isKindOfClass:MediaLibraryItem.class];
}

#pragma mark - TrackContextMenuDelegate

- (void)trackContextMenuShowTrackProperties:(TrackContextMenu *)trackContextMenu {
    if (!self.trkProperties) {
        self.trkProperties = [[TrackPropertiesWindowController alloc] initWithWindowNibName:@"TrackProperties"];
    }
    self.trkProperties.mediaLibraryItems = self.selectedItems;
    self.trkProperties.delegate = self;
    [self.trkProperties showWindow:self];
}

- (void)trackContextMenuDidReloadMetadata:(TrackContextMenu *)trackContextMenu {
    [self refreshMediasource];
}

- (void)trackContextMenuDidDeleteFiles:(TrackContextMenu *)trackContextMenu cancelled:(BOOL)cancelled {
    if (!cancelled) {
        [self refreshMediasource];
    }
}

#pragma mark - TrackPropertiesWindowControllerDelegate

- (void)trackPropertiesWindowControllerDidUpdateTracks:(TrackPropertiesWindowController *)windowController {
    [self refreshMediasource];
}

- (MediaLibraryItem *)selectedItem {
    NSInteger row = -1;
    MediaLibraryItem *item;

    row = self.outlineView.selectedRow;
    if (row == -1) {
        return NULL;
    }

    item = [self.outlineView itemAtRow:row];
    if (!item || ![item isKindOfClass:MediaLibraryItem.class]) {
        return NULL;
    }

    return item;
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
        tracks = calloc (self.selectedItems.count, sizeof (ddb_playItem_t *));
        for (MediaLibraryItem *item in self.selectedItems) {
            ddb_playItem_t *it = deadbeef->pl_item_alloc();
            deadbeef->pl_item_copy (it, item.playItem);
            tracks[count++] = it;
        }
    }

    [self.trackContextMenu updateWithTrackList:tracks count:count playlist:NULL currentTrack:NULL currentTrackIdx:-1];

    ddb_playlist_t *plt = deadbeef->plt_alloc("MediaLib Action Playlist");

    ddb_playItem_t *after = NULL;
    for (int i = 0; i < count; i++) {
        after = deadbeef->plt_insert_item(plt, after, tracks[i]);
    }
    deadbeef->plt_select_all(plt);

    deadbeef->action_set_playlist(plt);
    [self.trackContextMenu update:plt actionContext:DDB_ACTION_CTX_PLAYLIST isMediaLib:YES];

    deadbeef->plt_unref(plt);

    for (int i = 0; i < count; i++) {
        deadbeef->pl_item_unref (tracks[i]);
    }

    free (tracks);
}

- (NSIndexSet *)outlineView:(NSOutlineView *)outlineView selectionIndexesForProposedSelection:(NSIndexSet *)proposedSelectionIndexes {
    NSMutableIndexSet *selectionIndexes = [NSMutableIndexSet new];

    // prevent selecting filter items
    [proposedSelectionIndexes enumerateIndexesUsingBlock:^(NSUInteger row, BOOL * _Nonnull stop) {
        id item = [self.outlineView itemAtRow:row];
        if (item != self.medialibRootItem) {
            [selectionIndexes addIndex:row];
        }
    }];
    return selectionIndexes;
}

#pragma mark - NSSearchField

- (void)mediaLibrarySearchCellViewTextChanged:(nonnull NSString *)text {
}

- (IBAction)searchFieldAction:(NSSearchField *)sender {
    NSString *text = self.searchField.stringValue;
    if ([text isEqualToString:@""]) {
        self.searchString = nil;
    }
    else {
        self.searchString = text;
    }
    [self filterChanged];
}

@end
