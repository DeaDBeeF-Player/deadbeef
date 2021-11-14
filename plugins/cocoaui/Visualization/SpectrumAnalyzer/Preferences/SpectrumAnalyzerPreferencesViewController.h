//
//  SpectrumAnalyzerPreferencesViewController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SpectrumAnalyzerSettings;

NS_ASSUME_NONNULL_BEGIN

@protocol SpectrumAnalyzerPreferencesDelegate
- (void)spectrumAnalyzerPreferencesUseCustomPeakColorDidChange:(BOOL)enabled;
- (void)spectrumAnalyzerPreferencesUseCustomBarColorDidChange:(BOOL)enabled;
- (void)spectrumAnalyzerPreferencesCustomPeakColorDidChange:(NSColor *)color;
- (void)spectrumAnalyzerPreferencesCustomBarColorDidChange:(NSColor *)color;
@end

@interface SpectrumAnalyzerPreferencesViewController : NSViewController

@property (nonatomic, weak) id<SpectrumAnalyzerPreferencesDelegate> delegate;
@property (nonatomic) SpectrumAnalyzerSettings *settings;


@end

NS_ASSUME_NONNULL_END
