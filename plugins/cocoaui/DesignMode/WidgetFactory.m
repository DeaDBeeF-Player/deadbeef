//
//  WidgetFactory.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "AlbumArtWidget.h"
#import "ChiptuneVoicesWidget.h"
#import "DesignModeDeps.h"
#import "HolderWidget.h"
#import "LyricsWidget.h"
#import "PlaylistBrowserWidget.h"
#import "PlaylistWidget.h"
#import "PlaylistWithTabsWidget.h"
#import "PlaceholderWidget.h"
#import "SpectrumAnalyzerWidget.h"
#import "ScopeWidget.h"
#import "SplitterWidget.h"
#import "TabsWidget.h"
#import "SelectionPropertiesWidget.h"

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

// FIXME: should be a separate class
- (void)registerAllTypes {
    [self registerType:PlaceholderWidget.widgetType displayName:@"Placeholder" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaceholderWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:PlaylistWidget.widgetType displayName:@"Playlist" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaylistWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:PlaylistWithTabsWidget.widgetType displayName:@"Playlist With Tabs" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaylistWithTabsWidget alloc] initWithDeps:self.deps];
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
    [self registerType:ScopeWidget.widgetType displayName:@"Scope" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[ScopeWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:AlbumArtWidget.widgetType displayName:@"Album Art" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[AlbumArtWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:TabsWidget.widgetType displayName:@"Tabs" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[TabsWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:PlaylistBrowserWidget.widgetType displayName:@"Playlist Browser" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[PlaylistBrowserWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:LyricsWidget.widgetType displayName:@"Lyrics" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[LyricsWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:ChiptuneVoicesWidget.widgetType displayName:@"Chiptune Voices" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[ChiptuneVoicesWidget alloc] initWithDeps:self.deps];
    }];
    [self registerType:SelectionPropertiesWidget.widgetType displayName:@"Selection Properties" instantiatorBlock:^id<WidgetProtocol> _Nonnull{
        return [[SelectionPropertiesWidget alloc] initWithDeps:self.deps];
    }];
}

- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type {
    WidgetRegistration *widgetRegistration = self.registeredWidgets[type];
    if (widgetRegistration == nil) {
        return [[HolderWidget alloc] initWithDeps:self.deps originalTypeName:type];
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
    NSArray<WidgetRegistration *> *regs = self.registeredWidgets.allValues;
    regs = [regs sortedArrayUsingComparator:^NSComparisonResult(WidgetRegistration * _Nonnull obj1, WidgetRegistration * _Nonnull obj2) {
        return [obj1.displayName compare:obj2.displayName];
    }];
    NSMutableArray *result = [NSMutableArray new];
    for (WidgetRegistration *reg in regs) {
        [result addObject:reg.type];
    }
    return result;
}

- (NSString *)displayNameForType:(NSString *)type {
    WidgetRegistration *widgetRegistration = self.registeredWidgets[type];
    return widgetRegistration.displayName;
}

@end
