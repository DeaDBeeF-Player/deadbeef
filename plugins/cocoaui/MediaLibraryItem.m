//
//  MediaLibraryItem.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/5/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryItem.h"

extern DB_functions_t *deadbeef;

@implementation MediaLibraryItem {
    MediaLibraryItem *parent;
    NSString *_stringValue;

    ddb_medialib_item_t *_item;
    NSMutableArray *_children;
}

+ (id)initTree:(ddb_medialib_item_t *)tree {
    MediaLibraryItem *rootItem = [[MediaLibraryItem alloc] initRoot:tree];
    return rootItem;
}

- (id)initRoot:(ddb_medialib_item_t *)tree {
    self = [self init];
    _item = tree;
    return self;
}

- (id)initNode:(ddb_medialib_item_t *)item parent:(MediaLibraryItem *)parent{
    self = [self init];
    _item = item;
    return self;
}

- (NSUInteger)numberOfChildren {
    return _item->num_children;
}

- (MediaLibraryItem *)childAtIndex:(NSUInteger)index {
    return [[self children] objectAtIndex:index];
}

- (NSArray *)children {
    if (!_children && _item->num_children > 0) {
        _children = [[NSMutableArray alloc] initWithCapacity:_item->num_children];
        ddb_medialib_item_t *c = _item->children;
        for (int i = 0; i < _item->num_children; i++) {
            _children[i] = [[MediaLibraryItem alloc] initNode:c parent:self];
            c = c->next;
        }
    }
    return _children;
}

- (NSString *)stringValue {
    if (_item->num_children) {
        _stringValue = [NSString stringWithFormat:@"%@ (%d)", [NSString stringWithUTF8String:_item->text], _item->num_children];
    }
    else {
        _stringValue = [NSString stringWithFormat:@"%@", [NSString stringWithUTF8String:_item->text]];
    }
    return _stringValue;
}

@end
