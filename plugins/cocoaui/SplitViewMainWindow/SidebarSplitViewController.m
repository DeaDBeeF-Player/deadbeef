//
//  SidebarSplitViewController.m
//  SplitViewApp
//
//  Created by Alexey Yakovenko on 7/6/20.
//

#import "SidebarSplitViewController.h"
#import "SidebarSplitViewItem.h"

@interface SidebarSplitViewController ()

@property IBOutlet NSViewController* sidebarViewController;
@property IBOutlet NSViewController* bodyViewController;

@property IBOutlet NSSplitView* splitView;

@property (nonatomic) id trackingItem;

@end


@implementation SidebarSplitViewController

@dynamic splitView ;

- (void)viewDidLoad {
    self.splitView.wantsLayer = YES;
    
    NSSplitViewItem* sidebarItem = [SidebarSplitViewItem splitViewItemWithViewController:self.sidebarViewController];
    sidebarItem.canCollapse = YES;
    [self insertSplitViewItem:sidebarItem atIndex:0];

    NSSplitViewItem* bodyItem = [NSSplitViewItem splitViewItemWithViewController:self.bodyViewController];
    bodyItem.canCollapse = YES;
    [self insertSplitViewItem:bodyItem atIndex:1];

    if (@available(macOS 10.16, *)) {
        self.trackingItem = [NSTrackingSeparatorToolbarItem trackingSeparatorToolbarItemWithIdentifier:NSToolbarSidebarTrackingSeparatorItemIdentifier splitView:self.splitView dividerIndex:0];

        [self.view.window.toolbar insertItemWithItemIdentifier:NSToolbarSidebarTrackingSeparatorItemIdentifier atIndex:1];
    }

    [super viewDidLoad];

    self.splitView.autosaveName = @"MainWindowSplitView";
}

@end
