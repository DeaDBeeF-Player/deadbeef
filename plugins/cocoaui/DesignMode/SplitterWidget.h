//
//  SplitterWidget.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "WidgetBase.h"

NS_ASSUME_NONNULL_BEGIN

@interface SplitterWidget : WidgetBase<WidgetProtocol>

- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState vertical:(BOOL)vertical;


@end

NS_ASSUME_NONNULL_END
