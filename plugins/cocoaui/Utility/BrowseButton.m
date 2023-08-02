//
//  BrowseButton.m
//  NesEdit
//
//  Created by Oleksiy Yakovenko on 3/19/19.
//  Copyright © 2019 Oleksiy Yakovenko. All rights reserved.
//

#import "BrowseButton.h"

@implementation BrowseButton

- (instancetype)initWithFrame:(NSRect)frameRect {
    frameRect.size.width = 12;
    frameRect.size.height = 10;
    self = [super initWithFrame:frameRect];
    self.image = [NSImage imageNamed:@"btnBrowseTemplate"];
    self.target = self;
    self.action = @selector(browseButtonPressed);
    self.imagePosition = NSImageOnly;
    self.buttonType = NSButtonTypeMomentaryPushIn;
    self.bezelStyle = NSBezelStyleShadowlessSquare;
    self.bordered = NO;
    return self;
}

- (void)browseButtonPressed {
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    panel.canChooseDirectories = self.isDir;
    panel.canChooseFiles = !self.isDir;
    panel.allowsMultipleSelection = NO;
    panel.canCreateDirectories = YES;
    panel.message = @"Select a tileset bitmap file";
    panel.directoryURL = [NSURL fileURLWithPath:self.initialPath];

    // Display the panel attached to the document's window.
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result){
        if (result == NSModalResponseOK && self.fileSelectedBlock) {
            NSURL * url = panel.URL;
            self.fileSelectedBlock(url.path);
        }
    }];
}

@end
