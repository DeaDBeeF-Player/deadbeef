//
//  VisualizationViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "VisualizationViewController.h"
#include <deadbeef/deadbeef.h>
#import "Weakify.h"

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

- (void)loadView {
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kIsVisibleContext];
}

- (void)visibilityDidChange {
    if (!self.view.window.isVisible) {
        return;
    }

    if (deadbeef->get_output()->state() != DDB_PLAYBACK_STATE_PLAYING) {
        return;
    }

    weakify(self);
    if (self.tickTimer != nil) {
        return;
    }

    _coolDown = 60;

    self.tickTimer = [NSTimer timerWithTimeInterval:1/30.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
        strongify(self);
        if (!self.view.window.isVisible) {
            [self.tickTimer invalidate];
            self.tickTimer = nil;
        }

        if (deadbeef->get_output()->state() == DDB_PLAYBACK_STATE_PLAYING) {
            self->_coolDown = 60;
        }
        else if (self->_coolDown > 0) {
            self->_coolDown -= 1;
        }

        if (self->_coolDown <= 0) {
            [self.tickTimer invalidate];
            self.tickTimer = nil;
            return;
        }

        [self draw];
    }];

    [NSRunLoop.mainRunLoop addTimer:self.tickTimer forMode:NSRunLoopCommonModes];
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

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_PLAYBACK_STATE_DID_CHANGE) {
        [self performSelectorOnMainThread:@selector(visibilityDidChange) withObject:nil waitUntilDone:NO];
    }
}

- (void)draw {
}

@end
