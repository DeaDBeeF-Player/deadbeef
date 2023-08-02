//
//  MediaLibraryItem.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 2/5/17.
//  Copyright © 2017 Oleksiy Yakovenko. All rights reserved.
//

#import "MediaLibraryItem.h"

extern DB_functions_t *deadbeef;
static DB_mediasource_t *medialibPlugin;

@interface MediaLibraryItem() {
    NSString *_stringValue;

    const ddb_medialib_item_t *_item;
    NSMutableArray *_children;
}

@property (nonatomic,readonly) DB_mediasource_t *plugin;

@end

@implementation MediaLibraryItem

- (DB_mediasource_t *)plugin {
    if (medialibPlugin == nil) {
        medialibPlugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    }
    return medialibPlugin;
}

- (id)initWithItem:(const ddb_medialib_item_t *)item {
    _item = item;
    return self;
}

- (NSUInteger)numberOfChildren {
    if (_item == NULL) {
        return 0;
    }
    return self.plugin->tree_item_get_children_count(_item);
}

- (MediaLibraryItem *)childAtIndex:(NSUInteger)index {
    return (self.children)[index];
}

- (NSArray *)children {
    DB_mediasource_t *plugin = self.plugin;
    int count = plugin->tree_item_get_children_count(_item);
    if (!_children && count > 0) {
        _children = [[NSMutableArray alloc] initWithCapacity:count];
        const ddb_medialib_item_t *c = plugin->tree_item_get_children(_item);
        for (int i = 0; i < count; i++) {
            _children[i] = [[MediaLibraryItem alloc] initWithItem:c];
            c = plugin->tree_item_get_next(c);
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
        DB_mediasource_t *plugin = self.plugin;
        int count = plugin->tree_item_get_children_count(_item);
        const char *text = plugin->tree_item_get_text(_item);
        if (count) {
            _stringValue = [NSString stringWithFormat:@"%@ (%d)", @(text), count];
        }
        else {
            _stringValue = [NSString stringWithFormat:@"%@", @(text)];
        }
    }
    return _stringValue;
}

- (ddb_playItem_t *)playItem {
    return self.plugin->tree_item_get_track(_item);
}

- (const ddb_medialib_item_t *)medialibItem {
    return _item;
}

@end
