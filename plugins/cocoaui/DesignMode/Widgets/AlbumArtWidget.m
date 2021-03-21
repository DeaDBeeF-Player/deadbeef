//
//  AlbumArtWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 21/03/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "AlbumArtWidget.h"

@interface AlbumArtWidget()

@property (nonatomic) NSImageView *imageView;

@end

@implementation AlbumArtWidget

+ (NSString *)widgetType {
    return @"AlbumArt";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    // create view
    self.imageView = [NSImageView new];
    self.imageView.image = [NSImage imageNamed:@"noartwork.png"];


    // add view
    [self.topLevelView addSubview:self.imageView];

    // constrain view
    self.imageView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.imageView.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [self.imageView.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [self.imageView.topAnchor constraintEqualToAnchor:self.topLevelView.topAnchor].active = YES;
    [self.imageView.bottomAnchor constraintEqualToAnchor:self.topLevelView.bottomAnchor].active = YES;

    return self;
}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
}

@end
