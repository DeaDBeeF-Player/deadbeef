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
#ifndef __CONVERTER_H
#define __CONVERTER_H

#include <stdint.h>
#include <deadbeef.h>

enum {
    DDB_ENCODER_METHOD_PIPE = 0,
    DDB_ENCODER_METHOD_FILE = 1,
};

enum {
    DDB_ENCODER_FMT_8BIT = 0x1,
    DDB_ENCODER_FMT_16BIT = 0x2,
    DDB_ENCODER_FMT_24BIT = 0x4,
    DDB_ENCODER_FMT_32BIT = 0x8,
    DDB_ENCODER_FMT_32BITFLOAT = 0x10,
};

typedef struct ddb_preset_s {
    char *title;
    struct ddb_preset_s *next;
} ddb_preset_t;

typedef struct ddb_encoder_preset_s {
    char *title;
    struct ddb_encoder_preset_s *next;
    char *fname;
    char *encoder;
    int method; // pipe or file
    uint32_t formats; // combination of supported flags (FMT_*)
} ddb_encoder_preset_t;

typedef struct ddb_dsp_preset_s {
    char *title;
    struct ddb_dsp_preset_s *next;
    ddb_dsp_context_t *chain;
} ddb_dsp_preset_t;

typedef struct {
    DB_misc_t misc;

    /////////////////////////////
    // encoder preset management
    /////////////////////////////

    ddb_encoder_preset_t *
    (*encoder_preset_alloc) (void);

    void
    (*encoder_preset_free) (ddb_encoder_preset_t *p);

    ddb_encoder_preset_t *
    (*encoder_preset_load) (const char *fname);

    // @return -1 on path/write error, -2 if file already exists
    int
    (*encoder_preset_save) (ddb_encoder_preset_t *p, int overwrite);

    void
    (*encoder_preset_copy) (ddb_encoder_preset_t *to, ddb_encoder_preset_t *from);

    ddb_encoder_preset_t *
    (*encoder_preset_get_list) (void);

    ddb_encoder_preset_t *
    (*encoder_preset_get_for_idx) (int idx);

    void
    (*encoder_preset_append) (ddb_encoder_preset_t *p);

    void
    (*encoder_preset_remove) (ddb_encoder_preset_t *p);

    void
    (*encoder_preset_replace) (ddb_encoder_preset_t *from, ddb_encoder_preset_t *to);

    /////////////////////////////
    // dsp preset management
    /////////////////////////////

    ddb_dsp_preset_t *
    (*dsp_preset_alloc) (void);

    void
    (*dsp_preset_free) (ddb_dsp_preset_t *p);

    ddb_dsp_preset_t *
    (*dsp_preset_load) (const char *fname);

    // @return -1 on path/write error, -2 if file already exists
    int
    (*dsp_preset_save) (ddb_dsp_preset_t *p, int overwrite);

    void
    (*dsp_preset_copy) (ddb_dsp_preset_t *to, ddb_dsp_preset_t *from);

    ddb_dsp_preset_t *
    (*dsp_preset_get_list) (void);

    ddb_dsp_preset_t *
    (*dsp_preset_get_for_idx) (int idx);

    void
    (*dsp_preset_append) (ddb_dsp_preset_t *p);

    void
    (*dsp_preset_remove) (ddb_dsp_preset_t *p);

    void
    (*dsp_preset_replace) (ddb_dsp_preset_t *from, ddb_dsp_preset_t *to);

    /////////////////////////////
    // converter
    /////////////////////////////

    int
    (*convert) (DB_playItem_t *it, const char *outfolder, int selected_format, ddb_encoder_preset_t *encoder_preset, ddb_dsp_preset_t *dsp_preset, int *abort);

} ddb_converter_t;

#endif
