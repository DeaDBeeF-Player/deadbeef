//
//  SearchWindowController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 24/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DdbSearchViewController.h"

@interface SearchWindowController : NSWindowController

@property (strong) IBOutlet DdbSearchViewController *viewController;

- (void)reset;

@end
