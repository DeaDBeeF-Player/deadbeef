//
//  WidgetTopLevelView.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@protocol WidgetTopLevelViewDelegate

@property (nullable,nonatomic,readonly) NSMenu *menu;

@end

@interface WidgetTopLevelView : NSView

@property (weak,nonatomic) id<WidgetTopLevelViewDelegate> delegate;

- (instancetype)initWithFrame:(NSRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;
- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps NS_DESIGNATED_INITIALIZER;

@end

NS_ASSUME_NONNULL_END
