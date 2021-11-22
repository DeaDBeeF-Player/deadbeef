//
//  HolderWidget.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 22/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "WidgetBase.h"

NS_ASSUME_NONNULL_BEGIN

@interface HolderWidget : WidgetBase

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps originalTypeName:(NSString *)originalTypeName;

@end

NS_ASSUME_NONNULL_END
