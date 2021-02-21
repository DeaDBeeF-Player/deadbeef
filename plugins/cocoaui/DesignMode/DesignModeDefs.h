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

@protocol WidgetProtocol

@required
- (NSString *)serializedString;
- (NSView *)view;
@property (nullable,nonatomic,weak) id<WidgetProtocol> parentWidget;

@optional
- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2;
- (void)appendChild:(id<WidgetProtocol>)child;
- (void)removeChild:(id<WidgetProtocol>)child;
- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild;

@end

@protocol DesignModeProtocol

@property (nonatomic,getter=isEnabled) BOOL enabled;

@end

@protocol WidgetFactoryProtocol
- (nullable id<WidgetProtocol>)createWidgetWithType:(NSString *)type;
@end


NS_ASSUME_NONNULL_END
