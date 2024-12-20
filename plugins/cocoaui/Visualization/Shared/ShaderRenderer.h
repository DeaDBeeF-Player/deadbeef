/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <simd/simd.h>

typedef struct {
    CGFloat backingScaleFactor;
    BOOL isVisible;
    CGRect bounds;
} ShaderRendererParams;

@protocol ShaderRendererDelegate
- (BOOL)applyFragParamsWithViewport:(vector_uint2)viewport device:(nonnull id <MTLDevice>)device commandBuffer:(nonnull id<MTLCommandBuffer>)commandBuffer encoder:(nonnull id <MTLRenderCommandEncoder>)encoder viewParams:(ShaderRendererParams)params;
@end

@interface ShaderRenderer : NSObject

@property (nullable,weak,nonatomic) id<ShaderRendererDelegate> delegate;

- (nonnull instancetype)initWithMetalDevice:(nonnull id<MTLDevice>)device
                        drawablePixelFormat:(MTLPixelFormat)drawablePixelFormat
                         fragmentShaderName:(nonnull NSString *)fragmentShaderName;

- (void)drawableResize:(CGSize)drawableSize;

- (void)renderToMetalLayer:(nonnull CAMetalLayer *)metalLayer viewParams:(ShaderRendererParams)params;

@end
