//
//  WidgetSerializer.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 22/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "DesignModeDefs.h"

NS_ASSUME_NONNULL_BEGIN

@interface WidgetSerializer : NSObject<WidgetSerializerProtocol>

@property (nonatomic,class,readonly) WidgetSerializer *sharedInstance;

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps;

- (nullable NSDictionary *)saveWidgetToDictionary:(id<WidgetProtocol>)widget;
- (nullable id<WidgetProtocol>)loadFromDictionary:(NSDictionary *)dictionary;

@end

NS_ASSUME_NONNULL_END
