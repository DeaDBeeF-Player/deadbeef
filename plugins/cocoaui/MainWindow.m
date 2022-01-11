//
//  MainWindow.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "MainWindow.h"
#import "DesignModeEventHandler.h"
#import "DesignModeDeps.h"
#import "DdbVolumeBar.h"

@interface MainWindow()

@property (nonatomic) DesignModeEventHandler *designModeEventHandler;

@end


@implementation MainWindow

- (void)awakeFromNib {
    _designModeEventHandler = [[DesignModeEventHandler alloc] initWithDeps:DesignModeDeps.sharedInstance];
}

- (void)sendEvent:(NSEvent *)event {
    if ([self.designModeEventHandler sendEvent:event]) {
        return;
    }

    if ([self toolbarContextMenuHandlerWithEvent:event]) {
        return;
    }

    [super sendEvent:event];
}

- (BOOL)toolbarContextMenuHandlerWithEvent:(NSEvent *)event {
    if (event.type != NSEventTypeRightMouseDown) {
        return NO;
    }

    NSView *windowView = self.contentView.superview;
    NSView *view = [self findHitViewWithMouseLocation:NSEvent.mouseLocation view:windowView];
    if ([view isKindOfClass:DdbVolumeBar.class]) {
        [view rightMouseDown:event];
        return YES;
    }

    return NO;
}

- (NSView *)findHitViewWithMouseLocation:(NSPoint)mouseLocation view:(NSView *)view {
    NSRect windowRect =  [view convertRect:view.bounds toView:nil];
    NSRect screenRect = [view.window convertRectToScreen:windowRect];

    if (!NSPointInRect(mouseLocation, screenRect)) {
        return nil;
    }

    for (NSView *subview in view.subviews) {
        NSView *hitView = [self findHitViewWithMouseLocation:mouseLocation view:subview];
        if ([hitView isKindOfClass:DdbVolumeBar.class]) {
            return hitView;
        }
    }

    return view;
}


@end
