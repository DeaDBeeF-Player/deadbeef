//
//  MediaLibraryManager.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/29/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryManager.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryManager()

@property (nonatomic) DB_mediasource_t *medialibPlugin;
@property (nonatomic,readwrite) ddb_mediasource_source_t source;

@end

@implementation MediaLibraryManager

- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }

    _medialibPlugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    _source = self.medialibPlugin->create_source ("deadbeef");
    self.medialibPlugin->refresh(_source);

    return self;
}

- (void)dealloc
{
    if (_source) {
        _medialibPlugin->free_source(_source);
        _source = NULL;
    }
    _medialibPlugin = NULL;
}

@end
