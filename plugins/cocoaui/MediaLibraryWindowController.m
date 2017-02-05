//
//  MediaLibraryWindowController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/4/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryWindowController.h"
#include "../../deadbeef.h"
#include "../medialib/medialib.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryWindowController () {
    ddb_medialib_plugin_t *_medialib;
    NSString *_rootItem;
}

@end

@implementation MediaLibraryWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    _medialib = (ddb_medialib_plugin_t *)deadbeef->plug_get_for_id ("medialib");
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
//    [_outlineView setDelegate:(id<NSOutlineViewDelegate> _Nullable)self];

    _rootItem = @"All music";
    [_outlineView setDataSource:(id<NSOutlineViewDataSource> _Nullable)self];
}

- (NSInteger)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item {

    return (item == nil) ? 1 : 0;//[item numberOfChildren];
}


- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item {
    return (item == nil) ? YES : NO;//([item numberOfChildren] != -1);
}


- (id)outlineView:(NSOutlineView *)outlineView child:(NSInteger)index ofItem:(id)item {

    return (item == nil) ? _rootItem : nil;//[(FileSystemItem *)item childAtIndex:index];
}


- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item {
    return (item == nil) ? @"/" : item;
}

@end
