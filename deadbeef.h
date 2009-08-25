/*
  deadbeef.h -- plugin API of the DeaDBeeF audio player
  http://deadbeef.sourceforge.net

  Copyright (C) 2009 Alexey Yakovenko

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

#ifdef __cplusplus
extern "C" {
#endif

// every plugin must define following entry-point:
// extern "C" DB_plugin_t* $MODULENAME_load (DB_functions_t *api);
// where $MODULENAME is a name of module
// e.g. if your plugin is called "myplugin.so", $MODULENAME is "myplugin"
// plugin should check vmajor and vminor members of api,
// and return NULL if they are not compatible
// otherwise it should return pointer to DB_plugin_t structure
// that is enough for both static and dynamic modules

// playlist structures

// playlist item
// there are "public" fields, available to plugins
typedef struct {
    char *fname; // full pathname
    struct codec_s *codec; // codec to use with this file
    int tracknum; // used for stuff like sid, nsf, cue (will be ignored by most codecs)
    float timestart; // start time of cue track, or -1
    float timeend; // end time of cue track, or -1
    float duration; // in seconds
    int startoffset; // offset to seek to skip tags and info-headers
    int endoffset; // offset from end of file where music data ends
    int shufflerating; // sort order for shuffle mode
    const char *filetype; // e.g. MP3 or OGG
} DB_playItem_t;

// plugin types
enum {
    DB_PLUGIN_DECODER = 1,
    DB_PLUGIN_OUTPUT  = 2,
    DB_PLUGIN_DSP     = 3,
    DB_PLUGIN_MISC    = 4
};

// event callback type
typedef int (*db_callback_t)(int ev, uintptr_t data);
#define DB_CALLBACK(x) ((db_callback_t)(x))

// events
enum {
    DB_EV_FRAMEUPDATE = 0, // ticks around 20 times per second, but ticker may stop sometimes
    DB_EV_SONGCHANGED = 1, // triggers when song was just changed
    DB_EV_MAX
};

// typecasting macros
#define DB_PLUGIN(x) ((DB_plugin_t *)(x))

// forward decl for plugin struct
struct DB_plugin_s;

// player api definition
typedef struct {
    // event subscribing
    void (*ev_subscribe) (struct DB_plugin_s *plugin, int ev, db_callback_t callback, uintptr_t data);
    void (*ev_unsubscribe) (struct DB_plugin_s *plugin, int ev, db_callback_t callback, uintptr_t data);
    // md5sum calc
    void (*md5) (uint8_t sig[16], const char *in, int len);
    void (*md5_to_str) (char *str, const uint8_t sig[16]);

    // event sending and handling
    int (*messagepump_push)(uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2);
    int (*messagepump_pop)(uint32_t *id, uintptr_t *ctx, uint32_t *p1, uint32_t *p2);
    // playlist access
    DB_playItem_t *(*pl_item_alloc) (void);
    void (*pl_item_free)(DB_playItem_t *it);
    int (*pl_add_dir) (const char *dirname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    int (*pl_add_file) (const char *fname, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    DB_playItem_t *(*pl_insert_dir) (DB_playItem_t *after, const char *dirname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
    DB_playItem_t *(*pl_insert_file) (DB_playItem_t *after, const char *fname, int *pabort, int (*cb)(DB_playItem_t *it, void *data), void *user_data);
} DB_functions_t;

// base plugin interface
typedef struct DB_plugin_s {
    // type must be one of DB_PLUGIN_ types
    int32_t type;
    // may be deactivated on failures after load
    int inactive;
    // any of those can be left NULL
    // though it's much better to fill them with something useful
    const char *name;
    const char *descr;
    const char *author;
    const char *email;
    const char *website;
    // start is called to start plugin; can be NULL
    int (*start) (void);
    // stop is called to deinit plugin; can be NULL
    int (*stop) (void);
    // exec_cmdline may be called at any moment when user sends commandline to player
    // can be NULL if plugin doesn't support commandline processing
    // cmdline is 0-separated list of strings, guaranteed to have 0 at the end
    // cmdline_size is number of bytes pointed by cmdline
    int (*exec_cmdline) (const char *cmdline, int cmdline_size);
} DB_plugin_t;

// decoder plugin
typedef struct {
    DB_plugin_t plugin;
    // init is called to prepare song to be started
    int (*init) (DB_playItem_t *it);

    // free is called after decoding is finished
    void (*free) (void);

    // read is called by streamer to decode specified number of bytes
    // must return number of bytes that were successfully decoded (sample aligned)
    // output format is always single precision float
    int (*read) (char *buffer, int size);

    // perform seeking in samples (if possible)
    // return -1 if failed, or 0 on success
    // if -1 is returned, that will mean that streamer must skip that song
    int (*seek) (int64_t samples);

    // 'insert' is called to insert new item to playlist
    // decoder is responsible to calculate duration, split it into subsongs, load cuesheet, etc
    // after==NULL means "prepend before 1st item in playlist"
    struct playItem_s * (*insert) (DB_playItem_t *after, const char *fname); 

    // NULL terminated array of all supported extensions
    const char **exts;

    // codec id used for playlist serialization
    const char *id;
} DB_decoder_t;

// output plugin
typedef struct {
    DB_plugin_t plugin;
    // init is called once at plugin activation
    int (*init) (void (*callback)(char *stream, int len));
    // free is called if output plugin was changed to another, or unload is about to happen
    int (*free) (void);
    // play, stop, pause, unpause are called by deadbeef in response to user
    // events, or as part of streaming process
    // state must be 0 for stopped, 1 for playing and 2 for paused
    int (*play) (void);
    int (*stop) (void);
    int (*pause) (void);
    int (*unpause) (void);
    int (*state) (void);
    // following functions must return output sampling rate, bits per sample and number
    // of channels
    int (*samplerate) (void);
    int (*bitspersample) (void);
    int (*channels) (void);
    // must return 0 for little endian output, or 1 for big endian
    int (*endianess) (void);
} DB_output_t;

// dsp plugin
typedef struct {
    DB_plugin_t plugin;
    // process gets called before SRC
    // stereo samples are stored in interleaved format
    // stereo sample is counted as 1 sample
    void (*process) (float *samples, int channels, int nsamples);
} DB_dsp_t;

// misc plugin
// purpose is to provide extra services
// e.g. scrobbling, converting, tagging, custom gui, etc.
// misc plugins should be mostly event driven, so no special entry points in them
typedef struct {
    DB_plugin_t plugin;
} DB_misc_t;

#ifdef __cplusplus
}
#endif

#endif // __DEADBEEF_H
