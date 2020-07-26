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
#define LOWER_BOUND -70

@interface VisualizationView() {
    float saBars[NUM_BARS];
    float saPeaks[NUM_BARS];
    float saLowerBound;
}

@property (nonatomic) int samplerate;
@property (nonatomic) int channels;
@property (nonatomic) float *fftData;
@property (nonatomic) int nframes;
@end

@implementation VisualizationView

static void vis_callback (void *ctx, ddb_audio_data_t *data) {
    VisualizationView *view = (__bridge VisualizationView *)(ctx);
    [view updateFFTData:data];
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (!self) {
        return nil;
    }

    deadbeef->vis_spectrum_listen((__bridge void *)(self), vis_callback);
    memset (saBars, 0, sizeof (saBars));
    memset (saPeaks, 0, sizeof (saPeaks));
    saLowerBound = LOWER_BOUND;

    return self;
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
        saBars[i] -= 1/50.0f;
        if (saBars[i] < 0) {
            saBars[i] = 0;
        }
        saPeaks[i] -= 0.5/60.0f;
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

    NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
    paragraphStyle.alignment = NSTextAlignmentLeft;

    NSDictionary *attrs = @{
        NSFontAttributeName: [NSFont fontWithName:@"HelveticaNeue" size:10],
        NSParagraphStyleAttributeName: paragraphStyle,
        NSForegroundColorAttributeName: gridColor
    };

    for (int i = 0; i < NUM_BARS; i += 12) {
        NSString *string = [NSString stringWithFormat:@"A%d", 3+i/10];
        CGFloat x = (CGFloat)i*(NSWidth(self.bounds)-1)/(CGFloat)NUM_BARS + 4;
        [string drawAtPoint:NSMakePoint(x, NSHeight(self.bounds)-12) withAttributes:attrs];
    }

    // horz lines, db scale
    CGFloat lower = -floor(LOWER_BOUND);
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
        [string drawAtPoint:NSMakePoint(0, NSHeight(self.bounds)-y-12) withAttributes:attrs];
    }
    CGContextStrokePath(context);
}

- (void)drawSpectrumAnalyzer:(CGContextRef)context {
    CGFloat bw = NSWidth(self.bounds)/(CGFloat)NUM_BARS;
    NSColor *cBar;
    if (@available(macOS 10.14, *)) {
        cBar = NSColor.controlAccentColor;
    } else {
        cBar = NSColor.alternateSelectedControlColor;
    }
    CGContextSetFillColorWithColor(context, cBar.CGColor);
    for (int i = 0; i < NUM_BARS; i++) {
        CGFloat bh = (CGFloat)saBars[i] * NSHeight(self.bounds);
//        int pIdx = i * 255 / NUM_BARS;
//        let r = colorPalette[1][pIdx*4+0]
//        let g = colorPalette[1][pIdx*4+1]
//        let b = colorPalette[1][pIdx*4+2]
        CGContextFillRect(context, CGRectMake((CGFloat)i*bw+1, 0, bw-2, bh));
    }
    for (int i = 0; i < NUM_BARS; i++) {
        CGFloat bh = (CGFloat)saPeaks[i] * NSHeight(self.bounds);
        CGContextAddRect(context, CGRectMake((CGFloat)i*bw+1, bh, bw-2, 2));
    }

    CGFloat rgbaPeak[] = {0.8,0.8,0.8,1}; // peaks color
    CGColorRef cPeak = CGColorCreate(CGColorSpaceCreateDeviceRGB(), rgbaPeak);
    CGContextSetFillColorWithColor(context, cPeak);
    CFRelease(cPeak);
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
