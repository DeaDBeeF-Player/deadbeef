//
//  ScopePreferencesWindowController.m
//  DeaDBeeF
//
//  Created by Oleksiy Yakovenko on 14/11/2021.
//  Copyright © 2021 Oleksiy Yakovenko. All rights reserved.
//

#import "ScopePreferencesViewController.h"
#import "ScopePreferencesWindowController.h"

@interface ScopePreferencesWindowController ()

@property (nonatomic) ScopePreferencesViewController *viewController;

@end

@implementation ScopePreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    self.viewController = [ScopePreferencesViewController new];
    self.viewController.settings = self.settings;

    self.contentViewController = self.viewController;
}

@end
