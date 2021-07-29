//
//  SpectrumAnalyzerWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "SpectrumAnalyzerWidget.h"
#import "VisualizationViewController.h"
#import "VisualizationView.h"
#import "SpectrumAnalyzerSettings.h"

static void *kModeContext = &kModeContext;
static void *kDistanceBetweenBarsContext = &kDistanceBetweenBarsContext;
static void *kBarGranularity = &kBarGranularity;

@interface SpectrumAnalyzerWidget()

@property (nonatomic) id<DesignModeDepsProtocol> deps;
@property (nonatomic) VisualizationViewController *visualizationViewController;
@property (nonatomic) VisualizationView *visualizationView;
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
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _deps = deps;

    _settings = [SpectrumAnalyzerSettings new];

    _visualizationViewController = [VisualizationViewController new];
    _visualizationView = [[VisualizationView alloc] initWithFrame:NSZeroRect];
    _visualizationViewController.view = _visualizationView;
    _visualizationViewController.view.translatesAutoresizingMaskIntoConstraints = NO;
    [_visualizationViewController awakeFromNib];

    [self.topLevelView addSubview:_visualizationViewController.view];
    [_visualizationViewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_visualizationViewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_visualizationViewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_visualizationViewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    [_settings addObserver:self forKeyPath:@"mode" options:0 context:kModeContext];
    [_settings addObserver:self forKeyPath:@"distanceBetweenBars" options:0 context:kDistanceBetweenBarsContext];
    [_settings addObserver:self forKeyPath:@"barGranularity" options:0 context:kBarGranularity];

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

    return @{
        @"mode": mode,
        @"distanceBetweenBars": @(self.settings.distanceBetweenBars),
        @"barGranularity": @(self.settings.barGranularity),
    };
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

    return YES;
}


@end
