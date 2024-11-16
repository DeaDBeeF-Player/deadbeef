//
//  ScopeVisualizationViewController.m
//  deadbeef
//
//  Created by Oleksiy Yakovenko on 30/10/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#include <deadbeef/deadbeef.h>
#import <QuartzCore/CAMetalLayer.h>
#import "MetalView.h"
#import "MetalBufferLoop.h"
#import "ScopePreferencesViewController.h"
#import "ScopePreferencesWindowController.h"
#import "ScopeShaderTypes.h"
#import "ScopeVisualizationViewController.h"
#import "ShaderRenderer.h"
#import "ShaderRendererTypes.h"
#import "VisualizationSettingsUtil.h"
#include "scope.h"

extern DB_functions_t *deadbeef;

static NSString * const kWindowIsVisibleKey = @"view.window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface ScopeVisualizationViewController() <MetalViewDelegate, CALayerDelegate, ShaderRendererDelegate>

@property (nonatomic) BOOL isListening;
@property (nonatomic) ScopeScaleMode scaleMode;
@property (nonatomic) NSPopover *preferencesPopover;

@property (nonatomic) NSColor *baseColor;
@property (nonatomic) NSColor *backgroundColor;

@property (atomic) BOOL isVisible;

@property (nonatomic) MetalBufferLoop *bufferLoop;

@end

@implementation ScopeVisualizationViewController {
    ddb_waveformat_t _fmt;
    ddb_scope_t _scope;
    ddb_scope_draw_data_t _draw_data;
    ShaderRenderer *_renderer;
}

static void vis_callback (void *ctx, const ddb_audio_data_t *data) {
    ScopeVisualizationViewController *viewController = (__bridge ScopeVisualizationViewController *)(ctx);
    [viewController updateScopeData:data];
}

- (void)updateScopeData:(const ddb_audio_data_t *)data {
    @synchronized (self) {
        ddb_scope_process(&_scope, data->fmt->samplerate, data->fmt->channels, data->data, data->nframes);
    }
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

- (void)loadView {
    MetalView *metalView = [MetalView new];
    metalView.delegate = self;
    self.view = metalView;
    self.view.wantsLayer = YES;
    self.view.layerContentsRedrawPolicy = NSViewLayerContentsRedrawDuringViewResize;
    [self setupMetalRenderer];
    self.view.translatesAutoresizingMaskIntoConstraints = NO;
    [super loadView];
}

- (void)setupMetalRenderer {
    CAMetalLayer *metalLayer = (CAMetalLayer *)self.view.layer;
    self.view.layer.delegate = self;
    metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    metalLayer.device = MTLCreateSystemDefaultDevice();
    metalLayer.framebufferOnly = YES;

    _renderer = [[ShaderRenderer alloc] initWithMetalDevice:metalLayer.device
                                        drawablePixelFormat:metalLayer.pixelFormat
                                         fragmentShaderName:@"scopeFragmentShader"
    ];
    _renderer.delegate = self;
    self.bufferLoop = [[MetalBufferLoop alloc] initWithMetalDevice:metalLayer.device bufferCount:3];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    NSMenu *menu = [NSMenu new];
    NSMenuItem *renderModeMenuItem = [menu addItemWithTitle:@"Rendering Mode" action:nil keyEquivalent:@""];
    NSMenuItem *scaleModeMenuItem = [menu addItemWithTitle:@"Scale" action:nil keyEquivalent:@""];
    NSMenuItem *fragmentDurationMenuItem = [menu addItemWithTitle:@"Fragment Duration" action:nil keyEquivalent:@""];

    renderModeMenuItem.submenu = [NSMenu new];
    scaleModeMenuItem.submenu = [NSMenu new];
    fragmentDurationMenuItem.submenu = [NSMenu new];

    [renderModeMenuItem.submenu addItemWithTitle:@"Multichannel" action:@selector(setMultichannelRenderingMode:) keyEquivalent:@""];
    [renderModeMenuItem.submenu addItemWithTitle:@"Mono" action:@selector(setMonoRenderingMode:) keyEquivalent:@""];

    [scaleModeMenuItem.submenu addItemWithTitle:@"Auto" action:@selector(setScaleAuto:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"1x" action:@selector(setScale1x:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"2x" action:@selector(setScale2x:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"3x" action:@selector(setScale3x:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"4x" action:@selector(setScale4x:) keyEquivalent:@""];

    [fragmentDurationMenuItem.submenu addItemWithTitle:@"50 ms" action:@selector(setFragmentDuration50ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"100 ms" action:@selector(setFragmentDuration100ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"200 ms" action:@selector(setFragmentDuration200ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"300 ms" action:@selector(setFragmentDuration300ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"500 ms" action:@selector(setFragmentDuration500ms:) keyEquivalent:@""];

    [menu addItem:NSMenuItem.separatorItem];
    [menu addItemWithTitle:@"Preferences" action:@selector(preferences:) keyEquivalent:@""];

    self.view.menu = menu;

    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kIsVisibleContext];
    _isListening = NO;
    ddb_scope_init(&_scope);
    _scope.mode = DDB_SCOPE_MULTICHANNEL;

    [self setupMetalRenderer];
    self.baseColor = VisualizationSettingsUtil.shared.baseColor;
}

- (void)updateVisListening {
    if (!self.isListening && self.isVisible) {
        deadbeef->vis_waveform_listen((__bridge void *)(self), vis_callback);
        self.isListening = YES;
    }
    else if (self.isListening && !self.isVisible) {
        deadbeef->vis_waveform_unlisten ((__bridge void *)(self));
        self.isListening = NO;
    }
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context {
    if (context == kIsVisibleContext) {
        [self updateVisListening];
    }
    else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)updateScopeSettings:(ScopeSettings *)settings {
    self.settings = settings;
    if (_scope.mode != settings.renderMode) {
        _scope.mode_did_change = 1;
    }
    _scope.mode = settings.renderMode;

    self.scaleMode = settings.scaleMode;

    switch (settings.fragmentDuration) {
    case ScopeFragmentDuration50:
        _scope.fragment_duration = 50;
        break;
    case ScopeFragmentDuration100:
        _scope.fragment_duration = 100;
        break;
    case ScopeFragmentDuration200:
        _scope.fragment_duration = 200;
        break;
    case ScopeFragmentDuration300:
        _scope.fragment_duration = 300;
        break;
    case ScopeFragmentDuration500:
        _scope.fragment_duration = 500;
        break;
    }

    [self updateRendererSettings];
}

#pragma mark - Actions

- (void)setMultichannelRenderingMode:(NSMenuItem *)sender {
    self.settings.renderMode = DDB_SCOPE_MULTICHANNEL;
}

- (void)setMonoRenderingMode:(NSMenuItem *)sender {
    self.settings.renderMode = DDB_SCOPE_MONO;
}

- (void)setScaleAuto:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleModeAuto;
}

- (void)setScale1x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode1x;
}

- (void)setScale2x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode2x;
}

- (void)setScale3x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode3x;
}

- (void)setScale4x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode4x;
}

- (void)setFragmentDuration50ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration50;
}

- (void)setFragmentDuration100ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration100;
}

- (void)setFragmentDuration200ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration200;
}

- (void)setFragmentDuration300ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration300;
}

