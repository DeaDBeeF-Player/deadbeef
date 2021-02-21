//
//  WidgetBase.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DesignModeState.h"
#import "WidgetBase.h"
#import "WidgetTopLevelView.h"

@interface WidgetBase()

@property (nonatomic) id<DesignModeStateProtocol> designModeState;
@property (nonatomic,readwrite) WidgetTopLevelView *topLevelView;

@end

@implementation WidgetBase

- (instancetype)init {
    return [self initWithDesignModeState:[DesignModeState new]];
}

- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _designModeState = designModeState;
    _childWidgets = [NSMutableArray new];
    _topLevelView = [[WidgetTopLevelView alloc] initWithDesignModeState:designModeState];
    _topLevelView.translatesAutoresizingMaskIntoConstraints = NO;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    for (id<WidgetProtocol> widget in self.childWidgets) {
        [widget message:_id ctx:ctx p1:p1 p2:p2];
    }
}

- (NSView *)view {
    return self.topLevelView;
}

@end
