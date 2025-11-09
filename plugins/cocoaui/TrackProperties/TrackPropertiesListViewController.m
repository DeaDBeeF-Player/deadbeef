/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "AddNewFieldWindowController.h"
#import "EditSingleValueWindowController.h"
#import "MediaLibraryItem.h"
#import "TrackPropertiesListViewController.h"
#import "TrackPropertiesSingleLineFormatter.h"
#include "utf8.h"
#include "trkproperties_shared.h"

extern DB_functions_t *deadbeef;

static char *
_formatted_title_for_unknown_key(const char *key) {
    size_t l = strlen (key);
    char *title = malloc(l*4);
    title[0] = '<';
    char *t = title + 1;
    const char *p = key;
    while (*p) {
        int32_t size = 0;
        u8_nextchar (p, &size);
        int outsize = u8_toupper((const signed char *)p, size, t);
        t += outsize;
        p += size;
    }
    *t++ = '>';
    *t++ = 0;
    return title;
}

// NOTE: add_field gets called once for each unique key (e.g. Artist or Album),
// which means it will usually contain 10-20 fields
static void
add_field (NSMutableArray<TrackPropertiesListItem *> *store, const char *key, const char *title, int is_prop, DB_playItem_t **tracks, int numtracks) {

    // get all values for each key, convert from 0-separated to '; '-separated, and put into NSArray
    NSMutableArray<NSString *> *values = [NSMutableArray new];
    deadbeef->pl_lock ();
    for (int i = 0; i < numtracks; i++) {
        NSString *value = @"";
        DB_metaInfo_t *meta = deadbeef->pl_meta_for_key (tracks[i], key);
        if (meta && meta->valuesize == 1) {
            meta = NULL;
        }

        if (meta) {
            const char *p = meta->value;
            const char *end = p + meta->valuesize;

            while (p < end) {
                value = [value stringByAppendingString:@(p)];
                p += strlen (p) + 1;
                if (p < end) {
                    value = [value stringByAppendingString:p < end-1 ? @"; " : @";"];
                }
            }
        }
        [values addObject:value];
    }
    deadbeef->pl_unlock ();

    TrackPropertiesListItem *item = [TrackPropertiesListItem new];
    item.title = @(title);
    item.key = @(key);
    item.values = values;
    [store addObject:item];
}

@interface TrackPropertiesListViewController () <NSTableViewDataSource, NSTableViewDelegate, NSMenuDelegate, AddNewFieldWindowControllerDelegate, EditSingleValueWindowControllerDelegate>

@property (nonatomic) int iter;
@property (nonatomic) DB_playItem_t **tracks;
@property (nonatomic) int numtracks;
@property (nonatomic) NSUInteger flags;

@property (nonatomic) NSTableView *tableView;

@property (nonatomic) NSMenuItem *editItem;
@property (nonatomic) NSMenuItem *editInPlaceItem;

@property (nonatomic) AddNewFieldWindowController *addNewFieldWindowController;
@property (nonatomic) EditSingleValueWindowController *editSingleValueWindowController;

@end

@implementation TrackPropertiesListViewController

- (void)dealloc {
    [self freeTrackList];
}

- (void)freeTrackList {
    trkproperties_free_track_list (&_tracks, &_numtracks);
    _tracks = NULL;
    _numtracks = 0;
}

- (void)loadFromPlaylist:(ddb_playlist_t *)playlist context:(ddb_action_context_t)context flags:(NSUInteger)flags {
    self.flags = flags;
    [self freeTrackList];

    if (playlist != NULL) {
        trkproperties_build_track_list_for_ctx (playlist, context, &_tracks, &_numtracks);
    }
}

- (void)loadFromMediaLibraryItems:(NSArray<MediaLibraryItem *> *)mediaLibraryItems flags:(NSUInteger)flags {
    self.flags = flags;
    [self freeTrackList];

    if (mediaLibraryItems != nil) {
        NSInteger count = mediaLibraryItems.count;
        _tracks = calloc (count, sizeof (DB_playItem_t *));
        _numtracks = 0;

        for (NSInteger i = 0; i < count; i++) {
            ddb_playItem_t *it = mediaLibraryItems[i].playItem;
            if (it) {
                deadbeef->pl_item_ref (it);
                _tracks[_numtracks++] = it;
            }
        }
    }

    [self reloadData];
}

