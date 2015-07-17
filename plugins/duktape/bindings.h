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


// argument type conversion
typedef char *jscharbuffer;

jscharbuffer
js_init_jscharbuffer_argument (duk_context *ctx, int idx);

int
js_init_int_argument (duk_context *ctx, int idx);

int
js_init_float_argument (duk_context *ctx, int idx);

#endif
