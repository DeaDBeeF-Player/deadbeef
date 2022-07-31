//
//  PinnedGroupTitleView.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 2/28/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "PinnedGroupTitleView.h"
#import "PlaylistGroup.h"

static int grouptitleheight = 22;

@implementation PinnedGroupTitleView

- (instancetype)init
{
    self = [super init];
    if (!self) {
        return nil;
    }

    self.wantsLayer = YES;

    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!self.group) {
        return;
    }

    id<DdbListviewDelegate> delegate = self.delegate;
    NSRect groupRect = NSMakeRect(0, 0, self.frame.size.width, grouptitleheight);
    NSColor *clr = [NSColor.windowBackgroundColor colorWithAlphaComponent:0.9];
    [clr set];
#if DEBUG_DRAW_GROUP_TITLES
    [NSColor.greenColor set];
#endif
    [NSBezierPath fillRect:groupRect];
    [delegate drawGroupTitle:self.group->head inRect:groupRect];
}

- (void)setGroup:(PlaylistGroup *)group {
    if (group != _group) {
        _group = group;
        self.needsDisplay = YES;
    }
}

@end
