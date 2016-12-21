// this is a decoder plugin skeleton
// use to create new decoder plugins

#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>

#define trace(...) { fprintf(stderr, __VA_ARGS__); }

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    int startsample;
    int endsample;
    int currentsample;
} example_info_t;

static const char * exts[] = { "example", NULL }; // e.g. mp3

// allocate codec control structure
static DB_fileinfo_t *
example_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (example_info_t));
    example_info_t *info = (example_info_t *)_info;
    memset (info, 0, sizeof (example_info_t));
    return _info;
}

// prepare to decode the track, fill in mandatory plugin fields
// return -1 on failure
static int
example_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    example_info_t *info = (example_info_t *)_info;

    // take this parameters from your input file
    // we set constants for clarity sake
    _info->fmt.bps = 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate  = 44100;
    for (int i = 0; i < _info->fmt.channels; i++) {
        _info->fmt.channelmask |= 1 << i;
    }
    _info->readpos = 0;
    _info->plugin = &plugin;

    if (it->endsample > 0) {
        info->startsample = it->startsample;
        info->endsample = it->endsample;
        plugin.seek_sample (_info, 0);
    }
    else {
        info->startsample = 0;
        int TOTALSAMPLES = 1000; // calculate from file
        info->endsample = TOTALSAMPLES-1;
    }
    return 0;
}

// free everything allocated in _init
static void
example_free (DB_fileinfo_t *_info) {
    example_info_t *info = (example_info_t *)_info;
    if (info) {
        free (info);
    }
}


// try decode `size' bytes
// return number of decoded bytes
// or 0 on EOF/error
static int
example_read (DB_fileinfo_t *_info, char *bytes, int size) {
    example_info_t *info = (example_info_t *)_info;

    // CUE track switching support
    int samplesize = _info->fmt.bps / 8 * _info->fmt.channels;

    if (info->currentsample + size / samplesize > info->endsample) {
        size = (info->endsample - info->currentsample + 1) * samplesize;
        if (size <= 0) {
            return 0;
        }
    }

    int nblocks = size / samplesize;

    // ... decode nblocks blocks here ...

    info->currentsample += nblocks;
    return size;
}

// seek to specified sample (frame)
// return 0 on success
// return -1 on failure
static int
example_seek_sample (DB_fileinfo_t *_info, int sample) {
    example_info_t *info = (example_info_t *)_info;
    
    info->currentsample = sample + info->startsample;
    _info->readpos = (float)sample / _info->fmt.samplerate;
    return 0;
}

// seek to specified time in seconds
// return 0 on success
// return -1 on failure
static int
example_seek (DB_fileinfo_t *_info, float time) {
    return example_seek_sample (_info, time * _info->fmt.samplerate);
}

// read information from the track
// load/process cuesheet if exists
// insert track into playlist
// return track pointer on success
// return NULL on failure

static DB_playItem_t *
example_insert (ddb_playlist_t *plt, DB_playItem_t *after, const char *fname) {
    // open file
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        trace ("example: failed to fopen %s\n", fname);
        return NULL;
    }

    // decoder_* functions are imaginary -- you should replace them with real
    // decoder library calls
    decoder_info_t *di = decoder_open ();
    if (!di) {
        trace ("example: failed to init decoder\n");
        return NULL;
    }
    // read track info/tags
    track_info_t ti;
    if (decoder_read_info (&ti) < 0) {
        trace ("example: failed to read info\n");
        decoder_free (di);
        return NULL;
    }

    // replace "example" with your file type (e.g. MP3, WAV, etc)
    const char *ft = "example";

    // no cuesheet, prepare track for addition
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);

    deadbeef->pl_replace_meta (it, ":FILETYPE", ft);
    deadbeef->plt_set_item_duration (plt, it, (float)ti.total_num_samples/ti.samplerate);

    // now we should have track duration, and can try loading cuesheet
    // 1st try embedded cuesheet
    if (ti.embeddedcuesheet[0]) {
        DB_playItem_t *cue = deadbeef->plt_insert_cue_from_buffer (plt, after, it, ti.embeddedcuesheet, strlen (ti.embeddedcuesheet), ti.total_num_samples, ti.samplerate);
        if (cue) {
            deadbeef->pl_item_unref (it);
            deadbeef->pl_item_unref (cue);
            // cuesheet loaded
            decoder_free (di);
            return cue;
        }
    }

    // embedded cuesheet not found, try external one
    DB_playItem_t *cue = deadbeef->plt_insert_cue (plt, after, it, ti.total_num_samples, ti.samplerate);
    if (cue) {
        // cuesheet loaded
        deadbeef->pl_item_unref (it);
        deadbeef->pl_item_unref (cue);
        decoder_free (di);
        return cue;
    }


    // add metainfo
    if (!strlen (ti.title)) {
        // title is empty, this call will set track title to filename without extension
        deadbeef->pl_add_meta (it, "title", NULL);
    }
    else {
        deadbeef->pl_add_meta (it, "title", ti.title);
    }
    deadbeef->pl_add_meta (it, "artist", ti.artist);
    // ... etc ...

    // free decoder
    decoder_free (di);

    // now the track is ready, insert into playlist
    after = deadbeef->plt_insert_item (plt, after, it);
    deadbeef->pl_item_unref (it);
    return after;
}

static int
example_start (void) {
    // do one-time plugin initialization here
    // e.g. starting threads for background processing, subscribing to events, etc
    // return 0 on success
    // return -1 on failure
    return 0;
}

static int
example_stop (void) {
    // undo everything done in _start here
    // return 0 on success
    // return -1 on failure
    return 0;
}

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.name = "short plugin name",
    .plugin.id = "example",
    .plugin.descr = "plugin description",
    .plugin.copyright = "copyright message - author(s), license, etc",
    .plugin.start = example_start,
    .plugin.stop = example_stop,
    .open = example_open,
    .init = example_init,
    .free = example_free,
    .read = example_read,
    .seek = example_seek,
    .seek_sample = example_seek_sample,
    .insert = example_insert,
    .exts = exts,
};

DB_plugin_t *
example_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}

