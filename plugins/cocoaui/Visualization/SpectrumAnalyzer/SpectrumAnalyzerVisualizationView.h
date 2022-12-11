//
//  SpectrumAnalyzerVisualizationView.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/25/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SpectrumAnalyzerSettings.h"
#include "analyzer.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumAnalyzerVisualizationView : NSView

- (void)updateSettings:(SpectrumAnalyzerSettings * _Nonnull)settings;
- (void)updateDrawData:(ddb_analyzer_draw_data_t *)drawData;

@property (nonatomic) NSColor *baseColor;
@property (nonatomic) NSColor *backgroundColor;

@end

NS_ASSUME_NONNULL_END
