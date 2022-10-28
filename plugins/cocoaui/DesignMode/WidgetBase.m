//
//  WidgetBase.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "WidgetBase.h"
#import "WidgetTopLevelView.h"

@interface WidgetBase() <WidgetTopLevelViewDelegate>

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@property (nonatomic,readwrite) WidgetTopLevelView *topLevelView;
@property (nullable,nonatomic,readwrite) NSMutableArray<id<WidgetProtocol>> *childWidgets;

@end

@implementation WidgetBase

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _deps = deps;

    _childWidgets = [NSMutableArray new];
    _topLevelView = [[WidgetTopLevelView alloc] initWithDeps:deps];
    _topLevelView.translatesAutoresizingMaskIntoConstraints = NO;
    _topLevelView.delegate = self;

    return self;
}

+ (NSString *)widgetType {
    return @"base";
}

- (NSString *)widgetType {
    return self.class.widgetType;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    for (id<WidgetProtocol> widget in self.childWidgets) {
        [widget message:_id ctx:ctx p1:p1 p2:p2];
    }
}

- (NSView *)view {
    return self.topLevelView;
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    return YES;
}

- (void)configure {
    for (id<WidgetProtocol> widget in self.childWidgets) {
        [widget configure];
    }
}

- (NSDictionary *)serializedSettingsDictionary {
    return nil;
}

- (BOOL)isPlaceholder {
    return NO;
}

- (NSMenu *)menu {
    return [self.deps.menuBuilder menuForWidget:self includeParentMenu:YES];
}

- (void)appendChild:(id<WidgetProtocol>)child {
    [self.childWidgets addObject:child];
    child.parentWidget = self;
    [self configure];
}

- (void)removeChild:(id<WidgetProtocol>)child {
    [self.childWidgets removeObject:child];
    child.parentWidget = nil;
}

- (void)insertChild:(id<WidgetProtocol>)child atIndex:(NSInteger)position {
    [self.childWidgets insertObject:child atIndex:position];
    child.parentWidget = self;
    [self configure];
}

- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild {
    [self removeChild:child];
    [self appendChild:newChild];
    [self configure];
}

@end
