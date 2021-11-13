//
//  VisBaseColorUtil.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 13/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisBaseColorUtil : NSObject

+ (instancetype)shared;
@property (nonatomic,readonly) NSColor *baseColor;

@end

NS_ASSUME_NONNULL_END
