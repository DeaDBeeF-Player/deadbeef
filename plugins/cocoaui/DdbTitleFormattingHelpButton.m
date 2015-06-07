//
//  DdbTitleFormattingHelpButton.m
//  deadbeef
//
//  Created by waker on 07/06/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import "DdbTitleFormattingHelpButton.h"

@implementation DdbTitleFormattingHelpButton

- (void)awakeFromNib {
    [self setAction:@selector(handleAction:)];
    [self setTarget:self];
}

- (void)handleAction:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString:@"https://github.com/Alexey-Yakovenko/deadbeef/wiki/Title-formatting-2.0"]];
}

@end
