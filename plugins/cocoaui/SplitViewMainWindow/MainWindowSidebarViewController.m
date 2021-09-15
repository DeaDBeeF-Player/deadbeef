//
//  MainWindowSidebarViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/8/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "MainWindowSidebarViewController.h"

@interface MainWindowSidebarViewController ()

@property (weak) IBOutlet NSOutlineView *outlineView;

@end

@implementation MainWindowSidebarViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.mediaLibraryOutlineViewController = [[MediaLibraryOutlineViewController alloc] initWithOutlineView:self.outlineView];
}


@end
