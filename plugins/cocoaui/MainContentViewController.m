//
//  MainContentViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 02/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "DdbShared.h"
#import "MainContentViewController.h"

@implementation MainContentViewController

- (IBAction)createNewPlaylistAction:(id)sender {
    int playlist = cocoaui_add_new_playlist ();
    if (playlist != -1) {
        cocoaui_playlist_set_curr (playlist);
    }
}

@end
