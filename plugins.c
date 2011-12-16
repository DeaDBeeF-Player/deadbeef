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
#include <dirent.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
//#include <alloca.h>
#include <string.h>
#ifndef __linux__
#define _POSIX_C_SOURCE 1
#endif
#include <limits.h>
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include "gettext.h"
#include "plugins.h"
#include "md5/md5.h"
#include "messagepump.h"
#include "threading.h"
#include "playlist.h"
#include "volume.h"
#include "streamer.h"
#include "common.h"
#include "conf.h"
#include "junklib.h"
#include "vfs.h"
#include "premix.h"
#include "dsppreset.h"
#include "pltmeta.h"
#include "metacache.h"

#define trace(...) { fprintf(stderr, __VA_ARGS__); }
//#define trace(fmt,...)

#ifndef PATH_MAX
#define PATH_MAX    1024    /* max # of characters in a path name */
#endif

//#define DISABLE_VERSIONCHECK 1

// deadbeef api
static DB_functions_t deadbeef_api = {
    .vmajor = DB_API_VERSION_MAJOR,
    .vminor = DB_API_VERSION_MINOR,
    .md5 = plug_md5,
    .md5_to_str = plug_md5_to_str,
    .md5_init = (void (*)(DB_md5_t *s))md5_init,
    .md5_append = (void (*)(DB_md5_t *s, const uint8_t *daya, int nbytes))md5_append,
    .md5_finish = (void (*)(DB_md5_t *s, uint8_t digest[16]))md5_finish,
    .get_output = plug_get_output,
    .playback_get_pos = plug_playback_get_pos,
    .playback_set_pos = plug_playback_set_pos,
    // streamer access
    .streamer_get_playing_track = (DB_playItem_t *(*) (void))streamer_get_playing_track,
    .streamer_get_streaming_track = (DB_playItem_t *(*) (void))streamer_get_streaming_track,
    .streamer_get_playpos = streamer_get_playpos,
    .streamer_ok_to_read = streamer_ok_to_read,
    .streamer_reset = streamer_reset,
    .streamer_read = streamer_read,
    .streamer_set_bitrate = streamer_set_bitrate,
    .streamer_get_apx_bitrate = streamer_get_apx_bitrate,
    .streamer_get_current_fileinfo = streamer_get_current_fileinfo,
    .streamer_get_current_playlist = streamer_get_current_playlist,
    .streamer_get_dsp_chain = streamer_get_dsp_chain,
    .streamer_set_dsp_chain = streamer_set_dsp_chain,
    .streamer_dsp_refresh = streamer_dsp_refresh,
    // folders
    .get_config_dir = plug_get_config_dir,
    .get_prefix = plug_get_prefix,
    .get_doc_dir = plug_get_doc_dir,
    .get_plugin_dir = plug_get_plugin_dir,
    .get_pixmap_dir = plug_get_pixmap_dir,
    // threading
    .thread_start = thread_start,
    .thread_start_low_priority = thread_start_low_priority,
    .thread_join = thread_join,
    .thread_detach = thread_detach,
    .thread_exit = thread_exit,
    .mutex_create = mutex_create,
    .mutex_create_nonrecursive = mutex_create_nonrecursive,
    .mutex_free = mutex_free,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
    .cond_create = cond_create,
    .cond_free = cond_free,
    .cond_wait = cond_wait,
    .cond_signal = cond_signal,
    .cond_broadcast = cond_broadcast,
    // playlist management
    .plt_ref = (void (*) (ddb_playlist_t *plt))plt_ref,
    .plt_unref = (void (*) (ddb_playlist_t *plt))plt_unref,
    .plt_get_count = plt_get_count,
    .plt_get_head = (DB_playItem_t * (*) (int plt))plt_get_head,
    .plt_get_sel_count = plt_get_sel_count,
    .plt_add = plt_add,
    .plt_remove = plt_remove,
    .plt_clear = (void (*)(ddb_playlist_t *))plt_clear,
    .pl_clear = pl_clear,
    .plt_set_curr = (void (*) (ddb_playlist_t *plt))plt_set_curr,
    .plt_get_curr = (ddb_playlist_t *(*) (void))plt_get_curr,
    .plt_set_curr_idx = (void (*) (int plt))plt_set_curr_idx,
    .plt_get_curr_idx = (int (*) (void))plt_get_curr_idx,
    .plt_move = plt_move,
    // playlist saving and loading
    .plt_load = (DB_playItem_t * (*) (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data))plt_load,
    .plt_save = (int (*)(ddb_playlist_t *plt, DB_playItem_t *first, DB_playItem_t *last, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data))plt_save,
    // getting and working with a handle must be guarded using plt_lock/unlock
    .plt_get_for_idx = (ddb_playlist_t *(*)(int idx))plt_get_for_idx,
    .plt_get_title = (int (*)(ddb_playlist_t *handle, char *buffer, int sz))plt_get_title,
    .plt_set_title = (int (*)(ddb_playlist_t *handle, const char *buffer))plt_set_title,
    .plt_modified = (void (*) (ddb_playlist_t *handle))plt_modified,
    .plt_get_modification_idx = (int (*) (ddb_playlist_t *handle))plt_get_modification_idx,
    .plt_get_item_idx = (int (*) (ddb_playlist_t *plt, DB_playItem_t *it, int iter))plt_get_item_idx,

    // playlist metadata
    .plt_add_meta = (void (*) (ddb_playlist_t *handle, const char *key, const char *value))plt_add_meta,
    .plt_replace_meta = (void (*) (ddb_playlist_t *handle, const char *key, const char *value))plt_replace_meta,
    .plt_append_meta = (void (*) (ddb_playlist_t *handle, const char *key, const char *value))plt_append_meta,
    .plt_set_meta_int = (void (*) (ddb_playlist_t *handle, const char *key, int value))plt_set_meta_int,
    .plt_set_meta_float = (void (*) (ddb_playlist_t *handle, const char *key, float value))plt_set_meta_float,
    .plt_find_meta = (const char *(*) (ddb_playlist_t *handle, const char *key))plt_find_meta,
    .plt_get_metadata_head = (DB_metaInfo_t * (*) (ddb_playlist_t *handle))plt_get_metadata_head,
    .plt_delete_metadata = (void (*) (ddb_playlist_t *handle, DB_metaInfo_t *meta))plt_delete_metadata,
    .plt_find_meta_int = (int (*) (ddb_playlist_t *handle, const char *key, int def))plt_find_meta_int,
    .plt_find_meta_float = (float (*) (ddb_playlist_t *handle, const char *key, float def))plt_find_meta_float,
    .plt_delete_all_meta = (void (*) (ddb_playlist_t *handle))plt_delete_all_meta,

    // operating on playlist items
    .plt_insert_item = (DB_playItem_t *(*) (ddb_playlist_t *playlist, DB_playItem_t *after, DB_playItem_t *it))plt_insert_item,
    .plt_insert_file = (DB_playItem_t *(*) (ddb_playlist_t *playlist, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data))plt_insert_file,
    .plt_insert_dir = (DB_playItem_t *(*) (ddb_playlist_t *plt, DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data))plt_insert_dir,
    .plt_set_item_duration = (void (*) (ddb_playlist_t *plt, DB_playItem_t *it, float duration))plt_set_item_duration,
    .plt_remove_item = (int (*) (ddb_playlist_t *playlist, DB_playItem_t *it))plt_remove_item,
    .plt_getselcount = (int (*) (ddb_playlist_t *playlist))plt_getselcount,

    // playlist access
    .pl_lock = pl_lock,
    .pl_unlock = pl_unlock,
    //.plt_lock = plt_lock,
    //.plt_unlock = plt_unlock,
    .pl_item_alloc = (DB_playItem_t* (*)(void))pl_item_alloc,
    .pl_item_alloc_init = (DB_playItem_t* (*)(const char *fname, const char *decoder_id))pl_item_alloc_init,
    .pl_item_ref = (void (*)(DB_playItem_t *))pl_item_ref,
    .pl_item_unref = (void (*)(DB_playItem_t *))pl_item_unref,
    .pl_item_copy = (void (*)(DB_playItem_t *, DB_playItem_t *))pl_item_copy,
    .plt_add_file = (int (*) (ddb_playlist_t *plt, const char *, int (*cb)(DB_playItem_t *it, void *data), void *))plt_add_file,
    .plt_add_dir = (int (*) (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data))plt_add_dir,
    .pl_add_files_begin = (int (*) (ddb_playlist_t *plt))pl_add_files_begin,
    .pl_add_files_end = pl_add_files_end,
    .pl_get_idx_of = (int (*) (DB_playItem_t *it))pl_get_idx_of,
    .pl_get_idx_of_iter = (int (*) (DB_playItem_t *it, int iter))pl_get_idx_of_iter,
    .pl_get_for_idx = (DB_playItem_t * (*)(int))pl_get_for_idx,
    .pl_get_for_idx_and_iter = (DB_playItem_t * (*) (int idx, int iter))pl_get_for_idx_and_iter,
    .pl_get_item_duration = (float (*) (DB_playItem_t *it))pl_get_item_duration,
    .pl_get_item_flags = (uint32_t (*) (DB_playItem_t *it))pl_get_item_flags,
    .pl_set_item_flags = (void (*) (DB_playItem_t *it, uint32_t flags))pl_set_item_flags,
    .plt_sort = (void (*) (ddb_playlist_t *plt, int iter, int id, const char *format, int ascending))plt_sort,
    .pl_items_copy_junk = (void (*)(DB_playItem_t *from, DB_playItem_t *first, DB_playItem_t *last))pl_items_copy_junk,
    .pl_set_item_replaygain = (void (*)(DB_playItem_t *it, int idx, float value))pl_set_item_replaygain,
    .pl_get_item_replaygain = (float (*)(DB_playItem_t *it, int idx))pl_get_item_replaygain,
    .plt_get_totaltime = (float (*) (ddb_playlist_t *plt))plt_get_totaltime,
    .plt_get_item_idx = (int (*) (ddb_playlist_t *playlist, DB_playItem_t *it, int iter))plt_get_item_idx,
    .plt_get_item_for_idx = (DB_playItem_t * (*) (ddb_playlist_t *playlist, int idx, int iter))plt_get_item_for_idx,
    .pl_get_totaltime = pl_get_totaltime,
    .pl_getcount = pl_getcount,
    .plt_get_item_count = (int (*)(ddb_playlist_t *plt, int iter))plt_get_item_count,
    .plt_delete_selected = (int (*) (ddb_playlist_t *plt))plt_delete_selected,
    .pl_delete_selected = pl_delete_selected,
    .plt_set_cursor = (void (*)(ddb_playlist_t *plt, int iter, int cursor))plt_set_cursor,
    .pl_set_cursor = pl_set_cursor,
    .plt_get_cursor = (int (*)(ddb_playlist_t *plt, int iter))plt_get_cursor,
    .pl_get_cursor = pl_get_cursor,
    .pl_set_selected = (void (*) (DB_playItem_t *, int))pl_set_selected,
    .pl_is_selected = (int (*) (DB_playItem_t *))pl_is_selected,
    .pl_save_current = pl_save_current,
    .pl_save_all = pl_save_all,
    .plt_select_all = (void (*) (ddb_playlist_t *plt))plt_select_all,
    .pl_select_all = pl_select_all,
    .plt_crop_selected = (void (*) (ddb_playlist_t *plt))plt_crop_selected,
    .pl_crop_selected = pl_crop_selected,
    .pl_getselcount = pl_getselcount,
    .plt_get_first = (DB_playItem_t *(*) (ddb_playlist_t *plt, int))plt_get_first,
    .pl_get_first = (DB_playItem_t *(*) (int))pl_get_first,
    .plt_get_last = (DB_playItem_t *(*) (ddb_playlist_t *plt, int))plt_get_last,
    .pl_get_last = (DB_playItem_t *(*) (int))pl_get_last,
    .pl_get_next = (DB_playItem_t *(*) (DB_playItem_t *, int))pl_get_next,
    .pl_get_prev = (DB_playItem_t *(*) (DB_playItem_t *, int))pl_get_prev,
    .pl_format_title = (int (*) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt))pl_format_title,
    .pl_format_title_escaped = (int (*) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt))pl_format_title_escaped,
    .pl_format_time = pl_format_time,
    .plt_move_items = (void (*) (ddb_playlist_t *to, int iter, ddb_playlist_t *from, DB_playItem_t *drop_before, uint32_t *indexes, int count))plt_move_items,
    .plt_copy_items = (void (*) (ddb_playlist_t *to, int iter, ddb_playlist_t *from, DB_playItem_t *before, uint32_t *indices, int cnt))plt_copy_items,
    .plt_search_reset = (void (*)(ddb_playlist_t *plt))plt_search_reset,
    .plt_search_process = (void (*)(ddb_playlist_t *plt, const char *t))plt_search_process,
    .pl_get_playlist = (ddb_playlist_t * (*) (DB_playItem_t *it))pl_get_playlist,
    // metainfo
    .pl_add_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_add_meta,
    .pl_append_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_append_meta,
    .pl_set_meta_int = (void (*) (DB_playItem_t *it, const char *key, int value))pl_set_meta_int,
    .pl_set_meta_float = (void (*) (DB_playItem_t *it, const char *key, float value))pl_set_meta_float,
    .pl_delete_meta = (void (*)(DB_playItem_t *, const char *key))pl_delete_meta,
    .pl_find_meta = (const char *(*) (DB_playItem_t *, const char *))pl_find_meta,
    .pl_find_meta_int = (int (*) (DB_playItem_t *it, const char *key, int def))pl_find_meta_int,
    .pl_find_meta_float = (float (*) (DB_playItem_t *it, const char *key, float def))pl_find_meta_float,
    .pl_replace_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_replace_meta,
    .pl_delete_all_meta = (void (*) (DB_playItem_t *it))pl_delete_all_meta,
    .pl_get_metadata_head = (DB_metaInfo_t *(*)(DB_playItem_t *it))pl_get_metadata_head,
    .pl_delete_metadata = (void (*)(DB_playItem_t *, DB_metaInfo_t *))pl_delete_metadata,
    // cuesheet support
    .plt_insert_cue_from_buffer = (DB_playItem_t *(*) (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate))plt_insert_cue_from_buffer,
    .plt_insert_cue = (DB_playItem_t *(*)(ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *origin, int numsamples, int samplerate))plt_insert_cue,
    // playqueue support
    .pl_playqueue_push = (int (*) (DB_playItem_t *))pl_playqueue_push,
    .pl_playqueue_clear = pl_playqueue_clear,
    .pl_playqueue_pop = pl_playqueue_pop,
    .pl_playqueue_remove = (void (*) (DB_playItem_t *))pl_playqueue_remove,
    .pl_playqueue_test = (int (*) (DB_playItem_t *))pl_playqueue_test,
    // volume control
    .volume_set_db = plug_volume_set_db,
    .volume_get_db = volume_get_db,
    .volume_set_amp = plug_volume_set_amp,
    .volume_get_amp = volume_get_amp,
    .volume_get_min_db = volume_get_min_db,
    // junk reading
    .junk_id3v1_read = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_id3v1_read,
    .junk_id3v1_find = junk_id3v1_find,
    .junk_id3v1_write = (int (*) (FILE *, DB_playItem_t *, const char *))junk_id3v1_write,
    .junk_id3v2_find = junk_id3v2_find,
    .junk_id3v2_read = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_id3v2_read,
    .junk_id3v2_read_full = (int (*)(DB_playItem_t *, DB_id3v2_tag_t *tag, DB_FILE *fp))junk_id3v2_read_full,
    .junk_id3v2_convert_24_to_23 = junk_id3v2_convert_24_to_23,
    .junk_id3v2_convert_23_to_24 = junk_id3v2_convert_23_to_24,
    .junk_id3v2_convert_22_to_24 = junk_id3v2_convert_22_to_24,
    .junk_id3v2_free = junk_id3v2_free,
    .junk_id3v2_write = junk_id3v2_write,
    .junk_id3v2_remove_frames = junk_id3v2_remove_frames,
    .junk_id3v2_add_text_frame = junk_id3v2_add_text_frame,
    .junk_apev2_read = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_apev2_read,
    .junk_apev2_read_mem = (int (*) (DB_playItem_t *it, char *mem, int size))junk_apev2_read_mem,
    .junk_apev2_read_full = (int (*) (DB_playItem_t *it, DB_apev2_tag_t *tag_store, DB_FILE *fp))junk_apev2_read_full,
    .junk_apev2_read_full_mem = (int (*) (DB_playItem_t *it, DB_apev2_tag_t *tag_store, char *mem, int memsize))junk_apev2_read_full_mem,
    .junk_apev2_find = junk_apev2_find,
    .junk_apev2_remove_frames = junk_apev2_remove_frames,
    .junk_apev2_add_text_frame = junk_apev2_add_text_frame,
    .junk_apev2_free = junk_apev2_free,
    .junk_apev2_write = junk_apev2_write,
    .junk_get_leading_size = junk_get_leading_size,
    .junk_get_leading_size_stdio = junk_get_leading_size_stdio,
    .junk_detect_charset = junk_detect_charset,
    .junk_recode = junk_recode,
    .junk_iconv = junk_iconv,
    .junk_rewrite_tags = (int (*) (DB_playItem_t *it, uint32_t flags, int id3v2_version, const char *id3v1_encoding))junk_rewrite_tags,
    // vfs
    .fopen = vfs_fopen,
    .fclose = vfs_fclose,
    .fread = vfs_fread,
    .fseek = vfs_fseek,
    .ftell = vfs_ftell,
    .rewind = vfs_rewind,
    .fgetlength = vfs_fgetlength,
    .fget_content_type = vfs_get_content_type,
    .fset_track = (void (*) (DB_FILE *stream, DB_playItem_t *it))vfs_set_track,
    .fabort = vfs_fabort,
    // message passing
    .sendmessage = messagepump_push,
    .event_alloc = messagepump_event_alloc,
    .event_free = messagepump_event_free,
    .event_send = messagepump_push_event,
    // configuration access
    .conf_lock = conf_lock,
    .conf_unlock = conf_unlock,
    .conf_get_str_fast = conf_get_str_fast,
    .conf_get_str = conf_get_str,
    .conf_get_float = conf_get_float,
    .conf_get_int = conf_get_int,
    .conf_get_int64 = conf_get_int64,
    .conf_set_str = conf_set_str,
    .conf_set_int = conf_set_int,
    .conf_set_int64 = conf_set_int64,
    .conf_set_float = conf_set_float,
    .conf_find = conf_find,
    .conf_remove_items = conf_remove_items,
    .conf_save = conf_save,
    // plugin communication
    .plug_get_decoder_list = plug_get_decoder_list,
    .plug_get_vfs_list = plug_get_vfs_list,
    .plug_get_output_list = plug_get_output_list,
    .plug_get_dsp_list = plug_get_dsp_list,
    .plug_get_playlist_list = plug_get_playlist_list,
    .plug_get_list = plug_get_list,
    .plug_get_gui_names = plug_get_gui_names,
    .plug_get_decoder_id = plug_get_decoder_id,
    .plug_remove_decoder_id = plug_remove_decoder_id,
    .plug_get_for_id = plug_get_for_id,
    // misc utilities
    .is_local_file = plug_is_local_file,
    // pcm utilities
    .pcm_convert = pcm_convert,
    // dsp preset management
    .dsp_preset_load = dsp_preset_load,
    .dsp_preset_save = dsp_preset_save,
    .dsp_preset_free = dsp_preset_free,
    // new 1.2 APIs
    .plt_alloc = (ddb_playlist_t *(*)(const char *title))plt_alloc,
    .plt_free = (void (*)(ddb_playlist_t *plt))plt_free,
    //.plt_insert = plt_insert,
    .plt_set_fast_mode = (void (*)(ddb_playlist_t *plt, int fast))plt_set_fast_mode,
    .plt_is_fast_mode = (int (*)(ddb_playlist_t *plt))plt_is_fast_mode,
    .metacache_add_string = metacache_add_string,
    .metacache_remove_string = metacache_remove_string,
    .metacache_ref = metacache_ref,
    .metacache_unref = metacache_unref,
    .pl_find_meta_raw = (const char *(*) (DB_playItem_t *it, const char *key))pl_find_meta_raw,
};

