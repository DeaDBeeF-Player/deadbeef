//
//  WidgetFactory.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "PlaylistWidget.h"
#import "PlaceholderWidget.h"
#import "SplitterWidget.h"
#import "WidgetFactory.h"
#import "WidgetMenuBuilder.h"

@interface WidgetFactory()

@property NSMutableDictionary<NSString *,WidgetInstantiatorBlockType> *registeredWidgets;

@end

@implementation WidgetFactory

+ (void)initialize
{
    if (self == [WidgetFactory class]) {
        [WidgetFactory.sharedFactory registerType:@"Placeholder" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [[PlaceholderWidget alloc] initWithDesignModeState:DesignModeState.sharedInstance];
        }];
        [WidgetFactory.sharedFactory registerType:@"Playlist" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [[PlaylistWidget alloc] initWithDesignModeState:DesignModeState.sharedInstance];
        }];
        [WidgetFactory.sharedFactory registerType:@"Splitter (top and bottom)" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [[SplitterWidget alloc] initWithDesignModeState:DesignModeState.sharedInstance vertical:NO];
        }];
        [WidgetFactory.sharedFactory registerType:@"Splitter (left and right)" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [[SplitterWidget alloc] initWithDesignModeState:DesignModeState.sharedInstance vertical:YES];
        }];
    }
}

+ (WidgetFactory *)sharedFactory {
    static WidgetFactory *instance;

    if (instance == nil) {
        instance = [WidgetFactory new];
    }

    return instance;
}

- (instancetype)init
{
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _registeredWidgets = [NSMutableDictionary new];

    return self;
}

- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type {
    WidgetInstantiatorBlockType block = self.registeredWidgets[type];
    if (block == nil) {
        return nil;
    }

    return block();
}

- (void)registerType:(NSString *)type instantiatorBlock:(WidgetInstantiatorBlockType)instantiatorBlock {
    self.registeredWidgets[type] = instantiatorBlock;
}

- (void)unregisterType:(NSString *)type {
    [self.registeredWidgets removeObjectForKey:type];
}

- (NSArray<NSString *> *)types {
    return [self.registeredWidgets.allKeys sortedArrayUsingSelector:@selector(isEqualToString:)];
}

@end
