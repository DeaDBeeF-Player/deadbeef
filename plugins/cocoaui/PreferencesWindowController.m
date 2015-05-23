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
    // playback
    [_replaygain_mode selectItemAtIndex: deadbeef->conf_get_int ("replaygain_mode", 0)];
    [_replaygain_scale setState: deadbeef->conf_get_int ("replaygain_scale", 1) ? NSOnState : NSOffState];
    [_replaygain_preamp setIntValue:deadbeef->conf_get_int ("replaygain_preamp", 0)];
    [_global_preamp setIntValue:deadbeef->conf_get_int ("global_preamp", 0)];
    [_cli_add_to_specific_playlist setState: deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1) ? NSOnState : NSOffState];
    [_cli_add_playlist_name setStringValue: [NSString stringWithUTF8String: deadbeef->conf_get_str_fast ("cli_add_playlist_name", "Default")]];
    [_resume_last_session setState: deadbeef->conf_get_int ("resume_last_session", 0) ? NSOnState : NSOffState];
    [_ignore_archives setState: deadbeef->conf_get_int ("ignore_archives", 1) ? NSOnState : NSOffState];
    [_stop_after_current_reset setState: deadbeef->conf_get_int ("playlist.stop_after_current_reset", 0) ? NSOnState : NSOffState];
    [_stop_after_album_reset setState: deadbeef->conf_get_int ("playlist.stop_after_album_reset", 0) ? NSOnState : NSOffState];

    // dsp

    // gui/misc -> player
    [_enable_shift_jis_detection setState: deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0) ? NSOnState : NSOffState];
    [_enable_cp1251_detection setState: deadbeef->conf_get_int ("junk.enable_cp1251_detection", 0) ? NSOnState : NSOffState];
    [_enable_cp936_detection setState: deadbeef->conf_get_int ("junk.enable_cp936_detection", 0) ? NSOnState : NSOffState];
    [_refresh_rate setIntValue: deadbeef->conf_get_int ("cocoaui.refresh_rate", 10)];
    [_titlebar_playing setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.titlebar_playing", "%artist% - %title% - DeaDBeeF-%version%")]];
    [_titlebar_stopped setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.titlebar_stopped", "DeaDBeeF-%version%")]];

    // gui/misc -> playlist
    [_mmb_delete_playlist setState: deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1) ? NSOnState : NSOffState];
    [_hide_remove_from_disk setState: deadbeef->conf_get_int ("cocoaui.hide_remove_from_disk", 0) ? NSOnState : NSOffState];
    [_name_playlist_from_folder setState: deadbeef->conf_get_int ("cocoaui.name_playlist_from_folder", 1) ? NSOnState : NSOffState];
    [_autoresize_columns setState: deadbeef->conf_get_int ("cocoaui.autoresize_columns", 0) ? NSOnState : NSOffState];

    // appearance for seekbar / volumebar
    [_override_bar_colors setState: deadbeef->conf_get_int ("cocoaui.override_bar_colors", 0) ? NSOnState : NSOffState];

    NSColorList *clist = [NSColorList colorListNamed:@"System"];
    [_color_bar_foreground setColor:[clist colorWithKey:@"controlTextColor"]];
    [_color_bar_background setColor:[clist colorWithKey:@"controlShadowColor"]];

    // appearance for playlist
    [_override_playlist_colors setState: deadbeef->conf_get_int ("cocoaui.override_listview_colors", 0) ? NSOnState : NSOffState];
    
    [_color_listview_text setColor: [clist colorWithKey:@"controlTextColor"]];
    [_color_listview_playing_text setColor: [clist colorWithKey:@"controlTextColor"]];
    [_color_listview_selected_text setColor: [clist colorWithKey:@"alternateSelectedControlTextColor"]];
    [_color_listview_group_header_text setColor: [clist colorWithKey:@"controlTextColor"]];
    [_color_listview_cursor setColor: [clist colorWithKey:@"controlTextColor"]];
    [_color_listview_even_background setColor: [NSColor controlAlternatingRowBackgroundColors][0]];
    [_color_listview_odd_background setColor: [NSColor controlAlternatingRowBackgroundColors][1]];
    [_color_listview_selected_background setColor: [clist colorWithKey:@"alternateSelectedControlColor"]];

    [_listview_bold_current_text setState: deadbeef->conf_get_int ("cocoaui.embolden_current_track", 0) ? NSOnState : NSOffState];
    [_listview_bold_selected_text setState: deadbeef->conf_get_int ("cocoaui.embolden_selected_tracks", 0) ? NSOnState : NSOffState];
    [_listview_italic_current_text setState: deadbeef->conf_get_int ("cocoaui.italic_current_track", 0) ? NSOnState : NSOffState];
    [_listview_italic_selected_text setState: deadbeef->conf_get_int ("cocoaui.italic_selected_tracks", 0) ? NSOnState : NSOffState];

    // network
    [_network_proxy setState: deadbeef->conf_get_int ("network.proxy", 0) ? NSOnState : NSOffState];
    [_network_proxy_address setStringValue:[NSString stringWithUTF8String: deadbeef->conf_get_str_fast ("network.proxy.address", "")]];
    [_network_proxy_port setStringValue:[NSString stringWithUTF8String: deadbeef->conf_get_str_fast ("network.proxy.port", "8080")]];
    const char *type = deadbeef->conf_get_str_fast ("network.proxy.type", "HTTP");
    if (!strcasecmp (type, "HTTP")) {
        [_network_proxy_type selectItemAtIndex:0];
    }
    else if (!strcasecmp (type, "HTTP_1_0")) {
        [_network_proxy_type selectItemAtIndex:1];
    }
    else if (!strcasecmp (type, "SOCKS4")) {
        [_network_proxy_type selectItemAtIndex:2];
    }
    else if (!strcasecmp (type, "SOCKS5")) {
        [_network_proxy_type selectItemAtIndex:3];
    }
    else if (!strcasecmp (type, "SOCKS4A")) {
        [_network_proxy_type selectItemAtIndex:4];
    }
    else if (!strcasecmp (type, "SOCKS5_HOSTNAME")) {
        [_network_proxy_type selectItemAtIndex:5];
    }
    [_network_proxy_username setStringValue: [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("network.proxy.username", "")]];
    [_network_proxy_password setStringValue: [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("network.proxy.password", "")]];
    [_network_http_user_agent setStringValue: [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("network.http_user_agent", "")]];
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
