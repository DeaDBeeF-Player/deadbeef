/*
  This file is part of Deadbeef Player source code
  http://deadbeef.sourceforge.net

  plugin management

  Copyright (C) 2009-2013 Oleksiy Yakovenko

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

  Oleksiy Yakovenko waker@users.sourceforge.net
*/
#include <Block.h>
#include <ctype.h>
#include <dirent.h>
#include <dispatch/dispatch.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
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
#include "plmeta.h"
#include "volume.h"
#include "streamer.h"
#include <deadbeef/common.h>
#include "conf.h"
#include "junklib.h"
#include "vfs.h"
#include "premix.h"
#include "dsppreset.h"
#include "pltmeta.h"
#include "metacache.h"
#include "tf.h"
#include "playqueue.h"
#include "sort.h"
#include "logger.h"
#include "replaygain.h"
#include "playmodes.h"
#ifdef __APPLE__
#include "cocoautil.h"
#endif
#include "undo/undomanager.h"
#include "viz.h"

DB_plugin_t main_plugin = {
    .type = DB_PLUGIN_MISC,
    .version_major = DB_API_VERSION_MAJOR,
    .version_minor = DB_API_VERSION_MINOR,
    .name = "deadbeef",
    .id = "deadbeef",
    .flags = DDB_PLUGIN_FLAG_LOGGING,
};

//#define DISABLE_VERSIONCHECK 1

#if defined(HAVE_COCOAUI) || defined(OSX_APPBUNDLE)
#define PLUGINEXT ".dylib"
#elif defined __MINGW32__
#define PLUGINEXT ".dll"
#else
#define PLUGINEXT ".so"
#endif

const char *lowprio_plugin_ids[] = {
    "ffmpeg",
    NULL
};

// internal plugin list
typedef struct plugin_s {
    void *handle;
    char *filepath;
    DB_plugin_t *plugin;
    struct plugin_s *next;
} plugin_t;

static plugin_t *plugins;
static plugin_t *plugins_tail;

// this list only gets used during plugin loading,
// then it gets appended to the above "plugins" list,
// and set to NULL
static plugin_t *plugins_lowprio;
static plugin_t *plugins_lowprio_tail;

#define MAX_PLUGINS 100
static DB_plugin_t *g_plugins[MAX_PLUGINS+1];

#define MAX_GUI_PLUGINS 10
static char *g_gui_names[MAX_GUI_PLUGINS+1];
static int g_num_gui_names;

#define MAX_DECODER_PLUGINS 50
static DB_decoder_t *g_decoder_plugins[MAX_DECODER_PLUGINS+1];

#define MAX_VFS_PLUGINS 10
static DB_vfs_t *g_vfs_plugins[MAX_VFS_PLUGINS+1];

#define MAX_DSP_PLUGINS 10
static DB_dsp_t *g_dsp_plugins[MAX_DSP_PLUGINS+1];

#define MAX_OUTPUT_PLUGINS 10
static DB_output_t *g_output_plugins[MAX_OUTPUT_PLUGINS+1];
static DB_output_t *output_plugin = NULL;

#define MAX_PLAYLIST_PLUGINS 10
static DB_playlist_t *g_playlist_plugins[MAX_PLAYLIST_PLUGINS+1];

static uintptr_t background_jobs_mutex;
static int num_background_jobs;

static void
_viz_spectrum_listen_stub (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data)) {
}

static ddb_undo_interface_t *_undo_interface;

static void
_undo_process(void) {
    ddb_undomanager_t *undomanager = ddb_undomanager_shared();

    ddb_undobuffer_t *undobuffer = ddb_undomanager_consume_buffer (undomanager);

    int res = -1;
    if (ddb_undobuffer_has_operations (undobuffer) && _undo_interface != NULL) {
        res = _undo_interface->process_action(undobuffer, ddb_undomanager_get_action_name (undomanager));
    }

    if (res != 0) {
        ddb_undobuffer_free (undobuffer);
    }

    ddb_undomanager_set_action_name (undomanager, NULL);
}

