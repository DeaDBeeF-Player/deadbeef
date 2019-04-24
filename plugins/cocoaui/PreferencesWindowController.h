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

#import <Cocoa/Cocoa.h>
#import "PropertySheetViewController.h"
#import "DSPPresetListDataSource.h"
#import "ScriptableTableDataSource.h"

#define DEFAULT_TITLEBAR_PLAYING_VALUE "%artist% - %title% - DeaDBeeF-%_deadbeef_version%"
#define DEFAULT_TITLEBAR_STOPPED_VALUE "DeaDBeeF-%_deadbeef_version%"

@interface PreferencesWindowController : NSWindowController<NSToolbarDelegate,NSTableViewDelegate,NSTableViewDataSource,NSMenuDelegate,ScriptableItemDelegate>
@property (strong) IBOutlet NSView *soundView;
@property (strong) IBOutlet NSView *playbackView;
@property (strong) IBOutlet NSView *dspView;
@property (strong) IBOutlet NSView *guiView;
@property (strong) IBOutlet NSView *appearanceView;
@property (strong) IBOutlet NSView *networkView;
@property (strong) IBOutlet NSView *pluginsView;
@property (unsafe_unretained) IBOutlet NSToolbar *toolbar;

- (IBAction)soundAction:(id)sender;
- (IBAction)playbackAction:(id)sender;
- (IBAction)appearanceAction:(id)sender;
- (IBAction)dspAction:(id)sender;
- (IBAction)guiAction:(id)sender;
- (IBAction)networkAction:(id)sender;
- (IBAction)pluginsAction:(id)sender;

//// playback properties
@property (unsafe_unretained) IBOutlet NSPopUpButton *replaygain_source_mode;
@property (unsafe_unretained) IBOutlet NSPopUpButton *replaygain_processing;
@property (unsafe_unretained) IBOutlet NSSlider *replaygain_preamp_with_rg;
@property (unsafe_unretained) IBOutlet NSSlider *replaygain_preamp_without_rg;
@property (unsafe_unretained) IBOutlet NSTextField *replaygain_preamp_with_rg_label;
@property (unsafe_unretained) IBOutlet NSTextField *replaygain_preamp_without_rg_label;

- (IBAction)replaygain_preamp_with_rg_action:(id)sender;
- (IBAction)replaygain_preamp_without_rg_action:(id)sender;
- (IBAction)replaygain_source_mode_action:(id)sender;
- (IBAction)replaygain_processing_action:(id)sender;


@property (unsafe_unretained) IBOutlet NSButton *cli_add_to_specific_playlist;
@property (unsafe_unretained) IBOutlet NSTextField *cli_add_playlist_name;
@property (unsafe_unretained) IBOutlet NSButton *resume_last_session;

- (IBAction)resumeLastSessionAction:(id)sender;


@property (unsafe_unretained) IBOutlet NSButton *ignore_archives;

- (IBAction)ignoreArchivesAction:(id)sender;


@property (unsafe_unretained) IBOutlet NSButton *stop_after_current_reset;

- (IBAction)stopAfterCurrentResetAction:(id)sender;


@property (unsafe_unretained) IBOutlet NSButton *stop_after_album_reset;

- (IBAction)stopAfterCurrentAlbumResetAction:(id)sender;

// dsp properties
@property (strong) IBOutlet NSPanel *dspConfigPanel;
//@property (unsafe_unretained) IBOutlet NSScrollView *dspConfigView;
@property (strong) IBOutlet PropertySheetViewController *dspConfigViewController;



- (IBAction)dspConfigOkAction:(id)sender;
- (IBAction)dspConfigResetAction:(id)sender;

- (IBAction)dspChainAction:(id)sender;
- (IBAction)dspSaveAction:(id)sender;
- (IBAction)dspLoadAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSTableView *dspList;

@property (weak) IBOutlet NSView *dspPresetSelectorContainer;

@property (strong) IBOutlet DSPPresetListDataSource *dspPresetListDataSource;

// GUI misc properties
@property (unsafe_unretained) IBOutlet NSButton *enable_shift_jis_detection;
@property (unsafe_unretained) IBOutlet NSButton *enable_cp1251_detection;
@property (unsafe_unretained) IBOutlet NSButton *enable_cp936_detection;
@property (unsafe_unretained) IBOutlet NSSlider *refresh_rate;
@property (unsafe_unretained) IBOutlet NSTextField *titlebar_playing;
@property (unsafe_unretained) IBOutlet NSTextField *titlebar_stopped;

// GUI Playlist properties
@property (unsafe_unretained) IBOutlet NSButton *mmb_delete_playlist;
@property (unsafe_unretained) IBOutlet NSButton *hide_remove_from_disk;
@property (unsafe_unretained) IBOutlet NSButton *name_playlist_from_folder;
@property (unsafe_unretained) IBOutlet NSButton *autoresize_columns;

// Appearance properties
@property (unsafe_unretained) IBOutlet NSButton *override_bar_colors;
@property (unsafe_unretained) IBOutlet NSColorWell *color_bar_foreground;
@property (unsafe_unretained) IBOutlet NSColorWell *color_bar_background;

@property (unsafe_unretained) IBOutlet NSButton *override_playlist_colors;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_text;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_playing_text;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_selected_text;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_group_header_text;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_cursor;

@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_even_background;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_odd_background;
@property (unsafe_unretained) IBOutlet NSColorWell *color_listview_selected_background;

@property (unsafe_unretained) IBOutlet NSButton *listview_bold_current_text;
@property (unsafe_unretained) IBOutlet NSButton *listview_bold_selected_text;
@property (unsafe_unretained) IBOutlet NSButton *listview_italic_current_text;
@property (unsafe_unretained) IBOutlet NSButton *listview_italic_selected_text;

// Network properties
@property (unsafe_unretained) IBOutlet NSButton *network_proxy;
@property (unsafe_unretained) IBOutlet NSTextField *network_proxy_address;
@property (unsafe_unretained) IBOutlet NSTextField *network_proxy_port;
@property (unsafe_unretained) IBOutlet NSPopUpButton *network_proxy_type;
@property (unsafe_unretained) IBOutlet NSTextField *network_proxy_username;
@property (unsafe_unretained) IBOutlet NSSecureTextField *network_proxy_password;
@property (unsafe_unretained) IBOutlet NSTextField *network_http_user_agent;

- (IBAction)networkEditContentTypeMapping:(id)sender;

// Plugins properties
@property (unsafe_unretained) IBOutlet NSTextField *pluginUnselectedText;
@property (unsafe_unretained) IBOutlet NSTabView *pluginTabView;

@property (unsafe_unretained) IBOutlet NSTableView *pluginList;
@property (unsafe_unretained) IBOutlet NSTextField *pluginVersion;
@property (unsafe_unretained) IBOutlet NSTextView *pluginDescription;
@property (unsafe_unretained) IBOutlet NSTextView *pluginLicense;

@property (strong) IBOutlet PropertySheetViewController *pluginConfViewController;

- (IBAction)pluginOpenWebsite:(id)sender;
- (IBAction)pluginConfResetDefaults:(id)sender;

- (void)outputDeviceChanged;

@end
