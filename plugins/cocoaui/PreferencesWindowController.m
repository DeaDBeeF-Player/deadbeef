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
#import "DSPChainDataSource.h"
#include "deadbeef.h"
#include "pluginsettings.h"
#include "parser.h"

extern DB_functions_t *deadbeef;

@interface PreferencesWindowController () {
    settings_data_t _settingsData;
    NSMutableArray *_bindings;
    DSPChainDataSource *_dspChainDataSource;
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
    [_replaygain_source_mode selectItemAtIndex: deadbeef->conf_get_int ("replaygain.source_mode", 0)];

    int processing_idx = 0;
    int processing_flags = deadbeef->conf_get_int ("replaygain.processing_flags", 0);
    if (processing_flags == DDB_RG_PROCESSING_GAIN) {
        processing_idx = 1;
    }
    else if (processing_flags == (DDB_RG_PROCESSING_GAIN|DDB_RG_PROCESSING_PREVENT_CLIPPING)) {
        processing_idx = 2;
    }
    else if (processing_flags == DDB_RG_PROCESSING_PREVENT_CLIPPING) {
        processing_idx = 3;
    }

    [_replaygain_processing selectItemAtIndex:processing_idx];
    [_replaygain_preamp_with_rg setFloatValue:deadbeef->conf_get_float ("replaygain.preamp_with_rg", 0)];
    [_replaygain_preamp_without_rg setFloatValue:deadbeef->conf_get_float ("replaygain.preamp_without_rg", 0)];
    [self updateRGLabels];
    [_cli_add_to_specific_playlist setState: deadbeef->conf_get_int ("cli_add_to_specific_playlist", 1) ? NSOnState : NSOffState];
    [_cli_add_playlist_name setStringValue: [NSString stringWithUTF8String: deadbeef->conf_get_str_fast ("cli_add_playlist_name", "Default")]];
    [_resume_last_session setState: deadbeef->conf_get_int ("resume_last_session", 0) ? NSOnState : NSOffState];
    [_ignore_archives setState: deadbeef->conf_get_int ("ignore_archives", 1) ? NSOnState : NSOffState];
    [_stop_after_current_reset setState: deadbeef->conf_get_int ("playlist.stop_after_current_reset", 0) ? NSOnState : NSOffState];
    [_stop_after_album_reset setState: deadbeef->conf_get_int ("playlist.stop_after_album_reset", 0) ? NSOnState : NSOffState];

    // dsp
    _dspChainDataSource = [[DSPChainDataSource alloc] initWithChain:deadbeef->streamer_get_dsp_chain ()];
    [_dspList setDataSource:(id<NSTableViewDataSource>)_dspChainDataSource];

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

- (NSMenu *)getDSPMenu {
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"DspChainMenu"];
    [menu setDelegate:(id<NSMenuDelegate>)self];
    [menu setAutoenablesItems:NO];

    DB_dsp_t **plugins = deadbeef->plug_get_dsp_list ();

    for (int i = 0; plugins[i]; i++) {
        [[menu insertItemWithTitle:[NSString stringWithUTF8String:plugins[i]->plugin.name] action:@selector(addDspNode:) keyEquivalent:@"" atIndex:i] setTarget:self];
    }

    return menu;
}

- (void)addDspNode:(id)sender {
    NSMenuItem *item = sender;
    const char *name = [[item title] UTF8String];
    DB_dsp_t **plugins = deadbeef->plug_get_dsp_list ();

    for (int i = 0; plugins[i]; i++) {
        if (!strcmp (plugins[i]->plugin.name, name))
        {
            [_dspChainDataSource addItem:plugins[i]];
            [_dspList reloadData];
            break;
        }
    }
}

- (IBAction)dspAddAction:(id)sender {
    NSMenu *menu = [self getDSPMenu];
    [NSMenu popUpContextMenu:menu withEvent:[NSApp currentEvent] forView:sender];
}

- (IBAction)dspRemoveAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }

    [_dspChainDataSource removeItemAtIndex:(int)index];
    [_dspList reloadData];

    if (index >= [_dspList numberOfRows]) {
        index--;
    }
    if (index >= 0) {
        [_dspList selectRowIndexes:[NSIndexSet indexSetWithIndex:index] byExtendingSelection:NO];
    }
}

