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
#include "../../deadbeef.h"
#include "supereq.h"

static DB_functions_t *deadbeef;

void *paramlist_alloc (void);
void paramlist_free (void *);
void equ_makeTable(float *lbc,float *rbc,void *param,float fs);
int equ_modifySamples(char *buf,int nsamples,int nch,int bps);
void equ_clearbuf(int bps,int srate);
void equ_init(int wb);
void equ_quit(void);

float last_srate = 0;
int last_nch = 0, last_bps = 0;
float lbands[18] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
float rbands[18] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
void *paramsroot;

int
supereq_plugin_start (void) {
    equ_init (14);
    paramsroot = paramlist_alloc ();
    return 0;
}

int
supereq_plugin_stop (void) {
    equ_quit ();
    paramlist_free (paramsroot);
    return 0;
}


int
supereq_process_int16 (int16_t *samples, int nsamples, int nch, int bps, int srate) {
	if ((nch != 1 && nch != 2) || (bps != 8 && bps != 16 && bps != 24)) return nsamples;
	if (last_srate != srate) {
        equ_makeTable (lbands, rbands, paramsroot, srate);
		last_srate = srate;
		last_nch = nch;
		last_bps = bps;
		equ_clearbuf(bps,srate);
    }
	else if (last_nch != nch || last_bps != bps) {
		last_nch = nch;
		last_bps = bps;
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
    last_srate = 0;
    lbands[band] = rbands[band] = value;
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
//  .dsp.plugin.configdialog = settings_dlg,
    .dsp.process_int16 = supereq_process_int16,
    .get_band = supereq_get_band,
    .set_band = supereq_set_band,
};

DB_plugin_t *
supereq_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
