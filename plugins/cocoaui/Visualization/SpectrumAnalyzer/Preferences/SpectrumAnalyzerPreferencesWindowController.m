//
//  SpectrumAnalyzerPreferencesWindowController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "SpectrumAnalyzerPreferencesViewController.h"
#import "SpectrumAnalyzerPreferencesWindowController.h"

@interface SpectrumAnalyzerPreferencesWindowController ()

@property (nonatomic) SpectrumAnalyzerPreferencesViewController *viewController;

@end

@implementation SpectrumAnalyzerPreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    self.viewController = [[SpectrumAnalyzerPreferencesViewController alloc] initWithNibName:@"SpectrumAnalyzerPreferencesViewController" bundle:nil];
    self.viewController.settings = self.settings;

    self.contentViewController = self.viewController;
}

@end
