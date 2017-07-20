//
//  MediaLibraryWindowController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/4/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryWindowController.h"
#import "MediaLibraryItem.h"
#include "../../deadbeef.h"
#include "../medialib/medialib.h"

extern DB_functions_t *deadbeef;

enum {
    QUERY_GENRES,
    QUERY_ALBUMS,
    QUERY_ARTISTS,
    QUERY_FOLDERS,
};

static const char *indexNames[] = {
    "genre",
    "album",
    "artist",
    "folder",
};

@interface MediaLibraryWindowController () {
    ddb_medialib_plugin_t *_medialib;
    ddb_medialib_item_t *_tree;
    MediaLibraryItem *_root;
    int _index;
}
@end

@implementation MediaLibraryWindowController

static void _medialib_listener (int event, void *user_data) {
    MediaLibraryWindowController *ctl = (__bridge MediaLibraryWindowController *)user_data;
    dispatch_async(dispatch_get_main_queue(), ^{
        [ctl medialibEvent:event];
    });
}

- (void)medialibEvent:(int)event {
    [self initializeTreeView:_index];
}

- (void)windowDidLoad {
    [super windowDidLoad];

    _medialib = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    _medialib->add_listener (_medialib_listener, (__bridge void *)self);

    [self medialibEvent:DDB_MEDIALIB_EVENT_CHANGED];

    [_outlineView setDataSource:(id<NSOutlineViewDataSource> _Nullable)self];
}

- (void)initializeTreeView:(int)index {
    _index = index;
    _root = nil;
    if (_tree) {
        _medialib->free_list (_tree);
    }
    _tree = _medialib->get_list (indexNames[_index]);
    _root = [MediaLibraryItem initTree:_tree];
    [_outlineView reloadData];
}

- (IBAction)queryChanged:(id)sender {
    NSPopUpButton *btn = sender;
    NSInteger selected = [btn indexOfSelectedItem];
    [self initializeTreeView:(int)selected];
}

// tree view data source
- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    if (item == nil) {
        item = _root;
    }
    return [item numberOfChildren];
}


- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    if (item == nil) {
        item = _root;
    }
    return [item numberOfChildren] > 0;
}


- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    if (item == nil) {
        item = _root;
    }
    return [item childAtIndex:index];
}


- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
    return (item == nil) ? @"/" : [item stringValue];
}

@end
