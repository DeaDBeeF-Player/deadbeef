/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Custom view base class
*/

#import "AAPLView.h"

@implementation AAPLView

///////////////////////////////////////
#pragma mark - Initialization and Setup
///////////////////////////////////////

- (instancetype) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self)
    {
        [self initCommon];
    }
    return self;
}

- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [self initCommon];
    }
    return self;
}

- (void)initCommon
{
    _metalLayer = (CAMetalLayer*) self.layer;

    self.layer.delegate = self;
}

//////////////////////////////////
#pragma mark - Render Loop Control
//////////////////////////////////

#if ANIMATION_RENDERING

- (void)stopRenderLoop
{
    // Stubbed out method.  Subclasses need to implement this method.
}

- (void)dealloc
{
    [self stopRenderLoop];
}

#else // IF !ANIMATION_RENDERING

// Override methods needed to handle event-based rendering

- (void)displayLayer:(CALayer *)layer
{
    [self renderOnEvent];
}

- (void)drawLayer:(CALayer *)layer
        inContext:(CGContextRef)ctx
{
    [self renderOnEvent];
}

- (void)drawRect:(CGRect)rect
{
    [self renderOnEvent];
}

- (void)renderOnEvent
{
    AAPLViewParams params = {
        .backingScaleFactor = self.window.backingScaleFactor,
        .isVisible = self.window.visible,
        .bounds = self.bounds
    };

#if RENDER_ON_MAIN_THREAD
    [self renderWithViewParams:params];
#else
    // Dispatch rendering on a concurrent queue
    dispatch_queue_t globalQueue = dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0);
    dispatch_async(globalQueue, ^(){
        [self renderWithViewParams:params];
    });
#endif
}

#endif // END !ANIMAITON_RENDERING
///////////////////////
#pragma mark - Resizing
///////////////////////

#if AUTOMATICALLY_RESIZE

- (void)resizeDrawable:(CGFloat)scaleFactor
{
    CGSize newSize = self.bounds.size;
    newSize.width *= scaleFactor;
    newSize.height *= scaleFactor;

    if(newSize.width <= 0 || newSize.height <= 0)
    {
        return;
    }

#if RENDER_ON_MAIN_THREAD

    _metalLayer.drawableSize = newSize;

    [_delegate drawableResize:newSize];
    
#else
    // All AppKit and UIKit calls which notify of a resize are called on the main thread.  Use
    // a synchronized block to ensure that resize notifications on the delegate are atomic
    @synchronized(_metalLayer)
    {
        _metalLayer.drawableSize = newSize;

        [_delegate drawableResize:newSize];
    }
#endif
}

#endif

//////////////////////
#pragma mark - Drawing
//////////////////////

- (void)renderWithViewParams:(AAPLViewParams)params
{
#if RENDER_ON_MAIN_THREAD
    [_delegate renderToMetalLayer:_metalLayer viewParams:params];
#else
    // Must synchronize if rendering on background thread to ensure resize operations from the
    // main thread are complete before rendering which depends on the size occurs.
    @synchronized(_metalLayer)
    {
        [_delegate renderToMetalLayer:_metalLayer viewParams:params];
    }
#endif
}

@end
