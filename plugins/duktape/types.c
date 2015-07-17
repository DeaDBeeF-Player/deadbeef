#include <stdio.h>
#include "duktape.h"
#include "bindings.h"

void
js_return_int_value (duk_context *ctx, int val) {
    duk_push_number(ctx, val);
}

void
js_return_float_value (duk_context *ctx, float val) {
    duk_push_number(ctx, val);
}

void
js_return_DB_output_t_ptr_value (duk_context *ctx, DB_output_t *val) {
    // {
    //    int bps;
    //    int channels;
    //    int samplerate;
    //    uint32_t channelmask;
    //    int is_float;
    //    int is_bigendian;
    //    int has_volume;
    // }

    duk_idx_t obj_idx;

    obj_idx = duk_push_object (ctx);

    duk_push_int(ctx, val->fmt.bps);
    duk_put_prop_string(ctx, obj_idx, "bps");
    duk_push_int(ctx, val->fmt.channels);
    duk_put_prop_string(ctx, obj_idx, "channels");
    duk_push_int(ctx, val->fmt.samplerate);
    duk_put_prop_string(ctx, obj_idx, "samplerate");
    duk_push_int(ctx, val->fmt.channelmask);
    duk_put_prop_string(ctx, obj_idx, "channelmask");
    duk_push_int(ctx, val->fmt.is_float);
    duk_put_prop_string(ctx, obj_idx, "is_float");
    duk_push_int(ctx, val->fmt.is_bigendian);
    duk_put_prop_string(ctx, obj_idx, "is_bigendian");
    duk_push_int(ctx, val->has_volume);
    duk_put_prop_string(ctx, obj_idx, "has_volume");
}

void
js_return_DB_playItem_t_ptr_value (duk_context *ctx, DB_playItem_t *val) {
    // {
    //    int startsample;
    //    int endsample;
    //    int shufflerating;
    // }

    duk_idx_t obj_idx;

    obj_idx = duk_push_object (ctx);

    duk_push_int(ctx, val->startsample);
    duk_put_prop_string(ctx, obj_idx, "startsample");
    duk_push_int(ctx, val->endsample);
    duk_put_prop_string(ctx, obj_idx, "endsample");
    duk_push_int(ctx, val->shufflerating);
    duk_put_prop_string(ctx, obj_idx, "shufflerating");
    duk_push_pointer (ctx, val);
    duk_put_prop_string(ctx, obj_idx, "_ptr");
}

jscharbuffer
js_init_jscharbuffer_argument (duk_context *ctx, int idx) {
    // assumes there's a buffer on stack
    duk_size_t size;
    void *buf = duk_to_buffer(ctx, idx, &size);
    return (char *)buf;
}

int
js_init_int_argument (duk_context *ctx, int idx) {
    return duk_to_number (ctx, idx);
}

int
js_init_float_argument (duk_context *ctx, int idx) {
    return duk_to_number (ctx, idx);
}
