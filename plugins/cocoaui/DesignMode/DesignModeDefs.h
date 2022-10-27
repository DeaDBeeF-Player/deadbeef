//
//  Widget.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol WidgetProtocol<NSObject>

@required
@property (class,nonatomic,readonly) NSString *widgetType;
@property (nonatomic,readonly) NSString *widgetType;
@property (nullable,nonatomic,weak) id<WidgetProtocol> parentWidget;
@property (nonatomic,readonly) BOOL isPlaceholder;
@property (nullable,nonatomic,readonly) NSMutableArray<id<WidgetProtocol>> *childWidgets;

- (nullable NSDictionary *)serializedSettingsDictionary;
- (BOOL)deserializeFromSettingsDictionary:(nullable NSDictionary *)dictionary;
- (void)configure;
- (NSView *)view;
- (void)appendChild:(id<WidgetProtocol>)child;
- (void)removeChild:(id<WidgetProtocol>)child;
- (void)insertChild:(id<WidgetProtocol>)child atIndex:(NSInteger)position;
- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild;
- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;

@optional
@property (nullable,nonatomic,readonly) NSArray<NSMenuItem *> *menuItems;
- (BOOL)makeFirstResponder;
- (void)cleanup; // Implement this method if it's necessary to call removeObserver etc
- (void)didFinishLoading;
- (nullable NSDictionary *)serializedRootDictionary;
- (void)deserializeFromRootDictionary:(nullable NSDictionary *)dictionary;
@property (nullable,nonatomic,readonly) NSString *displayName;

@end

@protocol WidgetFactoryProtocol
- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type;
@property (nonatomic,readonly) NSArray<NSString *> *types;
- (nullable NSString *)displayNameForType:(NSString *)type;
@end

@protocol WidgetMenuBuilderProtocol

- (NSMenu *)menuForWidget:(id<WidgetProtocol>)widget includeParentMenu:(BOOL)includeParentMenu;

@end

@protocol DesignModeStateProtocol

@property (nonatomic) BOOL enabled;
@property (nonatomic,readonly) id<WidgetProtocol> rootWidget;

- (void)load;
- (void)layoutDidChange;

@end

@protocol WidgetSerializerProtocol

- (nullable NSDictionary *)saveWidgetToDictionary:(id<WidgetProtocol>)widget;
- (nullable id<WidgetProtocol>)loadFromDictionary:(NSDictionary *)dictionary;

@end

@protocol DesignModeDepsProtocol

@property (nonatomic,readonly) id<DesignModeStateProtocol> state;
@property (nonatomic,readonly) id<WidgetFactoryProtocol> factory;
@property (nonatomic,readonly) id<WidgetSerializerProtocol> serializer;
@property (nonatomic,readonly) id<WidgetMenuBuilderProtocol> menuBuilder;

@end

NS_ASSUME_NONNULL_END
