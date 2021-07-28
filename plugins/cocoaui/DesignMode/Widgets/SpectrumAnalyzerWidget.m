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

    _settings = [SpectrumAnalyzerSettings new];

    NSString *modeString = self.serializedSettingsDictionary[@"mode"];
    if ([modeString isKindOfClass:NSString.class]) {
        if ([modeString isEqualToString:@"frequencies"]) {
            _settings.mode = DDB_ANALYZER_MODE_FREQUENCIES;
        }
        else if ([modeString isEqualToString:@"bars"]) {
            _settings.mode = DDB_ANALYZER_MODE_FREQUENCIES;
        }
    }

    NSNumber *distanceBetweenBarsNumber = self.serializedSettingsDictionary[@"distanceBetweenBars"];
    if ([distanceBetweenBarsNumber isKindOfClass:NSNumber.class]) {
        _settings.distanceBetweenBars = distanceBetweenBarsNumber.intValue;
    }

    NSNumber *barGranularityNumber = self.serializedSettingsDictionary[@"barGranularity"];
    if ([barGranularityNumber isKindOfClass:NSNumber.class]) {
        _settings.barGranularity = barGranularityNumber.intValue;
    }


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

    _visualizationViewController.settings = _settings;
    [_visualizationView updateAnalyzerSettings:_settings];

    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    // update the visualization view and save settings
    if (context == kModeContext) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
    } else if (context == kDistanceBetweenBarsContext) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
    } else if (context == kBarGranularity) {
        [self.visualizationView updateAnalyzerSettings:self.settings];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

@end
