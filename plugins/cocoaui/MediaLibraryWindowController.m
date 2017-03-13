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

@interface MediaLibraryWindowController () {
    ddb_medialib_plugin_t *_medialib;
    ddb_medialib_item_t *_tree;
    MediaLibraryItem *_root;
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
    _root = nil;
    if (_tree) {
        _medialib->free_list (_tree);
    }
    _tree = _medialib->get_list ("genre");
    _root = [MediaLibraryItem initTree:_tree];
    [_outlineView reloadData];
}

- (void)windowDidLoad {
    [super windowDidLoad];

    _medialib = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    _medialib->add_listener (_medialib_listener, (__bridge void *)self);

    [self medialibEvent:DDB_MEDIALIB_EVENT_CHANGED];

    [_outlineView setDataSource:(id<NSOutlineViewDataSource> _Nullable)self];
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {
    return (item == nil) ? 1 : [item numberOfChildren];
}


- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    return [item numberOfChildren] > 0;
}


- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {
    return (item == nil) ? _root : [item childAtIndex:index];
}


- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
//    NSLog (@"%@", [item stringValue]);
    return (item == nil) ? @"/" : [item stringValue];
}

@end
