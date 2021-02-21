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

@property (nonatomic) NSSplitView *view;

@end

@implementation SplitterWidget

- (instancetype)init {
    self = [super init];
    if (self == nil) {
        return nil;
    }

    self.view = [[NSSplitView alloc] initWithFrame:NSZeroRect];

    return self;
}

- (nonnull NSString *)serializedString { 
    return @"{}";
}

@end
