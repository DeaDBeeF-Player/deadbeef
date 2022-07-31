//
//  RenameTabViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class RenameTabViewController;

@protocol RenameTabViewControllerDelegate
- (void)renameTabDone:(RenameTabViewController *)renameTabViewController withName:(NSString *)name;
@end

@interface RenameTabViewController : NSViewController

@property (nonatomic,weak) id<RenameTabViewControllerDelegate> delegate;
@property (nonatomic) NSString *name;
@property (nonatomic) NSPopover *popover;

@end

NS_ASSUME_NONNULL_END