static void
_register_for_undo (ddb_undo_interface_t *interface) {
    _undo_interface = interface;
    interface->initialize (ddb_undomanager_shared ());
}

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
    .streamer_get_playing_track = (DB_playItem_t *(*) (void))streamer_get_playing_track_unsafe,
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
    .plt_sort = (void (*) (ddb_playlist_t *plt, int iter, int id, const char *format, int order))plt_sort,
    .pl_items_copy_junk = (void (*)(DB_playItem_t *from, DB_playItem_t *first, DB_playItem_t *last))pl_items_copy_junk,
    .pl_set_item_replaygain = (void (*)(DB_playItem_t *it, int idx, float value))pl_set_item_replaygain,
    .pl_get_item_replaygain = (float (*)(DB_playItem_t *it, int idx))pl_get_item_replaygain,
    .plt_get_totaltime = (float (*) (ddb_playlist_t *plt))plt_get_totaltime,
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
    .pl_playqueue_push = (int (*) (DB_playItem_t *))playqueue_push,
    .pl_playqueue_clear = playqueue_clear,
    .pl_playqueue_pop = playqueue_pop,
    .pl_playqueue_remove = (void (*) (DB_playItem_t *))playqueue_remove,
    .pl_playqueue_test = (int (*) (DB_playItem_t *))playqueue_test,
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
    // ******* new 1.3 APIs ********
    .streamer_dsp_chain_save = streamer_dsp_chain_save,
    // ******* new 1.4 APIs ********
    .pl_get_meta = (int (*) (DB_playItem_t *it, const char *key, char *val, int size))pl_get_meta,
    .pl_get_meta_raw = (int (*) (DB_playItem_t *it, const char *key, char *val, int size))pl_get_meta_raw,
    .plt_get_meta = (int (*) (ddb_playlist_t *handle, const char *key, char *val, int size))plt_get_meta,
    .pl_meta_exists = (int (*) (DB_playItem_t *it, const char *key))pl_meta_exists,
    // ******* new 1.5 APIs ********
    .vis_waveform_listen = viz_waveform_listen,
    .vis_waveform_unlisten = viz_waveform_unlisten,
    .vis_spectrum_listen = _viz_spectrum_listen_stub,
    .vis_spectrum_listen2 = viz_spectrum_listen,
    .vis_spectrum_unlisten = viz_spectrum_unlisten,
    .audio_set_mute = audio_set_mute,
    .audio_is_mute = audio_is_mute,
    .background_job_increment = background_job_increment,
    .background_job_decrement = background_job_decrement,
    .have_background_jobs = have_background_jobs,
    .plt_get_idx = (int (*)(ddb_playlist_t *))plt_get_idx,
    .plt_save_n = plt_save_n,
    .plt_save_config = (int (*)(ddb_playlist_t *))plt_save_config,
    .listen_file_added = listen_file_added,
    .unlisten_file_added = unlisten_file_added,
    .listen_file_add_beginend = listen_file_add_beginend,
    .unlisten_file_add_beginend = unlisten_file_add_beginend,
    .plt_load2 = (DB_playItem_t * (*) (int visibility, ddb_playlist_t *plt, ddb_playItem_t *after, const char *fname, int *pabort, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data))plt_load2,
    .plt_add_file2 = (int (*) (int visibility, ddb_playlist_t *plt, const char *fname, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data))plt_add_file2,
    .plt_add_dir2 = (int (*) (int visibility, ddb_playlist_t *plt, const char *dirname, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data))plt_add_dir2,
    .plt_insert_file2 = (ddb_playItem_t * (*) (int visibility, ddb_playlist_t *playlist, ddb_playItem_t *after, const char *fname, int *pabort, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data))plt_insert_file2,
    .plt_insert_dir2 = (ddb_playItem_t *(*) (int visibility, ddb_playlist_t *plt, ddb_playItem_t *after, const char *dirname, int *pabort, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data))plt_insert_dir2,
    .plt_add_files_begin = (int (*) (ddb_playlist_t *plt, int visibility))plt_add_files_begin,
    .plt_add_files_end = (void (*) (ddb_playlist_t *plt, int visibility))plt_add_files_end,
    .plt_deselect_all = (void (*) (ddb_playlist_t *plt))plt_deselect_all,
    // ******* new 1.6 APIs ********
    .plt_set_scroll = (void (*) (ddb_playlist_t *plt, int scroll))plt_set_scroll,
    .plt_get_scroll = (int (*) (ddb_playlist_t *plt))plt_get_scroll,
    .tf_compile = tf_compile,
    .tf_free = tf_free,
    .tf_eval= tf_eval,

    .plt_sort_v2 = (void (*) (ddb_playlist_t *plt, int iter, int id, const char *format, int order))plt_sort_v2,

    .playqueue_push = (int (*) (DB_playItem_t *))playqueue_push,
    .playqueue_clear = playqueue_clear,
    .playqueue_pop = playqueue_pop,
    .playqueue_remove = (void (*) (DB_playItem_t *))playqueue_remove,
    .playqueue_test = (int (*) (DB_playItem_t *))playqueue_test,
    .playqueue_get_count = (int (*) (void))playqueue_getcount,
    .playqueue_get_item = (DB_playItem_t *(*) (int n))playqueue_get_item,
    .playqueue_remove_nth = (int (*) (int n))playqueue_remove_nth,
    .playqueue_insert_at = (void (*) (int n, DB_playItem_t *it))playqueue_insert_at,

    .get_system_dir = plug_get_system_dir,

    .action_set_playlist = action_set_playlist,
    .action_get_playlist = action_get_playlist,

    .tf_import_legacy = tf_import_legacy,

    .plt_search_process2 = (void (*) (ddb_playlist_t *plt, const char *text, int select_results))plt_search_process2,
    .plt_process_cue = (DB_playItem_t * (*) (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *it, uint64_t numsamples, int samplerate))plt_process_cue,
    .pl_meta_for_key = (DB_metaInfo_t * (*) (DB_playItem_t *it, const char *key))pl_meta_for_key,

    .log_detailed = ddb_log_detailed,
    .vlog_detailed = ddb_vlog_detailed,
    .log = ddb_log,
    .vlog = ddb_vlog,

    .log_viewer_register = ddb_log_viewer_register,
    .log_viewer_unregister = ddb_log_viewer_unregister,

    .register_fileadd_filter = register_fileadd_filter,
    .unregister_fileadd_filter = unregister_fileadd_filter,

    .metacache_get_string = metacache_get_string,
    .metacache_add_value = metacache_add_value,
    .metacache_get_value = metacache_get_value,
    .metacache_remove_value = metacache_remove_value,

    .replaygain_apply = replaygain_apply,
    .replaygain_apply_with_settings = replaygain_apply_with_settings,
    .replaygain_init_settings = (void (*) (ddb_replaygain_settings_t *settings, DB_playItem_t *it))replaygain_init_settings,

    .sort_track_array = (void (*) (ddb_playlist_t *playlist, DB_playItem_t **tracks, int num_tracks, const char *format, int order))sort_track_array,

    .pl_item_init = (DB_playItem_t *(*)(const char *fname))pl_item_init,

    .pl_item_get_startsample = (int64_t (*) (DB_playItem_t *it))pl_item_get_startsample,
    .pl_item_get_endsample = (int64_t (*) (DB_playItem_t *it))pl_item_get_endsample,
    .pl_item_set_startsample = (void (*) (DB_playItem_t *it, int64_t sample))pl_item_set_startsample,
    .pl_item_set_endsample = (void (*) (DB_playItem_t *it, int64_t sample))pl_item_set_endsample,

    .plt_get_selection_playback_time = (float (*) (ddb_playlist_t *plt))plt_get_selection_playback_time,

    .junk_get_tail_size = junk_get_tail_size,
    .junk_get_tag_offsets = junk_get_tag_offsets,

    .plt_is_loading_cue = (int (*)(ddb_playlist_t *))plt_is_loading_cue,

    .streamer_set_shuffle = streamer_set_shuffle,
    .streamer_get_shuffle = streamer_get_shuffle,
    .streamer_set_repeat = streamer_set_repeat,
    .streamer_get_repeat = streamer_get_repeat,

    .pl_meta_for_key_with_override = (DB_metaInfo_t *(*) (ddb_playItem_t *it, const char *key))pl_meta_for_key_with_override,
    .pl_find_meta_with_override = (const char *(*) (ddb_playItem_t *it, const char *key))pl_find_meta_with_override,
    .pl_get_meta_with_override = (int (*) (ddb_playItem_t *it, const char *key, char *val, size_t size))pl_get_meta_with_override,
    .pl_meta_exists_with_override = (int (*) (ddb_playItem_t *it, const char *key))pl_meta_exists_with_override,

    .plt_item_set_selected = (void (*)(ddb_playlist_t *plt, ddb_playItem_t *it, int sel))pl_set_selected_in_playlist,
    .plt_find_by_name = (ddb_playlist_t * (*) (const char *name))plt_find_by_name,
    .plt_append = (ddb_playlist_t * (*) (const char *title))plt_append,

    .plt_get_head_item = (ddb_playItem_t * (*) (ddb_playlist_t *p, int iter))plt_get_head_item,
    .plt_get_tail_item = (ddb_playItem_t * (*) (ddb_playlist_t *p, int iter))plt_get_tail_item,

    .plug_get_path_for_plugin_ptr = (const char* (*) (DB_plugin_t *plugin_ptr))plug_get_path_for_plugin_ptr,
    .plt_insert_dir3 = (ddb_playItem_t *(*) (int visibility, uint32_t flags, ddb_playlist_t *plt, ddb_playItem_t *after, const char *dirname, int *pabort, int (*callback)(ddb_insert_file_result_t result, const char *fname, void *user_data), void *user_data))plt_insert_dir3,

    .streamer_get_playing_track_safe = (DB_playItem_t *(*) (void))streamer_get_playing_track,
    .plt_move_all_items = (void (*) (ddb_playlist_t *to, ddb_playlist_t *from, ddb_playItem_t *insert_after))plt_move_all_items,
    .undo_process = _undo_process,
    .register_for_undo = _register_for_undo,
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