DB_functions_t *deadbeef = &deadbeef_api;

const char *
plug_get_config_dir (void) {
    return dbconfdir;
}

const char *
plug_get_prefix (void) {
    return dbinstalldir;
}

const char *
plug_get_doc_dir (void) {
    return dbdocdir;
}

const char *
plug_get_plugin_dir (void) {
    return dbplugindir;
}

const char *
plug_get_pixmap_dir (void) {
    return dbpixmapdir;
}

void
plug_volume_set_db (float db) {
    volume_set_db (db);
    messagepump_push (DB_EV_VOLUMECHANGED, 0, 0, 0);
}

void
plug_volume_set_amp (float amp) {
    volume_set_amp (amp);
    messagepump_push (DB_EV_VOLUMECHANGED, 0, 0, 0);
}

#define MAX_PLUGINS 100
DB_plugin_t *g_plugins[MAX_PLUGINS+1];

#define MAX_GUI_PLUGINS 10
char *g_gui_names[MAX_GUI_PLUGINS+1];
int g_num_gui_names;

DB_decoder_t *g_decoder_plugins[MAX_DECODER_PLUGINS+1];

#define MAX_VFS_PLUGINS 10
DB_vfs_t *g_vfs_plugins[MAX_VFS_PLUGINS+1];

#define MAX_DSP_PLUGINS 10
DB_dsp_t *g_dsp_plugins[MAX_DSP_PLUGINS+1];

