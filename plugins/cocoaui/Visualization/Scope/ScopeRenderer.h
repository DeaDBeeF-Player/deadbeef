//
//  ScopeRenderer.h
//  deadbeef
//
//  Created by Alexey Yakovenko on 04/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "scope.h"

NS_ASSUME_NONNULL_BEGIN

@interface ScopeRenderer : NSObject

- (nonnull instancetype)initWithMetalDevice:(nonnull id<MTLDevice>)device
                        drawablePixelFormat:(MTLPixelFormat)drawabklePixelFormat;

- (void)renderToMetalLayer:(nonnull CAMetalLayer*)metalLayer drawData:(nonnull ddb_scope_draw_data_t *)drawData scale:(float)scale;

- (void)drawableResize:(CGSize)drawableSize;


@end

NS_ASSUME_NONNULL_END
