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

- (void)setPreset:(NSString *)preset {
    deadbeef->conf_set_str("medialib.preset", preset.UTF8String);
    deadbeef->sendmessage(DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (NSString *)preset {
    char buffer[100];
    deadbeef->conf_get_str("medialib.preset", "", buffer, sizeof(buffer));
    return @(buffer);
}

@end
