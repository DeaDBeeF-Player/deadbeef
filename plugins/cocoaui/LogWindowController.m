/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2016 Alexey Yakovenko and other contributors

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

#import "LogWindowController.h"

@implementation LogWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    //[[_textView textStorage] setFont:[NSFont fontWithName:@"Fixed" size:18]];
}

- (void)appendText:(NSString *)text {
    NSAttributedString* attr = [[NSAttributedString alloc] initWithString:text attributes:@{NSForegroundColorAttributeName:[NSColor controlTextColor]}];

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

- (IBAction)clearAction:(id)sender {
    [_textView.textStorage setAttributedString: [[NSAttributedString alloc] initWithString:@"" attributes:@{NSForegroundColorAttributeName:[NSColor controlTextColor]}]];
}

@end
