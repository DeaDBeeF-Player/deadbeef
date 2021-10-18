//
//  VisualizationViewController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 18/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationViewController.h"

static NSString * const kWindowIsVisibleKey = @"view.window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface VisualizationViewController ()

@property (nonatomic) NSTimer *tickTimer;

@end

@implementation VisualizationViewController

- (void)dealloc {
    [self removeObserver:self forKeyPath:kWindowIsVisibleKey];
    [self.tickTimer invalidate];
}

- (void)awakeFromNib {
    [super awakeFromNib];
    // Do view setup here.
    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kIsVisibleContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if (context == kIsVisibleContext) {
        if (self.view.window.isVisible) {
            __weak VisualizationViewController *weakSelf = self;
            if (self.tickTimer == nil) {
                self.tickTimer = [NSTimer timerWithTimeInterval:1/30.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
                    VisualizationViewController *strongSelf = weakSelf;
                    if (!strongSelf.view.window.isVisible) {
                        [strongSelf.tickTimer invalidate];
                        strongSelf.tickTimer = nil;
                    }

                    strongSelf.view.needsDisplay = YES;
                }];

                [[NSRunLoop currentRunLoop] addTimer:self.tickTimer forMode:NSRunLoopCommonModes];
            }
        }
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

@end
