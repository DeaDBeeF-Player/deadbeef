//
//  MediaLibraryItem.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/5/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryItem.h"

extern DB_functions_t *deadbeef;

static char *title_script;
static char *fname_script;

@implementation MediaLibraryItem {
    ddb_medialib_list_t *_list;
    ddb_medialib_item_t *_item;
    DB_playItem_t *_track;
    NSMutableArray *_children;
}

+ (id)initTree:(ddb_medialib_list_t *)list {
    MediaLibraryItem *rootItem = [[MediaLibraryItem alloc] initRoot:list];
    return rootItem;
}

- (id)initRoot:(ddb_medialib_list_t *)list {
    self = [self init];
    _list = list;
    return self;
}

- (id)initNode:(ddb_medialib_item_t *)item {
    self = [self init];
    _item = item;
    return self;
}

- (id)initLeaf:(DB_playItem_t *)track {
    self = [self init];
    _track = track;
    return self;
}

- (NSUInteger)numberOfChildren {
    if (_list) {
        return _list->count;
    }
    else if (_item) {
        return _item->num_tracks;
    }
    return 0;
}

- (MediaLibraryItem *)childAtIndex:(NSUInteger)index {
    return [[self children] objectAtIndex:index];
}

- (NSArray *)children {
    if (!_children && (_list || _item)) {
        if (_list) {
            _children = [[NSMutableArray alloc] initWithCapacity:_list->count];
            for (int i = 0; i < _list->count; i++) {
                _children[i] = [[MediaLibraryItem alloc] initNode:&_list->items[i]];
            }
        }
        else if (_item) {
            _children = [[NSMutableArray alloc] initWithCapacity:_item->num_tracks];
            for (int i = 0; i < _item->num_tracks; i++) {
                _children[i] = [[MediaLibraryItem alloc] initLeaf:_item->tracks[i]];
            }
        }
    }
    return _children;
}

- (NSString *)stringValue {
    if (_list) {
        return @"All Music";
    }
    else if (_item) {
        return [NSString stringWithFormat:@"%@ (%d)", [NSString stringWithUTF8String:_item->text], _item->num_tracks];
    }

    if (!title_script) {
        title_script = deadbeef->tf_compile ("[%artist% - ]%title%");
    }

    ddb_tf_context_t ctx;
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = _track;
    char buf[1000];
    deadbeef->tf_eval (&ctx, title_script, buf, sizeof (buf));
    return [NSString stringWithUTF8String:buf];

}

@end