const char *
plug_get_system_dir (int dir_id) {
    switch (dir_id) {
    case DDB_SYS_DIR_CONFIG:
        return dbconfdir;
    case DDB_SYS_DIR_PREFIX:
        return dbinstalldir;
    case DDB_SYS_DIR_DOC:
        return dbdocdir;
    case DDB_SYS_DIR_PLUGIN:
        return dbplugindir;
    case DDB_SYS_DIR_PIXMAP:
        return dbpixmapdir;
    case DDB_SYS_DIR_CACHE:
        return dbcachedir;
    case DDB_SYS_DIR_PLUGIN_RESOURCES:
        return dbresourcedir;
    }
    return NULL;
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
        int same_id = p->plugin->id && plugin_api->id && !strcmp (p->plugin->id, plugin_api->id);
        int same_name = (!p->plugin->id || !plugin_api->id) && p->plugin->name && plugin_api->name && !strcmp (p->plugin->name, plugin_api->name);
        if (same_id || same_name) {
            if (plugin_api->version_major > p->plugin->version_major || (plugin_api->version_major == p->plugin->version_major && plugin_api->version_minor > p->plugin->version_minor)) {
                trace_err ("found newer version of plugin \"%s\" (%s), replacing\n", plugin_api->id, plugin_api->name);
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
                break;
            }
            else {
                trace_err ("found copy of plugin \"%s\" (%s), but newer version is already loaded\n", plugin_api->id, plugin_api->name)
                return -1;
            }
        }
    }

