//
//  MainWindowSidebarViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/8/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MediaLibraryOutlineViewController.h"

NS_ASSUME_NONNULL_BEGIN

@interface MainWindowSidebarViewController : NSViewController

@property (nonatomic) MediaLibraryOutlineViewController *mediaLibraryOutlineViewController;

@end

NS_ASSUME_NONNULL_END
