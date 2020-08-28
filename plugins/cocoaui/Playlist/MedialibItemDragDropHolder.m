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

@property (nonatomic) ddb_playItem_t *playItem;

@end


@implementation MedialibItemDragDropHolder

- (instancetype)init {
    ddb_playItem_t item;
    return [self initWithItem:&item];
}

- (instancetype)initWithItem:(ddb_playItem_t *)item {
    self = [super init];
    if (!self) {
        return nil;
    }
    _playItem = item;
    deadbeef->pl_item_ref (_playItem);
    return self;
}

// NSPasteboardReading
+ (nonnull NSArray<NSPasteboardType> *)readableTypesForPasteboard:(nonnull NSPasteboard *)pasteboard {
    return [NSArray arrayWithObjects:ddbMedialibItemUTIType, nil];
}

+ (NSPasteboardReadingOptions)readingOptionsForType:(NSPasteboardType)type pasteboard:(NSPasteboard *)pasteboard {
    // FIXME: this is a low effort impl, which may fail. Proper implementation needs to serialize into plist.
    return NSPasteboardReadingAsData;
}

// NSPasteboardWriting
- (NSArray<NSString *> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard {
    return [NSArray arrayWithObjects:ddbMedialibItemUTIType, nil];
}

- (id)pasteboardPropertyListForType:(NSString *)type {
    if ([type isEqualToString:ddbMedialibItemUTIType]) {
        NSData *data = [NSData dataWithBytes:&_playItem length:sizeof (_playItem)];
        return data;
    }

    return nil;
}


@end
