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

@interface SpectrumAnalyzerWidget()

@property (nonatomic) VisualizationViewController *visualizationViewController;
@property (nonatomic) VisualizationView *visualizationView;

@end

@implementation SpectrumAnalyzerWidget

+ (NSString *)widgetType {
    return @"SpectrumAnalyzer";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _visualizationViewController = [VisualizationViewController new];
    _visualizationView = [[VisualizationView alloc] initWithFrame:NSZeroRect];
    _visualizationViewController.view = _visualizationView;
    _visualizationViewController.view.translatesAutoresizingMaskIntoConstraints = NO;

    [self.topLevelView addSubview:_visualizationViewController.view];
    [_visualizationViewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_visualizationViewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_visualizationViewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_visualizationViewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

@end
