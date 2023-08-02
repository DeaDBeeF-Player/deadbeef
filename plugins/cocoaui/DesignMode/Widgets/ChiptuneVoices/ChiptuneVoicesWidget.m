/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "ChiptuneVoicesWidget.h"
#import "ChiptuneVoicesViewController.h"
#include <deadbeef/deadbeef.h>

extern DB_functions_t *deadbeef;

@interface ChiptuneVoicesWidget() <ChiptuneVoicesViewControllerDelegate> {
    int _voices;
}

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic) ChiptuneVoicesViewController *viewController;

@end

@implementation ChiptuneVoicesWidget

+ (NSString *)widgetType {
    return @"ChiptuneVoices";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }
    
    _deps = deps;

    _viewController = [ChiptuneVoicesViewController new];
    _viewController.delegate = self;
    _voices = deadbeef->conf_get_int ("chip.voices", 0xff);

    _viewController.view.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:_viewController.view];

    [NSLayoutConstraint activateConstraints:@[
        [_viewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor],
        [_viewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor],
        [_viewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor],
        [_viewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor],
    ]];

    return self;
}

- (void)updateView {
    for (int i = 0; i < 8; i++) {
        [_viewController setVoice:i enabled:(_voices&(1<<i)) != 0];
    }
}

#pragma mark - Overrides

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_CONFIGCHANGED) {
        int voices = deadbeef->conf_get_int ("chip.voices", 0xff);
        if (voices != _voices) {
            _voices = voices;
            [self updateView];
        }
    }
}

#pragma mark - ChiptuneVoicesViewControllerDelegate

- (void)voicesViewDidLoad {
    [self updateView];
}

- (void)voiceDidToggle:(NSInteger)voice {
    _voices ^= (1 << voice);
    deadbeef->conf_set_int ("chip.voices", _voices);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
