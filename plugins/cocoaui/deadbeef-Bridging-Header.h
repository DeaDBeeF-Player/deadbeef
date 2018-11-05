//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//


#include <string.h>
#include "../../deadbeef.h"
#include "../../conf.h"
#include "../../plugins.h"
#include "../../common.h"
#include "../../logger.h"

static void set_dbconfdir (const char *value) {
    strcpy (dbconfdir, value);
}
