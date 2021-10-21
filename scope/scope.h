//
//  scope.h
//  DeaDBeeF
//
//  Created by Alexey Yakovenko on 21/10/2021.
//  Copyright © 2021 Alexey Yakovenko. All rights reserved.
//

#ifndef scope_h
#define scope_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float x;
    float ymin, ymax;
} ddb_scope_point_t;

typedef struct {
    int point_count;
    ddb_scope_point_t *points;
} ddb_scope_draw_data_t;

typedef struct {
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

void ddb_scope_get_draw_data (ddb_scope_t *scope, int view_width, int view_height, ddb_scope_draw_data_t *draw_data);

void ddb_scope_draw_data_dealloc (ddb_scope_draw_data_t *draw_data);

#ifdef __cplusplus
}
#endif

#endif /* scope_h */
