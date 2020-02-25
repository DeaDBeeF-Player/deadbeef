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

#import "DdbShared.h"
#import "NetworkPreferencesViewController.h"
#import "SoundPreferencesViewController.h"
#import "PluginsPreferencesViewController.h"
#import "PreferencesWindowController.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorViewController.h"
#import "PlaybackPreferencesViewController.h"
#include "deadbeef.h"
#include "pluginsettings.h"
#include "scriptable_dsp.h"

extern DB_functions_t *deadbeef;

@interface PreferencesWindowController ()

@property (strong) IBOutlet ScriptableSelectViewController *dspSelectViewController;

@property ScriptableTableDataSource *dspChainDataSource;
@property ScriptableTableDataSource *dspPresetsDataSource;
@property (weak) IBOutlet NSView *dspNodeEditorContainer;
@property ScriptableNodeEditorViewController *dspNodeEditorViewController;


@property (strong) IBOutlet SoundPreferencesViewController *soundViewController;
@property (strong) IBOutlet PlaybackPreferencesViewController *playbackViewController;
@property (strong) IBOutlet NetworkPreferencesViewController *networkViewController;
@property (strong) IBOutlet PluginsPreferencesViewController *pluginsViewController;

@end

@implementation PreferencesWindowController

- (void)windowDidLoad {
    [super windowDidLoad];

    // dsp
    scriptableItem_t *chain = scriptableDspConfigFromDspChain (deadbeef->streamer_get_dsp_chain ());
    self.dspChainDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:chain pasteboardItemIdentifier:@"deadbeef.dspnode.preferences"];

    self.dspPresetsDataSource = [[ScriptableTableDataSource alloc] initWithScriptable:scriptableDspRoot() pasteboardItemIdentifier:@"deadbeef.dsppreset.preferences"];

    // preset list and browse button
    self.dspSelectViewController = [[ScriptableSelectViewController alloc] initWithNibName:@"ScriptableSelectView" bundle:nil];
    self.dspSelectViewController.dataSource = self.dspPresetsDataSource;
    self.dspSelectViewController.delegate = self;
    self.dspSelectViewController.view.frame = _dspPresetSelectorContainer.bounds;
    [_dspPresetSelectorContainer addSubview:self.dspSelectViewController.view];

    // current dsp chain node list / editor
    self.dspNodeEditorViewController = [[ScriptableNodeEditorViewController alloc] initWithNibName:@"ScriptableNodeEditorView" bundle:nil];
    self.dspNodeEditorViewController.dataSource = self.dspChainDataSource;
    self.dspNodeEditorViewController.delegate = self;
    self.dspNodeEditorViewController.view.frame = _dspNodeEditorContainer.bounds;
    [_dspNodeEditorContainer addSubview:self.dspNodeEditorViewController.view];

    [self setInitialValues];

    // toolbar
    _toolbar.delegate = self;
    _toolbar.selectedItemIdentifier = @"Sound";

    [self switchToView:self.playbackViewController.view];
}

#pragma mark - ScriptableItemDelegate
- (void)scriptableItemChanged:(scriptableItem_t *)scriptable {
    if (scriptable == self.dspChainDataSource.scriptable
        || scriptableItemIndexOfChild(self.dspChainDataSource.scriptable, scriptable) >= 0) {
        ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (self.dspChainDataSource.scriptable);
        deadbeef->streamer_set_dsp_chain (chain);
        deadbeef->dsp_preset_free (chain);
    }
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    self.dspSelectViewController.scriptable = scriptableDspRoot();
}


#pragma mark - Unsorted

- (NSString *)cfgFormattedColorForName:(NSString *)colorName {
    NSColorList *clist = [NSColorList colorListNamed:@"System"];
    return [self cfgFormattedColor:[clist colorWithKey:colorName]];
}

- (NSString *)cfgFormattedColor:(NSColor *)color {
    CGFloat r, g, b, a;
    [[color colorUsingColorSpaceName:NSDeviceRGBColorSpace] getRed:&r green:&g blue:&b alpha:&a];
    return [NSString stringWithFormat:@"#%02x%02x%02x%02x", (int)(r*255), (int)(g*255), (int)(b*255), (int)(a*255)];
}

- (NSColor *)configColorForKey:(NSString *)key withDefault:(NSString *)def {
    char buf[10];
    deadbeef->conf_get_str ([key UTF8String], [def UTF8String], buf, sizeof (buf));
    int r, g, b, a;
    if (4 != sscanf (buf, "#%02x%02x%02x%02x", &r, &g, &b, &a)) {
        return NSColor.blackColor;
    }
    return [NSColor colorWithDeviceRed:r/255.f green:g/255.f blue:b/255.f alpha:a/255.f];
}

