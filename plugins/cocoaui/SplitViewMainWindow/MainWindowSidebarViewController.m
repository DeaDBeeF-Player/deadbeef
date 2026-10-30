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

    self.mediaLibraryOutlineViewController = [[MediaLibraryOutlineViewController alloc] initWithOutlineView:self.outlineView searchField:self.searchField];
}

- (void)viewDidLayout {
    [super viewDidLayout];

    if (@available(macOS 11.0, *)) {
        NSWindow *window = self.view.window;
        if (window == nil) {
            return;
        }

        // The sidebar toggle button in the toolbar can extend below the system-provided top safe area inset.
        // Make sure the sidebar contents start below the titlebar and toolbar.
        // The top row is already pinned 20pt below the safe area in the XIB, so don't count that margin twice.
        static const CGFloat xibTopMargin = 20;
        CGFloat toolbarHeight = NSHeight (window.frame) - NSHeight (window.contentLayoutRect);
        NSEdgeInsets insets = self.view.additionalSafeAreaInsets;
        CGFloat systemTopInset = self.view.safeAreaInsets.top - insets.top;
        CGFloat additionalTopInset = MAX (0, toolbarHeight - systemTopInset - xibTopMargin);

        if (insets.top != additionalTopInset) {
            insets.top = additionalTopInset;
            self.view.additionalSafeAreaInsets = insets;
        }
    }
}


@end
