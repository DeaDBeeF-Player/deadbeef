//
//  HelpWindowController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 6/12/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HelpWindowController.h"

@interface HelpWindowController ()

@property (unsafe_unretained) IBOutlet NSTextView *textView;


@end

@implementation HelpWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.

    NSURL *path = [[NSBundle mainBundle] URLForResource:@"help-cocoa" withExtension:@"txt"];
    NSString *content = [NSString stringWithContentsOfFile:[path path] encoding:NSUTF8StringEncoding error:nil];
    if (content) {
        [self.textView.textStorage setAttributedString:
         [[NSAttributedString alloc] initWithString:content attributes:@{NSForegroundColorAttributeName: NSColor.controlTextColor}]];
        self.textView.selectedRange = NSMakeRange(0, 0);
    }
}

@end