#define MAX_OUTPUT_PLUGINS 10
DB_output_t *g_output_plugins[MAX_OUTPUT_PLUGINS+1];
DB_output_t *output_plugin = NULL;

#define MAX_PLAYLIST_PLUGINS 10
DB_playlist_t *g_playlist_plugins[MAX_PLAYLIST_PLUGINS+1];

void
plug_md5 (uint8_t sig[16], const char *in, int len) {
    md5_state_t st;
    md5_init (&st);
    md5_append (&st, in, len);
    md5_finish (&st, sig);
}

void
plug_md5_to_str (char *str, const uint8_t sig[16]) {
    int i = 0;
    static const char hex[] = "0123456789abcdef";
    for (i = 0; i < 16; i++) {
        *str++ = hex[(sig[i]&0xf0)>>4];
        *str++ = hex[sig[i]&0xf];
    }
    *str = 0;
}

// plugin control structures
typedef struct plugin_s {
    void *handle;
    DB_plugin_t *plugin;
    struct plugin_s *next;
} plugin_t;

plugin_t *plugins;
plugin_t *plugins_tail;

float
plug_playback_get_pos (void) {
    playItem_t *trk = streamer_get_playing_track ();
    float dur = trk ? pl_get_item_duration (trk) : -1;
    if (!trk || dur <= 0) {
        if (trk) {
            pl_item_unref (trk);
        }
        return 0;
    }
    if (trk) {
        pl_item_unref (trk);
    }
    return streamer_get_playpos () * 100 / dur;
}

