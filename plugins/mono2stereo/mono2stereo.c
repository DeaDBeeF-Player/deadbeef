/*
    Mono to stereo converter DSP plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko

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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <deadbeef/deadbeef.h>

enum {
    M2S_PARAM_LEFTMIX,
    M2S_PARAM_RIGHTMIX,
    M2S_PARAM_COUNT
};

static DB_functions_t *deadbeef;
static DB_dsp_t plugin;

typedef struct {
    ddb_dsp_context_t ctx;
    float leftmix;
    float rightmix;
    float *buffer;
    int buffer_nframes;
} ddb_m2s_t;

ddb_dsp_context_t*
m2s_open (void) {
    ddb_m2s_t *m2s = malloc (sizeof (ddb_m2s_t));
    DDB_INIT_DSP_CONTEXT (m2s,ddb_m2s_t,&plugin);

    // initialize
    m2s->leftmix = 1;
    m2s->rightmix = 1;

    return (ddb_dsp_context_t *)m2s;
}

void
m2s_close (ddb_dsp_context_t *ctx) {
    ddb_m2s_t *m2s = (ddb_m2s_t *)ctx;

    // free instance-specific allocations
    free (m2s->buffer);

    free (m2s);
}

void
m2s_reset (ddb_dsp_context_t *ctx) {
    // use this method to flush dsp buffers, reset filters, etc
}

static void
_m2s_alloc_buffer (ddb_m2s_t *m2s, int nframes) {
    if (m2s->buffer_nframes < nframes) {
        free (m2s->buffer);
        m2s->buffer = malloc (nframes * 2 * sizeof (float));
        m2s->buffer_nframes = nframes;
    }
}

int
m2s_process (ddb_dsp_context_t *ctx, float * restrict samples, int nframes, int maxframes, ddb_waveformat_t * restrict fmt, float * restrict r) {
    if (fmt->channels >= 2) {
        return nframes;
    }
    ddb_m2s_t *m2s = (ddb_m2s_t *)ctx;

    _m2s_alloc_buffer(m2s, nframes);
    float *input = samples;
    float *output = m2s->buffer;
    for (int i = 0; i < nframes; i++) {
        float sample = *input++;
        *output++ = sample * m2s->leftmix;
        *output++ = sample * m2s->rightmix;
    }
    memcpy (samples, m2s->buffer, nframes * 2 * sizeof (float));

    fmt->channels = 2;
    fmt->channelmask = 3;
    return nframes;
}

const char *
m2s_get_param_name (int p) {
    switch (p) {
    case M2S_PARAM_LEFTMIX:
        return "Left mix";
    case M2S_PARAM_RIGHTMIX:
        return "Right mix";
    default:
        fprintf (stderr, "m2s_param_name: invalid param index (%d)\n", p);
    }
    return NULL;
}

int
m2s_num_params (void) {
    return M2S_PARAM_COUNT;
}

void
m2s_set_param (ddb_dsp_context_t *ctx, int p, const char *val) {
    ddb_m2s_t *m2s = (ddb_m2s_t *)ctx;
    switch (p) {
    case M2S_PARAM_LEFTMIX:
        m2s->leftmix = atof (val);
        break;
    case M2S_PARAM_RIGHTMIX:
        m2s->rightmix = atof (val);
        break;
    default:
        fprintf (stderr, "m2s_set_param: invalid param index (%d)\n", p);
    }
}

void
m2s_get_param (ddb_dsp_context_t *ctx, int p, char *val, int sz) {
    ddb_m2s_t *m2s = (ddb_m2s_t *)ctx;
    switch (p) {
    case M2S_PARAM_LEFTMIX:
        snprintf (val, sz, "%f", m2s->leftmix);
        break;
    case M2S_PARAM_RIGHTMIX:
        snprintf (val, sz, "%f", m2s->rightmix);
        break;
    default:
        fprintf (stderr, "m2s_get_param: invalid param index (%d)\n", p);
    }
}

static const char settings_dlg[] =
    "property \"Left mix\" hscale[0,1,0.001] 0 1;\n"
    "property \"Right mix\" hscale[0,1,0.001] 1 1;\n"
;

static DB_dsp_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .open = m2s_open,
    .close = m2s_close,
    .process = m2s_process,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "m2s",
    .plugin.name = "Mono to stereo",
    .plugin.descr = "Mono to stereo converter DSP",
    .plugin.copyright = 
        "Mono to stereo converter DSP plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .num_params = m2s_num_params,
    .get_param_name = m2s_get_param_name,
    .set_param = m2s_set_param,
    .get_param = m2s_get_param,
    .reset = m2s_reset,
    .configdialog = settings_dlg,
};

DB_plugin_t *
ddb_mono2stereo_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.plugin;
}

