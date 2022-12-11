//
//  SpectrumAnalyzerVisualizationView.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/25/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import "AAPLNSView.h"
#import "AAPLView.h"
#import "SpectrumAnalyzerVisualizationView.h"
#import "VisualizationSettingsUtil.h"
#include "analyzer.h"
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

#define LOWER_BOUND -80

@interface SpectrumAnalyzerVisualizationView() {
    float saLowerBound;
    ddb_analyzer_draw_data_t *_draw_data;
}

@property (nonatomic) SpectrumAnalyzerSettings *settings;

@property (nonatomic,readonly) NSColor *barColor;
@property (nonatomic,readonly) NSColor *peakColor;
@property (nonatomic) NSColor *gridColor;

@end

@implementation SpectrumAnalyzerVisualizationView

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
    saLowerBound = LOWER_BOUND;

    self.gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.4];

}

- (NSColor *)barColor {
    if (self.settings.useCustomBarColor) {
        NSColor *color = self.settings.customBarColor;
        if (color != nil) {
            return color;
        }
    }
    NSColor *tempColor = [self.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat h, s, b, a;
    [tempColor getHue:&h saturation:&s brightness:&b alpha:&a];
    return [NSColor colorWithHue:h saturation:s brightness:b*0.7 alpha:1];
}

- (NSColor *)peakColor {
    if (self.settings.useCustomPeakColor) {
        NSColor *color = self.settings.customPeakColor;
        if (color != nil) {
            return color;
        }
    }
    NSColor *tempColor = [self.baseColor colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    CGFloat h, s, b, a;
    [tempColor getHue:&h saturation:&s brightness:&b alpha:&a];

    return [NSColor colorWithHue:h saturation:s*0.7 brightness:b*1.1 alpha:1];
}

- (NSColor *)backgroundColor {
    if (self.settings.useCustomBackgroundColor) {
        NSColor *color = self.settings.customBackgroundColor;
        if (color != nil) {
            return color;
        }
    }

    return VisualizationSettingsUtil.shared.backgroundColor;
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
}


- (void)drawRect:(NSRect)dirtyRect {
    if (_draw_data == NULL) {
        return;
    }

    [self drawSaGrid];
    [self drawAnalyzer];
}

- (void)drawAnalyzerDiscreteFrequencies {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    ddb_analyzer_draw_bar_t *bar = _draw_data->bars;
    for (int i = 0; i < _draw_data->bar_count; i++, bar++) {
            CGContextMoveToPoint(context, bar->xpos, 0);
            CGContextAddLineToPoint(context, bar->xpos, bar->bar_height);
    }
    CGContextSetStrokeColorWithColor(context, self.barColor.CGColor);
    CGContextStrokePath(context);

    bar = _draw_data->bars;
    for (int i = 0; i < _draw_data->bar_count; i++, bar++) {
        CGContextMoveToPoint(context, bar->xpos-0.5, bar->peak_ypos);
        CGContextAddLineToPoint(context, bar->xpos+0.5, bar->peak_ypos);
    }
    CGContextSetStrokeColorWithColor(context, self.peakColor.CGColor);
    CGContextStrokePath(context);
}

- (void)drawAnalyzerOctaveBands {
    CGContextRef context = NSGraphicsContext.currentContext.CGContext;
    ddb_analyzer_draw_bar_t *bar = _draw_data->bars;
    for (int i = 0; i < _draw_data->bar_count; i++, bar++) {
        CGContextAddRect(context, CGRectMake(bar->xpos, 0, _draw_data->bar_width, bar->bar_height));
    }
    CGContextSetFillColorWithColor(context, self.barColor.CGColor);
    CGContextFillPath(context);

    bar = _draw_data->bars;
    for (int i = 0; i < _draw_data->bar_count; i++, bar++) {
        CGContextAddRect(context, CGRectMake(bar->xpos, bar->peak_ypos, _draw_data->bar_width, 1));
    }
    CGContextSetFillColorWithColor(context, self.peakColor.CGColor);
    CGContextFillPath(context);
}


- (void)drawAnalyzer {
    if (_draw_data->mode == DDB_ANALYZER_MODE_FREQUENCIES) {
        [self drawAnalyzerDiscreteFrequencies];
    }
    else {
        [self drawAnalyzerOctaveBands];
    }
}

- (void)updateDrawData:(ddb_analyzer_draw_data_t *)drawData {
    _draw_data = drawData;
}

- (void)updateSettings:(SpectrumAnalyzerSettings * _Nonnull)settings {
    self.settings = settings;
}

@end