- (void)setFragmentDuration500ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration500;
}

- (void)preferences:(NSMenuItem *)sender {
    if (self.preferencesPopover != nil) {
        [self.preferencesPopover close];
        self.preferencesPopover = nil;
    }

    self.preferencesPopover = [NSPopover new];
    self.preferencesPopover.behavior = NSPopoverBehaviorTransient;

    ScopePreferencesViewController *preferencesViewController = [ScopePreferencesViewController new];
    preferencesViewController.settings = self.settings;
    preferencesViewController.popover = self.preferencesPopover;

    self.preferencesPopover.contentViewController = preferencesViewController;

    [self.preferencesPopover showRelativeToRect:NSZeroRect ofView:self.view preferredEdge:NSRectEdgeMaxY];
}

- (CGFloat)scaleFactorForBackingScaleFactor:(CGFloat)backingScaleFactor {
    switch (self.scaleMode) {
    case ScopeScaleModeAuto:
        return 1;
    case ScopeScaleMode1x:
        return backingScaleFactor;
    case ScopeScaleMode2x:
        return backingScaleFactor / 2;
    case ScopeScaleMode3x:
        return backingScaleFactor / 3;
    case ScopeScaleMode4x:
        return backingScaleFactor / 4;
    }
}

- (BOOL)updateDrawDataWithViewParams:(ShaderRendererParams)params {
    self.isVisible = params.isVisible;
    [self updateVisListening];

    if (!self.isListening) {
        return NO;
    }

    @synchronized (self) {
        CGFloat scale = [self scaleFactorForBackingScaleFactor:params.backingScaleFactor];

        if (_scope.sample_count != 0) {
            ddb_scope_tick(&_scope);
            ddb_scope_get_draw_data(&_scope, (int)(params.bounds.size.width * scale), (int)(params.bounds.size.height * scale), 1, &_draw_data);
        }
    }

    return YES;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    if (menuItem.action == @selector(setScaleAuto:) && self.settings.scaleMode == ScopeScaleModeAuto) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setScale1x:) && self.settings.scaleMode == ScopeScaleMode1x) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setScale2x:) && self.settings.scaleMode == ScopeScaleMode2x) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setScale3x:) && self.settings.scaleMode == ScopeScaleMode3x) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setScale4x:) && self.settings.scaleMode == ScopeScaleMode4x) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFragmentDuration50ms:) && self.settings.fragmentDuration == ScopeFragmentDuration50) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFragmentDuration100ms:) && self.settings.fragmentDuration == ScopeFragmentDuration100) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFragmentDuration200ms:) && self.settings.fragmentDuration == ScopeFragmentDuration200) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFragmentDuration300ms:) && self.settings.fragmentDuration == ScopeFragmentDuration300) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setFragmentDuration500ms:) && self.settings.fragmentDuration == ScopeFragmentDuration500) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setMultichannelRenderingMode:) && self.settings.renderMode == DDB_SCOPE_MULTICHANNEL) {
        menuItem.state = NSControlStateValueOn;
    }
    else if (menuItem.action == @selector(setMonoRenderingMode:) && self.settings.renderMode == DDB_SCOPE_MONO) {
        menuItem.state = NSControlStateValueOn;
    }
    else {
        menuItem.state = NSControlStateValueOff;
    }

    return YES;
}

