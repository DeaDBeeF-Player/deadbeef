/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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
#ifndef prefwin_h
#define prefwin_h

extern int PREFWIN_TAB_INDEX_SOUND;
extern int PREFWIN_TAB_INDEX_PLAYBACK;
extern int PREFWIN_TAB_INDEX_DSP;
extern int PREFWIN_TAB_INDEX_GUI;
extern int PREFWIN_TAB_INDEX_APPEARANCE;
extern int PREFWIN_TAB_INDEX_MEDIALIB;
extern int PREFWIN_TAB_INDEX_NETWORK;
extern int PREFWIN_TAB_INDEX_HOTKEYS;
extern int PREFWIN_TAB_INDEX_PLUGINS;

/// Run the Preferences window.
/// @param tab_index The page index to activate, or -1 to keep the previous one active.
void
prefwin_run (int tab_index);

void
prefwin_fill_soundcards (void);

void
prefwin_set_scale (const char *scale_name, int value);

void
prefwin_set_combobox (GtkComboBox *combo, int i);

void
prefwin_set_toggle_button (const char *button_name, int value);

void
prefwin_set_entry_text (const char *entry_name, const char *text);

#endif /* prefwin_h */
