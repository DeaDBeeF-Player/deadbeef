//
//  WidgetFactory.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "WidgetFactory.h"
#import "PlaylistWidget.h"
#import "PlaceholderWidget.h"
#import "SplitterWidget.h"

@interface WidgetFactory()

@property NSMutableDictionary<NSString *,WidgetInstantiatorBlockType> *registeredWidgets;

@end

@implementation WidgetFactory

+ (void)initialize
{
    if (self == [WidgetFactory class]) {
        [WidgetFactory.sharedFactory registerType:@"Placeholder" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [PlaceholderWidget new];
        }];
        [WidgetFactory.sharedFactory registerType:@"Playlist" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [PlaylistWidget new];
        }];
        [WidgetFactory.sharedFactory registerType:@"Splitter" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
            return [SplitterWidget new];
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

@end
