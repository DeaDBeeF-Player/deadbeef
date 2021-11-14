//
//  SpectrumAnalyzerPreferencesWindowController.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SpectrumAnalyzerSettings;

@protocol SpectrumAnalyzerPreferencesDelegate;

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumAnalyzerPreferencesWindowController : NSWindowController

@property (nonatomic,weak) id<SpectrumAnalyzerPreferencesDelegate> preferencesDelegate;
@property (nonatomic) SpectrumAnalyzerSettings *settings;

@end

NS_ASSUME_NONNULL_END
