//
//  DesignModeState.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "DesignModeDeps.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

@interface DesignModeState()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic,readwrite) id<WidgetProtocol> rootWidget;


@end

@implementation DesignModeState

+ (DesignModeState *)sharedInstance {
    static DesignModeState *instance;

    if (instance == nil) {
        instance = [[DesignModeState alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }

    return instance;
}

- (instancetype)init {
    return [self initWithDeps: nil];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _deps = deps;

    self.rootWidget = [deps.factory createWidgetWithType:@"Placeholder"];

    char *layout = malloc (100000);
    deadbeef->conf_get_str ("cocoaui.layout", "", layout, 100000);
    NSString *strLayout = [NSString stringWithUTF8String:layout];
    free (layout);

    NSData *data = [strLayout dataUsingEncoding:NSUTF8StringEncoding];

    NSError *err = nil;
    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&err];

    id<WidgetProtocol> layoutWidget;
    if (json && [json isKindOfClass:NSDictionary.class]) {
        layoutWidget = [deps.serializer loadFromDictionary:json];
    }

    if (!layoutWidget) {
        layoutWidget = [deps.serializer loadFromDictionary:@{
            @"type":@"Playlist"
        }];
    }


    if (!layoutWidget) {
        // bummer...
    }

    [self.rootWidget appendChild:layoutWidget];

    return self;
}


- (void)layoutDidChange {
    NSDictionary *dict = [self.deps.serializer saveWidgetToDictionary:self.rootWidget.childWidgets.firstObject];

    NSError *err;
    NSData *dt = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&err];
    NSString *json = [[NSString alloc] initWithData:dt encoding:NSUTF8StringEncoding];

    deadbeef->conf_set_str ("cocoaui.layout", json.UTF8String);
    deadbeef->conf_save ();
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