- (void)loadView {
    NSTableView *tableView = [NSTableView new];
    tableView.usesAlternatingRowBackgroundColors = YES;
    tableView.allowsMultipleSelection = YES;
    tableView.allowsColumnReordering = NO;
    tableView.allowsColumnResizing = YES;
    tableView.allowsColumnSelection = NO;
    tableView.allowsEmptySelection = YES;
    tableView.allowsTypeSelect = YES;
    tableView.allowsExpansionToolTips = YES;
    tableView.headerView = [[NSTableHeaderView alloc] initWithFrame:NSMakeRect(0, 0, 400, 20)];
    tableView.rowSizeStyle = NSTableViewRowSizeStyleDefault;
    tableView.selectionHighlightStyle = NSTableViewSelectionHighlightStyleRegular;
    tableView.columnAutoresizingStyle = NSTableViewUniformColumnAutoresizingStyle;
    tableView.allowsMultipleSelection = YES;

    // --- Column 1: Name ---
    NSTableColumn *nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    nameColumn.title = @"Name";
    nameColumn.resizingMask = NSTableColumnUserResizingMask;
    nameColumn.editable = NO;
    nameColumn.width = 120;

    // --- Column 2: Value ---
    NSTableColumn *valueColumn = [[NSTableColumn alloc] initWithIdentifier:@"value"];
    valueColumn.title = @"Value";
    valueColumn.resizingMask = NSTableColumnUserResizingMask;
    valueColumn.editable = YES;
    valueColumn.width = 300;

    [tableView addTableColumn:nameColumn];
    [tableView addTableColumn:valueColumn];

    // Create scroll view container
    NSScrollView *scrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 400, 300)];
    scrollView.hasVerticalScroller = YES;
    scrollView.hasHorizontalScroller = YES;
    scrollView.autohidesScrollers = YES;
    scrollView.documentView = tableView;

    tableView.autosaveName = @"TrackPropertiesTableView";
    tableView.autosaveTableColumns = YES;

    self.tableView = tableView;
    self.tableView.delegate = self;
    self.tableView.dataSource = self;
    self.view = scrollView;

    NSMenu *menu = [self createContextMenu];
    menu.delegate = self;
    tableView.menu = menu;

    [self viewDidLoad];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self.tableView reloadData];
}

- (NSMenu *)createContextMenu {
    NSMenu *menu = [NSMenu new];
    self.editItem = [menu addItemWithTitle:@"Edit" action:@selector(editValueAction:) keyEquivalent:@""];
    self.editItem.target = self;

    self.editInPlaceItem = [menu addItemWithTitle:@"Edit in Place" action:@selector(editInPlaceAction:) keyEquivalent:@""];
    self.editInPlaceItem.target = self;

    NSMenuItem *removeItem = [menu addItemWithTitle:@"Remove" action:@selector(delete:) keyEquivalent:@""];
    removeItem.target = self;

    NSMenuItem *cropItem = [menu addItemWithTitle:@"Crop" action:@selector(editCropAction:) keyEquivalent:@""];
    cropItem.target = self;

    //    [menu addItem:NSMenuItem.separatorItem];
    //
    //    NSMenuItem *cutItem = [menu addItemWithTitle:@"Cut" action:@selector(cutAction:) keyEquivalent:@""];
    //    cutItem.target = self;
    //
    //    NSMenuItem *copyItem = [menu addItemWithTitle:@"Copy" action:@selector(copyAction:) keyEquivalent:@""];
    //    copyItem.target = self;
    //
    //    NSMenuItem *pasteItem = [menu addItemWithTitle:@"Paste" action:@selector(pasteAction:) keyEquivalent:@""];
    //    pasteItem.target = self;

    [menu addItem:NSMenuItem.separatorItem];

    NSMenuItem *capitalizeItem = [menu addItemWithTitle:@"Capitalize" action:@selector(editCapitalizeAction:) keyEquivalent:@""];
    capitalizeItem.target = self;

    //    NSMenuItem *cleanUpItem = [menu addItemWithTitle:@"Clean Up" action:@selector(cleanUpAction:) keyEquivalent:@""];
    //    cleanUpItem.target = self;
    //    NSMenuItem *formatFromOtherFieldsItem = [menu addItemWithTitle:@"Format from Other Fields" action:@selector(formatFromOtherFieldsAction:) keyEquivalent:@""];
    //    formatFromOtherFieldsItem.target = self;

    [menu addItem:NSMenuItem.separatorItem];

    NSMenuItem *addMewFieldItem = [menu addItemWithTitle:@"Add New Field…" action:@selector(addNewField:) keyEquivalent:@""];
    addMewFieldItem.target = self;

    //    NSMenuItem *pasteFieldsItem = [menu addItemWithTitle:@"Paste Fields" action:@selector(pasteFields:) keyEquivalent:@""];
    //    pasteFieldsItem.target = self;

    //    NSMenuItem *automaticallyFillValuesItem = [menu addItemWithTitle:@"Automatically Fill Values…" action:@selector(automaticallyFillValuesAction:) keyEquivalent:@""];
    //    automaticallyFillValuesItem.target = self;

    return menu;
}

