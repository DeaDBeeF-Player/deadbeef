#ifndef _RESAMPLER_H_
#define _RESAMPLER_H_

/* Copyright (C) 2004-2008 Shay Green.
   Copyright (C) 2015 Christopher Snowhill. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

#define RESAMPLER_BITS 32
#define RESAMPLER_DECORATE vgmplay

#ifdef RESAMPLER_DECORATE
#undef PASTE
#undef EVALUATE
#define PASTE(a,b) a ## b
#define EVALUATE(a,b) PASTE(a,b)
#define resampler_create EVALUATE(RESAMPLER_DECORATE,_resampler_create)
#define resampler_dup EVALUATE(RESAMPLER_DECORATE,_resampler_dup)
#define resampler_destroy EVALUATE(RESAMPLER_DECORATE,_resampler_destroy)
#define resampler_clear EVALUATE(RESAMPLER_DECORATE,_resampler_clear)
#define resampler_set_rate EVALUATE(RESAMPLER_DECORATE,_resampler_set_rate)
#define resampler_get_free EVALUATE(RESAMPLER_DECORATE,_resampler_get_free)
#define resampler_get_min_fill EVALUATE(RESAMPLER_DECORATE,_resampler_get_min_fill)
#define resampler_write_pair EVALUATE(RESAMPLER_DECORATE,_resampler_write_pair)
#define resampler_get_avail EVALUATE(RESAMPLER_DECORATE,_resampler_get_avail)
#define resampler_read_pair EVALUATE(RESAMPLER_DECORATE,_resampler_read_pair)
#define resampler_peek_pair EVALUATE(RESAMPLER_DECORATE,_resampler_peek_pair)
#endif

#include <stdint.h>

#if RESAMPLER_BITS == 16
typedef int16_t sample_t;
#elif RESAMPLER_BITS == 32
typedef int32_t sample_t;
#else
#error Choose a bit depth!
#endif

#ifdef __cplusplus
extern "C" {
#endif

void * resampler_create();
void * resampler_dup(void *);
void resampler_destroy(void *);

void resampler_clear(void *);

void resampler_set_rate( void *, double new_factor );

int resampler_get_free(void *);
int resampler_get_min_fill(void *);

void resampler_write_pair(void *, sample_t ls, sample_t rs);

int resampler_get_avail(void *);

void resampler_read_pair( void *, sample_t *ls, sample_t *rs );
void resampler_peek_pair( void *, sample_t *ls, sample_t *rs );

#ifdef __cplusplus
}
#endif

#endif
