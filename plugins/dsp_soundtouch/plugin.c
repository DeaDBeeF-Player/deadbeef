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
#include "../../deadbeef.h"
#include "st.h"

enum {
    ST_PARAM_TEMPO,
    ST_PARAM_PITCH,
    ST_PARAM_RATE,
    ST_PARAM_USE_AA_FILTER,
    ST_PARAM_AA_FILTER_LENGTH,
    ST_PARAM_USE_QUICKSEEK,
    ST_PARAM_SEQUENCE_MS,
    ST_PARAM_SEEKWINDOW_MS,
    ST_PARAM_COUNT
};

static DB_functions_t *deadbeef;
static DB_dsp_t plugin;

typedef struct {
    ddb_dsp_context_t ctx;
    void *st;
    float tempo;
    float pitch;
    float rate;
    int use_aa_filter;
    int aa_filter_length;
    int use_quickseek;
    int sequence_ms;
    int seekwindow_ms;
    int changed;
} ddb_soundtouch_t;

ddb_dsp_context_t*
st_open (void) {
    ddb_soundtouch_t *st = malloc (sizeof (ddb_soundtouch_t));
    DDB_INIT_DSP_CONTEXT (st,ddb_soundtouch_t,&plugin);
    st->st = st_alloc ();
    st->changed = 1;
    st->tempo = 0;
    st->rate = 0;
    st->pitch = 0;
    st->use_aa_filter = 0;
    st->aa_filter_length = 32;
    st->use_quickseek = 0;
    st->sequence_ms = 82;
    st->seekwindow_ms = 28;
    return (ddb_dsp_context_t *)st;
}

void
st_close (ddb_dsp_context_t *_src) {
    ddb_soundtouch_t *st = (ddb_soundtouch_t *)_src;
    if (st->st) {
        st_free (st->st);
    }
    free (st);
}

void
st_reset (ddb_dsp_context_t *_src) {
    ddb_soundtouch_t *st = (ddb_soundtouch_t *)_src;
    st_clear (st->st);
}

int
st_process (ddb_dsp_context_t *_src, float *samples, int nframes, ddb_waveformat_t *fmt) {
    ddb_soundtouch_t *st = (ddb_soundtouch_t *)_src;
    if (st->changed) {
        st_set_rate_change (st->st, st->rate);
        st_set_pitch_semi_tones (st->st, st->pitch);
        st_set_tempo_change (st->st, st->tempo);
        st_set_setting (st->st, SETTING_USE_AA_FILTER, st->use_aa_filter);
        st_set_setting (st->st, SETTING_AA_FILTER_LENGTH, st->aa_filter_length);
        st_set_setting (st->st, SETTING_USE_QUICKSEEK, st->use_quickseek);
        st_set_setting (st->st, SETTING_SEQUENCE_MS, st->sequence_ms);
        st_set_setting (st->st, SETTING_SEEKWINDOW_MS, st->seekwindow_ms);
        st->changed = 0;
    }

    st_set_sample_rate (st->st, fmt->samplerate);
    st_set_channels (st->st, fmt->channels);

    st_put_samples (st->st, samples, nframes);
    int nout = 0;
    int nmax = nframes * 24;
    int n = 0;
    do {
        n = st_receive_samples (st->st, samples, nmax);
        nmax -= n;
        samples += n * fmt->channels;
        nout += n;
    } while (n != 0);

    return nout;
}

const char *
st_get_param_name (int p) {
    switch (p) {
    case ST_PARAM_TEMPO:
        return "Tempo";
    case ST_PARAM_PITCH:
        return "Pitch";
    case ST_PARAM_RATE:
        return "Playback Rate";
    case ST_PARAM_USE_AA_FILTER:
        return "Use AA Filter";
    case ST_PARAM_AA_FILTER_LENGTH:
        return "AA Filter Length";
    case ST_PARAM_USE_QUICKSEEK:
        return "Use Quickseek";
    case ST_PARAM_SEQUENCE_MS:
        return "Time Stretch Sequence Length (ms)";
    case ST_PARAM_SEEKWINDOW_MS:
        return "Time Stretch Seek Window Length (ms)";
    default:
        fprintf (stderr, "st_param_name: invalid param index (%d)\n", p);
    }
    return NULL;
}

