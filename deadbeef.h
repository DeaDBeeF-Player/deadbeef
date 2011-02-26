/*
  deadbeef.h -- plugin API of the DeaDBeeF audio player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2011 Alexey Yakovenko

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

  Note: DeaDBeeF player itself uses different license
*/


#ifndef __DEADBEEF_H
#define __DEADBEEF_H

#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

// every plugin must define following entry-point:
// extern "C" DB_plugin_t* $MODULENAME_load (DB_functions_t *api);
// where $MODULENAME is a name of module
// e.g. if your plugin is called "myplugin.so", $MODULENAME is "myplugin"
// this function should return pointer to DB_plugin_t structure
// that is enough for both static and dynamic modules

// add DB_PLUGIN_SET_API_VERSION macro when you define plugin structure
// like this:
// static DB_decoder_t plugin = {
//   DB_PLUGIN_SET_API_VERSION
//  ............
// }
// this is required for versioning
// if you don't do it -- no version checking will be done (useful for
// debugging/development)
// DON'T release plugins without DB_PLUGIN_SET_API_VERSION

// api version history:
// 9.9 -- devel
// 0.10 -- deadbeef-0.4.4-portable-r1
// 0.9 -- deadbeef-0.4.3-portable-build3
// 0.8 -- deadbeef-0.4.2
// 0.7 -- deabdeef-0.4.0
// 0.6 -- deadbeef-0.3.3
// 0.5 -- deadbeef-0.3.2
// 0.4 -- deadbeef-0.3.0
// 0.3 -- deadbeef-0.2.3.2
// 0.2 -- deadbeef-0.2.3
// 0.1 -- deadbeef-0.2.0

#define DB_API_VERSION_MAJOR 9
#define DB_API_VERSION_MINOR 9

#define DB_PLUGIN_SET_API_VERSION\
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,\
    .plugin.api_vminor = DB_API_VERSION_MINOR,

#define MAX_DECODER_PLUGINS 50

////////////////////////////
// playlist structures

// iterators
// that's a good candidate for redesign
#define PL_MAIN 0
#define PL_SEARCH 1

enum {
    DDB_IS_SUBTRACK = (1<<0), // file is not single-track, might have metainfo in external file
    DDB_IS_READONLY = (1<<1), // check this flag to block tag writing (e.g. in iso.wv)
    DDB_HAS_EMBEDDED_CUESHEET = (1<<2),

    DDB_TAG_ID3V1 = (1<<8),
    DDB_TAG_ID3V22 = (1<<9),
    DDB_TAG_ID3V23 = (1<<10),
    DDB_TAG_ID3V24 = (1<<11),
    DDB_TAG_APEV2 = (1<<12),
    DDB_TAG_VORBISCOMMENTS = (1<<13),
    DDB_TAG_CUESHEET = (1<<14),
    DDB_TAG_ICY = (1<<15),

    DDB_TAG_MASK = 0x0000ff00
};

// playlist item
// these are "public" fields, available to plugins
typedef struct DB_playItem_s {
    char *fname; // full pathname
    const char *decoder_id;
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    int startsample; // start sample of track, or -1 for auto
    int endsample; // end sample of track, or -1 for auto
    int shufflerating; // sort order for shuffle mode
    float playtime; // actual playback time of this track in seconds
    time_t started_timestamp; // result of calling time(NULL)
    const char *filetype; // e.g. MP3 or OGG
    float replaygain_album_gain;
    float replaygain_album_peak;
    float replaygain_track_gain;
    float replaygain_track_peak;
} DB_playItem_t;

typedef struct DB_metaInfo_s {
    struct DB_metaInfo_s *next;
    const char *key;
    const char *value;
} DB_metaInfo_t;

// FIXME: that needs to be in separate plugin

#define JUNK_STRIP_ID3V2 1
#define JUNK_STRIP_APEV2 2
#define JUNK_STRIP_ID3V1 4
#define JUNK_WRITE_ID3V2 8
#define JUNK_WRITE_APEV2 16
#define JUNK_WRITE_ID3V1 32

typedef struct DB_id3v2_frame_s {
    struct DB_id3v2_frame_s *next;
    char id[5];
    uint32_t size;
    uint8_t flags[2];
    uint8_t data[0];
} DB_id3v2_frame_t;

typedef struct DB_id3v2_tag_s {
    uint8_t version[2];
    uint8_t flags;
    DB_id3v2_frame_t *frames;
} DB_id3v2_tag_t;

typedef struct DB_apev2_frame_s {
    struct DB_apev2_frame_s *next;
    uint32_t flags;
    char key[256];
    uint32_t size; // size of data
    uint8_t data[0];
} DB_apev2_frame_t;

