/*
 *                                sc68 - API
 *         Copyright (C) 2001 Ben(jamin) Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <config68.h>

#include <string.h>

#include "api68/api68.h"
#include "api68/mixer68.h"
#include "api68/conf68.h"
#include "emu68/emu68.h"
#include "file68/error68.h"
#include "file68/string68.h"
#include "file68/alloc68.h"
#include "file68/file68.h"
#include "file68/rsc68.h"
#include "file68/debugmsg68.h"

#include "emu68/emu68.h"
#include "emu68/ioplug68.h"
#include "io68/io68.h"

#define TRAP_14_ADDR 0x600
/* ST xbios function emulator.#
 *  Just 4 SUPEREXEC in LOGICAL & some other music files
 */
static u8 trap14[] = {
  0x0c,0x6f,0x00,0x26,0x00,0x06,0x67,0x00,0x00,0x22,0x0c,0x6f,0x00,0x1f,0x00,
  0x06,0x67,0x00,0x00,0x28,0x0c,0x6f,0x00,0x22,0x00,0x06,0x67,0x00,0x00,0x86,
  0x0c,0x6f,0x00,0x0e,0x00,0x06,0x67,0x00,0x00,0xec,0x4e,0x73,0x48,0xe7,0xff,
  0xfe,0x20,0x6f,0x00,0x44,0x4e,0x90,0x4c,0xdf,0x7f,0xff,0x4e,0x73,0x48,0xe7,
  0xff,0xfe,0x41,0xef,0x00,0x44,0x4c,0x98,0x00,0x07,0x02,0x40,0x00,0x03,0xd0,
  0x40,0x16,0x38,0xfa,0x17,0x02,0x43,0x00,0xf0,0xe5,0x4b,0x36,0x7b,0x00,0x42,
  0xd6,0xc3,0x76,0x00,0x45,0xf8,0xfa,0x1f,0xd4,0xc0,0x43,0xf8,0xfa,0x19,0xd2,
  0xc0,0xe2,0x48,0x04,0x00,0x00,0x02,0x6b,0x1a,0x57,0xc3,0x18,0x03,0x0a,0x03,
  0x00,0xf0,0x44,0x04,0x48,0x84,0xd8,0x44,0x43,0xf1,0x40,0xfe,0xd8,0x44,0x02,
  0x41,0x00,0x0f,0xe9,0x69,0xc7,0x11,0x14,0x82,0x26,0x90,0x83,0x11,0x4c,0xdf,
  0x7f,0xff,0x4e,0x73,0x00,0x34,0x00,0x20,0x00,0x14,0x00,0x10,0x48,0xe7,0x00,
  0xc0,0x43,0xfa,0x00,0x20,0x70,0x00,0x20,0x7b,0x00,0x3e,0x41,0xfb,0x80,0x5e,
  0x23,0x88,0x00,0x00,0x58,0x40,0xb0,0x7c,0x00,0x24,0x66,0xec,0x20,0x09,0x4c,
  0xdf,0x03,0x00,0x4e,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x4e,0x75,0xc1,0x88,0x41,0xfa,0x00,0x0c,0x21,0x48,0x00,0x04,0x58,
  0x48,0xc1,0x88,0x4e,0x73,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
};


struct _api68_s {
  int          version;    /**< sc68 version.                      */
  reg68_t      reg;        /**< 68k registers.                     */
  disk68_t    *disk;       /**< Current loaded disk.               */
  music68_t   *mus;        /**< Current playing music.             */
  int          track;      /**< Current playing track.             */
  int          track_to;   /**< Track to set.                      */
  int          track_here; /**< Force first track here.            */
  unsigned int playaddr;   /**< Current play address in 68 memory. */
  SC68config_t config;     /**< Config.                            */

  /** Playing time info. */
  struct {
    unsigned int def_ms;     /**< default time in ms.             */
    unsigned int elapsed;    /**< Current track elapsed time.     */
    unsigned int rem;        /**< Error diffusion (cycle to sec). */
    unsigned int elapsed_ms; /**< Current track elapsed ms.       */
    unsigned int rem_ms;     /**< Error diffusion (cycle to ms).  */
  } time;

