//
//  SidebarSplitViewController.h
//  SplitViewApp
//
//  Created by Oleksiy Yakovenko on 7/6/20.
//

#import <Cocoa/Cocoa.h>
#import "MainContentViewController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SidebarSplitViewController : NSSplitViewController

@property (nonatomic, readonly) MainContentViewController* bodyViewController;

@end

NS_ASSUME_NONNULL_END