- (void)setInitialValues {
    // gui/misc -> player
    _enable_shift_jis_detection.state =  deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0) ? NSOnState : NSOffState;
    _enable_cp1251_detection.state =  deadbeef->conf_get_int ("junk.enable_cp1251_detection", 0) ? NSOnState : NSOffState;
    _enable_cp936_detection.state =  deadbeef->conf_get_int ("junk.enable_cp936_detection", 0) ? NSOnState : NSOffState;
    _refresh_rate.intValue =  deadbeef->conf_get_int ("cocoaui.refresh_rate", 10);
    _titlebar_playing.stringValue = conf_get_nsstr ("cocoaui.titlebar_playing", DEFAULT_TITLEBAR_PLAYING_VALUE);
    _titlebar_stopped.stringValue = conf_get_nsstr ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_STOPPED_VALUE);

    // gui/misc -> playlist
    _mmb_delete_playlist.state =  deadbeef->conf_get_int ("cocoaui.mmb_delete_playlist", 1) ? NSOnState : NSOffState;
    _hide_remove_from_disk.state =  deadbeef->conf_get_int ("cocoaui.hide_remove_from_disk", 0) ? NSOnState : NSOffState;
    _name_playlist_from_folder.state =  deadbeef->conf_get_int ("cocoaui.name_playlist_from_folder", 1) ? NSOnState : NSOffState;
    _autoresize_columns.state =  deadbeef->conf_get_int ("cocoaui.autoresize_columns", 0) ? NSOnState : NSOffState;

    // appearance for seekbar / volumebar
    _override_bar_colors.state =  deadbeef->conf_get_int ("cocoaui.override_bar_colors", 0) ? NSOnState : NSOffState;

    // make the config strings with defaults
    NSString *cfg_textcolor = [self cfgFormattedColorForName:@"controlTextColor"];
    NSString *cfg_shadowcolor = [self cfgFormattedColorForName:@"controlShadowColor"];
    NSString *cfg_selectedtextcolor = [self cfgFormattedColorForName:@"alternateSelectedControlTextColor"];
    NSString *cfg_evenrowcolor = [self cfgFormattedColor:NSColor.controlAlternatingRowBackgroundColors[0]];
    NSString *cfg_oddrowcolor = [self cfgFormattedColor:NSColor.controlAlternatingRowBackgroundColors[1]];
    NSString *cfg_selectedrowcolor = [self cfgFormattedColorForName:@"alternateSelectedControlColor"];

    _color_bar_foreground.color = [self configColorForKey:@"cocoaui.color.bar_foreground" withDefault:cfg_textcolor];
    _color_bar_background.color = [self configColorForKey:@"cocoaui.color.bar_background" withDefault:cfg_shadowcolor];

    // appearance for playlist
    _override_playlist_colors.state =  deadbeef->conf_get_int ("cocoaui.override_listview_colors", 0) ? NSOnState : NSOffState;
    
    _color_listview_text.color =  [self configColorForKey:@"cocoaui.color.listview_text" withDefault:cfg_textcolor];
    _color_listview_playing_text.color =  [self configColorForKey:@"cocoaui.color.listview_playing_text" withDefault:cfg_textcolor];
    _color_listview_selected_text.color =  [self configColorForKey:@"cocoaui.color.listview_selected_text" withDefault:cfg_selectedtextcolor];
    _color_listview_group_header_text.color =  [self configColorForKey:@"cocoaui.color.listview_group_header_text" withDefault:cfg_textcolor];
    _color_listview_cursor.color =  [self configColorForKey:@"cocoaui.color.cursor" withDefault:cfg_textcolor];
    _color_listview_even_background.color =  [self configColorForKey:@"cocoaui.color.listview_even_background" withDefault:cfg_evenrowcolor];
    _color_listview_odd_background.color =  [self configColorForKey:@"cocoaui.color.listview_odd_background" withDefault:cfg_oddrowcolor];
    _color_listview_selected_background.color =  [self configColorForKey:@"cocoaui.color.listview_selected_background" withDefault:cfg_selectedrowcolor];

    _listview_bold_current_text.state =  deadbeef->conf_get_int ("cocoaui.embolden_current_track", 0) ? NSOnState : NSOffState;
    _listview_bold_selected_text.state =  deadbeef->conf_get_int ("cocoaui.embolden_selected_tracks", 0) ? NSOnState : NSOffState;
    _listview_italic_current_text.state =  deadbeef->conf_get_int ("cocoaui.italic_current_track", 0) ? NSOnState : NSOffState;
    _listview_italic_selected_text.state =  deadbeef->conf_get_int ("cocoaui.italic_selected_tracks", 0) ? NSOnState : NSOffState;
}

- (NSArray *)toolbarSelectableItemIdentifiers: (NSToolbar *)toolbar
{
    return [NSArray arrayWithObjects:
            @"Sound",
            @"Playback",
            @"DSP",
            @"GUI",
//            @"Appearance",
            @"Network",
            @"Plugins",
            nil];
}

- (void)switchToView:(NSView *)view {
    self.window.contentView = nil;

    NSRect oldFrame = [self.window frame];
    NSRect rc = [self.window frameRectForContentRect:view.frame];
    rc.origin.x = oldFrame.origin.x;
    rc.origin.y = oldFrame.origin.y + oldFrame.size.height - rc.size.height;
    self.window.contentView = view;
    [self.window setFrame:rc display:YES animate:YES];
}

- (IBAction)soundAction:(id)sender {
    [self switchToView:self.playbackViewController.view];
}

- (IBAction)playbackAction:(id)sender {
    [self switchToView:self.soundViewController.view];
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
    [self switchToView:self.networkViewController.view];
}

- (IBAction)pluginsAction:(id)sender {
    [self switchToView:self.pluginsViewController.view];
}

- (void)outputDeviceChanged {
    [self.soundViewController outputDeviceChanged];
}


@end