- (IBAction)dspConfigureAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }
    ddb_dsp_context_t *dsp = [_dspChainDataSource getItemAtIndex:(int)index];
    [self initPluginConfiguration:dsp->plugin->configdialog inView:_dspConfigView dsp:dsp];
    [NSApp beginSheet:_dspConfigPanel modalForWindow:[self window] modalDelegate:self didEndSelector:@selector(didEndDspConfigPanel:returnCode:contextInfo:) contextInfo:nil];
}

- (void)didEndDspConfigPanel:(NSWindow *)sheet returnCode:(NSInteger)returnCode contextInfo:(void *)contextInfo {
    if (returnCode == NSOKButton) {
        NSInteger index = [_dspList selectedRow];
        if (index < 0) {
            return;
        }
        ddb_dsp_context_t *dsp = [_dspChainDataSource getItemAtIndex:(int)index];
        [self savePluginConfiguration:dsp];
    }

    [_dspConfigPanel orderOut:self];
}

- (IBAction)dspConfigCancelAction:(id)sender {
    [NSApp endSheet:_dspConfigPanel returnCode:NSCancelButton];
}

- (IBAction)dspConfigOkAction:(id)sender {
    [NSApp endSheet:_dspConfigPanel returnCode:NSOKButton];
}

- (IBAction)dspConfigResetAction:(id)sender {
    NSInteger index = [_dspList selectedRow];
    if (index < 0) {
        return;
    }
    ddb_dsp_context_t *dsp = [_dspChainDataSource getItemAtIndex:(int)index];
    [self resetPluginConfigToDefaults:dsp];
}

- (IBAction)dspChainAction:(id)sender {
    NSInteger selectedSegment = [sender selectedSegment];

    switch (selectedSegment) {
    case 0:
        [self dspAddAction:sender];
        break;
    case 1:
        [self dspRemoveAction:sender];
        break;
    case 2:
        [self dspConfigureAction:sender];
        break;
    }
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
    [self resetPluginConfigToDefaults:NULL];
}

- (void)savePluginConfiguration:(ddb_dsp_context_t *)dsp {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"propname"]) {
            id sender = binding[@"sender"];
            NSString *value;
            if (binding[@"isInteger"] && [binding[@"isInteger"] boolValue]) {
                value = [@([sender integerValue]) stringValue];
            }
            else if ([sender isKindOfClass:[NSPopUpButton class]]) {
                value = [@([sender indexOfSelectedItem]) stringValue];
            }
            else {
                value = [sender stringValue];
            }
            const char *propname = [binding[@"propname"] UTF8String];
            const char *svalue = [value UTF8String];
            if (dsp) {
                dsp->plugin->set_param (dsp, atoi (propname), svalue);
            }
            else {
                deadbeef->conf_set_str (propname, svalue);
            }
        }
    }
    if (dsp) {
        [_dspChainDataSource apply];
    }
    else {
        deadbeef->conf_save ();
    }
}

- (void)resetPluginConfigToDefaults:(ddb_dsp_context_t *)dsp {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] && binding[@"default"]) {
            id sender = binding[@"sender"];
            if ([sender isKindOfClass:[NSPopUpButton class]]) {
                [sender selectItemAtIndex:[binding[@"default"] intValue]];
            }
            else {
                [sender setStringValue:binding[@"default"]];
            }
        }
    }
}

