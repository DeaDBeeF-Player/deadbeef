/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#import "SpectrumAnalyzerLabelsView.h"
#import "VisualizationSettingsUtil.h"

#define LOWER_BOUND -80

@interface SpectrumAnalyzerLabelsView() {
    ddb_analyzer_draw_data_t *_draw_data;
}

@property (nonatomic) SpectrumAnalyzerSettings *settings;
@property (nonatomic) NSDictionary *textAttrs;

@end

@implementation SpectrumAnalyzerLabelsView

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
    NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
    paragraphStyle.alignment = NSTextAlignmentLeft;

    NSColor *gridColor = [NSColor.whiteColor colorWithAlphaComponent:0.4];

    self.textAttrs = @{
        NSFontAttributeName: [NSFont fontWithName:@"HelveticaNeue" size:10],
        NSParagraphStyleAttributeName: paragraphStyle,
        NSForegroundColorAttributeName: gridColor
    };
}

- (void)drawFrequencyLabels {
    CGFloat xOffset = self.subviews[0].frame.origin.x;

    // octaves text
    for (int i = 0; i < _draw_data->label_freq_count; i++) {
        if (_draw_data->label_freq_positions < 0) {
            continue;
        }
        NSString *string = @(_draw_data->label_freq_texts[i]);
        CGFloat x = xOffset + _draw_data->label_freq_positions[i];
        [string drawAtPoint:NSMakePoint(x / self.window.backingScaleFactor, NSHeight(self.bounds)-12) withAttributes:self.textAttrs];
    }
}

- (void)drawDBLabels {
    CGFloat visHeight = self.subviews[0].frame.size.height;
    CGFloat yOffset = self.subviews[0].frame.origin.y + visHeight;
    CGFloat xSpace = self.subviews[0].frame.origin.x;
    float saLowerBound;
    saLowerBound = LOWER_BOUND;
    CGFloat lower = -floor(saLowerBound);
    for (int db = 0; db < lower; db += 10) {
        CGFloat y = (CGFloat)(db / lower) * visHeight;
        if (y >= NSHeight(self.bounds)) {
            break;
        }

        NSString *string = [NSString stringWithFormat:@"%d dB", -db];
        NSSize extents = [string sizeWithAttributes:self.textAttrs];
        [string drawAtPoint:NSMakePoint(xSpace - extents.width - 4, yOffset - y - 12) withAttributes:self.textAttrs];
    }
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

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    [self.backgroundColor setFill];
    NSRectFill(dirtyRect);

    if (_draw_data == NULL) {
        return;
    }

    [self drawFrequencyLabels];
    [self drawDBLabels];
}

- (void)updateDrawData:(ddb_analyzer_draw_data_t *)drawData {
    if (_draw_data == NULL) {
        self.needsDisplay = YES;
    }
    _draw_data = drawData;
}

- (void)updateSettings:(SpectrumAnalyzerSettings * _Nonnull)settings {
    self.settings = settings;
    self.needsDisplay = YES;
}

@end
