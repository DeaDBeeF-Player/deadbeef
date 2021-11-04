//
//  ScopeRenderer.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 04/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import "ScopeRenderer.h"
#import "ScopeShaderTypes.h"

@interface ScopeRenderer()
@property (nonatomic,readonly) NSColor *baseColor;
@end

@implementation ScopeRenderer
{
    // renderer global ivars
    id <MTLDevice>              _device;
    id <MTLCommandQueue>        _commandQueue;
    id <MTLRenderPipelineState> _pipelineState;
//    id <MTLBuffer>              _vertices;
//    id <MTLTexture>             _depthTarget;

    // Render pass descriptor which creates a render command encoder to draw to the drawable
    // textures
    MTLRenderPassDescriptor *_drawableRenderDescriptor;

    CGSize _viewportSize;

    NSUInteger _frameNum;
}

- (NSColor *)baseColor {
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        return NSColor.controlAccentColor;
    }
#endif
    return NSColor.alternateSelectedControlColor;
}


- (nonnull instancetype)initWithMetalDevice:(nonnull id<MTLDevice>)device
                        drawablePixelFormat:(MTLPixelFormat)drawabklePixelFormat
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

            id <MTLFunction> fragmentProgram = [shaderLib newFunctionWithName:@"fragmentShader"];
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
            pipelineDescriptor.colorAttachments[0].pixelFormat = drawabklePixelFormat;

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

- (void)renderToMetalLayer:(nonnull CAMetalLayer*)metalLayer drawData:(ddb_scope_draw_data_t *)drawData scale:(float)scale
{
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

    ScopeVertex quadVertices[] =
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
                          atIndex:ScopeVertexInputIndexVertices];

    vector_uint2 vp = { (uint)_viewportSize.width, (uint)_viewportSize.height };
    [renderEncoder setVertexBytes:&vp
                           length:sizeof(vp)
                          atIndex:ScopeVertexInputIndexViewportSize];

    NSColor *color = [self.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat components[4];
    [color getComponents:components];

    struct FragParams params;
    params.color = (vector_float4){ (float)components[0], (float)components[1], (float)components[2], 1 };
    params.size.x = vp.x;
    params.size.y = vp.y;
    params.point_count = drawData->point_count;
    params.channels = drawData->mode == DDB_SCOPE_MONO ? 1 : drawData->channels;
    params.scale = scale;
    [renderEncoder setFragmentBytes:&params length:sizeof (params) atIndex:0];

    // Metal documentation states that MTLBuffer should be used for buffers larger than 4K in size.
    // Alternative is to use setFragmentBytes, which also works, but could have compatibility issues on older hardware.
    id<MTLBuffer> buffer = [_device newBufferWithBytes:drawData->points length:drawData->point_count * sizeof (ddb_scope_point_t) * params.channels options:0];

    [renderEncoder setFragmentBuffer:buffer offset:0 atIndex:1];

    // Draw the triangle.
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                      vertexStart:0
                      vertexCount:4];

    // end


    [renderEncoder endEncoding];

    [commandBuffer presentDrawable:currentDrawable];

    [commandBuffer commit];
}

- (void)drawableResize:(CGSize)drawableSize
{
    _viewportSize = drawableSize;
}

@end
