//
//  SearchWindowController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 24/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import "SearchWindowController.h"

@interface SearchWindowController ()

@end

@implementation SearchWindowController

- (void)windowDidLoad {
    [super windowDidLoad];
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

- (void)reset {
    DdbSearchViewController *ctl = (DdbSearchViewController *)_viewController;
    [ctl reset];
}
@end