int
st_num_params (void) {
    return ST_PARAM_COUNT;
}

void
st_set_param (ddb_dsp_context_t *ctx, int p, const char *val) {
    ddb_soundtouch_t *st = (ddb_soundtouch_t *)ctx;
    switch (p) {
    case ST_PARAM_TEMPO:
        st->tempo = atof (val);
        st->changed = 1;
        break;
    case ST_PARAM_PITCH:
        st->pitch = atof (val);
        st->changed = 1;
        break;
    case ST_PARAM_RATE:
        st->rate = atof (val);
        st->changed = 1;
        break;
    case ST_PARAM_USE_AA_FILTER:
        st->use_aa_filter = atoi (val);
        st->changed = 1;
        break;
    case ST_PARAM_AA_FILTER_LENGTH:
        st->aa_filter_length = atoi (val);
        st->changed = 1;
        break;
    case ST_PARAM_USE_QUICKSEEK:
        st->use_quickseek = atoi (val);
        st->changed = 1;
        break;
    case ST_PARAM_SEQUENCE_MS:
        st->sequence_ms = atoi (val);
        break;
    case ST_PARAM_SEEKWINDOW_MS:
        st->seekwindow_ms = atoi (val);
        break;
    default:
        fprintf (stderr, "st_param: invalid param index (%d)\n", p);
    }
}

void
st_get_param (ddb_dsp_context_t *ctx, int p, char *val, int sz) {
    ddb_soundtouch_t *st = (ddb_soundtouch_t *)ctx;
    switch (p) {
    case ST_PARAM_TEMPO:
        snprintf (val, sizeof (val), "%f", st->tempo);
        break;
    case ST_PARAM_PITCH:
        snprintf (val, sizeof (val), "%f", st->pitch);
        break;
    case ST_PARAM_RATE:
        snprintf (val, sizeof (val), "%f", st->rate);
        break;
    case ST_PARAM_USE_AA_FILTER:
        snprintf (val, sizeof (val), "%d", st->use_aa_filter);
        break;
    case ST_PARAM_AA_FILTER_LENGTH:
        snprintf (val, sizeof (val), "%d", st->aa_filter_length);
        break;
    case ST_PARAM_USE_QUICKSEEK:
        snprintf (val, sizeof (val), "%d", st->use_quickseek);
        break;
    case ST_PARAM_SEQUENCE_MS:
        snprintf (val, sizeof (val), "%d", st->sequence_ms);
        break;
    case ST_PARAM_SEEKWINDOW_MS:
        snprintf (val, sizeof (val), "%d", st->seekwindow_ms);
        break;
    default:
        fprintf (stderr, "st_get_param: invalid param index (%d)\n", p);
    }
}

static const char settings_dlg[] =
    "property \"Tempo Change (%)\" spinbtn[-200,200,1] 0 0;\n"
    "property \"Pitch Change (semi-tones)\" spinbtn[-24,24,1] 1 0;\n"
    "property \"Rate Change (%)\" spinbtn[-200,200,1] 2 0;\n"
    "property \"Use AA Filter\" checkbox 3 0;\n"
    "property \"AA Filter Length\" spinbtn[16,64,1] 4 32;\n"
    "property \"Use Quickseek\" checkbox 5 0;\n"
    "property \"Time Stretch Sequence Length (ms)\" spinbtn[10,500,1] 6 82;\n"
    "property \"Time Stretch Seek Window Length (ms)\" spinbtn[10,500,1] 7 28;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .open = st_open,
    .close = st_close,
    .process = st_process,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "soundtouch",
    .plugin.name = "Soundtouch",
    .plugin.descr = "Tempo/Pitch/Rate changer using SoundTouch Library (http://www.surina.net/soundtouch)",
    .plugin.author = "Alexey Yakovenko",
    .plugin.email = "waker@users.sf.net",
    .plugin.website = "http://deadbeef.sf.net",
    .num_params = st_num_params,
    .get_param_name = st_get_param_name,
    .set_param = st_set_param,
    .get_param = st_get_param,
    .reset = st_reset,
    .configdialog = settings_dlg,
};

DB_plugin_t *
ddb_dsp_soundtouch_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.plugin;
}