  /** Mixer info struture. */
  struct
  {
    unsigned int   rate;         /**< Sampling rate in hz.              */
    unsigned int * buffer;       /**< Current PCM position.             */
    int            buflen;       /**< PCM count in buffer.              */
    int            stdbuflen;    /**< Default number of PCM per pass.   */
    unsigned int   cycleperpass; /**< Number of 68K cycles per pass.    */
    int            amiga_blend;  /**< Amiga LR blend factor [0..65536]. */
    unsigned int   sample_cnt;   /**< Number of mixed PCM.              */
    unsigned int   pass_cnt;     /**< Current pass.                     */
    unsigned int   pass_total;   /**< Total number of pass.             */
  } mix;

};

/* Currently only one API is possible. */
static api68_t * api68;

static int stream_read_68k(unsigned int dest, istream_t * is, unsigned int sz)
{
  if (EMU68_memvalid(dest, sz)) {
    return SC68error_add("68Kread_stream() : %s",EMU68error_get());
  }
  return (istream_read(is, reg68.mem+dest, sz) == sz) ? 0 : -1;
}

static int init68k(api68_t * api, const int mem68_size)
{
  u8 * mem = 0;
  int alloc_size;

  api68_debug("init_68k() : enter\n");

  /* Calculate 68k memory buffer size. See EMU68_init() doc for more info. */
  alloc_size = mem68_size;

  if (EMU68_debugmode()) {
    api68_debug("init_68k() : debug mode detected\n");
    alloc_size <<= 1;
  } else {
    api68_debug("init_68k() : fast mode detected\n");
    alloc_size += 3;
  }

  /* Allocate 68k memory buffer. */
  api68_debug("init_68k() : alloc 68 memory (%dkb)\n", alloc_size >> 10);
  mem = SC68alloc(alloc_size);
  if (!mem) {
    goto error;
  }

  /* Do initialization. */
  api68_debug("init_68k() : 68K emulator init\n");
  if (EMU68_init(mem, mem68_size) < 0) {
    SC68error_add(EMU68error_get());
    goto error;
  }

  api->reg = reg68;
  api->reg.sr = 0x2000;
  api->reg.a[7] = mem68_size - 4;

  /* Initialize chipset */
  api68_debug("init_68k() : chipset init\n");

  if (YM_init() < 0) {
    SC68error_add("Yamaha 2149 emulator init failed");
    goto error;
  }

  if (MW_init() < 0) {
    SC68error_add("Microwire emulator init failed");
    goto error;
  }

  if (MFP_init() < 0) {
    SC68error_add("MFP emulator init failed");
    goto error;
  }

  if (PL_init() < 0) {
    SC68error_add("Paula emulator init failed");
    goto error;
  }

  api68_debug("init_68k() : Success\n");
  return 0;

 error:
  api68_debug("init_68k() : Failed\n");

  SC68free(mem);
  return -1;
}

static void set_config(api68_t * api)
{
  int idx;

  if (idx = SC68config_get_id("version"), idx >= 0) {
    api->version = api->config[idx];
  } else {
    api->version = VERSION68_NUM;
  }


  if (idx = SC68config_get_id("amiga_blend"), idx >= 0) {
    api->mix.amiga_blend = api->config[idx];
  } else {
    api->mix.amiga_blend = 0x4000;
  }

  if (idx = SC68config_get_id("force_track"), idx >= 0) {
    api->track_here = api->config[idx];
  } else {
    api->track_here = 1;
  }

  if (idx = SC68config_get_id("default_time"), idx >= 0) {
    api->time.def_ms = api->config[idx] * 1000u;
  } else {
    api->time.def_ms = 3 * 60 * 1000u;
  }

  if (idx = SC68config_get_id("sampling_rate"), idx >= 0) {
    api->mix.rate = api->config[idx];
  } else {
    api->mix.rate = 44100;
  }

}

