/*
    DeaDBeeF - The Ultimate Music Player
    Copyright (C) 2009-2013 Oleksiy Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <deadbeef/deadbeef.h>

int
main (int argc, char *argv[]) {
    char d_name[256];
    void *handle;
    size_t l;

    if (argc <= 1) {
        fprintf (stderr, "usage: pluginfo plugin.so\n");
        exit (-1);
    }
    char *slash = strrchr (argv[1], '/');
    if (!slash) {
        slash = argv[1];
    }
    else {
        slash++;
    }
    l = strlen (slash);
    if (l < 3 || strcasecmp (&slash[l-3], ".so")) {
        fprintf (stderr, "pluginfo: invalid fname %s\n", argv[1]);
        exit (-1);
    }

    strcpy (d_name, slash);

    handle = dlopen (argv[1], RTLD_NOW);
    if (!handle) {
        fprintf (stderr, "dlopen error: %s\n", dlerror ());
        exit (-1);
    }
    d_name[l-3] = 0;
    strcat (d_name, "_load");
    DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name);
    if (!plug_load) {
        fprintf (stderr, "pluginfo: dlsym error: %s\n", dlerror ());
        dlclose (handle);
        exit (-1);
    }

    DB_plugin_t *plug = plug_load ((DB_functions_t *)0xdeadbeef);
    printf ("type=\"%d\"\n", plug->type);
    printf ("api_version=\"%d.%d\"\n", plug->api_vmajor, plug->api_vminor);
    printf ("version=\"%d.%d\"\n", plug->version_major, plug->version_minor);
    printf ("id=\"%s\"\n", plug->id);
    printf ("name=\"%s\"\n", plug->name);
    printf ("descr=\"");
    const char *c;
    for (c = plug->descr; *c; c++) {
        if (*c == '"') {
            printf ("\\\"");
        }
        else {
            printf ("%c", *c);
        }
    }
    
    printf ("\"\n");
    printf ("website=\"%s\"\n", plug->website);

    dlclose (handle);
    return 0;
}
