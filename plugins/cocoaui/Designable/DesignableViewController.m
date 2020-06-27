//
//  DesignableViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 2/9/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "DesignableViewController.h"

@interface DesignableViewController ()

@end

@implementation DesignableViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
}

- (NSDictionary *)serialize {
    return nil;
}

- (int)sendMessage:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    for (DesignableViewController *ctl in self.subviewControllers) {
        [ctl sendMessage:_id ctx:ctx p1:p1 p2:p2];
    }
    return 0;
}

- (void)cleanup {
}

@end
