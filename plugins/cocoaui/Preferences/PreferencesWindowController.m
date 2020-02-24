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

#import "NetworkPreferencesViewController.h"
#import "PreferencesWindowController.h"
#import "ScriptableTableDataSource.h"
#import "ScriptableSelectViewController.h"
#import "ScriptableNodeEditorViewController.h"
#import "SoundPreferencesViewController.h"
#include "deadbeef.h"
#include "pluginsettings.h"
#include "scriptable_dsp.h"

extern DB_functions_t *deadbeef;

@interface PluginConfigPropertySheetDataSource : NSObject<PropertySheetDataSource> {
    DB_plugin_t *_plugin;
    BOOL _multipleChanges;
}
@end

@implementation PluginConfigPropertySheetDataSource
- (instancetype)initWithPlugin:(DB_plugin_t *)plugin {
    self = [super init];
    _plugin = plugin;
    return self;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc configForItem:(id)item {
    return _plugin->configdialog ? [NSString stringWithUTF8String:_plugin->configdialog] : nil;
}

- (NSString *)propertySheet:(PropertySheetViewController *)vc valueForKey:(NSString *)key def:(NSString *)def item:(id)item {
    char str[200];
    deadbeef->conf_get_str ([key UTF8String], [def UTF8String], str, sizeof (str));
    return [NSString stringWithUTF8String:str];
}

- (void)propertySheet:(PropertySheetViewController *)vc setValue:(NSString *)value forKey:(NSString *)key item:(id)item {
    deadbeef->conf_set_str ([key UTF8String], [value UTF8String]);
    if (!_multipleChanges) {
        deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    }
}

- (void)propertySheetBeginChanges {
    _multipleChanges = YES;
}

- (void)propertySheetCommitChanges {
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    _multipleChanges = NO;
}

@end


extern DB_functions_t *deadbeef;

@interface PreferencesWindowController () {
    settings_data_t _settingsData;
}

@property (strong) IBOutlet ScriptableSelectViewController *dspSelectViewController;

@property ScriptableTableDataSource *dspChainDataSource;
@property ScriptableTableDataSource *dspPresetsDataSource;
@property (weak) IBOutlet NSView *dspNodeEditorContainer;
@property ScriptableNodeEditorViewController *dspNodeEditorViewController;


@property PluginConfigPropertySheetDataSource *pluginPropertySheetDataSource;

@property (weak) IBOutlet NSPopUpButton *outputPluginsPopupButton;

@property NSMutableArray<NSString *> *audioDevices;
@property (weak) IBOutlet NSPopUpButton *audioDevicesPopupButton;
@property (weak) IBOutlet NSTextField *targetSamplerateLabel;
@property (weak) IBOutlet NSTextField *multiplesOf48Label;
@property (weak) IBOutlet NSTextField *multiplesOf44Label;

@property (weak) IBOutlet NSButton *overrideSamplerateCheckbox;
@property (weak) IBOutlet NSComboBox *targetSamplerateComboBox;
@property (weak) IBOutlet NSButton *basedOnInputSamplerateCheckbox;
@property (weak) IBOutlet NSComboBox *multiplesOf48ComboBox;
@property (weak) IBOutlet NSComboBox *multiplesOf44ComboBox;

@property (strong) IBOutlet SoundPreferencesViewController *soundViewController;
@property (strong) IBOutlet NetworkPreferencesViewController *networkViewController;


@end

@implementation PreferencesWindowController

- (void)dealloc {
    settings_data_free (&_settingsData);
}

static void
ca_enum_callback (const char *s, const char *d, void *userdata) {
    NSMutableArray<NSString *> *devices = (__bridge NSMutableArray<NSString *> *)userdata;

    [devices addObject:[NSString stringWithUTF8String:s]];
}

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

    [self initPluginList];

    [self setInitialValues];

    // toolbar
    _toolbar.delegate = self;
    _toolbar.selectedItemIdentifier = @"Sound";

    [self switchToView:self.soundViewController.view];
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

- (void)outputDeviceChanged {
    [self initAudioDeviceList];
}

- (void)initAudioDeviceList {
    [self.audioDevicesPopupButton removeAllItems];

    self.audioDevices = [NSMutableArray new];
    DB_output_t *output = deadbeef->get_output ();
    if (!output->enum_soundcards) {
        self.audioDevicesPopupButton.enabled = NO;
        return;
    }

    self.audioDevicesPopupButton.enabled = YES;

    output->enum_soundcards (ca_enum_callback, (__bridge void *)(self.audioDevices));

    NSString *conf_name = [[NSString stringWithUTF8String:output->plugin.id] stringByAppendingString:@"_soundcard"];
    char curdev[200];
    deadbeef->conf_get_str ([conf_name UTF8String], "", curdev, sizeof (curdev));
    [self.audioDevicesPopupButton removeAllItems];
    [self.audioDevicesPopupButton addItemWithTitle:@"System Default"];
    [self.audioDevicesPopupButton selectItemAtIndex:0];
    NSInteger index = 1;
    for (NSString *dev in self.audioDevices) {
        [self.audioDevicesPopupButton addItemWithTitle:dev];
        if (!strcmp ([dev UTF8String], curdev)) {
            [self.audioDevicesPopupButton selectItemAtIndex:index];
        }
        index++;
    }
}

- (void)initializeAudioTab {
    // output plugins

    NSInteger index = 0;
    [self.outputPluginsPopupButton removeAllItems];

    char curplug[200];
    deadbeef->conf_get_str ("output_plugin", "coreaudio", curplug, sizeof (curplug));
    DB_output_t **o = deadbeef->plug_get_output_list ();
    for (index = 0; o[index]; index++) {
        [self.outputPluginsPopupButton addItemWithTitle:[NSString stringWithUTF8String:o[index]->plugin.name]];
        if (!strcmp (o[index]->plugin.id, curplug)) {
            [self.outputPluginsPopupButton selectItemAtIndex:index];
        }
    }

    // audio devices
    [self initAudioDeviceList];

    self.overrideSamplerateCheckbox.state = deadbeef->conf_get_int ("streamer.override_samplerate", 0) ? NSControlStateValueOn : NSControlStateValueOff;
    self.targetSamplerateComboBox.stringValue = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("streamer.samplerate", "44100")];

    self.basedOnInputSamplerateCheckbox.state = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0) ? NSControlStateValueOn : NSControlStateValueOff;
    self.multiplesOf48ComboBox.stringValue = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("streamer.samplerate_mult_48", "48000")];
    self.multiplesOf44ComboBox.stringValue = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("streamer.samplerate_mult_44", "44100")];
    [self validateAudioSettingsViews];
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    [self initializeAudioTab];
    self.dspSelectViewController.scriptable = scriptableDspRoot();
}

