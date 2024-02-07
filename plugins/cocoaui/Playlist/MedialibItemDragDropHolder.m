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

@property (nonatomic,readwrite) ddb_playItem_t **items;
@property (nonatomic,readwrite) NSInteger count;

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

    if (count > 0) {
        _count = count;
        _items = calloc(count, sizeof (ddb_playItem_t *));
        for (NSInteger i = 0; i < _count; i++) {
            ddb_playItem_t *item = items[i];
            deadbeef->pl_item_ref (item);
            _items[i] = item;
        }
    }

    return self;
}

- (void)dealloc {
    for (NSInteger i = 0; i < _count; i++) {
        deadbeef->pl_item_unref (_items[i]);
    }
    free (_items);
    _items = NULL;
}

// NSCoding

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
    self = [self initWithItem:nil];
    if (!self) {
        return nil;
    }

    NSData *data = [aDecoder decodeObjectOfClass:[NSArray class] forKey:@"PlayItemListPtr"];
    const void *bytes = data.bytes;

    memcpy (&_count, bytes, sizeof (NSInteger));
    if (_count > 0) {
        _items = calloc(_count, sizeof (ddb_playItem_t *));
        ddb_playItem_t **items = (ddb_playItem_t **)(bytes + sizeof (NSInteger));
        for (NSInteger i = 0; i < _count; i++) {
            ddb_playItem_t *item = items[i];
            deadbeef->pl_item_ref (item);
            _items[i] = item;
        }

    }

    return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
    for (NSInteger i = 0; i < _count; i++) {
        deadbeef->pl_item_ref (_items[i]);
    }

    NSUInteger length = sizeof (NSInteger) + self.count * sizeof (ddb_playItem_t *);
    void *bytes = malloc (length);
    memcpy (bytes, &_count, sizeof (NSInteger));
    if (self.count > 0) {
        memcpy (bytes + sizeof (NSInteger), _items, self.count * sizeof (ddb_playItem_t *));
    }

    // FIXME: this is a low effort impl, which may fail. Proper implementation needs to serialize into plist.
    NSData *data = [NSData dataWithBytes:bytes length:length];
    [aCoder encodeObject:data forKey:@"PlayItemListPtr"];
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
