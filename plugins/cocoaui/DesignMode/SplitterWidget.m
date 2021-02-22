//
//  SplitterWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 20/02/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "SplitterWidget.h"
#import "WidgetFactory.h"

@interface SplitterWidget()

@property (nonatomic) NSSplitView *splitView;

@property (nonatomic) id<WidgetProtocol> pane1;
@property (nonatomic) id<WidgetProtocol> pane2;

@end

@implementation SplitterWidget

+ (NSString *)widgetType {
    return @"Splitter";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps vertical:(BOOL)vertical {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    _splitView = [[NSSplitView alloc] initWithFrame:NSZeroRect];
    _splitView.vertical = vertical;
    _pane1 = [deps.factory createWidgetWithType:@"Placeholder"];
    _pane2 = [deps.factory createWidgetWithType:@"Placeholder"];

    [super appendChild:_pane1];
    [super appendChild:_pane2];

    [_splitView insertArrangedSubview:_pane1.view atIndex:0];
    [_splitView insertArrangedSubview:_pane2.view atIndex:1];
    _splitView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.topLevelView addSubview:_splitView];
    [_splitView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [_splitView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [_splitView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [_splitView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

@end
