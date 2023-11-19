//
//  MediaLibraryManager.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 8/29/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "MediaLibraryManager.h"

extern DB_functions_t *deadbeef;

@interface MediaLibraryManager()

@property (nonatomic,readwrite) DB_mediasource_t *medialibPlugin;
@property (nonatomic,readwrite) ddb_mediasource_source_t *source;
@property (nonatomic,readwrite) scriptableModel_t *model;

@end

@implementation MediaLibraryManager

- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }

    _medialibPlugin = (DB_mediasource_t *)deadbeef->plug_get_for_id ("medialib");
    if (_medialibPlugin == nil) {
        return nil;
    }
    _source = self.medialibPlugin->create_source ("deadbeef");
    self.medialibPlugin->refresh(_source);

    self.model = scriptableModelInit(scriptableModelAlloc(), deadbeef, "medialib.preset");

    return self;
}

- (void)dealloc
{
    scriptableModelFree(_model);
    if (_source) {
        _medialibPlugin->free_source(_source);
        _source = NULL;
    }
    _medialibPlugin = NULL;
}

@end
