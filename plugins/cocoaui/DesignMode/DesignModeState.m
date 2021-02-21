//
//  DesignModeState.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "WidgetFactory.h"
#import "WidgetMenuBuilder.h"

@interface DesignModeState()

@property (nonatomic) id<WidgetFactoryProtocol> widgetFactory;
@property (nonatomic) id<WidgetMenuBuilderProtocol> menuBuilder;

@end

@implementation DesignModeState

+ (DesignModeState *)sharedInstance {
    static DesignModeState *instance;

    if (instance == nil) {
        instance = [[DesignModeState alloc] initWithWidgetFactory:WidgetFactory.sharedFactory menuBuilder:WidgetMenuBuilder.sharedInstance];
    }

    return instance;
}

- (instancetype)initWithWidgetFactory:(nullable id<WidgetFactoryProtocol>)widgetFactory menuBuilder:(nullable id<WidgetMenuBuilderProtocol>)menuBuilder {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _widgetFactory = widgetFactory;
    _menuBuilder = menuBuilder;

    return self;
}

@end
