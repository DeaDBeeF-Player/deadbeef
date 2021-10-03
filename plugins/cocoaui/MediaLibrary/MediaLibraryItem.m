//
//  MediaLibraryItem.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 2/5/17.
//  Copyright © 2017 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryItem.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryItem() {
    NSString *_stringValue;

    ddb_medialib_item_t *_item;
    NSMutableArray *_children;
}
@end

@implementation MediaLibraryItem

- (instancetype)init {
    static ddb_medialib_item_t item;
    return [self initWithItem:&item];
}

- (id)initWithItem:(ddb_medialib_item_t *)item {
    _item = item;
    return self;
}

- (NSUInteger)numberOfChildren {
    if (_item == NULL) {
        return 0;
    }
    return _item->num_children;
}

- (MediaLibraryItem *)childAtIndex:(NSUInteger)index {
    return [self.children objectAtIndex:index];
}

- (NSArray *)children {
    if (!_children && _item->num_children > 0) {
        _children = [[NSMutableArray alloc] initWithCapacity:_item->num_children];
        ddb_medialib_item_t *c = _item->children;
        for (int i = 0; i < _item->num_children; i++) {
            _children[i] = [[MediaLibraryItem alloc] initWithItem:c];
            c = c->next;
        }

        [_children sortUsingComparator:^NSComparisonResult(MediaLibraryItem  * _Nonnull obj1, MediaLibraryItem * _Nonnull obj2) {
            if (!obj1.playItem || !obj2.playItem) {
                return [obj1.stringValue caseInsensitiveCompare:obj2.stringValue];
            }

            int n1 = atoi (deadbeef->pl_find_meta (obj1.playItem, "track") ?: "0");
            int n2 = atoi (deadbeef->pl_find_meta (obj2.playItem, "track") ?: "0");
            int d1 = atoi (deadbeef->pl_find_meta (obj1.playItem, "disc") ?: "0") + 1;
            int d2 = atoi (deadbeef->pl_find_meta (obj2.playItem, "disc") ?: "0") + 1;
            n1 = d1 * 10000 + n1;
            n2 = d2 * 10000 + n2;
            if (n1 == n2) {
                return NSOrderedSame;
            }
            else if (n1 > n2) {
                return NSOrderedDescending;
            }
            else {
                return NSOrderedAscending;
            }
        }];
    }
    return _children;
}

- (NSString *)stringValue {
    if (!_item) {
        return @"";
    }
    if (!_stringValue) {
        if (_item->num_children) {
            _stringValue = [NSString stringWithFormat:@"%@ (%d)", [NSString stringWithUTF8String:_item->text], _item->num_children];
        }
        else {
            _stringValue = [NSString stringWithFormat:@"%@", [NSString stringWithUTF8String:_item->text]];
        }
    }
    return _stringValue;
}

- (ddb_playItem_t *)playItem {
    return _item->track;
}

- (ddb_medialib_item_t *)medialibItem {
    return _item;
}

@end
