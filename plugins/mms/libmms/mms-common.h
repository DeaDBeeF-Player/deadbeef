/*
 * Copyright (C) 2010 Hans de Goede <j.w.r.degoede@hhs.nl>
 *
 * This file is part of libmms a free mms protocol library
 *
 * libmms is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libmss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

/* This file contains code which is shared between the mms and mmsh protocol
   handling code. */

#ifndef __MMS_COMMON_H
#define __MMS_COMMON_H

typedef struct mms_stream_s mms_stream_t;
struct mms_stream_s {
  int           stream_id;
  int           stream_type;
  uint32_t      bitrate;
  uint32_t      bitrate_pos;
};

#endif
