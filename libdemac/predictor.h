/*

libdemac - A Monkey's Audio decoder

$Id: predictor.h 19236 2008-11-26 18:01:18Z amiconn $

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

#ifndef _APE_PREDICTOR_H
#define _APE_PREDICTOR_H

#include <inttypes.h>
#include "parser.h"
#include "filter.h"

void init_predictor_decoder(struct predictor_t* p);
void predictor_decode_stereo(struct predictor_t* p, int32_t* decoded0,
                             int32_t* decoded1, int count);
void predictor_decode_mono(struct predictor_t* p, int32_t* decoded0,
                           int count);

#endif
