#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dumb/dumb-kode54/include/dumb.h"
#include "codec.h"
#include "cdumb.h"
#include "playlist.h"

extern int sdl_player_freq; // hack!
static int dumb_initialized;
static DUH *myduh;
static DUH_SIGRENDERER *renderer;
//#define DUMB_RQ_ALIASING
//#define DUMB_RQ_LINEAR
//#define DUMB_RQ_CUBIC
//#define DUMB_RQ_N_LEVELS
extern int dumb_resampling_quality;
extern int dumb_it_max_to_mix;

void
cdumb_free (void);

int
cdumb_startrenderer (void);

int
cdumb_init (const char *fname, int track, float start, float end) {
    if (!dumb_initialized) {
        atexit (&dumb_exit);
    }
    dumb_register_stdfiles ();
    const char *ext = fname + strlen (fname) - 1;
    while (*ext != '.' && ext > fname) {
        ext--;
    }
    ext++;
    if (!strcasecmp (ext, "mod")) {
        myduh = dumb_load_mod_quick (fname, 0);
    }
    else if (!strcasecmp (ext, "s3m")) {
        myduh = dumb_load_s3m_quick (fname);
    }
    else if (!strcasecmp (ext, "it")) {
        myduh = dumb_load_it_quick (fname);
    }
    else if (!strcasecmp (ext, "xm")) {
        myduh = dumb_load_xm_quick (fname);
    }
    else {
        return -1;
    }
    dumb_it_do_initial_runthrough (myduh);

    cdumb.info.bitsPerSample = 16;
    cdumb.info.channels = 2;
    cdumb.info.samplesPerSecond = sdl_player_freq;
    cdumb.info.position = 0;
    cdumb.info.duration = duh_get_length (myduh)/65536.0f;
    printf ("duration: %f\n", cdumb.info.duration);

    cdumb_startrenderer ();

    return 0;
}

int
cdumb_startrenderer (void) {
    // reopen
    if (renderer) {
        duh_end_sigrenderer (renderer);
        renderer = NULL;
    }
    renderer = duh_start_sigrenderer (myduh, 0, 2, 0);
    if (!renderer) {
        cdumb_free ();
        return -1;
    }

    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer (renderer);
    dumb_it_set_loop_callback (itsr, &dumb_it_callback_terminate, NULL);
    dumb_it_set_resampling_quality (itsr, 2);
    dumb_it_set_xm_speed_zero_callback (itsr, &dumb_it_callback_terminate, NULL);
    dumb_it_set_global_volume_zero_callback (itsr, &dumb_it_callback_terminate, NULL);
}

void
cdumb_free (void) {
    if (renderer) {
        duh_end_sigrenderer (renderer);
        renderer = NULL;
    }
    if (myduh) {
        unload_duh (myduh);
        myduh = NULL;
    }
}

int
cdumb_read (char *bytes, int size) {
    int length = size / 4;
    long ret;
    ret = duh_render (renderer, 16, 0, 1, 65536.f / cdumb.info.samplesPerSecond, length, bytes);
    cdumb.info.position += ret / (float)cdumb.info.samplesPerSecond;
    return ret*4;
}

int
cdumb_seek (float time) {
    if (time < cdumb.info.position) {
        cdumb_startrenderer ();
    }
    else {
        time -= cdumb.info.position;
    }
    int pos = time * cdumb.info.samplesPerSecond;
    duh_sigrenderer_generate_samples (renderer, 0, 65536.0f / cdumb.info.samplesPerSecond, pos, NULL);
    cdumb.info.position = duh_sigrenderer_get_position (renderer) / 65536.f;
    return 0;
}

int
cdumb_add (const char *fname) {
    return 0;
}

codec_t cdumb = {
    .init = cdumb_init,
    .free = cdumb_free,
    .read = cdumb_read,
    .seek = cdumb_seek,
    .add = cdumb_add
};

