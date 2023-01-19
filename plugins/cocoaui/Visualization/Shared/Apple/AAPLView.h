/*
See LICENSE folder for this sample’s licensing information.

Abstract:
Custom view base class
*/

#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import "AAPLConfig.h"

#if TARGET_IOS || TARGET_TVOS
@import UIKit;
#else
#import <AppKit/AppKit.h>
#endif

typedef struct {
    CGFloat backingScaleFactor;
    BOOL isVisible;
    CGRect bounds;
} AAPLViewParams;

// Protocol to provide resize and redraw callbacks to a delegate
@protocol AAPLViewDelegate <NSObject>

- (void)drawableResize:(CGSize)size;

- (void)renderToMetalLayer:(nonnull CAMetalLayer *)metalLayer viewParams:(AAPLViewParams)params;

@end

// Metal view base class
#if TARGET_IOS || TARGET_TVOS
@interface AAPLView : UIView <CALayerDelegate>
#else
@interface AAPLView : NSView <CALayerDelegate>
#endif

@property (nonatomic, nonnull, readonly) CAMetalLayer *metalLayer;

@property (nonatomic, getter=isPaused) BOOL paused;

@property (nonatomic, nullable) id<AAPLViewDelegate> delegate;

- (void)initCommon;

#if AUTOMATICALLY_RESIZE
- (void)resizeDrawable:(CGFloat)scaleFactor;
#endif

#if ANIMATION_RENDERING
- (void)stopRenderLoop;
#endif

- (void)renderWithViewParams:(AAPLViewParams)params;

@end
