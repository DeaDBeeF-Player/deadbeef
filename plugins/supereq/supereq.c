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
#include "../../deadbeef.h"
#include "supereq.h"

static DB_functions_t *deadbeef;
static DB_supereq_dsp_t plugin;

void *paramlist_alloc (void);
void paramlist_free (void *);
void equ_makeTable(float *lbc,float *rbc,void *param,float fs);
int equ_modifySamples(char *buf,int nsamples,int nch,int bps);
void equ_clearbuf(int bps,int srate);
void equ_init(int wb);
void equ_quit(void);

void supereq_reset (void);

static float last_srate = 0;
static int last_nch = 0, last_bps = 0;
static float lbands[18] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
static float rbands[18] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
static float preamp = 1;
static void *paramsroot;

static int params_changed = 0;
static intptr_t tid = 0;
static uintptr_t mutex = 0;
static int enabled = 0;

static int
supereq_on_configchanged (DB_event_t *ev, uintptr_t data) {
    int e = deadbeef->conf_get_int ("supereq.enable", 0);
    if (e != enabled) {
        if (e) {
            supereq_reset ();
        }
        enabled = e;
    }
    
    return 0;
}

void
recalc_table (void) {
    void *params = paramlist_alloc ();

    deadbeef->mutex_lock (mutex);
    float lbands_copy[18];
    float rbands_copy[18];
    float srate = last_srate;
    memcpy (lbands_copy, lbands, sizeof (lbands));
    memcpy (rbands_copy, rbands, sizeof (rbands));
    for (int i = 0; i < 18; i++) {
        lbands_copy[i] *= preamp;
        rbands_copy[i] *= preamp;
    }
    deadbeef->mutex_unlock (mutex);

    equ_makeTable (lbands_copy, rbands_copy, params, srate);

    deadbeef->mutex_lock (mutex);
    paramlist_free (paramsroot);
    paramsroot = params;
    deadbeef->mutex_unlock (mutex);
}

int
supereq_plugin_start (void) {
    enabled = deadbeef->conf_get_int ("supereq.enable", 0);
    // load bands from config
    preamp = deadbeef->conf_get_float ("eq.preamp", 1);
    for (int i = 0; i < 18; i++) {
        char key[100];
        snprintf (key, sizeof (key), "eq.band%d", i);
        lbands[i] = rbands[i] = deadbeef->conf_get_float (key, 1);
    }

    equ_init (14);
    paramsroot = paramlist_alloc ();
    last_srate = 44100;
    last_nch = 2;
    last_bps = 16;
    mutex = deadbeef->mutex_create ();
    recalc_table ();
    equ_clearbuf (last_bps,last_srate);
    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (supereq_on_configchanged), 0);
    return 0;
}

int
supereq_plugin_stop (void) {
    deadbeef->ev_unsubscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (supereq_on_configchanged), 0);
    if (tid) {
        deadbeef->thread_join (tid);
        tid = 0;
    }
    if (mutex) {
        deadbeef->mutex_free (mutex);
        mutex = 0;
    }
    equ_quit ();
    paramlist_free (paramsroot);
    return 0;
}

void
supereq_regen_table_thread (void *param) {
    recalc_table ();
    tid = 0;
}

int
supereq_process_int16 (int16_t *samples, int nsamples, int nch, int bps, int srate) {
	if ((nch != 1 && nch != 2) || (bps != 8 && bps != 16 && bps != 24)) return nsamples;
    if (params_changed && !tid) {
        tid = deadbeef->thread_start (supereq_regen_table_thread, NULL);
        params_changed = 0;
    }
	if (last_srate != srate) {
        deadbeef->mutex_lock (mutex);
        //equ_makeTable (lbands, rbands, paramsroot, srate);
		last_srate = srate;
		last_nch = nch;
		last_bps = bps;
        recalc_table ();
        deadbeef->mutex_unlock (mutex);
		equ_clearbuf(bps,srate);
    }
	else if (last_nch != nch || last_bps != bps) {
        deadbeef->mutex_lock (mutex);
		last_nch = nch;
		last_bps = bps;
        deadbeef->mutex_unlock (mutex);
		equ_clearbuf(bps,srate);
    }
	equ_modifySamples((char *)samples,nsamples,nch,bps);
	return nsamples;
}

float
supereq_get_band (int band) {
    return lbands[band];
}

void
supereq_set_band (int band, float value) {
    deadbeef->mutex_lock (mutex);
    lbands[band] = rbands[band] = value;
    deadbeef->mutex_unlock (mutex);
    params_changed = 1;
    char key[100];
    snprintf (key, sizeof (key), "eq.band%d", band);
    deadbeef->conf_set_float (key, value);
}

float
supereq_get_preamp (void) {
    return preamp;
}

void
supereq_set_preamp (float value) {
    deadbeef->mutex_lock (mutex);
    preamp = value;
    deadbeef->mutex_unlock (mutex);
    params_changed = 1;
    deadbeef->conf_set_float ("eq.preamp", value);
}

void
supereq_reset (void) {
    deadbeef->mutex_lock (mutex);
    equ_clearbuf(last_bps,last_srate);
    deadbeef->mutex_unlock (mutex);
}

void
supereq_enable (int e) {
    if (e != enabled) {
        deadbeef->conf_set_int ("supereq.enable", e);
        if (e && !enabled) {
            supereq_reset ();
        }
        enabled = e;
    }
}

int
supereq_enabled (void) {
    return enabled;
}

static DB_supereq_dsp_t plugin = {
    .dsp.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .dsp.plugin.api_vminor = DB_API_VERSION_MINOR,
    .dsp.plugin.type = DB_PLUGIN_DSP,
    .dsp.plugin.id = "supereq",
    .dsp.plugin.name = "SuperEQ",
    .dsp.plugin.descr = "equalizer plugin using SuperEQ library by Naoki Shibata",
    .dsp.plugin.author = "Alexey Yakovenko",
    .dsp.plugin.email = "waker@users.sourceforge.net",
    .dsp.plugin.website = "http://deadbeef.sf.net",
    .dsp.plugin.start = supereq_plugin_start,
    .dsp.plugin.stop = supereq_plugin_stop,
    .dsp.process_int16 = supereq_process_int16,
    .dsp.reset = supereq_reset,
    .dsp.enable = supereq_enable,
    .dsp.enabled = supereq_enabled,
    .get_band = supereq_get_band,
    .set_band = supereq_set_band,
    .get_preamp = supereq_get_preamp,
    .set_preamp = supereq_set_preamp,
};

DB_plugin_t *
supereq_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
