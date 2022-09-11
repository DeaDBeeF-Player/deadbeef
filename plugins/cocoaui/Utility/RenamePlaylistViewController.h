//
//  RenamePlaylistViewController.h
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 21/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

@class RenamePlaylistViewController;

@protocol RenamePlaylistViewControllerDelegate

- (void)renamePlaylist:(RenamePlaylistViewController *)viewController doneWithName:(NSString *)name;

@end


@interface RenamePlaylistViewController : NSViewController

@property (nonatomic, weak) id<RenamePlaylistViewControllerDelegate> delegate;
@property (nonatomic) NSString *name;
@property (nonatomic, weak) NSPopover *popover;

@end

NS_ASSUME_NONNULL_END
