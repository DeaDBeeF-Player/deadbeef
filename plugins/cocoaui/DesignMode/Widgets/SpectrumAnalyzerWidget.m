//
//  SpectrumAnalyzerWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "SpectrumAnalyzerWidget.h"
#import "SpectrumAnalyzerVisualizationViewController.h"
#import "SpectrumAnalyzerVisualizationView.h"
#import "SpectrumAnalyzerSettings.h"
#import "VisualizationSettingsUtil.h"

static void *kModeContext = &kModeContext;
static void *kDistanceBetweenBarsContext = &kDistanceBetweenBarsContext;
static void *kBarGranularity = &kBarGranularity;
static void *kUseCustomPeakColor = &kUseCustomPeakColor;
static void *kUseCustomBarColor = &kUseCustomBarColor;
static void *kCustomPeakColor = &kCustomPeakColor;
static void *kCustomBarColor = &kCustomBarColor;

@interface SpectrumAnalyzerWidget()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic) SpectrumAnalyzerVisualizationViewController *visualizationViewController;
@property (nonatomic) SpectrumAnalyzerVisualizationView *visualizationView;
@property (nonatomic) SpectrumAnalyzerSettings *settings;

@end

@implementation SpectrumAnalyzerWidget

+ (NSString *)widgetType {
    return @"SpectrumAnalyzer";
}

- (void)dealloc {
    [self.settings removeObserver:self forKeyPath:@"mode"];
    [self.settings removeObserver:self forKeyPath:@"distanceBetweenBars"];
    [self.settings removeObserver:self forKeyPath:@"barGranularity"];
    [self.settings removeObserver:self forKeyPath:@"useCustomPeakColor"];
    [self.settings removeObserver:self forKeyPath:@"useCustomBarColor"];
    [self.settings removeObserver:self forKeyPath:@"customPeakColor"];
    [self.settings removeObserver:self forKeyPath:@"customBarColor"];
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _deps = deps;

    _visualizationViewController = [SpectrumAnalyzerVisualizationViewController new];
    _visualizationView = [[SpectrumAnalyzerVisualizationView alloc] initWithFrame:NSZeroRect];
    _visualizationViewController.view = _visualizationView;
    _visualizationViewController.view.translatesAutoresizingMaskIntoConstraints = NO;
    [_visualizationViewController awakeFromNib];

    [self.topLevelView addSubview:_visualizationViewController.view];
    [_visualizationViewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_visualizationViewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_visualizationViewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_visualizationViewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    _settings = [SpectrumAnalyzerSettings new];
    [_settings addObserver:self forKeyPath:@"mode" options:0 context:kModeContext];
    [_settings addObserver:self forKeyPath:@"distanceBetweenBars" options:0 context:kDistanceBetweenBarsContext];
    [_settings addObserver:self forKeyPath:@"barGranularity" options:0 context:kBarGranularity];

    [_settings addObserver:self forKeyPath:@"useCustomPeakColor" options:0 context:kUseCustomPeakColor];
    [_settings addObserver:self forKeyPath:@"useCustomBarColor" options:0 context:kUseCustomBarColor];
    [_settings addObserver:self forKeyPath:@"customPeakColor" options:0 context:kCustomPeakColor];
    [_settings addObserver:self forKeyPath:@"customBarColor" options:0 context:kCustomBarColor];

    self.settings.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
    self.settings.barGranularity = 1;
    self.settings.distanceBetweenBars = 3;

    _visualizationViewController.settings = _settings;
    [_visualizationView updateAnalyzerSettings:_settings];

    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    // update the visualization view and save settings
    if (context == kModeContext) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kDistanceBetweenBarsContext) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kBarGranularity) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kUseCustomPeakColor) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kUseCustomBarColor) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kCustomPeakColor) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else if (context == kCustomBarColor) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
        [self.deps.state layoutDidChange];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark - Overrides

- (NSDictionary *)serializedSettingsDictionary {
    NSString *mode;
    switch (self.settings.mode) {
    case DDB_ANALYZER_MODE_FREQUENCIES:
        mode = @"frequencies";
        break;
    case DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS:
        mode = @"bands";
        break;
    }

    NSMutableDictionary *d = [NSMutableDictionary new];
    d[@"mode"] = mode;
    d[@"distanceBetweenBars"] = @(self.settings.distanceBetweenBars);
    d[@"barGranularity"] = @(self.settings.barGranularity);
    d[@"useCustomPeakColor"] = @(self.settings.useCustomPeakColor);
    d[@"useCustomBarColor"] = @(self.settings.useCustomBarColor);
    NSString *customPeakColor = [VisualizationSettingsUtil.shared stringForColor:self.settings.customPeakColor];
    if (customPeakColor != nil) {
        d[@"customPeakColor"] = customPeakColor;
    }

    NSString *customBarColor = [VisualizationSettingsUtil.shared stringForColor:self.settings.customBarColor];
    if (customBarColor) {
        d[@"customBarColor"] = customBarColor;
    }
    return d.copy;
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    // deserialize
    NSString *modeString = dictionary[@"mode"];
    if ([modeString isKindOfClass:NSString.class]) {
        if ([modeString isEqualToString:@"frequencies"]) {
            self.settings.mode = DDB_ANALYZER_MODE_FREQUENCIES;
        }
        else if ([modeString isEqualToString:@"bars"]) {
            self.settings.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
        }
    }

    NSNumber *distanceBetweenBarsNumber = dictionary[@"distanceBetweenBars"];
    if ([distanceBetweenBarsNumber isKindOfClass:NSNumber.class]) {
        self.settings.distanceBetweenBars = distanceBetweenBarsNumber.intValue;
    }

    NSNumber *barGranularityNumber = dictionary[@"barGranularity"];
    if ([barGranularityNumber isKindOfClass:NSNumber.class]) {
        self.settings.barGranularity = barGranularityNumber.intValue;
    }

    NSNumber *useCustomPeakColorNumber = dictionary[@"useCustomPeakColor"];
    if ([useCustomPeakColorNumber isKindOfClass:NSNumber.class]) {
        self.settings.useCustomPeakColor = useCustomPeakColorNumber.boolValue;
    }

    NSNumber *useCustomBarColorNumber = dictionary[@"useCustomBarColor"];
    if ([useCustomBarColorNumber isKindOfClass:NSNumber.class]) {
        self.settings.useCustomBarColor = useCustomBarColorNumber.boolValue;
    }

    NSString *customPeakColorString = dictionary[@"customPeakColor"];
    if ([customPeakColorString isKindOfClass:NSString.class]) {
        self.settings.customPeakColor = [VisualizationSettingsUtil.shared colorForString:customPeakColorString];
    }

    NSString *customBarColorString = dictionary[@"customBarColor"];
    if ([customBarColorString isKindOfClass:NSString.class]) {
        self.settings.customBarColor = [VisualizationSettingsUtil.shared colorForString:customBarColorString];
    }

    return YES;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    [self.visualizationViewController message:_id ctx:ctx p1:p1 p2:p2];
}

@end