typedef struct DB_apev2_tag_s {
    uint32_t version;
    uint32_t flags;
    DB_apev2_frame_t *frames;
} DB_apev2_tag_t;

// plugin types
enum {
    DB_PLUGIN_DECODER = 1,
    DB_PLUGIN_OUTPUT  = 2,
    DB_PLUGIN_DSP     = 3,
    DB_PLUGIN_MISC    = 4,
    DB_PLUGIN_VFS     = 5,
    DB_PLUGIN_PLAYLIST = 6,
};

// output plugin states
enum output_state_t {
    OUTPUT_STATE_STOPPED = 0,
    OUTPUT_STATE_PLAYING = 1,
    OUTPUT_STATE_PAUSED = 2,
};

// playback order
enum playback_order_t {
    PLAYBACK_ORDER_LINEAR = 0,
    PLAYBACK_ORDER_SHUFFLE_TRACKS = 1,
    PLAYBACK_ORDER_RANDOM = 2,
    PLAYBACK_ORDER_SHUFFLE_ALBUMS = 3,
};

// playback modes
enum playback_mode_t {
    PLAYBACK_MODE_LOOP_ALL = 0, // loop playlist
    PLAYBACK_MODE_NOLOOP = 1, // don't loop
    PLAYBACK_MODE_LOOP_SINGLE = 2, // loop single track
};

typedef struct {
    int event;
    time_t time;
} DB_event_t;

typedef struct {
    DB_event_t ev;
    DB_playItem_t *track;
} DB_event_track_t;

typedef struct {
    DB_event_t ev;
    DB_playItem_t *from;
    DB_playItem_t *to;
} DB_event_trackchange_t;

typedef struct {
    DB_event_t ev;
    int state;
} DB_event_state_t;

typedef struct DB_conf_item_s {
    char *key;
    char *value;
    struct DB_conf_item_s *next;
} DB_conf_item_t;

// event callback type
typedef int (*DB_callback_t)(DB_event_t *, uintptr_t data);

// events
enum {
    DB_EV_FRAMEUPDATE = 0, // ticks around 20 times per second, but ticker may stop sometimes
    DB_EV_SONGCHANGED = 1, // triggers when song was just changed
    DB_EV_SONGSTARTED = 2, // triggers when song started playing (for scrobblers and such)
    DB_EV_SONGFINISHED = 3, // triggers when song finished playing (for scrobblers and such)
    DB_EV_CONFIGCHANGED = 5, // configuration option changed
    DB_EV_ACTIVATE = 6, // will be fired every time player is activated
    DB_EV_TRACKINFOCHANGED = 7, // notify plugins that trackinfo was changed
    DB_EV_PAUSED = 8, // player was paused or unpaused
    DB_EV_PLAYLISTCHANGED = 9, // playlist contents were changed
    DB_EV_VOLUMECHANGED = 10, // volume was changed
    DB_EV_OUTPUTCHANGED = 11, // sound output plugin changed
    DB_EV_PLAYLISTSWITCH = 13, // playlist switch occured
    DB_EV_MAX
};

// preset columns, working using IDs
// DON'T add new ids in range 2-7, they are reserved for backwards compatibility
enum pl_column_t {
    DB_COLUMN_FILENUMBER = 0,
    DB_COLUMN_PLAYING = 1,
    DB_COLUMN_ALBUM_ART = 8,
    DB_COLUMN_ID_MAX
};

// message ids for communicating with player
enum {
    M_SONGFINISHED,
    M_NEXT,
    M_PREV,
    M_PLAY_CURRENT,
    M_PLAY_NUM,
    M_STOP,
    M_PAUSE,
    M_PLAY_RANDOM,
    M_TERMINATE, // must be sent to player thread to terminate
    M_PLAYLIST_REFRESH, // means
    M_REINIT_SOUND,
    M_CONFIG_CHANGED, // no arguments
    M_TOGGLE_PAUSE,
};

// typecasting macros
#define DB_PLUGIN(x) ((DB_plugin_t *)(x))
#define DB_CALLBACK(x) ((DB_callback_t)(x))
#define DB_EVENT(x) ((DB_event_t *)(x))
#define DB_PLAYITEM(x) ((DB_playItem_t *)(x))

// FILE object wrapper for vfs access
typedef struct {
    struct DB_vfs_s *vfs;
} DB_FILE;

// md5 calc control structure (see md5/md5.h)
typedef struct DB_md5_s {
    char data[88];
} DB_md5_t;

typedef struct {
    int bps;
    int is_float; // bps must be 32 if this is true
    int channels;
    int samplerate;
    uint32_t channelmask;
} ddb_waveformat_t;

// forward decl for plugin struct
struct DB_plugin_s;