static void get_config(api68_t * api)
{
  int idx;

  if (idx = SC68config_get_id("version"), idx >= 0) {
    api->config[idx] = VERSION68_NUM;
  }

  if (idx = SC68config_get_id("amiga_blend"), idx >= 0) {
    api->config[idx] = api->mix.amiga_blend;
  }

  if (idx = SC68config_get_id("force_track"), idx >= 0) {
    api->config[idx] = api->track_here;
  }

  if (idx = SC68config_get_id("default_time"), idx >= 0) {
    api->config[idx] = (api->time.def_ms+999u) / 1000u;
  }

  if (idx = SC68config_get_id("sampling_rate"), idx >= 0) {
    api->config[idx] = api->mix.rate;
  }
}

int api68_config_load(api68_t * api)
{
  int err = -1;
  if (api) {
    err = SC68config_load(&api->config);
    set_config(api);
  }
  return err;
}

int api68_config_save(api68_t * api)
{
  int err = -1;
  if (api) {
    get_config(api);
    err = SC68config_save(&api->config);
  }
  return err;
}

int api68_config_id(api68_t * api, const char * name)
{
  return SC68config_get_id(name);
}

int api68_config_get(api68_t * api, int idx, int * v)
{
  const unsigned int idx_max = sizeof(api->config)/sizeof(*api->config);
  int err = -1;
  if (api && v && (unsigned int) idx < idx_max) {
    *v = api->config[idx];
    err = 0;
  }
  return err;
}

int api68_config_set(api68_t * api, int idx, int v)
{
  const unsigned int idx_max = sizeof(api->config)/sizeof(*api->config);
  int err = -1;
  if (api && (unsigned int) idx < idx_max) {
    api->config[idx] = v;
    err = 0;
  }
  return err;
}

void api68_config_apply(api68_t * api)
{
  if (api) {
    SC68config_valid(&api->config);
    set_config(api);
  }
}


api68_t * api68_init(api68_init_t * init)
{
  const int mem68_size = (512<<10); /* $$$ should be retrieve form EMU68. */
  api68_t *api = 0;

  /* First things to do to have some debug messages. */
  if (init) {
    /* Set debug handler. */
    debugmsg68_set_handler(init->debug);
    debugmsg68_set_cookie(init->debug_cookie);
  }

  api68_debug("api68_init(): enter\n");

  if (api68) {
    api68_debug("api68_init(): already initialized\n");
    return 0;
  }

  if (!init) {
    SC68error_add("api68_init() : missing init struct");
    return 0;
  }

  /* Set dynamic memory handler. */
  if (!init->alloc || !init->free) {
    SC68error_add("api68_init() : missing dynamic memory handler(s)");
    return 0;
  }
  SC68set_alloc(init->alloc);
  SC68set_free(init->free);

  /* Set resource pathes. */
  if (init->user_path) {
    api68_debug("api68_init(): user resource [%s]\n", init->user_path);
    SC68rsc_set_user(init->user_path);
  }
  if (init->shared_path) {
    api68_debug("api68_init(): shared resource [%s]\n", init->shared_path);
    SC68rsc_set_share(init->shared_path);
  }
  SC68rsc_get_path(&init->shared_path, &init->user_path);

  /* Alloc API struct. */
  api = SC68alloc(sizeof(api68_t));
  if (!api) {
    goto error;
  }
  memset(api, 0, sizeof(*api));

  /* Load config file */
  api68_config_load(api);

  /* Override config. */
  if (init->sampling_rate) {
    api->mix.rate = init->sampling_rate;
  }
  if (!api->mix.rate) {
    api->mix.rate = SAMPLING_RATE_DEF;
  }
  init->sampling_rate = api68_sampling_rate(api, api->mix.rate);

  if (init68k(api, mem68_size)) {
    goto error;
  }
    
  api68 = api;
  api68_debug("api68_init(): Success\n");
  return api;

 error:
  api68_shutdown(api);
  api68_debug("api68_init(): Failed\n");
  return 0;
}

void api68_shutdown(api68_t * api)
{
  u8 * mem = reg68.mem;
  api68_debug("api68_shutdown(): enter\n");

  api68_config_save(api);

  SC68rsc_set_user(0);
  SC68rsc_set_share(0);

  EMU68_kill();
  SC68free(mem);
  SC68free(api);

  api68_debug("api68_shutdown(): leave\n");
  api68 = 0;
}

