/*
    SuperEQ DSP plugin for DeaDBeeF Player
    Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <deadbeef/deadbeef.h>
#include "Equ.h"

static DB_functions_t *deadbeef;
static DB_dsp_t plugin;

typedef struct {
    ddb_dsp_context_t ctx;
    float last_srate;
    int last_nch;
    float bands[18];
    float preamp;
    void *paramsroot;
    int params_changed;
    uintptr_t mutex;
    SuperEqState state;
    int enabled;
} ddb_supereq_ctx_t;

void supereq_reset (ddb_dsp_context_t *ctx);

void
recalc_table (ddb_supereq_ctx_t *eq) {
    void *params = paramlist_alloc ();

    deadbeef->mutex_lock (eq->mutex);
    float bands_copy[18];
    float srate = eq->last_srate;
    memcpy (bands_copy, eq->bands, sizeof (eq->bands));
    for (int i = 0; i < 18; i++) {
        bands_copy[i] *= eq->preamp;
    }
    deadbeef->mutex_unlock (eq->mutex);

    equ_makeTable (&eq->state, bands_copy, params, srate);

    deadbeef->mutex_lock (eq->mutex);
    paramlist_free (eq->paramsroot);
    eq->paramsroot = params;
    deadbeef->mutex_unlock (eq->mutex);
}

int
supereq_plugin_start (void) {
    return 0;
}

int
supereq_plugin_stop (void) {
    return 0;
}

int
supereq_process (ddb_dsp_context_t *ctx, float *samples, int frames, int maxframes, ddb_waveformat_t *fmt, float *r) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    if (supereq->enabled != ctx->enabled) {
        if (ctx->enabled && !supereq->enabled) {
            supereq_reset (ctx);
        }
        supereq->enabled = ctx->enabled;
    }
    if (supereq->params_changed) {
        recalc_table (supereq);
        supereq->params_changed = 0;
    }
	if (supereq->last_srate != fmt->samplerate || supereq->last_nch != fmt->channels) {
        deadbeef->mutex_lock (supereq->mutex);
		supereq->last_srate = fmt->samplerate;
		supereq->last_nch = fmt->channels;
        equ_init (&supereq->state, 10, fmt->channels);
        recalc_table (supereq);
		equ_clearbuf(&supereq->state);
        deadbeef->mutex_unlock (supereq->mutex);
    }
	equ_modifySamples_float(&supereq->state, (char *)samples,frames,fmt->channels);
	return frames;
}

float
supereq_get_band (ddb_dsp_context_t *ctx, int band) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    return supereq->bands[band];
}

void
supereq_set_band (ddb_dsp_context_t *ctx, int band, float value) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    deadbeef->mutex_lock (supereq->mutex);
    supereq->bands[band] = value;
    deadbeef->mutex_unlock (supereq->mutex);
    supereq->params_changed = 1;
}

float
supereq_get_preamp (ddb_dsp_context_t *ctx) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    return supereq->preamp;
}

void
supereq_set_preamp (ddb_dsp_context_t *ctx, float value) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    deadbeef->mutex_lock (supereq->mutex);
    supereq->preamp = value;
    deadbeef->mutex_unlock (supereq->mutex);
    supereq->params_changed = 1;
}

void
supereq_reset (ddb_dsp_context_t *ctx) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    deadbeef->mutex_lock (supereq->mutex);
    equ_clearbuf(&supereq->state);
    deadbeef->mutex_unlock (supereq->mutex);
}

int
supereq_num_params (void) {
    return 19;
}

static const char *bandnames[] = {
    "Preamp",
    "55 Hz",
    "77 Hz",
    "110 Hz",
    "156 Hz",
    "220 Hz",
    "311 Hz",
    "440 Hz",
    "622 Hz",
    "880 Hz",
    "1.2 kHz",
    "1.8 kHz",
    "2.5 kHz",
    "3.5 kHz",
    "5 kHz",
    "7 kHz",
    "10 kHz",
    "14 kHz",
    "20 kHz"
};

const char *
supereq_get_param_name (int p) {
    return bandnames[p];
}


static inline float
db_to_amp (float dB) {
    const float ln10=2.3025850929940002f;
    return exp(ln10*dB/20.f);
}

static inline float
amp_to_db (float amp) {
    return 20*log10 (amp);
}

void
supereq_set_param (ddb_dsp_context_t *ctx, int p, const char *val) {
    switch (p) {
    case 0:
        supereq_set_preamp (ctx, db_to_amp (atof (val)));
        break;
    case 1 ... 18:
        supereq_set_band (ctx, p-1, db_to_amp (atof (val)));
        break;
    default:
        fprintf (stderr, "supereq_set_param: invalid param index (%d)\n", p);
    }
}

void
supereq_get_param (ddb_dsp_context_t *ctx, int p, char *v, int sz) {
    switch (p) {
    case 0:
        snprintf (v, sz, "%f", amp_to_db (supereq_get_preamp (ctx)));
        break;
    case 1 ... 18:
        snprintf (v, sz, "%f", amp_to_db (supereq_get_band (ctx, p-1)));
        break;
    default:
        fprintf (stderr, "supereq_get_param: invalid param index (%d)\n", p);
    }
}


ddb_dsp_context_t*
supereq_open (void) {
    ddb_supereq_ctx_t *supereq = malloc (sizeof (ddb_supereq_ctx_t));
    DDB_INIT_DSP_CONTEXT (supereq,ddb_supereq_ctx_t,&plugin);

    equ_init (&supereq->state, 10, 2);
    supereq->paramsroot = paramlist_alloc ();
    supereq->last_srate = 44100;
    supereq->last_nch = 2;
    supereq->mutex = deadbeef->mutex_create ();
    supereq->preamp = 1;
    for (int i = 0; i < 18; i++) {
        supereq->bands[i] = 1;
    }
    recalc_table (supereq);
    equ_clearbuf (&supereq->state);

    return (ddb_dsp_context_t*)supereq;
}

void
supereq_close (ddb_dsp_context_t *ctx) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    if (supereq->mutex) {
        deadbeef->mutex_free (supereq->mutex);
        supereq->mutex = 0;
    }
    equ_quit (&supereq->state);
    paramlist_free (supereq->paramsroot);
    free (ctx);
}

static const char settings_dlg[] =
    "property \"\" hbox[19] hmg fill expand border=0 spacing=8 height=200 noclip itemwidth=30;\n"
        "property \"Preamp\" vscale[20,-20,0.5] vert 0 0;\n"
        "property \"55\" vscale[20,-20,0.5] vert 1 0;\n"
        "property \"77\" vscale[20,-20,0.5] vert 2 0;\n"
        "property \"110\" vscale[20,-20,0.5] vert 3 0;\n"
        "property \"156\" vscale[20,-20,0.5] vert 4 0;\n"
        "property \"220\" vscale[20,-20,0.5] vert 5 0;\n"
        "property \"311\" vscale[20,-20,0.5] vert 6 0;\n"
        "property \"440\" vscale[20,-20,0.5] vert 7 0;\n"
        "property \"622\" vscale[20,-20,0.5] vert 8 0;\n"
        "property \"880\" vscale[20,-20,0.5] vert 9 0;\n"
        "property \"1.2K\" vscale[20,-20,0.5] vert 10 0;\n"
        "property \"1.8K\" vscale[20,-20,0.5] vert 11 0;\n"
        "property \"2.5K\" vscale[20,-20,0.5] vert 12 0;\n"
        "property \"3.5K\" vscale[20,-20,0.5] vert 13 0;\n"
        "property \"5K\" vscale[20,-20,0.5] vert 14 0;\n"
        "property \"7K\" vscale[20,-20,0.5] vert 15 0;\n"
        "property \"10K\" vscale[20,-20,0.5] vert 16 0;\n"
        "property \"14K\" vscale[20,-20,0.5] vert 17 0;\n"
        "property \"20K\" vscale[20,-20,0.5] vert 18 0;\n"
;

static DB_dsp_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "supereq",
    .plugin.name = "SuperEQ",
    .plugin.descr = "equalizer plugin using SuperEQ library",
    .plugin.copyright = 
        "SuperEQ DSP plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2014 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses supereq library by Naoki Shibata, http://shibatch.sourceforge.net\n"
        "Uses FFT library by Takuya Ooura, http://www.kurims.kyoto-u.ac.jp/~ooura/\n"
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
        "\n"
        "\n"
        "\n"
        "Based on:\n"
        "Shibatch Super Equalizer ver 0.03 for winamp\n"
        "written by Naoki Shibata  shibatch@users.sourceforge.net\n"
        "\n"
        "Shibatch Super Equalizer is a graphic and parametric equalizer plugin\n"
        "for winamp. This plugin uses 16383th order FIR filter with FFT algorithm.\n"
        "It's equalization is very precise. Equalization setting can be done\n"
        "for each channel separately.\n"
        "\n"
        "Processes of internal equalizer in winamp are actually done by each\n"
        "input plugin, so the results may differ for each input plugin.\n"
        "With this plugin, this problem can be avoided.\n"
        "\n"
        "This plugin is optimized for processors which have cache equal to or\n"
        "greater than 128k bytes(16383*2*sizeof(float) = 128k). This plugin\n"
        "won't work efficiently with K6 series processors(buy Athlon!!!).\n"
        "\n"
        "Do not forget pressing \"preview\" button after changing setting.\n"
        "\n"
        "http://shibatch.sourceforge.net/\n"
        "\n"
        "***\n"
        "\n"
        "  This program(except FFT part) is distributed under LGPL. See LGPL.txt for\n"
        "details.\n"
        "\n"
        "  FFT part is a routine made by Mr.Ooura. This routine is a freeware. Contact\n"
        "Mr.Ooura for details of distributing licenses.\n"
        "\n"
        "http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = supereq_plugin_start,
    .plugin.stop = supereq_plugin_stop,
    .open = supereq_open,
    .close = supereq_close,
    .process = supereq_process,
    .reset = supereq_reset,
    .num_params = supereq_num_params,
    .get_param_name = supereq_get_param_name,
    .set_param = supereq_set_param,
    .get_param = supereq_get_param,
    .configdialog = settings_dlg,
};

DB_plugin_t *
supereq_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