// player api definition
typedef struct {
    // versioning
    int vmajor;
    int vminor;
    // event subscribing
    void (*ev_subscribe) (struct DB_plugin_s *plugin, int ev, DB_callback_t callback, uintptr_t data);
    void (*ev_unsubscribe) (struct DB_plugin_s *plugin, int ev, DB_callback_t callback, uintptr_t data);
    // md5sum calc
    void (*md5) (uint8_t sig[16], const char *in, int len);
    void (*md5_to_str) (char *str, const uint8_t sig[16]);
    void (*md5_init)(DB_md5_t *s);
    void (*md5_append)(DB_md5_t *s, const uint8_t *data, int nbytes);
    void (*md5_finish)(DB_md5_t *s, uint8_t digest[16]);
    // playback control
    struct DB_output_s* (*get_output) (void);
    float (*playback_get_pos) (void); // [0..100]
    void (*playback_set_pos) (float pos); // [0..100]
    // streamer access
    DB_playItem_t *(*streamer_get_playing_track) (void);
    DB_playItem_t *(*streamer_get_streaming_track) (void);
    float (*streamer_get_playpos) (void);
    void (*streamer_seek) (float time);
    int (*streamer_ok_to_read) (int len);
    void (*streamer_reset) (int full);
    int (*streamer_read) (char *bytes, int size);
    void (*streamer_set_bitrate) (int bitrate);
    int (*streamer_get_apx_bitrate) (void);
    struct DB_fileinfo_s *(*streamer_get_current_fileinfo) (void);
    int (*streamer_get_current_playlist) (void);
    struct ddb_dsp_context_s * (*streamer_get_dsp_chain) (void);
    void (*streamer_set_dsp_chain) (struct ddb_dsp_context_s *chain);
    // system folders
    // normally functions will return standard folders derived from --prefix
    // portable version will return pathes specified in comments below
    const char *(*get_config_dir) (void); // installdir/config | $XDG_CONFIG_HOME/.config/deadbeef
    const char *(*get_prefix) (void); // installdir | PREFIX
    const char *(*get_doc_dir) (void); // installdir/doc | DOCDIR
    const char *(*get_plugin_dir) (void); // installdir/plugins | LIBDIR/deadbeef
    const char *(*get_pixmap_dir) (void); // installdir/pixmaps | PREFIX "/share/deadbeef/pixmaps"
    // process control
    void (*quit) (void);
    // threading
    intptr_t (*thread_start) (void (*fn)(void *ctx), void *ctx);
    intptr_t (*thread_start_low_priority) (void (*fn)(void *ctx), void *ctx);
    int (*thread_join) (intptr_t tid);
    int (*thread_detach) (intptr_t tid);
    void (*thread_exit) (void *retval);
    uintptr_t (*mutex_create) (void);
    uintptr_t (*mutex_create_nonrecursive) (void);
    void (*mutex_free) (uintptr_t mtx);
    int (*mutex_lock) (uintptr_t mtx);
    int (*mutex_unlock) (uintptr_t mtx);
    uintptr_t (*cond_create) (void);
    void (*cond_free) (uintptr_t cond);
    int (*cond_wait) (uintptr_t cond, uintptr_t mutex);
    int (*cond_signal) (uintptr_t cond);
    int (*cond_broadcast) (uintptr_t cond);
    // playlist management
    int (*plt_get_count) (void);
    DB_playItem_t * (*plt_get_head) (int plt);
    int (*plt_get_sel_count) (int plt);
    int (*plt_add) (int before, const char *title);
    void (*plt_remove) (int plt);
    void (*plt_free) (void);
    void (*plt_set_curr) (int plt);
    int (*plt_get_curr) (void);
    int (*plt_get_title) (int plt, char *buffer, int bufsize);
    int (*plt_set_title) (int plt, const char *title);
    void (*plt_move) (int from, int before);
    // playlist control
    void (*pl_lock) (void);
    void (*pl_unlock) (void);
    void (*plt_lock) (void);
    void (*plt_unlock) (void);
    // playlist tracks access
    DB_playItem_t * (*pl_item_alloc) (void);
    void (*pl_item_ref) (DB_playItem_t *it);
    void (*pl_item_unref) (DB_playItem_t *it);
    void (*pl_item_copy) (DB_playItem_t *out, DB_playItem_t *in);
    int (*pl_add_file) (const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    int (*pl_add_dir) (const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    void (*pl_add_files_begin) (int playlist);
    void (*pl_add_files_end) (void);
    DB_playItem_t *(*pl_insert_item) (DB_playItem_t *after, DB_playItem_t *it);
    DB_playItem_t *(*pl_insert_dir) (DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    DB_playItem_t *(*pl_insert_file) (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    int (*pl_get_idx_of) (DB_playItem_t *it);
    int (*pl_get_idx_of_iter) (DB_playItem_t *it, int iter);
    DB_playItem_t * (*pl_get_for_idx) (int idx);
    DB_playItem_t * (*pl_get_for_idx_and_iter) (int idx, int iter);
    float (*pl_get_totaltime) (void);
    int (*pl_getcount) (int iter);
    int (*pl_delete_selected) (void);
    void (*pl_set_cursor) (int iter, int cursor);
    int (*pl_get_cursor) (int iter);
    void (*pl_set_selected) (DB_playItem_t *it, int sel);
    int (*pl_is_selected) (DB_playItem_t *it);
    void (*pl_clear) (void);
    int (*pl_load) (const char *name);
    int (*pl_save) (const char *name);
    int (*pl_save_current) (void);
    int (*pl_save_all) (void);
    void (*pl_select_all) (void);
    void (*pl_crop_selected) (void);
    int (*pl_getselcount) (void);
    DB_playItem_t *(*pl_get_first) (int iter);
    DB_playItem_t *(*pl_get_last) (int iter);
    DB_playItem_t *(*pl_get_next) (DB_playItem_t *it, int iter);
    DB_playItem_t *(*pl_get_prev) (DB_playItem_t *it, int iter);
    /*
       this function formats line for display in playlist
       @it pointer to playlist item
       @idx number of that item in playlist (or -1)
       @s output buffer
       @size size of output buffer
       @id one of IDs defined in pl_column_id_t enum, can be -1
       @fmt format string, used if id is -1
       format is printf-alike. specification:
       %a artist
       %t title
       %b album
       %B band / album artist
       %n track
       %l length (duration)
       %y year
       %g genre
       %c comment
       %r copyright
       %T tags
       %f filename without path
       %F full pathname/uri
       %d directory without path (e.g. /home/user/file.mp3 -> user)
       %D directory name with full path (e.g. /home/user/file.mp3 -> /home/user)
       more to come
    */
    int (*pl_format_title) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt);
    // _escaped version wraps all conversions with '' and replaces every ' in conversions with \'
    int (*pl_format_title_escaped) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt);
    void (*pl_format_time) (float t, char *dur, int size);
    void (*pl_move_items) (int iter, int plt_from, DB_playItem_t *drop_before, uint32_t *indexes, int count);
    void (*pl_copy_items) (int iter, int plt_from, DB_playItem_t *before, uint32_t *indices, int cnt);
    void (*pl_search_reset) (void);
    void (*pl_search_process) (const char *text);
    // metainfo
    void (*pl_add_meta) (DB_playItem_t *it, const char *key, const char *value);
    void (*pl_append_meta) (DB_playItem_t *it, const char *key, const char *value);
    // must be used from within explicit pl_lock/unlock block
    const char *(*pl_find_meta) (DB_playItem_t *it, const char *key);
    void (*pl_replace_meta) (DB_playItem_t *it, const char *key, const char *value);
    void (*pl_delete_all_meta) (DB_playItem_t *it);
    DB_metaInfo_t * (*pl_get_metadata) (DB_playItem_t *it);
    void (*pl_set_item_duration) (DB_playItem_t *it, float duration);
    float (*pl_get_item_duration) (DB_playItem_t *it);
    uint32_t (*pl_get_item_flags) (DB_playItem_t *it);
    void (*pl_set_item_flags) (DB_playItem_t *it, uint32_t flags);
    void (*pl_sort) (int iter, int id, const char *format, int ascending);
    void (*pl_items_copy_junk)(DB_playItem_t *from, DB_playItem_t *first, DB_playItem_t *last);

    // playqueue support
    int (*pl_playqueue_push) (DB_playItem_t *it);
    void (*pl_playqueue_clear) (void);
    void (*pl_playqueue_pop) (void);
    void (*pl_playqueue_remove) (DB_playItem_t *it);
    int (*pl_playqueue_test) (DB_playItem_t *it);
    // cuesheet support
    DB_playItem_t *(*pl_insert_cue_from_buffer) (DB_playItem_t *after, DB_playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate);
    DB_playItem_t * (*pl_insert_cue) (DB_playItem_t *after, DB_playItem_t *origin, int numsamples, int samplerate);
    // volume control
    void (*volume_set_db) (float dB);
    float (*volume_get_db) (void);
    void (*volume_set_amp) (float amp);
    float (*volume_get_amp) (void);
    float (*volume_get_min_db) (void);
    // junk reading/writing
    int (*junk_id3v1_read) (DB_playItem_t *it, DB_FILE *fp);
    int (*junk_id3v1_find) (DB_FILE *fp);
    int (*junk_id3v1_write) (FILE *fp, DB_playItem_t *it);
    int (*junk_id3v2_find) (DB_FILE *fp, int *psize);
    int (*junk_id3v2_read) (DB_playItem_t *it, DB_FILE *fp);
    int (*junk_id3v2_read_full) (DB_playItem_t *it, DB_id3v2_tag_t *tag, DB_FILE *fp);
    int (*junk_id3v2_convert_24_to_23) (DB_id3v2_tag_t *tag24, DB_id3v2_tag_t *tag23);
    int (*junk_id3v2_convert_23_to_24) (DB_id3v2_tag_t *tag23, DB_id3v2_tag_t *tag24);
    int (*junk_id3v2_convert_22_to_24) (DB_id3v2_tag_t *tag22, DB_id3v2_tag_t *tag24);
    void (*junk_id3v2_free) (DB_id3v2_tag_t *tag);
    int (*junk_id3v2_write) (FILE *file, DB_id3v2_tag_t *tag);
    DB_id3v2_frame_t *(*junk_id3v2_add_text_frame) (DB_id3v2_tag_t *tag, const char *frame_id, const char *value); 
    int (*junk_id3v2_remove_frames) (DB_id3v2_tag_t *tag, const char *frame_id);
    int (*junk_apev2_read) (DB_playItem_t *it, DB_FILE *fp);
    int (*junk_apev2_read_mem) (DB_playItem_t *it, char *mem, int size);
    int (*junk_apev2_read_full) (DB_playItem_t *it, DB_apev2_tag_t *tag_store, DB_FILE *fp);
    int (*junk_apev2_read_full_mem) (DB_playItem_t *it, DB_apev2_tag_t *tag_store, char *mem, int memsize);
    int (*junk_apev2_find) (DB_FILE *fp, int32_t *psize, uint32_t *pflags, uint32_t *pnumitems);
    int (*junk_apev2_remove_frames) (DB_apev2_tag_t *tag, const char *frame_id);
    DB_apev2_frame_t * (*junk_apev2_add_text_frame) (DB_apev2_tag_t *tag, const char *frame_id, const char *value);
    void (*junk_apev2_free) (DB_apev2_tag_t *tag);
    int (*junk_apev2_write) (FILE *fp, DB_apev2_tag_t *tag, int write_header, int write_footer);
    int (*junk_get_leading_size) (DB_FILE *fp);
    int (*junk_get_leading_size_stdio) (FILE *fp);
    void (*junk_copy) (DB_playItem_t *from, DB_playItem_t *first, DB_playItem_t *last);
    const char * (*junk_detect_charset) (const char *s);
    int (*junk_recode) (const char *in, int inlen, char *out, int outlen, const char *cs);
    int (*junk_iconv) (const char *in, int inlen, char *out, int outlen, const char *cs_in, const char *cs_out);
    int (*junk_rewrite_tags) (DB_playItem_t *it, uint32_t flags, int id3v2_version, const char *id3v1_encoding);
    // vfs
    DB_FILE* (*fopen) (const char *fname);
    void (*fclose) (DB_FILE *f);
    size_t (*fread) (void *ptr, size_t size, size_t nmemb, DB_FILE *stream);
    int (*fseek) (DB_FILE *stream, int64_t offset, int whence);
    int64_t (*ftell) (DB_FILE *stream);
    void (*rewind) (DB_FILE *stream);
    int64_t (*fgetlength) (DB_FILE *stream);
    const char *(*fget_content_type) (DB_FILE *stream);
    void (*fset_track) (DB_FILE *stream, DB_playItem_t *it);
    void (*fabort) (DB_FILE *stream);
    // message passing
    int (*sendmessage) (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);
    // configuration access
    const char * (*conf_get_str) (const char *key, const char *def);
    float (*conf_get_float) (const char *key, float def);
    int (*conf_get_int) (const char *key, int def);
    int64_t (*conf_get_int64) (const char *key, int64_t def);
    void (*conf_set_str) (const char *key, const char *val);
    void (*conf_set_int) (const char *key, int val);
    void (*conf_set_int64) (const char *key, int64_t val);
    void (*conf_set_float) (const char *key, float val);
    DB_conf_item_t * (*conf_find) (const char *group, DB_conf_item_t *prev);
    void (*conf_remove_items) (const char *key);
    int (*conf_save) (void);
    // plugin communication
    struct DB_decoder_s **(*plug_get_decoder_list) (void);
    struct DB_vfs_s **(*plug_get_vfs_list) (void);
    struct DB_output_s **(*plug_get_output_list) (void);
    struct DB_dsp_s **(*plug_get_dsp_list) (void);
    struct DB_playlist_s **(*plug_get_playlist_list) (void);
    struct DB_plugin_s **(*plug_get_list) (void);
    const char * (*plug_get_decoder_id) (const char *id);
    void (*plug_remove_decoder_id) (const char *id);
    struct DB_plugin_s *(*plug_get_for_id) (const char *id);
    // plugin events
    void (*plug_trigger_event_trackchange) (DB_playItem_t *from, DB_playItem_t *to);
    void (*plug_trigger_event_trackinfochanged) (DB_playItem_t *track);
    void (*plug_trigger_event_playlistchanged) (void);
    // misc utilities
    int (*is_local_file) (const char *fname); // returns 1 for local filename, 0 otherwise

    // pcm utilities
    int (*pcm_convert) (const ddb_waveformat_t * inputfmt, const char *input, const ddb_waveformat_t *outputfmt, char *output, int inputsize);

    // dsp preset management
    int (*dsp_preset_load) (const char *fname, struct ddb_dsp_context_s **head);
    int (*dsp_preset_save) (const char *fname, struct ddb_dsp_context_s *head);
    void (*dsp_preset_free) (struct ddb_dsp_context_s *head);
} DB_functions_t;

enum {
    /* Action in main menu (or whereever ui prefers) */
    DB_ACTION_COMMON = 1 << 0,

    /* Action allowed for single track */
    DB_ACTION_SINGLE_TRACK = 1 << 1,

    /* Action allowed for multiple tracks at once */
    DB_ACTION_ALLOW_MULTIPLE_TRACKS = 1 << 2,

    /* Action can (and prefer) traverse multiple tracks by itself */
    DB_ACTION_CAN_MULTIPLE_TRACKS = 1 << 3,
    
    /* Action is inactive */
    DB_ACTION_DISABLED = 1 << 4
};

struct DB_plugin_action_s;

typedef int (*DB_plugin_action_callback_t) (struct DB_plugin_action_s *action, DB_playItem_t *it);

typedef struct DB_plugin_action_s {
    const char *title;
    const char *name;
    uint32_t flags;
    /**
     * Function called when user activates menu item
     * @action pointer to action struct itself
     * @it pointer to selected playitem for single-track action,
     *   to first playitem for multiple-track action,
     *   or NULL for common action
     * @returns unused
     */
    DB_plugin_action_callback_t callback;

    //we have linked list here
    struct DB_plugin_action_s *next;
} DB_plugin_action_t;

// base plugin interface
typedef struct DB_plugin_s {
    // type must be one of DB_PLUGIN_ types
    int32_t type;
    // api version
    int16_t api_vmajor;
    int16_t api_vminor;
    // plugin version
    int16_t version_major;
    int16_t version_minor;

    uint32_t flags; // currently unused
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;

    // any of those can be left NULL
    // though it's much better to fill them with something useful
    const char *id; // id used for serialization and runtime binding
    const char *name; // short name
    const char *descr; // short description
    const char *author; // author's name
    const char *email; // author's email
    const char *website; // author's website

    // plugin-specific command interface; can be NULL
    int (*command) (int cmd, ...);

    // start is called to start plugin; can be NULL
    int (*start) (void);
    
    // stop is called to deinit plugin; can be NULL
    int (*stop) (void);

    // connect is called to setup connections between different plugins
    // it is called after all plugin's start method was executed
    // can be NULL
    int (*connect) (void);

    // opposite of connect, will be called before stop, while all plugins are still
    // in "started" state
    int (*disconnect) (void);
    
    // exec_cmdline may be called at any moment when user sends commandline to player
    // can be NULL if plugin doesn't support commandline processing
    // cmdline is 0-separated list of strings, guaranteed to have 0 at the end
    // cmdline_size is number of bytes pointed by cmdline
    int (*exec_cmdline) (const char *cmdline, int cmdline_size);
    
    // @returns linked list of actions
    DB_plugin_action_t* (*get_actions) (DB_playItem_t *it);

    // plugin configuration dialog is constructed from this data
    // can be NULL
    const char *configdialog;
} DB_plugin_t;

// file format stuff

// channel mask - combine following flags to tell streamer which channels are
// present in input/output streams
enum {
    DDB_SPEAKER_FRONT_LEFT = 0x1,
    DDB_SPEAKER_FRONT_RIGHT = 0x2,
    DDB_SPEAKER_FRONT_CENTER = 0x4,
    DDB_SPEAKER_LOW_FREQUENCY = 0x8,
    DDB_SPEAKER_BACK_LEFT = 0x10,
    DDB_SPEAKER_BACK_RIGHT = 0x20,
    DDB_SPEAKER_FRONT_LEFT_OF_CENTER = 0x40,
    DDB_SPEAKER_FRONT_RIGHT_OF_CENTER = 0x80,
    DDB_SPEAKER_BACK_CENTER = 0x100,
    DDB_SPEAKER_SIDE_LEFT = 0x200,
    DDB_SPEAKER_SIDE_RIGHT = 0x400,
    DDB_SPEAKER_TOP_CENTER = 0x800,
    DDB_SPEAKER_TOP_FRONT_LEFT = 0x1000,
    DDB_SPEAKER_TOP_FRONT_CENTER = 0x2000,
    DDB_SPEAKER_TOP_FRONT_RIGHT = 0x4000,
    DDB_SPEAKER_TOP_BACK_LEFT = 0x8000,
    DDB_SPEAKER_TOP_BACK_CENTER = 0x10000,
    DDB_SPEAKER_TOP_BACK_RIGHT = 0x20000
};

typedef struct DB_fileinfo_s {
    struct DB_decoder_s *plugin;

    // these parameters should be set in decoder->open
    ddb_waveformat_t fmt;

    // readpos should be updated to current decoder time (in seconds) 
    float readpos;

    // this is the (optional) file handle, that can be used by streamer to
    // request interruption of current read operation
    DB_FILE *file;
} DB_fileinfo_t;

enum {
    DDB_DECODER_HINT_16BIT = 0x1, // that flag means streamer prefers 16 bit streams for performance reasons
};

// decoder plugin
typedef struct DB_decoder_s {
    DB_plugin_t plugin;

    DB_fileinfo_t *(*open) (uint32_t hints);

    // init is called to prepare song to be started
    int (*init) (DB_fileinfo_t *info, DB_playItem_t *it);

    // free is called after decoding is finished
    void (*free) (DB_fileinfo_t *info);

    // read is called by streamer to decode specified number of bytes
    // must return number of bytes that were successfully decoded (sample aligned)
    int (*read) (DB_fileinfo_t *info, char *buffer, int nbytes);

    int (*seek) (DB_fileinfo_t *info, float seconds);

    // perform seeking in samples (if possible)
    // return -1 if failed, or 0 on success
    // if -1 is returned, that will mean that streamer must skip that song
    int (*seek_sample) (DB_fileinfo_t *info, int sample);

    // 'insert' is called to insert new item to playlist
    // decoder is responsible to calculate duration, split it into subsongs, load cuesheet, etc
    // after==NULL means "prepend before 1st item in playlist"
    DB_playItem_t * (*insert) (DB_playItem_t *after, const char *fname); 

    int (*numvoices) (DB_fileinfo_t *info);
    void (*mutevoice) (DB_fileinfo_t *info, int voice, int mute);

    int (*read_metadata) (DB_playItem_t *it);
    int (*write_metadata) (DB_playItem_t *it);

    // NULL terminated array of all supported extensions
    const char **exts;

    // NULL terminated array of all supported prefixes (UADE support needs that)
    // e.g. "mod.song_title"
    const char **prefixes;

    // NULL terminated array of all file type names
    const char **filetypes;
} DB_decoder_t;

// output plugin
typedef struct DB_output_s {
    DB_plugin_t plugin;
    // init is called once at plugin activation
    int (*init) (void);
    // free is called if output plugin was changed to another, or unload is about to happen
    int (*free) (void);
    // reconfigure output to another format
    int (*setformat) (ddb_waveformat_t *fmt);
    // play, stop, pause, unpause are called by deadbeef in response to user
    // events, or as part of streaming process
    int (*play) (void);
    int (*stop) (void);
    int (*pause) (void);
    int (*unpause) (void);
    // one of output_state_t enum values
    int (*state) (void);
    // soundcard enumeration (can be NULL)
    void (*enum_soundcards) (void (*callback)(const char *name, const char *desc, void*), void *userdata);

    // parameters of current output
    ddb_waveformat_t fmt;

    // set to 1 if volume control is done internally by plugin
    int has_volume;
} DB_output_t;

// dsp plugin
// see also: examples/dsp_template.c in git
#define DDB_INIT_DSP_CONTEXT(var,type,plug) {\
    memset(var,0,sizeof(type));\
    var->ctx.plugin=plug;\
    var->ctx.enabled=1;\
}

typedef struct ddb_dsp_context_s {
    // pointer to DSP plugin which created this context
    struct DB_dsp_s *plugin;

    // pointer to the next DSP plugin context in the chain
    struct ddb_dsp_context_s *next;

    // read only flag; set by DB_dsp_t::enable
    unsigned enabled : 1;
} ddb_dsp_context_t;

typedef struct DB_dsp_s {
    DB_plugin_t plugin;

    ddb_dsp_context_t* (*open) (void);

    void (*close) (ddb_dsp_context_t *ctx);

    // samples are always interleaved floating point
    // returned value is number of output frames (multichannel samples)
    // plugins are allowed to modify channels, samplerate, channelmask in the fmt structure
    // buffer size can fit up to maxframes frames
    // by default ratio=1, and plugins don't need to touch it unless they have to
    int (*process) (ddb_dsp_context_t *ctx, float *samples, int frames, int maxframes, ddb_waveformat_t *fmt, float *ratio);

    void (*reset) (ddb_dsp_context_t *ctx);

    // num_params can be NULL, to indicate that plugin doesn't expose any params
    //
    // if num_params is non-NULL -- get_param_name, set_param and get_param must
    // all be implemented
    //
    // param names are for display-only, and are allowed to contain spaces
    int (*num_params) (void);
    const char *(*get_param_name) (int p);
    void (*set_param) (ddb_dsp_context_t *ctx, int p, const char *val);
    void (*get_param) (ddb_dsp_context_t *ctx, int p, char *str, int len);

    // config dialog implementation uses set/get param, so they must be
    // implemented if this is nonzero
    const char *configdialog;
} DB_dsp_t;

// misc plugin
// purpose is to provide extra services
// e.g. scrobbling, converting, tagging, custom gui, etc.
// misc plugins should be mostly event driven, so no special entry points in them
typedef struct {
    DB_plugin_t plugin;
} DB_misc_t;

// vfs plugin
// provides means for reading, seeking, etc
// api is based on stdio
typedef struct DB_vfs_s {
    DB_plugin_t plugin;

// capabilities
    const char **(*get_schemes) (void); // NULL-terminated list of supported schemes, e.g. {"http://", "ftp://", NULL}; can be NULL

    int (*is_streaming) (void); // return 1 if the plugin streaming data over slow connection, e.g. http; plugins will avoid scanning entire files if this is the case

    int (*is_container) (const char *fname); // should return 1 if this plugin can parse specified file

// this is an evil hack to interrupt frozen vfs_curl streams
// FIXME: pass it through command API
    void (*abort) (DB_FILE *stream);

// file access, follows stdio API with few extension
    DB_FILE* (*open) (const char *fname);
    void (*close) (DB_FILE *f);
    size_t (*read) (void *ptr, size_t size, size_t nmemb, DB_FILE *stream);
    int (*seek) (DB_FILE *stream, int64_t offset, int whence);
    int64_t (*tell) (DB_FILE *stream);
    void (*rewind) (DB_FILE *stream);
    int64_t (*getlength) (DB_FILE *stream);

    // should return mime-type of a stream, if known; can be NULL
    const char * (*get_content_type) (DB_FILE *stream);

    // associates stream with a track, to allow dynamic metadata updating, like
    // in icy protocol
    void (*set_track) (DB_FILE *f, DB_playItem_t *it);

// folder access, follows dirent API, and uses dirent data structures
    int (*scandir) (const char *dir, struct dirent ***namelist, int (*selector) (const struct dirent *), int (*cmp) (const struct dirent **, const struct dirent **));
} DB_vfs_t;

// gui plugin
// only one gui plugin can be running at the same time
// should provide GUI services to other plugins

// this structure represents a gui dialog with callbacks to set/get params
// documentation should be available here:
// https://sourceforge.net/apps/mediawiki/deadbeef/index.php?title=Development:Gui_Script
typedef struct {
    const char *title;
    const char *layout;
    void (*set_param) (const char *key, const char *value);
    void (*get_param) (const char *key, char *value, int len, const char *def);
} ddb_dialog_t;

enum {
    ddb_button_ok,
    ddb_button_cancel,
    ddb_button_close,
    ddb_button_apply,
    ddb_button_yes,
    ddb_button_no,
    ddb_button_max,
};

typedef struct DB_gui_s {
    DB_plugin_t plugin;

    // returns response code (ddb_button_*)
    // buttons is a bitset, e.g. (1<<ddb_button_ok)|(1<<ddb_button_cancel)
    int (*run_dialog) (ddb_dialog_t *dlg, uint32_t buttons, int (*callback)(int button, void *ctx), void *ctx);
} DB_gui_t;

// playlist plugin
typedef struct DB_playlist_s {
    DB_plugin_t plugin;

    DB_playItem_t * (*load) (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

    DB_playItem_t * (*save) (DB_playItem_t *first, DB_playItem_t *last, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

    const char **extensions; // NULL-terminated list of supported file extensions, e.g. {"m3u", "pls", NULL}
} DB_playlist_t;

#ifdef __cplusplus
}
#endif

#endif // __DEADBEEF_H
