/*

libdemac - A Monkey's Audio decoder

$Id: decoder.h 19743 2009-01-10 21:10:56Z zagor $

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

#ifndef _APE_DECODER_H
#define _APE_DECODER_H

#include <inttypes.h>
#include "parser.h"

void init_frame_decoder(struct ape_ctx_t* ape_ctx,
                        unsigned char* inbuffer, int* firstbyte,
                        int* bytesconsumed);

int decode_chunk(struct ape_ctx_t* ape_ctx,
                 unsigned char* inbuffer, int* firstbyte,
                 int* bytesconsumed,
                 int32_t* decoded0, int32_t* decoded1, 
                 int count);
#endif
