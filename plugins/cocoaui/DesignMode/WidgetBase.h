//
//  WidgetBase.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 20/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "DesignModeDefs.h"
#import "WidgetFactory.h"

NS_ASSUME_NONNULL_BEGIN

@interface WidgetBase : NSObject<WidgetProtocol>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps NS_DESIGNATED_INITIALIZER;

- (void)configure;

@property (nullable,nonatomic,weak) id<WidgetProtocol> parentWidget;
@property (nonatomic,readonly) NSView *topLevelView;
@property (nonatomic,readonly) NSView *view;
@property (nonatomic,readonly) BOOL canInsert;

- (void)appendChild:(id<WidgetProtocol>)child;
- (void)removeChild:(id<WidgetProtocol>)child;
- (void)insertChild:(id<WidgetProtocol>)child atIndex:(NSInteger)position;
- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild;

@end

NS_ASSUME_NONNULL_END
