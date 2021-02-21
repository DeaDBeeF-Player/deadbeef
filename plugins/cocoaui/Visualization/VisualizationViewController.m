    //
//  VisualizationViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationViewController.h"

@interface VisualizationViewController ()

@property (nonatomic) NSTimer *tickTimer;

@end

@implementation VisualizationViewController

- (void)dealloc {
    [self.tickTimer invalidate];
}

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    __weak VisualizationViewController *weakSelf = self;
    self.tickTimer = [NSTimer scheduledTimerWithTimeInterval:1/30.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
        VisualizationViewController *strongSelf = weakSelf;
        strongSelf.view.needsDisplay = YES;
    }];

    return self;
}

@end