#pragma mark - Playback

- (void)validateAudioSettingsViews {
    BOOL override = deadbeef->conf_get_int ("streamer.override_samplerate", 0) ? YES : NO;
    BOOL useDependent = deadbeef->conf_get_int ("streamer.use_dependent_samplerate", 0) ? YES : NO;

    self.targetSamplerateLabel.enabled = override;
    self.targetSamplerateComboBox.enabled = override;

    self.basedOnInputSamplerateCheckbox.enabled = override;

    self.multiplesOf48Label.enabled = override && useDependent;
    self.multiplesOf48ComboBox.enabled = override && useDependent;

    self.multiplesOf44Label.enabled = override && useDependent;
    self.multiplesOf44ComboBox.enabled = override && useDependent;
}

- (IBAction)outputPluginAction:(id)sender {
    DB_output_t **o = deadbeef->plug_get_output_list ();
    deadbeef->conf_set_str ("output_plugin", o[[self.outputPluginsPopupButton indexOfSelectedItem]]->plugin.id);
    deadbeef->sendmessage(DB_EV_REINIT_SOUND, 0, 0, 0);
}

- (IBAction)playbackDeviceAction:(NSPopUpButton *)sender {
    NSString *title = [[sender selectedItem] title];
    DB_output_t *output = deadbeef->get_output ();
    NSString *dev = [[NSString stringWithUTF8String:output->plugin.id] stringByAppendingString:@"_soundcard"];
    deadbeef->conf_set_str ([dev UTF8String], [title UTF8String]);
    deadbeef->sendmessage(DB_EV_REINIT_SOUND, 0, 0, 0);
}

- (IBAction)overrideSamplerateAction:(NSButton *)sender {
    deadbeef->conf_set_int ("streamer.override_samplerate", sender.state == NSControlStateValueOn ? 1 : 0);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    [self validateAudioSettingsViews];
}

