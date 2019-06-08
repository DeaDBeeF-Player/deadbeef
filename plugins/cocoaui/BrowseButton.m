//
//  BrowseButton.m
//  NesEdit
//
//  Created by Alexey Yakovenko on 3/19/19.
//  Copyright © 2019 Alexey Yakovenko. All rights reserved.
//

#import "BrowseButton.h"

@implementation BrowseButton

- (instancetype)initWithFrame:(NSRect)frameRect {
    frameRect.size.width = 12;
    frameRect.size.height = 10;
    self = [super initWithFrame:frameRect];
    [self setImage:[NSImage imageNamed:@"btnBrowseTemplate"]];
    [self setTarget:self];
    [self setAction:@selector(browseButtonPressed)];
    [self setImagePosition:NSImageOnly];
    [self setButtonType:NSButtonTypeMomentaryPushIn];
    [self setBezelStyle:NSBezelStyleShadowlessSquare];
    [self setBordered:NO];
    [self setContentTintColor:[[NSColor controlTextColor] colorWithAlphaComponent:0.35f]];
    return self;
}

- (void)browseButtonPressed {
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    [panel setCanChooseDirectories:self.isDir];
    [panel setCanChooseFiles:!self.isDir];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanCreateDirectories:YES];
    [panel setMessage:@"Select a tileset bitmap file"];

    // Display the panel attached to the document's window.
    [panel beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result){
        if (result == NSModalResponseOK && self.fileSelectedBlock) {
            NSURL * url = [panel URL];
            self.fileSelectedBlock([url path]);
        }
    }];
}

@end
