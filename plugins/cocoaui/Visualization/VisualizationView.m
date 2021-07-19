//
//  VisualizationView.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import "VisualizationView.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

#define NUM_BARS 84

// FIXME: add UPPER_BOUND, -30dB seems to be reasonable
#define LOWER_BOUND -70

@interface VisualizationView() {
    float saBars[NUM_BARS];
    float saPeaks[NUM_BARS];
    float saPeaksSpeed[NUM_BARS];
    float saLowerBound;
}

@property (nonatomic) int samplerate;
@property (nonatomic) int channels;
@property (nonatomic) float *fftData;
@property (nonatomic) int nframes;
@property (nonatomic) NSDictionary *textAttrs;
@property (nonatomic) NSColor *barColor;
@property (nonatomic) NSColor *peakColor;

@end

@implementation VisualizationView

static void vis_callback (void *ctx, ddb_audio_data_t *data) {
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
    memset (saBars, 0, sizeof (saBars));
    memset (saPeaks, 0, sizeof (saPeaks));
    saLowerBound = LOWER_BOUND;

    NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
    paragraphStyle.alignment = NSTextAlignmentLeft;
    NSColor *gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.8];

    self.textAttrs = @{
        NSFontAttributeName: [NSFont fontWithName:@"HelveticaNeue" size:10],
        NSParagraphStyleAttributeName: paragraphStyle,
        NSForegroundColorAttributeName: gridColor
    };

    self.peakColor = [NSColor colorWithRed:0.8 green:0.8 blue:0.8 alpha:1];

#ifdef MAC_OS_X_VERSION_10_14
    if (@available(macOS 10.14, *)) {
        self.barColor = NSColor.controlAccentColor;
    } else
#endif
    {
        self.barColor = NSColor.alternateSelectedControlColor;
    }
}

- (void)dealloc
{
    deadbeef->vis_spectrum_unlisten ((__bridge void *)(self));
}

- (void)updateFFTData:(ddb_audio_data_t *)data {
    @synchronized (self) {
        if (self.nframes != data->nframes || self.samplerate != data->fmt->samplerate || self.channels != data->fmt->channels) {
            free (self.fftData);
            self.fftData = NULL;
            self.nframes = 0;
            if (data->fmt->channels != 2) {
                self.samplerate = 0;
                return;
            }
            self.fftData = malloc (data->nframes * sizeof (float) * 2);
            self.nframes = data->nframes;
            self.samplerate = data->fmt->samplerate;
            self.channels = data->fmt->channels;
        }
        memcpy (self.fftData, data->data, data->nframes * sizeof (float) * 2);
    }
}

- (void)updateSpectrumAnalyzer {
    float *spectrumData = NULL;
    int sdCount = 0;
    float samplerate = 0;
    @synchronized (self) {
        sdCount = self.nframes;
        spectrumData = malloc (self.nframes * sizeof (float));
        memcpy (spectrumData, self.fftData, self.nframes * sizeof (float));
        samplerate = self.samplerate;
    }

    for (int i = 0; i < NUM_BARS; i++) {
        // first attenuate bars and peaks
        saBars[i] -= 1/50.0f*4;
        if (saBars[i] < 0) {
            saBars[i] = 0;
        }

        const float a = 9.8f;
        const float t = 1/60.f;
        saPeaksSpeed[i] = saPeaksSpeed[i] + a * t / 2;

        saPeaks[i] -= saPeaksSpeed[i] * 1/60.f;
        if (saPeaks[i] < 0) {
            saPeaks[i] = 0;
        }

        // now calculate new values

        // convert index to musical note starting with A3 (220Hz)
        float fn = 220.0*pow(1.059463094359,i);

        int si = MIN(sdCount/2-1,(int)(fn/samplerate*(float)(sdCount/2)));

        float newBar = MAX(spectrumData[si*2+0], spectrumData[si*2+1]);
        newBar = MAX(0, MIN(1, newBar));
        float bound = -saLowerBound;
        newBar = (20*log10(newBar) + bound)/bound;
        newBar = MAX(0, MIN(1, newBar));

        if (newBar > saBars[i]) {
            saBars[i] = newBar;
        }
        if (saPeaks[i] < saBars[i]) {
            saPeaks[i] = saBars[i];
            saPeaksSpeed[i] = 0;
        }
    }

    free (spectrumData);
}

- (void)drawSaGrid:(CGContextRef)context {
    // vert lines, octaves
    NSColor *gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.8];
    CGContextSetStrokeColorWithColor(context, gridColor.CGColor);
    for (int i = 12; i < NUM_BARS; i += 12) {
        CGFloat x = (CGFloat)i * NSWidth(self.bounds) / (CGFloat)NUM_BARS;
        CGPoint points[] = {
            CGPointMake(x, 0),
            CGPointMake(x, NSHeight(self.bounds)-1)
        };
        CGContextAddLines(context, points, 2);
    }
    CGContextStrokePath(context);

    for (int i = 0; i < NUM_BARS; i += 12) {
        NSString *string = [NSString stringWithFormat:@"A%d", 3+i/12];
        CGFloat x = (CGFloat)i*(NSWidth(self.bounds)-1)/(CGFloat)NUM_BARS + 4;
        [string drawAtPoint:NSMakePoint(x, NSHeight(self.bounds)-12) withAttributes:self.textAttrs];
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

        NSString *string = [NSString stringWithFormat:@"%d dB", -db];
        [string drawAtPoint:NSMakePoint(0, NSHeight(self.bounds)-y-12) withAttributes:self.textAttrs];
    }
    CGContextStrokePath(context);
}

- (void)drawSpectrumAnalyzer:(CGContextRef)context {
    CGFloat bw = NSWidth(self.bounds)/(CGFloat)NUM_BARS;
    CGContextSetFillColorWithColor(context, self.barColor.CGColor);

    const CGFloat expand = 2; // how much to expand outside of the view bottom edge

    CGFloat h = NSHeight(self.bounds) + expand;

    for (int i = 0; i < NUM_BARS; i++) {
        CGFloat bh = (CGFloat)saBars[i] * h;
//        int pIdx = i * 255 / NUM_BARS;
//        let r = colorPalette[1][pIdx*4+0]
//        let g = colorPalette[1][pIdx*4+1]
//        let b = colorPalette[1][pIdx*4+2]
        CGContextAddRect(context, CGRectMake((CGFloat)i*bw+1, -expand, bw-2, bh));
    }
    CGContextFillPath(context);
    for (int i = 0; i < NUM_BARS; i++) {
        CGFloat bh = (CGFloat)saPeaks[i] * h - expand;
        CGContextAddRect(context, CGRectMake((CGFloat)i*bw+1, bh, bw-2, 2));
    }

    CGContextSetFillColorWithColor(context, self.peakColor.CGColor);
    CGContextFillPath(context);

    [self drawSaGrid:context];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    [NSColor.blackColor setFill];
    NSRectFill(dirtyRect);

    if (self.samplerate == 0) {
        return;
    }

    [self updateSpectrumAnalyzer];
    [self drawSpectrumAnalyzer:NSGraphicsContext.currentContext.CGContext];
}

@end
