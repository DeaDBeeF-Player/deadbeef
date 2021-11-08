//
//  DesignModeEventHandler.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeEventHandler.h"
#import "DesignModeDeps.h"
#import "WidgetTopLevelView.h"

@interface DesignModeEventHandler()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@end

@implementation DesignModeEventHandler

- (instancetype)init {
    return [self initWithDeps: DesignModeDeps.sharedInstance];
}


- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    _deps = deps;
    return self;
}

- (BOOL)sendEvent:(NSEvent *)event {
    BOOL isRightMouseDown = event.type == NSEventTypeRightMouseDown;
    BOOL isCtrlLeftMouseDown = event.type == NSEventTypeLeftMouseDown
        && ((event.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask) == NSEventModifierFlagControl);
    if (!isRightMouseDown && !isCtrlLeftMouseDown) {
        return NO;
    }

    if (!self.deps.state.enabled) {
        return NO;
    }

    NSPoint contentViewPoint = [event.window.contentView.superview convertPoint:event.locationInWindow fromView:nil];
    NSView *hitView = [event.window.contentView hitTest:contentViewPoint];
    NSInteger count = 5;
    while (count-- > 0 && hitView) {
        if ([hitView isKindOfClass:WidgetTopLevelView.class]) {
            break;
        }
        hitView = hitView.superview;
    }

    if (hitView && count >= 0) {
        if (isRightMouseDown || isCtrlLeftMouseDown) {
            [hitView rightMouseDown:event];
        }
        return YES;
    }

    return NO;
}

@end
