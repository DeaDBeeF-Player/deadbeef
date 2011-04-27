/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// based on fb2k dumb plugin from http://kode54.foobar2000.org

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dumb.h"
#include "internal/it.h"
#include "../../deadbeef.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

static DB_decoder_t plugin;
static DB_functions_t *deadbeef;

typedef struct {
    DB_fileinfo_t info;
    DUH *duh;
    DUH_SIGRENDERER *renderer;
} dumb_info_t;

//#define DUMB_RQ_ALIASING
//#define DUMB_RQ_LINEAR
//#define DUMB_RQ_CUBIC
//#define DUMB_RQ_N_LEVELS
extern int dumb_resampling_quality;
extern int dumb_it_max_to_mix;

static int
cdumb_startrenderer (DB_fileinfo_t *_info);

static DUH*
open_module(const char *fname, const char *ext, int *start_order, int *is_it, int *is_dos, const char **filetype);

static DB_fileinfo_t *
cdumb_open (uint32_t hints) {
    DB_fileinfo_t *_info = malloc (sizeof (dumb_info_t));
    memset (_info, 0, sizeof (dumb_info_t));
    return _info;
}

static int
cdumb_init (DB_fileinfo_t *_info, DB_playItem_t *it) {
    trace ("cdumb_init %s\n", deadbeef->pl_find_meta (it, ":URI"));
    dumb_info_t *info = (dumb_info_t *)_info;

    int start_order = 0;
	int is_dos, is_it;
    const char *ext = deadbeef->pl_find_meta (it, ":URI") + strlen (deadbeef->pl_find_meta (it, ":URI")) - 1;
    while (*ext != '.' && ext > deadbeef->pl_find_meta (it, ":URI")) {
        ext--;
    }
    ext++;
    const char *ftype;
    info->duh = open_module(deadbeef->pl_find_meta (it, ":URI"), ext, &start_order, &is_it, &is_dos, &ftype);

    dumb_it_do_initial_runthrough (info->duh);

    _info->plugin = &plugin;
    _info->fmt.bps = deadbeef->conf_get_int ("dumb.8bitoutput", 0) ? 8 : 16;
    _info->fmt.channels = 2;
    _info->fmt.samplerate = deadbeef->conf_get_int ("synth.samplerate", 44100);
    _info->readpos = 0;
    _info->fmt.channelmask = _info->fmt.channels == 1 ? DDB_SPEAKER_FRONT_LEFT : (DDB_SPEAKER_FRONT_LEFT | DDB_SPEAKER_FRONT_RIGHT);

    if (cdumb_startrenderer (_info) < 0) {
        return -1;
    }

    trace ("cdumb_init success (ptr=%p)\n", _info);
    return 0;
}

static int
cdumb_startrenderer (DB_fileinfo_t *_info) {
    dumb_info_t *info = (dumb_info_t *)_info;
    // reopen
    if (info->renderer) {
        duh_end_sigrenderer (info->renderer);
        info->renderer = NULL;
    }
    info->renderer = duh_start_sigrenderer (info->duh, 0, 2, 0);
    if (!info->renderer) {
        return -1;
    }

    DUMB_IT_SIGRENDERER *itsr = duh_get_it_sigrenderer (info->renderer);
    dumb_it_set_loop_callback (itsr, &dumb_it_callback_terminate, NULL);

    int q = deadbeef->conf_get_int ("dumb.resampling_quality", 2);
    if (q < 0) {
        q = 0;
    }
    else if (q >= DUMB_RQ_N_LEVELS) {
        q = DUMB_RQ_N_LEVELS - 1;
    }

    dumb_it_set_resampling_quality (itsr, q);
    dumb_it_set_xm_speed_zero_callback (itsr, &dumb_it_callback_terminate, NULL);
    dumb_it_set_global_volume_zero_callback (itsr, &dumb_it_callback_terminate, NULL);
    return 0;
}

static void
cdumb_free (DB_fileinfo_t *_info) {
    trace ("cdumb_free %p\n", _info);
    dumb_info_t *info = (dumb_info_t *)_info;
    if (info) {
        if (info->renderer) {
            duh_end_sigrenderer (info->renderer);
            info->renderer = NULL;
        }
        if (info->duh) {
            unload_duh (info->duh);
            info->duh = NULL;
        }
        free (info);
    }
}

