//
//  WidgetFactory.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "PlaylistWidget.h"
#import "PlaceholderWidget.h"
#import "SpectrumAnalyzerWidget.h"
#import "SplitterWidget.h"
#import "DesignModeDeps.h"

@interface WidgetFactory()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property NSMutableDictionary<NSString *,WidgetInstantiatorBlockType> *registeredWidgets;

@end

@implementation WidgetFactory

+ (WidgetFactory *)sharedInstance {
    static WidgetFactory *instance;

    if (instance == nil) {
        instance = [[WidgetFactory alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }

    return instance;
}

- (instancetype)init {
    return [self initWithDeps:nil];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    _deps = deps;
    _registeredWidgets = [NSMutableDictionary new];

    [self registerAllTypes];

    return self;
}


- (void)registerAllTypes {
    [self registerType:@"Placeholder" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaceholderWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:@"Playlist" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaylistWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:@"Splitter (top and bottom)" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[SplitterWidget alloc] initWithDeps:self.deps vertical:NO];
    }];
    [self registerType:@"Splitter (left and right)" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[SplitterWidget alloc] initWithDeps:self.deps vertical:YES];
    }];
    [self registerType:@"Spectrum Analyzer" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[SpectrumAnalyzerWidget alloc] initWithDeps:self.deps];
    }];
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
