//
//  DdbTableViewRightClickActivate.m
//  
//
//  Created by waker on 26/04/16.
//
//

#import "DdbTableViewRightClickActivate.h"

@implementation DdbTableViewRightClickActivate

- (NSMenu *)menuForEvent:(NSEvent *)theEvent;
{
    NSInteger row = [self rowAtPoint: [self convertPoint: [theEvent locationInWindow] fromView: nil]];
    if (row != -1) {
        [self selectRowIndexes: [NSIndexSet indexSetWithIndex:row] byExtendingSelection: NO];
    }
    return [super menu];
}

@end