unsigned int api68_sampling_rate(api68_t * api, unsigned int f)
{
  if (!f) {
    f = api ? api->mix.rate : MW_sampling_rate(0);
  } else {
    f = YM_sampling_rate(f);
    PL_sampling_rate(f);
    MW_sampling_rate(f);
    if (api) {
      api->mix.rate = f;
    }
  }
  api68_debug("api68_sampling_rate() : %u hz\n", f);
  return f;
}

void api68_set_share(api68_t * api, const char * path)
{
  SC68rsc_set_share(path);
}

void api68_set_user(api68_t * api, const char * path)
{
  SC68rsc_set_user(path);
}

/** Start current music of current disk.
 *
 *  @ingroup SC68app
 *
 */
static int apply_change_track(api68_t * api)
{
  u32 a0;
  disk68_t * d;
  music68_t * m;
  int track;

  if (!api || !api->disk) {
    return API68_MIX_ERROR;
  }
  if (track = api->track_to, !track) {
    return API68_MIX_OK;
  }

  api->track_to = 0;

  /* -1 : stop */
  if (track == -1) {
    api68_debug("apply_change_track() : stop\n");
    api->mus = 0;
    api->track = 0;
    return API68_END | API68_CHANGE;
  }

  api68_debug("apply_change_track(%d) : enter\n", track);

  d = api->disk;
  if (track < 1 || track > d->nb_six) {
    SC68error_add("track [%d] out of range [%d]", track, d->nb_six);
    return API68_MIX_ERROR;
  }
  m = d->mus + track - 1;

  api68_debug(" -> Starting track #%02d - [%s]:\n", track, m->name);

  /* ReInit 68K & IO */
  EMU68memory_reset();
  EMU68ioplug_unplug_all();
  if (m->flags.amiga) {
    api68_debug(" -> Add Paula hardware\n");
    EMU68ioplug(&paula_io);
    EMU68_set_interrupt_io(0);
  }
  if (m->flags.ym) {
    api68_debug(" -> Add YM hardware\n");
    EMU68ioplug(&shifter_io);
    EMU68ioplug(&ym_io);
    EMU68ioplug(&mfp_io);
    EMU68_set_interrupt_io(&mfp_io);
  }
  if (m->flags.ste) {
    api68_debug(" -> Add STE hardware\n");
    EMU68ioplug(&mw_io);
  }
  EMU68_reset();

  /* Init exceptions */
  memset(reg68.mem, 0, reg68.memsz);
  reg68.mem[0] = 0x4e;
  reg68.mem[1] = 0x73;
  reg68.mem[0x41a] = 0;         /* Zound Dragger */
  reg68.mem[0x41b] = 0x10;      /* Zound Dragger */
  reg68.mem[TRAP_VECTOR(14) + 0] = (u8) (TRAP_14_ADDR >> 24);
  reg68.mem[TRAP_VECTOR(14) + 1] = (u8) (TRAP_14_ADDR >> 16);
  reg68.mem[TRAP_VECTOR(14) + 2] = (u8) (TRAP_14_ADDR >> 8);
  reg68.mem[TRAP_VECTOR(14) + 3] = (u8) (TRAP_14_ADDR);
  memcpy(reg68.mem + TRAP_14_ADDR, trap14, sizeof(trap14));

  /* Address in 68K memory : default $8000 */
  api->playaddr = a0 = (!m->a0) ? 0x8000 : m->a0;
  api68_debug(" -> play address %06x\n", api->playaddr);

  /* Check external replay */
  if (m->replay) {
    int err;
    int size = 0;
    istream_t * is;

    api68_debug(" -> external replay '%s'\n", m->replay);
    is = SC68rsc_open(SC68rsc_replay, m->replay, 1);
    err = !is || (size = istream_length(is), size < 0);
    err = err || stream_read_68k(a0, is, size);
    istream_destroy(is);
    if (err) {
      return API68_MIX_ERROR;
    }
    api68_debug(" -> external replay [%06x-%06x]\n", a0, a0+size);
    a0 = a0 + ((size + 1) & -2);
  }

  /* Copy Data into 68K memory */
  if (EMU68_memput(a0, (u8 *)m->data, m->datasz)) {
    api68_debug(" -> Failed music data [%06x-%06x]\n", a0, a0+m->datasz);

    SC68error_add(EMU68error_get());
    return API68_MIX_ERROR;
  }

  api68_debug(" -> music data [%06x-%06x)\n", a0, a0+m->datasz);

  /* Reset time counter */
  api->time.elapsed = 0;
  api->time.elapsed_ms = 0;
  api->time.rem = 0;
  api->time.rem_ms = 0;

  api->mix.rate = 44100;
  api->mix.buffer = 0;
  api->mix.buflen = 0;
  api->mix.sample_cnt = 0;
  api->mix.pass_cnt = 0;

  if (!m->frames) {
    u64 fr;
    fr = api->time.def_ms;
    fr *= m->frq;
    fr /= 1000u;
    api->mix.pass_total = (unsigned int) fr;
    api68_debug(" -> default time ms : %u\n", api->time.def_ms);
  } else {
    api68_debug(" -> time ms : %u\n", m->time_ms);
    api->mix.pass_total = m->frames;
  }
  api68_debug(" -> frames :  %u\n", api->mix.pass_total);
  api->mix.cycleperpass = (m->frq == 50) ? 160256 : (8000000 / m->frq);
  /* make cycleperpass multiple of 32 (can't remember why :) ) */
  api->mix.cycleperpass = (api->mix.cycleperpass+31) & ~31;
  api68_debug(" -> cycle/frame :  %u\n", api->mix.cycleperpass);

  {
    u64 len;
    len = api->mix.rate;
    len *= api->mix.cycleperpass;
    len /= 8000000;
    api->mix.stdbuflen = (int) len;
  }
  api68_debug(" -> buffer length : %u\n", api->mix.stdbuflen);

  /* Set 68K register value for INIT */
  api68_debug(" -> Running music init code...\n");
  api->reg.d[0] = m->d0;
  api->reg.d[1] = !m->flags.ste;
  api->reg.d[2] = m->datasz;
  api->reg.a[0] = a0;
  api->reg.a[7] = reg68.memsz - 16;
  api->reg.pc = api->playaddr;
  api->reg.sr = 0x2300;
  EMU68_set_registers(&api->reg);
  reg68.cycle = 0;
  EMU68_level_and_interrupt(0);
  api68_debug(" -> OK\n");

  /* Set 68K PC register to music play address */
  EMU68_get_registers(&api->reg);
  api->reg.pc = api->playaddr+8;
  api->reg.sr = 0x2300;
  EMU68_set_registers(&api->reg);

  reg68.cycle = 0;

  api->mus = m;
  api->track = track;

  return API68_CHANGE;
}


