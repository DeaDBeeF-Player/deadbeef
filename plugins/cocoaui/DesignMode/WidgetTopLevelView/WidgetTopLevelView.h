//
//  WidgetTopLevelView.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@interface WidgetTopLevelView : NSView

- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