- (void)reloadData {
    [self fillStore];
    [self.tableView reloadData];
}

- (void)fillStore {
    [self.store removeAllObjects];
    if (self.tracks == NULL) {
        return;
    }

    self.isModified = NO;

    deadbeef->pl_lock ();

    if (self.flags & TrackPropertiesListFlagMetadata) {
        [self fillMeta];
    }
    if (self.flags & TrackPropertiesListFlagProperties) {
        [self fillProps];
    }

    deadbeef->pl_unlock ();
}

- (void)fillMeta {
    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 0, self.tracks, self.numtracks);

    // add "standard" fields
    for (int i = 0; trkproperties_types[i]; i += 2) {
        add_field (self.store, trkproperties_types[i], trkproperties_types[i+1], 0, self.tracks, self.numtracks);
    }

    // add all other fields
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; trkproperties_types[i]; i += 2) {
            if (!strcasecmp (keys[k], trkproperties_types[i])) {
                break;
            }
        }
        if (trkproperties_types[i]) {
            continue;
        }

        char *title = _formatted_title_for_unknown_key(keys[k]);
        add_field (self.store, keys[k], title, 0, self.tracks, self.numtracks);
        free (title);
        title = NULL;
    }
    if (keys) {
        free (keys);
    }
}

- (void)fillProps {
    // hardcoded properties
    for (int i = 0; trkproperties_hc_props[i]; i += 2) {
        add_field (self.store, trkproperties_hc_props[i], trkproperties_hc_props[i+1], 1, self.tracks, self.numtracks);
    }
    // properties
    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 1, self.tracks, self.numtracks);
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; trkproperties_hc_props[i]; i += 2) {
            if (!strcasecmp (keys[k], trkproperties_hc_props[i])) {
                break;
            }
        }
        if (trkproperties_hc_props[i]) {
            continue;
        }
        char *title = _formatted_title_for_unknown_key(keys[k] + 1);
        add_field (self.store, keys[k], title, 1, self.tracks, self.numtracks);
        free (title);
        title = NULL;
    }
    if (keys) {
        free (keys);
    }
}

- (NSString *)fieldValueForIndex:(NSInteger)rowIndex store:(NSMutableArray<TrackPropertiesListItem *> *)store isMult:(nullable BOOL *)isMult {
    NSMutableArray<NSString *> *values = store[rowIndex].values;
    // get uniq values
    NSArray *uniq = [NSOrderedSet orderedSetWithArray:values].array;
    NSInteger n = uniq.count;

    NSString *val = n > 1 ? @"[Multiple Values] " : @"";
    for (NSUInteger i = 0; i < uniq.count; i++) {
        val = [val stringByAppendingString:uniq[i]];
        if (i < uniq.count - 1) {
            val = [val stringByAppendingString:@"; "];
        }
    }

    if (isMult != NULL) {
        *isMult = n > 1;
    }

    return val;
}

#pragma mark - Menu item actions