#if !DISABLE_VERSIONCHECK
    if (plugin_api->api_vmajor != 0 || plugin_api->api_vminor != 0) {
        // version check enabled
        if (DB_API_VERSION_MAJOR != 9 || DB_API_VERSION_MINOR != 9) {
            if (plugin_api->api_vmajor != DB_API_VERSION_MAJOR || plugin_api->api_vminor > DB_API_VERSION_MINOR) {
                trace_err ("WARNING: plugin \"%s\" wants API v%d.%d (got %d.%d), will not be loaded\n", plugin_api->name, plugin_api->api_vmajor, plugin_api->api_vminor, DB_API_VERSION_MAJOR, DB_API_VERSION_MINOR);
                return -1;
            }
        }
    }
    else {
            trace_err ("WARNING: plugin \"%s\" has disabled version check. please don't distribute it!\n", plugin_api->name);
    }
#endif

    plugin_t *plug = malloc (sizeof (plugin_t));
    memset (plug, 0, sizeof (plugin_t));
    plug->plugin = plugin_api;
    plug->handle = handle;

    int lowprio = 0;
    for (int n = 0; lowprio_plugin_ids[n]; n++) {
        if (plugin_api->id && !strcmp (lowprio_plugin_ids[n], plugin_api->id)) {
            lowprio = 1;
            break;
        }
    }

    if (lowprio) {
        if (plugins_lowprio_tail) {
            plugins_lowprio_tail->next = plug;
            plugins_lowprio_tail = plug;
        }
        else {
            plugins_lowprio = plugins_lowprio_tail = plug;
        }
    }
    else {
        if (plugins_tail) {
            plugins_tail->next = plug;
            plugins_tail = plug;
        }
        else {
            plugins = plugins_tail = plug;
        }
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
    // hack for osx to skip *.0.so files
    if (strstr (d_name, ".0.so")) {
        return -1;
    }

    char fullname[PATH_MAX];
    snprintf (fullname, PATH_MAX, "%s/%s", plugdir, d_name);

    // check if the file exists, to avoid printing bogus errors
    struct stat s;
    if (0 != stat (fullname, &s)) {
        return -1;
    }

    trace ("loading plugin %s/%s\n", plugdir, d_name);
    void *handle = dlopen (fullname, RTLD_NOW);
    if (!handle) {
        trace ("dlopen error: %s\n", dlerror ());
#if defined(ANDROID) || defined(OSX_APPBUNDLE)
        return -1;
#else
        strcpy (fullname + strlen(fullname) - sizeof (PLUGINEXT)+1, ".fallback.so");
        trace ("trying %s...\n", fullname);
        handle = dlopen (fullname, RTLD_NOW);
        if (!handle) {
            //trace ("dlopen error: %s\n", dlerror ());
            return -1;
        }
        else {
            trace ("successfully started fallback plugin %s\n", fullname);
        }
#endif
    }
    d_name[l-sizeof (PLUGINEXT)+1] = 0;
    strcat (d_name, "_load");
#ifndef ANDROID
    DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name);
#else
    DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name+3);
#endif
    if (!plug_load) {
        int android = 0;
#ifdef ANDROID
        android = 1;
#endif
        dlclose (handle);
        // don't error after failing to load plugins starting with "lib",
        // e.g. attempting to load a plugin from "libmp4ff.so",
        // except android, where all plugins have lib prefix
        if (android || strlen (d_name) < 3 || memcmp (d_name, "lib", 3)) {
            trace ("dlsym error: %s (%s)\n", dlerror (), d_name + 3);
            return -1;
        }
        return 0;
    }
    if (plug_init_plugin (plug_load, handle) < 0) {
        d_name[l-sizeof (PLUGINEXT)+1] = 0;
        dlclose (handle);
        return -1;
    }
    plugins_tail->filepath = strdup (fullname);
    return 0;
}

