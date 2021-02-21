//
//  DesignModeEventHandler.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeEventHandler.h"
#import "DesignModeState.h"
#import "WidgetTopLevelView.h"

@interface DesignModeEventHandler()

@property (nonatomic) id<DesignModeStateProtocol> designModeState;

@end

@implementation DesignModeEventHandler

- (instancetype)init {
    return [self initWithDesignModeState: DesignModeState.sharedInstance];
}


- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    _designModeState = designModeState;
    return self;
}

- (BOOL)sendEvent:(NSEvent *)event {
    if (event.type != NSEventTypeRightMouseDown) {
        return NO;
    }

    if (!self.designModeState.isEnabled) {
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
        [hitView rightMouseDown:event];
        return YES;
    }

    return NO;
}

@end
