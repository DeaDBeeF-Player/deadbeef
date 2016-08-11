//
//  LogWindowController.m
//  deadbeef
//
//  Created by waker on 11/08/16.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import "LogWindowController.h"

@implementation LogWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    //[[_textView textStorage] setFont:[NSFont fontWithName:@"Fixed" size:18]];
}

- (void)appendText:(NSString *)text {
    NSAttributedString* attr = [[NSAttributedString alloc] initWithString:text];

    NSRect visibleRect = [_clipView documentVisibleRect];
    NSRect docRect = [_textView frame];

    BOOL scroll = NO;
    if (visibleRect.origin.y + visibleRect.size.height >= docRect.size.height) {
        scroll = YES;
    }

    [[_textView textStorage] appendAttributedString:attr];
    if (scroll) {
        [_textView scrollRangeToVisible:NSMakeRange([[_textView string] length], 0)];
    }
}

@end
