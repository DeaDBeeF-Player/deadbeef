//
//  DdbTabStripViewController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#include "deadbeef.h"
#import "DdbTabStrip.h"
#import "DdbShared.h"
#import "DdbTabStripViewController.h"

extern DB_functions_t *deadbeef;

@interface DdbTabStripViewController ()

@property (weak) IBOutlet DdbTabStrip *tabStripView;

@end

@implementation DdbTabStripViewController

- (IBAction)createNewPlaylistAction:(id)sender {
    int playlist = cocoaui_add_new_playlist ();
    if (playlist != -1) {
        deadbeef->plt_set_curr_idx (playlist);
    }
}

- (int)widgetMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    return [self.tabStripView widgetMessage:_id ctx:ctx p1:p1 p2:p2];
}

@end
