//
//  LogWindowController.h
//  deadbeef
//
//  Created by waker on 11/08/16.
//  Copyright © 2016 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface LogWindowController : NSWindowController

@property (unsafe_unretained) IBOutlet NSTextView *textView;
@property (unsafe_unretained) IBOutlet NSClipView *clipView;

- (void)appendText:(NSString *)text;

@end
