//
//  SidebarSplitViewController.m
//  SplitViewApp
//
//  Created by Oleksiy Yakovenko on 7/6/20.
//

#import "SidebarSplitViewController.h"
#import "SidebarSplitViewItem.h"

@interface SidebarSplitViewController ()

@property IBOutlet NSViewController* sidebarViewController;

@property IBOutlet NSSplitView* splitView;
@property (nonatomic,readwrite) MainContentViewController* bodyViewController;
@property (nonatomic) id trackingItem;

@end


@implementation SidebarSplitViewController

@dynamic splitView ;

- (void)viewDidLoad {
    [super viewDidLoad];
    self.splitView.wantsLayer = YES;

    NSSplitViewItem* sidebarItem = [NSSplitViewItem sidebarWithViewController:self.sidebarViewController];
    [self addSplitViewItem:sidebarItem];

    self.bodyViewController = [MainContentViewController new];

    NSSplitViewItem* bodyItem = [NSSplitViewItem splitViewItemWithViewController:self.bodyViewController];
    [self addSplitViewItem:bodyItem];

    if (@available(macOS 11.0, *)) {
        NSToolbar* toolbar = self.view.window.toolbar;

        [toolbar insertItemWithItemIdentifier:NSToolbarToggleSidebarItemIdentifier atIndex:0];
        [toolbar insertItemWithItemIdentifier:NSToolbarSidebarTrackingSeparatorItemIdentifier atIndex:1];
    }

    self.splitView.autosaveName = @"MainWindowSplitView";
}

@end
