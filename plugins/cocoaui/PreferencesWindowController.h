//
//  PreferencesWindowController.h
//  deadbeef
//
//  Created by waker on 18/02/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface PreferencesWindowController : NSWindowController
@property (strong) IBOutlet NSView *playbackView;
@property (strong) IBOutlet NSView *dspView;
@property (strong) IBOutlet NSView *guiView;
@property (strong) IBOutlet NSView *appearanceView;
@property (strong) IBOutlet NSView *networkView;
@property (strong) IBOutlet NSView *pluginsView;
@property (unsafe_unretained) IBOutlet NSToolbar *toolbar;

- (IBAction)playbackAction:(id)sender;
- (IBAction)appearanceAction:(id)sender;
- (IBAction)dspAction:(id)sender;
- (IBAction)guiAction:(id)sender;
- (IBAction)networkAction:(id)sender;
- (IBAction)pluginsAction:(id)sender;

//// playback properties
@property (unsafe_unretained) IBOutlet NSPopUpButton *replaygain_mode;
@property (unsafe_unretained) IBOutlet NSButton *replaygain_scale;
@property (unsafe_unretained) IBOutlet NSSlider *replaygain_preamp;
@property (unsafe_unretained) IBOutlet NSSlider *global_preamp;
@property (unsafe_unretained) IBOutlet NSButton *cli_add_to_specific_playlist;
@property (unsafe_unretained) IBOutlet NSTextField *cli_add_playlist_name;
@property (unsafe_unretained) IBOutlet NSButton *resume_last_session;
@property (unsafe_unretained) IBOutlet NSButton *ignore_archives;
@property (unsafe_unretained) IBOutlet NSButton *stop_after_current_reset;
@property (unsafe_unretained) IBOutlet NSButton *stop_after_album_reset;

// dsp properties
- (IBAction)dspAddAction:(id)sender;
- (IBAction)dspRemoveAction:(id)sender;
- (IBAction)dspConfigureAction:(id)sender;
- (IBAction)dspMoveUpAction:(id)sender;
- (IBAction)dspMoveDownAction:(id)sender;
- (IBAction)dspSaveAction:(id)sender;
- (IBAction)dspLoadAction:(id)sender;

@property (unsafe_unretained) IBOutlet NSScrollView *dspList;
@property (unsafe_unretained) IBOutlet NSComboBox *dspPresets;

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
@property (unsafe_unretained) IBOutlet NSScrollView *pluginList;
@property (unsafe_unretained) IBOutlet NSTextField *pluginVersion;
@property (unsafe_unretained) IBOutlet NSScrollView *pluginDescription;

- (IBAction)pluginConfigure:(id)sender;
- (IBAction)pluginShowCopyright:(id)sender;
- (IBAction)pluginOpenWebsite:(id)sender;

@end
