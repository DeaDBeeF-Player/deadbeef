//
//  PlaylistLocalDragDropHolder.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/1/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "PlaylistLocalDragDropHolder.h"
#import "DdbShared.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

// data has to be serialized, so we code idx and not pointers
@interface PlaylistLocalDragDropHolder()
@end

@implementation PlaylistLocalDragDropHolder

// NSSecureCoding
@dynamic supportsSecureCoding;
+ (BOOL) supportsSecureCoding {
    return YES;
}

// NSCoding

- (instancetype)initWithCoder:(NSCoder *)aDecoder {

    self = [super init];
    if (self) {
        _playlistIdx = [aDecoder decodeIntegerForKey:@"Playlist"];
        _itemsIndices = [aDecoder decodeObjectOfClass:[NSMutableArray class] forKey:@"Items"];
    }

    return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {

    [aCoder encodeInteger:_playlistIdx forKey:@"Playlist"];
    [aCoder encodeObject:_itemsIndices forKey:@"Items"];
}


// NSPasteboardReading

+ (NSArray<NSString *> *)readableTypesForPasteboard:(NSPasteboard *)pasteboard {
    return [NSArray arrayWithObjects:ddbPlaylistItemsUTIType, nil];
}

+ (NSPasteboardReadingOptions)readingOptionsForType:(NSString *)type pasteboard:(NSPasteboard *)pasteboard {

    return NSPasteboardReadingAsKeyedArchive;
}

// NSPasteboardWriting

- (NSArray<NSString *> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard {

    return [NSArray arrayWithObjects:ddbPlaylistItemsUTIType, nil];
}

- (id)pasteboardPropertyListForType:(NSString *)type {

    if( [type isEqualToString:ddbPlaylistItemsUTIType] ) {
        return [NSKeyedArchiver archivedDataWithRootObject:self];
    }

    return nil;
}

- (int) count {
    return (int)[_itemsIndices count];
}

- (NSInteger) playlistIdx {

    return (int)_playlistIdx;
}

- (instancetype)initWithSelectedItemsOfPlaylist:(ddb_playlist_t *)playlist {
    self = [super init];
    if (!self) {
        return nil;
    }
    deadbeef->pl_lock ();
    _playlistIdx = deadbeef->plt_get_idx (playlist);

    int count = deadbeef->plt_getselcount (playlist);
    NSMutableArray *indices = [NSMutableArray arrayWithCapacity:(NSUInteger)count];
    if (count) {

        DB_playItem_t *it = deadbeef->plt_get_first (playlist, PL_MAIN);
        while (it) {
            if (deadbeef->pl_is_selected (it)) {
                [indices addObject: [NSNumber numberWithInt: deadbeef->plt_get_item_idx(playlist, it, PL_MAIN)]];
            }
            DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
            deadbeef->pl_item_unref (it);
            it = next;
        }
    }
    _itemsIndices = (NSArray*) indices;
    deadbeef->pl_unlock ();
    return self;
}

@end

