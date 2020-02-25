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
#import "ScriptableTableDataSource.h"

#define DEFAULT_TITLEBAR_PLAYING_VALUE "%artist% - %title% - DeaDBeeF-%_deadbeef_version%"
#define DEFAULT_TITLEBAR_STOPPED_VALUE "DeaDBeeF-%_deadbeef_version%"

@interface PreferencesWindowController : NSWindowController<NSToolbarDelegate,NSTableViewDelegate,NSTableViewDataSource,NSMenuDelegate,ScriptableItemDelegate>
@property (strong) IBOutlet NSView *dspView;
@property (strong) IBOutlet NSView *guiView;
@property (strong) IBOutlet NSView *appearanceView;
@property (unsafe_unretained) IBOutlet NSToolbar *toolbar;

- (IBAction)soundAction:(id)sender;
- (IBAction)playbackAction:(id)sender;
- (IBAction)appearanceAction:(id)sender;
- (IBAction)dspAction:(id)sender;
- (IBAction)guiAction:(id)sender;
- (IBAction)networkAction:(id)sender;
- (IBAction)pluginsAction:(id)sender;

@property (weak) IBOutlet NSView *dspPresetSelectorContainer;

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

- (void)outputDeviceChanged;

@end
