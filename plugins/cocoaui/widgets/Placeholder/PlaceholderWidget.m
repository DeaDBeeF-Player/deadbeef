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

@property (nonatomic) NSView *view;

@end

@implementation PlaceholderWidget

- (instancetype)init
{
    self = [super init];
    if (self == nil) {
        return nil;
    }

    NSImage *checker;
    checker = [[NSImage alloc] initWithSize:NSMakeSize (12, 12)];
    [checker lockFocus];

    [NSColor.lightGrayColor set];
    [NSBezierPath strokeLineFromPoint:NSMakePoint(0, 0) toPoint:NSMakePoint(1,1)];

    [checker unlockFocus];

    self.view = [[NSView alloc] initWithFrame:NSZeroRect];
    self.view.layer = [CALayer new];
    self.view.layer.backgroundColor = [NSColor colorWithPatternImage:checker].CGColor;

    return self;
}

- (nonnull NSString *)serializedString {
    return @"{}";
}

- (void)appendChild:(id<WidgetProtocol>)child {
    if (self.view.subviews.count != 0) {
        return;
    }

    [self.view addSubview:child.view];
    child.parentWidget = self;

    [child.view.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor].active = YES;
    [child.view.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor].active = YES;
    [child.view.topAnchor constraintEqualToAnchor:self.view.topAnchor].active = YES;
    [child.view.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor].active = YES;
}

- (void)removeChild:(id<WidgetProtocol>)child {
    if (self.view.subviews.count != 0 && self.view.subviews.firstObject == child) {
        [child.view removeFromSuperview];
    }
}

- (void)replaceChild:(id<WidgetProtocol>)child withChild:(id<WidgetProtocol>)newChild {
    [self removeChild:child];
    [self appendChild:newChild];
}

@end
