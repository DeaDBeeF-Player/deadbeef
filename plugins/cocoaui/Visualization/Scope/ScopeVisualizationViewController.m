//
//  ScopeVisualizationViewController.m
//  deadbeef
//
//  Created by Alexey Yakovenko on 30/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#import "ScopeVisualizationViewController.h"

@implementation ScopeVisualizationViewController

- (void)awakeFromNib {
    [super awakeFromNib];

    NSMenu *menu = [NSMenu new];
    NSMenuItem *renderModeMenuItem = [menu addItemWithTitle:@"Rendering Mode" action:nil keyEquivalent:@""];
    NSMenuItem *scaleModeMenuItem = [menu addItemWithTitle:@"Scale" action:nil keyEquivalent:@""];
    NSMenuItem *fragmentDurationMenuItem = [menu addItemWithTitle:@"Fragment Duration" action:nil keyEquivalent:@""];

    renderModeMenuItem.submenu = [NSMenu new];
    scaleModeMenuItem.submenu = [NSMenu new];
    fragmentDurationMenuItem.submenu = [NSMenu new];

    [renderModeMenuItem.submenu addItemWithTitle:@"Multichannel" action:@selector(setMultichannelRenderingMode:) keyEquivalent:@""];
    [renderModeMenuItem.submenu addItemWithTitle:@"Mono" action:@selector(setMonoRenderingMode:) keyEquivalent:@""];

    [scaleModeMenuItem.submenu addItemWithTitle:@"Auto" action:@selector(setScaleAuto:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"1x" action:@selector(setScale1x:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"2x" action:@selector(setScale2x:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"3x" action:@selector(setScale3x:) keyEquivalent:@""];
    [scaleModeMenuItem.submenu addItemWithTitle:@"4x" action:@selector(setScale4x:) keyEquivalent:@""];

    [fragmentDurationMenuItem.submenu addItemWithTitle:@"50 ms" action:@selector(setFragmentDuration50ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"100 ms" action:@selector(setFragmentDuration100ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"200 ms" action:@selector(setFragmentDuration200ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"300 ms" action:@selector(setFragmentDuration300ms:) keyEquivalent:@""];
    [fragmentDurationMenuItem.submenu addItemWithTitle:@"500 ms" action:@selector(setFragmentDuration500ms:) keyEquivalent:@""];

    self.view.menu = menu;
}

#pragma mark - Actions

- (void)setMultichannelRenderingMode:(NSMenuItem *)sender {
    self.settings.renderMode = DDB_SCOPE_MULTICHANNEL;
}

- (void)setMonoRenderingMode:(NSMenuItem *)sender {
    self.settings.renderMode = DDB_SCOPE_MONO;
}

- (void)setScaleAuto:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleModeAuto;
}

- (void)setScale1x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode1x;
}

- (void)setScale2x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode2x;
}

- (void)setScale3x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode3x;
}

- (void)setScale4x:(NSMenuItem *)sender {
    self.settings.scaleMode = ScopeScaleMode4x;
}

- (void)setFragmentDuration50ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration50;
}

- (void)setFragmentDuration100ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration100;
}

- (void)setFragmentDuration200ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration200;
}

- (void)setFragmentDuration300ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration300;
}

- (void)setFragmentDuration500ms:(NSMenuItem *)sender {
    self.settings.fragmentDuration = ScopeFragmentDuration500;
}

@end
