/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  plugin management

  Copyright (C) 2009-2013 Alexey Yakovenko

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

  Alexey Yakovenko waker@users.sourceforge.net
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

const char *
plug_get_system_dir (int dir_id);

int
plug_activate (DB_plugin_t *plug, int activate);

DB_output_t *
plug_get_output (void);

void
plug_reinit_sound (void);

int
plug_select_output (void);

void
plug_set_output (DB_output_t *out);

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

void
plug_event_call (ddb_event_t *ev);

void
background_job_increment (void);

void
background_job_decrement (void);

int
have_background_jobs (void);

void
action_set_playlist (ddb_playlist_t *plt);

ddb_playlist_t *
action_get_playlist (void);

void
plug_register_in (DB_plugin_t *inplug);

void
plug_register_out (DB_plugin_t *outplug);

DB_functions_t *
plug_get_api (void);

int
plug_init_plugin (DB_plugin_t* (*loadfunc)(DB_functions_t *), void *handle);

#endif // __PLUGINS_H