int api68_process(api68_t * api, void * buf16st, int n)
{
  int ret = API68_IDLE;
  u32 * buffer;
  int buflen;
  //  const unsigned int sign = MIXER68_CHANGE_SIGN;

  if (!api || n < 0) {
    return API68_MIX_ERROR;
  }

  if (!api->mus) {
    ret = apply_change_track(api);
  }

  if (!api->mus) {
    return API68_MIX_ERROR;
  }

  if (n==0) {
    return ret;
  }

  if (!buf16st) {
    return API68_MIX_ERROR;
  }

  buffer = api->mix.buffer;
  buflen = api->mix.buflen;

  while (n > 0) {
    int code;

    code = apply_change_track(api);
    if (code & API68_MIX_ERROR) {
      return code;
    }
    ret |= code;
    if (code & API68_END) {
      break;
    }

/*     if (api->mix.flush_me) { */
/*       api68_debug("Flushing : %d PCM\n", buflen); */
/*       buflen = 0; */
/*       api->mix.flush_me = 0; */
/*     } */
    
    if (!buflen) {
      /* Not idle */
      ret &= ~API68_IDLE;

      /* Run the emulator. */
      EMU68_level_and_interrupt(api->mix.cycleperpass);

      /* Always used YM buffer, even if no YM required. */
      buffer = YM_get_buffer();

      if (api->mus->flags.amiga) {
	/* Amiga - Paula */
	buflen = api->mix.stdbuflen;
        PL_mix(buffer, api->reg.mem, buflen);
        SC68mixer_blend_LR(buffer, buffer, buflen,api->mix.amiga_blend, 0, 0);
      } else {
	/* Buffer length depends on YM mixer. */
	buflen = (api->mus->flags.ym)
	  ? YM_mix(api->mix.cycleperpass)
	  : api->mix.stdbuflen;

	if (api->mus->flags.ste) {
	  /* STE - MicroWire */
	  MW_mix(buffer, api->reg.mem, buflen);
	} else {
	  /* No STE, process channel duplication. */
	  SC68mixer_dup_L_to_R(buffer, buffer, buflen, 0);
	}
      }

      /* Advance time */
      api->mix.sample_cnt += buflen;
      api->mix.pass_cnt++;

      /* Reach end of track */
      if (api->mix.pass_cnt >= api->mix.pass_total) {
	int next_track;
	ret |= API68_LOOP;
	next_track = api->track+1;
	api->track_to = (next_track > api->disk->nb_six) ? -1 : next_track;
      }
    }

    /* Copy to destination buffer. */
    {
      int len = buflen;
      if (len > n) {
	len = n;
      }
      if (len) {
	u32 * b = (u32 *)buf16st;
	n -= len;
	buflen -= len;
	do {
	  *b++ = *buffer++;
	} while (--len);
	buf16st = b;
      }
    }
  }

  /* Fill buffer with null PCM */
  if (n > 0) {
    u32 * b = (u32 *)buf16st;
    do {
      *b++ = 0;
    } while (--n);
  }

  api->mix.buffer = buffer;
  api->mix.buflen = buflen;

  return ret;
}

