//
//  ScopeWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 18/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "ScopeVisualizationView.h"
#import "VisualizationViewController.h"
#import "ScopeWidget.h"

@interface ScopeWidget()

@property (nonatomic,weak) id<DesignModeDepsProtocol> deps;
@property (nonatomic) VisualizationViewController *visualizationViewController;
@property (nonatomic) ScopeVisualizationView *visualizationView;

@end

@implementation ScopeWidget

+ (NSString *)widgetType {
    return @"Scope";
}

- (void)dealloc {
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _deps = deps;

    _visualizationViewController = [VisualizationViewController new];
    _visualizationView = [[ScopeVisualizationView alloc] initWithFrame:NSZeroRect];
    _visualizationViewController.view = _visualizationView;
    _visualizationViewController.view.translatesAutoresizingMaskIntoConstraints = NO;
    [_visualizationViewController awakeFromNib];

    [self.topLevelView addSubview:_visualizationViewController.view];
    [_visualizationViewController.view.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_visualizationViewController.view.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_visualizationViewController.view.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_visualizationViewController.view.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

#pragma mark - Overrides

- (NSDictionary *)serializedSettingsDictionary {
    return nil;
}

- (BOOL)deserializeFromSettingsDictionary:(NSDictionary *)dictionary {
    return YES;
}

@end
