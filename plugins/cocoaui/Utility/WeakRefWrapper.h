//
//  WeakRefWrapper.h
//  ddbcore
//
//  Created by Alexey Yakovenko on 27/01/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface WeakRefWrapper : NSObject

@property (nonatomic,weak,readonly) id object;
- (instancetype)initWithObject:(id)object;

@end

NS_ASSUME_NONNULL_END
