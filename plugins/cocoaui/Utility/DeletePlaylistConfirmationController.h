//
//  DeletePlaylistConfirmationController.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <AppKit/AppKit.h>

NS_ASSUME_NONNULL_BEGIN

@class DeletePlaylistConfirmationController;

@protocol DeletePlaylistConfirmationControllerDelegate

- (void)deletePlaylistDone:(DeletePlaylistConfirmationController *)controller;

@end

@interface DeletePlaylistConfirmationController : NSObject

@property (nonatomic,weak) id<DeletePlaylistConfirmationControllerDelegate> delegate;
@property (nonatomic) NSString *title;
@property (nonatomic,weak) NSWindow *window;

- (void)run;

@end

NS_ASSUME_NONNULL_END
