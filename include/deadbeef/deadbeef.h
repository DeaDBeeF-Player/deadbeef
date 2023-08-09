/*
  deadbeef.h -- plugin API of the DeaDBeeF audio player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009-2022 Oleksiy Yakovenko

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


#ifndef __DEADBEEF_H
#define __DEADBEEF_H

#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// every plugin must define the following entry-point:
// extern "C" DB_plugin_t* $MODULENAME_load (DB_functions_t *api);
// where $MODULENAME is a name of module
// e.g. if your plugin is called "myplugin.so", $MODULENAME is "myplugin"
// this function should return pointer to DB_plugin_t structure
// that is enough for both static and dynamic modules

// backwards compatibility is supported since API version 1.0
// that means that the plugins which use the API 1.0 will work without recompiling until API 2.0.
//
// increments in the major version number mean that there are API breaks, and
// plugins must be recompiled to be compatible.
//
// add DDB_REQUIRE_API_VERSION(x,y) macro when you define the plugin structure
// like this:
// static DB_decoder_t plugin = {
//   DDB_REQUIRE_API_VERSION(1,0)
//  ............
// }
// this is required for versioning
// if you don't do it -- no version checking will be done (useful for debugging/development)
//
// please DON'T release plugins without version requirement
//
// to ensure compatibility, use the following before including deadbeef.h:
// #define DDB_API_LEVEL x
// where x is the minor API version number.
// that way, you'll get errors or warnings when using incompatible stuff.
//
// if you also want to get the deprecation warnings, use the following:
// #define DDB_WARN_DEPRECATED 1
//
// NOTE: deprecation doesn't mean the API is going to be removed, it just means
// that there's a better replacement in the newer deadbeef versions.

// API version history:
// 1.17 -- deadbeef-1.9.6
// 1.16 -- deadbeef-1.9.4
// 1.15 -- deadbeef-1.9.0
// 1.14 -- deadbeef-1.8.8
// 1.12 -- deadbeef-1.8.4
// 1.11 -- deadbeef-1.8.3
// 1.10 -- deadbeef-1.8.0
// 1.9 -- deadbeef-0.7.2
// 1.8 -- deadbeef-0.7.0
// 1.7 -- deadbeef-0.6.2
// 1.6 -- deadbeef-0.6.1
// 1.5 -- deadbeef-0.6
// 1.4 -- deadbeef-0.5.5
// 1.3 -- deadbeef-0.5.3
// 1.2 -- deadbeef-0.5.2
// 1.1 -- deadbeef-0.5.1
//   adds pass_through method to dsp plugins for optimization purposes
// 1.0 -- deadbeef-0.5.0
// 0.10 -- deadbeef-0.4.4-portable-r1 (note: 0.4.4 uses api v0.9)
// 0.9 -- deadbeef-0.4.3-portable-build3
// 0.8 -- deadbeef-0.4.2
// 0.7 -- deabdeef-0.4.0
// 0.6 -- deadbeef-0.3.3
// 0.5 -- deadbeef-0.3.2
// 0.4 -- deadbeef-0.3.0
// 0.3 -- deadbeef-0.2.3.2
// 0.2 -- deadbeef-0.2.3
// 0.1 -- deadbeef-0.2.0

#define DB_API_VERSION_MAJOR 1
#define DB_API_VERSION_MINOR 17

#if defined(__clang__)

#   define DDB_DEPRECATED(x) __attribute__ ((deprecated(x)))

#elif defined(__GNUC__)

    #if !defined(__GNUC_PREREQ)
        // avoid including glibc headers, this is not very portable
        #if defined __GNUC_MINOR__
        #   define __GNUC_PREREQ(maj, min) \
            ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
        #else
        #   define __GNUC_PREREQ(maj, min) 0
        #endif

    #endif

    // Deprecating of enum values requires GCC 6+.
    // Older GCC can still be used to build.
    #if __GNUC_PREREQ(6,0)
    #   define DDB_DEPRECATED(x) __attribute__ ((deprecated(x)))
    #else
    #   define DDB_DEPRECATED(x)
    #endif

#else

    #define DDB_DEPRECATED(x)

#endif

#ifndef DDB_API_LEVEL
#define DDB_API_LEVEL DB_API_VERSION_MINOR
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 17)
#define DEPRECATED_117 DDB_DEPRECATED("since deadbeef API 1.17")
#else
#define DEPRECATED_117
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 16)
#define DEPRECATED_116 DDB_DEPRECATED("since deadbeef API 1.16")
#else
#define DEPRECATED_116
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 15)
#define DEPRECATED_115 DDB_DEPRECATED("since deadbeef API 1.15")
#else
#define DEPRECATED_115
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 14)
#define DEPRECATED_114 DDB_DEPRECATED("since deadbeef API 1.14")
#else
#define DEPRECATED_114
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 13)
#define DEPRECATED_113 DDB_DEPRECATED("since deadbeef API 1.13")
#else
#define DEPRECATED_113
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 12)
#define DEPRECATED_112 DDB_DEPRECATED("since deadbeef API 1.12")
#else
#define DEPRECATED_112
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 11)
#define DEPRECATED_111 DDB_DEPRECATED("since deadbeef API 1.11")
#else
#define DEPRECATED_111
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 10)
#define DEPRECATED_110 DDB_DEPRECATED("since deadbeef API 1.10")
#else
#define DEPRECATED_110
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 9)
#define DEPRECATED_19 DDB_DEPRECATED("since deadbeef API 1.9")
#else
#define DEPRECATED_19
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 8)
#define DEPRECATED_18 DDB_DEPRECATED("since deadbeef API 1.8")
#else
#define DEPRECATED_18
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 7)
#define DEPRECATED_17 DDB_DEPRECATED("since deadbeef API 1.7")
#else
#define DEPRECATED_17
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 6)
#define DEPRECATED_16 DDB_DEPRECATED("since deadbeef API 1.6")
#else
#define DEPRECATED_16
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 5)
#define DEPRECATED_15 DDB_DEPRECATED("since deadbeef API 1.5")
#else
#define DEPRECATED_15
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 4)
#define DEPRECATED_14 DDB_DEPRECATED("since deadbeef API 1.4")
#else
#define DEPRECATED_14
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 3)
#define DEPRECATED_13 DDB_DEPRECATED("since deadbeef API 1.3")
#else
#define DEPRECATED_13
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 2)
#define DEPRECATED_12 DDB_DEPRECATED("since deadbeef API 1.2")
#else
#define DEPRECATED_12
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 1)
#define DEPRECATED_11 DDB_DEPRECATED("since deadbeef API 1.1")
#else
#define DEPRECATED_11
#endif

#if (DDB_WARN_DEPRECATED && DDB_API_LEVEL >= 0)
#define DEPRECATED DDB_DEPRECATED("since deadbeef API 1.0")
#else
#define DEPRECATED
#endif

#define DDB_PLUGIN_SET_API_VERSION\
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,\
    .plugin.api_vminor = DB_API_VERSION_MINOR,

// backwards compat macro
#define DB_PLUGIN_SET_API_VERSION DDB_PLUGIN_SET_API_VERSION

#define PLUG_TEST_COMPAT(plug,x,y) ((plug)->version_major == (x) && (plug)->version_minor >= (y))

#define DDB_REQUIRE_API_VERSION(x,y)\
    .plugin.api_vmajor = x,\
    .plugin.api_vminor = y,

////////////////////////////
// default values for some common config variables should go here

// network.ctmapping : content-type to plugin mapping
#define DDB_DEFAULT_CTMAPPING "audio/mpeg {stdmpg ffmpeg} audio/x-mpeg {stdmpg ffmpeg} application/ogg {stdogg opus ffmpeg} audio/ogg {stdogg opus ffmpeg} audio/aac {aac ffmpeg} audio/aacp {aac ffmpeg} audio/x-m4a {aac ffmpeg} audio/wma {wma ffmpeg}"

////////////////////////////
// playlist structures

// that's a good candidate for redesign
// short explanation: PL_MAIN and PL_SEARCH are used as "iter" argument in
// playlist functions, to reference main or search playlist, respectively
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
    DDB_TAG_ITUNES = (1<<16),

    DDB_TAG_MASK = 0x000fff00
};

// playlist item
// these are "public" fields, available to plugins
typedef struct DB_playItem_s {
    // NOTE: the startsample and endsample fields are 32 bit, and are kept for
    // compatibility. Please use pl_item_get_startsample and friends instead.
    int32_t startsample DEPRECATED_110;
    int32_t endsample DEPRECATED_110;
    int32_t shufflerating; // sort order for shuffle mode
} ddb_playItem_t;

typedef ddb_playItem_t DB_playItem_t;

typedef struct {
    char unused; // to shut up C++ warning
} ddb_playlist_t;

typedef struct DB_metaInfo_s {
    struct DB_metaInfo_s *next;
    const char *key;
    const char *value;

#if (DDB_API_LEVEL >= 10)
    int valuesize;
#endif
} DB_metaInfo_t;

/// These flags should be used with `junk_rewrite_tags`
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
    DB_PLUGIN_GUI = 7,
#if (DDB_API_LEVEL >= 15)
    DB_PLUGIN_MEDIASOURCE = 8,
#endif
};

// output plugin states
enum output_state_t {
    OUTPUT_STATE_STOPPED DEPRECATED_111 = 0,
    OUTPUT_STATE_PLAYING DEPRECATED_111 = 1,
    OUTPUT_STATE_PAUSED DEPRECATED_111 = 2,
};

#if (DDB_API_LEVEL >= 11)

typedef enum ddb_playback_state_e {
    DDB_PLAYBACK_STATE_STOPPED = 0,
    DDB_PLAYBACK_STATE_PLAYING = 1,
    DDB_PLAYBACK_STATE_PAUSED = 2,
} ddb_playback_state_t;

#endif

// playback order
enum playback_order_t {
    PLAYBACK_ORDER_LINEAR DEPRECATED_111 = 0,
    PLAYBACK_ORDER_SHUFFLE_TRACKS DEPRECATED_111 = 1,
    PLAYBACK_ORDER_RANDOM DEPRECATED_111 = 2,
    PLAYBACK_ORDER_SHUFFLE_ALBUMS DEPRECATED_111 = 3,
};

// playback modes
enum playback_mode_t {
    PLAYBACK_MODE_LOOP_ALL DEPRECATED_111 = 0, // loop playlist
    PLAYBACK_MODE_NOLOOP DEPRECATED_111 = 1, // don't loop
    PLAYBACK_MODE_LOOP_SINGLE DEPRECATED_111 = 2, // loop single track
};

#if (DDB_API_LEVEL >= 11)

typedef enum ddb_shuffle_e {
    DDB_SHUFFLE_OFF = 0,
    DDB_SHUFFLE_TRACKS = 1,
    DDB_SHUFFLE_RANDOM = 2,
    DDB_SHUFFLE_ALBUMS = 3,
} ddb_shuffle_t;

typedef enum ddb_repeat_e {
    DDB_REPEAT_ALL = 0,
    DDB_REPEAT_OFF = 1,
    DDB_REPEAT_SINGLE = 2,
} ddb_repeat_t;

#endif

#if (DDB_API_LEVEL >= 8)
// playlist change info, used in the DB_EV_PLAYLISTCHANGED p1 argument
// NOTE: these events can only be sent individually, and can't be ORed.
enum ddb_playlist_change_t {
    DDB_PLAYLIST_CHANGE_CONTENT, // this is the most generic one, will work for the cases when p1 was omitted (0)
    DDB_PLAYLIST_CHANGE_CREATED,
    DDB_PLAYLIST_CHANGE_DELETED,
    DDB_PLAYLIST_CHANGE_POSITION,
    DDB_PLAYLIST_CHANGE_TITLE,
    // When handling DDB_PLAYLIST_CHANGE_SELECTION,
    // `ctx` is assumed to be a unique ID of the event sender,
    // for example a UI view pointer which caused the selection change,
    // but it should not be expected to point to a specific type.
    // This is used to filter the events when they hit the same
    // view which sent them.
    DDB_PLAYLIST_CHANGE_SELECTION,
    DDB_PLAYLIST_CHANGE_SEARCHRESULT,
    DDB_PLAYLIST_CHANGE_PLAYQUEUE,
};
#endif

typedef struct {
    int event;
    int size;
} ddb_event_t;

typedef struct {
    ddb_event_t ev;
    DB_playItem_t *track;
    float playtime; // for SONGFINISHED event -- for how many seconds track was playing
    time_t started_timestamp; // time when "track" started playing
} ddb_event_track_t;

typedef struct {
    ddb_event_t ev;
    DB_playItem_t *from;
    DB_playItem_t *to;
    float playtime; // for SONGCHANGED event -- for how many seconds prev track was playing
    time_t started_timestamp; // time when "from" started playing
} ddb_event_trackchange_t;

typedef struct {
    ddb_event_t ev;
    int state;
} ddb_event_state_t;

typedef struct {
    ddb_event_t ev;
    DB_playItem_t *track;
    float playpos;
} ddb_event_playpos_t;

typedef struct DB_conf_item_s {
    char *key;
    char *value;
    struct DB_conf_item_s *next;
} DB_conf_item_t;

// event callback type
typedef int (*DB_callback_t)(ddb_event_t *, uintptr_t data);

// events
enum {
    DB_EV_NEXT = 1, // switch to next track
    DB_EV_PREV = 2, // switch to prev track
    DB_EV_PLAY_CURRENT = 3, // play current track (will start/unpause if stopped or paused)
    DB_EV_PLAY_NUM = 4, // play track nr. p1
    DB_EV_STOP = 5, // stop current track
    DB_EV_PAUSE = 6, // pause playback
    DB_EV_PLAY_RANDOM = 7, // play random track
    DB_EV_TERMINATE = 8, // must be sent to player thread to terminate
    DB_EV_PLAYLIST_REFRESH DEPRECATED_18 = 9, // [use DB_EV_PLAYLISTCHANGED instead]
    DB_EV_REINIT_SOUND = 10, // reinitialize sound output with current output_plugin config value
    DB_EV_CONFIGCHANGED = 11, // one or more config options were changed
    DB_EV_TOGGLE_PAUSE = 12,
    DB_EV_ACTIVATED = 13, // will be fired every time player is activated
    DB_EV_PAUSED = 14, // player was paused (p1=1) or unpaused (p1=0)

    DB_EV_PLAYLISTCHANGED = 15, // playlist contents were changed (e.g. metadata in any track)
    // DB_EV_PLAYLISTCHANGED NOTE: it's usually sent on LARGE changes,
    // when multiple tracks are affected, while for single tracks
    // the DB_EV_TRACKINFOCHANGED is preferred
    // added in API level 8:
    // p1 is one of ddb_playlist_change_t enum values, detailing what exactly has been changed.

    DB_EV_VOLUMECHANGED = 16, // volume was changed
    DB_EV_OUTPUTCHANGED = 17, // sound output plugin changed
    DB_EV_PLAYLISTSWITCHED = 18, // playlist switch occurred
    DB_EV_SEEK = 19, // seek current track to position p1 (ms)
    DB_EV_ACTIONSCHANGED = 20, // plugin actions were changed, e.g. for reinitializing gui
    DB_EV_DSPCHAINCHANGED = 21, // emitted when any parameter of the main dsp chain has been changed

    // since 1.5
#if (DDB_API_LEVEL >= 5)
    // DB_EV_SELCHANGED is obsolete and isn't emitted; DB_EV_PLAYLISTCHANGED with DDB_PLAYLIST_CHANGE_SELECTION should be used instead.
    DB_EV_SELCHANGED = 22,
    DB_EV_PLUGINSLOADED = 23, // after all plugins have been loaded and connected
#endif

#if (DDB_API_LEVEL >= 8)
    // A caller sends this event, to ask playlist viewer(s) to focus on selected track.
    DB_EV_FOCUS_SELECTION = 24, 
#endif

#if (DDB_API_LEVEL >= 17)
    // Notify about playback state change,
    // which includes a switch to another output plugin.
    // p1 contains the new state.
    DB_EV_PLAYBACK_STATE_DID_CHANGE = 25,
#endif

    // -----------------
    // structured events

    DB_EV_FIRST = 1000, // this is not an event id by itself, but used for checking which events are structured (>=DB_EV_FIRST)

    DB_EV_SONGCHANGED = 1000, // current song changed from one to another, ctx=ddb_event_trackchange_t
    DB_EV_SONGSTARTED = 1001, // song started playing, ctx=ddb_event_track_t
    DB_EV_SONGFINISHED = 1002, // song finished playing, ctx=ddb_event_track_t

    DB_EV_TRACKINFOCHANGED = 1004, // trackinfo was changed (included medatata, playback status, playqueue state, etc), ctx=ddb_event_track_t
    // DB_EV_TRACKINFOCHANGED NOTE: when multiple tracks change, DB_EV_PLAYLISTCHANGED may be sent instead,
    // for speed reasons, so always handle both events.

    DB_EV_SEEKED = 1005, // seek happened, ctx=ddb_event_playpos_t

    // since 1.5
#if (DDB_API_LEVEL >= 5)
    // NOTE: this is not a structured event, but too late to fix, needs to stay here for backwards compat
    DB_EV_TRACKFOCUSCURRENT = 1006, // user wants to highlight/find the current playing track
#endif

#if (DDB_API_LEVEL >= 10)
    DB_EV_CURSOR_MOVED = 1007, // used for syncing cursor position between playlist views, p1 = PL_MAIN or PL_SEARCH, ctx is a ddb_event_track_t, containing the new track under cursor
#endif

    DB_EV_MAX
};

// preset columns, working using IDs
// DON'T add new ids in range 2-7, they are reserved for backwards compatibility
enum pl_column_t {
#if (DDB_API_LEVEL >= 10)
    DB_COLUMN_STANDARD = -1,
#endif
    DB_COLUMN_FILENUMBER = 0,
    DB_COLUMN_PLAYING = 1,
    DB_COLUMN_ALBUM_ART = 8,
#if (DDB_API_LEVEL >= 10)
    DB_COLUMN_CUSTOM = 9
#endif
};

// replaygain constants
enum {
    DDB_REPLAYGAIN_ALBUMGAIN,
    DDB_REPLAYGAIN_ALBUMPEAK,
    DDB_REPLAYGAIN_TRACKGAIN,
    DDB_REPLAYGAIN_TRACKPEAK,
};

#if (DDB_API_LEVEL >= 10)
#if (DDB_API_LEVEL >= 11)
typedef enum ddb_rg_source_mode_e {
#else
enum {
#endif

    DDB_RG_SOURCE_MODE_PLAYBACK_ORDER = 0,
    DDB_RG_SOURCE_MODE_TRACK = 1,
    DDB_RG_SOURCE_MODE_ALBUM = 2,

#if (DDB_API_LEVEL >= 11)
} ddb_rg_source_mode_t;
#else
};
#endif

#if (DDB_API_LEVEL >= 11)
typedef enum ddb_rg_processing_e {
#else
    enum {
#endif

    DDB_RG_PROCESSING_NONE = 0,
    DDB_RG_PROCESSING_GAIN = 1,
    DDB_RG_PROCESSING_PREVENT_CLIPPING = 2,

#if (DDB_API_LEVEL >= 11)
} ddb_rg_processing_t;
#else
};
#endif

typedef struct {
    int _size;
    int source_mode;
    uint32_t processing_flags;
    float preamp_with_rg;
    float preamp_without_rg;
    int has_album_gain;
    int has_track_gain;
    float albumgain;
    float albumpeak;
    float trackgain;
    float trackpeak;
} ddb_replaygain_settings_t;
#endif

// sort order constants
enum ddb_sort_order_t {
    DDB_SORT_DESCENDING,
    DDB_SORT_ASCENDING,
// since 1.3
#if (DDB_API_LEVEL >= 3)
    DDB_SORT_RANDOM,
#endif
};

enum ddb_sys_directory_t {
    DDB_SYS_DIR_CONFIG = 1,
    DDB_SYS_DIR_PREFIX = 2,
    DDB_SYS_DIR_DOC = 3,
    DDB_SYS_DIR_PLUGIN = 4,
    DDB_SYS_DIR_PIXMAP = 5,
    DDB_SYS_DIR_CACHE = 6,
#if (DDB_API_LEVEL >= 13)
    DDB_SYS_DIR_PLUGIN_RESOURCES = 7,
#endif
};

// typecasting macros
#define DB_PLUGIN(x) ((DB_plugin_t *)(x))
#define DB_CALLBACK(x) ((DB_callback_t)(x))
#define DB_EVENT(x) ((ddb_event_t *)(x))
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
    int channels;
    int samplerate;
    uint32_t channelmask;
    int is_float; // bps must be 32 if this is true
#if (DDB_API_LEVEL >= 17)
    uint32_t flags;
#else
    int is_bigendian;
#endif
} ddb_waveformat_t;

#if (DDB_API_LEVEL >= 17)
enum {
    DDB_WAVEFORMAT_FLAG_IS_DOP = 0x01,
};
#endif

// since 1.5
#if (DDB_API_LEVEL >= 5)

/// NOTE: The @c DDB_FREQ_BANDS and the related @c vis_spectrum_listen is no longer used / supported.
/// They do not do anything, just allowing to build old code.
/// Use the nframes field instead. Your code should adapt to any
/// number of bands produced by the visualization engine.
static const int DDB_FREQ_BANDS DEPRECATED_115 = 256;

/// Max number of channels allowed in the ddb_audio_data_t for frequency data
static const int DDB_FREQ_MAX_CHANNELS = 9;

typedef struct ddb_audio_data_s {
    ddb_waveformat_t *fmt;
    float *data;
    int nframes;
} ddb_audio_data_t;

typedef struct ddb_fileadd_data_s {
    int visibility;
    ddb_playlist_t *plt;
    ddb_playItem_t *track;
} ddb_fileadd_data_t;
#endif

// since 1.8
#if (DDB_API_LEVEL >= 8)
enum {
    DDB_TF_CONTEXT_HAS_INDEX = 1,
    DDB_TF_CONTEXT_HAS_ID = 2,
    DDB_TF_CONTEXT_NO_DYNAMIC = 4, // skip dynamic fields (%playback_time%)
// since 1.9
#if (DDB_API_LEVEL >= 9)
    // By default, non-printable characters will be replaced with underscores.
    // In multiline mode, they stay as they are in the input.
    DDB_TF_CONTEXT_MULTILINE = 8,
#endif
// since 1.10
#if (DDB_API_LEVEL >= 10)
    // the caller supports text dimming functions
    DDB_TF_CONTEXT_TEXT_DIM = 16,
#endif
    // since 1.13
#if (DDB_API_LEVEL >= 13)
    // the caller guarantees that metadata access is thread safe
    DDB_TF_CONTEXT_NO_MUTEX_LOCK = 32,
#endif
};

// since 1.10
#if (DDB_API_LEVEL >= 10)
enum {
    DDB_TF_ESC_DIM = 1,
#if DDB_API_LEVEL >= 14
    DDB_TF_ESC_RGB = 2,
#endif
};
#endif

// since 1.10
#if (DDB_API_LEVEL >= 10)
typedef struct ddb_file_found_data_s {
    ddb_playlist_t *plt;
    const char *filename;
    int is_dir;
} ddb_file_found_data_t;
#endif

// context for title formatting interpreter
typedef struct ddb_tf_context_s {
    int _size; // must be set to sizeof(tf_context_t)
    uint32_t flags; // DDB_TF_CONTEXT_ flags
    ddb_playItem_t *it; // track to get information from, or NULL
    ddb_playlist_t *plt; // playlist in which the track resides, or NULL

    // NOTE: when plt is NULL, it means that the track is not in any playlist,
    // that is -- playlist will never be automatically guessed, for performance
    // reasons.

    // index of the track in playlist the track belongs to
    // if present, DDB_TF_CONTEXT_HAS_INDEX flag must be set
    int idx;

    // predefined column id, one of the DB_COLUMN_
    // if present, DDB_TF_CONTEXT_HAS_ID flag must be set
    int id;

    int iter; // playlist iteration (PL_MAIN, PL_SEARCH)

    // update is a returned value
    // meaning:
    // 0: no automatic updates
    // <0: updates on every call
    // >0: number of milliseconds between updates / until next update
    int update;

#if (DDB_API_LEVEL >= 10)
    // Return value, is set to non-zero if text was <<<dimmed>>> or >>>brightened<<<
    // It's used to determine whether the text needs to be searched for the corresponding esc sequences
    int dimmed;
#endif
#if (DDB_API_LEVEL >= 17)
    void (*metadata_transformer)(struct ddb_tf_context_s *ctx, char *data, size_t size);
#endif
} ddb_tf_context_t;
#endif

#if (DDB_API_LEVEL>=10)
enum {
    // Layer 0 means it's always on, and important.
    // This layer is suitable for critical error messages,
    // but it's also useful for plugin-specific messages,
    // which can be turned on and off in the settings.
    // It is expected that the UI plugins will auto-show the Log View,
    // when any message is logged on this layer
    DDB_LOG_LAYER_DEFAULT = 0,

    // Layer 1 should contain informational non-critical messages, like boot log.
    // This layer is always on.
    // UI should not auto-show the Log View for this layer.
    DDB_LOG_LAYER_INFO = 1,
};
#endif

#if (DDB_API_LEVEL>=15)
typedef enum {
    DDB_INSERT_FILE_RESULT_SUCCESS = 0,
    DDB_INSERT_FILE_RESULT_UNRECOGNIZED_FILE = 1, // File was not unrecognized
    DDB_INSERT_FILE_RESULT_RECOGNIZED_FAILED = 2, // File extension was recognized, but could not be loaded by decoder
    DDB_INSERT_FILE_RESULT_RELATIVE_PATH = 3, // File path is relative: unsupported
    DDB_INSERT_FILE_RESULT_NULL_FILENAME = 4, // File name is NULL
    DDB_INSERT_FILE_RESULT_ESCAPE_CHARACTERS_IN_FILENAME = 5, // Escape characters are not allowed in filenames
    DDB_INSERT_FILE_RESULT_NO_FILE_EXTENSION = 6, // File doesn't have an extension
    DDB_INSERT_FILE_RESULT_CUESHEET_ERROR = 7, // Error while loading cuesheet
} ddb_insert_file_result_t;

typedef enum {
    DDB_INSERT_FILE_FLAG_FOLLOW_SYMLINKS = 1<<0,
    DDB_INSERT_FILE_FLAG_ENTER_ARCHIVES = 1<<1,
} ddb_insert_file_flags_t;
#endif

// forward decl for plugin struct
struct DB_plugin_s;

// player api definition
typedef struct {
    // versioning
    int vmajor;
    int vminor;

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
    /// This function is unsafe, and has been deprecated in favor of @c streamer_get_playing_track_safe
    DB_playItem_t *(*streamer_get_playing_track) (void) DEPRECATED_16;
    DB_playItem_t *(*streamer_get_streaming_track) (void);
    float (*streamer_get_playpos) (void);
    int (*streamer_ok_to_read) (int len);
    void (*streamer_reset) (int full);
    int (*streamer_read) (char *bytes, int size);
    void (*streamer_set_bitrate) (int bitrate);
    int (*streamer_get_apx_bitrate) (void);
    struct DB_fileinfo_s *(*streamer_get_current_fileinfo) (void);
    int (*streamer_get_current_playlist) (void);
    struct ddb_dsp_context_s * (*streamer_get_dsp_chain) (void);
    void (*streamer_set_dsp_chain) (struct ddb_dsp_context_s *chain);
    void (*streamer_dsp_refresh) (void); // call after changing parameters

    // system folders
    // normally functions will return standard folders derived from --prefix
    // portable version will return pathes specified in comments below
    // DEPRECATED IN API LEVEL 8, use get_system_dir instead
    const char *(*get_config_dir) (void) DEPRECATED_18; // installdir/config | $XDG_CONFIG_HOME/.config/deadbeef
    const char *(*get_prefix) (void) DEPRECATED_18; // installdir | PREFIX
    const char *(*get_doc_dir) (void) DEPRECATED_18; // installdir/doc | DOCDIR
    const char *(*get_plugin_dir) (void) DEPRECATED_18; // installdir/plugins | LIBDIR/deadbeef
    const char *(*get_pixmap_dir) (void) DEPRECATED_18; // installdir/pixmaps | PREFIX "/share/deadbeef/pixmaps"

    // This function is not implemented, and should not be called. A remnant
    // from old API before 0.5.0.
    void (*do_not_call) (void) DEPRECATED;

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

    /////// playlist management //////
    void (*plt_ref) (ddb_playlist_t *plt);
    void (*plt_unref) (ddb_playlist_t *plt);

    // total number of playlists
    int (*plt_get_count) (void);

    // 1st item in playlist nr. 'plt'
    DB_playItem_t * (*plt_get_head) (int plt) DEPRECATED_113;

    // nr. of selected items in playlist nr. 'plt'
    int (*plt_get_sel_count) (int plt);

    // add new playlist into position before nr. 'before', with title='title'
    // returns index of new playlist
    int (*plt_add) (int before, const char *title);

    // remove playlist nr. plt
    void (*plt_remove) (int plt);

    // clear playlist
    void (*plt_clear) (ddb_playlist_t *plt);
    void (*pl_clear) (void);

    // set current playlist
    void (*plt_set_curr) (ddb_playlist_t *plt);
    void (*plt_set_curr_idx) (int plt);

    // get current playlist
    // note: caller is responsible to call plt_unref after using pointer
    // returned by plt_get_curr
    ddb_playlist_t *(*plt_get_curr) (void);
    int (*plt_get_curr_idx) (void);

    // move playlist nr. 'from' into position before nr. 'before', where
    // before=-1 means last position
    void (*plt_move) (int from, int before);

    // playlist saving and loading
    DB_playItem_t * (*plt_load) (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) DEPRECATED_15;
    int (*plt_save) (ddb_playlist_t *plt, DB_playItem_t *first, DB_playItem_t *last, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

    ddb_playlist_t *(*plt_get_for_idx) (int idx);
    int (*plt_get_title) (ddb_playlist_t *plt, char *buffer, int bufsize);
    int (*plt_set_title) (ddb_playlist_t *plt, const char *title);

    // Increments modification index.
    // This would mark playlist as "dirty" -- meaning it needs to be saved when `pl_save_all` is called.
    // The flag is reset as soon as playlist is saved.
    // This is called automatically when playlists are created / cleared / removed, and when items are added/removed to them.
    // However, other changes -- like changing track metadata -- would not call this function.
    // You need to call it yourself, to make sure the playlist gets saved on exit.
    // It doesn't need to be called if you save the playlist via direct call to `plt_save_*`, or `pl_save_current`
    void (*plt_modified) (ddb_playlist_t *handle);

    // returns modification index
    // the index is incremented by 1 every time playlist changes
    int (*plt_get_modification_idx) (ddb_playlist_t *handle);

    // return index of an item in specified playlist, or -1 if not found
    int (*plt_get_item_idx) (ddb_playlist_t *plt, DB_playItem_t *it, int iter);

    // playlist metadata
    // this kind of metadata is stored in playlist (dbpl) files
    // that is, this is the properties of playlist itself,
    // not of the tracks in the playlist.
    // for example, playlist tab color can be stored there, etc

    // add meta if it doesn't exist yet
    void (*plt_add_meta) (ddb_playlist_t *handle, const char *key, const char *value);

    // replace (or add) existing meta
    void (*plt_replace_meta) (ddb_playlist_t *handle, const char *key, const char *value);

    // append meta to existing one, or add if doesn't exist
    void (*plt_append_meta) (ddb_playlist_t *handle, const char *key, const char *value);

    // set integer meta (works same as replace)
    void (*plt_set_meta_int) (ddb_playlist_t *handle, const char *key, int value);

    // set float meta (works same as replace)
    void (*plt_set_meta_float) (ddb_playlist_t *handle, const char *key, float value);

    // plt_find_meta must always be used in the pl_lock/unlock block
    const char *(*plt_find_meta) (ddb_playlist_t *handle, const char *key);

    // returns head of metadata linked list, for direct access
    // remember pl_lock/unlock
    DB_metaInfo_t * (*plt_get_metadata_head) (ddb_playlist_t *handle);

    // delete meta item from list
    void (*plt_delete_metadata) (ddb_playlist_t *handle, DB_metaInfo_t *meta);

    // returns integer value of requested meta, def is the default value if not found
    int (*plt_find_meta_int) (ddb_playlist_t *handle, const char *key, int def);

    // returns float value of requested meta, def is the default value if not found
    float (*plt_find_meta_float) (ddb_playlist_t *handle, const char *key, float def);

    // delete all metadata
    void (*plt_delete_all_meta) (ddb_playlist_t *handle);

    // operating on playlist items
    DB_playItem_t * (*plt_insert_item) (ddb_playlist_t *playlist, DB_playItem_t *after, DB_playItem_t *it);
    DB_playItem_t * (*plt_insert_file) (ddb_playlist_t *playlist, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) DEPRECATED_15;
    DB_playItem_t *(*plt_insert_dir) (ddb_playlist_t *plt, DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data) DEPRECATED_15;
    void (*plt_set_item_duration) (ddb_playlist_t *plt, DB_playItem_t *it, float duration);
    int (*plt_remove_item) (ddb_playlist_t *playlist, DB_playItem_t *it);
    int (*plt_getselcount) (ddb_playlist_t *playlist);
    float (*plt_get_totaltime) (ddb_playlist_t *plt);
    int (*plt_get_item_count) (ddb_playlist_t *plt, int iter);
    int (*plt_delete_selected) (ddb_playlist_t *plt);
    void (*plt_set_cursor) (ddb_playlist_t *plt, int iter, int cursor);
    int (*plt_get_cursor) (ddb_playlist_t *plt, int iter);
    void (*plt_select_all) (ddb_playlist_t *plt);
    void (*plt_crop_selected) (ddb_playlist_t *plt);
    DB_playItem_t *(*plt_get_first) (ddb_playlist_t *plt, int iter);
    DB_playItem_t *(*plt_get_last) (ddb_playlist_t *plt, int iter);
    DB_playItem_t * (*plt_get_item_for_idx) (ddb_playlist_t *playlist, int idx, int iter);
    void (*plt_move_items) (ddb_playlist_t *to, int iter, ddb_playlist_t *from, DB_playItem_t *drop_before, uint32_t *indexes, int count);
    void (*plt_copy_items) (ddb_playlist_t *to, int iter, ddb_playlist_t * from, DB_playItem_t *before, uint32_t *indices, int cnt);

    // Empty the PL_SEARCH list, and mark the previous results as unselected.
    void (*plt_search_reset) (ddb_playlist_t *plt);

    // Find the specified text in playlist, and populate the PL_SEARCH linked
    // list. The results are also marked as selected.
    void (*plt_search_process) (ddb_playlist_t *plt, const char *text);

    // sort using the title formatting v1 (deprecated)
    void (*plt_sort) (ddb_playlist_t *plt, int iter, int id, const char *format, int order) DEPRECATED_18;

    // add files and folders to current playlist
    int (*plt_add_file) (ddb_playlist_t *plt, const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data) DEPRECATED_15;
    int (*plt_add_dir) (ddb_playlist_t *plt, const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data) DEPRECATED_15;

    // cuesheet support
    DB_playItem_t *(*plt_insert_cue_from_buffer) (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *origin, const uint8_t *buffer, int buffersize, int numsamples, int samplerate) DEPRECATED_110;
    DB_playItem_t * (*plt_insert_cue) (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *origin, int numsamples, int samplerate) DEPRECATED_110;

    // playlist locking
    void (*pl_lock) (void);
    void (*pl_unlock) (void);

    // playlist tracks access
    DB_playItem_t * (*pl_item_alloc) (void);
    DB_playItem_t * (*pl_item_alloc_init) (const char *fname, const char *decoder_id);
    void (*pl_item_ref) (DB_playItem_t *it);
    void (*pl_item_unref) (DB_playItem_t *it);
    void (*pl_item_copy) (DB_playItem_t *out, DB_playItem_t *in);

    // request lock for adding files to playlist
    // this function may return -1 if it is not possible to add files right now.
    // caller must cancel operation in this case,
    // or wait until previous operation finishes
    int (*pl_add_files_begin) (ddb_playlist_t *plt) DEPRECATED_15;

    // release the lock for adding files to playlist
    // end must be called when add files operation is finished
    void (*pl_add_files_end) (void) DEPRECATED_15;

    // most of this functions are self explanatory
    // if you don't get what they do -- look in the code

    // --- the following functions work with current playlist ---

    // get index of the track in MAIN
    int (*pl_get_idx_of) (DB_playItem_t *it);

    // get index of the track in MAIN or SEARCH
    int (*pl_get_idx_of_iter) (DB_playItem_t *it, int iter);

    // get track for index in MAIN
    DB_playItem_t * (*pl_get_for_idx) (int idx);

    // get track for index in MAIN or SEARCH
    DB_playItem_t * (*pl_get_for_idx_and_iter) (int idx, int iter);

    // get total play time of all tracks in MAIN
    float (*pl_get_totaltime) (void);

    // get number of tracks in MAIN or SEARCH
    int (*pl_getcount) (int iter);

    // delete selected tracks
    int (*pl_delete_selected) (void);

    // set cursor position in MAIN or SEARCH
    void (*pl_set_cursor) (int iter, int cursor);

    // get cursor position in MAIN
    int (*pl_get_cursor) (int iter);

    // remove all except selected tracks
    void (*pl_crop_selected) (void);

    // get number of selected tracks
    int (*pl_getselcount) (void);

    // get first track in MAIN or SEARCH
    DB_playItem_t *(*pl_get_first) (int iter);

    // get last track in MAIN or SEARCH
    DB_playItem_t *(*pl_get_last) (int iter);

    // --- misc functions ---

    // mark the track as selected or unselected (1 or 0 respectively)
    void (*pl_set_selected) (DB_playItem_t *it, int sel);

    // test whether the track is selected
    int (*pl_is_selected) (DB_playItem_t *it);

    // save current playlist
    int (*pl_save_current) (void);

    // save all playlists
    // Remember to call `plt_modified` on playlists which need saving.
    // See more information near the `plt_modified` declaration
    int (*pl_save_all) (void);

    // select all tracks in current playlist
    void (*pl_select_all) (void);

    // get next track
    DB_playItem_t *(*pl_get_next) (DB_playItem_t *it, int iter);

    // get previous track
    DB_playItem_t *(*pl_get_prev) (DB_playItem_t *it, int iter);

    /*
       pl_format_title formats the line for display in playlist
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
    int (*pl_format_title) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt) DEPRECATED_18;

    // _escaped version wraps all conversions with '' and replaces every ' in conversions with \'
    int (*pl_format_title_escaped) (DB_playItem_t *it, int idx, char *s, int size, int id, const char *fmt) DEPRECATED_18;

    // format duration 't' (fractional seconds) into string, for display in playlist
    void (*pl_format_time) (float t, char *dur, int size);

    // find which playlist the specified item belongs to, returns NULL if none
    ddb_playlist_t * (*pl_get_playlist) (DB_playItem_t *it);

    // direct access to metadata structures
    // not thread-safe, make sure to wrap with pl_lock/pl_unlock
    DB_metaInfo_t * (*pl_get_metadata_head) (DB_playItem_t *it); // returns head of metadata linked list
    void (*pl_delete_metadata) (DB_playItem_t *it, DB_metaInfo_t *meta);

    // high-level access to metadata
    void (*pl_add_meta) (DB_playItem_t *it, const char *key, const char *value);
    void (*pl_append_meta) (DB_playItem_t *it, const char *key, const char *value);
    void (*pl_set_meta_int) (DB_playItem_t *it, const char *key, int value);
    void (*pl_set_meta_float) (DB_playItem_t *it, const char *key, float value);
    void (*pl_delete_meta) (DB_playItem_t *it, const char *key);

    // this function is not thread-safe
    // make sure to wrap it with pl_lock/pl_unlock block
    const char *(*pl_find_meta) (DB_playItem_t *it, const char *key);

    // following functions are thread-safe
    int (*pl_find_meta_int) (DB_playItem_t *it, const char *key, int def);
    float (*pl_find_meta_float) (DB_playItem_t *it, const char *key, float def);
    void (*pl_replace_meta) (DB_playItem_t *it, const char *key, const char *value);
    void (*pl_delete_all_meta) (DB_playItem_t *it);
    float (*pl_get_item_duration) (DB_playItem_t *it);
    uint32_t (*pl_get_item_flags) (DB_playItem_t *it);
    void (*pl_set_item_flags) (DB_playItem_t *it, uint32_t flags);
    void (*pl_items_copy_junk)(DB_playItem_t *from, DB_playItem_t *first, DB_playItem_t *last);
    // idx is one of DDB_REPLAYGAIN_* constants
    void (*pl_set_item_replaygain) (DB_playItem_t *it, int idx, float value);
    float (*pl_get_item_replaygain) (DB_playItem_t *it, int idx);

    // playqueue support (obsolete since API 1.8)
    int (*pl_playqueue_push) (DB_playItem_t *it) DEPRECATED_18;
    void (*pl_playqueue_clear) (void) DEPRECATED_18;
    void (*pl_playqueue_pop) (void) DEPRECATED_18;
    void (*pl_playqueue_remove) (DB_playItem_t *it) DEPRECATED_18;
    int (*pl_playqueue_test) (DB_playItem_t *it) DEPRECATED_18;

    // volume control
    void (*volume_set_db) (float dB);
    float (*volume_get_db) (void);
    void (*volume_set_amp) (float amp);
    float (*volume_get_amp) (void);
    float (*volume_get_min_db) (void);

    // junk reading/writing
    int (*junk_id3v1_read) (DB_playItem_t *it, DB_FILE *fp);
    int (*junk_id3v1_find) (DB_FILE *fp);
    int (*junk_id3v1_write) (FILE *fp, DB_playItem_t *it, const char *enc);
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

    // Returns an offset to the audio packets, after ID3v2 and APEv2 tags.
    // Only the values >=0 can be returned.
    // The position is relative to the current file offset, at the time of the call.
    int (*junk_get_leading_size) (DB_FILE *fp);
    int (*junk_get_leading_size_stdio) (FILE *fp);

    // This is an API bug that was introduced during 0.7.0 development cycle.
    // The function was accidentally removed from the codebase, so this pointer is always NULL.
    void (*do_not_call2) (DB_playItem_t *, DB_playItem_t *, DB_playItem_t *);

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

    // convenience functions to send events, uses sendmessage internally
    ddb_event_t *(*event_alloc) (uint32_t id);
    void (*event_free) (ddb_event_t *ev);
    int (*event_send) (ddb_event_t *ev, uint32_t p1, uint32_t p2);

    // configuration access
    //
    // conf_get_str_fast is not thread-safe, and
    // must only be used from within conf_lock/conf_unlock block
    // it should be preferred for fast non-blocking lookups
    //
    // all the other config access functions are thread safe
    void (*conf_lock) (void);
    void (*conf_unlock) (void);
    const char * (*conf_get_str_fast) (const char *key, const char *def);
    void (*conf_get_str) (const char *key, const char *def, char *buffer, int buffer_size);
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
    const char **(*plug_get_gui_names) (void);
    const char * (*plug_get_decoder_id) (const char *id);
    void (*plug_remove_decoder_id) (const char *id);
    struct DB_plugin_s *(*plug_get_for_id) (const char *id);

    // misc utilities
    // returns 1 if the track is represented as a local file
    // returns 0 if it's a remote file, e.g. a network stream
    // since API 1.5 it also returns 1 for vfs tracks, e.g. from ZIP files
    int (*is_local_file) (const char *fname);

    // pcm utilities
    int (*pcm_convert) (const ddb_waveformat_t * inputfmt, const char *input, const ddb_waveformat_t *outputfmt, char *output, int inputsize);

    // dsp preset management
    int (*dsp_preset_load) (const char *fname, struct ddb_dsp_context_s **head);
    int (*dsp_preset_save) (const char *fname, struct ddb_dsp_context_s *head);
    void (*dsp_preset_free) (struct ddb_dsp_context_s *head);

    // since 1.2
#if (DDB_API_LEVEL >= 2)
    ddb_playlist_t *(*plt_alloc) (const char *title);
    void (*plt_free) (ddb_playlist_t *plt);

    void (*plt_set_fast_mode) (ddb_playlist_t *plt, int fast);
    int (*plt_is_fast_mode) (ddb_playlist_t *plt);

    const char * (*metacache_add_string) (const char *str);
    void (*metacache_remove_string) (const char *str);

    // These functions are broken, and should not be used.
    // Use metacache_add_string/metacache_remove_string instead.
    void (*metacache_ref) (const char *str) DEPRECATED_113;
    void (*metacache_unref) (const char *str) DEPRECATED_113;

    // this function must return original un-overridden value (ignoring the keys prefixed with '!')
    // it's not thread-safe, and must be used under the same conditions as the
    // pl_find_meta
    const char *(*pl_find_meta_raw) (DB_playItem_t *it, const char *key);
#endif

    // since 1.3
#if (DDB_API_LEVEL >= 3)
    int (*streamer_dsp_chain_save) (void);
#endif

    // since 1.4
#if (DDB_API_LEVEL >= 4)
    int (*pl_get_meta) (DB_playItem_t *it, const char *key, char *val, int size);
    int (*pl_get_meta_raw) (DB_playItem_t *it, const char *key, char *val, int size);
    int (*plt_get_meta) (ddb_playlist_t *handle, const char *key, char *val, int size);

    // fast way to test if a field exists in playitem
    int (*pl_meta_exists) (DB_playItem_t *it, const char *key);
#endif

    // since 1.5
#if (DDB_API_LEVEL >= 5)
    /// Register for getting continuous visualization wave data.
    /// Incoming waveform data can be arbitrary size.
    /// Samples are stored in interleaved layout.
    /// @param ctx Unique pointer identifying your listener.
    void (*vis_waveform_listen) (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data));

    /// Unregister from getting visualization wave data.
    /// @param ctx The pointer used with the matching @c vis_waveform_listen call.
    void (*vis_waveform_unlisten) (void *ctx);

    /// This method does nothing starting with API 1.15.
    /// Please use @c vis_spectrum_listen2 instead.
    void (*vis_spectrum_listen) (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data)) DEPRECATED_115;

    void (*vis_spectrum_unlisten) (void *ctx);

    /// Mute/unmute audio without touching volume control.
    void (*audio_set_mute) (int mute);

    /// @return the value set with @c audio_set_mute
    int (*audio_is_mute) (void);

    // this is useful for prompting a user when he attempts to quit the player
    // while something is working in background, e.g. the Converter,
    // and let him finish or cancel the background jobs.
    void (*background_job_increment) (void);
    void (*background_job_decrement) (void);
    int (*have_background_jobs) (void);

    // utility function to get plt idx from handle
    int (*plt_get_idx) (ddb_playlist_t *plt);

    // save referenced playlist in config
    // same as pl_save_current, but for index
    int (*plt_save_n) (int n);

    // same as pl_save_current, but for playlist pointer
    int (*plt_save_config) (ddb_playlist_t *plt);

    // register file added callback
    // the callback will be called for each file
    // the visibility is taken from plt_add_* arguments
    // the callback must return 0 to continue, or -1 to abort the operation.
    // returns ID
    int (*listen_file_added) (int (*callback)(ddb_fileadd_data_t *data, void *user_data), void *user_data);
    void (*unlisten_file_added) (int id);

    int (*listen_file_add_beginend) (void (*callback_begin) (ddb_fileadd_data_t *data, void *user_data), void (*callback_end)(ddb_fileadd_data_t *data, void *user_data), void *user_data);
    void (*unlisten_file_add_beginend) (int id);

    // visibility is a number, which tells listeners about the caller.
    // the value DDB_FILEADD_VISIBILITY_GUI (or 0) is reserved for callers which
    // want the GUI to intercept the calls and show visual updates.
    //
    // To skip UI, it is recommended to use visibility=-1
    //
    // this is the default value passed from plt_load, plt_add_dir, plt_add_file.
    //
    // the values up to 10 are registered for deadbeef itself, so please avoid
    // using them in your plugins, unless you really know what you're doing.
    // any values above 10 are free for any use.
    //
    // the "callback", if not NULL, will be called with the passed "user_data",
    // for each track.
    //
    // the registered listeners will be called too, the ddb_fileadd_data_t
    // has the visibility
    DB_playItem_t * (*plt_load2) (int visibility, ddb_playlist_t *plt, ddb_playItem_t *after, const char *fname, int *pabort, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data);
    int (*plt_add_file2) (int visibility, ddb_playlist_t *plt, const char *fname, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data);
    int (*plt_add_dir2) (int visibility, ddb_playlist_t *plt, const char *dirname, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data);
    ddb_playItem_t * (*plt_insert_file2) (int visibility, ddb_playlist_t *playlist, ddb_playItem_t *after, const char *fname, int *pabort, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data);
    ddb_playItem_t *(*plt_insert_dir2) (int visibility, ddb_playlist_t *plt, ddb_playItem_t *after, const char *dirname, int *pabort, int (*callback)(DB_playItem_t *it, void *user_data), void *user_data);

    // request lock for adding files to playlist
    // returns 0 on success
    // this function may return -1 if it is not possible to add files right now.
    // caller must cancel operation in this case,
    // or wait until previous operation finishes
    // NOTE: it's not guaranteed that all deadbeef versions support
    // adding the files to different playlists in parallel.
    int (*plt_add_files_begin) (ddb_playlist_t *plt, int visibility);

    // release the lock for adding files to playlist
    // end must be called when add files operation is finished
    void (*plt_add_files_end) (ddb_playlist_t *plt, int visibility);

    // deselect all tracks in playlist
    void (*plt_deselect_all) (ddb_playlist_t *plt);
#endif
    // since 1.6
#if (DDB_API_LEVEL >= 6)
    void (*plt_set_scroll) (ddb_playlist_t *plt, int scroll);
    int (*plt_get_scroll) (ddb_playlist_t *plt);
#endif
    // since 1.8
#if (DDB_API_LEVEL >= 8)
    // **** title formatting v2 ****

    // compile the input title formatting string into bytecode
    // script: freeform string with title formatting special characters in it
    // returns the pointer to compiled bytecode, which must be tf_free'd by the caller.
    char *(*tf_compile) (const char *script);

    // free the code returned by tf_compile
    void (*tf_free) (char *code);

    // Evaluate the compiled titleformatting script in the given context
    // ctx: a pointer to ddb_tf_context_t structure initialized by the caller
    // code: the bytecode data created by tf_compile
    // out: buffer allocated by the caller, must be big enough to fit the output string
    // outlen: the size of out buffer
    // returns -1 on failure, output size on success
    int (*tf_eval) (ddb_tf_context_t *ctx, const char *code, char *out, int outlen);

    // sort using title formatting v2
    void (*plt_sort_v2) (ddb_playlist_t *plt, int iter, int id, const char *format, int order);

    // playqueue APIs
    int (*playqueue_push) (DB_playItem_t *it);
    void (*playqueue_pop) (void);
    void (*playqueue_remove) (DB_playItem_t *it);
    void (*playqueue_clear) (void);
    int (*playqueue_test) (DB_playItem_t *it);
    int (*playqueue_get_count) (void);
    DB_playItem_t *(*playqueue_get_item) (int n);
    int (*playqueue_remove_nth) (int n);
    void (*playqueue_insert_at) (int n, DB_playItem_t *it);

    // system directory API, returns path by id from ddb_sys_directory_t enum
    const char *(*get_system_dir) (int dir_id);

    // set the selected playlist for the ongoing plugin action.
    // the "set" function is expected to be called by the UI plugin,
    // while the "get" is expected to be called by the action code.
    void (*action_set_playlist) (ddb_playlist_t *plt);

    // returns one of:
    // selected playlist for context menu for the DDB_ACTION_CTX_PLAYLIST,
    // or the current active playlist for any other context.
    // returned value cannot be NULL
    // returned value is refcounted, so remember to call plt_unref.
    ddb_playlist_t *(*action_get_playlist) (void);

    // convert legacy title formatting to the new format, usable with tf_compile
    void (*tf_import_legacy) (const char *fmt, char *out, int outsize);
#endif

    // since 1.10
#if (DDB_API_LEVEL >= 10)
    // same as plt_search_process, but allows to choose whether to select the
    // search results, or not
    void (*plt_search_process2) (ddb_playlist_t *plt, const char *text, int select_results);

    // try loading external and embedded cuesheet, using the configured order (cue.prefer_embedded, default=0)
    DB_playItem_t * (*plt_process_cue) (ddb_playlist_t *plt, DB_playItem_t *after, DB_playItem_t *it, uint64_t numsamples, int samplerate);

    // return direct-access metadata structure for the given track and key
    DB_metaInfo_t * (*pl_meta_for_key) (DB_playItem_t *it, const char *key);

    ////////////  Logging  ///////////

    // The recommended usage in plugins:
    // #define trace(...) { deadbeef->log_detailed (&plugin.plugin, 0, __VA_ARGS__); }
    // Then use trace () as you would use printf
    // The user would be able to enable/disable logging in your plugin via the standard UI features.
    // Remember to set plugin.api_vminor = 10 or higher

    // Low level log function, where plugin and level can be specified.
    // Plugin defines the scope, so logging can be toggled per plugin
    // Layers is a combination of bits, which define the priority/visibility of the message.
    // See DDB_LOG_LAYER_* for details
    void (*log_detailed) (struct DB_plugin_s *plugin, uint32_t layers, const char *fmt, ...);

    // Same as log_detailed but uses va_list
    void (*vlog_detailed) (struct DB_plugin_s *plugin, uint32_t layer, const char *fmt, va_list ap);

    // High level easy-to-use log function, with no scope
    // These log messages cannot be disabled, and will always appear in the Log Viewers
    void (*log) (const char *fmt, ...);

    // Same as log but uses va_list
    void (*vlog) (const char *fmt, va_list ap);

    // Custom log viewers, for use in UI plugins and similar
    void (*log_viewer_register) (void (*callback)(struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx), void *ctx);
    void (*log_viewer_unregister) (void (*callback)(struct DB_plugin_s *plugin, uint32_t layers, const char *text, void *ctx), void *ctx);

    ///////// File add filtering ///////

    // It works by calling the filter right after a file or folder was found, but before it's open / processed
    // Then if the filter returns a negative value -- the file/folder will be skipped
    // It's designed to work with plt_insert_dir and plt_insert_file.
    // In case of plt_insert_file, the filter will only be used for recursive plt_insert_dir calls (e.g. for VFS containers)

    // Registers the file add filter, and returns the filter ID, which can be used to unregister the filter
    // Calls the callback before each file is processed
    // The callback must return 0 to continue, or a negative value to skip the file
    int (*register_fileadd_filter) (int (*callback)(ddb_file_found_data_t *data, void *user_data), void *user_data);

    // Unregisters the filter by ID, returned by register_file_filter
    void (*unregister_fileadd_filter) (int id);

    ////// MetaCache APIs available from 1.10+ //////

    // Returns an existing NULL-terminated string, or NULL if it doesn't exist
    const char * (*metacache_get_string) (const char *str);

    // Adds a new value of specified size, or finds an existing one
    const char * (*metacache_add_value) (const char *value, size_t valuesize);

    // Returns an existing value of specified size, or NULL if it doesn't exist
    const char *(*metacache_get_value) (const char *value, size_t valuesize);

    // Removes an existing value of specified size, ignoring refcount
    void (*metacache_remove_value) (const char *value, size_t valuesize);

    ////// ReplayGain APIs available from 1.10+ //////

    // Apply replaygain according to the current settings.
    // NOTE: This only works for the current streaming_track,
    // as the current settings are controlled by the streamer.
    // Can be used to apply replaygain in decoders, but the appropriate flags
    // need to be checked, e.g. the DDB_DECODER_HINT_RAW_SIGNAL.
    // Make sure to set the DDB_PLUGIN_FLAG_REPLAYGAIN bit in plugin.flags,
    // otherwise replaygain could be applied twice.
    void (*replaygain_apply) (ddb_waveformat_t *fmt, char *bytes, int numbytes);

    // Same as replaygain_apply, but with specified settings.
    // Suitable to use from the converter and for other similar uses.
    void (*replaygain_apply_with_settings) (ddb_replaygain_settings_t *settings, ddb_waveformat_t *fmt, char *bytes, int numbytes);

    // Utility function to init the replaygain settings from the current
    // player configuration, and the supplied track.
    // After initializing the settings, pass the pointer to replaygain_apply_with_settings
    void (*replaygain_init_settings) (ddb_replaygain_settings_t *settings, DB_playItem_t *it);

    ////// Sort APIs available from 1.10+ //////

    // Sort a plain array of tracks, according to specified title formatting v2 script and column id.
    // The sorting is applied in-place, i.e. the input array is modified.
    // `playlist` can be NULL, otherwise must point to the playlist which ALL of the tracks belong to;
    // `tracks` and `numtracks` specify the array of tracks;
    // `format` is title formatting v2 script;
    // `order` can be one of DDB_SORT_ASCENDING or DDB_SORT_DESCENDING (no random).
    void (*sort_track_array) (ddb_playlist_t *playlist, DB_playItem_t **tracks, int num_tracks, const char *format, int order);

    // initialize playitem, same as plt_add_file, except do not add to any playlist
    DB_playItem_t *(*pl_item_init) (const char *fname);

    int64_t (*pl_item_get_startsample) (DB_playItem_t *it);

    int64_t (*pl_item_get_endsample) (DB_playItem_t *it);

    void (*pl_item_set_startsample) (DB_playItem_t *it, int64_t sample);

    void (*pl_item_set_endsample) (DB_playItem_t *it, int64_t sample);

    // get total playback time of selected tracks
    float (*plt_get_selection_playback_time) (ddb_playlist_t *plt);

    // get the size of known tags at the end of file, or -1 on error
    int (*junk_get_tail_size) (DB_FILE *fp);

    // get the sizes of known tags at the beginning and end of file
    // no error is reported
    void (*junk_get_tag_offsets) (DB_FILE *fp, uint32_t *head, uint32_t *tail);

    // returns 1 to tell that cuesheet is being loaded now.
    // this should be called by plugins to prevent running cuesheet code at a wrong time.
    int (*plt_is_loading_cue) (ddb_playlist_t *plt);
#endif

// since 1.11
#if (DDB_API_LEVEL >= 11)
    // Set and get shuffle / repeat modes.

    void (*streamer_set_shuffle) (ddb_shuffle_t shuffle);

    ddb_shuffle_t (*streamer_get_shuffle) (void);

    void (*streamer_set_repeat) (ddb_repeat_t repeat);

    ddb_repeat_t (*streamer_get_repeat) (void);
#endif

// since 1.12
#if (DDB_API_LEVEL >= 12)
    DB_metaInfo_t *(*pl_meta_for_key_with_override) (ddb_playItem_t *it, const char *key);
    const char *(*pl_find_meta_with_override) (DB_playItem_t *it, const char *key);
    int (*pl_get_meta_with_override) (ddb_playItem_t *it, const char *key, char *val, size_t size);
    int (*pl_meta_exists_with_override) (DB_playItem_t *it, const char *key);
#endif

// since 1.13
#if (DDB_API_LEVEL >= 13)
    void (*plt_item_set_selected)(ddb_playlist_t *plt, ddb_playItem_t *it, int sel);
    ddb_playlist_t * (*plt_find_by_name) (const char *name);

    /// Append a playlist, and return the pointer
    ///
    /// NOTE: Before API level 15, this function had a bug and didn't increment reference count.
    /// It's not recommended to use it unless API level 15 is available.
    ddb_playlist_t * (*plt_append) (const char *title);
    ddb_playItem_t * (*plt_get_head_item) (ddb_playlist_t *p, int iter);
    ddb_playItem_t * (*plt_get_tail_item) (ddb_playlist_t *p, int iter);
#endif

// since 1.14
#if (DDB_API_LEVEL >= 14)
    // Get full filesystem path to the specified plugin file (.so/.dll/.dylib)
    const char* (*plug_get_path_for_plugin_ptr) (struct DB_plugin_s *plugin_ptr);
#endif

#if (DDB_API_LEVEL >= 15)
    /// Register for getting spectrum (frequency domain) visualization data.
    ///
    /// Use the @c nframes field in the @c data to get the number of frequency samples.
    ///
    /// Max number of channels is @c DDB_FREQ_MAX_CHANNELS.
    ///
    /// The samples are planar-ordered (non-interleaved).
    ///
    /// Use @c vis_spectrum_unlisten to unregister.
    ///
    /// Callback will run on a background thread, so make sure to synchronize the data access.
    /// @param ctx Associated context, must be unique
    /// @param callback The callback which will be called every time new fft data is ready
    void (*vis_spectrum_listen2) (void *ctx, void (*callback)(void *ctx, const ddb_audio_data_t *data));

    /// This method will inserts directory contents at the specified point in playlist.
    /// Improvements from previous revision: flags, and callback with result parameter.
    /// @param visibility See @c plt_load2 summary.
    /// @param flags A combination of flags from @c ddb_insert_file_flags_t enum.
    /// @param plt Playlist to insert files to.
    /// @param after The item to insert new items after. Must exist in the playlist.
    /// @param dirname Directory path.
    /// @param pabort A pointer to an integer, which can be set to 1 to abort the execution. Can be NULL.
    /// @param callback The callback function that will be called for each file. It must return 0 to insert the file, or -1 to skip the file. Can be NULL.
    /// @param user_data A pointer to arbitrary data to pass to the callback.
    /// @return the last inserted item.
    ddb_playItem_t *(*plt_insert_dir3) (
        int visibility,
        uint32_t flags,
        ddb_playlist_t *plt,
        ddb_playItem_t *after,
        const char *dirname,
        int *pabort,
        int (*callback)(ddb_insert_file_result_t result, const char *filename, void *user_data),
        void *user_data
    );
#endif

#if (DDB_API_LEVEL >= 16)
    /// @return The currently playing track
    /// Please ensure that this function is not called from within @c pl_lock,
    /// since this function internally uses streamer_lock, which may cause a deadlock against pl_lock.
    ddb_playItem_t * (*streamer_get_playing_track_safe) (void);
#endif
} DB_functions_t;

// NOTE: an item placement must be selected like this
// if (flags & DB_ACTION_COMMON)  -> main menu, or nowhere, or where GUI plugin wants
//    basically, to put it into main menu, prefix the item title with the menu name
//    e.g. title = "File/MyItem" --> this will add the item under File menu
//
// if (flags & PLAYLIST)  -> playlist (tab) context menu
//
// if (none of the above)  -> track context menu

enum {
    // An menu item for this action should be added to the main menu (ex. Playback/Skip to/Previous genre)
    DB_ACTION_COMMON = 1 << 0,

    // Indicates that this action can work when a single track is selected
    DB_ACTION_SINGLE_TRACK = 1 << 1,

    // Indicates that this action can work when multiple tracks are selected
    DB_ACTION_MULTIPLE_TRACKS = 1 << 2,

    // Use DB_ACTION_MULTIPLE_TRACKS instead
    DB_ACTION_ALLOW_MULTIPLE_TRACKS DEPRECATED_15 = 1 << 2,

    // Ignored in callback2
    // Action will get the track list by itself, instead of getting the list as argument.
    // This is the default behavior when using callback2
    DB_ACTION_CAN_MULTIPLE_TRACKS DEPRECATED_15 = 1 << 3,

    // Action is inactive
    DB_ACTION_DISABLED = 1 << 4,

#if (DDB_API_LEVEL >= 2)
    // Action is compatible with action context is DDB_ACTION_CTX_PLAYLIST,
    // I.e. it should show up in the playlist tab context menu.
    // Example "Duplicate playlist".
    DB_ACTION_PLAYLIST = (1 << 5),
#endif

#if (DDB_API_LEVEL >= 5)
    // Add item to the menus.
    // When constructing the main menu, only add if the action name contains a slash.
    DB_ACTION_ADD_MENU = 1 << 6,
#endif

#if (DDB_API_LEVEL >= 10)
    // Don't allow running this action in playlist context, even if it supports multiple selection
    DB_ACTION_EXCLUDE_FROM_CTX_PLAYLIST = 1 << 7
#endif
};

// action contexts
// since 1.5
#if (DDB_API_LEVEL >= 5)
#if (DDB_API_LEVEL >= 11)
typedef enum ddb_action_context_e {
#else
enum {
#endif
    DDB_ACTION_CTX_MAIN,
    DDB_ACTION_CTX_SELECTION,
    // NOTE: starting with API 1.8, plugins should be using the
    // action_get_playlist function for getting the playlist pointer.
    DDB_ACTION_CTX_PLAYLIST,
    DDB_ACTION_CTX_NOWPLAYING,
    DDB_ACTION_CTX_COUNT
#if (DDB_API_LEVEL >= 11)
} ddb_action_context_t;
#else
};
#endif
#endif

struct DB_plugin_action_s;

typedef int (*DB_plugin_action_callback_t) (struct DB_plugin_action_s *action, void *userdata);

#if (DDB_API_LEVEL >= 11)
typedef int (*DB_plugin_action_callback2_t) (struct DB_plugin_action_s *action, ddb_action_context_t ctx);
#elif (DDB_API_LEVEL >= 5)
typedef int (*DB_plugin_action_callback2_t) (struct DB_plugin_action_s *action, int ctx);
#endif

typedef struct DB_plugin_action_s {
    const char *title;
    const char *name;
    uint32_t flags;
    // Only use it if the code must be compatible with API 1.4,
    // otherwise switch to callback2
    DB_plugin_action_callback_t callback DEPRECATED_14;
    struct DB_plugin_action_s *next;
#if (DDB_API_LEVEL >= 5)
    DB_plugin_action_callback2_t callback2;
#endif
} DB_plugin_action_t;

#if (DDB_API_LEVEL >= 10)
enum {
    // Tells the system to capture the logs from this plugin.
    DDB_PLUGIN_FLAG_LOGGING = 1,

    // Tells the system that the plugin supports replaygain, and streamer should not do it.
    DDB_PLUGIN_FLAG_REPLAYGAIN = 2,

#if (DDB_API_LEVEL >= 14)
    // Tells that the plugin implements ddb_decoder2_t interface
    DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2 = 4,
#endif

#if (DDB_API_LEVEL >= 15)
    DDB_PLUGIN_FLAG_ASYNC_STOP = 8,
#endif
};
#endif

#if (DDB_API_LEVEL >= 15)
/// Reserved command IDs, usable with @c plugin.command method.
/// The commands with numbers 1000 and up are reserved for internal use.
enum {
    // Stop plugin asynchronously.
    // The plugin is expected to handle this command, if it has the flag @c DDB_PLUGIN_FLAG_ASYNC_STOP.
    // The command 2nd argument is a void (^completion_block)(void).
    DDB_COMMAND_PLUGIN_ASYNC_STOP = 1000,
};

typedef struct ddb_response_s {
    size_t _size;
    int (*append)(struct ddb_response_s *response, char *bytes, size_t size);
} ddb_response_t;

#endif

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

    uint32_t flags; // DDB_PLUGIN_FLAG_*
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;

    // any of those can be left NULL
    // though it's much better to fill them with something useful
    const char *id; // id used for serialization and runtime binding
    const char *name; // short name
    const char *descr; // short description (what the plugin is doing)
    const char *copyright; // copyright notice(s), list of developers, links to original works, etc
    const char *website; // plugin website

    // plugin-specific command interface; can be NULL
    int (*command) (int cmd, ...);

    // start is called to start plugin; can be NULL
    int (*start) (void);

    // stop is called to deinit plugin; can be NULL
    int (*stop) (void);

    // connect is called to setup connections between different plugins
    // it is called after all plugin's start method was executed
    // can be NULL
    // NOTE for GUI plugin developers: don't initialize your widgets/windows in
    // the connect method.
    int (*connect) (void);

    // opposite of connect, will be called before stop, while all plugins are still
    // in "started" state
    int (*disconnect) (void);

#if (DDB_API_LEVEL >= 15)
    /// Ask the plugin to execute an arbitrary command line
    ///
    /// @c exec_cmdline may be called at any moment when the user sends commandline to player.
    /// A plugin must have api_vminor>=15 in order to use this API.
    /// The method can be NULL if plugin doesn't support commandline processing.
    /// @param cmdline is 0-separated list of strings, guaranteed to have 0 at the end
    /// @param cmdline_size is number of bytes pointed by cmdline
    /// @param response the interface to create a response for the caller
    /// @return 0 on success or error code on failure
    int (*exec_cmdline) (const char *cmdline, int cmdline_size, ddb_response_t *response);
#else
    /// Don't use this API.
    /// It is kept here to guarantee the API-level backwards compatibility, but this method will never be called.
    int (*exec_cmdline) (const char *cmdline, int cmdline_size);
#endif

    // @return linked list of actions for the specified track
    // when it is NULL -- the plugin must return list of all actions
    DB_plugin_action_t* (*get_actions) (DB_playItem_t *it);

    // mainloop will call this function for every plugin
    // so that plugins may handle all events;
    // can be NULL
    int (*message) (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);

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
    // Decoders should try to output 16 bit stream when this flag is set, for
    // performance reasons.
    DDB_DECODER_HINT_16BIT = 0x1,
#if (DDB_API_LEVEL >= 8)
    // Decoders should only call the streamer_set_bitrate from plugin.read function,
    // and only when this flag is set.
    DDB_DECODER_HINT_NEED_BITRATE = 0x2,
    // Decoders can do their own infinite looping when this flag is set, in the
    // "Loop Single" looping mode.
    DDB_DECODER_HINT_CAN_LOOP = 0x4,
#endif
#if (DDB_API_LEVEL >= 10)
    // Don't modify the stream (e.g. no replaygain, clipping, etc), provide the maximum possible precision, preferably in float32.
    // Supposed to be used by converter, replaygain scanner, etc.
    DDB_DECODER_HINT_RAW_SIGNAL = 0x8,
#endif
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
    DB_playItem_t * (*insert) (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname);

    int (*numvoices) (DB_fileinfo_t *info);
    void (*mutevoice) (DB_fileinfo_t *info, int voice, int mute);

    int (*read_metadata) (DB_playItem_t *it);
    int (*write_metadata) (DB_playItem_t *it);

    // NULL terminated array of all supported extensions
    // examples:
    // { "aac", "wma", "tak", NULL } -- supports 3 file extensions
    // since API 1.9: { "*", NULL } -- supports any file extensions
    const char **exts;

    // NULL terminated array of all supported prefixes (UADE support needs that)
    // e.g. "mod.song_title"
    const char **prefixes;

#if (DDB_API_LEVEL >= 7)
    // This function's purpose is to open the file, so that the file handle is
    // immediately accessible via DB_fileinfo_t, and can be used with fabort.
    // If a plugin is using open2, it should not reopen the file from init.
    // Plugins _must_ implement open even if open2 is present,
    // because existing code may rely on it.
    DB_fileinfo_t *(*open2) (uint32_t hints, DB_playItem_t *it);
#endif
} DB_decoder_t;

#if (DDB_API_LEVEL >= 14)
/// Extended decoder interface, with 64 bit seeking support.
/// Usage:
///    Use ddb_decoder2_t as your base plugin type
///    Add DDB_PLUGIN_FLAG_IMPLEMENTS_DECODER2 to your plugin's flags field.
typedef struct ddb_decoder2_s {
    DB_decoder_t decoder;

    // perform seeking in samples (if possible)
    // return -1 if failed, or 0 on success
    // if -1 is returned, that will mean that streamer must skip that song
    int (*seek_sample64) (DB_fileinfo_t *info, int64_t sample);

    void *padding[8];
} ddb_decoder2_t;
#endif

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
#if (DDB_API_LEVEL >= 11)
    ddb_playback_state_t (*state) (void);
#else
    int (*state) (void);
#endif
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

    // since 1.1
#if (DDB_API_LEVEL >= 1)
    // can be NULL
    // should return 1 if the DSP plugin will not touch data with the current parameters;
    // 0 otherwise
    int (*can_bypass) (ddb_dsp_context_t *ctx, ddb_waveformat_t *fmt);
#endif
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

// Was used to interrupt hanging network streams, but not used since API 1.11.
// Use get_identifier / abort_with_identifier instead
    void (*abort) (DB_FILE *stream) DEPRECATED_111;

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

#if (DDB_API_LEVEL >= 6)
    // returns URI scheme for a given file name, e.g. "zip://"
    // can be NULL
    // can return NULL
    const char *(*get_scheme_for_name) (const char *fname);
#endif

#if (DDB_API_LEVEL >= 11)
    // Optional method, which should return a unique ID associated with the file
    uint64_t (*get_identifier) (DB_FILE *f);

    // Optional method to abort any file / stream operation on a file with specified identifier
    void (*abort_with_identifier) (uint64_t identifier);
#endif
} DB_vfs_t;

// gui plugin
// only one gui plugin can be running at the same time
// should provide GUI services to other plugins

// this structure represents a gui dialog with callbacks to set/get params
// documentation should be available here:
// http://github.com/DeaDBeeF-Player/deadbeef/wiki/GUI-Script-Syntax
typedef struct {
    const char *title;
    const char *layout;
    void (*set_param) (const char *key, const char *value);
    void (*get_param) (const char *key, char *value, int len, const char *def);

#if (DDB_API_LEVEL >= 4)
    void *parent;
#endif
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

    DB_playItem_t * (*load) (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);

    // will save items from first to last (inclusive)
    // format is determined by extension
    // playlist is protected from changes during the call
    int (*save) (ddb_playlist_t *plt, const char *fname, DB_playItem_t *first, DB_playItem_t *last);

    const char **extensions; // NULL-terminated list of supported file extensions, e.g. {"m3u", "pls", NULL}

    // since 1.5
#if (DDB_API_LEVEL >= 5)
    // NOTE: load2 is not used by any existing plugins, and its purpose it lost in history.
    // Supposedly, it was added to support plugins which could implement cuesheet loading
    // as a playlist format, which didn't work out.
    // Generally, it's not recommended to use this, as the behavior is undefined.
    DB_playItem_t * (*load2) (int visibility, ddb_playlist_t *plt, DB_playItem_t *after, const char *fname, int *pabort);
#endif
} DB_playlist_t;

// NOTE: Media source API is a work in progress, and is disabled in this version of source code.
// This is to prevent plugin devs from releasing media source plugins, before this API is finalized.
// Please use the appropriate development branch to test media source plugins.
// The media source API is a subject to change.
#if (DDB_API_LEVEL >= 15)

// Mediasource plugin
// The purpose is to provide access to external media sources.
// It's used for the built-in media library plugin.

/// Numbers from 0 to 1023 are reserved to base interface, as declared in deadbeef.h
typedef enum {
    DDB_MEDIASOURCE_EVENT_CONTENT_DID_CHANGE = 0,
    DDB_MEDIASOURCE_EVENT_STATE_DID_CHANGE = 1,
    DDB_MEDIASOURCE_EVENT_ENABLED_DID_CHANGE = 2,
    DDB_MEDIASOURCE_EVENT_SELECTORS_DID_CHANGE = 3,
    DDB_MEDIASOURCE_EVENT_OUT_OF_SYNC = 4, // Needs refresh -- e.g. if there are new files in music folders
} ddb_mediasource_event_type_t;

/// Numbers from 1024 and up can be used by the plugins for additional events.
#define DDB_MEDIASOURCE_EVENT_MAX 1023

typedef enum {
    DDB_MEDIASOURCE_STATE_IDLE,
    DDB_MEDIASOURCE_STATE_LOADING,
    DDB_MEDIASOURCE_STATE_SCANNING,
    DDB_MEDIASOURCE_STATE_INDEXING,
    DDB_MEDIASOURCE_STATE_SAVING,
} ddb_mediasource_state_t;

typedef void (* ddb_medialib_listener_t)(ddb_mediasource_event_type_t event, void *user_data);

/// Each media source plugin can create source instances for you, by calling @c create_source.
typedef void *ddb_mediasource_source_t;

/// Abstract type representing a selector for media source query (e.g. Albums, Artists, Genres)
/// Use @c get_selectors_list method to get the list of available selectors.
/// Use the values to specify the selector when calling @c create_item_tree.
typedef struct ddb_mediasource_list_selector_s *ddb_mediasource_list_selector_t;

/// Opaque struct representing the extended API of the underlying plugin. Use @c get_extended_api method to get it.
typedef struct ddb_mediasource_api_s ddb_mediasource_api_t;

/// Opaque struct representing the item in a tree. Use tree_item_* set of functions to access the values.
typedef struct ddb_medialib_item_s ddb_medialib_item_t;

/// NOTE: never "subclass" the DB_mediasource_t struct - this is not backwards compatible.
/// Instead, implement the get_extended_api method to provide access to the plugin API.
typedef struct {
    DB_plugin_t plugin;

    /// Get access to the underlying plugin API.
    /// The returned pointer would have to be cast to the plugin's API structure type.
    ddb_mediasource_api_t *(*get_extended_api) (void);

    /// The name of the source (e.g. "Media Library").
    /// This could be used by a UI plugin to display in a list of all available sources.
    const char *(*source_name) (void);

    /// Create a media source instance. It must be freed after use by calling the @c free_source
    /// @param source_path a unique name to identify the instance, this will be used to prefix individual instance configuration files, caches, etc.
    ddb_mediasource_source_t (*create_source) (const char *source_path);

    /// Free the @c source created by @c create_source
    void (*free_source) (ddb_mediasource_source_t source);

    /// Enable or disable the source
    void (*set_source_enabled) (ddb_mediasource_source_t source, int enabled);

    /// Get the enabled state
    int (*is_source_enabled) (ddb_mediasource_source_t source);

    /// This tells the source to start operating with the current configuration.
    /// It may cancel any current operation, and get the new state with the new settings.
    /// For example, the medialib plugin is supposed to start the scanner with the current / new settings.
    /// This source is not supposed to run any operations automatically, and the caller is expected to call refresh
    /// every time when the plugin configuration changes.
    /// However, when the source is created - it's may load its initial state.
    void (*refresh) (ddb_mediasource_source_t source);

    /// A selector is a token, which can be used to find out all top level items that can be queried from the library.
    /// For example - Folders, Albums, Artists, Genres.
    /// @return the list of selectors. The caller must free the list after use, by calling @c free_selectors
    ddb_mediasource_list_selector_t *(*get_selectors_list) (ddb_mediasource_source_t source);

    /// Free the selector list
    void (*free_selectors_list) (ddb_mediasource_source_t source, ddb_mediasource_list_selector_t *selectors);

    /// Get selector name
    const char *(*selector_name) (ddb_mediasource_source_t source, ddb_mediasource_list_selector_t selector);

    /// Add event listener. Your callback function will be called every time some event occurs. Such as state change, content update, and so on.
    /// The callback function may be executed on background thread, so make sure to dispatch to main to update UI.
    int (*add_listener) (ddb_mediasource_source_t source, ddb_medialib_listener_t listener, void *user_data);

    /// Remove event listener
    void (*remove_listener) (ddb_mediasource_source_t source, int listener_id);

    /// Create a tree of items for the given @c selector.
    /// The tree is immutable, and can be used by the caller in any way it needs.
    /// The caller must free the returned object by calling the @c free_list
    ddb_medialib_item_t * (*create_item_tree) (ddb_mediasource_source_t source, ddb_mediasource_list_selector_t selector, const char *filter);

    /// Free the tree created by the @c create_list
    void (*free_item_tree) (ddb_mediasource_source_t source, ddb_medialib_item_t *list);

    /// Whether the scanner/indexer is active
    ddb_mediasource_state_t (*scanner_state) (ddb_mediasource_source_t source);

    // It is recommended to use the select/expand methods below
    // to preserve selected/expanded state across medialib refreshes.

    /// Returns 1 if the specified item is selected, 0 otherwise.
    int (*is_tree_item_selected) (ddb_mediasource_source_t source, const ddb_medialib_item_t *item);

    /// Select/delesect the specified item
    void (*set_tree_item_selected) (ddb_mediasource_source_t source, const ddb_medialib_item_t *item, int selected);

    /// Returns 1 if the specified item is expanded, 0 otherwise
    int (*is_tree_item_expanded) (ddb_mediasource_source_t source, const ddb_medialib_item_t *item);

    /// Expand/collapse the specified item
    void (*set_tree_item_expanded) (ddb_mediasource_source_t source, const ddb_medialib_item_t *item, int expanded);

    /// Returns the text associated with the item, e.g. a genre value, or the artist, etc.
    const char *(*tree_item_get_text) (const ddb_medialib_item_t *item);

    /// Returns the track associated with the item. Can be null - typically for non-leaf nodes.
    ddb_playItem_t *(*tree_item_get_track) (const ddb_medialib_item_t *item);

    /// Returns the next item in the list.
    const ddb_medialib_item_t *(*tree_item_get_next) (const ddb_medialib_item_t *item);

    /// Returns the first item in the list of child items.
    const ddb_medialib_item_t *(*tree_item_get_children) (const ddb_medialib_item_t *item);

    /// Returns the number of children of this item.
    int (*tree_item_get_children_count) (const ddb_medialib_item_t *item);
} DB_mediasource_t;

#endif

#undef DDB_DEPRECATED
#undef DEPRECATED

#ifdef __cplusplus
}
#endif

#endif // __DEADBEEF_H
