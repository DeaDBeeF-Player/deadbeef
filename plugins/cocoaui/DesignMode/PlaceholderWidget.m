/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "PlaceholderWidget.h"
#import "WidgetFactory.h"

const NSInteger GRIDSIZE = 16;

@interface PlaceholderWidget()

@property (nonatomic) NSView *placeholderView;
@property (nonatomic) CALayer *backgroundLayer;
@property (nonatomic) id<WidgetProtocol> containedWidget;

@end

@implementation PlaceholderWidget

+ (NSString *)widgetType {
    return @"Placeholder";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    NSImage *checker;
    checker = [[NSImage alloc] initWithSize:NSMakeSize (12, 12)];
    [checker lockFocus];

    [NSColor.lightGrayColor set];
    [NSBezierPath strokeLineFromPoint:NSMakePoint(0, 0) toPoint:NSMakePoint(1,1)];

    [checker unlockFocus];

    _placeholderView = [[NSView alloc] initWithFrame:NSZeroRect];
    _backgroundLayer = [CALayer new];
    _backgroundLayer.backgroundColor = [NSColor colorWithPatternImage:checker].CGColor;
    _placeholderView.layer = _backgroundLayer;

    _placeholderView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:_placeholderView];
    [_placeholderView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_placeholderView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_placeholderView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_placeholderView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)appendChild:(id<WidgetProtocol>)child {
    if (self.containedWidget != nil) {
        return;
    }

    _placeholderView.layer = nil;

    self.containedWidget = child;
    [self.placeholderView addSubview:child.view];
    child.parentWidget = self;

    [child.view.leadingAnchor constraintEqualToAnchor:self.placeholderView.leadingAnchor].active = YES;
    [child.view.trailingAnchor constraintEqualToAnchor:self.placeholderView.trailingAnchor].active = YES;
    [child.view.topAnchor constraintEqualToAnchor:self.placeholderView.topAnchor].active = YES;
    [child.view.bottomAnchor constraintEqualToAnchor:self.placeholderView.bottomAnchor].active = YES;
    [super appendChild:child];
}

- (void)removeChild:(id<WidgetProtocol>)child {
    if (self.placeholderView.subviews.count != 0 && self.containedWidget == child) {
        [child.view removeFromSuperview];
        child.parentWidget = nil;
        self.containedWidget = nil;
        _placeholderView.layer = self.backgroundLayer;
        [super removeChild:child];
    }
}

- (BOOL)isPlaceholder {
    return YES;
}

@end
