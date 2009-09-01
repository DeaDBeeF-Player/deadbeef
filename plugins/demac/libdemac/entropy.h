/*

libdemac - A Monkey's Audio decoder

$Id: entropy.h 19236 2008-11-26 18:01:18Z amiconn $

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

#ifndef _APE_ENTROPY_H
#define _APE_ENTROPY_H

#include <inttypes.h>

void init_entropy_decoder(struct ape_ctx_t* ape_ctx,
                          unsigned char* inbuffer, int* firstbyte,
                          int* bytesconsumed);

void entropy_decode(struct ape_ctx_t* ape_ctx,
                    unsigned char* inbuffer, int* firstbyte,
                    int* bytesconsumed,
                    int32_t* decoded0, int32_t* decoded1,
                    int blockstodecode);

#endif