void
plug_playback_set_pos (float pos) {
    playItem_t *trk = streamer_get_playing_track ();
    float dur = trk ? pl_get_item_duration (trk) : -1;
    if (!trk || dur <= 0) {
        if (trk) {
            pl_item_unref (trk);
        }
        return;
    }
    float t = pos * dur / 100.f;
    if (trk) {
        pl_item_unref (trk);
    }
    streamer_set_seek (t);
}

int
plug_init_plugin (DB_plugin_t* (*loadfunc)(DB_functions_t *), void *handle) {
    DB_plugin_t *plugin_api = loadfunc (&deadbeef_api);
    if (!plugin_api) {
        return -1;
    }

    // check if same plugin with the same or bigger version is loaded already
    plugin_t *prev = NULL;
    for (plugin_t *p = plugins; p; prev = p, p = p->next) {
        if (p->plugin->id && plugin_api->id && !strcmp (p->plugin->id, plugin_api->id)) {
            if (plugin_api->version_major > p->plugin->version_major || (plugin_api->version_major == p->plugin->version_major && plugin_api->version_minor > p->plugin->version_minor)) {
                trace ("found newer version of plugin \"%s\", replacing\n", plugin_api->id);
                // unload older plugin before replacing
                dlclose (p->handle);
                if (prev) {
                    prev->next = p->next;
                }
                else {
                    plugins = p->next;
                }
                if (p->handle) {
                    dlclose (p->handle);
                }
                free (p);
            }
            else {
                trace ("found copy of plugin \"%s\", but newer version is already loaded\n", plugin_api->id)
                return -1;
            }
        }
    }

#if !DISABLE_VERSIONCHECK
    if (plugin_api->api_vmajor != 0 || plugin_api->api_vminor != 0) {
        // version check enabled
        if (DB_API_VERSION_MAJOR != 9 && DB_API_VERSION_MINOR != 9) {
            if (plugin_api->api_vmajor != DB_API_VERSION_MAJOR || plugin_api->api_vminor > DB_API_VERSION_MINOR) {
                trace ("\033[0;31mWARNING: plugin \"%s\" wants API v%d.%d (got %d.%d), will not be loaded\033[0;m\n", plugin_api->name, plugin_api->api_vmajor, plugin_api->api_vminor, DB_API_VERSION_MAJOR, DB_API_VERSION_MINOR);
                return -1;
            }
        }
    }
    else {
            trace ("\033[0;31mWARNING: plugin \"%s\" has disabled version check. please don't distribute it!\033[0;m\n", plugin_api->name);
    }
#endif
    plugin_t *plug = malloc (sizeof (plugin_t));
    memset (plug, 0, sizeof (plugin_t));
    plug->plugin = plugin_api;
    plug->handle = handle;
    if (plugins_tail) {
        plugins_tail->next = plug;
        plugins_tail = plug;
    }
    else {
        plugins = plugins_tail = plug;
    }
    return 0;
}

