/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

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
st_process (ddb_dsp_context_t *_src, float *samples, int nframes, int maxframes, ddb_waveformat_t *fmt, float *ratio) {
    ddb_soundtouch_t *st = (ddb_soundtouch_t *)_src;
    if (st->changed) {
        st_set_rate (st->st, 1);
        st_set_rate_change (st->st, st->rate);
        st_set_pitch_semi_tones (st->st, st->pitch);
        st_set_tempo_change (st->st, st->tempo);
        st_set_setting (st->st, SETTING_USE_AA_FILTER, st->use_aa_filter);
        st_set_setting (st->st, SETTING_AA_FILTER_LENGTH, st->aa_filter_length &~ 7);
        st_set_setting (st->st, SETTING_USE_QUICKSEEK, st->use_quickseek);
        st_set_setting (st->st, SETTING_SEQUENCE_MS, st->sequence_ms);
        st_set_setting (st->st, SETTING_SEEKWINDOW_MS, st->seekwindow_ms);
        st->changed = 0;
    }
    *ratio = (1.f + 0.01f * st->tempo);

    st_set_sample_rate (st->st, fmt->samplerate);
    st_set_channels (st->st, fmt->channels);

    st_put_samples (st->st, samples, nframes);
    int nout = 0;
    int n = 0;
    do {
        n = st_receive_samples (st->st, samples, maxframes);
        maxframes -= n;
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
        st->changed = 1;
        break;
    case ST_PARAM_SEEKWINDOW_MS:
        st->seekwindow_ms = atoi (val);
        st->changed = 1;
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
        snprintf (val, sz, "%f", st->tempo);
        break;
    case ST_PARAM_PITCH:
        snprintf (val, sz, "%f", st->pitch);
        break;
    case ST_PARAM_RATE:
        snprintf (val, sz, "%f", st->rate);
        break;
    case ST_PARAM_USE_AA_FILTER:
        snprintf (val, sz, "%d", st->use_aa_filter);
        break;
    case ST_PARAM_AA_FILTER_LENGTH:
        snprintf (val, sz, "%d", st->aa_filter_length);
        break;
    case ST_PARAM_USE_QUICKSEEK:
        snprintf (val, sz, "%d", st->use_quickseek);
        break;
    case ST_PARAM_SEQUENCE_MS:
        snprintf (val, sz, "%d", st->sequence_ms);
        break;
    case ST_PARAM_SEEKWINDOW_MS:
        snprintf (val, sz, "%d", st->seekwindow_ms);
        break;
    default:
        fprintf (stderr, "st_get_param: invalid param index (%d)\n", p);
    }
}

static const char settings_dlg[] =
    "property \"Tempo Change (%)\" spinbtn[-90,200,1] 0 0;\n"
    "property \"Pitch Change (semi-tones)\" spinbtn[-24,24,0.0000001] 1 0;\n"
    "property \"Playback Rate Change (%)\" spinbtn[-90,200,1] 2 0;\n"
    "property \"Use AA Filter\" checkbox 3 0;\n"
    "property \"AA Filter Length\" spinbtn[16,64,1] 4 32;\n"
    "property \"Use Quickseek\" checkbox 5 0;\n"
    "property \"Time Stretch Sequence Length (ms)\" spinbtn[10,50,1] 6 10;\n"
    "property \"Time Stretch Seek Window Length (ms)\" spinbtn[10,50,1] 7 28;\n"
;

static DB_dsp_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .open = st_open,
    .close = st_close,
    .process = st_process,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "soundtouch",
    .plugin.name = "Soundtouch",
    .plugin.descr = "Tempo/Pitch/Rate changer using SoundTouch Library (http://www.surina.net/soundtouch)",
    .plugin.copyright = 
        "Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses SoundTouch Library 2.1.2, (C) Olli Parviainen"
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
    .plugin.website = "http://deadbeef.sf.net",
    .num_params = st_num_params,
    .get_param_name = st_get_param_name,
    .set_param = st_set_param,
    .get_param = st_get_param,
    .reset = st_reset,
    .configdialog = settings_dlg,
};

DB_plugin_t *
ddb_soundtouch_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.plugin;
}