- (IBAction)editValueAction:(id)sender {
    // TODO
#if 0
    NSIndexSet *ind = self.tableView.selectedRowIndexes;
    if (ind.count != 1) {
        return; // multiple fields can't be edited at the same time
    }

    NSInteger idx = ind.firstIndex;

    if (self.numtracks != 1) {
        // Allow editing the previous value, if all tracks have the same
        BOOL isMult;
        NSString *value = [self fieldValueForIndex:idx store:self.store isMult:&isMult];
        if (!isMult) {
            self.multiValueSingle.string = value;
        }
        else {
            self.multiValueSingle.string = @"";
        }

        NSString *key = self.store[idx][@"key"];
        self.multiValueFieldName.stringValue =  key.uppercaseString;

        NSMutableArray<NSString *> *fields = [NSMutableArray new];
        NSMutableArray<NSString *> *items = [NSMutableArray new];

        deadbeef->pl_lock ();

        char *item_tf = deadbeef->tf_compile ("%title%[ // %track artist%]");

        ddb_tf_context_t ctx;
        memset (&ctx, 0, sizeof (ctx));

        ctx._size = sizeof (ctx);
        ctx.plt = NULL;
        ctx.idx = -1;
        ctx.id = -1;

        fields = self.store[idx][@"values"];

        for (int i = 0; i < self.numtracks; i++) {
            char item[1000];
            ctx.it = self.tracks[i];
            deadbeef->tf_eval(&ctx, item_tf, item, sizeof (item));
            [items addObject:@(item)];
        }
        deadbeef->pl_unlock ();
        deadbeef->tf_free (item_tf);

        self.multipleFieldsTableData = [TrackPropertiesMultipleFieldsTableData new];
        self.multipleFieldsTableData.fields = [[NSMutableArray alloc] initWithArray:fields copyItems:NO];
        self.multipleFieldsTableData.items = items;
        self.multiValueTableView.delegate = self.multipleFieldsTableData;
        self.multiValueTableView.dataSource = self.multipleFieldsTableData;
        [self.window beginSheet:self.editMultipleValuesPanel completionHandler:^(NSModalResponse returnCode) {
            if (returnCode == NSModalResponseOK) {
                if ([(self.multiValueTabView).selectedTabViewItem.identifier isEqualToString:@"singleValue"]) {
                    [self setSameValuesForIndex:(int)idx value:(self.multiValueSingle).textStorage.string];
                }
                else {
                    for (int i = 0; i < self.numtracks; i++) {
                        self.store[idx][@"values"] = [[NSMutableArray alloc] initWithArray:self.multipleFieldsTableData.fields copyItems:NO];
                    }
                }
                self.isModified = YES;
            }
        }];
        return;
    }

    self.editSingleValueWindowController = [[EditSingleValueWindowController alloc] initWithWindowNibName:@"EditSingleValueWindowController"];
    self.editSingleValueWindowController.delegate = self;
    (void)self.editSingleValueWindowController.window;

    self.editSingleValueWindowController.fieldName.stringValue =  ((NSString *)self.store[idx][@"key"]).uppercaseString;
    self.editSingleValueWindowController.fieldValue.string =  self.store[idx][@"values"][0];

    [self.window beginSheet:self.editSingleValueWindowController.window completionHandler:^(NSModalResponse returnCode) {
    }];
#endif
}

- (IBAction)editInPlaceAction:(id)sender {
    NSIndexSet *ind = self.tableView.selectedRowIndexes;
    if (ind.count != 1) {
        return; // multiple fields can't be edited at the same time
    }

    NSInteger idx = ind.firstIndex;

    [self.tableView editColumn:1 row:idx withEvent:nil select:YES];
}

- (void)setSameValuesForIndex:(NSUInteger)idx value:(NSString *)value {
    NSMutableArray<NSString *> *values = self.store[idx].values;
    for (NSUInteger i = 0; i < values.count; i++) {
        values[i] = value;
    }
}

- (IBAction)delete:(id)sender {
    NSIndexSet *ind = self.tableView.selectedRowIndexes;

    [ind enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *stop) {
        [self setSameValuesForIndex:(int)idx value:@""];
        self.isModified = YES;
    }];

    if (self.isModified) {
        [self.tableView reloadData];
    }
}

- (IBAction)editCropAction:(id)sender {
    NSIndexSet *ind = self.tableView.selectedRowIndexes;

    for (NSUInteger i = 0; i < self.store.count; i++) {
        if (![ind containsIndex:i]) {
            [self setSameValuesForIndex:i value:@""];
            self.isModified = YES;
        }
    }

    if (self.isModified) {
        [self.tableView reloadData];
    }
}

- (IBAction)editCapitalizeAction:(id)sender {
    NSIndexSet *ind = self.tableView.selectedRowIndexes;

    for (NSUInteger i = 0; i < self.store.count; i++) {
        if ([ind containsIndex:i]) {
            NSMutableArray<NSString *> *values = self.store[i].values;
            for (NSUInteger n = 0; n < values.count; n++) {
                values[n] =  values[n].uppercaseString;
            }
            self.isModified = YES;
        }
    }

    if (self.isModified) {
        [self.tableView reloadData];
    }
}