static int
cdumb_read (DB_fileinfo_t *_info, char *bytes, int size) {
    trace ("cdumb_read req %d\n", size);
    dumb_info_t *info = (dumb_info_t *)_info;
    int samplesize = (_info->fmt.bps >> 3) * _info->fmt.channels;
    int length = size / samplesize;
    long ret;
    ret = duh_render (info->renderer, _info->fmt.bps, 0, 1, 65536.f / _info->fmt.samplerate, length, bytes);
    _info->readpos += ret / (float)_info->fmt.samplerate;
    trace ("cdumb_read %d\n", ret*samplesize);
    return ret*samplesize;
}

static int
cdumb_seek (DB_fileinfo_t *_info, float time) {
    trace ("cdumb_read seek %f\n", time);
    dumb_info_t *info = (dumb_info_t *)_info;
    if (time < _info->readpos) {
        if (cdumb_startrenderer (_info) < 0) {
            return -1;
        }       
    }
    else {
        time -= _info->readpos;
    }
    int pos = time * _info->fmt.samplerate;
    duh_sigrenderer_generate_samples (info->renderer, 0, 65536.0f / _info->fmt.samplerate, pos, NULL);
    _info->readpos = duh_sigrenderer_get_position (info->renderer) / 65536.f;
    return 0;
}

static const char * exts[]=
{
	"mod","mdz",
	"s3m","s3z",
	"stm","stz",
	"it","itz",
	"xm","xmz",
	"ptm","ptz",
	"mtm","mtz",
	"669",
	"psm",
	"umx",
	"am","j2b",
	"dsm",
	"amf",
	NULL
};

