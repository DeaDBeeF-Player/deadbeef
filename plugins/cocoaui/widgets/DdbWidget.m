/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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

#import "DdbWidget.h"
#import "DdbWidgetManager.h"

@implementation DdbWidget {
    BOOL _registered;
}

- (void)awakeFromNib {
    if (!_registered) {
        [[DdbWidgetManager defaultWidgetManager] addWidget:self];
        _registered = YES;
    }
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        if (!_registered) {
            [[DdbWidgetManager defaultWidgetManager] addWidget:self];
            _registered = YES;
        }
#if 0
        // Initialization code here.
        NSMenu *menu = [[NSMenu alloc] initWithTitle:@"WidgetMenu"];
        [menu insertItemWithTitle:@"Insert" action:@selector(widgetInsert:) keyEquivalent:@"" atIndex:0];
        [menu insertItemWithTitle:@"Delete" action:@selector(widgetDelete:) keyEquivalent:@"" atIndex:1];
        [menu insertItemWithTitle:@"Cut" action:@selector(widgetCut:) keyEquivalent:@"" atIndex:2];
        [menu insertItemWithTitle:@"Copy" action:@selector(widgetCopy:) keyEquivalent:@"" atIndex:3];
        [menu insertItemWithTitle:@"Paste" action:@selector(widgetPaste:) keyEquivalent:@"" atIndex:4];
        menu.delegate = self;
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
