//
//  WidgetMenuBuilder.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface WidgetMenuBuilder : NSObject<WidgetMenuBuilderProtocol>

@property (nonatomic,class,readonly) WidgetMenuBuilder *sharedInstance;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps NS_DESIGNATED_INITIALIZER;
- (NSMenu *)menuForWidget:(id<WidgetProtocol>)widget;

@end

NS_ASSUME_NONNULL_END