- (void)initPluginConfiguration:(const char *)config inView:(NSView *)view dsp:(ddb_dsp_context_t *)dsp {
    _bindings = [[NSMutableArray alloc] init];
    
    NSScrollView *scrollView = (NSScrollView *)view;
    view = [scrollView documentView];
    
    while ([[view subviews] count] > 0) {
        [[[view subviews] lastObject] removeFromSuperview];
    }
    
    settings_data_free (&_settingsData);

    BOOL have_settings = YES;
    if (!config || settings_data_init(&_settingsData, config) < 0) {
        have_settings = NO;
    }

    int label_padding = 8;
    int unit_spacing = 4;
    int unit_h = 22;
    int h = _settingsData.nprops * (unit_h + unit_spacing);
    
    NSSize sz = [scrollView contentSize];
    
    if (h < sz.height) {
        h = sz.height;
    }
    NSRect frame = [view frame];
    [view setFrame:NSMakeRect(0, 0, frame.size.width, h)];

    NSPoint pt = NSMakePoint(0.0, [[scrollView documentView]
                                   bounds].size.height);
    [[scrollView documentView] scrollPoint:pt];

    sz = [scrollView contentSize];

    if (!have_settings) {
        NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(0, sz.height/2 - unit_h/2, sz.width, unit_h)];
        [lbl setStringValue:@"This plugin doesn't have settings"];
        [lbl setAlignment:NSCenterTextAlignment];
        [lbl setBezeled:NO];
        [lbl setDrawsBackground:NO];
        [lbl setEditable:NO];
        [lbl setSelectable:NO];
        [view addSubview:lbl];
        return;
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
                NSTextField *lbl = [[NSTextField alloc] initWithFrame:NSMakeRect(4, y-2, 500, unit_h)];
                [lbl setStringValue:[[NSString stringWithUTF8String:_settingsData.props[i].title] stringByAppendingString:@":"]];
                [lbl setBezeled:NO];
                [lbl setDrawsBackground:NO];
                [lbl setEditable:NO];
                [lbl setSelectable:NO];
                
                // resize label to fit content
                [[lbl cell] setLineBreakMode:NSLineBreakByClipping];
                label_w = [[lbl cell] cellSizeForBounds:lbl.bounds].width;
                [lbl setFrameSize:NSMakeSize(label_w, unit_h)];
                label_w += label_padding;
                
                [view addSubview:lbl];
            }
        }
        
        // set entry
        char value[1000];
        if (dsp) {
            int param = atoi (_settingsData.props[i].key);
            dsp->plugin->get_param (dsp, param, value, sizeof (value));
        }
        else {
            deadbeef->conf_get_str (_settingsData.props[i].key, _settingsData.props[i].def, value, sizeof (value));
        }
        NSString *propname = [NSString stringWithUTF8String:_settingsData.props[i].key];

        switch (_settingsData.props[i].type) {
            case PROP_ENTRY:
            case PROP_PASSWORD:
            case PROP_FILE:
            {
                int right_offs = 0;
                if (_settingsData.props[i].type == PROP_FILE) {
                    right_offs = 24;
                }
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width-label_w - label_padding - 4 - right_offs, unit_h);
                NSTextField *tf = _settingsData.props[i].type == PROP_PASSWORD ? [[NSSecureTextField alloc] initWithFrame:frame] : [[NSTextField alloc] initWithFrame:frame];
                [tf setUsesSingleLineMode:YES];
                [tf setStringValue:[NSString stringWithUTF8String:value]];
                [view addSubview:tf];
                [_bindings addObject:@{@"sender":tf,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                break;
            }
            case PROP_CHECKBOX:
            {
                NSRect frame = NSMakeRect(4, y, sz.width - 4, unit_h);
                NSButton *checkbox = [[NSButton alloc] initWithFrame:frame];
                [checkbox setButtonType:NSSwitchButton];
                [checkbox setTitle:[NSString stringWithUTF8String:_settingsData.props[i].title]];
                [checkbox setState:atoi(value) ? NSOnState : NSOffState];
                [view addSubview:checkbox];

                [_bindings addObject:@{@"sender":checkbox,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];
                break;
            }
            case PROP_SLIDER:
            {
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width - (label_w + label_padding) - 72, unit_h);
                NSSlider *slider = [[NSSlider alloc] initWithFrame:frame];
                const char *opts = _settingsData.props[i].select_options;
                float min, max, step;
                sscanf (opts, "%f,%f,%f", &min, &max, &step);
                [slider setMinValue:min];
                [slider setMaxValue:max];
                [slider setContinuous:YES];
                [slider setIntValue:atoi(value)];
                
                frame = NSMakeRect(label_w + sz.width-label_w - label_padding - 64, y, 68, unit_h);
                NSTextField *valueedit = [[NSTextField alloc] initWithFrame:frame];
                [valueedit setStringValue:[NSString stringWithUTF8String: value]];
                [valueedit setEditable:YES];

                [_bindings addObject:@{@"sender":slider,
                                       @"propname":propname,
                                       @"valueview":valueedit,
                                       @"isInteger":[NSNumber numberWithBool:YES],
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [_bindings addObject:@{@"sender":valueedit,
                                       @"propname":propname,
                                       @"valueview":slider,
                                       @"isInteger":[NSNumber numberWithBool:YES],
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [slider setTarget:self];
                [slider setAction:@selector(valueChanged:)];

                [valueedit setDelegate:(id<NSTextFieldDelegate>)self];

                [view addSubview:slider];
                [view addSubview:valueedit];
                break;
            }
            case PROP_SELECT:
            {
                NSRect frame = NSMakeRect(label_w + label_padding, y, sz.width-label_w - label_padding - 4, unit_h);
                NSPopUpButton *popUpButton = [[NSPopUpButton alloc] initWithFrame:frame];
                
                char token[MAX_TOKEN];
                const char *script = _settingsData.props[i].select_options;
                
                int selectedIdx = atoi (value);
                
                while ((script = gettoken (script, token)) && strcmp (token, ";")) {
                    [popUpButton addItemWithTitle:[NSString stringWithUTF8String:token]];
                    if (selectedIdx == [popUpButton numberOfItems]-1) {
                        [popUpButton selectItemAtIndex:[popUpButton numberOfItems]-1];
                    }
                }
                
                [_bindings addObject:@{@"sender":popUpButton,
                                       @"propname":propname,
                                       @"default":[NSString stringWithUTF8String:_settingsData.props[i].def]
                                       }];

                [view addSubview:popUpButton];
                break;
            }
        }
    }
    deadbeef->conf_unlock ();
}

- (void)initPluginConfiguration:(NSInteger)idx {
    DB_plugin_t *p = deadbeef->plug_get_list()[idx];
    
    const char *config = p->configdialog;
    
    [self initPluginConfiguration:config inView:_pluginPropertiesView dsp:NULL];
}

- (void)controlTextDidChange:(NSNotification *)notification {
    NSTextField *textField = [notification object];
    [self valueChanged:textField];
}

- (void)valueChanged:(id)sender {
    for (NSDictionary *binding in _bindings) {
        if (binding[@"sender"] == sender && binding[@"valueview"]) {
            if (binding[@"isInteger"]) {
                [binding[@"valueview"] setStringValue:[@([sender integerValue]) stringValue]];
            }
            else {
                [binding[@"valueview"] setStringValue:[sender stringValue]];
            }
            break;
        }
    }
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

- (void)updateRGLabels {
    float value = [_replaygain_preamp_with_rg floatValue];
    [_replaygain_preamp_with_rg_label setStringValue:[NSString stringWithFormat:@"%s%0.2fdB", value >= 0 ? "+" : "", value]];
    value = [_replaygain_preamp_without_rg floatValue];
    [_replaygain_preamp_without_rg_label setStringValue:[NSString stringWithFormat:@"%s%0.2fdB", value >= 0 ? "+" : "", value]];
}

- (IBAction)replaygain_preamp_with_rg_action:(id)sender {
    float value = [sender floatValue];
    deadbeef->conf_set_float ("replaygain.preamp_with_rg", value);
    [self updateRGLabels];
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)replaygain_preamp_without_rg_action:(id)sender {
    float value = [sender floatValue];
    deadbeef->conf_set_float ("replaygain.preamp_without_rg", value);
    [self updateRGLabels];
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)replaygain_source_mode_action:(id)sender {
    NSInteger idx = [_replaygain_source_mode indexOfSelectedItem];
    deadbeef->conf_set_int ("replaygain.source_mode", (int)idx);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)replaygain_processing_action:(id)sender {
    uint32_t flags = 0;
    NSInteger idx = [_replaygain_processing indexOfSelectedItem];
    if (idx == 1) {
        flags = DDB_RG_PROCESSING_GAIN;
    }
    if (idx == 2) {
        flags = DDB_RG_PROCESSING_GAIN | DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }
    if (idx == 3) {
        flags = DDB_RG_PROCESSING_PREVENT_CLIPPING;
    }

    deadbeef->conf_set_int ("replaygain.processing_flags", flags);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

@end
