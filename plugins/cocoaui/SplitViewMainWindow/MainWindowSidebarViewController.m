//
//  MainWindowSidebarViewController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/8/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "MainWindowSidebarViewController.h"

@interface MainWindowSidebarViewController ()

@property (weak) IBOutlet NSOutlineView *outlineView;
@property (weak) IBOutlet NSSearchField *searchField;
@property (weak) IBOutlet NSPopUpButton *selectorPopup;


@end

@implementation MainWindowSidebarViewController

- (void)close {
    self.mediaLibraryOutlineViewController = nil;
    self.outlineView = nil;
}

- (void)viewDidLoad {
    [super viewDidLoad];

#if ENABLE_MEDIALIB
    self.mediaLibraryOutlineViewController = [[MediaLibraryOutlineViewController alloc] initWithOutlineView:self.outlineView searchField:self.searchField selectorPopup:self.selectorPopup];
#endif
}


@end
