//
//  DdbWidget.m
//  deadbeef
//
//  Created by waker on 29/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidget.h"
#import "DdbWidgetManager.h"

@implementation DdbWidget

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [[DdbWidgetManager defaultWidgetManager] addWidget:self];
#if 0
        // Initialization code here.
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@"WidgetMenu"];
        [menu insertItemWithTitle:@"Insert" action:@selector(widgetInsert:) keyEquivalent:@"" atIndex:0];
        [menu insertItemWithTitle:@"Delete" action:@selector(widgetDelete:) keyEquivalent:@"" atIndex:1];
        [menu insertItemWithTitle:@"Cut" action:@selector(widgetCut:) keyEquivalent:@"" atIndex:2];
        [menu insertItemWithTitle:@"Copy" action:@selector(widgetCopy:) keyEquivalent:@"" atIndex:3];
        [menu insertItemWithTitle:@"Paste" action:@selector(widgetPaste:) keyEquivalent:@"" atIndex:4];
        [menu setDelegate:(id<NSMenuDelegate>)self];
        [self setMenu:menu];
#endif
    }
    return self;
}

- (void)dealloc {
    [[DdbWidgetManager defaultWidgetManager] removeWidget:self];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    return;
    if (self.inDesignMode) {
        [[NSColor colorWithDeviceRed:0 green:0 blue:1 alpha:0.3f] set];
        [NSBezierPath fillRect:dirtyRect];
    }
}


- (void)menuWillOpen:(NSMenu *)menu
{
    return;
    self.inDesignMode = YES;
    [self setNeedsDisplay:YES];
}

- (void)menuDidClose:(NSMenu *)menu
{
    return;
    self.inDesignMode = NO;
    [self setNeedsDisplay:YES];
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    return 0;
}

@end
