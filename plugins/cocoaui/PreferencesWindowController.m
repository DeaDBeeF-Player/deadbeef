/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

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

- (NSString *)cfgFormattedColorForName:(NSString *)colorName {
    NSColorList *clist = [NSColorList colorListNamed:@"System"];
    return [self cfgFormattedColor:[clist colorWithKey:colorName]];
}

- (NSString *)cfgFormattedColor:(NSColor *)color {
    CGFloat r, g, b, a;
    [[color colorUsingColorSpaceName:NSDeviceRGBColorSpace] getRed:&r green:&g blue:&b alpha:&a];
    return [NSString stringWithFormat:@"#%02x%02x%02x%02x", (int)(r*255), (int)(g*255), (int)(b*255), (int)(a*255)];
}

- (NSColor *)getConfigColor:(NSString *)key withDefault:(NSString *)def {
    char buf[10];
    deadbeef->conf_get_str ([key UTF8String], [def UTF8String], buf, sizeof (buf));
    int r, g, b, a;
    if (4 != sscanf (buf, "#%02x%02x%02x%02x", &r, &g, &b, &a)) {
        return [NSColor blackColor];
    }
    return [NSColor colorWithDeviceRed:r/255.f green:g/255.f blue:b/255.f alpha:a/255.f];
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
    [_titlebar_playing setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.titlebar_playing", DEFAULT_TITLEBAR_PLAYING_VALUE)]];
    [_titlebar_stopped setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_STOPPED_VALUE)]];

    // gui/misc -> playlist
    [_mmb_delete_playlist setState: deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1) ? NSOnState : NSOffState];
    [_hide_remove_from_disk setState: deadbeef->conf_get_int ("cocoaui.hide_remove_from_disk", 0) ? NSOnState : NSOffState];
    [_name_playlist_from_folder setState: deadbeef->conf_get_int ("cocoaui.name_playlist_from_folder", 1) ? NSOnState : NSOffState];
    [_autoresize_columns setState: deadbeef->conf_get_int ("cocoaui.autoresize_columns", 0) ? NSOnState : NSOffState];

    // appearance for seekbar / volumebar
    [_override_bar_colors setState: deadbeef->conf_get_int ("cocoaui.override_bar_colors", 0) ? NSOnState : NSOffState];

    // make the config strings with defaults
    NSString *cfg_textcolor = [self cfgFormattedColorForName:@"controlTextColor"];
    NSString *cfg_shadowcolor = [self cfgFormattedColorForName:@"controlShadowColor"];
    NSString *cfg_selectedtextcolor = [self cfgFormattedColorForName:@"alternateSelectedControlTextColor"];
    NSString *cfg_evenrowcolor = [self cfgFormattedColor:[NSColor controlAlternatingRowBackgroundColors][0]];
    NSString *cfg_oddrowcolor = [self cfgFormattedColor:[NSColor controlAlternatingRowBackgroundColors][1]];
    NSString *cfg_selectedrowcolor = [self cfgFormattedColorForName:@"alternateSelectedControlColor"];

    [_color_bar_foreground setColor:[self getConfigColor:@"cocoaui.color.bar_foreground" withDefault:cfg_textcolor]];
    [_color_bar_background setColor:[self getConfigColor:@"cocoaui.color.bar_background" withDefault:cfg_shadowcolor]];

    // appearance for playlist
    [_override_playlist_colors setState: deadbeef->conf_get_int ("cocoaui.override_listview_colors", 0) ? NSOnState : NSOffState];
    
    [_color_listview_text setColor: [self getConfigColor:@"cocoaui.color.listview_text" withDefault:cfg_textcolor]];
    [_color_listview_playing_text setColor: [self getConfigColor:@"cocoaui.color.listview_playing_text" withDefault:cfg_textcolor]];
    [_color_listview_selected_text setColor: [self getConfigColor:@"cocoaui.color.listview_selected_text" withDefault:cfg_selectedtextcolor]];
    [_color_listview_group_header_text setColor: [self getConfigColor:@"cocoaui.color.listview_group_header_text" withDefault:cfg_textcolor]];
    [_color_listview_cursor setColor: [self getConfigColor:@"cocoaui.color.cursor" withDefault:cfg_textcolor]];
    [_color_listview_even_background setColor: [self getConfigColor:@"cocoaui.color.listview_even_background" withDefault:cfg_evenrowcolor]];
    [_color_listview_odd_background setColor: [self getConfigColor:@"cocoaui.color.listview_odd_background" withDefault:cfg_oddrowcolor]];
    [_color_listview_selected_background setColor: [self getConfigColor:@"cocoaui.color.listview_selected_background" withDefault:cfg_selectedrowcolor]];

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
        int y = h - (unit_h + unit_spacing) * i - unit_h - 4;

        int label_w = 0;

        // set label
        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_SELECT:
            case PROP_SLIDER:
            case PROP_FILE:
            {
                NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(0, y-2, 500, unit_h)];
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
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width-label_w - label_padding - 4, unit_h);
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [[NSSecureTextField alloc] initWithFrame:frame] : [[NSTextField alloc] initWithFrame:frame];
                [tf setStringValue:[NSString stringWithUTF8String:deadbeef->conf_get_str_fast (_settingsData.props[i].key, _settingsData.props[i].def)]];
                if (_settingsData.props[i].type == PROP_FILE) {
                    [tf setEditable:NO];
                }
                [v addSubview:tf];
                break;
            }
            case PROP_CHECKBOX:
            {
                NSRect frame = NSMakeRect(0, y, sz.width - 4, unit_h);
                NSButton *checkbox = [[NSButton alloc] initWithFrame:frame];
                [checkbox setButtonType:NSSwitchButton];
                [checkbox setTitle:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [checkbox setState:deadbeef->conf_get_int (_settingsData.props[i].key, atoi(_settingsData.props[i].def)) ? NSOnState : NSOffState];
                [v addSubview:checkbox];
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
