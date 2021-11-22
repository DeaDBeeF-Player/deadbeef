//
//  HolderWidget.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 22/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "HolderWidget.h"

@interface HolderWidget()

@property (nonatomic) NSString *originalTypeName;
@property (nonatomic,copy) NSDictionary *settings;

@end

@implementation HolderWidget

+ (NSString *)widgetType {
    return @"Holder";
}

- (instancetype)initWithDeps:(id<DesignModeDepsProtocol>)deps originalTypeName:(NSString *)originalTypeName {
    self = [super initWithDeps:deps];
    if (self == nil) {
        return nil;
    }

    self.originalTypeName = originalTypeName;

    // create view
    NSTextField *textField = [[NSTextField alloc] initWithFrame:NSZeroRect];

    textField.bordered = NO;
    textField.editable = NO;
    textField.selectable = NO;
    textField.drawsBackground = NO;
    textField.alignment = NSTextAlignmentCenter;
    [textField setContentCompressionResistancePriority:NSLayoutPriorityDefaultLow forOrientation:NSLayoutConstraintOrientationHorizontal];

    textField.stringValue = [NSString stringWithFormat:@"Widget of type %@ is not available", originalTypeName];

    [self.topLevelView addSubview:textField];

    // constrain view
    textField.translatesAutoresizingMaskIntoConstraints = NO;
    [textField.leadingAnchor constraintEqualToAnchor:self.topLevelView.leadingAnchor].active = YES;
    [textField.trailingAnchor constraintEqualToAnchor:self.topLevelView.trailingAnchor].active = YES;
    [textField.centerYAnchor constraintEqualToAnchor:self.topLevelView.centerYAnchor].active = YES;

    return self;
}

- (NSDictionary *)serializedRootDictionary {
    return self.settings;
}

- (void)deserializeFromRootDictionary:(NSDictionary *)dictionary {
    self.settings = dictionary;
}

- (NSString *)displayName {
    NSString *type = self.settings[@"type"];
    if (type != nil) {
        return [NSString stringWithFormat:@"Unavailable: %@", type];
    }
    else {
        return @"-";
    }
}

@end
