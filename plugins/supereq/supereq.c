/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include "../../deadbeef.h"
#include "Equ.h"

static DB_functions_t *deadbeef;
static DB_dsp_t plugin;

typedef struct {
    ddb_dsp_context_t ctx;
    float last_srate;
    int last_nch;
    float lbands[18];
    float rbands[18];
    float preamp;
    void *paramsroot;
    int params_changed;
    uintptr_t mutex;
    SuperEqState state;
} ddb_supereq_ctx_t;

void supereq_reset (ddb_dsp_context_t *ctx);

void
recalc_table (ddb_supereq_ctx_t *eq) {
    void *params = paramlist_alloc ();

    deadbeef->mutex_lock (eq->mutex);
    float lbands_copy[18];
    float rbands_copy[18];
    float srate = eq->last_srate;
    memcpy (lbands_copy, eq->lbands, sizeof (eq->lbands));
    memcpy (rbands_copy, eq->rbands, sizeof (eq->rbands));
    for (int i = 0; i < 18; i++) {
        lbands_copy[i] *= eq->preamp;
        rbands_copy[i] *= eq->preamp;
    }
    deadbeef->mutex_unlock (eq->mutex);

    equ_makeTable (&eq->state, lbands_copy, rbands_copy, params, srate);

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
supereq_process (ddb_dsp_context_t *ctx, float *samples, int frames, ddb_waveformat_t *fmt) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    if (supereq->params_changed) {
        recalc_table (supereq);
        supereq->params_changed = 0;
    }
	if (supereq->last_srate != fmt->samplerate) {
        deadbeef->mutex_lock (supereq->mutex);
		supereq->last_srate = fmt->samplerate;
		supereq->last_nch = fmt->channels;
        recalc_table (supereq);
        deadbeef->mutex_unlock (supereq->mutex);
		equ_clearbuf(&supereq->state);
    }
	else if (supereq->last_nch != fmt->channels) {
        deadbeef->mutex_lock (supereq->mutex);
		supereq->last_nch = fmt->channels;
        deadbeef->mutex_unlock (supereq->mutex);
		equ_clearbuf(&supereq->state);
    }
	equ_modifySamples_float(&supereq->state, (char *)samples,frames,fmt->channels);
	return frames;
}

float
supereq_get_band (ddb_dsp_context_t *ctx, int band) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    return supereq->lbands[band];
}

void
supereq_set_band (ddb_dsp_context_t *ctx, int band, float value) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    deadbeef->mutex_lock (supereq->mutex);
    supereq->lbands[band] = supereq->rbands[band] = value;
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

void
supereq_enable (ddb_dsp_context_t *ctx, int e) {
    ddb_supereq_ctx_t *supereq = (ddb_supereq_ctx_t *)ctx;
    if (e != supereq->ctx.enabled) {
        if (e && !supereq->ctx.enabled) {
            supereq_reset (ctx);
        }
        supereq->ctx.enabled = e;
    }

    DB_playItem_t *it = deadbeef->streamer_get_playing_track ();
    if (it) {
        float playpos = deadbeef->streamer_get_playpos ();
        deadbeef->streamer_seek (playpos);
        deadbeef->pl_item_unref (it);
    }
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
void
supereq_set_param (ddb_dsp_context_t *ctx, int p, float val) {
    switch (p) {
    case 0:
        supereq_set_preamp (ctx, val);
        break;
    case 1 ... 18:
        supereq_set_band (ctx, p, val);
        break;
    default:
        fprintf (stderr, "supereq_set_param: invalid param index (%d)\n", p);
    }
}

float
supereq_get_param (ddb_dsp_context_t *ctx, int p) {
    switch (p) {
    case 0:
        return supereq_get_preamp (ctx);
        break;
    case 1 ... 18:
        return supereq_get_band (ctx, p);
        break;
    default:
        fprintf (stderr, "supereq_get_param: invalid param index (%d)\n", p);
    }
}


ddb_dsp_context_t*
supereq_open (void) {
    ddb_supereq_ctx_t *supereq = malloc (sizeof (ddb_supereq_ctx_t));
    DDB_INIT_DSP_CONTEXT (supereq,ddb_supereq_ctx_t,&plugin);

    equ_init (&supereq->state, 14);
    supereq->paramsroot = paramlist_alloc ();
    supereq->last_srate = 44100;
    supereq->last_nch = 2;
    supereq->mutex = deadbeef->mutex_create ();
    recalc_table (supereq);
    equ_clearbuf (&supereq->state);
//    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (supereq_on_configchanged), 0);


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

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "supereq",
    .plugin.name = "SuperEQ",
    .plugin.descr = "equalizer plugin using SuperEQ library by Naoki Shibata",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sourceforge.net",
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.start = supereq_plugin_start,
    .plugin.stop = supereq_plugin_stop,
    .open = supereq_open,
    .close = supereq_close,
    .process = supereq_process,
    .reset = supereq_reset,
    .enable = supereq_enable,
    .num_params = supereq_num_params,
    .get_param_name = supereq_get_param_name,
    .set_param = supereq_set_param,
    .get_param = supereq_get_param,
};

DB_plugin_t *
supereq_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
