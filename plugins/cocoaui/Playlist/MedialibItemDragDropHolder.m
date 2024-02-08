//
//  MedialibItemDragDropHolder.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "MedialibItemDragDropHolder.h"
#import "DdbShared.h"

extern DB_functions_t *deadbeef;

@interface MedialibItemDragDropHolder()

@property (nonatomic,readwrite) ddb_playlist_t *plt;
@end

@implementation MedialibItemDragDropHolder

// NSSecureCoding
@dynamic supportsSecureCoding;
+ (BOOL) supportsSecureCoding {
    return YES;
}

- (instancetype)init {
    return [self initWithItem:nil];
}

- (instancetype)initWithItem:(ddb_playItem_t *)item {
    if (item == NULL) {
        return [self initWithItems:NULL count:0];
    }
    return [self initWithItems:&item count:1];
}

- (instancetype)initWithItems:(ddb_playItem_t * _Nonnull *)items count:(NSInteger)count {
    self = [super init];
    if (!self) {
        return nil;
    }


    if (count == 0) {
        return self;
    }

    ddb_playlist_t *plt = deadbeef->plt_alloc ("clipboard");
    ddb_playItem_t *after = NULL;
    for (NSInteger i = 0; i < count; i++) {
        deadbeef->plt_insert_item (plt, after, items[i]);
        after = items[i];
    }

    self.plt = plt;

    return self;
}

- (void)dealloc
{
    if (self.plt != NULL) {
        deadbeef->plt_unref (self.plt);
    }
}
// NSCoding

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
    self = [self initWithItem:nil];
    if (!self) {
        return nil;
    }

    NSData *data = [aDecoder decodeObjectOfClass:NSArray.class forKey:@"DdbPlaylistData"];
    if (data == nil) {
        return self;
    }

    // FIXME
//    ddb_playlist_t *plt = deadbeef->plt_alloc("clipboard");
//    int res = deadbeef->plt_load_from_buffer (plt, data.bytes, data.length);
//    if (res == 0) {
//        self.plt = plt;
//    }
//    else {
//        deadbeef->plt_unref (plt);
//    }

    return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
    if (self.plt == NULL) {
        return;
    }

    uint8_t *buffer = NULL;
    ssize_t size = deadbeef->plt_save_to_buffer (self.plt, &buffer);

    if (size == 0) {
        return;
    }
    NSData *data = [NSData dataWithBytes:buffer length:size];
    free (buffer);

    [aCoder encodeObject:data forKey:@"DdbPlaylistData"];
}



// NSPasteboardReading
+ (nonnull NSArray<NSPasteboardType> *)readableTypesForPasteboard:(nonnull NSPasteboard *)pasteboard {
    return @[ddbMedialibItemUTIType];
}

+ (NSPasteboardReadingOptions)readingOptionsForType:(NSPasteboardType)type pasteboard:(NSPasteboard *)pasteboard {
    return NSPasteboardReadingAsKeyedArchive;
}

// NSPasteboardWriting
- (NSArray<NSString *> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard {
    return @[ddbMedialibItemUTIType];
}

- (id)pasteboardPropertyListForType:(NSString *)type {
    if( [type isEqualToString:ddbMedialibItemUTIType] ) {
#ifdef MAC_OS_X_VERSION_10_14
        if (@available(macOS 10.13, *)) {
            return [NSKeyedArchiver archivedDataWithRootObject:self requiringSecureCoding:NO error:nil];
        }
        else
#endif
        {
            return [NSKeyedArchiver archivedDataWithRootObject:self];
        }
    }

    return nil;
}


@end
