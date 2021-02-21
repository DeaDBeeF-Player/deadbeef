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

@interface WidgetBase : NSObject

@property (nullable,nonatomic,weak) id<WidgetProtocol> parentWidget;
@property (nullable,nonatomic) NSMutableArray<id<WidgetProtocol>> *childWidgets;

@end

NS_ASSUME_NONNULL_END
