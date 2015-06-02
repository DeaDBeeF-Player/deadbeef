//
//  testbootstrap.c
//  deadbeef
//
//  Created by waker on 12/03/15.
//  Copyright (c) 2015 Alexey Yakovenko. All rights reserved.
//

#include <limits.h>

char sys_install_path[PATH_MAX]; // see deadbeef->get_prefix
char confdir[PATH_MAX]; // $HOME/.config
char dbconfdir[PATH_MAX]; // $HOME/.config/deadbeef
char dbinstalldir[PATH_MAX]; // see deadbeef->get_prefix
char dbdocdir[PATH_MAX]; // see deadbeef->get_doc_dir
char dbplugindir[PATH_MAX]; // see deadbeef->get_plugin_dir
char dbpixmapdir[PATH_MAX]; // see deadbeef->get_pixmap_dir
char dbcachedir[PATH_MAX];
