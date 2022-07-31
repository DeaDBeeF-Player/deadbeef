//
//  DesignModeDeps.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 22/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "DesignModeDeps.h"
#import "DesignModeState.h"
#import "WidgetFactory.h"
#import "WidgetMenuBuilder.h"
#import "WidgetSerializer.h"

static DesignModeDeps *instance;

@interface DesignModeDeps() {
    id<DesignModeStateProtocol> _state;
    id<WidgetFactoryProtocol> _factory;
    id<WidgetSerializerProtocol> _serializer;
    id<WidgetMenuBuilderProtocol> _menuBuilder;
}

@end

@implementation DesignModeDeps

+ (DesignModeDeps *)sharedInstance {
    if (instance == nil) {
        instance = [DesignModeDeps new];
    }

    return instance;
}

+ (void)cleanup {
    instance = nil;
}

- (id<DesignModeStateProtocol>)state {
    if (_state == nil) {
        _state = DesignModeState.sharedInstance;
    }
    return _state;
}

- (id<WidgetFactoryProtocol>)factory {
    if (_factory == nil) {
        _factory = WidgetFactory.sharedInstance;
    }
    return _factory;
}

- (id<WidgetSerializerProtocol>)serializer {
    if (_serializer == nil) {
        _serializer = WidgetSerializer.sharedInstance;
    }
    return _serializer;
}

- (id<WidgetMenuBuilderProtocol>)menuBuilder {
    if (_menuBuilder == nil) {
        _menuBuilder = WidgetMenuBuilder.sharedInstance;
    }
    return _menuBuilder;
}



@end
