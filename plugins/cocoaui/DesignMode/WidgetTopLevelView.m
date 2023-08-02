//
//  WidgetTopLevelView.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/02/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <CoreImage/CIFilter.h>
#import <CoreImage/CIFilterBuiltins.h>
#import "DesignModeDefs.h"
#import "WidgetTopLevelView.h"

@interface WidgetTopLevelView()

@property (nonatomic) NSView *selectionOverlayView;
@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;

@end

@implementation WidgetTopLevelView

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithFrame:NSZeroRect];
    if (self == nil) {
        return nil;
    }
    _deps = deps;
    [self setup];
    return self;
}

- (NSMenu *)menu {
    return self.delegate.menu;
}

- (void)setup {
    _selectionOverlayView = [[NSView alloc] initWithFrame:NSZeroRect];
    _selectionOverlayView.translatesAutoresizingMaskIntoConstraints = NO;
    _selectionOverlayView.layer = [CALayer new];

    CIFilter *blur = (CIFilter<CIGaussianBlur> *)[CIFilter filterWithName:@"CIGaussianBlur"];
    [blur setDefaults];
    [blur setValue:@(1) forKey:kCIInputRadiusKey];
    _selectionOverlayView.layer.backgroundFilters = @[blur];

    NSColor *viewColor;
    if (@available(macOS 10.14, *)) {
        viewColor = NSColor.controlAccentColor;
    } else {
        viewColor = NSColor.alternateSelectedControlColor;
    }
    _selectionOverlayView.layer.backgroundColor = [viewColor colorWithAlphaComponent:0.7].CGColor;
    _selectionOverlayView.layer.borderColor = viewColor.CGColor;
    _selectionOverlayView.layer.borderWidth = 1;
}

- (void)rightMouseDown:(NSEvent *)event {
    if (!self.deps.state.enabled) {
        return;
    }
    [self.selectionOverlayView removeFromSuperview];
    [self addSubview:self.selectionOverlayView];
    [self.selectionOverlayView.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
    [self.selectionOverlayView.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
    [self.selectionOverlayView.topAnchor constraintEqualToAnchor:self.topAnchor].active = YES;
    [self.selectionOverlayView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor].active = YES;

    [super rightMouseDown: event];
    [self.selectionOverlayView removeFromSuperview];
}

- (void)rightMouseUp:(NSEvent *)event {
    if (!self.deps.state.enabled) {
        return;
    }
    [self.selectionOverlayView removeFromSuperview];
}

- (BOOL)wantsLayer {
    return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];
}

@end
