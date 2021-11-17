//
//  VisualizationSettingsUtil.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 13/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationSettingsUtil : NSObject

+ (instancetype)shared;

@property (nonatomic) NSColor *baseColor;
@property (nonatomic) NSColor *backgroundColor;
@property (nonatomic) NSColor *spectrumAnalyzerPeakColor;
@property (nonatomic) NSColor *spectrumAnalyzerBarColor;

- (nullable NSString *)stringForColor:(NSColor *)color;
- (nullable NSColor *)colorForString:(NSString *)colorString;

@end

NS_ASSUME_NONNULL_END
