/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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
#include <deadbeef/deadbeef.h>

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
        PlaylistHeaderView *thv = [PlaylistHeaderView new];

        thv.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:thv];

        [thv.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
        [thv.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
        [thv.topAnchor constraintEqualToAnchor:self.topAnchor].active = YES;
        [thv.heightAnchor constraintEqualToConstant:headerheight].active = YES;

        thv.listview = self;
        self.headerView = thv;

        NSScrollView *sv = [NSScrollView new];
        sv.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:sv];

        [sv.leadingAnchor constraintEqualToAnchor:self.leadingAnchor].active = YES;
        [sv.trailingAnchor constraintEqualToAnchor:self.trailingAnchor].active = YES;
        [sv.topAnchor constraintEqualToAnchor:thv.bottomAnchor].active = YES;
        [sv.bottomAnchor constraintEqualToAnchor:self.bottomAnchor].active = YES;

        NSSize size = sv.contentSize;
        NSRect lcvrect = NSMakeRect(0, 0, size.width, size.height-headerheight);
        PlaylistContentView *lcv = [[PlaylistContentView alloc] initWithFrame:lcvrect];
        self.contentView = lcv;

        lcv.translatesAutoresizingMaskIntoConstraints = NO;
        sv.documentView = lcv;

        [lcv.leadingAnchor constraintEqualToAnchor:sv.contentView.leadingAnchor].active = YES;
        [lcv.topAnchor constraintEqualToAnchor:sv.contentView.topAnchor].active = YES;
        [lcv.widthAnchor constraintGreaterThanOrEqualToAnchor:sv.contentView.widthAnchor].active = YES;
        [lcv.heightAnchor constraintGreaterThanOrEqualToAnchor:sv.contentView.heightAnchor].active = YES;

        sv.hasVerticalScroller = YES;
        sv.hasHorizontalScroller = YES;
        sv.autohidesScrollers = YES;
        sv.autoresizingMask = NSViewWidthSizable|NSViewMinYMargin|NSViewHeightSizable;

        NSView *synchronizedContentView = sv.contentView;
        synchronizedContentView.postsBoundsChangedNotifications = YES;
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(scrollChanged:) name:NSViewBoundsDidChangeNotification object:synchronizedContentView];

    }
    return self;
}

- (void)scrollChanged:(id)notification {
    self.headerView.needsDisplay = YES;

    NSScrollView *sv = (self.contentView).enclosingScrollView;
    NSRect rect = sv.documentVisibleRect;
    [self.contentView scrollChanged:rect];
}

- (void)setDelegate:(id<DdbListviewDelegate>)delegate {
    _delegate = delegate;
    self.contentView.delegate = delegate;
}

- (void)setDataModel:(id<DdbListviewDataModelProtocol>)dataModel {
    _dataModel = dataModel;
    self.contentView.dataModel = dataModel;
}

@end