- (void) updateRendererSettings {
    NSColor *color;
    if (self.settings.useCustomColor) {
        color = self.settings.customColor;
    }
    if (color == nil) {
        color = VisualizationSettingsUtil.shared.baseColor;
    }
    self.baseColor = color;

    color = nil;
    if (self.settings.useCustomBackgroundColor) {
        color = self.settings.customBackgroundColor;
    }
    if (color == nil) {
        color = VisualizationSettingsUtil.shared.backgroundColor;
    }
    self.backgroundColor = color;

}

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_CONFIGCHANGED) {
        [self updateRendererSettings];
    }
    [super message:_id ctx:ctx p1:p1 p2:p2];
}

- (NSColor *)baselor {
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        return NSColor.controlAccentColor;
    }
#endif
    return NSColor.alternateSelectedControlColor;
}

// Called by the timer in superclass
- (void)draw {
    self.view.needsDisplay = YES;
}

#pragma mark - MetalViewDelegate

- (void)metalViewDidResize:(NSView *)view {
    NSSize size = [self.view convertSizeToBacking:self.view.bounds.size];
    [_renderer drawableResize:size];
}

#pragma mark - CALayerDelegate

- (void)displayLayer:(CALayer *)layer {
    ShaderRendererParams params = {
        .backingScaleFactor = self.view.window.screen.backingScaleFactor,
        .isVisible = self.view.window.isVisible,
        .bounds = self.view.bounds
    };
    if (![self updateDrawDataWithViewParams:params]) {
        return;
    }

    [_renderer renderToMetalLayer:(CAMetalLayer *)layer viewParams:params];
}

#pragma mark - ShaderRendererDelegate

- (BOOL)applyFragParamsWithViewport:(vector_uint2)viewport device:(id<MTLDevice>)device commandBuffer:(id<MTLCommandBuffer>)commandBuffer encoder:(id<MTLRenderCommandEncoder>)encoder viewParams:(ShaderRendererParams)viewParams {
    float scale = (float)(viewParams.backingScaleFactor / [self scaleFactorForBackingScaleFactor:viewParams.backingScaleFactor]);

    struct ScopeFragParams params;
    NSColor *color = [self.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    NSColor *backgroundColor = [self.backgroundColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat components[4];
    [color getComponents:components];
    CGFloat backgroundComponents[4];
    [backgroundColor getComponents:backgroundComponents];

    params.color = (vector_float4){ (float)components[0], (float)components[1], (float)components[2], 1 };
    params.backgroundColor = (vector_float4){ (float)backgroundComponents[0], (float)backgroundComponents[1], (float)backgroundComponents[2], 1 };
    params.size.x = viewport.x;
    params.size.y = viewport.y;
    params.point_count = _draw_data.point_count;
    params.channels = _draw_data.mode == DDB_SCOPE_MONO ? 1 : _draw_data.channels;
    params.scale = scale;
    [encoder setFragmentBytes:&params length:sizeof (params) atIndex:0];

    if (_draw_data.points == NULL) {
        id<MTLBuffer> buffer = [self.bufferLoop nextBufferForSize:12];
        [encoder setFragmentBuffer:buffer offset:0 atIndex:1];
    }
    else {
        NSUInteger size = _draw_data.point_count * sizeof (ddb_scope_point_t) * params.channels;
        id<MTLBuffer> buffer = [self.bufferLoop nextBufferForSize:size];
        memcpy (buffer.contents, _draw_data.points, size);
        [encoder setFragmentBuffer:buffer offset:0 atIndex:1];
    }

    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull completedCommandBuffer) {
        [self.bufferLoop signalCompletion];
    }];

    return YES;
}

@end
