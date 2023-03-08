//
//  DesignModeState.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 21/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "DesignModeDeps.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

static DesignModeState *_sharedInstance;

@interface DesignModeState()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic,readwrite) id<WidgetProtocol> rootWidget;
@property (nonatomic) BOOL isLoading;
@property (nonatomic,copy) NSDictionary *previousLayout;

@end

@implementation DesignModeState

+ (DesignModeState *)sharedInstance {
    if (_sharedInstance == nil) {
        _sharedInstance = [[DesignModeState alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }

    return _sharedInstance;
}

+ (void)freeSharedInstance {
    _sharedInstance = nil;
}

- (void)cleanupWidget:(id<WidgetProtocol>)widget {
    for (id<WidgetProtocol> childWidget in widget.childWidgets) {
        [self cleanupWidget:childWidget];
    }
    if ([widget respondsToSelector:@selector(cleanup)]) {
        [widget cleanup];
    }
}

- (void)dealloc
{
    [self cleanupWidget:_rootWidget];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _deps = deps;
    return self;
}

- (void)load {
    self.isLoading = YES;
    self.rootWidget = [self.deps.factory createWidgetWithType:@"Placeholder"];

    char *layout = malloc (100000);
    deadbeef->conf_get_str ("cocoaui.layout", "", layout, 100000);
    NSString *strLayout = [NSString stringWithUTF8String:layout];
    free (layout);

    NSData *data = [strLayout dataUsingEncoding:NSUTF8StringEncoding];

    NSError *err = nil;
    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

    id<WidgetProtocol> layoutWidget;
    if (json && [json isKindOfClass:NSDictionary.class]) {
        layoutWidget = [self.deps.serializer loadFromDictionary:json];
    }

    if (!layoutWidget) {
        layoutWidget = [self.deps.serializer loadFromDictionary:@{
            @"type":@"PlaylistWithTabs"
        }];
    }


    if (!layoutWidget) {
        // bummer...
    }

    [self.rootWidget appendChild:layoutWidget];

    self.previousLayout = json;

    self.isLoading = NO;
}

- (void)layoutDidChange {
    if (self.isLoading) {
        return;
    }
    NSDictionary *dict = [self.deps.serializer saveWidgetToDictionary:self.rootWidget.childWidgets.firstObject];

    if ([dict isEqual:self.previousLayout]) {
        return;
    }

    self.previousLayout = dict;

    NSError *err;
    NSData *dt = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&err];
    NSString *json = [[NSString alloc] initWithData:dt encoding:NSUTF8StringEncoding];

    deadbeef->conf_set_str ("cocoaui.layout", json.UTF8String);
    deadbeef->conf_save ();
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
