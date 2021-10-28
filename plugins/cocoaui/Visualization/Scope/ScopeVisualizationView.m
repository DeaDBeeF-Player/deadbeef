//
//  ScopeVisualizationView.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 18/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#import "ScopeShaderTypes.h"
#import "ScopeVisualizationView.h"
#include "deadbeef.h"
#include "scope.h"

extern DB_functions_t *deadbeef;

static NSString * const kWindowIsVisibleKey = @"window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface ScopeVisualizationView() <MTKViewDelegate>

@property (nonatomic) BOOL isListening;
@property (nonatomic,readonly) NSColor *baseColor;

@end

@implementation ScopeVisualizationView {
    ddb_audio_data_t _input_data;
    ddb_waveformat_t _fmt;
    ddb_scope_t _scope;
    ddb_scope_draw_data_t _draw_data;
    id<MTLCommandQueue> _commandQueue;
    id<MTLRenderPipelineState> _pipelineState;
    CGSize _viewportSize;
}

static void vis_callback (void *ctx, const ddb_audio_data_t *data) {
    ScopeVisualizationView *view = (__bridge ScopeVisualizationView *)(ctx);
    [view updateScopeData:data];
}

- (void)dealloc {
    [self removeObserver:self forKeyPath:kWindowIsVisibleKey];
    if (self.isListening) {
        deadbeef->vis_waveform_unlisten ((__bridge void *)(self));
        self.isListening = NO;
    }
    ddb_scope_dealloc(&_scope);
    ddb_scope_draw_data_dealloc(&_draw_data);
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil) {
        return nil;
    }

    [self setup];

    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self == nil) {
        return nil;
    }

    [self setup];

    return self;
}

- (void)setup {
    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kIsVisibleContext];
    _isListening = NO;
    _input_data.fmt = &_fmt;
    ddb_scope_init(&_scope);
    _scope.mode = DDB_SCOPE_MULTICHANNEL;

    self.device = MTLCreateSystemDefaultDevice();

    // Load all the shader files with a .metal file extension in the project.
    id<MTLLibrary> defaultLibrary = [self.device newDefaultLibrary];

    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];

    // Configure a pipeline descriptor that is used to create a pipeline state.
    MTLRenderPipelineDescriptor *pipelineStateDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineStateDescriptor.label = @"Simple Pipeline";
    pipelineStateDescriptor.vertexFunction = vertexFunction;
    pipelineStateDescriptor.fragmentFunction = fragmentFunction;
    pipelineStateDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;

    NSError *error;
    _pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error];
    NSAssert(_pipelineState, @"Failed to create pipeline state: %@", error);

    _commandQueue = [self.device newCommandQueue];
    self.clearColor = MTLClearColorMake(0.0, 0.5, 1.0, 1.0);
    self.enableSetNeedsDisplay = YES;
    self.delegate = self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if (context == kIsVisibleContext) {
        if (!self.isListening && self.window.isVisible) {
            deadbeef->vis_waveform_listen((__bridge void *)(self), vis_callback);
            self.isListening = YES;
        }
        else if (self.isListening && !self.window.isVisible) {
            deadbeef->vis_waveform_unlisten ((__bridge void *)(self));
            self.isListening = NO;
        }
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (BOOL)updateDrawData {
    // for some reason KVO is not triggered when the window becomes hidden
    if (self.isListening && !self.window.isVisible) {
        deadbeef->vis_waveform_unlisten ((__bridge void *)(self));
        self.isListening = NO;
        return NO;
    }

    if (_input_data.nframes == 0) {
        return NO;
    }

    @synchronized (self) {
        ddb_scope_process(&_scope, _input_data.fmt->samplerate, _input_data.fmt->channels, _input_data.data, _input_data.nframes);
        ddb_scope_tick(&_scope);
        ddb_scope_get_draw_data(&_scope, (int)self.bounds.size.width, (int)self.bounds.size.height, &_draw_data);
    }

    return YES;
}

#if 0
- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    if (![self updateDrawData]) {
        return;
    }

    [NSColor.blackColor setFill];
    NSRectFill(dirtyRect);

    CGContextRef context = NSGraphicsContext.currentContext.CGContext;

    int output_channels = _scope.mode == DDB_SCOPE_MONO ? 1 : _scope.channels;

    ddb_scope_point_t *point = _draw_data.points;
    for (int ch = 0; ch < output_channels; ch++) {
        for (int i = 0; i < _draw_data.point_count; i++, point++) {
            CGContextMoveToPoint(context, i, point->ymin);
            CGContextAddLineToPoint(context, i, point->ymax);
        }
    }

    CGContextSetShouldAntialias(context, false);
    CGContextSetStrokeColorWithColor(context, self.baseColor.CGColor);
    CGContextStrokePath(context);
}
#endif

- (void)updateScopeData:(const ddb_audio_data_t *)data {
    @synchronized (self) {
        // copy the input data for later consumption
        if (_input_data.nframes != data->nframes) {
            free (_input_data.data);
            _input_data.data = malloc (data->nframes * data->fmt->channels * sizeof (float));
            _input_data.nframes = data->nframes;
        }
        memcpy (_input_data.fmt, data->fmt, sizeof (ddb_waveformat_t));
        memcpy (_input_data.data, data->data, data->nframes * data->fmt->channels * sizeof (float));
    }
}

- (NSColor *)baseColor {
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        return NSColor.controlAccentColor;
    }
#endif
    return NSColor.alternateSelectedControlColor;
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    _viewportSize = size;
}

- (void)drawInMTKView:(MTKView *)view {
    if (![self updateDrawData]) {
        return;
    }

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
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    commandBuffer.label = @"MyCommand";

    // Obtain a renderPassDescriptor generated from the view's drawable textures.
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;

    if(renderPassDescriptor != nil)
    {
        // Create a render command encoder.
        id<MTLRenderCommandEncoder> renderEncoder =
        [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        renderEncoder.label = @"MyRenderEncoder";

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
        params.point_count = _draw_data.point_count;
        params.channels = _scope.mode == DDB_SCOPE_MONO ? 1 : _scope.channels;
        [renderEncoder setFragmentBytes:&params length:sizeof (params) atIndex:0];

        [renderEncoder setFragmentBytes:_draw_data.points length:_draw_data.point_count * sizeof (ddb_scope_point_t) * params.channels atIndex:1];

        // Draw the triangle.
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                          vertexStart:0
                          vertexCount:4];

        [renderEncoder endEncoding];

        // Schedule a present once the framebuffer is complete using the current drawable.
        [commandBuffer presentDrawable:view.currentDrawable];
    }

    // Finalize rendering here & push the command buffer to the GPU.
    [commandBuffer commit];
}

@end
