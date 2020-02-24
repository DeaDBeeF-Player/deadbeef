//
//  NSImage+Additions.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/9/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "NSImage+Additions.h"

#import <AppKit/AppKit.h>


@implementation NSImage (Additions)

- (NSImage *)flippedImage {
    NSImage *fi = [[NSImage alloc] initWithSize:self.size];
    [fi lockFocusFlipped:YES];

    NSRect rect = NSMakeRect(0,0,self.size.width,self.size.height);
    [self drawInRect:rect fromRect:rect operation:NSCompositingOperationCopy fraction:1 respectFlipped:NO hints:nil];

    [fi unlockFocus];
    return fi;
}

@end
