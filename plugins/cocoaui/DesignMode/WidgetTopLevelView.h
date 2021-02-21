//
//  WidgetTopLevelView.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@protocol WidgetTopLevelViewDelegate

@property (nullable,nonatomic,readonly) NSMenu *menu;

@end

@interface WidgetTopLevelView : NSView

@property (weak,nonatomic) id<WidgetTopLevelViewDelegate> delegate;

- (instancetype)initWithDesignModeState:(nullable id<DesignModeStateProtocol>)designModeState NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
