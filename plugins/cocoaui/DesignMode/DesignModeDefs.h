//
//  Widget.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol WidgetProtocol<NSObject>

@required
- (NSString *)serializedString;
- (NSView *)view;
@property (nullable,nonatomic,weak) id<WidgetProtocol> parentWidget;
@property (nonatomic,readonly) BOOL canInsert;

@optional
- (void)makeFirstResponder;
- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;
- (void)appendChild:(id<WidgetProtocol>)child;
- (void)removeChild:(id<WidgetProtocol>)child;
- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild;

@end

@protocol WidgetFactoryProtocol
- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type;
@property (nonatomic,readonly) NSArray<NSString *> *types;
@end

@protocol WidgetMenuBuilderProtocol

- (NSMenu *)menuForWidget:(id<WidgetProtocol>)widget;

@end

@protocol DesignModeStateProtocol

@property (nonatomic) BOOL enabled;
@property (nonatomic,readonly) id<WidgetFactoryProtocol> widgetFactory;
@property (nonatomic,readonly) id<WidgetMenuBuilderProtocol> menuBuilder;

@end

NS_ASSUME_NONNULL_END
