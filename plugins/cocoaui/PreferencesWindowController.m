//
//  PreferencesWindowController.m
//  deadbeef
//
//  Created by waker on 18/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import "PreferencesWindowController.h"
#include "deadbeef.h"
#include "pluginsettings.h"

extern DB_functions_t *deadbeef;

@interface PreferencesWindowController () {
    settings_data_t _settingsData;
}

@end

@implementation PreferencesWindowController

- (void)dealloc {
    settings_data_free (&_settingsData);
}

- (void)windowDidLoad {
    [super windowDidLoad];

    [_toolbar setDelegate:(id<NSToolbarDelegate>)self];
    [_toolbar setSelectedItemIdentifier:@"Playback"];

    [self initPluginList];

    [self setInitialValues];

    [self switchToView:_playbackView];
}

- (void)initPluginList {
    [_pluginList setDataSource:(id<NSTableViewDataSource>)self];
    [_pluginList setDelegate:(id<NSTableViewDelegate>)self];
    [self setPluginInfo:-1];
}

- (void)setInitialValues {
    [_replaygain_mode selectItemAtIndex: deadbeef->conf_get_int ("replaygain_mode", 0)];
    [_replaygain_scale setState: deadbeef->conf_get_int ("replaygain_scale", 1) ? NSOnState : NSOffState];
}

- (NSArray *)toolbarSelectableItemIdentifiers: (NSToolbar *)toolbar;
{
    return [NSArray arrayWithObjects:@"Playback",
            @"DSP",
            @"GUI",
            @"Appearance",
            @"Network",
            @"Plugins",
            nil];
}

- (void)switchToView:(NSView *)view {
    [[self window] setContentView:nil];

    NSRect oldFrame = [[self window] frame];
    NSRect rc = [[self window] frameRectForContentRect:[view frame]];
    rc.origin.x = oldFrame.origin.x;
    rc.origin.y = oldFrame.origin.y + oldFrame.size.height - rc.size.height;
    [[self window] setContentView:view];
    [[self window] setFrame:rc display:YES animate:YES];
}

- (IBAction)playbackAction:(id)sender {
    [self switchToView:_playbackView];
}

- (IBAction)appearanceAction:(id)sender {
    [self switchToView:_appearanceView];
}

- (IBAction)dspAction:(id)sender {
    [self switchToView:_dspView];
}

- (IBAction)guiAction:(id)sender {
    [self switchToView:_guiView];
}

- (IBAction)networkAction:(id)sender {
    [self switchToView:_networkView];
}

- (IBAction)pluginsAction:(id)sender {
    [self switchToView:_pluginsView];
}

- (IBAction)dspAddAction:(id)sender {
}

- (IBAction)dspRemoveAction:(id)sender {
}

- (IBAction)dspConfigureAction:(id)sender {
}

- (IBAction)dspMoveUpAction:(id)sender {
}

- (IBAction)dspMoveDownAction:(id)sender {
}

- (IBAction)dspSaveAction:(id)sender {
}

- (IBAction)dspLoadAction:(id)sender {
}

- (IBAction)networkEditContentTypeMapping:(id)sender {
}

- (IBAction)pluginOpenWebsite:(id)sender {
}

- (IBAction)pluginConfResetDefaults:(id)sender {
}

- (void)initPluginConfiguration:(NSInteger)idx {
    NSView *v = _pluginPropertiesView;

    DB_plugin_t *p = deadbeef->plug_get_list()[idx];

    while ([[v subviews] count] > 0) {
        [[[v subviews] lastObject] removeFromSuperview];
    }

    settings_data_free (&_settingsData);

    if (!p->configdialog || settings_data_init(&_settingsData, p->configdialog) < 0) {
        // TODO: show information that the plugin doesn't have settings
        return;
    }

    int label_padding = 8;
    int unit_spacing = 4;
    int unit_h = 22;
    int h = _settingsData.nprops * (unit_h + unit_spacing);

    NSSize sz = [_pluginPropertiesScroller contentSize];

    if (h < sz.height) {
        h = sz.height;
    }

    deadbeef->conf_lock ();
    for (int i = 0; i < _settingsData.nprops; i++) {
        int y = h - (unit_h + unit_spacing) * i - unit_h;

        int label_w = 0;

        // set label
        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_SELECT:
            case PROP_SLIDER:
            case PROP_FILE:
            {
                NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(0, y, 500, unit_h)];
                [lbl setStringValue:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [lbl setBezeled:NO];
                [lbl setDrawsBackground:NO];
                [lbl setEditable:NO];
                [lbl setSelectable:NO];

                // resize label to fit content
                [[lbl cell] setLineBreakMode:NSLineBreakByClipping];
                label_w = [[lbl cell] cellSizeForBounds:lbl.bounds].width;
                [lbl setFrameSize:NSMakeSize(label_w, unit_h)];
                label_w += label_padding;

                [v addSubview:lbl];
            }
        }

        // set entry
        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_FILE:
            {
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width-label_w, unit_h);
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [[NSSecureTextField alloc] initWithFrame:frame] : [[NSTextField alloc] initWithFrame:frame];
                [tf setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast (_settingsData.props[i].key, _settingsData.props[i].def)]];
                if (_settingsData.props[i].type == PROP_FILE) {
                    [tf setEditable:NO];
                }
                [v addSubview:tf];
                break;
            }
        }
    }
    deadbeef->conf_unlock ();
    NSRect frame = [v frame];
    [v setFrame:NSMakeRect(0, 0, frame.size.width, h)];
}

- (void)setPluginInfo:(NSInteger)idx {
    NSString *version = @"";
    NSString *description = @"";
    NSString *license = @"";

    if (idx != -1) {
        [_pluginUnselectedText setHidden:YES];
        [_pluginTabView setHidden:NO];
        DB_plugin_t **p = deadbeef->plug_get_list();
        version = [NSString stringWithFormat:@"%d.%d", p[idx]->version_major, p[idx]->version_minor];
        if (p[idx]->descr) {
            description = [NSString stringWithUTF8String:p[idx]->descr];
        }
        if (p[idx]->copyright) {
            license = [NSString stringWithUTF8String:p[idx]->copyright];
        }

        [self initPluginConfiguration:idx];
    }
    else {
        [_pluginUnselectedText setHidden:NO];
        [_pluginTabView setHidden:YES];
    }

    [_pluginVersion setStringValue:version];
    [_pluginDescription setString:description];
    [_pluginLicense setString:license];
}

// data source for plugin list
- (int)numberOfRowsInTableView:(NSTableView *)aTableView {
    DB_plugin_t **p = deadbeef->plug_get_list();
    int cnt;
    for (cnt = 0; p[cnt]; cnt++);

    return cnt;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex {
    DB_plugin_t **p = deadbeef->plug_get_list();

    return [NSString stringWithUTF8String: p[rowIndex]->name];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    [self setPluginInfo:[_pluginList selectedRow]];
}

@end
