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

@interface WidgetBase() <WidgetTopLevelViewDelegate,WidgetProtocol>

@property (nonatomic) id<DesignModeStateProtocol> designModeState;
@property (nonatomic) id<WidgetMenuBuilderProtocol> menuBuilder;
@property (nonatomic,readwrite) WidgetTopLevelView *topLevelView;

@end

@implementation WidgetBase

- (instancetype)init {
    return [self initWithDesignModeState:nil menuBuilder:nil];
}

- (instancetype)initWithDesignModeState:(id<DesignModeStateProtocol>)designModeState menuBuilder:(id<WidgetMenuBuilderProtocol>)menuBuilder {
    self = [super init];

    if (self == nil) {
        return nil;
    }

    _designModeState = designModeState;
    _menuBuilder = menuBuilder;

    _childWidgets = [NSMutableArray new];
    _topLevelView = [[WidgetTopLevelView alloc] initWithDesignModeState:designModeState];
    _topLevelView.translatesAutoresizingMaskIntoConstraints = NO;
    _topLevelView.delegate = self;

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

- (nonnull NSString *)serializedString {
    return @"";
}

- (BOOL)canInsert {
    return NO;
}

- (NSMenu *)menu {
    return [self.menuBuilder menuForWidget:self];
}

@end