static int dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

void
plug_remove_plugin (void *p) {
    int i;
    for (i = 0; g_plugins[i]; i++) {
        if (g_plugins[i] == p) {
            memmove (&g_plugins[i], &g_plugins[i+1], (MAX_PLUGINS+1-i-1) * sizeof (void*));
        }
    }
    for (i = 0; g_decoder_plugins[i]; i++) {
        if (g_decoder_plugins[i] == p) {
            memmove (&g_decoder_plugins[i], &g_decoder_plugins[i+1], (MAX_DECODER_PLUGINS+1-i-1) * sizeof (void*));
            break;
        }
    }
    for (i = 0; g_vfs_plugins[i]; i++) {
        if (g_vfs_plugins[i] == p) {
            memmove (&g_vfs_plugins[i], &g_vfs_plugins[i+1], (MAX_VFS_PLUGINS+1-i-1) * sizeof (void*));
            break;
        }
    }
    for (i = 0; g_dsp_plugins[i]; i++) {
        if (g_dsp_plugins[i] == p) {
            memmove (&g_dsp_plugins[i], &g_dsp_plugins[i+1], (MAX_DSP_PLUGINS+1-i-1) * sizeof (void*));
            break;
        }
    }
    for (i = 0; g_output_plugins[i]; i++) {
        if (g_output_plugins[i] == p) {
            memmove (&g_output_plugins[i], &g_output_plugins[i+1], (MAX_OUTPUT_PLUGINS+1-i-1) * sizeof (void*));
            break;
        }
    }
    for (i = 0; g_playlist_plugins[i]; i++) {
        if (g_playlist_plugins[i] == p) {
            memmove (&g_playlist_plugins[i], &g_playlist_plugins[i+1], (MAX_PLAYLIST_PLUGINS+1-i-1) * sizeof (void*));
            break;
        }
    }
}

// d_name must be writable w/o sideeffects; contain valid .so name
// l must be strlen(d_name)
static int
load_plugin (const char *plugdir, char *d_name, int l) {
    char fullname[PATH_MAX];
    snprintf (fullname, PATH_MAX, "%s/%s", plugdir, d_name);
    trace ("loading plugin %s/%s\n", plugdir, d_name);
    void *handle = dlopen (fullname, RTLD_NOW);
    if (!handle) {
        trace ("dlopen error: %s\n", dlerror ());
#ifdef ANDROID
        return -1;
#else
        strcpy (fullname + strlen(fullname) - 3, ".fallback.so");
        trace ("trying %s...\n", fullname);
        handle = dlopen (fullname, RTLD_NOW);
        if (!handle) {
            //trace ("dlopen error: %s\n", dlerror ());
            return -1;
        }
        else {
            fprintf (stderr, "successfully started fallback plugin %s\n", fullname);
        }
#endif
    }
    d_name[l-3] = 0;
    strcat (d_name, "_load");
#ifndef ANDROID
    DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name);
#else
    DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name+3);
#endif
    if (!plug_load) {
        trace ("dlsym error: %s\n", dlerror ());
        dlclose (handle);
        return -1;
    }
    if (plug_init_plugin (plug_load, handle) < 0) {
        d_name[l-3] = 0;
        dlclose (handle);
        return -1;
    }
    return 0;
}

