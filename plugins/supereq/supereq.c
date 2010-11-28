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
#include "supereq.h"
#include "Equ.h"

static DB_functions_t *deadbeef;
static DB_supereq_dsp_t plugin;

typedef struct {
    DB_dsp_instance_t inst;
    float last_srate;
    int last_nch;
    float lbands[18];
    float rbands[18];
    float preamp;
    void *paramsroot;
    int params_changed;
    intptr_t tid;
    uintptr_t mutex;
    SuperEqState state;
} ddb_supereq_instance_t;

static float
dsp_conf_get_float (DB_dsp_instance_t *inst, const char *name, float def) {
    char key[100];
    snprintf (key, sizeof (key), "%s.%s", inst->id, name);
    return deadbeef->conf_get_float (key, def);
}

static int
dsp_conf_get_int (DB_dsp_instance_t *inst, const char *name, int def) {
    char key[100];
    snprintf (key, sizeof (key), "%s.%s", inst->id, name);
    return deadbeef->conf_get_int (key, def);
}

static void
dsp_conf_set_float (DB_dsp_instance_t *inst, const char *name, float value) {
    char key[100];
    snprintf (key, sizeof (key), "%s.%s", inst->id, name);
    return deadbeef->conf_set_float (key, value);
}

static void
dsp_conf_set_int (DB_dsp_instance_t *inst, const char *name, int value) {
    char key[100];
    snprintf (key, sizeof (key), "%s.%s", inst->id, name);
    return deadbeef->conf_set_int (key, value);
}

void supereq_reset (DB_dsp_instance_t *inst);

#if 0
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
#endif

void
recalc_table (ddb_supereq_instance_t *eq) {
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
#if 0
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
    mutex = deadbeef->mutex_create ();
    recalc_table ();
    equ_clearbuf ();
//    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (supereq_on_configchanged), 0);
#endif
    return 0;
}

int
supereq_plugin_stop (void) {
#if 0
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
#endif
    return 0;
}

void
supereq_regen_table_thread (void *param) {
    ddb_supereq_instance_t *inst = param;
    recalc_table (inst);
    inst->tid = 0;
}

int
supereq_process (DB_dsp_instance_t *inst, float *samples, int frames, int samplerate, int channels) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    if (supereq->params_changed && !supereq->tid) {
        supereq->tid = deadbeef->thread_start (supereq_regen_table_thread, inst);
        supereq->params_changed = 0;
    }
	if (supereq->last_srate != samplerate) {
        deadbeef->mutex_lock (supereq->mutex);
        //equ_makeTable (lbands, rbands, paramsroot, srate);
		supereq->last_srate = samplerate;
		supereq->last_nch = channels;
        recalc_table ((ddb_supereq_instance_t *)inst);
        deadbeef->mutex_unlock (supereq->mutex);
		equ_clearbuf(&supereq->state);
    }
	else if (supereq->last_nch != channels) {
        deadbeef->mutex_lock (supereq->mutex);
		supereq->last_nch = channels;
        deadbeef->mutex_unlock (supereq->mutex);
		equ_clearbuf(&supereq->state);
    }
	equ_modifySamples_float(&supereq->state, (char *)samples,frames,channels);
	return frames;
}

float
supereq_get_band (DB_dsp_instance_t *inst, int band) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    return supereq->lbands[band];
}

void
supereq_set_band (DB_dsp_instance_t *inst, int band, float value) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    deadbeef->mutex_lock (supereq->mutex);
    supereq->lbands[band] = supereq->rbands[band] = value;
    deadbeef->mutex_unlock (supereq->mutex);
    supereq->params_changed = 1;
    char key[100];
    snprintf (key, sizeof (key), "band%d", band);
    dsp_conf_set_float (inst, key, value);
}

float
supereq_get_preamp (DB_dsp_instance_t *inst) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    return supereq->preamp;
}

void
supereq_set_preamp (DB_dsp_instance_t *inst, float value) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    deadbeef->mutex_lock (supereq->mutex);
    supereq->preamp = value;
    deadbeef->mutex_unlock (supereq->mutex);
    supereq->params_changed = 1;
    dsp_conf_set_float (inst, "preamp", value);
}

void
supereq_reset (DB_dsp_instance_t *inst) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    deadbeef->mutex_lock (supereq->mutex);
    equ_clearbuf(&supereq->state);
    deadbeef->mutex_unlock (supereq->mutex);
}

void
supereq_enable (DB_dsp_instance_t *inst, int e) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    if (e != supereq->inst.enabled) {
        dsp_conf_set_int (inst, "enabled", e);
        if (e && !supereq->inst.enabled) {
            supereq_reset (inst);
        }
        supereq->inst.enabled = e;
    }
}

DB_dsp_instance_t*
supereq_open (const char *id) {
    ddb_supereq_instance_t *supereq = malloc (sizeof (ddb_supereq_instance_t));
    DDB_INIT_DSP_INSTANCE (supereq,ddb_supereq_instance_t,&plugin.dsp);

    supereq->preamp = dsp_conf_get_float (&supereq->inst, "preamp", 1);
    for (int i = 0; i < 18; i++) {
        char key[100];
        snprintf (key, sizeof (key), "band%d", i);
        supereq->lbands[i] = supereq->rbands[i] = dsp_conf_get_float (&supereq->inst, key, 1);
    }

    supereq->inst.enabled = dsp_conf_get_int (&supereq->inst, "enabled", 0);

    equ_init (&supereq->state, 14);
    supereq->paramsroot = paramlist_alloc ();
    supereq->last_srate = 44100;
    supereq->last_nch = 2;
    supereq->mutex = deadbeef->mutex_create ();
    recalc_table (supereq);
    equ_clearbuf (&supereq->state);
//    deadbeef->ev_subscribe (DB_PLUGIN (&plugin), DB_EV_CONFIGCHANGED, DB_CALLBACK (supereq_on_configchanged), 0);


    return (DB_dsp_instance_t*)supereq;
}

void
supereq_close (DB_dsp_instance_t *inst) {
    ddb_supereq_instance_t *supereq = (ddb_supereq_instance_t *)inst;
    if (supereq->tid) {
        deadbeef->thread_join (supereq->tid);
        supereq->tid = 0;
    }
    if (supereq->mutex) {
        deadbeef->mutex_free (supereq->mutex);
        supereq->mutex = 0;
    }
    equ_quit (&supereq->state);
    paramlist_free (supereq->paramsroot);
    free (inst);
}

static DB_supereq_dsp_t plugin = {
    .dsp.plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .dsp.plugin.api_vminor = DB_API_VERSION_MINOR,
    .dsp.plugin.version_major = 1,
    .dsp.plugin.version_minor = 0,
    .dsp.plugin.type = DB_PLUGIN_DSP,
    .dsp.plugin.id = "supereq",
    .dsp.plugin.name = "SuperEQ",
    .dsp.plugin.descr = "equalizer plugin using SuperEQ library by Naoki Shibata",
    .dsp.plugin.author = "Alexey Yakovenko",
    .dsp.plugin.email = "waker@users.sourceforge.net",
    .dsp.plugin.website = "http://deadbeef.sf.net",
    .dsp.plugin.start = supereq_plugin_start,
    .dsp.plugin.stop = supereq_plugin_stop,
    .dsp.open = supereq_open,
    .dsp.close = supereq_close,
    .dsp.process = supereq_process,
    .dsp.reset = supereq_reset,
    .dsp.enable = supereq_enable,
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
