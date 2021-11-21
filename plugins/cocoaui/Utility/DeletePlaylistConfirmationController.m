//
//  DeletePlaylistConfirmationController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DeletePlaylistConfirmationController.h"

@implementation DeletePlaylistConfirmationController

- (void)run {
    NSAlert *alert = [NSAlert new];

    alert.messageText = @"Removing playlist";
    alert.informativeText = [NSString stringWithFormat:@"Do you really want to remove the playlist '%@'?", self.title];
    [alert addButtonWithTitle:@"No"];
    [alert addButtonWithTitle:@"Yes"];
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 101600
    if (@available(macOS 10.16, *)) {
        alert.buttons[1].hasDestructiveAction = YES;
    }
#endif

//    self.playlistConfirmationAlertOpen = YES;

    [alert beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSAlertSecondButtonReturn) {
            [self.delegate deletePlaylistDone:self];
        }
    }];
}

@end
