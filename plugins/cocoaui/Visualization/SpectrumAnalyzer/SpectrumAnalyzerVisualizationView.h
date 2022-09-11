//
//  SpectrumAnalyzerVisualizationView.h
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 7/25/20.
//  Copyright © 2020 Oleksiy Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SpectrumAnalyzerSettings.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumAnalyzerVisualizationView : NSView

- (void)updateAnalyzerSettings:(SpectrumAnalyzerSettings *)settings;

@property (nonatomic) NSColor *baseColor;
@property (nonatomic) NSColor *backgroundColor;

@end

NS_ASSUME_NONNULL_END
