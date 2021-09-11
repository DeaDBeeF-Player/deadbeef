//
//  VisualizationView.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 7/25/20.
//  Copyright © 2020 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SpectrumAnalyzerSettings.h"

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationView : NSView

- (void)updateAnalyzerSettings:(SpectrumAnalyzerSettings *)settings;

@end

NS_ASSUME_NONNULL_END