static int
clamp_samplerate (int val) {
    if (val < 8000) {
        return 8000;
    }
    else if (val > 768000) {
        return 768000;
    }
    return val;
}

- (IBAction)targetSamplerateAction:(NSComboBox *)sender {
    int samplerate = clamp_samplerate ((int)[sender integerValue]);
    deadbeef->conf_set_int ("streamer.samplerate", samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)basedOnInputSamplerateAction:(NSButton *)sender {
    deadbeef->conf_set_int ("streamer.use_dependent_samplerate", sender.state == NSControlStateValueOn ? 1 : 0);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
    [self validateAudioSettingsViews];
}

- (IBAction)multiplesOf48Action:(NSComboBox *)sender {
    int samplerate = clamp_samplerate ((int)[sender integerValue]);
    deadbeef->conf_set_int ("streamer.samplerate_mult_48", samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
}

- (IBAction)multiplesOf44Action:(NSComboBox *)sender {
    int samplerate = clamp_samplerate ((int)[sender integerValue]);
    deadbeef->conf_set_int ("streamer.samplerate_mult_44", samplerate);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, 0, 0, 0);
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

- (void)initPluginList {
    _pluginList.dataSource = self;
    _pluginList.delegate = self;
    self.pluginInfo = -1;
}


- (void)setInitialValues {
    // gui/misc -> player
    _enable_shift_jis_detection.state =  deadbeef->conf_get_int ("junk.enable_shift_jis_detection", 0) ? NSOnState : NSOffState;
    _enable_cp1251_detection.state =  deadbeef->conf_get_int ("junk.enable_cp1251_detection", 0) ? NSOnState : NSOffState;
    _enable_cp936_detection.state =  deadbeef->conf_get_int ("junk.enable_cp936_detection", 0) ? NSOnState : NSOffState;
    _refresh_rate.intValue =  deadbeef->conf_get_int ("cocoaui.refresh_rate", 10);
    _titlebar_playing.stringValue = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.titlebar_playing", DEFAULT_TITLEBAR_PLAYING_VALUE)];
    _titlebar_stopped.stringValue = [NSString stringWithUTF8String:deadbeef->conf_get_str_fast ("cocoaui.titlebar_stopped", DEFAULT_TITLEBAR_STOPPED_VALUE)];

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
    [self switchToView:self.soundViewController.view];
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
    [self switchToView:self.networkViewController.view];
}

- (IBAction)pluginsAction:(id)sender {
    [self switchToView:_pluginsView];
}

- (IBAction)pluginOpenWebsite:(id)sender {
}

- (IBAction)pluginConfResetDefaults:(id)sender {
    [_pluginConfViewController reset];
}

- (void)initPluginConfiguration:(NSInteger)idx {
    DB_plugin_t *p = deadbeef->plug_get_list()[idx];
    
    self.pluginPropertySheetDataSource = [[PluginConfigPropertySheetDataSource alloc] initWithPlugin:p];

    _pluginConfViewController.labelFontSize = 10;
    _pluginConfViewController.contentFontSize = 11;
    _pluginConfViewController.unitSpacing = 4;
    _pluginConfViewController.autoAlignLabels = NO;
    _pluginConfViewController.labelFixedWidth = 120;
    _pluginConfViewController.dataSource = self.pluginPropertySheetDataSource;
}

- (void)setPluginInfo:(NSInteger)idx {
    NSString *version = @"";
    NSString *description = @"";
    NSString *license = @"";

    if (idx != -1) {
        _pluginUnselectedText.hidden = YES;
        _pluginTabView.hidden = NO;
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
        _pluginUnselectedText.hidden = NO;
        _pluginTabView.hidden = YES;
    }

    _pluginVersion.stringValue = version;
    NSAttributedString *str = [[NSAttributedString alloc] initWithString:description attributes:@{NSForegroundColorAttributeName:NSColor.controlTextColor}];
    [_pluginDescription textStorage].attributedString = str;
    _pluginDescription.string = description;
    _pluginLicense.string = license;
}

// data source for plugin list
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView {
    DB_plugin_t **p = deadbeef->plug_get_list();
    int cnt;
    for (cnt = 0; p[cnt]; cnt++);

    return cnt;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex {
    DB_plugin_t **p = deadbeef->plug_get_list();

    return [NSString stringWithUTF8String: p[rowIndex]->name];
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
    self.pluginInfo = [_pluginList selectedRow];
}

@end