- (IBAction)addNewField:(id)sender {
    self.addNewFieldWindowController = [[AddNewFieldWindowController alloc] initWithWindowNibName:@"AddNewFieldWindowController"];
    self.addNewFieldWindowController.delegate = self;

    [self.view.window beginSheet:self.addNewFieldWindowController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode != NSModalResponseOK) {
            return;
        }

        NSString *key = self.addNewFieldWindowController.addFieldName.stringValue;

        char *title = _formatted_title_for_unknown_key(key.UTF8String);
        add_field (self.store, key.UTF8String, title, 0, self.tracks, self.numtracks);
        free (title);
        title = NULL;
        self.isModified = YES;
        [self.tableView reloadData];
    }];
}

- (IBAction)cancelEditMultipleValuesPanel:(id)sender {
    // TODO
#if 0
    [self.window endSheet:self.editMultipleValuesPanel returnCode:NSModalResponseCancel];
#endif
}

- (IBAction)okEditMultipleValuesAction:(id)sender {
    // TODO
#if 0
    NSIndexSet *ind = self.tableView.selectedRowIndexes;
    NSInteger idx = ind.firstIndex;

    self.isModified = YES;

    self.store[idx].values = [[NSMutableArray alloc] initWithArray: self.multipleFieldsTableData.fields copyItems:NO];

    [self.tableView reloadData];

    [self.window endSheet:self.editMultipleValuesPanel returnCode:NSModalResponseOK];
#endif
}


#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    return (NSInteger)self.store.count;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    NSMutableArray<TrackPropertiesListItem *> *store = self.store;
    if (self.store == nil) {
        return nil;
    }

    if ([aTableColumn.identifier isEqualToString:@"name"]) {
        NSString *title = store[rowIndex].title;
        return title;
    }
    else if ([aTableColumn.identifier isEqualToString:@"value"]) {
        return [self fieldValueForIndex:rowIndex store:store isMult:NULL];
    }
    return nil;
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    NSArray<TrackPropertiesListItem *> *store = self.store;
    if (store == nil) {
        return;
    }

    TrackPropertiesListItem *item = store[rowIndex];

    NSMutableArray<NSString *> *values = item.values;
    for (NSUInteger i = 0; i < values.count; i++) {
        if ([values[i] isNotEqualTo:anObject]) {
            values[i] = anObject;
            self.isModified = YES;
        }
    }
}

#pragma mark - NSTableViewDelegate

// when editing the "multiple values" cells, turn them into ""
// this, unfortunately, is not undoable, so as soon as the user starts editing -- no way back
- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    NSMutableArray *store = self.store;
    if (store == nil) {
        return;
    }

    if([aTableColumn.identifier isEqualToString:@"value"]){
        ((NSTextFieldCell *)aCell).formatter = [TrackPropertiesSingleLineFormatter new];
    }
}

#pragma mark - NSMenuDelegate

- (void)menuNeedsUpdate:(NSMenu *)menu {
    self.editItem.enabled = self.tableView.selectedRowIndexes.count == 1;
    self.editInPlaceItem.enabled = self.tableView.selectedRowIndexes.count == 1;
}

#pragma mark - AddNewFieldWindowControllerDelegate

- (BOOL)addNewFieldAlreadyExists:(NSString *)newFieldName {
    NSString *key = self.addNewFieldWindowController.addFieldName.stringValue;
    for (NSUInteger i = 0; i < self.store.count; i++) {
        if (NSOrderedSame == [key caseInsensitiveCompare:self.store[i].key]) {
            return YES;
        }
    }
    return NO;
}

- (void)addNewFieldDidEndWithResponse:(NSModalResponse)response {
    [self.view.window endSheet:self.addNewFieldWindowController.window returnCode:response];
}

#pragma mark - EditSingleValueWindowControllerDelegate

- (void)editSingleValueDidEndWithResponse:(NSModalResponse)response {
    if (response == NSModalResponseOK) {
        NSIndexSet *ind = self.tableView.selectedRowIndexes;
        NSInteger idx = ind.firstIndex;
        if (![self.store[idx].values[0] isEqualToString:(self.editSingleValueWindowController.fieldValue).string]) {
            self.store[idx].values[0] = (self.editSingleValueWindowController.fieldValue).string;
            [self.tableView reloadData];
            self.isModified = YES;
        }
    }

    [NSApp endSheet:self.editSingleValueWindowController.window];
}

@end
