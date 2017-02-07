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

enum {
    LV_ROOT,
    LV_LIST_ITEMS,
};

@implementation MediaLibraryItem {
    MediaLibraryItem *parent;
    NSString *_stringValue;

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

- (id)initItemNode:(ddb_medialib_item_t *)item parent:(MediaLibraryItem *)parent{
    self = [self init];
    _item = item;
    return self;
}

- (id)initTrackNode:(DB_playItem_t *)track parent:(MediaLibraryItem *)parent {
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
                _children[i] = [[MediaLibraryItem alloc] initItemNode:&_list->items[i] parent:self];
            }
        }
        else if (_item) {
            _children = [[NSMutableArray alloc] initWithCapacity:_item->num_tracks];
            for (int i = 0; i < _item->num_tracks; i++) {
                _children[i] = [[MediaLibraryItem alloc] initTrackNode:_item->tracks[i] parent:self];
            }
        }
    }
    return _children;
}

- (NSString *)stringValue {
    if (_list) {
        _stringValue = @"All Music";
        return _stringValue;
    }
    else if (_item) {
        _stringValue = [NSString stringWithFormat:@"%@ (%d)", [NSString stringWithUTF8String:_item->text], _item->num_tracks];
        return _stringValue;
    }

    if (!title_script) {
        title_script = deadbeef->tf_compile ("[%artist% - ]%title%");
    }

    ddb_tf_context_t ctx;
    ctx._size = sizeof (ddb_tf_context_t);
    ctx.it = _track;
    char buf[1000];
    deadbeef->tf_eval (&ctx, title_script, buf, sizeof (buf));
    _stringValue = [NSString stringWithUTF8String:buf];
    return _stringValue;

}

@end