// derived from mod.cpp of foo_dumb source code
static DUH * open_module(const char *fname, const char *ext, int *start_order, int *is_it, int *is_dos, const char** filetype)
{
    *filetype = NULL;
	DUH * duh = 0;

	*is_it = 0;
	*is_dos = 1;

    uint8_t ptr[2000];
    DB_FILE *fp = deadbeef->fopen (fname);
    if (!fp) {
        return NULL;
    }
    int size = deadbeef->fread (ptr, 1, 2000, fp);
    deadbeef->fclose (fp);

	DUMBFILE * f = dumbfile_open (fname);
	if (!f) {
        return NULL;
    }

// {{{ no umr yet
#if 0
    if (size >= 4 &&
		ptr[0] == 0xC1 && ptr[1] == 0x83 &&
		ptr[2] == 0x2A && ptr[3] == 0x9E)
	{
		umr_mem_reader memreader(ptr, size);
		umr::upkg pkg;
		if (pkg.open(&memreader))
		{
			for (int i = 1, j = pkg.ocount(); i <= j; i++)
			{
				char * classname = pkg.oclassname(i);
				if (classname && !strcmp(pkg.oclassname(i), "Music"))
				{
					char * type = pkg.otype(i);
					if (!type) continue;
					/*
					if (!stricmp(type, "it"))
					{
					is_it = true;
					ptr += memdata.offset = pkg.object_offset(i);
					size = memdata.size = memdata.offset + pkg.object_size(i);
					duh = dumb_read_it_quick(f);
					break;
					}
					else if (!stricmp(type, "s3m"))
					{
					memdata.offset = pkg.object_offset(i);
					memdata.size = memdata.offset + pkg.object_size(i);
					duh = dumb_read_s3m_quick(f);
					break;
					}
					else if (!stricmp(type, "xm"))
					{
					memdata.offset = pkg.object_offset(i);
					memdata.size = memdata.offset + pkg.object_size(i);
					duh = dumb_read_xm_quick(f);
					break;
					}
					*/
					// blah, type can't be trusted
					if (!stricmp(type, "it") || !stricmp(type, "s3m") || !stricmp(type, "xm"))
					{
						ptr += memdata.offset = pkg.object_offset(i);
						size = memdata.size = memdata.offset + pkg.object_size(i);
						if (size >= 4 && ptr[0] == 'I' && ptr[1] == 'M' && ptr[2] == 'P' && ptr[3] == 'M')
						{
							is_it = true;
							duh = dumb_read_it_quick(f);
						}
						else if (size >= 42 && ptr[38] == 'F' && ptr[39] == 'a' && ptr[40] == 's' && ptr[41] == 't')
						{
							duh = dumb_read_xm_quick(f);
						}
						else if (size >= 48 && ptr[44] == 'S' && ptr[45] == 'C' && ptr[46] == 'R' && ptr[47] == 'M')
						{
							duh = dumb_read_s3m_quick(f);
						}

						break;
					}
				}
			}
		}
	}
	else
#endif
// end of umr code
// }}}
	if (size >= 4 &&
		ptr[0] == 'I' && ptr[1] == 'M' &&
		ptr[2] == 'P' && ptr[3] == 'M')
	{
		*is_it = 1;
		duh = dumb_read_it_quick(f);
		*filetype = "IT";
	}
	else if (size >= 17 && !memcmp(ptr, "Extended Module: ", 17))
	{
		duh = dumb_read_xm_quick(f);
		*filetype = "XM";
	}
	else if (size >= 0x30 &&
		ptr[0x2C] == 'S' && ptr[0x2D] == 'C' &&
		ptr[0x2E] == 'R' && ptr[0x2F] == 'M')
	{
		duh = dumb_read_s3m_quick(f);
		*filetype = "S3M";
	}
	else if (size >= 1168 &&
		/*ptr[28] == 0x1A &&*/ ptr[29] == 2 &&
		( ! strncasecmp( ( const char * ) ptr + 20, "!Scream!", 8 ) ||
		! strncasecmp( ( const char * ) ptr + 20, "BMOD2STM", 8 ) ||
		! strncasecmp( ( const char * ) ptr + 20, "WUZAMOD!", 8 ) ) )
	{
		duh = dumb_read_stm_quick(f);
		*filetype = "STM";
	}
	else if (size >= 2 &&
		((ptr[0] == 0x69 && ptr[1] == 0x66) ||
		(ptr[0] == 0x4A && ptr[1] == 0x4E)))
	{
		duh = dumb_read_669_quick(f);
		*filetype = "669";
	}
	else if (size >= 0x30 &&
		ptr[0x2C] == 'P' && ptr[0x2D] == 'T' &&
		ptr[0x2E] == 'M' && ptr[0x2F] == 'F')
	{
		duh = dumb_read_ptm_quick(f);
		*filetype = "PTM";
	}
	else if (size >= 4 &&
		ptr[0] == 'P' && ptr[1] == 'S' &&
		ptr[2] == 'M' && ptr[3] == ' ')
	{
		duh = dumb_read_psm_quick(f, *start_order);
		*start_order = 0;
		*filetype = "PSM";
	}
	else if (size >= 4 &&
		ptr[0] == 'P' && ptr[1] == 'S' &&
		ptr[2] == 'M' && ptr[3] == 254)
	{
		duh = dumb_read_old_psm_quick(f);
		*filetype = "PSM";
	}
	else if (size >= 3 &&
		ptr[0] == 'M' && ptr[1] == 'T' &&
		ptr[2] == 'M')
	{
		duh = dumb_read_mtm_quick(f);
		*filetype = "MTM";
	}
	else if ( size >= 4 &&
		ptr[0] == 'R' && ptr[1] == 'I' &&
		ptr[2] == 'F' && ptr[3] == 'F')
	{
		duh = dumb_read_riff_quick(f);
		*filetype = "RIFF";
	}
	else if ( size >= 32 &&
		!memcmp( ptr, "ASYLUM Music Format", 19 ) &&
		!memcmp( ptr + 19, " V1.0", 5 ) )
	{
		duh = dumb_read_asy_quick(f);
		*filetype = "ASY";
	}

	if (!duh)
	{
        dumbfile_close(f);
        f = dumbfile_open (fname);
		*is_dos = 0;
		duh = dumb_read_mod_quick (f, (!strcasecmp (ext, exts[0]) || !strcasecmp (ext, exts[1])) ? 0 : 1);
		*filetype = "MOD";
	}

    if (f) {
        dumbfile_close(f);
    }

// {{{ no volume ramping
#if 0
	// XXX test
	if (duh)
	{
		int ramp_mode = 0; // none
		if (ramp_mode)
		{
			DUMB_IT_SIGDATA * itsd = duh_get_it_sigdata(duh);
			if (itsd)
			{
				if (ramp_mode > 2)
				{
					if ( ( itsd->flags & ( IT_WAS_AN_XM | IT_WAS_A_MOD ) ) == IT_WAS_AN_XM )
						ramp_mode = 2;
					else
						ramp_mode = 1;
				}
				for (int i = 0, j = itsd->n_samples; i < j; i++)
				{
					IT_SAMPLE * sample = &itsd->sample[i];
					if ( sample->flags & IT_SAMPLE_EXISTS && !( sample->flags & IT_SAMPLE_LOOP ) )
					{
						double rate = 1. / double( sample->C5_speed );
						double length = double( sample->length ) * rate;
						if ( length >= .1 )
						{
							int k, l = sample->length;
							if ( ramp_mode == 1 && ( ( rate * 16. ) < .01 ) )
							{
								if (sample->flags & IT_SAMPLE_16BIT)
								{
									k = l - 15;
									signed short * data = (signed short *) sample->data;
									if (sample->flags & IT_SAMPLE_STEREO)
									{
										for (int shift = 1; k < l; k++, shift++)
										{
											data [k * 2] >>= shift;
											data [k * 2 + 1] >>= shift;
										}
									}
									else
									{
										for (int shift = 1; k < l; k++, shift++)
										{
											data [k] >>= shift;
										}
									}
								}
								else
								{
									k = l - 7;
									signed char * data = (signed char *) sample->data;
									if (sample->flags & IT_SAMPLE_STEREO)
									{
										for (int shift = 1; k < l; k++, shift++)
										{
											data [k * 2] >>= shift;
											data [k * 2 + 1] >>= shift;
										}
									}
									else
									{
										for (int shift = 1; k < l; k++, shift++)
										{
											data [k] >>= shift;
										}
									}
								}
							}
							else
							{
								int m = int( .01 * double( sample->C5_speed ) + .5 );
								k = l - m;
								if (sample->flags & IT_SAMPLE_16BIT)
								{
									signed short * data = (signed short *) sample->data;
									if (sample->flags & IT_SAMPLE_STEREO)
									{
										for (; k < l; k++)
										{
											data [k * 2] =     MulDiv( data [k * 2],     l - k, m );
											data [k * 2 + 1] = MulDiv( data [k * 2 + 1], l - k, m );
										}
									}
									else
									{
										for (; k < l; k++)
										{
											data [k] =     MulDiv( data [k],     l - k, m );
										}
									}
								}
								else
								{
									signed char * data = (signed char *) sample->data;
									if (sample->flags & IT_SAMPLE_STEREO)
									{
										for (; k < l; k++)
										{
											data [k * 2] =     MulDiv( data [k * 2],     l - k, m );
											data [k * 2 + 1] = MulDiv( data [k * 2 + 1], l - k, m );
										}
									}
									else
									{
										for (; k < l; k++)
										{
											data [k] =     MulDiv( data [k],     l - k, m );
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
#endif
// }}}

// {{{ no autochip
#if 0
	if (duh && cfg_autochip)
	{
		int size_force = cfg_autochip_size_force;
		int size_scan = cfg_autochip_size_scan;
		int scan_threshold_8 = ((cfg_autochip_scan_threshold * 0x100) + 50) / 100;
		int scan_threshold_16 = ((cfg_autochip_scan_threshold * 0x10000) + 50) / 100;
		DUMB_IT_SIGDATA * itsd = duh_get_it_sigdata(duh);

		if (itsd)
		{
			for (int i = 0, j = itsd->n_samples; i < j; i++)
			{
				IT_SAMPLE * sample = &itsd->sample[i];
				if (sample->flags & IT_SAMPLE_EXISTS)
				{
					int channels = sample->flags & IT_SAMPLE_STEREO ? 2 : 1;
					if (sample->length < size_force) sample->max_resampling_quality = 0;
					else if (sample->length < size_scan)
					{
						if ((sample->flags & (IT_SAMPLE_LOOP|IT_SAMPLE_PINGPONG_LOOP)) == IT_SAMPLE_LOOP)
						{
							int loop_start = sample->loop_start * channels;
							int loop_end = sample->loop_end * channels;
							int s1, s2;
							if (sample->flags & IT_SAMPLE_16BIT)
							{
								s1 = ((signed short *)sample->data)[loop_start];
								s2 = ((signed short *)sample->data)[loop_end - channels];
								if (abs(s1 - s2) > scan_threshold_16)
								{
									sample->max_resampling_quality = 0;
									continue;
								}
								if (channels == 2)
								{
									s1 = ((signed short *)sample->data)[loop_start + 1];
									s2 = ((signed short *)sample->data)[loop_end - 1];
									if (abs(s1 - s2) > scan_threshold_16)
									{
										sample->max_resampling_quality = 0;
										continue;
									}
								}
							}
							else
							{
								s1 = ((signed char *)sample->data)[loop_start];
								s2 = ((signed char *)sample->data)[loop_end - channels];
								if (abs(s1 - s2) > scan_threshold_8)
								{
									sample->max_resampling_quality = 0;
									continue;
								}
								if (channels == 2)
								{
									s1 = ((signed char *)sample->data)[loop_start + 1];
									s2 = ((signed char *)sample->data)[loop_end - 1];
									if (abs(s1 - s2) > scan_threshold_8)
									{
										sample->max_resampling_quality = 0;
										continue;
									}
								}
							}
						}
						if ((sample->flags & (IT_SAMPLE_SUS_LOOP|IT_SAMPLE_PINGPONG_SUS_LOOP)) == IT_SAMPLE_SUS_LOOP)
						{
							int sus_loop_start = sample->sus_loop_start * channels;
							int sus_loop_end = sample->sus_loop_end * channels;
							int s1, s2;
							if (sample->flags & IT_SAMPLE_16BIT)
							{
								s1 = ((signed short *)sample->data)[sus_loop_start];
								s2 = ((signed short *)sample->data)[sus_loop_end - channels];
								if (abs(s1 - s2) > scan_threshold_16)
								{
									sample->max_resampling_quality = 0;
									continue;
								}
								if (channels == 2)
								{
									s1 = ((signed short *)sample->data)[sus_loop_start + 1];
									s2 = ((signed short *)sample->data)[sus_loop_end - 1];
									if (abs(s1 - s2) > scan_threshold_16)
									{
										sample->max_resampling_quality = 0;
										continue;
									}
								}
							}
							else
							{
								s1 = ((signed char *)sample->data)[sus_loop_start];
								s2 = ((signed char *)sample->data)[sus_loop_end - channels];
								if (abs(s1 - s2) > scan_threshold_8)
								{
									sample->max_resampling_quality = 0;
									continue;
								}
								if (channels == 2)
								{
									s1 = ((signed char *)sample->data)[sus_loop_start + 1];
									s2 = ((signed char *)sample->data)[sus_loop_end - 1];
									if (abs(s1 - s2) > scan_threshold_8)
									{
										sample->max_resampling_quality = 0;
										continue;
									}
								}
							}
						}

						int k, l = sample->length * channels;
						if (sample->flags & IT_SAMPLE_LOOP) l = sample->loop_end * channels;
						if (sample->flags & IT_SAMPLE_16BIT)
						{
							for (k = channels; k < l; k += channels)
							{
								if (abs(((signed short *)sample->data)[k - channels] - ((signed short *)sample->data)[k]) > scan_threshold_16)
								{
									break;
								}
							}
							if (k < l)
							{
								sample->max_resampling_quality = 0;
								continue;
							}
							if (channels == 2)
							{
								for (k = 2 + 1; k < l; k += 2)
								{
									if (abs(((signed short *)sample->data)[k - 2] - ((signed short *)sample->data)[k]) > scan_threshold_16)
									{
										break;
									}
								}
							}
							if (k < l)
							{
								sample->max_resampling_quality = 0;
								continue;
							}
						}
						else
						{
							for (k = channels; k < l; k += channels)
							{
								if (abs(((signed char *)sample->data)[k - channels] - ((signed char *)sample->data)[k]) > scan_threshold_8)
								{
									break;
								}
							}
							if (k < l)
							{
								sample->max_resampling_quality = 0;
								continue;
							}
							if (channels == 2)
							{
								for (k = 2 + 1; k < l; k += 2)
								{
									if (abs(((signed char *)sample->data)[k - 2] - ((signed char *)sample->data)[k]) > scan_threshold_8)
									{
										break;
									}
								}
							}
							if (k < l)
							{
								sample->max_resampling_quality = 0;
								continue;
							}
						}
					}
				}
			}
		}
	}
#endif
// }}}

// {{{ no trim
#if 0
	if ( duh && cfg_trim )
	{
		if ( dumb_it_trim_silent_patterns( duh ) < 0 )
		{
			unload_duh( duh );
			duh = 0;
		}
	}
#endif
// }}}

	return duh;
}

static const char *
convstr (const char* str, int sz, char *out, int out_sz) {
    int i;
    for (i = 0; i < sz; i++) {
        if (str[i] != ' ') {
            break;
        }
    }
    if (i == sz) {
        out[0] = 0;
        return out;
    }

    // check for utf8 (hack)
    if (deadbeef->junk_iconv (str, sz, out, out_sz, "utf-8", "utf-8") >= 0) {
        return out;
    }

    if (deadbeef->junk_iconv (str, sz, out, out_sz, "cp1252", "utf-8") >= 0) {
        return out;
    }

    trace ("cdumb: failed to detect charset\n");
    return NULL;
}

static void
read_metadata_internal (DB_playItem_t *it, DUMB_IT_SIGDATA *itsd) {
    char temp[2048];

    if (itsd->name[0])     {
        int tl = sizeof(itsd->name);
        int i;
        for (i = 0; i < tl && itsd->name[i] && itsd->name[i] == ' '; i++);
        if (i == tl || !itsd->name[i]) {
            deadbeef->pl_add_meta (it, "title", NULL);
        }
        else {
            deadbeef->pl_add_meta (it, "title", convstr ((char*)&itsd->name, sizeof(itsd->name), temp, sizeof (temp)));
        }
    }
    else {
        deadbeef->pl_add_meta (it, "title", NULL);
    }
    int i;
    for (i = 0; i < itsd->n_instruments; i++) {
        char key[100];
        snprintf (key, sizeof (key), "INST%03d", i);
        deadbeef->pl_add_meta (it, key, convstr ((char *)&itsd->instrument[i].name, sizeof (itsd->instrument[i].name), temp, sizeof (temp)));
    }
    for (i = 0; i < itsd->n_samples; i++) {
        char key[100];
        snprintf (key, sizeof (key), "SAMP%03d", i);
        deadbeef->pl_add_meta (it, key, convstr ((char *)&itsd->sample[i].name, sizeof (itsd->sample[i].name), temp, sizeof (temp)));
    }

    char s[100];

    snprintf (s, sizeof (s), "%d", itsd->n_orders);
    deadbeef->pl_add_meta (it, ":MOD_ORDERS", s);
    snprintf (s, sizeof (s), "%d", itsd->n_instruments);
    deadbeef->pl_add_meta (it, ":MOD_INSTRUMENTS", s);
    snprintf (s, sizeof (s), "%d", itsd->n_samples);
    deadbeef->pl_add_meta (it, ":MOD_SAMPLES", s);
    snprintf (s, sizeof (s), "%d", itsd->n_patterns);
    deadbeef->pl_add_meta (it, ":MOD_PATTERNS", s);
    snprintf (s, sizeof (s), "%d", itsd->n_pchannels);
    deadbeef->pl_add_meta (it, ":MOD_CHANNELS", s);
}

static int
cdumb_read_metadata (DB_playItem_t *it) {
    const char *fname = deadbeef->pl_find_meta (it, ":URI");
    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }
    else {
        ext = "";
    }
    int start_order = 0;
    int is_it;
    int is_dos;
    const char *ftype;
    DUH* duh = open_module(fname, ext, &start_order, &is_it, &is_dos, &ftype);
    if (!duh) {
        unload_duh (duh);
        return -1;
    }
    DUMB_IT_SIGDATA * itsd = duh_get_it_sigdata(duh);

    deadbeef->pl_delete_all_meta (it);
    read_metadata_internal (it, itsd);
    unload_duh (duh);
    return 0;
}

static DB_playItem_t *
cdumb_insert (DB_playItem_t *after, const char *fname) {
    const char *ext = strrchr (fname, '.');
    if (ext) {
        ext++;
    }
    else {
        ext = "";
    }
    int start_order = 0;
    int is_it;
    int is_dos;
    const char *ftype;
    DUH* duh = open_module(fname, ext, &start_order, &is_it, &is_dos, &ftype);
    if (!duh) {
        return NULL;
    }
    DB_playItem_t *it = deadbeef->pl_item_alloc_init (fname, plugin.plugin.id);
    DUMB_IT_SIGDATA * itsd = duh_get_it_sigdata(duh);

    read_metadata_internal (it, itsd);

    dumb_it_do_initial_runthrough (duh);
    deadbeef->pl_set_item_duration (it, duh_get_length (duh)/65536.0f);
    deadbeef->pl_add_meta (it, ":FILETYPE", ftype);
//    printf ("duration: %f\n", _info->duration);
    after = deadbeef->pl_insert_item (after, it);
    deadbeef->pl_item_unref (it);
    unload_duh (duh);

    return after;
}

static DUMBFILE_SYSTEM dumb_vfs;

static int
dumb_vfs_skip (void *f, long n) {
    return deadbeef->fseek (f, n, SEEK_CUR);
}

static int
dumb_vfs_getc (void *f) {
    uint8_t c;
    deadbeef->fread (&c, 1, 1, f);
    return (int)c;
}

static long
dumb_vfs_getnc (char *ptr, long n, void *f) {
    return deadbeef->fread (ptr, 1, n, f);
}

static void
dumb_vfs_close (void *f) {
    deadbeef->fclose (f);
}

static void
dumb_register_db_vfs (void) {
    dumb_vfs.open = (void *(*)(const char *))deadbeef->fopen;
    dumb_vfs.skip = dumb_vfs_skip;
    dumb_vfs.getc = dumb_vfs_getc;
    dumb_vfs.getnc = dumb_vfs_getnc;
    dumb_vfs.close = dumb_vfs_close;
    register_dumbfile_system (&dumb_vfs);
}

int
cgme_start (void) {
    dumb_register_db_vfs ();
    return 0;
}

int
cgme_stop (void) {
    dumb_exit ();
    return 0;
}

static const char settings_dlg[] =
    "property \"Resampling quality (0..2, higher is better)\" entry dumb.resampling_quality 2;\n"
    "property \"8-bit output (default is 16)\" checkbox dumb.8bitoutput 0;\n"
;

// define plugin interface
static DB_decoder_t plugin = {
    DB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_DECODER,
    .plugin.id = "stddumb",
    .plugin.name = "DUMB module player",
    .plugin.descr = "module player based on DUMB library",
    .plugin.copyright = 
        "Copyright (C) 2009-2011 Alexey Yakovenko <waker@users.sourceforge.net>\n"
        "\n"
        "Uses a fork of DUMB (Dynamic Universal Music Bibliotheque), Version 0.9.3\n"
        "Copyright (C) 2001-2005 Ben Davis, Robert J Ohannessian and Julien Cugniere\n"
        "Uses code from kode54's foobar2000 plugin, http://kode54.foobar2000.org/\n"
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
    .plugin.start = cgme_start,
    .plugin.stop = cgme_stop,
    .plugin.configdialog = settings_dlg,
    .open = cdumb_open,
    .init = cdumb_init,
    .free = cdumb_free,
    .read = cdumb_read,
    .seek = cdumb_seek,
    .insert = cdumb_insert,
    .read_metadata = cdumb_read_metadata,
    .exts = exts,
};

DB_plugin_t *
dumb_load (DB_functions_t *api) {
    deadbeef = api;
    return DB_PLUGIN (&plugin);
}
