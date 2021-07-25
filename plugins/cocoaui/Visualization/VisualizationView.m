//
//  VisualizationView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationView.h"
#include "deadbeef.h"
#include "analyzer.h"

extern DB_functions_t *deadbeef;

#define LOWER_BOUND -80

@interface VisualizationView() {
    float saLowerBound;
    ddb_analyzer_t _analyzer;
    ddb_analyzer_draw_data_t _draw_data;
}

@property (nonatomic) int samplerate;
@property (nonatomic) int channels;
@property (nonatomic) float *fftData;
@property (nonatomic) int nframes;
@property (nonatomic) NSDictionary *textAttrs;
@property (nonatomic,readonly) NSColor *baseColor;
@property (nonatomic,readonly) NSColor *barColor;
@property (nonatomic,readonly) NSColor *peakColor;
@property (nonatomic) NSColor *gridColor;

@end

@implementation VisualizationView

static void vis_callback (void *ctx, const ddb_audio_data_t *data) {
    VisualizationView *view = (__bridge VisualizationView *)(ctx);
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
    deadbeef->vis_spectrum_listen((__bridge void *)(self), vis_callback);
    saLowerBound = LOWER_BOUND;

    NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
    paragraphStyle.alignment = NSTextAlignmentLeft;
    self.gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.2];

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
    _analyzer.mode = DDB_ANALYZER_MODE_FREQUENCIES; // DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
}

- (NSColor *)baseColor {
#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        return NSColor.controlAccentColor;
    }
#endif
    return NSColor.alternateSelectedControlColor;
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
    deadbeef->vis_spectrum_unlisten ((__bridge void *)(self));
    ddb_analyzer_dealloc(&_analyzer);
    ddb_analyzer_draw_data_dealloc(&_draw_data);
}

- (void)updateFFTData:(const ddb_audio_data_t *)data {
    @synchronized (self) {
        if (self.nframes != data->nframes || self.samplerate != data->fmt->samplerate || self.channels != data->fmt->channels) {
            free (self.fftData);
            self.fftData = NULL;
            self.nframes = 0;
            if (data->fmt->channels != 2) {
                self.samplerate = 0;
                return;
            }
            self.fftData = calloc (data->nframes * 2, sizeof (float));
            self.nframes = data->nframes;
            self.samplerate = data->fmt->samplerate;
            self.channels = data->fmt->channels;
        }

#if 0
        // low pass the spectrum
        for (int i = 0; i < data->nframes * 2; i++) {
            self.fftData[i] = self.fftData[i] + (data->data[i] - self.fftData[i]) * 0.7;
        }
#endif
        // copy without smoothing
        memcpy (self.fftData, data->data, data->nframes*2*sizeof(float));

        ddb_analyzer_process(&_analyzer, data->fmt->samplerate, data->fmt->channels, data->data, data->nframes);
    }
}

- (void)drawSaGrid {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
#define NUM_BARS (7*12)
    // vert lines, octaves
    CGContextSetStrokeColorWithColor(context, self.gridColor.CGColor);
    for (int i = 12; i < NUM_BARS; i += 12) {
        CGFloat x = (CGFloat)i * NSWidth(self.bounds) / (CGFloat)NUM_BARS;
        CGPoint points[] = {
            CGPointMake(x, 0),
            CGPointMake(x, NSHeight(self.bounds)-1)
        };
        CGContextAddLines(context, points, 2);
    }

    // horz lines, db scale
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
    CGContextStrokePath(context);

    // octaves text
    for (int i = 0; i < NUM_BARS; i += 12) {
        NSString *string = [NSString stringWithFormat:@"A%d", 3+i/12];
        CGFloat x = (CGFloat)i*(NSWidth(self.bounds)-1)/(CGFloat)NUM_BARS + 4;
        [string drawAtPoint:NSMakePoint(x, NSHeight(self.bounds)-12) withAttributes:self.textAttrs];
    }

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

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    [NSColor.blackColor setFill];
    NSRectFill(dirtyRect);

    if (self.samplerate == 0) {
        return;
    }
//    [self drawFFTWithChannel:0];
//    [self drawFFTWithChannel channel:1];

    [self drawSaGrid];
    [self drawFrequencyLines];
}

// draw fft for specified channel
- (void)drawFFTWithChannel:(int)channel {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    __block int fftSize;
    float *fftData = NULL;

    @synchronized (self) {
        fftSize = self.nframes;
        fftData = malloc (fftSize * sizeof (float));
        memcpy (fftData, self.fftData + channel * fftSize, fftSize * sizeof (float));
    }

    CGContextSetStrokeColorWithColor(context, channel == 0 ? NSColor.redColor.CGColor : NSColor.greenColor.CGColor);
    CGFloat prevBar = 0;
    for (int i = 0; i < fftSize; i++) {
        CGFloat bar = 0;
        if (i == 0) {
            bar = prevBar = fftData[i];
        }
        else {
            bar = prevBar + (fftData[i] - prevBar) * 0.1;
            prevBar = bar;
        }
        bar = MAX(0, MIN(1, bar));
        float bound = -saLowerBound;
        bar = (20*log10(bar) + bound)/bound;
        bar = MAX(0, MIN(1, bar));
        CGFloat x = (CGFloat)i/fftSize*self.bounds.size.width;
        CGFloat y = bar * self.bounds.size.height;
        if (i == 0) {
            CGContextMoveToPoint(context, x, y);
        }
        else {
            CGContextAddLineToPoint(context, x, y);
        }
    }

    CGContextStrokePath(context);
    free (fftData);
}

- (void)drawFrequencyLines {
    @synchronized (self) {
        ddb_analyzer_tick(&_analyzer);
        ddb_analyzer_get_draw_data(&_analyzer, self.bounds.size.width, self.bounds.size.height, &_draw_data);
    }

    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    ddb_analuzer_draw_bar_t *bar = _draw_data.bars;
    for (int i = 0; i < _draw_data.bar_count; i++, bar++) {
        if (_analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
            CGContextMoveToPoint(context, bar->xpos, 0);
            CGContextAddLineToPoint(context, bar->xpos, bar->bar_height);
        }
        else {
            CGContextAddRect(context, CGRectMake(bar->xpos, 0, _draw_data.bar_width, bar->bar_height));
        }
    }
    if (_analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        CGContextSetStrokeColorWithColor(context, self.barColor.CGColor);
        CGContextStrokePath(context);
    }
    else {
        CGContextSetFillColorWithColor(context, self.barColor.CGColor);
        CGContextFillPath(context);
    }

    bar = _draw_data.bars;
    for (int i = 0; i < _draw_data.bar_count; i++, bar++) {
        CGContextAddRect(context, CGRectMake(bar->xpos, bar->peak_ypos, _draw_data.bar_width, 1));
    }
    CGContextSetFillColorWithColor(context, self.peakColor.CGColor);
    CGContextFillPath(context);
}

@end
