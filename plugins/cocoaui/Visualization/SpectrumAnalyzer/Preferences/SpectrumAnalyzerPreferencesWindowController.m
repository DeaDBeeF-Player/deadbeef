//
//  SpectrumAnalyzerPreferencesWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 14/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "SpectrumAnalyzerPreferencesViewController.h"
#import "SpectrumAnalyzerPreferencesWindowController.h"

@interface SpectrumAnalyzerPreferencesWindowController ()

@property (nonatomic) SpectrumAnalyzerPreferencesViewController *viewController;

@end

@implementation SpectrumAnalyzerPreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    self.viewController = [SpectrumAnalyzerPreferencesViewController new];
    self.viewController.settings = self.settings;

    self.contentViewController = self.viewController;
}

@end
