/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __PLUGINS_H
#define __PLUGINS_H
#include "deadbeef.h"

extern DB_functions_t *deadbeef;

struct playItem_s;

int
plug_load_all (void);

void
plug_unload_all (void);

void
plug_connect_all (void);

void
plug_disconnect_all (void);

void
plug_cleanup (void);

void
plug_ev_subscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data);

void
plug_ev_unsubscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data);

void
plug_trigger_event (int ev, uintptr_t param);

void
plug_trigger_event_trackchange (struct playItem_s *from, struct playItem_s *to);

void
plug_trigger_event_trackinfochanged (struct playItem_s *track);

void
plug_trigger_event_paused (int paused);

void
plug_trigger_event_playlistchanged (void);

void
plug_trigger_event_volumechanged (void);

void
plug_md5 (uint8_t sig[16], const char *in, int len);

void
plug_md5_to_str (char *str, const uint8_t sig[16]);

float
plug_playback_get_pos (void);

void
plug_playback_set_pos (float pos);

struct DB_plugin_s **
plug_get_list (void);

struct DB_decoder_s **
plug_get_decoder_list (void);

struct DB_output_s **
plug_get_output_list (void);

struct DB_vfs_s **
plug_get_vfs_list (void);

struct DB_dsp_s **
plug_get_dsp_list (void);

struct DB_playlist_s **
plug_get_playlist_list (void);

void
plug_volume_set_db (float db);

void
plug_volume_set_amp (float amp);

const char *
plug_get_config_dir (void);
const char *
plug_get_prefix (void);
const char *
plug_get_doc_dir (void);
const char *
plug_get_plugin_dir (void);
const char *
plug_get_pixmap_dir (void);

int
plug_activate (DB_plugin_t *plug, int activate);

DB_output_t *
plug_get_output (void);

void
plug_reinit_sound (void);

int
plug_select_output (void);

const char *
plug_get_decoder_id (const char *id);

void
plug_remove_decoder_id (const char *id);

void
plug_free_decoder_ids (void);

DB_decoder_t *
plug_get_decoder_for_id (const char *id);

DB_plugin_t *
plug_get_for_id (const char *id);

int
plug_is_local_file (const char *fname);

const char **
plug_get_gui_names (void);

#endif // __PLUGINS_H
