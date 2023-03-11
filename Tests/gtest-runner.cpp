#include <gtest/gtest.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include "conf.h"
#include "playlist.h"
#include <deadbeef/common.h>
#include "logger.h"
#include "vfs.h"
#include "plugins.h"
#include "playmodes.h"

int main(int argc, char **argv) {
    char buf[PATH_MAX];
    getcwd(buf, sizeof(buf));

    snprintf (dbplugindir, sizeof(dbplugindir), "%s/Tests", buf);

    ddb_logger_init ();
    conf_init ();
    conf_enable_saving (0);
    streamer_playmodes_init();
    pl_init ();

    if (plug_load_all ()) { // required to add files to playlist from commandline
        exit (1);
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
