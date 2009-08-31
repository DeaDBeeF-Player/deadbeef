/*

libdemac - A Monkey's Audio decoder

$Id: demac_config.h 19199 2008-11-24 18:40:49Z amiconn $

Copyright (C) Dave Chapman 2007

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#ifndef _DEMAC_CONFIG_H
#define _DEMAC_CONFIG_H

/* Build-time choices for libdemac.
 * Note that this file is included by both .c and .S files. */

#ifdef ROCKBOX

#include "config.h"

#ifndef __ASSEMBLER__
#include "codeclib.h"
#include <codecs.h>
#endif

#define APE_OUTPUT_DEPTH 29

/* On ARMv4, using 32 bit ints for the filters is faster. */
#if defined(CPU_ARM) && (ARM_ARCH == 4)
#define FILTER_BITS 32
#endif

#if CONFIG_CPU == PP5002
/* Code in IRAM for speed, not enough IRAM for the insane filter buffer. */
#define ICODE_SECTION_DEMAC_ARM   .icode
#define ICODE_ATTR_DEMAC          ICODE_ATTR
#define IBSS_ATTR_DEMAC_INSANEBUF
#elif CONFIG_CPU == PP5020
/* Not enough IRAM for the insane filter buffer. */
#define ICODE_SECTION_DEMAC_ARM   .text
#define ICODE_ATTR_DEMAC
#define IBSS_ATTR_DEMAC_INSANEBUF
#else
#define ICODE_SECTION_DEMAC_ARM   .text
#define ICODE_ATTR_DEMAC
#define IBSS_ATTR_DEMAC_INSANEBUF IBSS_ATTR
#endif

#else /* !ROCKBOX */

#define APE_OUTPUT_DEPTH (ape_ctx->bps)

#define IBSS_ATTR
#define IBSS_ATTR_DEMAC_INSANEBUF
#define ICONST_ATTR
#define ICODE_ATTR
#define ICODE_ATTR_DEMAC

/* Use to give gcc hints on which branch is most likely taken */
#if defined(__GNUC__) && __GNUC__ >= 3
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#endif

#endif /* !ROCKBOX */

/* Defaults */

#ifndef UDIV32
#define UDIV32(a, b) (a / b)
#endif

#ifndef FILTER_HISTORY_SIZE
#define FILTER_HISTORY_SIZE 512
#endif

#ifndef PREDICTOR_HISTORY_SIZE
#define PREDICTOR_HISTORY_SIZE 512
#endif     

#ifndef FILTER_BITS
#define FILTER_BITS 16
#endif


#ifndef __ASSEMBLER__
#include <inttypes.h>
#if FILTER_BITS == 32
typedef int32_t filter_int;
#elif FILTER_BITS == 16
typedef int16_t filter_int;
#endif
#endif

#endif /* _DEMAC_CONFIG_H */
