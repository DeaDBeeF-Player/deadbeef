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

#import "PlaylistView.h"
#import "PlaylistHeaderView.h"
#import "PlaylistContentView.h"
#import "DdbShared.h"
#include "../../deadbeef.h"

extern DB_functions_t *deadbeef;

static int headerheight = 23;


#pragma mark -

@interface PlaylistView()

@property (nonatomic,readwrite) PlaylistHeaderView *headerView;
@property (nonatomic,readwrite) PlaylistContentView *contentView;

@end

@implementation PlaylistView

- (PlaylistView *)initWithFrame:(NSRect)rect {
    self = [super initWithFrame:rect];
    if (self) {
        PlaylistHeaderView *thv = [[PlaylistHeaderView alloc] initWithFrame:NSMakeRect(0, rect.size.height-headerheight, rect.size.width, headerheight)];
        thv.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin;
        [self addSubview:thv];
        thv.listview = self;
        self.headerView = thv;

        NSScrollView *sv = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, rect.size.width, rect.size.height-headerheight)];
        [self addSubview:sv];

        NSSize size = [sv contentSize];
        NSRect lcvrect = NSMakeRect(0, 0, size.width, size.height-headerheight);
        PlaylistContentView *lcv = [[PlaylistContentView alloc] initWithFrame:lcvrect];
        lcv.autoresizingMask = NSViewWidthSizable;
        self.contentView = lcv;

        sv.documentView = lcv;

        sv.hasVerticalScroller = YES;
        sv.hasHorizontalScroller = YES;
        sv.autohidesScrollers = YES;
        sv.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin|NSViewHeightSizable;
        sv.contentView.copiesOnScroll = NO;

        NSView *synchronizedContentView = [sv contentView];
        synchronizedContentView.postsBoundsChangedNotifications = YES;
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(scrollChanged:) name:NSViewBoundsDidChangeNotification object:synchronizedContentView];

        [sv addObserver:self forKeyPath:@"frameSize" options:0 context:NULL];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidBecomeKey:)
                                                     name:NSWindowDidBecomeKeyNotification
                                                   object:self.window];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(windowDidBecomeKey:)
                                                     name:NSWindowDidResignKeyNotification
                                                   object:self.window];
    }
    return self;
}

- (void)windowDidBecomeKey:(id)sender {
    self.headerView.needsDisplay = YES;
    self.contentView.needsDisplay = YES;
}


- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if ([keyPath isEqualToString:@"frameSize"]) {
        [self updateContentFrame];
    }
}

- (void)scrollChanged:(id)notification {
    self.headerView.needsDisplay = YES;

    NSScrollView *sv = [self.contentView enclosingScrollView];
    NSRect vis = [sv documentVisibleRect];
    if ([_delegate respondsToSelector:@selector(scrollChanged:)]) {
        [_delegate scrollChanged:vis.origin.y];
    }
}

- (void)updateContentFrame {
    [self.contentView updateContentFrame];
}

- (void)setDelegate:(id<DdbListviewDelegate>)delegate {
    _delegate = delegate;
    self.contentView.delegate = delegate;
}

@end