static int
load_gui_plugin (const char **plugdirs) {
#if defined HAVE_COCOAUI || defined HAVE_XGUI
    return 0;
#else

    char conf_gui_plug[100];
    conf_get_str ("gui_plugin", "GTK2", conf_gui_plug, sizeof (conf_gui_plug));
    char name[100];

    // try to load selected plugin
    for (int i = 0; g_gui_names[i]; i++) {
        trace ("checking GUI plugin: %s\n", g_gui_names[i]);
        if (!strcmp (g_gui_names[i], conf_gui_plug)) {
            trace ("found selected GUI plugin: %s\n", g_gui_names[i]);
            for (int n = 0; plugdirs[n]; n++) {
                snprintf (name, sizeof (name), "ddb_gui_%s" PLUGINEXT, conf_gui_plug);
                if (!load_plugin (plugdirs[n], name, (int)strlen (name))) {
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
            snprintf (name, sizeof (name), "ddb_gui_%s" PLUGINEXT, g_gui_names[i]);
            if (!load_plugin (plugdirs[n], name, (int)strlen (name))) {
                return 0;
            }
            else {
                trace ("the plugin not found or failed to load\n");
            }
        }
    }
    return -1;
#endif
}

static int
load_plugin_dir (const char *plugdir, int gui_scan) {
    int n = 0;
    char conf_blacklist_plugins[1000];
    conf_get_str ("blacklist_plugins", "", conf_blacklist_plugins, sizeof (conf_blacklist_plugins));
    if (gui_scan) {
        trace ("searching for GUI plugins in %s\n", plugdir);
    }
    else {
        trace ("loading plugins from %s\n", plugdir);
    }
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
        trace ("load_plugin_dir %s: scandir found %d files\n", plugdir, n);
        int i;
        for (i = 0; i < n; i++)
        {
            // skip hidden files and fallback plugins
            while (namelist[i]->d_name[0] != '.'
#if !defined(ANDROID) && !defined(OSX_APPBUNDLE)
                    && !strstr (namelist[i]->d_name, ".fallback.")
#elif !defined(ANDROID)
                    && !strstr (namelist[i]->d_name, "libdeadbeef")
#endif
                  )
            {
                size_t l = strlen (namelist[i]->d_name);
                if (l < (sizeof(PLUGINEXT)-1)) {
                    break;
                }
                if (strcasecmp (namelist[i]->d_name + l - sizeof(PLUGINEXT) + 1, PLUGINEXT)) {
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
                    if (l-sizeof (PLUGINEXT)+1 == (uintptr_t)(e-p)) {
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
                    if (gui_scan) {
                        trace ("found gui plugin %s\n", d_name);
                        if (g_num_gui_names >= MAX_GUI_PLUGINS) {
                            trace_err ("too many gui plugins\n");
                            break; // no more gui plugins allowed
                        }
                        char *nm = d_name + 8;
                        char *e = strrchr (nm, '.');
                        if (!e) {
                            break;
                        }
                        if (strcmp (e, PLUGINEXT)) {
                            break;
                        }
                        *e = 0;
                        // ignore fallbacks
                        e = strrchr (nm, '.');
                        if (e && !strcasecmp (e, ".fallback")) {
                            break;
                        }
                        // add to list of unique names
                        size_t idx;
                        for (idx = 0; g_gui_names[idx] && strcmp(g_gui_names[idx], nm); idx++);
                        if (!g_gui_names[idx]) {
                            g_gui_names[idx] = strdup (nm);
                            g_gui_names[++g_num_gui_names] = NULL;
                            trace ("added %s gui plugin\n", nm);
                        }
                    }
                    break;
                }

                if (!gui_scan) {
                    if (0 != load_plugin (plugdir, d_name, (int)l)) {
                        trace ("plugin %s not found or failed to load\n", d_name);
                    }
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

    background_jobs_mutex = mutex_create ();

    const char *dirname = plug_get_system_dir (DDB_SYS_DIR_PLUGIN);

    // remember how many plugins to skip if called Nth time
    plugin_t *prev_plugins_tail = plugins_tail;

#ifdef OSX_APPBUNDLE
    char libpath[PATH_MAX];
    int res = cocoautil_get_application_support_path (libpath, sizeof (libpath));
    if (!res) {
        strncat (libpath, "/Deadbeef/Plugins", sizeof (libpath) - strlen (libpath) - 1);
    }
    const char *plugins_dirs[] = { dirname, !res ? libpath : NULL, NULL };
#else
#ifndef ANDROID
    char *xdg_local_home = getenv (LOCALDIR);
    char xdg_plugin_dir[1024];
    char xdg_plugin_dir_explicit_arch[1024];

    if (xdg_local_home) {
        strncpy (xdg_plugin_dir, xdg_local_home, sizeof (xdg_plugin_dir));
        xdg_plugin_dir[sizeof(xdg_plugin_dir)-1] = 0;
        xdg_plugin_dir_explicit_arch[0] = 0;
    } else {
        char *homedir = getenv (HOMEDIR);

        if (!homedir) {
            trace_err ("plug_load_all: warning: unable to find home directory\n");
            xdg_plugin_dir[0] = 0;
            xdg_plugin_dir_explicit_arch[0] = 0;
        }
        else {
            // multilib support:
            // 1. load from lib$ARCH if present
            // 2. load from lib if present
            int written = snprintf (xdg_plugin_dir, sizeof (xdg_plugin_dir), LOCAL_PLUGINS_DIR, homedir);
            if (written > sizeof (xdg_plugin_dir)) {
                trace_err ("warning: XDG_LOCAL_HOME value is too long: %s. Ignoring.", xdg_local_home);
                xdg_plugin_dir[0] = 0;
            }
#ifdef __x86_64__
#define ARCH_BITS 64
#elif defined(__i386__)
#define ARCH_BITS 32
#else
#define ARCH_BITS (int)(sizeof (long) * 8)
#endif
            written = snprintf (xdg_plugin_dir_explicit_arch, sizeof (xdg_plugin_dir_explicit_arch), LOCAL_ARCH_PLUGINS_DIR, homedir, ARCH_BITS);
            if (written > sizeof (xdg_plugin_dir_explicit_arch)) {
                trace_err ("warning: XDG_LOCAL_HOME value is too long: %s. Ignoring.", xdg_local_home);
                xdg_plugin_dir_explicit_arch[0] = 0;
            }
        }
    }

    // load from HOME 1st, than replace from installdir if needed
    const char *plugins_dirs[] = { xdg_plugin_dir_explicit_arch, xdg_plugin_dir, dirname, NULL };

    // If xdg_plugin_dir and dirname is the same, we should avoid each plugin
    // to be load twice.
    // XXX: Here absolute path is assumed, however if dirname is a relative
    // path it won't work.
    if (strcmp(xdg_plugin_dir, dirname) == 0) {
        xdg_plugin_dir[0] = 0;
    }
#else
    const char *plugins_dirs[] = { dirname, NULL };
#endif
#endif

#ifndef ANDROID
// this filepath is always in the android.plugin_path config var
    int k = 0;
#ifndef ANDROID
    // load gui plugin before others
    while (plugins_dirs[k]) {
        const char *plugdir = plugins_dirs[k++];
        if (!(*plugdir)) {
            continue;
        }
        load_plugin_dir (plugdir, 1);
    }
    trace ("load gui plugin\n");
    load_gui_plugin (plugins_dirs);
#endif

    k = 0;
    while (plugins_dirs[k]) {
        const char *plugdir = plugins_dirs[k++];
        if (!(*plugdir)) {
            continue;
        }
        load_plugin_dir (plugdir, 0);
    }
#endif

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
        load_plugin_dir (path, 0);
        if (!e) {
            break;
        }
        p = e+1;
    }
#endif

// load all compiled-in modules
#define PLUG(n) extern DB_plugin_t * n##_load (DB_functions_t *api);
#include "moduleconf.h"
#undef PLUG
#define PLUG(n) plug_init_plugin (n##_load, NULL);
#include "moduleconf.h"
#undef PLUG

    if (plugins_lowprio) {
        if (plugins_tail) {
            plugins_tail->next = plugins_lowprio;
        }
        else {
            plugins = plugins_tail = plugins_lowprio;
        }
        while (plugins_tail->next) {
            plugins_tail = plugins_tail->next;
        }
        plugins_lowprio = plugins_lowprio_tail = NULL;
    }

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
    plugin_t *head = prev_plugins_tail ? prev_plugins_tail->next : plugins;
    for (plug = head; plug;) {
        trace ("starting plugin %s\n", plug->plugin->name);
        if (plug->plugin->type != DB_PLUGIN_GUI && plug->plugin->start) {
            if (plug->plugin->start () < 0) {
                trace_err ("plugin %s failed to start, deactivated.\n", plug->plugin->name);
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
#ifndef XCTEST
    if (plug_reinit_sound () < 0) {
        trace ("failed to find output plugin!\n");
        return -1;
    }
#endif
    return 0;
}

void
plug_connect_all (void) {
    plugin_t *plug;
    plugin_t *prev = NULL;
    for (plug = plugins; plug;) {
        if (plug->plugin->connect) {
            if (plug->plugin->connect () < 0) {
                // NOTE: This message is confusing, and looks like a serious error.
                // Potentially, it should be logged at some additional verbose log level.
                // trace ("plugin %s failed to connect to dependencies, deactivated.\n", plug->plugin->name);

                if (plug->plugin->disconnect) {
                    plug->plugin->disconnect ();
                }
                if (plug->plugin->type != DB_PLUGIN_GUI && plug->plugin->stop) {
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
    for (plug = plugins; plug;) {
        if (plug->plugin->disconnect) {
            if (plug->plugin->disconnect () < 0) {
                trace ("plugin %s failed to disconnect\n", plug->plugin->name);
            }
        }
        plug = plug->next;
    }
}

static void
_plug_unload_stop_complete (void);

static int _async_stop_count = 0;
static void (^_async_stop_completion_block)(void);

static void
_handle_async_stop (void) {
    _async_stop_count -= 1;
    if (_async_stop_count == 0) {
        _plug_unload_stop_complete();
    }
}

static void
_plug_unload_stop_complete (void) {
    // Stop the normal plugins with synchronous stop
    for (plugin_t *p = plugins; p; p = p->next) {
        if (p->plugin->stop && !(p->plugin->flags & DDB_PLUGIN_FLAG_ASYNC_STOP)) {
            trace ("Stopping %s...\n", p->plugin->name);
            fflush (stderr);
#if HAVE_COCOAUI
            if (p->plugin->type == DB_PLUGIN_GUI) {
                continue;
            }
#endif
            p->plugin->stop ();
        }
    }

    while (plugins) {
        plugin_t *next = plugins->next;
        if (plugins->handle) {
            dlclose (plugins->handle);
        }
        free (plugins->filepath);
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

    trace ("All plugins had been unloaded\n");
    if (background_jobs_mutex) {
        mutex_free (background_jobs_mutex);
        background_jobs_mutex = 0;
    }
    _async_stop_completion_block();
    Block_release(_async_stop_completion_block);
    _async_stop_completion_block = NULL;
}

void
plug_unload_all (void(^completion_block)(void)) {
    _async_stop_completion_block = Block_copy(completion_block);
    action_set_playlist (NULL);
    trace ("plug_unload_all\n");
    trace ("Waiting for async plugins to finish...\n");
    _async_stop_count = 1;
    for (plugin_t *p = plugins; p; p = p->next) {
        if ((p->plugin->flags & DDB_PLUGIN_FLAG_ASYNC_STOP) && p->plugin->command) {
            _async_stop_count += 1;
            if (0 > p->plugin->command(DDB_COMMAND_PLUGIN_ASYNC_STOP, ^{
                trace ("Stopped %s...\n", p->plugin->name);
                _handle_async_stop();
            })) {
                _async_stop_count -= 1;
            }
        }
    }
    _handle_async_stop();
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

#pragma mark - Output proxy

// A proxy output plugin which helps to catch output state changes and convert them to events.

static ddb_playback_state_t _curr_playback_state = (ddb_playback_state_t)-1;

static void
call_notify_state_change(void (^block)(void)) {
    block();
    ddb_playback_state_t state = output_plugin->state();
    if (state != _curr_playback_state) {
        _curr_playback_state = state;
        deadbeef->sendmessage(DB_EV_PLAYBACK_STATE_DID_CHANGE, 0, state, 0);
    }
}

static int _out_init (void) {
    return output_plugin->init();
}

static int _out_free (void) {
    return output_plugin->free();
}

static int _out_setformat (ddb_waveformat_t *fmt) {
    __block ddb_playback_state_t result;
    call_notify_state_change(^{
        result = output_plugin->setformat(fmt);
    });
    return result;
}

static int _out_play (void) {
    __block ddb_playback_state_t result;
    call_notify_state_change(^{
        result = output_plugin->play();
    });
    return result;
}

static int _out_stop (void) {
    __block ddb_playback_state_t result;
    call_notify_state_change(^{
        result = output_plugin->stop();
    });
    return result;
}

static int _out_pause (void) {
    __block ddb_playback_state_t result;
    call_notify_state_change(^{
        result = output_plugin->pause();
    });
    return result;
}

static int _out_unpause (void) {
    __block ddb_playback_state_t result;
    call_notify_state_change(^{
        result = output_plugin->unpause();
    });
    return result;
}

static ddb_playback_state_t _out_state (void) {
    __block ddb_playback_state_t result;
    call_notify_state_change(^{
        result = output_plugin->state();
    });
    return result;
}

static void _out_enum_soundcards (void (*callback)(const char *name, const char *desc, void*), void *userdata) {
    if (output_plugin->enum_soundcards) {
        call_notify_state_change(^{
            output_plugin->enum_soundcards(callback, userdata);
        });
    }
}

static DB_output_t _output_proxy = {
    DDB_PLUGIN_SET_API_VERSION
    .init = _out_init,
    .free = _out_free,
    .setformat = _out_setformat,
    .play = _out_play,
    .stop = _out_stop,
    .pause = _out_pause,
    .unpause = _out_unpause,
    .state = _out_state,
    .enum_soundcards = _out_enum_soundcards
};

#pragma mark - Current output plugin

DB_output_t *
plug_get_output (void) {
    if (output_plugin == NULL) {
        return NULL;
    }
    memcpy (&_output_proxy.plugin, &output_plugin->plugin, sizeof (DB_plugin_t));
    memcpy (&_output_proxy.fmt, &output_plugin->fmt, sizeof (ddb_waveformat_t));
    _output_proxy.has_volume = output_plugin->has_volume;
    return &_output_proxy;
}

void
plug_set_output (DB_output_t *out) {
    call_notify_state_change(^{
        output_plugin = out;
        conf_set_str ("output_plugin", output_plugin->plugin.id);
        trace ("selected output plugin: %s\n", output_plugin->plugin.name);
    });
}

static DB_output_t *
_select_output_plugin (void) {
#ifndef ANDROID
    char outplugname[100];
#ifdef OSX_APPBUNDLE
    conf_get_str ("output_plugin", "coreaudio", outplugname, sizeof (outplugname));
#else
    conf_get_str ("output_plugin", "alsa", outplugname, sizeof (outplugname));
#endif
    for (int i = 0; g_output_plugins[i]; i++) {
        DB_output_t *p = g_output_plugins[i];
        if (!strcmp (p->plugin.id, outplugname)
            || !strcmp (p->plugin.name, outplugname)) {
            return p;
        }
    }
#endif
    return g_output_plugins[0];
}

int
plug_reinit_sound (void) {
    DB_output_t *new_out = _select_output_plugin ();
    if (!new_out) {
        return -1;
    }

    streamer_set_output (new_out);
    return 0;
}

#pragma mark -

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
    if (!id) {
        return NULL;
    }
    DB_decoder_t **decoder_plugins = plug_get_decoder_list ();
    for (int c = 0; decoder_plugins[c]; c++) {
        if (!strcmp (id, decoder_plugins[c]->plugin.id)) {
            return decoder_plugins[c];
        }
    }
    return NULL;
}

DB_plugin_t *
plug_get_for_id (const char *id) {
    DB_plugin_t **all_plugins = plug_get_list ();
    for (int c = 0; all_plugins[c]; c++) {
        if (all_plugins[c]->id && !strcmp (id, all_plugins[c]->id)) {
            return all_plugins[c];
        }
    }
    return NULL;
}

const char *
plug_get_path_for_plugin_ptr (DB_plugin_t *plugin_ptr) {
    plugin_t *p;
    for (p = plugins; p; p = p->next) {
        if (p->plugin == plugin_ptr) {
            return p->filepath;
        }
    }
    return NULL;
}

int
plug_is_local_file (const char *fname) {
    if (!strncasecmp (fname, "file://", 7)) {
        return 1;
    }

    const char *f = fname;
    for (; *f; f++) {
        if (*f != ':' && !isalpha (*f)) {
            break;
        }
        if (!strncmp (f, "://", 3)) {
            DB_vfs_t **plug = plug_get_vfs_list ();
            for (int i = 0; plug[i]; i++) {
                if (plug[i]->get_schemes && plug[i]->is_streaming && plug[i]->is_streaming()) {
                    const char **sch = plug[i]->get_schemes ();
                    for (int k = 0; sch[k]; k++) {
                        if (!strncmp (sch[k], fname, strlen (sch[k]))) {
                            return 0;
                        }
                    }
                }
            }
            break;
        }
    }

    return 1;
}

static int is_url (const char *path_or_url) {
    const char *f = path_or_url;
    for (; *f; f++) {
        if (*f != ':' && !isalpha (*f)) {
            break; // not a URL
        }
        if (!strncmp (f, "://", 3)) {
            return 1; // some URL
        }
    }
    return 0;
}

int
is_relative_path_posix (const char *path_or_url) {
    // file url?
    if (!strncasecmp (path_or_url, "file://", 7)) {
        path_or_url += 7;
    }

    // other url?
    if (is_url (path_or_url)) {
        return 0;
    }

    // path starts with a '/'?
    return *path_or_url != '/';
}

int
is_relative_path_win32 (const char *path_or_url) {
    // file url?
    if (!strncasecmp (path_or_url, "file://", 7)) {
        path_or_url += 7;
    }
    else {
        // other url?
        if (is_url (path_or_url)) {
            return 0;
        }
    }

    // absolute paths start with "C:\" (or any other letter)
    // UNC paths can also be absolute (starting with "\\")
    // NOTE: this test won't cover \\? relative path and absolute path starting with "\"
    if (strlen (path_or_url) >= 3) {
        if (isalpha(path_or_url[0]) && path_or_url[1] == ':' && (path_or_url[2] == '\\' || path_or_url[2] == '/')) {
            return 0;
        }
        else if ((path_or_url[0] == '\\' && path_or_url[1] == '\\') || (path_or_url[0] == '/' && path_or_url[1] == '/')) {
            return 0;
        }
    }
    return 1;
}

#ifndef _WIN32
int
is_relative_path (const char *path_or_url) {
    return is_relative_path_posix (path_or_url);
}
#else
int
is_relative_path (const char *path_or_url) {
    return is_relative_path_win32 (path_or_url);
}
#endif

void
background_job_increment (void) {
    mutex_lock (background_jobs_mutex);
    num_background_jobs++;
    mutex_unlock (background_jobs_mutex);
}

void
background_job_decrement (void) {
    mutex_lock (background_jobs_mutex);
    num_background_jobs--;
    mutex_unlock (background_jobs_mutex);
}

int
have_background_jobs (void) {
    return num_background_jobs;
}

static ddb_playlist_t *action_playlist;

void
action_set_playlist (ddb_playlist_t *plt) {
    if (action_playlist) {
        plt_unref ((playlist_t *)action_playlist);
    }
    action_playlist = plt;
    if (action_playlist) {
        plt_ref ((playlist_t *)action_playlist);
    }
}

ddb_playlist_t *
action_get_playlist (void) {
    if (!action_playlist) {
        return (ddb_playlist_t *)plt_get_curr ();
    }

    plt_ref ((playlist_t *)action_playlist);
    return action_playlist;
}

// for tests
void
plug_register_in (DB_plugin_t *inplug) {
    int i;
    for (i = 0; g_plugins[i]; i++);
    g_plugins[i++] = inplug;
    g_plugins[i] = NULL;

    for (i = 0; g_decoder_plugins[i]; i++);
    g_decoder_plugins[i++] = (DB_decoder_t *)inplug;
    g_decoder_plugins[i] = NULL;
}

// for tests
void
plug_register_out (DB_plugin_t *outplug) {
    int i;
    for (i = 0; g_plugins[i]; i++);
    g_plugins[i++] = outplug;
    g_plugins[i] = NULL;

    for (i = 0; g_output_plugins[i]; i++);
    g_output_plugins[i++] = (DB_output_t *)outplug;
    g_output_plugins[i] = NULL;
}

// for tests
DB_functions_t *
plug_get_api (void) {
    return &deadbeef_api;
}

