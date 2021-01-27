//
//  WeakRefWrapper.m
//  ddbcore
//
//  Created by Alexey Yakovenko on 27/01/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "WeakRefWrapper.h"

@interface WeakRefWrapper()

@property (nonatomic,weak) id object;

@end

@implementation WeakRefWrapper

- (instancetype)initWithObject:(id)object {
    self = [super init];
    if (self == nil) {
        return nil;
    }
    self.object = object;
    return self;
}

@end