int api68_verify(istream_t * is)
{
  return SC68file_verify(is);
}

int api68_verify_file(const char * filename)
{
  return SC68file_verify_file(filename);
}

int api68_verify_mem(const void * buffer, int len)
{
  return SC68file_verify_mem(buffer,len);
}


static int load_disk(api68_t * api, disk68_t * d)
{
  int track;
  if (!api || !d) {
    goto error;
  }

  if (api->disk) {
    SC68error_add("disk is already loaded");
    goto error;
  }

  api->disk = d;
  api->track = 0;
  api->mus = 0;

  track = api->track_here;
  if (track > d->nb_six) {
    track = d->default_six;
  }

  return api68_play(api, track);

 error:
  SC68free(d);
  return -1;
}

int api68_load(api68_t * api, istream_t * is)
{
  return load_disk(api, SC68file_load(is));
}

int api68_load_file(api68_t * api, const char * filename)
{
  return load_disk(api, SC68file_load_file(filename));
}

int api68_load_mem(api68_t * api, const void * buffer, int len)
{
  return load_disk(api,SC68file_load_mem(buffer, len));
}


api68_disk_t api68_load_disk(istream_t * is)
{
  return (api68_disk_t) SC68file_load(is);
}

api68_disk_t api68_load_disk_file(const char * filename)
{
  return (api68_disk_t) SC68file_load_file(filename);
}

api68_disk_t api68_disk_load_mem(const void * buffer, int len)
{
  return (api68_disk_t) SC68file_load_mem(buffer, len);
}


int api68_open(api68_t * api, api68_disk_t disk)
{
  if (!disk) {
    api68_close(api);
    return -1; /* Not an error but notifiy no disk has been loaded */
  }
  if (!api) {
    return -1;
  }
  return load_disk(api, disk);
}

void api68_close(api68_t * api)
{
  if (!api || !api->disk) {
    return;
  }

  api68_debug("api68_close() : enter\n");

  SC68free(api->disk);
  api->disk = 0;
  api->mus = 0;
  api->track = 0;
  api->track_to = 0;

  api68_debug("api68_close() : close\n");
}

int api68_tracks(api68_t * api)
{
  return (api && api->disk) ? api->disk->nb_six : -1;
}

int api68_play(api68_t * api, int track)
{
  disk68_t * d;
  api68_debug("api68_play(%d) : enter\n", track);

  if (!api) {
    return -1;
  }
  d = api->disk;
  if (!d) {
    return -1;
  }

  /* -1 : read current track. */
  if (track == -1) {
    return api->track;
  }
  /* 0 : set disk default. */
  if (track == 0) {
    track = d->default_six + 1;
  }
  /* Check track range. */
  if (track <= 0 || track > d->nb_six) {
    return SC68error_add("track [%d] out of range [%d]", track, d->nb_six);
  }
  /* Set change track. Real track loading occurs during process thread to
     avoid multi-threading bug. */
  api->track_to = track;

  return 0;
}



