//
//  DDBTestInitializer.m
//  Tests
//
//  Created by Oleksiy Yakovenko on 19/06/2018.
//  Copyright © 2018 Oleksiy Yakovenko. All rights reserved.
//

#import "DDBTestInitializer.h"
#include "conf.h"
#include "playlist.h"
#include <deadbeef/common.h>
#include "logger.h"
#include "vfs.h"
#include "plugins.h"
#include "playmodes.h"
#include "tf.h"
#include "metacache.h"

@implementation DDBTestInitializer
- (instancetype)init {
    NSString *resPath = [NSBundle bundleForClass:self.class].resourcePath;
    const char *str = resPath.UTF8String;
    strcpy (dbplugindir, str);

    ddb_logger_init ();
    conf_init ();
    metacache_init ();
    tf_init();
    conf_enable_saving (0);
    streamer_playmodes_init();
    pl_init ();

    if (plug_load_all ()) { // required to add files to playlist from commandline
        exit (1);
    }

    return self;
}
@end
