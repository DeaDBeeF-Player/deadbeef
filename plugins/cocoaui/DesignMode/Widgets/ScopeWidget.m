//
//  ScopeWidget.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 18/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "ScopeVisualizationViewController.h"
#import "ScopeSettings.h"
#import "ScopeWidget.h"
#import "VisualizationSettingsUtil.h"

static void *kRenderModeContext = &kRenderModeContext;
static void *kScaleModeContext = &kScaleModeContext;
static void *kFragmentDurationContext = &kFragmentDurationContext;
static void *kUseCustomColorContext = &kUseCustomColorContext;
static void *kCustomColorContext = &kCustomColorContext;
static void *kUseCustomBackgroundColorContext = &kUseCustomBackgroundColorContext;
static void *kCustomBackgroundColorContext = &kCustomBackgroundColorContext;

@interface ScopeWidget()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic) ScopeVisualizationViewController *visualizationViewController;
@property (nonatomic) ScopeSettings *settings;

@end

@implementation ScopeWidget

+ (NSString *)widgetType {
    return @"Scope";
}

- (void)dealloc {
    [self.settings removeObserver:self forKeyPath:@"renderMode"];
    [self.settings removeObserver:self forKeyPath:@"scaleMode"];
    [self.settings removeObserver:self forKeyPath:@"fragmentDuration"];
    [self.settings removeObserver:self forKeyPath:@"useCustomColor"];
    [self.settings removeObserver:self forKeyPath:@"customColor"];
    [self.settings removeObserver:self forKeyPath:@"useCustomBackgroundColor"];
    [self.settings removeObserver:self forKeyPath:@"customBackgroundColor"];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _deps = deps;

    _visualizationViewController = [ScopeVisualizationViewController new];

    [self.topLevelView addSubview:_visualizationViewController.view];
    [_visualizationViewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_visualizationViewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_visualizationViewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_visualizationViewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    _settings = [ScopeSettings new];
    [_settings addObserver:self forKeyPath:@"renderMode" options:0 context:kRenderModeContext];
    [_settings addObserver:self forKeyPath:@"scaleMode" options:0 context:kScaleModeContext];
    [_settings addObserver:self forKeyPath:@"fragmentDuration" options:0 context:kFragmentDurationContext];
    [_settings addObserver:self forKeyPath:@"useCustomColor" options:0 context:kUseCustomColorContext];
    [_settings addObserver:self forKeyPath:@"customColor" options:0 context:kCustomColorContext];
    [_settings addObserver:self forKeyPath:@"useCustomBackgroundColor" options:0 context:kUseCustomBackgroundColorContext];
    [_settings addObserver:self forKeyPath:@"customBackgroundColor" options:0 context:kCustomBackgroundColorContext];

    _visualizationViewController.settings = _settings;
    [_visualizationViewController updateScopeSettings:_settings];

    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    // update the visualization view and save settings
    if (context == kRenderModeContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kScaleModeContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kFragmentDurationContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kUseCustomColorContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kCustomColorContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kUseCustomBackgroundColorContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kCustomBackgroundColorContext) {
        [self.visualizationViewController updateScopeSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark - Overrides

- (NSDictionary *)serializedSettingsDictionary {
    NSString *renderMode;
    switch (self.settings.renderMode) {
    case DDB_SCOPE_MONO:
        renderMode = @"mono";
        break;
    case DDB_SCOPE_MULTICHANNEL:
        renderMode = @"multichannel";
        break;
    }

    NSString *scaleMode;
    switch (self.settings.scaleMode) {
    case ScopeScaleModeAuto:
        scaleMode = @"auto";
        break;
    case ScopeScaleMode1x:
        scaleMode = @"1";
        break;
    case ScopeScaleMode2x:
        scaleMode = @"2";
        break;
    case ScopeScaleMode3x:
        scaleMode = @"3";
        break;
    case ScopeScaleMode4x:
        scaleMode = @"4";
        break;
    }

    NSString *fragmentDuration;
    switch (self.settings.fragmentDuration) {
    case ScopeFragmentDuration50:
        fragmentDuration = @"50";
        break;
    case ScopeFragmentDuration100:
        fragmentDuration = @"100";
        break;
    case ScopeFragmentDuration200:
        fragmentDuration = @"200";
        break;
    case ScopeFragmentDuration300:
        fragmentDuration = @"300";
        break;
    case ScopeFragmentDuration500:
        fragmentDuration = @"500";
        break;
    }

    NSMutableDictionary *d = [NSMutableDictionary new];

    d[@"renderMode"] = renderMode;
    d[@"scaleMode"] = scaleMode;
    d[@"fragmentDuration"] = fragmentDuration;
    d[@"useCustomColor"] = @(self.settings.useCustomColor);
    NSString *customColor = [VisualizationSettingsUtil.shared stringForColor:self.settings.customColor];
    if (customColor != nil) {
        d[@"customColor"] = customColor;
    }

    d[@"useCustomBackgroundColor"] = @(self.settings.useCustomBackgroundColor);
    NSString *customBackgroundColor = [VisualizationSettingsUtil.shared stringForColor:self.settings.customBackgroundColor];
    if (customBackgroundColor != nil) {
        d[@"customBackgroundColor"] = customBackgroundColor;
    }

    return d.copy;
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    // deserialize
    NSString *renderModeString = dictionary[@"renderMode"];
    if ([renderModeString isKindOfClass:NSString.class]) {
        if ([renderModeString isEqualToString:@"mono"]) {
            self.settings.renderMode = DDB_SCOPE_MONO;
        }
        else if ([renderModeString isEqualToString:@"multichannel"]) {
            self.settings.renderMode = DDB_SCOPE_MULTICHANNEL;
        }
    }

    NSString *scaleModeString = dictionary[@"scaleMode"];
    if ([scaleModeString isKindOfClass:NSString.class]) {
        if ([scaleModeString isEqualToString:@"auto"]) {
            self.settings.scaleMode = ScopeScaleModeAuto;
        }
        else if ([scaleModeString isEqualToString:@"1"]) {
            self.settings.scaleMode = ScopeScaleMode1x;
        }
        else if ([scaleModeString isEqualToString:@"2"]) {
            self.settings.scaleMode = ScopeScaleMode2x;
        }
        else if ([scaleModeString isEqualToString:@"3"]) {
            self.settings.scaleMode = ScopeScaleMode3x;
        }
        else if ([scaleModeString isEqualToString:@"4"]) {
            self.settings.scaleMode = ScopeScaleMode4x;
        }
    }

    NSString *fragmentDurationString = dictionary[@"fragmentDuration"];
    if ([fragmentDurationString isKindOfClass:NSString.class]) {
        if ([fragmentDurationString isEqualToString:@"50"]) {
            self.settings.fragmentDuration = ScopeFragmentDuration50;
        }
        else if ([fragmentDurationString isEqualToString:@"100"]) {
            self.settings.fragmentDuration = ScopeFragmentDuration100;
        }
        else if ([fragmentDurationString isEqualToString:@"200"]) {
            self.settings.fragmentDuration = ScopeFragmentDuration200;
        }
        else if ([fragmentDurationString isEqualToString:@"300"]) {
            self.settings.fragmentDuration = ScopeFragmentDuration300;
        }
        else if ([fragmentDurationString isEqualToString:@"500"]) {
            self.settings.fragmentDuration = ScopeFragmentDuration500;
        }
    }

    NSNumber *useCustomColorNumber = dictionary[@"useCustomColor"];
    if ([useCustomColorNumber isKindOfClass:NSNumber.class]) {
        self.settings.useCustomColor = useCustomColorNumber.boolValue;
    }

    NSString *customColorString = dictionary[@"customColor"];
    if ([customColorString isKindOfClass:NSString.class]) {
        self.settings.customColor = [VisualizationSettingsUtil.shared colorForString:customColorString];
    }

    NSNumber *useCustomBackgroundColorNumber = dictionary[@"useCustomBackgroundColor"];
    if ([useCustomBackgroundColorNumber isKindOfClass:NSNumber.class]) {
        self.settings.useCustomBackgroundColor = useCustomBackgroundColorNumber.boolValue;
    }

    NSString *customBackgroundColorString = dictionary[@"customBackgroundColor"];
    if ([customBackgroundColorString isKindOfClass:NSString.class]) {
        self.settings.customBackgroundColor = [VisualizationSettingsUtil.shared colorForString:customBackgroundColorString];
    }

    return YES;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.visualizationViewController message:_id ctx:ctx p1:p1 p2:p2];
}

@end
