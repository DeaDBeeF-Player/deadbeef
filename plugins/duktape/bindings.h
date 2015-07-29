#ifndef deadbeef_bindings_h
#define deadbeef_bindings_h

#include "../../deadbeef.h"

void duktape_bind_all (duk_context *ctx);

// return type conversion
void
js_return_int_value (duk_context *ctx, int val);

void
js_return_float_value (duk_context *ctx, float val);

void
js_return_DB_output_t_ptr_value (duk_context *ctx, DB_output_t *val);

void
js_return_DB_playItem_t_ptr_value (duk_context *ctx, DB_playItem_t *val);

void
js_return_DB_fileinfo_t_ptr_value (duk_context *ctx, DB_fileinfo_t *val);

void
js_return_ddb_dsp_context_t_ptr_value (duk_context *ctx, ddb_dsp_context_t *val);

void
js_return_ddb_playlist_t_ptr_value (duk_context *ctx, ddb_playlist_t *val);

void
js_return_DB_metaInfo_t_ptr_value (duk_context *ctx, DB_metaInfo_t *val);

void
js_return_jsstring_value (duk_context *ctx, const char *val);

// argument type conversion
typedef char *jscharbuffer;
typedef const char *jsstring;
typedef void *jsnull;

jscharbuffer
js_init_jscharbuffer_argument (duk_context *ctx, int idx);

int
js_init_int_argument (duk_context *ctx, int idx);

int
js_init_float_argument (duk_context *ctx, int idx);

ddb_dsp_context_t *
js_init_ddb_dsp_context_t_ptr_argument (duk_context *ctx, int idx);

ddb_playlist_t *
js_init_ddb_playlist_t_ptr_argument (duk_context *ctx, int idx);

DB_playItem_t *
js_init_DB_playItem_t_ptr_argument (duk_context *ctx, int idx);

DB_metaInfo_t *
js_init_DB_metaInfo_t_ptr_argument (duk_context *ctx, int idx);

const char *
js_init_jsstring_argument (duk_context *ctx, int idx);

jsnull
js_init_jsnull_argument (duk_context *ctx, int idx);

#endif
