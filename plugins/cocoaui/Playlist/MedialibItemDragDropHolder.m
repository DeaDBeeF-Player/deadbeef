//
//  MedialibItemDragDropHolder.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/28/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MedialibItemDragDropHolder.h"
#import "DdbShared.h"

extern DB_functions_t *deadbeef;

@interface MedialibItemDragDropHolder()

@property (nullable,nonatomic,readwrite) ddb_playItem_t *playItem;

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
    self = [super init];
    if (!self) {
        return nil;
    }
    _playItem = item;
    if (_playItem) {
        deadbeef->pl_item_ref (_playItem);
    }
    return self;
}

- (void)dealloc {
    if (_playItem) {
        deadbeef->pl_item_unref (_playItem);
    }
}

// NSCoding

- (instancetype)initWithCoder:(NSCoder *)aDecoder {
    self = [self initWithItem:nil];
    if (!self) {
        return nil;
    }

    NSData *data = [aDecoder decodeObjectOfClass:[NSArray class] forKey:@"PlayItemPtr"];
    memcpy (&_playItem, data.bytes, sizeof (_playItem));
    if (_playItem) {
        deadbeef->pl_item_ref (_playItem);
    }

    return self;
}

- (void)encodeWithCoder:(NSCoder *)aCoder {
    // FIXME: this is a low effort impl, which may fail. Proper implementation needs to serialize into plist.
    NSData *data = [NSData dataWithBytes:&_playItem length:sizeof (_playItem)];
    [aCoder encodeObject:data forKey:@"PlayItemPtr"];
}



// NSPasteboardReading
+ (nonnull NSArray<NSPasteboardType> *)readableTypesForPasteboard:(nonnull NSPasteboard *)pasteboard {
    return [NSArray arrayWithObjects:ddbMedialibItemUTIType, nil];
}

+ (NSPasteboardReadingOptions)readingOptionsForType:(NSPasteboardType)type pasteboard:(NSPasteboard *)pasteboard {
    return NSPasteboardReadingAsKeyedArchive;
}

// NSPasteboardWriting
- (NSArray<NSString *> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard {
    return [NSArray arrayWithObjects:ddbMedialibItemUTIType, nil];
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
