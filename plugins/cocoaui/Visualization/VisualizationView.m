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

#define NUM_BARS 60

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
    saLowerBound = -70;

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

//func drawSaGrid (context : CGContext) {
//    // vert lines, octaves
//    let gridColor = UIColor.white.withAlphaComponent(0.8)
//    context.setStrokeColor(gridColor.cgColor)
//    for i in stride(from: 12, to: saBars.count, by: 12) {
//        let x = CGFloat(i)*bounds.width/CGFloat(saBars.count)
//        context.addLines(between: [CGPoint(x: x, y: 0), CGPoint(x: x, y: bounds.height-1)])
//    }
//    context.strokePath()
//
//    let paragraphStyle = NSMutableParagraphStyle()
//    paragraphStyle.alignment = .left
//
//    let attrs = [NSAttributedString.Key.font: UIFont(name: "HelveticaNeue", size: bounds.width/40)!, NSAttributedString.Key.paragraphStyle: paragraphStyle,
//                 NSAttributedString.Key.foregroundColor: gridColor]
//    for i in stride(from: 0, through: saBars.count, by: 12) {
//        let string = "A"+String(3+i/12)
//        let x = CGFloat(i)*(bounds.width-1)/CGFloat(saBars.count) + 4
//        string.draw(with: CGRect(x: x, y: 0, width: 448, height: 448), options: .usesLineFragmentOrigin, attributes: attrs, context: nil)
//    }
//
//    // horz lines, db scale
//    let lower = -floor(SettingsManager.shared.saLowerBound)
//    for db in stride(from: 10, through: lower, by: 10) {
//        let y = CGFloat(db / lower) * bounds.height
//        if y >= bounds.height {
//            break
//        }
//        context.addLines(between: [CGPoint(x: 0, y: y), CGPoint(x: bounds.width-1, y: y)])
//
//        let string = String(Int(-db))+" dB"
//        string.draw(with: CGRect(x: 0, y: y, width: 448, height: 448), options: .usesLineFragmentOrigin, attributes: attrs, context: nil)
//    }
//    context.strokePath()
//}

- (void)drawSpectrumAnalyzer:(CGContextRef)context {
    CGFloat bw = NSWidth(self.bounds)/(CGFloat)NUM_BARS;
    CGFloat rgbaBar[] = {0,1,1,1}; // bars color
    CGColorRef cBar = CGColorCreate(CGColorSpaceCreateDeviceRGB(), rgbaBar);
    CGContextSetFillColorWithColor(context, cBar);
    CFRelease(cBar);
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

    CGFloat rgbaPeak[] = {1,1,1,1}; // peaks color
    CGColorRef cPeak = CGColorCreate(CGColorSpaceCreateDeviceRGB(), rgbaPeak);
    CGContextSetFillColorWithColor(context, cPeak);
    CFRelease(cPeak);
    CGContextFillPath(context);

//    if SettingsManager.shared.saShowGrid {
//        drawSaGrid (context:context)
//    }
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