static int
load_gui_plugin (const char **plugdirs) {
    char conf_gui_plug[100];
    conf_get_str ("gui_plugin", "GTK2", conf_gui_plug, sizeof (conf_gui_plug));
    char name[100];

    // try to load selected plugin
    for (int i = 0; g_gui_names[i]; i++) {
        trace ("checking GUI plugin: %s\n", g_gui_names[i]);
        if (!strcmp (g_gui_names[i], conf_gui_plug)) {
            trace ("found selected GUI plugin: %s\n", g_gui_names[i]);
            for (int n = 0; plugdirs[n]; n++) {
                snprintf (name, sizeof (name), "ddb_gui_%s.so", conf_gui_plug);
                if (!load_plugin (plugdirs[n], name, strlen (name))) {
                    return 0;
                }
            }
            break;
        }
    }
    trace ("selected GUI plugin not found or failed to load, trying to find another GUI plugin\n");

    // try any plugin
    for (int i = 0; g_gui_names[i]; i++) {
        for (int n = 0; plugdirs[n]; n++) {
            snprintf (name, sizeof (name), "ddb_gui_%s.so", g_gui_names[i]);
            if (!load_plugin (plugdirs[n], name, strlen (name))) {
                return 0;
            }
            else {
                trace ("plugin not found or failed to load\n");
            }
        }
    }
    return -1;
}

int
load_plugin_dir (const char *plugdir) {
    int n = 0;
    char conf_blacklist_plugins[1000];
    conf_get_str ("blacklist_plugins", "", conf_blacklist_plugins, sizeof (conf_blacklist_plugins));
    trace ("loading plugins from %s\n", plugdir);
    struct dirent **namelist = NULL;
    n = scandir (plugdir, &namelist, NULL, dirent_alphasort);
    if (n < 0)
    {
        if (namelist) {
            free (namelist);
        }
        return 0;
    }
    else
    {
        trace ("plug_load_all: scandir found %d files\n", n);
        int i;
        for (i = 0; i < n; i++)
        {
            // skip hidden files and fallback plugins
            while (namelist[i]->d_name[0] != '.'
#ifndef ANDROID
                    && !strstr (namelist[i]->d_name, ".fallback.")
#else
                    && !strstr (namelist[i]->d_name, "libdeadbeef")
#endif
                  )
            {
                int l = strlen (namelist[i]->d_name);
                if (l < 3) {
                    break;
                }
                if (strcasecmp (&namelist[i]->d_name[l-3], ".so")) {
                    break;
                }
                char d_name[256];
                memcpy (d_name, namelist[i]->d_name, l+1);
#ifndef ANDROID
                // no blacklisted
                const uint8_t *p = conf_blacklist_plugins;
                while (*p) {
                    const uint8_t *e = p;
                    while (*e && *e > 0x20) {
                        e++;
                    }
                    if (l-3 == e-p) {
                        if (!strncmp (p, d_name, e-p)) {
                            p = NULL;
                            break;
                        }
                    }
                    p = e;
                    while (*p && *p <= 0x20) {
                        p++;
                    }
                }
                if (!p) {
                    trace ("plugin %s is blacklisted in config file\n", d_name);
                    break;
                }
#endif

                // add gui plugin names
                if (!strncmp (d_name, "ddb_gui_", 8)) {
                    trace ("found gui plugin %s\n", d_name);
                    if (g_num_gui_names >= MAX_GUI_PLUGINS) {
                        fprintf (stderr, "too many gui plugins\n");
                        break; // no more gui plugins allowed
                    }
                    char *nm = d_name + 8;
                    char *e = strrchr (nm, '.');
                    if (!e) {
                        break;
                    }
                    if (strcmp (e, ".so")) {
                        break;
                    }
                    *e = 0;
                    // ignore fallbacks
                    e = strrchr (nm, '.');
                    if (e && !strcasecmp (e, ".fallback")) {
                        break;
                    }
                    // add to list
                    // FIXME check for gui plugins dupes
                    g_gui_names[g_num_gui_names++] = strdup (nm);
                    g_gui_names[g_num_gui_names] = NULL;
                    trace ("added %s gui plugin\n", nm);
                    break;
                }

                if (0 != load_plugin (plugdir, d_name, l)) {
                    trace ("plugin not found or failed to load\n");
                }
                break;
            }
            free (namelist[i]);
        }
        free (namelist);
    }
    return 0;
}

