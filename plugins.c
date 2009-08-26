#include <dirent.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "plugins.h"
#include "md5/md5.h"
#include "messagepump.h"
#include "messages.h"
#include "threading.h"
#include "progress.h"

// deadbeef api
DB_functions_t deadbeef_api = {
    // FIXME: set to 1.0 after api freeze
    .vmajor = 0,
    .vminor = 0,
    .ev_subscribe = plug_ev_subscribe,
    .ev_unsubscribe = plug_ev_unsubscribe,
    .md5 = plug_md5,
    .md5_to_str = plug_md5_to_str,
    .playback_next = plug_playback_next,
    .playback_prev = plug_playback_prev,
    .playback_pause = plug_playback_pause,
    .playback_stop = plug_playback_stop,
    .playback_play = plug_playback_play,
    .quit = plug_quit,
    .thread_start = thread_start,
    .mutex_create = mutex_create,
    .mutex_free = mutex_free,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
};

void
plug_md5 (uint8_t sig[16], const char *in, int len) {
    md5_buffer (in, len, sig);
}

void
plug_md5_to_str (char *str, const uint8_t sig[16]) {
    md5_sig_to_string ((char *)sig, str, 33);
}

// event handlers
typedef struct {
    DB_plugin_t *plugin;
    db_callback_t callback;
    uintptr_t data;
} evhandler_t;
#define MAX_HANDLERS 100
static evhandler_t handlers[DB_EV_MAX][MAX_HANDLERS];

// plugin control structures
typedef struct plugin_s {
    void *handle;
    DB_plugin_t *plugin;
    struct plugin_s *next;
} plugin_t;
plugin_t *plugins;

void
plug_ev_subscribe (DB_plugin_t *plugin, int ev, db_callback_t callback, uintptr_t data) {
    assert (ev < DB_EV_MAX && ev >= 0);
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (!handlers[ev][i].plugin) {
            handlers[ev][i].plugin = plugin;
            handlers[ev][i].callback = callback;
            handlers[ev][i].data = data;
            return;
        }
    }
    fprintf (stderr, "failed to subscribe plugin %s to event %d (too many event handlers)\n", plugin->name, ev);
}

void
plug_ev_unsubscribe (DB_plugin_t *plugin, int ev, db_callback_t callback, uintptr_t data) {
    assert (ev < DB_EV_MAX && ev >= 0);
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handlers[ev][i].plugin == plugin) {
            handlers[ev][i].plugin = NULL;
            handlers[ev][i].callback = NULL;
            handlers[ev][i].data = 0;
            return;
        }
    }
}

void
plug_playback_next (void) {
    messagepump_push (M_NEXTSONG, 0, 0, 0);
}

void
plug_playback_prev (void) {
    messagepump_push (M_PREVSONG, 0, 0, 0);
}

void
plug_playback_pause (void) {
    messagepump_push (M_PAUSESONG, 0, 0, 0);
}

void 
plug_playback_stop (void) {
    messagepump_push (M_STOPSONG, 0, 0, 0);
}

void 
plug_playback_play (void) {
    messagepump_push (M_PLAYSONG, 0, 0, 0);
}

void 
plug_quit (void) {
    progress_abort ();
    messagepump_push (M_TERMINATE, 0, 0, 0);
}

/////// non-api functions (plugin support)
void
plug_trigger_event (int ev) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handlers[ev][i].plugin && !handlers[ev][i].plugin->inactive) {
            handlers[ev][i].callback (ev, handlers[ev][i].data);
        }
    }
}

void
plug_load_all (void) {
    char dirname[1024];
    snprintf (dirname, 1024, "%s/lib/deadbeef", PREFIX);
    struct dirent **namelist = NULL;
    int n = scandir (dirname, &namelist, NULL, alphasort);
    if (n < 0)
    {
        if (namelist) {
            free (namelist);
        }
        return;	// not a dir or no read access
    }
    else
    {
        int i;
        for (i = 0; i < n; i++)
        {
            // no hidden files
            if (namelist[i]->d_name[0] != '.')
            {
                int l = strlen (namelist[i]->d_name);
                if (l < 3) {
                    continue;
                }
                if (strcasecmp (&namelist[i]->d_name[l-3], ".so")) {
                    continue;
                }
                char fullname[1024];
                strcpy (fullname, dirname);
                strncat (fullname, "/", 1024);
                strncat (fullname, namelist[i]->d_name, 1024);
                printf ("loading plugin %s\n", namelist[i]->d_name);
                void *handle = dlopen (fullname, RTLD_NOW);
                if (!handle) {
                    fprintf (stderr, "dlopen error: %s\n", dlerror ());
                    continue;
                }
                namelist[i]->d_name[l-3] = 0;
                printf ("module name is %s\n", namelist[i]->d_name);
                strcat (namelist[i]->d_name, "_load");
                DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, namelist[i]->d_name);
                if (!plug_load) {
                    fprintf (stderr, "dlsym error: %s\n", dlerror ());
                    dlclose (handle);
                    continue;
                }
                DB_plugin_t *plugin_api = plug_load (&deadbeef_api);
                if (!plugin_api) {
                    namelist[i]->d_name[l-3] = 0;
                    fprintf (stderr, "plugin %s is incompatible with current version of deadbeef, please upgrade the plugin\n");
                    dlclose (handle);
                    continue;
                }
                plugin_t *plug = malloc (sizeof (plugin_t));
                memset (plug, 0, sizeof (plugin_t));
                plug->plugin = plugin_api;
                plug->handle = handle;
                plug->next = plugins;
                if (plug->plugin->start) {
                    if (plug->plugin->start () < 0) {
                        plug->plugin->inactive = 1;
                    }
                }
                plugins = plug;
            }
            free (namelist[i]);
        }
        free (namelist);
    }
}

void
plug_unload_all (void) {
    while (plugins) {
        plugin_t *next = plugins->next;
        if (plugins->plugin->stop) {
            plugins->plugin->stop ();
        }
        dlclose (plugins->handle);
        plugins = next;
    }
}
