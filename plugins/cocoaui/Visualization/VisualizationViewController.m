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

- (void)viewDidLoad {
    [super viewDidLoad];

    self.tickTimer = [NSTimer scheduledTimerWithTimeInterval:1/60.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
        self.view.needsDisplay = YES;
    }];
}

@end
