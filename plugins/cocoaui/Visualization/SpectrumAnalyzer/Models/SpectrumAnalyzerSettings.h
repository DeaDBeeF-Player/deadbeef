//
//  SpectrumAnalyzerSettings.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 28/07/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "analyzer.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumAnalyzerSettings : NSObject

@property (nonatomic) ddb_analyzer_mode_t mode;
@property (nonatomic) int distanceBetweenBars; // WIDTH/x
@property (nonatomic) int barGranularity; // 24/x

@property (nonatomic) BOOL useCustomPeakColor;
@property (nonatomic) BOOL useCustomBarColor;
@property (nonatomic) BOOL useCustomBackgroundColor;
@property (nonatomic) NSColor *customPeakColor;
@property (nonatomic) NSColor *customBarColor;
@property (nonatomic) NSColor *customBackgroundColor;

@end

NS_ASSUME_NONNULL_END
