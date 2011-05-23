// this is a dsp plugin skeleton/example
// use to create new dsp plugins

// usage:
// 1. copy to your plugin folder
// 2. s/example/plugname/g
// 3. s/EXAMPLE/PLUGNAME/g

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <deadbeef/deadbeef.h>

enum {
    EXAMPLE_PARAM_LEVEL,
    EXAMPLE_PARAM_COUNT
};

static DB_functions_t *deadbeef;
static DB_dsp_t plugin;

typedef struct {
    ddb_dsp_context_t ctx;
    // instance-specific variables here
    float level; // this is example
} ddb_example_t;

ddb_dsp_context_t*
example_open (void) {
    ddb_example_t *example = malloc (sizeof (ddb_example_t));
    DDB_INIT_DSP_CONTEXT (example,ddb_example_t,&plugin);

    // initialize
    example->level = 0.5;

    return (ddb_dsp_context_t *)example;
}

void
example_close (ddb_dsp_context_t *ctx) {
    ddb_example_t *example = (ddb_example_t *)ctx;

    // free instance-specific allocations

    free (example);
}

void
example_reset (ddb_dsp_context_t *ctx) {
    // use this method to flush dsp buffers, reset filters, etc
}

int
example_process (ddb_dsp_context_t *ctx, float *samples, int nframes, int maxframes, ddb_waveformat_t *fmt, float *r) {
    ddb_example_t *example = (ddb_example_t *)ctx;
    for (int i = 0; i < nframes*fmt->channels; i++) {
        samples[i] *= example->level;
    }
    return nframes;
}

const char *
example_get_param_name (int p) {
    switch (p) {
    case EXAMPLE_PARAM_LEVEL:
        return "Volume level";
    default:
        fprintf (stderr, "example_param_name: invalid param index (%d)\n", p);
    }
    return NULL;
}

int
example_num_params (void) {
    return EXAMPLE_PARAM_COUNT;
}

void
example_set_param (ddb_dsp_context_t *ctx, int p, const char *val) {
    ddb_example_t *example = (ddb_example_t *)ctx;
    switch (p) {
    case EXAMPLE_PARAM_LEVEL:
        example->level = atof (val);
        break;
    default:
        fprintf (stderr, "example_param: invalid param index (%d)\n", p);
    }
}

void
example_get_param (ddb_dsp_context_t *ctx, int p, char *val, int sz) {
    ddb_example_t *example = (ddb_example_t *)ctx;
    switch (p) {
    case EXAMPLE_PARAM_LEVEL:
        snprintf (val, sz, "%f", example->level);
        break;
    default:
        fprintf (stderr, "example_get_param: invalid param index (%d)\n", p);
    }
}

static const char settings_dlg[] =
    "property \"Volume Level\" spinbtn[0,2,0.1] 0 0.5;\n"
;

static DB_dsp_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .open = example_open,
    .close = example_close,
    .process = example_process,
    .plugin.version_major = 0,
    .plugin.version_minor = 1,
    .plugin.type = DB_PLUGIN_DSP,
    .plugin.id = "example",
    .plugin.name = "example",
    .plugin.descr = "example DSP Plugin",
    .plugin.copyright = "copyright message - author(s), license, etc",
    .plugin.website = "http://example.com",
    .num_params = example_num_params,
    .get_param_name = example_get_param_name,
    .set_param = example_set_param,
    .get_param = example_get_param,
    .reset = example_reset,
    .configdialog = settings_dlg,
};

DB_plugin_t *
example_load (DB_functions_t *f) {
    deadbeef = f;
    return &plugin.plugin;
}
