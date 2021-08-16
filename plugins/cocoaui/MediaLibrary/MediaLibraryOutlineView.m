//
//  MediaLibraryOutlineView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 8/27/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MediaLibraryOutlineView.h"

@implementation MediaLibraryOutlineView

- (BOOL)validateProposedFirstResponder:(NSResponder *)responder forEvent:(NSEvent *)event {
    // This is required so that the Search field can become first responder.
    return YES;
}

- (void)otherMouseDown:(NSEvent *)event {
    if ((event.modifierFlags &~ NSEventModifierFlagDeviceIndependentFlagsMask) != 0) {
        return;
    }

    NSPoint windowLocation = event.locationInWindow;
    NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
    NSInteger row = [self rowAtPoint:viewLocation];

    if (row == -1) {
        return;
    }

    if (![self isRowSelected:row]) {
        id item = [self itemAtRow:row];
        if ([self.delegate respondsToSelector:@selector(outlineView:shouldSelectItem:)]
            && [self.delegate outlineView:self shouldSelectItem:item]) {
            [self selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
        }
        else {
            return;
        }
    }

    if ([self.delegate respondsToSelector:@selector(mediaLibraryOutlineViewDidActivateAlternative:)]) {
        id<MediaLibraryOutlineViewDelegate> delegate = (id<MediaLibraryOutlineViewDelegate>)self.delegate;
        [delegate mediaLibraryOutlineViewDidActivateAlternative:self];
    }
}

- (void)rightMouseDown:(NSEvent *)event {
    if ([self.delegate respondsToSelector:@selector(mediaLibraryOutlineView:shouldDisplayMenuForRow:)]) {
        id<MediaLibraryOutlineViewDelegate> delegate = (id<MediaLibraryOutlineViewDelegate>)self.delegate;

        NSPoint windowLocation = event.locationInWindow;
        NSPoint viewLocation = [self convertPoint:windowLocation fromView:nil];
        NSInteger row = [self rowAtPoint:viewLocation];

        BOOL shouldDisplayMenu = [delegate mediaLibraryOutlineView:self shouldDisplayMenuForRow:row];
        if (!shouldDisplayMenu) {
            return;
        }
    }
    [super rightMouseDown:event];
}

@end
