//
//  VisualizationViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "VisualizationViewController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

static NSString * const kWindowIsVisibleKey = @"view.window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface VisualizationViewController () {
    int _coolDown;
}

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

- (void)visibilityDidChange {
    if (self.view.window.isVisible) {
        __weak VisualizationViewController *weakSelf = self;
        if (self.tickTimer == nil) {
            self.tickTimer = [NSTimer timerWithTimeInterval:1/30.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
                VisualizationViewController *strongSelf = weakSelf;
                if (!strongSelf.view.window.isVisible) {
                    [strongSelf.tickTimer invalidate];
                    strongSelf.tickTimer = nil;
                }

                DB_output_t *output = deadbeef->get_output();
                if (output->state() == DDB_PLAYBACK_STATE_PLAYING) {
                    strongSelf->_coolDown = 60;
                }
                else if (strongSelf->_coolDown > 0) {
                    strongSelf->_coolDown -= 1;
                }


                if (strongSelf->_coolDown > 0) {
                    [strongSelf draw];
                }
            }];

            // FIXME: this should not be called for the metal visualizations, since AAPLNSView handles it
            [[NSRunLoop currentRunLoop] addTimer:self.tickTimer forMode:NSRunLoopCommonModes];
        }
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if (context == kIsVisibleContext) {
        [self visibilityDidChange];
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)viewDidAppear {
    [self visibilityDidChange];
}

- (void)draw {
}

@end
