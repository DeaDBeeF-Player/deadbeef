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

@interface WidgetRegistration : NSObject

@property (nonatomic) NSString *type;
@property (nonatomic) NSString *displayName;
@property (nonatomic) WidgetInstantiatorBlockType instantiatorBlock;

@end

@implementation WidgetRegistration
@end

#pragma mark -

@interface WidgetFactory()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property NSMutableDictionary<NSString *,WidgetRegistration *> *registeredWidgets;

@end

@implementation WidgetFactory

+ (WidgetFactory *)sharedInstance {
    static WidgetFactory *instance;

    if (instance == nil) {
        instance = [[WidgetFactory alloc] initWithDeps:DesignModeDeps.sharedInstance];
    }

    return instance;
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
    [self registerType:PlaceholderWidget.widgetType displayName:@"Placeholder" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaceholderWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:PlaylistWidget.widgetType displayName:@"Playlist" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaylistWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:VSplitterWidget.widgetType displayName:@"Splitter (top and bottom)" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[VSplitterWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:HSplitterWidget.widgetType displayName:@"Splitter (left and right)" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[HSplitterWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:SpectrumAnalyzerWidget.widgetType displayName:@"Spectrum Analyzer" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[SpectrumAnalyzerWidget alloc] initWithDeps:self.deps];
    }];
}

- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type {
    WidgetRegistration *widgetRegistration = self.registeredWidgets[type];
    if (widgetRegistration == nil) {
        return nil;
    }
    return widgetRegistration.instantiatorBlock();
}

- (void)registerType:(NSString *)type displayName:(NSString *)displayName instantiatorBlock:(WidgetInstantiatorBlockType)instantiatorBlock {
    WidgetRegistration *widgetRegistration = [WidgetRegistration new];
    widgetRegistration.type = type;
    widgetRegistration.displayName = displayName;
    widgetRegistration.instantiatorBlock = instantiatorBlock;
    self.registeredWidgets[type] = widgetRegistration;
}

- (void)unregisterType:(NSString *)type {
    [self.registeredWidgets removeObjectForKey:type];
}

- (NSArray<NSString *> *)types {
    return [self.registeredWidgets.allKeys sortedArrayUsingSelector:@selector(isEqualToString:)];
}

- (NSString *)displayNameForType:(NSString *)type {
    WidgetRegistration *widgetRegistration = self.registeredWidgets[type];
    return widgetRegistration.displayName;
}

@end
