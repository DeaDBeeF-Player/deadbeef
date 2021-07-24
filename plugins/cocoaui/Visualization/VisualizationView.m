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

#define FIRST_NOTE_FREQ 110
#define NUM_BARS (8*12)
#define LOWER_BOUND -80
#define PEAK_DRAW_HEIGHT 1
#define A 9.8f

static double noteFrequencies[NUM_BARS];

@interface VisualizationView() {
    float saBarsTargets[NUM_BARS]; // interpolated bars from FFT data
    float saBars[NUM_BARS]; // peaked bars falling down
    float saPeaks[NUM_BARS]; // peaks falling at different rate
    float saPeaksSpeed[NUM_BARS]; // current speed of the peaks
    float saBarSpeed[NUM_BARS]; // current speed of the bars
    float saPeaksHold[NUM_BARS]; // time remaining to hold the peaks
    float saLowerBound; // lower bound of the graph (NOTE: do not delete -- it can be made configurable)
    NSTimeInterval _prevTime;
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
    if (noteFrequencies[0] == 0) {
        for (int i = 0; i < NUM_BARS; i++) {
            noteFrequencies[i] = pow(1.059463094359,i);
        }
    }

    deadbeef->vis_spectrum_listen((__bridge void *)(self), vis_callback);
    memset (saBars, 0, sizeof (saBars));
    memset (saPeaks, 0, sizeof (saPeaks));
    saLowerBound = LOWER_BOUND;

    NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
    paragraphStyle.alignment = NSTextAlignmentLeft;
    self.gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.2];

    self.textAttrs = @{
        NSFontAttributeName: [NSFont fontWithName:@"HelveticaNeue" size:10],
        NSParagraphStyleAttributeName: paragraphStyle,
        NSForegroundColorAttributeName: self.gridColor
    };

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

    return [NSColor colorWithHue:h saturation:1 brightness:1 alpha:1];
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
    spectrumData = malloc (self.nframes * sizeof (float));
    NSTimeInterval currentTime = CFAbsoluteTimeGetCurrent();
    NSTimeInterval dt = currentTime - _prevTime;
    _prevTime = currentTime;

    @synchronized (self) {
        sdCount = self.nframes;
        memcpy (spectrumData, self.fftData, self.nframes * sizeof (float));
        samplerate = self.samplerate;
    }

    for (int i = 0; i < NUM_BARS; i++) {
        const float a = A;
        const float t = dt;
        // first attenuate bars and peaks
        saBars[i] -= saBarSpeed[i];
        if (saBars[i] < 0) {
            saBars[i] = 0;
        }
        saBarSpeed[i] += a * dt / 2;
        if (saBarSpeed[i] > t*3) {
            saBarSpeed[i] = t*3;
        }

        if (saPeaksHold[i] > 0) {
            saPeaksHold[i] -= t;
        }
        else {
            saPeaksSpeed[i] += a * t / 2;

            saPeaks[i] -= saPeaksSpeed[i] * t;
            if (saPeaks[i] < 0) {
                saPeaks[i] = 0;
            }
        }

        // now calculate new values

        // convert index to musical note
        float fn = FIRST_NOTE_FREQ * noteFrequencies[i];
        float fn_next_bar = FIRST_NOTE_FREQ * noteFrequencies[i+1];

        int si = MIN(sdCount/2-1,(int)(fn/samplerate*(float)(sdCount/2)));
        int si_end = (int)(fn_next_bar/samplerate*(float)(sdCount/2)) - 1;
        si_end = MIN(sdCount/2-1,si_end);
        si_end = MAX(si,si_end);

        // get peak frequency in the band
        float newBar = 0;
        for (int n = si; n <= si_end; n++) {
            float val = MAX(spectrumData[si*2+0], spectrumData[si*2+1]);
            if (val > newBar) {
                newBar = val;
            }
        }
        newBar = MAX(0, MIN(1, newBar));
        float bound = -saLowerBound;
        newBar = (20*log10(newBar) + bound)/bound;
        newBar = MAX(0, MIN(1, newBar));

        // interpolate fft data
        if (newBar > saBarsTargets[i]) {
            saBarsTargets[i] += dt * 2;
            if (saBarsTargets[i] > newBar) {
                saBarsTargets[i] = newBar;
            }
        }
        else if (newBar < saBarsTargets[i]) {
            saBarsTargets[i] -= dt * 2;
            if (saBarsTargets[i] < 0) {
                saBarsTargets[i] = 0;
            }
        }

        // update bars
        if (saBarsTargets[i] > saBars[i]) {
            saBars[i] = saBarsTargets[i];
            saBarSpeed[i] = 0;
        }
        if (saPeaks[i] < saBars[i]) {
            saPeaks[i] = saBars[i];
            saPeaksHold[i] = 0.2;
            saPeaksSpeed[i] = 0;
        }
    }

    free (spectrumData);
}

- (void)drawSaGrid:(CGContextRef)context {
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
        NSString *string = [NSString stringWithFormat:@"A%d", 1+i/12];
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
        CGContextAddRect(context, CGRectMake((CGFloat)i*bw+1, bh, bw-2, PEAK_DRAW_HEIGHT));
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
