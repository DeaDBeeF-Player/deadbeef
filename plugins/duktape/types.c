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
js_return_ddb_waveformat_t_ptr_value (duk_context *ctx, ddb_waveformat_t *val) {
    // {
    //     void *_ptr;
    //     int channels;
    //     int samplerate;
    //     uint32_t channelmask;
    //     int is_float;
    //     int is_bigendian;
    // }
    duk_idx_t obj_idx;

    obj_idx = duk_push_object (ctx);

    duk_push_pointer (ctx, val);
    duk_put_prop_string(ctx, obj_idx, "_ptr");

    duk_push_int(ctx, val->bps);
    duk_put_prop_string(ctx, obj_idx, "bps");
    duk_push_int(ctx, val->channels);
    duk_put_prop_string(ctx, obj_idx, "channels");
    duk_push_int(ctx, val->samplerate);
    duk_put_prop_string(ctx, obj_idx, "samplerate");
    duk_push_int(ctx, val->channelmask);
    duk_put_prop_string(ctx, obj_idx, "channelmask");
    duk_push_int(ctx, val->is_float);
    duk_put_prop_string(ctx, obj_idx, "is_float");
    duk_push_int(ctx, val->is_bigendian);
    duk_put_prop_string(ctx, obj_idx, "is_bigendian");
}

void
js_return_DB_output_t_ptr_value (duk_context *ctx, DB_output_t *val) {
    // {
    //     void *_ptr;
    //     int has_volume;
    //     object fmt;
    // }

    duk_idx_t obj_idx;

    obj_idx = duk_push_object (ctx);

    duk_push_pointer (ctx, val);
    duk_put_prop_string(ctx, obj_idx, "_ptr");
    duk_push_int(ctx, val->has_volume);
    duk_put_prop_string(ctx, obj_idx, "has_volume");
    js_return_ddb_waveformat_t_ptr_value(ctx, &val->fmt);
    duk_put_prop_string(ctx, obj_idx, "fmt");
}

void
js_return_DB_playItem_t_ptr_value (duk_context *ctx, DB_playItem_t *val) {
    // {
    //     int startsample;
    //     int endsample;
    //     int shufflerating;
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

void
js_return_DB_fileinfo_t_ptr_value (duk_context *ctx, DB_fileinfo_t *val) {
    // {
    //     void *_ptr;
    //     void *plugin;
    //     ddb_waveformat_t fmt;
    //     float readpos;
    //     void *file;
    // }
    duk_idx_t obj_idx;

    obj_idx = duk_push_object (ctx);
    duk_push_pointer (ctx, val);
    duk_put_prop_string(ctx, obj_idx, "_ptr");
    duk_push_pointer (ctx, val->plugin);
    duk_put_prop_string(ctx, obj_idx, "plugin");
    js_return_ddb_waveformat_t_ptr_value(ctx, &val->fmt);
    duk_put_prop_string(ctx, obj_idx, "fmt");
    duk_push_number (ctx, val->readpos);
    duk_put_prop_string(ctx, obj_idx, "readpos");
    duk_push_pointer (ctx, val->file);
    duk_put_prop_string(ctx, obj_idx, "file");

}

void
js_return_ddb_dsp_context_t_ptr_value (duk_context *ctx, ddb_dsp_context_t *val) {
    duk_idx_t obj_idx;
    obj_idx = duk_push_object (ctx);
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

ddb_dsp_context_t *
js_init_ddb_dsp_context_t_ptr_argument (duk_context *ctx, int idx) {
    // FIXME: right now, the dsp context pointer is just passed back to deadbeef API
    // instead, the pointed struct needs to be reinitialized with the values from the object.
    duk_to_object (ctx, idx);
    duk_get_prop_string (ctx, 0, "_ptr");
    void *ptr = duk_to_pointer (ctx, -1);
    return ptr;
}
