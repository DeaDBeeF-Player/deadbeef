//
//  ScopePreferencesWindowController.m
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 14/11/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "ScopePreferencesViewController.h"
#import "ScopePreferencesWindowController.h"

@interface ScopePreferencesWindowController ()

@property (strong) IBOutlet ScopePreferencesViewController *viewController;

@end

@implementation ScopePreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    self.viewController.settings = self.settings;
}

@end
