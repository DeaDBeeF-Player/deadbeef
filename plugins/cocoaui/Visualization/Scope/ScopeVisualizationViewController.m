//
//  ScopeVisualizationViewController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 30/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "AAPLView.h"
#import "ScopeRenderer.h"
#import "ScopeVisualizationViewController.h"
#import "VisBaseColorUtil.h"
#include "deadbeef.h"
#include "scope.h"

extern DB_functions_t *deadbeef;

static NSString * const kWindowIsVisibleKey = @"view.window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface ScopeVisualizationViewController() <AAPLViewDelegate>
@property (nonatomic) BOOL isListening;
@property (nonatomic) ScopeScaleMode scaleMode;
@property (nonatomic,readonly) CGFloat scaleFactor;
@end

@implementation ScopeVisualizationViewController {
    ddb_audio_data_t _input_data;
    ddb_waveformat_t _fmt;
    ddb_scope_t _scope;
    ddb_scope_draw_data_t _draw_data;
    ScopeRenderer *_renderer;
}

static void vis_callback (void *ctx, const ddb_audio_data_t *data) {
    ScopeVisualizationViewController *viewController = (__bridge ScopeVisualizationViewController *)(ctx);
    [viewController updateScopeData:data];
}

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

        if (_input_data.nframes == 0) {
            return;
        }

        @synchronized (self) {
            ddb_scope_process(&_scope, _input_data.fmt->samplerate, _input_data.fmt->channels, _input_data.data, _input_data.nframes);
        }

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

- (void)awakeFromNib {
    [super awakeFromNib];

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

    self.view.menu = menu;

    [self addObserver:self forKeyPath:kWindowIsVisibleKey options:NSKeyValueObservingOptionInitial context:kIsVisibleContext];
    _isListening = NO;
    _input_data.fmt = &_fmt;
    ddb_scope_init(&_scope);
    _scope.mode = DDB_SCOPE_MULTICHANNEL;

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();

    AAPLView *view = (AAPLView *)self.view;

    // Set the device for the layer so the layer can create drawable textures that can be rendered to
    // on this device.
    view.metalLayer.device = device;

    // Set this class as the delegate to receive resize and render callbacks.
    view.delegate = self;

    view.metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

    _renderer = [[ScopeRenderer alloc] initWithMetalDevice:device
                                      drawablePixelFormat:view.metalLayer.pixelFormat];
    _renderer.baseColor = VisBaseColorUtil.shared.baseColor;
}

- (void)updateVisListening {
    if (!self.isListening && self.view.window.isVisible) {
        deadbeef->vis_waveform_listen((__bridge void *)(self), vis_callback);
        self.isListening = YES;
    }
    else if (self.isListening && !self.view.window.isVisible) {
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

- (void)drawableResize:(CGSize)size
{
    [_renderer drawableResize:size];
}

- (CGFloat)scaleFactor {
    switch (self.scaleMode) {
    case ScopeScaleModeAuto:
        return 1;
    case ScopeScaleMode1x:
        return self.view.window.backingScaleFactor;
    case ScopeScaleMode2x:
        return self.view.window.backingScaleFactor / 2;
    case ScopeScaleMode3x:
        return self.view.window.backingScaleFactor / 3;
    case ScopeScaleMode4x:
        return self.view.window.backingScaleFactor / 4;
    }
}

- (BOOL)updateDrawData {
    [self updateVisListening];

    if (!self.isListening) {
        return NO;
    }

    @synchronized (self) {
        CGFloat scale = self.scaleFactor;

        if (_scope.sample_count != 0) {
            ddb_scope_tick(&_scope);
            ddb_scope_get_draw_data(&_scope, (int)(self.view.bounds.size.width * scale), (int)(self.view.bounds.size.height * scale), 1, &_draw_data);
        }
    }

    return YES;
}

- (void)renderToMetalLayer:(nonnull CAMetalLayer *)layer
{
    if (![self updateDrawData]) {
        return;
    }

    float scale = (float)(self.view.window.backingScaleFactor / self.scaleFactor);
    [_renderer renderToMetalLayer:layer drawData:&_draw_data scale:scale];
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

- (void)message:(uint32_t)_id ctx:(uintptr_t)ctx p1:(uint32_t)p1 p2:(uint32_t)p2 {
    if (_id == DB_EV_CONFIGCHANGED) {
        _renderer.baseColor = VisBaseColorUtil.shared.baseColor;
    }
}

@end
