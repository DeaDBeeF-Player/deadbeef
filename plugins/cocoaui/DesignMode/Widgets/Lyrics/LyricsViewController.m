//
//  LyricsViewController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "LyricsViewController.h"

extern DB_functions_t *deadbeef;

@interface LyricsViewController ()

@property (unsafe_unretained) IBOutlet NSTextView *textView;

@end

@implementation LyricsViewController

- (void)dealloc {
    if (_track != NULL) {
        deadbeef->pl_item_unref (_track);
        _track = NULL;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (void)setTrack:(ddb_playItem_t *)track {
    if (_track != NULL) {
        deadbeef->pl_item_unref (_track);
    }

    _track = track;
    if (_track != NULL) {
        deadbeef->pl_item_ref (_track);
    }
}

@end
