//
//  WidgetSerializer.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 22/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "WidgetSerializer.h"
#import "DesignModeDeps.h"

@interface WidgetSerializer()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@end

@implementation WidgetSerializer

+ (WidgetSerializer *)sharedInstance {
    static WidgetSerializer *instance;

    if (instance == nil) {
        instance = [[WidgetSerializer alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }

    return instance;
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _deps = deps;

    return self;
}

- (NSDictionary *)saveWidgetToDictionary:(id<WidgetProtocol>)widget {
    NSMutableDictionary *widgetDictionary = [NSMutableDictionary new];
    widgetDictionary[@"type"] = widget.widgetType;
    NSDictionary *settings = widget.serializedSettingsDictionary;
    if (settings != nil) {
        widgetDictionary[@"settings"] = widget.serializedSettingsDictionary;
    }

    NSMutableArray<NSDictionary *> *serializedChildren = [NSMutableArray new];

    for (id<WidgetProtocol> childWidget in widget.childWidgets) {
        [serializedChildren addObject:[self saveWidgetToDictionary:childWidget]];
    }

    if (serializedChildren.count) {
        widgetDictionary[@"children"] = serializedChildren.copy;
    }

    return widgetDictionary;
}

- (BOOL)loadWidget:(id<WidgetProtocol>)widget fromDictionary:(NSDictionary *)dictionary {
    NSDictionary *settings = dictionary[@"settings"];

    BOOL loaded = NO;
    if ([settings isKindOfClass:NSDictionary.class]) {
        loaded = [widget deserializeFromSettingsDictionary:settings];
    }

    if (!loaded) {
        return NO;
    }

    NSArray *children = dictionary[@"children"];
    if (children == nil) {
        return YES;
    }

    for (id child in children) {
        if (![child isKindOfClass:NSDictionary.class]) {
            continue;
        }
        NSDictionary *childDictionary = child;
        id<WidgetProtocol> childWidget = [self loadFromDictionary:childDictionary];
        if (childWidget != nil) {
            [widget appendChild:childWidget];
        }
    }
    return YES;
}

- (id<WidgetProtocol>)loadFromDictionary:(NSDictionary *)dictionary {
    NSString *type = dictionary[@"type"];

    id<WidgetProtocol> widget;
    if (type == nil) {
        return nil;
    }
    else {
        widget = [self.deps.factory createWidgetWithType:type];
    }

    [self loadWidget:widget fromDictionary:dictionary];

    return widget;
}

@end