int api68_stop(api68_t * api)
{
  if (!api || !api->disk) {
    return -1;
  }
  api->track_to = -1;
  return 0;
}


static int calc_current_ms(api68_t * api)
{
  u64 ms;

  ms = api->mix.pass_cnt;
  ms *= api->mix.cycleperpass;
  ms /= 8000u;
  ms += api->mus->start_ms;

  return (int) ms;
}

int api68_seek(api68_t * api, int time_ms)
{
  disk68_t * d;

  if (!api || (d=api->disk, !d)) {
    return -1;
  }

  if (time_ms == -1) {
    if (!api->mus) {
      return -1;
    } else {
      time_ms = calc_current_ms(api);
      return time_ms;
    }
  } else {
    int i,n;

    api68_debug("Seek to %d ms\n", time_ms);

    for (i=0, n=d->nb_six ; i<n; ++i) {
      /*unsigned*/ int start_ms = d->mus[i].start_ms;
      /*unsigned*/ int end_ms = start_ms + d->mus[i].time_ms;

      if (time_ms >= start_ms && time_ms < end_ms) {
	api68_debug("Find track #%d [%u - %u]\n", i+1, start_ms, end_ms);

	if (i+1 == api->track) {
	  time_ms = calc_current_ms(api);
	  api68_debug("Already my track -> %d\n", time_ms);
	  /* Returns -1 because we don't really seek. */
	  return -1;
	} else {
	  api->track_to = i+1;
  	  return start_ms;
	}
      }
    }
    api68_debug("-> Not in disk range !!! [%d>%d]\n",time_ms, d->time_ms);
    return -1;
  }
}

int api68_music_info(api68_t * api, api68_music_info_t * info, int track,
		     api68_disk_t disk)
{
  disk68_t * d;
  music68_t * m = 0;
  int hw;

  static const char * hwtable[8] = {
    "none",
    "Yamaha-2149",
    "MicroWire (STE)",
    "Yamaha-2149 & MicroWire (STE)",
    "Amiga/Paula",
    "Yamaha-2149  & Amiga/Paula",
    "MicroWire (STE) & Amiga/Paula",
    "Yamaha-2149 & MicroWire (STE) & Amiga/Paula",
  };

  if ((!api && !disk) || !info) {
    return -1;
  }

  d = disk ? disk : api->disk;
  if (!d) {
    return -1;
  }

  /* -1 : use current track or default track (depending if disk was given) */
  if (track == -1) {
    track = disk ? d->default_six : api->track;
  }

  if (track < 0 || track > d->nb_six) {
    return SC68error_add("track [%d] out of range [%d]", track, d->nb_six);
  }

  info->track = track;
  info->tracks = d->nb_six;
  if (!track) {
    /* disk info */
    m = d->mus + d->default_six;
    info->title = d->name;
    info->replay = 0;
    info->time_ms = d->time_ms;
    info->start_ms = 0;
    hw = d->flags;
    track = info->tracks;
  } else {
    /* track info */
    m = d->mus + track - 1;
    info->title = m->name;
    info->replay = m->replay;
    info->time_ms = m->time_ms;
    info->start_ms = m->start_ms;
    hw = *(int *)&m->flags;
  }
  info->author = m->author;
  info->composer = m->composer;
  if (!info->replay) {
    info->replay = "built-in";
  }
  *(int *)&info->hw = hw &= 7;
  info->rate = m->frq;
  info->addr = m->a0;
  info->hwname = hwtable[hw];
  SC68time_str(info->time, track, (info->time_ms+999u)/1000u);

  return 0;
}

const char * api68_error(void)
{
  return SC68error_get();
}

void * api68_alloc(unsigned int n)
{
  return SC68alloc(n);
}

void api68_free(void * data)
{
  SC68free(data);
}

void api68_debug(const char * fmt, ...)
{
  va_list list;
  va_start(list,fmt);
  vdebugmsg68(fmt,list);
  va_end(list);
}
