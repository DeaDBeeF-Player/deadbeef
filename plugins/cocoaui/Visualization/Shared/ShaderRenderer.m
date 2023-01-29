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

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import "ShaderRenderer.h"
#import "ShaderRendererTypes.h"

@implementation ShaderRenderer {
    // renderer global ivars
    id <MTLDevice>              _device;
    id <MTLCommandQueue>        _commandQueue;
    id <MTLRenderPipelineState> _pipelineState;

    // Render pass descriptor which creates a render command encoder to draw to the drawable
    // textures
    MTLRenderPassDescriptor *_drawableRenderDescriptor;

    CGSize _viewportSize;

    NSUInteger _frameNum;
}

- (nonnull instancetype)initWithMetalDevice:(nonnull id<MTLDevice>)device
                        drawablePixelFormat:(MTLPixelFormat)drawablePixelFormat
                         fragmentShaderName:(NSString *)fragmentShaderName
{
    self = [super init];
    if (self)
    {
        _frameNum = 0;

        _device = device;

        _commandQueue = [_device newCommandQueue];

        _drawableRenderDescriptor = [MTLRenderPassDescriptor new];
        _drawableRenderDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
        _drawableRenderDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        _drawableRenderDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 1, 1, 1);

        {
            id<MTLLibrary> shaderLib = [_device newDefaultLibrary];
            if(!shaderLib)
            {
                NSLog(@" ERROR: Couldnt create a default shader library");
                // assert here because if the shader libary isn't loading, nothing good will happen
                return nil;
            }

            id <MTLFunction> vertexProgram = [shaderLib newFunctionWithName:@"vertexShader"];
            if(!vertexProgram)
            {
                NSLog(@">> ERROR: Couldn't load vertex function from default library");
                return nil;
            }

            id <MTLFunction> fragmentProgram = [shaderLib newFunctionWithName:fragmentShaderName];
            if(!fragmentProgram)
            {
                NSLog(@" ERROR: Couldn't load fragment function from default library");
                return nil;
            }

            // Create a pipeline state descriptor to create a compiled pipeline state object
            MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];

            pipelineDescriptor.label                           = @"MyPipeline";
            pipelineDescriptor.vertexFunction                  = vertexProgram;
            pipelineDescriptor.fragmentFunction                = fragmentProgram;
            pipelineDescriptor.colorAttachments[0].pixelFormat = drawablePixelFormat;

            NSError *error;
            _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                     error:&error];
            if(!_pipelineState)
            {
                NSLog(@"ERROR: Failed aquiring pipeline state: %@", error);
                return nil;
            }
        }
    }
    return self;
}


- (void)drawableResize:(CGSize)drawableSize
{
    _viewportSize = drawableSize;
}

- (void)renderToMetalLayer:(nonnull CAMetalLayer*)metalLayer viewParams:(AAPLViewParams)viewParams {
    if (_viewportSize.width == 0 || _viewportSize.height == 0) {
        return;
    }

    _frameNum++;

    // Create a new command buffer for each render pass to the current drawable.
    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];

    id<CAMetalDrawable> currentDrawable = [metalLayer nextDrawable];

    // If the current drawable is nil, skip rendering this frame
    if(!currentDrawable)
    {
        return;
    }

    _drawableRenderDescriptor.colorAttachments[0].texture = currentDrawable.texture;

    id <MTLRenderCommandEncoder> renderEncoder =
    [commandBuffer renderCommandEncoderWithDescriptor:_drawableRenderDescriptor];
    renderEncoder.label = @"MyRenderEncoder";

    // begin

    // Upload min/max coordinates into texture

    ShaderRendererVertex quadVertices[] =
    {
        // 2D positions,    RGBA colors
        { {  (float)_viewportSize.width/2, -(float)_viewportSize.height/2 } },
        { { -(float)_viewportSize.width/2, -(float)_viewportSize.height/2 } },
        { {  (float)_viewportSize.width/2,  (float)_viewportSize.height/2 } },
        { { -(float)_viewportSize.width/2,  (float)_viewportSize.height/2 } },
    };

    // Create a new command buffer for each render pass to the current drawable.
    commandBuffer.label = @"MyCommand";

    // Create a render command encoder.

    // Set the region of the drawable to draw into.
    [renderEncoder setViewport:(MTLViewport){0.0, 0.0, _viewportSize.width, _viewportSize.height, 0.0, 1.0 }];

    [renderEncoder setRenderPipelineState:_pipelineState];

    // Pass in the parameter data.
    [renderEncoder setVertexBytes:quadVertices
                           length:sizeof(quadVertices)
                          atIndex:ShaderRendererVertexInputIndexVertices];

    vector_uint2 vp = { (uint)_viewportSize.width, (uint)_viewportSize.height };
    [renderEncoder setVertexBytes:&vp
                           length:sizeof(vp)
                          atIndex:ShaderRendererVertexInputIndexViewportSize];

    [self.delegate applyFragParamsWithViewport:vp device:_device encoder:renderEncoder viewParams:viewParams];

    // Draw the triangle.
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                      vertexStart:0
                      vertexCount:4];

    // end


    [renderEncoder endEncoding];

    [commandBuffer presentDrawable:currentDrawable];

    [commandBuffer commit];
}


@end