int
plug_load_all (void) {
#if DISABLE_VERSIONCHECK
    trace ("\033[0;31mDISABLE_VERSIONCHECK=1! do not distribute!\033[0;m\n");
#endif

    const char *dirname = deadbeef->get_plugin_dir ();

#ifndef ANDROID
    char *xdg_local_home = getenv ("XDG_LOCAL_HOME");
    char xdg_plugin_dir[1024];

    if (xdg_local_home) {
        strncpy (xdg_plugin_dir, xdg_local_home, sizeof (xdg_plugin_dir));
        xdg_plugin_dir[sizeof(xdg_plugin_dir)-1] = 0;
    } else {
        char *homedir = getenv ("HOME");

        if (!homedir) {
            trace ("plug_load_all: warning: unable to find home directory\n");
            xdg_plugin_dir[0] = 0;
        }
        else {
            int written = snprintf (xdg_plugin_dir, sizeof (xdg_plugin_dir), "%s/.local/lib/deadbeef", homedir);
            if (written > sizeof (xdg_plugin_dir)) {
                trace ("warning: XDG_LOCAL_HOME value is too long: %s. Ignoring.", xdg_local_home);
                xdg_plugin_dir[0] = 0;
            }
        }
    }

    // load from HOME 1st, than replace from installdir if needed
    const char *plugins_dirs[] = { xdg_plugin_dir, dirname, NULL };

    // If xdg_plugin_dir and dirname is the same, we should avoid each plugin
    // to be load twice.
    // XXX: Here absolute path is assumed, however if dirname is a relative
    // path it won't work.
    if (strcmp(xdg_plugin_dir, dirname) == 0) {
        plugins_dirs[1] = NULL;
    }
#else
    const char *plugins_dirs[] = { dirname, NULL };
#endif

    int k = 0;

    while (plugins_dirs[k]) {
        const char *plugdir = plugins_dirs[k++];
        if (!(*plugdir)) {
            continue;
        }
        load_plugin_dir (plugdir);
    }

#ifdef ANDROID
    char plugin_path[1000];
    strncpy (plugin_path, conf_get_str_fast ("android.plugin_path", ""), sizeof (plugin_path)-1);
    plugin_path[sizeof(plugin_path)-1] = 0;
    char *p = plugin_path;
    while (*p) {
        while (*p == ':') {
            p++;
        }
        if (!(*p)) {
            break;
        }
        char *e = strchr (p, ':');
        if (e) {
            *e = 0;
        }

        char path[PATH_MAX];
        snprintf (path, sizeof (path), "/data/data/%s/lib", p);
        load_plugin_dir (path);
        if (!e) {
            break;
        }
        p = e+1;
    }
#endif


    // load gui plugin
    load_gui_plugin (plugins_dirs);

// load all compiled-in modules
#define PLUG(n) extern DB_plugin_t * n##_load (DB_functions_t *api);
#include "moduleconf.h"
#undef PLUG
#define PLUG(n) plug_init_plugin (n##_load, NULL);
#include "moduleconf.h"
#undef PLUG
#ifdef ANDROID
#define PLUG(n) extern DB_plugin_t * n##_load (DB_functions_t *api);
#include "moduleconf-android.h"
#undef PLUG
#define PLUG(n) plug_init_plugin (n##_load, NULL);
#include "moduleconf-android.h"
#undef PLUG
#endif

    plugin_t *plug;
    // categorize plugins
    int numplugins = 0;
    int numdecoders = 0;
    int numvfs = 0;
    int numoutput = 0;
    int numdsp = 0;
    int numplaylist = 0;
    for (plug = plugins; plug; plug = plug->next) {
        g_plugins[numplugins++] = plug->plugin;
        if (plug->plugin->type == DB_PLUGIN_DECODER) {
//            trace ("found decoder plugin %s\n", plug->plugin->name);
            if (numdecoders >= MAX_DECODER_PLUGINS) {
                break;
            }
            g_decoder_plugins[numdecoders++] = (DB_decoder_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_VFS) {
//            trace ("found vfs plugin %s\n", plug->plugin->name);
            if (numvfs >= MAX_VFS_PLUGINS) {
                break;
            }
            g_vfs_plugins[numvfs++] = (DB_vfs_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_OUTPUT) {
//            trace ("found output plugin %s\n", plug->plugin->name);
            if (numoutput >= MAX_OUTPUT_PLUGINS) {
                break;
            }
            g_output_plugins[numoutput++] = (DB_output_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_DSP) {
//            trace ("found dsp plugin %s\n", plug->plugin->name);
            if (numdsp >= MAX_DSP_PLUGINS) {
                break;
            }
            g_dsp_plugins[numdsp++] = (DB_dsp_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_PLAYLIST) {
            if (numplaylist >= MAX_PLAYLIST_PLUGINS) {
                break;
            }
            g_playlist_plugins[numplaylist++] = (DB_playlist_t *)plug->plugin;
        }
    }
    // start plugins
    plugin_t *prev = NULL;
    for (plug = plugins; plug;) {
        if (plug->plugin->start) {
            if (plug->plugin->start () < 0) {
                fprintf (stderr, "plugin %s failed to start, deactivated.\n", plug->plugin->name);
                if (plug->plugin->stop) {
                    plug->plugin->stop ();
                }
                if (plug->handle) {
                    dlclose (plug->handle);
                }
                plug_remove_plugin (plug->plugin);
                if (prev) {
                    prev->next = plug->next;
                }
                else {
                    plugins = plug->next;
                }
                plugin_t *next = plug->next;
                free (plug);
                plug = next;
                continue;
            }
        }
        prev = plug;
        plug = plug->next;
    }
//    trace ("numplugins: %d, numdecoders: %d, numvfs: %d\n", numplugins, numdecoders, numvfs);
    g_plugins[numplugins] = NULL;
    g_decoder_plugins[numdecoders] = NULL;
    g_vfs_plugins[numvfs] = NULL;
    g_output_plugins[numoutput] = NULL;
    g_dsp_plugins[numdsp] = NULL;
    g_playlist_plugins[numplaylist] = NULL;

    // select output plugin
    if (plug_select_output () < 0) {
        trace ("failed to find output plugin!\n");
        return -1;
    }
    return 0;
}

void
plug_connect_all (void) {
    plugin_t *plug;
    plugin_t *prev = NULL;
    for (plug = plugins; plug;) {
        if (plug->plugin->connect) {
            if (plug->plugin->connect () < 0) {
                fprintf (stderr, "plugin %s failed to connect to dependencies, deactivated.\n", plug->plugin->name);

                if (plug->plugin->disconnect) {
                    plug->plugin->disconnect ();
                }
                if (plug->plugin->stop) {
                    plug->plugin->stop ();
                }
                if (plug->handle) {
                    dlclose (plug->handle);
                }
                plug_remove_plugin (plug->plugin);

                if (prev) {
                    prev->next = plug->next;
                }
                else {
                    plugins = plug->next;
                }
                plugin_t *next = plug->next;
                free (plug);
                plug = next;
                continue;
            }
        }
        prev = plug;
        plug = plug->next;
    }

}

void
plug_disconnect_all (void) {
    trace ("plug_disconnect_all\n");
    plugin_t *plug;
    plugin_t *prev = NULL;
    for (plug = plugins; plug;) {
        if (plug->plugin->disconnect) {
            if (plug->plugin->disconnect () < 0) {
                trace ("plugin %s failed to disconnect\n", plug->plugin->name);
            }
        }
        prev = plug;
        plug = plug->next;
    }
}

void
plug_unload_all (void) {
    trace ("plug_unload_all\n");
    plugin_t *p;
    for (p = plugins; p; p = p->next) {
        if (p->plugin->stop) {
            trace ("stopping %s...\n", p->plugin->name);
            fflush (stderr);
            p->plugin->stop ();
        }
    }
    trace ("stopped all plugins\n");
    while (plugins) {
        plugin_t *next = plugins->next;
        if (plugins->handle) {
            dlclose (plugins->handle);
        }
        free (plugins);
        plugins = next;
    }
    for (int i = 0; g_gui_names[i]; i++) {
        free (g_gui_names[i]);
        g_gui_names[i] = NULL;
    }
    plugins_tail = NULL;

    memset (g_plugins, 0, sizeof (g_plugins));
    memset (g_gui_names, 0, sizeof (g_gui_names));
    g_num_gui_names = 0;
    memset (g_decoder_plugins, 0, sizeof (g_decoder_plugins));
    memset (g_vfs_plugins, 0, sizeof (g_vfs_plugins));
    memset (g_dsp_plugins, 0, sizeof (g_dsp_plugins));
    memset (g_output_plugins, 0, sizeof (g_output_plugins));
    output_plugin = NULL;
    memset (g_playlist_plugins, 0, sizeof (g_playlist_plugins));

    trace ("all plugins had been unloaded\n");
}

void
plug_cleanup (void) {
    plug_free_decoder_ids ();
}

struct DB_decoder_s **
plug_get_decoder_list (void) {
    return g_decoder_plugins;
}

struct DB_vfs_s **
plug_get_vfs_list (void) {
    return g_vfs_plugins;
}

struct DB_output_s **
plug_get_output_list (void) {
    return g_output_plugins;
}

struct DB_dsp_s **
plug_get_dsp_list (void) {
    return g_dsp_plugins;
}

struct DB_playlist_s **
plug_get_playlist_list (void) {
    return g_playlist_plugins;
}

struct DB_plugin_s **
plug_get_list (void) {
    return g_plugins;
}

const char **
plug_get_gui_names (void) {
    return (const char **)g_gui_names;
}

DB_output_t *
plug_get_output (void) {
    return output_plugin;
}

int
plug_select_output (void) {
#ifdef ANDROID
    return 0;
#else
    char outplugname[100];
    conf_get_str ("output_plugin", "ALSA output plugin", outplugname, sizeof (outplugname));
    for (int i = 0; g_output_plugins[i]; i++) {
        DB_output_t *p = g_output_plugins[i];
        if (!strcmp (p->plugin.name, outplugname)) {
            trace ("selected output plugin: %s\n", outplugname);
            output_plugin = p;
            break;
        }
    }
    if (!output_plugin) {
        output_plugin = g_output_plugins[0];
        if (output_plugin) {
            trace ("selected output plugin: %s\n", output_plugin->plugin.name);
            conf_set_str ("output_plugin", output_plugin->plugin.name);
        }
    }
    if (!output_plugin) {
        return -1;
    }
    messagepump_push (DB_EV_OUTPUTCHANGED, 0, 0, 0);
    return 0;
#endif
}

void
plug_reinit_sound (void) {
    DB_output_t *prev = plug_get_output ();
    int state = OUTPUT_STATE_STOPPED;

    ddb_waveformat_t fmt = {0};

    streamer_get_output_format (&fmt);
    if (prev) {
        state = prev->state ();
        if (!fmt.channels) {
            memcpy (&fmt, &prev->fmt, sizeof (fmt));
        }
        prev->free ();
    }

    if (plug_select_output () < 0) {
        char outplugname[100];
        conf_get_str ("output_plugin", "ALSA output plugin", outplugname, sizeof (outplugname));
        trace ("failed to select output plugin %s\nreverted to %s\n", outplugname, prev->plugin.name);
        output_plugin = prev;
    }
    DB_output_t *output = plug_get_output ();
    if (fmt.channels) {
        output->setformat (&fmt);
    }
    if (output->init () < 0) {
        streamer_reset (1);
        streamer_set_nextsong (-2, 0);
        return;
    }

    if (state != OUTPUT_STATE_PAUSED && state != OUTPUT_STATE_STOPPED) {
        if (output->play () < 0) {
            trace ("failed to reinit sound output\n");
            streamer_set_nextsong (-2, 0);
        }
    }
}

// list of all unique decoder ids used in current session
static char *decoder_ids[MAX_DECODER_PLUGINS];

const char *
plug_get_decoder_id (const char *id) {
    int i;
    char **lastnull = NULL;
    for (i = 0; i < MAX_DECODER_PLUGINS; i++) {
        if (decoder_ids[i] && !strcmp (id, decoder_ids[i])) {
            return decoder_ids[i];
        }
        else if (!lastnull && !decoder_ids[i]) {
            lastnull = &decoder_ids[i];
        }
    }
    if (!lastnull) {
        return NULL;
    }
    char *newid = strdup (id);
    *lastnull = newid;
    return newid;
}

void
plug_remove_decoder_id (const char *id) {
    int i;
    for (i = 0; i < MAX_DECODER_PLUGINS; i++) {
        if (decoder_ids[i] && !strcmp (decoder_ids[i], id)) {
            free (decoder_ids[i]);
            decoder_ids[i] = NULL;
        }
    }
}

void
plug_free_decoder_ids (void) {
    int i;
    for (i = 0; i < MAX_DECODER_PLUGINS; i++) {
        if (decoder_ids[i]) {
            free (decoder_ids[i]);
            decoder_ids[i] = NULL;
        }
    }
}

DB_decoder_t *
plug_get_decoder_for_id (const char *id) {
    DB_decoder_t **plugins = plug_get_decoder_list ();
    for (int c = 0; plugins[c]; c++) {
        if (!strcmp (id, plugins[c]->plugin.id)) {
            return plugins[c];
        }
    }
    return NULL;
}

DB_plugin_t *
plug_get_for_id (const char *id) {
    DB_plugin_t **plugins = plug_get_list ();
    for (int c = 0; plugins[c]; c++) {
        if (plugins[c]->id && !strcmp (id, plugins[c]->id)) {
            return plugins[c];
        }
    }
    return NULL;
}

int
plug_is_local_file (const char *fname) {
    if (!strncasecmp (fname, "file://", 7)) {
        return 1;
    }

    for (; *fname; fname++) {
        if (!strncmp (fname, "://", 3)) {
            return 0;
        }
    }

    return 1;
}
