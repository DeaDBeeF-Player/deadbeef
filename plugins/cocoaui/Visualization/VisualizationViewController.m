    //
//  VisualizationViewController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationViewController.h"

static void *kHiddenOrHasHiddenAncestorContext = &kHiddenOrHasHiddenAncestorContext;

@interface VisualizationViewController ()

@property (nonatomic) NSTimer *tickTimer;

@end

@implementation VisualizationViewController

- (void)dealloc {
    [self removeObserver:self forKeyPath:@"view.hiddenOrHasHiddenAncestor"];
    [self.tickTimer invalidate];
}

- (void)awakeFromNib {
    [self addObserver:self forKeyPath:@"view.window.isVisible" options:NSKeyValueObservingOptionInitial context:kHiddenOrHasHiddenAncestorContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if (context == kHiddenOrHasHiddenAncestorContext) {
        if (self.view.window.isVisible) {
            __weak VisualizationViewController *weakSelf = self;
            if (self.tickTimer == nil) {
                self.tickTimer = [NSTimer timerWithTimeInterval:1/30.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
                    VisualizationViewController *strongSelf = weakSelf;
                    if (!strongSelf.view.window.isVisible) {
                        [strongSelf.tickTimer invalidate];
                        strongSelf.tickTimer = nil;
                        return;
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
