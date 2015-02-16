//
//  TrackPropertiesWindowController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 16/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import "TrackPropertiesWindowController.h"

@interface TrackPropertiesWindowController () {
    int _iter;
}

@end

@implementation TrackPropertiesWindowController

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
    }
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

- (void)initWithData:(int)iter {
    _iter = iter;
}
@end
