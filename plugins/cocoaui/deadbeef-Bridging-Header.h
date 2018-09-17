//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//


#include <string.h>
#include "../../deadbeef.h"
#include "../../conf.h"
#include "../../plugins.h"
#include "../../common.h"
#include "../../logger.h"
#import "PluginConfigurationViewController.h"

static void set_dbconfdir (const char *value) {
    strcpy (dbconfdir, value);
}

static DB_dsp_t *
plug_get_dsp_for_id (const char *id) {
    return (DB_dsp_t *)plug_get_for_id (id);
}
