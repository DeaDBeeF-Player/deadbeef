//
//  ScopeVisualizationView.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 18/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "ScopeVisualizationView.h"
#include "deadbeef.h"
#include "scope.h"

extern DB_functions_t *deadbeef;

static NSString * const kWindowIsVisibleKey = @"window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

@interface ScopeVisualizationView() {
    ddb_audio_data_t _input_data;
    ddb_waveformat_t _fmt;
    ddb_scope_t _scope;
    ddb_scope_draw_data_t _draw_data;
}

@property (nonatomic) BOOL isListening;
@property (nonatomic,readonly) NSColor *baseColor;

@end

@implementation ScopeVisualizationView

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

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    // for some reason KVO is not triggered when the window becomes hidden
    if (self.isListening && !self.window.isVisible) {
        deadbeef->vis_waveform_unlisten ((__bridge void *)(self));
        self.isListening = NO;
        return;
    }

    [NSColor.blackColor setFill];
    NSRectFill(dirtyRect);

    if (_input_data.nframes == 0) {
        return;
    }

    @synchronized (self) {
        ddb_scope_process(&_scope, _input_data.fmt->samplerate, _input_data.fmt->channels, _input_data.data, _input_data.nframes);
        ddb_scope_tick(&_scope);
        ddb_scope_get_draw_data(&_scope, (int)self.bounds.size.width, (int)self.bounds.size.height, &_draw_data);
    }

    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    CGContextMoveToPoint(context, 0, self.bounds.size.height/2);
    ddb_scope_point_t *point = _draw_data.points;
    for (int i = 0; i < _draw_data.point_count; i++, point++) {
        CGContextAddLineToPoint(context, point->x, point->ymin);
        CGContextAddLineToPoint(context, point->x, point->ymax);
    }

    CGContextSetStrokeColorWithColor(context, self.baseColor.CGColor);
    CGContextStrokePath(context);
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

@end
