//
//  SpectrumAnalyzerbaseco.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "SpectrumAnalyzerVisualizationView.h"
#include "deadbeef.h"
#include "analyzer.h"

extern DB_functions_t *deadbeef;

static NSString * const kWindowIsVisibleKey = @"window.isVisible";
static void *kIsVisibleContext = &kIsVisibleContext;

#define LOWER_BOUND -80

@interface SpectrumAnalyzerVisualizationView() {
    float saLowerBound;
    ddb_analyzer_t _analyzer;
    ddb_analyzer_draw_data_t _draw_data;
    ddb_waveformat_t _fmt;
    ddb_audio_data_t _input_data;
}

@property (nonatomic) NSDictionary *textAttrs;
@property (nonatomic,readonly) NSColor *barColor;
@property (nonatomic,readonly) NSColor *peakColor;
@property (nonatomic) NSColor *gridColor;

@property (nonatomic) BOOL isListening;

@end

@implementation SpectrumAnalyzerVisualizationView

static void vis_callback (void *ctx, const ddb_audio_data_t *data) {
    SpectrumAnalyzerVisualizationView *view = (__bridge SpectrumAnalyzerVisualizationView *)(ctx);
    [view updateFFTData:data];
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

- (instancetype)initWithCoder:(NSCoder *)coder {
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
    saLowerBound = LOWER_BOUND;

    _input_data.fmt = &_fmt;

    NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
    paragraphStyle.alignment = NSTextAlignmentLeft;
    self.gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.4];

    self.textAttrs = @{
        NSFontAttributeName: [NSFont fontWithName:@"HelveticaNeue" size:10],
        NSParagraphStyleAttributeName: paragraphStyle,
        NSForegroundColorAttributeName: self.gridColor
    };

    // using the analyzer framework
    ddb_analyzer_init(&_analyzer);
    _analyzer.db_lower_bound = LOWER_BOUND;
    _analyzer.peak_hold = 10;
    _analyzer.view_width = 1000;
    _analyzer.fractional_bars = 1;
    _analyzer.octave_bars_step = 2;
    _analyzer.max_of_stereo_data = 1;
    _analyzer.mode = DDB_ANALYZER_MODE_FREQUENCIES;
    _analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
}

