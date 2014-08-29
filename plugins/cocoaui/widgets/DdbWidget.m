//
//  DdbWidget.m
//  deadbeef
//
//  Created by waker on 29/08/14.
//  Copyright (c) 2014 Alexey Yakovenko. All rights reserved.
//

#import "DdbWidget.h"

@implementation DdbWidget

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code here.
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@"WidgetMenu"];
        [menu insertItemWithTitle:@"Insert" action:@selector(widgetInsert:) keyEquivalent:@"" atIndex:0];
        [menu insertItemWithTitle:@"Delete" action:@selector(widgetDelete:) keyEquivalent:@"" atIndex:1];
        [menu insertItemWithTitle:@"Cut" action:@selector(widgetCut:) keyEquivalent:@"" atIndex:2];
        [menu insertItemWithTitle:@"Copy" action:@selector(widgetCopy:) keyEquivalent:@"" atIndex:3];
        [menu insertItemWithTitle:@"Paste" action:@selector(widgetPaste:) keyEquivalent:@"" atIndex:4];
        [menu setDelegate:(id<NSMenuDelegate>)self];
        [self setMenu:menu];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    
    if (self.inDesignMode) {
        [[NSColor colorWithDeviceRed:0 green:0 blue:1 alpha:0.3f] set];
        [NSBezierPath fillRect:dirtyRect];
    }
}


- (void)menuWillOpen:(NSMenu *)menu
{
    self.inDesignMode = YES;
    [self setNeedsDisplay:YES];
}

- (void)menuDidClose:(NSMenu *)menu
{
    self.inDesignMode = NO;
    [self setNeedsDisplay:YES];
}

@end
