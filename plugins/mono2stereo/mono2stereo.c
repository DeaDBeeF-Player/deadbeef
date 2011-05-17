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

    free (m2s);
}

void
m2s_reset (ddb_dsp_context_t *ctx) {
    // use this method to flush dsp buffers, reset filters, etc
}

int
m2s_process (ddb_dsp_context_t *ctx, float *samples, int nframes, int maxframes, ddb_waveformat_t *fmt, float *r) {
    if (fmt->channels >= 2) {
        return nframes;
    }
    ddb_m2s_t *m2s = (ddb_m2s_t *)ctx;

    for (int i = nframes-1; i >= 0; i--) {
        samples[i*2+1] = samples[i] * m2s->rightmix;
        samples[i*2+0] = samples[i] * m2s->leftmix;
    }
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
    "property \"Left mix:\" hscale[0,1,0.001] 0 1;\n"
    "property \"Right mix:\" hscale[0,1,0.001] 1 1;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = 1,
    .plugin.api_vminor = 0,
    .open = m2s_open,
    .close = m2s_close,
    .process = m2s_process,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "m2s",
    .plugin.name = "Mono to stereo",
    .plugin.descr = "DSP plugin to convert mono to stereo",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n"
        "\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "GNU General Public License for more details.\n"
        "\n"
        "You should have received a copy of the GNU General Public License\n"
        "along with this program; if not, write to the Free Software\n"
        "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n"
    ,
    .num_params = m2s_num_params,
    .get_param_name = m2s_get_param_name,
    .set_param = m2s_set_param,
    .get_param = m2s_get_param,
    .reset = m2s_reset,
    .configdialog = settings_dlg,
};

DB_plugin_t *
mono2stereo_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.plugin;
}

