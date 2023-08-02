//
//  HelpWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 6/12/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "HelpWindowController.h"

@interface HelpWindowController ()

@property (unsafe_unretained) IBOutlet NSTextView *textView;

@end

@implementation HelpWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    [self loadContent];
}

- (void)loadContent {
    NSURL *path = self.contentURL;
    NSString *content = [NSString stringWithContentsOfFile:path.path encoding:NSUTF8StringEncoding error:nil];
    if (content) {

        NSFont *font;

        if (@available(macOS 10.15, *)) {
            font = [NSFont monospacedSystemFontOfSize:0 weight:NSFontWeightRegular];
        } else {
            font = [NSFont systemFontOfSize:0];
        }

        [self.textView.textStorage setAttributedString:[[NSAttributedString alloc] initWithString:content attributes:@{
            NSForegroundColorAttributeName: NSColor.controlTextColor,
            NSFontAttributeName: font

        }]];
        self.textView.selectedRange = NSMakeRange(0, 0);
    }
}

- (void)setContentURL:(NSURL *)contentURL {
    _contentURL = contentURL;

    if (self.textView != nil) {
        [self loadContent];
    }
}

@end
