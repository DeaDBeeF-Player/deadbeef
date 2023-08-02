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
    self.splitView.wantsLayer = YES;

    NSSplitViewItem* sidebarItem = [SidebarSplitViewItem splitViewItemWithViewController:self.sidebarViewController];
    sidebarItem.canCollapse = YES;
    [self insertSplitViewItem:sidebarItem atIndex:0];
    sidebarItem.holdingPriority = NSLayoutPriorityDefaultLow+10;

    self.bodyViewController = [MainContentViewController new];

    NSSplitViewItem* bodyItem = [NSSplitViewItem splitViewItemWithViewController:self.bodyViewController];
    bodyItem.canCollapse = NO;
    [self insertSplitViewItem:bodyItem atIndex:1];
    bodyItem.holdingPriority = NSLayoutPriorityDefaultLow;

#if 0 // FIXME: broken in Big Sur beta4
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
    if (@available(macOS 10.16, *)) {
        self.trackingItem = [NSTrackingSeparatorToolbarItem trackingSeparatorToolbarItemWithIdentifier:NSToolbarSidebarTrackingSeparatorItemIdentifier splitView:self.splitView dividerIndex:0];

        [self.view.window.toolbar insertItemWithItemIdentifier:NSToolbarSidebarTrackingSeparatorItemIdentifier atIndex:1];
    }
#endif
#endif

    [super viewDidLoad];

    self.splitView.autosaveName = @"MainWindowSplitView";
}

@end
