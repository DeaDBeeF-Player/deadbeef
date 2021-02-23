//
//  WidgetBase.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "DesignModeDefs.h"
#import "WidgetFactory.h"

NS_ASSUME_NONNULL_BEGIN

@interface WidgetBase : NSObject<WidgetProtocol>

- (instancetype)initWithDeps:(nullable id<DesignModeDepsProtocol>)deps NS_DESIGNATED_INITIALIZER;

@property (nullable,nonatomic,weak) id<WidgetProtocol> parentWidget;
@property (nonatomic,readonly) NSView *topLevelView;
@property (nonatomic,readonly) NSView *view;
@property (nonatomic,readonly) BOOL canInsert;

- (void)appendChild:(id<WidgetProtocol>)child NS_REQUIRES_SUPER;
- (void)removeChild:(id<WidgetProtocol>)child NS_REQUIRES_SUPER;
- (void)insertChild:(id<WidgetProtocol>)child atIndex:(NSInteger)position;
- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild;

@end

NS_ASSUME_NONNULL_END
