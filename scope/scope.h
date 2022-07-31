/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifndef scope_h
#define scope_h

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DDB_SCOPE_MONO,
    DDB_SCOPE_MULTICHANNEL,
} ddb_scope_mode_t;

typedef struct {
    float ymin, ymax;
} ddb_scope_point_t;

typedef struct {
    ddb_scope_mode_t mode;
    int channels;
    int point_count;
    ddb_scope_point_t *points; // real point count is multiplied by number of channels, in planar layout
} ddb_scope_draw_data_t;

typedef struct {
    ddb_scope_mode_t mode;
    int mode_did_change;
    int fragment_duration; // ms
    int samplerate;
    int channels;
    int sample_count;
    float *samples;
} ddb_scope_t;

ddb_scope_t *ddb_scope_alloc (void);

ddb_scope_t *ddb_scope_init (ddb_scope_t *scope);

void ddb_scope_dealloc (ddb_scope_t *scope);

void ddb_scope_free (ddb_scope_t *scope);

void ddb_scope_process (ddb_scope_t *scope, int samplerate, int channels, const float *samples, int sample_count);

void ddb_scope_tick (ddb_scope_t *scope);

void ddb_scope_get_draw_data (ddb_scope_t *scope, int view_width, int view_height, int y_axis_flip, ddb_scope_draw_data_t *draw_data);

void ddb_scope_draw_data_dealloc (ddb_scope_draw_data_t *draw_data);

#ifdef __cplusplus
}
#endif

#endif /* scope_h */