- (void)updateVisListening {
    if (self.isListening && !self.window.isVisible) {
        deadbeef->vis_spectrum_unlisten ((__bridge void *)(self));
        self.isListening = NO;
    }
    else if (!self.isListening && self.window.isVisible) {
        deadbeef->vis_spectrum_listen2((__bridge void *)(self), vis_callback);
        self.isListening = YES;
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

- (void)updateAnalyzerSettings:(SpectrumAnalyzerSettings *)settings {
    if (_analyzer.mode != settings.mode || _analyzer.octave_bars_step != settings.barGranularity) {
        _analyzer.mode_did_change = 1;
    }
    _analyzer.mode = settings.mode;
    _analyzer.bar_gap_denominator = settings.distanceBetweenBars;
    _analyzer.octave_bars_step = settings.barGranularity;
}


- (NSColor *)barColor {
    NSColor *tempColor = [self.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat h, s, b, a;
    [tempColor getHue:&h saturation:&s brightness:&b alpha:&a];
    return [NSColor colorWithHue:h saturation:s brightness:b*0.7 alpha:1];
}

- (NSColor *)peakColor {
    NSColor *tempColor = [self.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat h, s, b, a;
    [tempColor getHue:&h saturation:&s brightness:&b alpha:&a];

    return [NSColor colorWithHue:h saturation:s*0.7 brightness:b*1.1 alpha:1];
}

- (void)dealloc {
    [self removeObserver:self forKeyPath:kWindowIsVisibleKey];
    if (self.isListening) {
        deadbeef->vis_spectrum_unlisten ((__bridge void *)(self));
        self.isListening = NO;
    }
    ddb_analyzer_dealloc(&_analyzer);
    ddb_analyzer_draw_data_dealloc(&_draw_data);
    free (_input_data.data);
    _input_data.data = NULL;
}

- (void)updateFFTData:(const ddb_audio_data_t *)data {
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

- (void)drawSaGrid {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;

    // horz lines, db scale
    CGContextSetStrokeColorWithColor(context, self.gridColor.CGColor);
    CGFloat lower = -floor(saLowerBound);
    for (int db = 10; db < lower; db += 10) {
        CGFloat y = (CGFloat)(db / lower) * NSHeight(self.bounds);
        if (y >= NSHeight(self.bounds)) {
            break;
        }

        CGPoint points[] = {
            CGPointMake(0, y),
            CGPointMake(NSWidth(self.bounds)-1, y)
        };
        CGContextAddLines(context, points, 2);
    }
    CGFloat dash[2] = {1, 2};
    CGContextSetLineDash(context, 0, dash, 2);
    CGContextStrokePath(context);
    CGContextSetLineDash(context, 0, NULL, 0);

    // db text
    for (int db = 10; db < lower; db += 10) {
        CGFloat y = (CGFloat)(db / lower) * NSHeight(self.bounds);
        if (y >= NSHeight(self.bounds)) {
            break;
        }

        NSString *string = [NSString stringWithFormat:@"%d dB", -db];
        [string drawAtPoint:NSMakePoint(0, NSHeight(self.bounds)-y-12) withAttributes:self.textAttrs];
    }
}

- (void)drawFrequencyLabels {
    // octaves text
    for (int i = 0; i < _draw_data.label_freq_count; i++) {
        if (_draw_data.label_freq_positions < 0) {
            continue;
        }
        NSString *string = [NSString stringWithUTF8String:_draw_data.label_freq_texts[i]];
        CGFloat x = _draw_data.label_freq_positions[i];
        [string drawAtPoint:NSMakePoint(x, NSHeight(self.bounds)-12) withAttributes:self.textAttrs];
    }

}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    // for some reason KVO is not triggered when the window becomes hidden
    [self updateVisListening];

    [NSColor.blackColor setFill];
    NSRectFill(dirtyRect);

    if (_input_data.nframes == 0) {
        return;
    }

    if (self.isListening) {
        @synchronized (self) {
            ddb_analyzer_process(&_analyzer, _input_data.fmt->samplerate, _input_data.fmt->channels, _input_data.data, _input_data.nframes);
            ddb_analyzer_tick(&_analyzer);
            ddb_analyzer_get_draw_data(&_analyzer, self.bounds.size.width, self.bounds.size.height, &_draw_data);
        }
    }

    [self drawSaGrid];
    [self drawFrequencyLabels];
    [self drawAnalyzer];
}

- (void)drawAnalyzerDescreteFrequencies {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    ddb_analyzer_draw_bar_t *bar = _draw_data.bars;
    for (int i = 0; i < _draw_data.bar_count; i++, bar++) {
            CGContextMoveToPoint(context, bar->xpos, 0);
            CGContextAddLineToPoint(context, bar->xpos, bar->bar_height);
    }
    CGContextSetStrokeColorWithColor(context, self.barColor.CGColor);
    CGContextStrokePath(context);

    bar = _draw_data.bars;
    for (int i = 0; i < _draw_data.bar_count; i++, bar++) {
        CGContextMoveToPoint(context, bar->xpos-0.5, bar->peak_ypos);
        CGContextAddLineToPoint(context, bar->xpos+0.5, bar->peak_ypos);
    }
    CGContextSetStrokeColorWithColor(context, self.peakColor.CGColor);
    CGContextStrokePath(context);
}

- (void)drawAnalyzerOctaveBands {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    ddb_analyzer_draw_bar_t *bar = _draw_data.bars;
    for (int i = 0; i < _draw_data.bar_count; i++, bar++) {
        CGContextAddRect(context, CGRectMake(bar->xpos, 0, _draw_data.bar_width, bar->bar_height));
    }
    CGContextSetFillColorWithColor(context, self.barColor.CGColor);
    CGContextFillPath(context);

    bar = _draw_data.bars;
    for (int i = 0; i < _draw_data.bar_count; i++, bar++) {
        CGContextAddRect(context, CGRectMake(bar->xpos, bar->peak_ypos, _draw_data.bar_width, 1));
    }
    CGContextSetFillColorWithColor(context, self.peakColor.CGColor);
    CGContextFillPath(context);
}


- (void)drawAnalyzer {
    if (_analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        [self drawAnalyzerDescreteFrequencies];
    }
    else {
        [self drawAnalyzerOctaveBands];
    }
}

@end
