/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009  Alexey Yakovenko

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
#include <dirent.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "plugins.h"
#include "md5/md5.h"
#include "messagepump.h"
#include "threading.h"
#include "progress.h"
#include "playlist.h"
#include "volume.h"
#include "streamer.h"
#include "playback.h"
#include "common.h"
#include "conf.h"
#include "junklib.h"
#include "vfs.h"

//#define DISABLE_VERSIONCHECK 1

static uintptr_t mutex;

// deadbeef api
static DB_functions_t deadbeef_api = {
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
    .playback_random = plug_playback_random,
    .playback_get_pos = plug_playback_get_pos,
    .playback_set_pos = plug_playback_set_pos,
    .playback_get_samplerate = p_get_rate,
    .playback_update_bitrate = streamer_update_bitrate,
    .get_config_dir = plug_get_config_dir,
    .quit = plug_quit,
    // threading
    .thread_start = thread_start,
    .thread_join = thread_join,
    .mutex_create = mutex_create,
    .mutex_free = mutex_free,
    .mutex_lock = mutex_lock,
    .mutex_unlock = mutex_unlock,
    .cond_create = cond_create,
    .cond_free = cond_free,
    .cond_wait = cond_wait,
    .cond_signal = cond_signal,
    .cond_broadcast = cond_broadcast,
    // playlist access
    .pl_item_alloc = (DB_playItem_t* (*)(void))pl_item_alloc,
    .pl_item_free = (void (*)(DB_playItem_t *))pl_item_free,
    .pl_item_copy = (void (*)(DB_playItem_t *, DB_playItem_t *))pl_item_copy,
    .pl_insert_item = (DB_playItem_t *(*) (DB_playItem_t *after, DB_playItem_t *it))pl_insert_item,
    .pl_get_idx_of = (int (*) (DB_playItem_t *it))pl_get_idx_of,
    .pl_set_item_duration = (void (*) (DB_playItem_t *it, float duration))pl_set_item_duration,
    .pl_get_item_duration = (float (*) (DB_playItem_t *it))pl_get_item_duration,
    // metainfo
    .pl_add_meta = (void (*) (DB_playItem_t *, const char *, const char *))pl_add_meta,
    .pl_find_meta = (const char *(*) (DB_playItem_t *, const char *))pl_find_meta,
    .pl_delete_all_meta = (void (*) (DB_playItem_t *it))pl_delete_all_meta,
    // cuesheet support
    .pl_insert_cue_from_buffer = (DB_playItem_t *(*) (DB_playItem_t *after, const char *fname, const uint8_t *buffer, int buffersize, struct DB_decoder_s *decoder, const char *ftype, int numsamples, int samplerate))pl_insert_cue_from_buffer,
    .pl_insert_cue = (DB_playItem_t *(*)(DB_playItem_t *, const char *, struct DB_decoder_s *, const char *ftype, int numsamples, int samplerate))pl_insert_cue,
    // volume control
    .volume_set_db = plug_volume_set_db,
    .volume_get_db = volume_get_db,
    .volume_set_amp = plug_volume_set_amp,
    .volume_get_amp = volume_get_amp,
    // junk reading
    .junk_read_id3v1 = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_read_id3v1,
    .junk_read_id3v2 = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_read_id3v2,
    .junk_read_ape = (int (*)(DB_playItem_t *it, DB_FILE *fp))junk_read_ape,
    .junk_get_leading_size = junk_get_leading_size,
    // vfs
    .fopen = vfs_fopen,
    .fclose = vfs_fclose,
    .fread = vfs_fread,
    .fseek = vfs_fseek,
    .ftell = vfs_ftell,
    .rewind = vfs_rewind,
    .fgetlength = vfs_fgetlength,
    .fget_content_type = vfs_get_content_type,
    .fget_content_name = vfs_get_content_name,
    .fget_content_genre = vfs_get_content_genre,
    .fstop = vfs_fstop,
    // message passing
    .sendmessage = messagepump_push,
    // configuration access
    .conf_get_str = conf_get_str,
    .conf_get_float = conf_get_float,
    .conf_get_int = conf_get_int,
    .conf_set_str = conf_set_str,
    .conf_find = conf_find,
    .gui_lock = plug_gui_lock,
    .gui_unlock = plug_gui_unlock,
};

DB_functions_t *deadbeef = &deadbeef_api;

const char *
plug_get_config_dir (void) {
    return dbconfdir;
}

void
volumebar_notify_changed (void);

void
plug_volume_set_db (float db) {
    volume_set_db (db);
    volumebar_notify_changed ();
}

void
plug_volume_set_amp (float amp) {
    volume_set_amp (amp);
    volumebar_notify_changed ();
}

#define MAX_PLUGINS 100
DB_plugin_t *g_plugins[MAX_PLUGINS+1];

#define MAX_DECODER_PLUGINS 50
DB_decoder_t *g_decoder_plugins[MAX_DECODER_PLUGINS+1];

#define MAX_VFS_PLUGINS 10
DB_vfs_t *g_vfs_plugins[MAX_VFS_PLUGINS+1];

void
plug_gui_lock (void) {
    gdk_threads_enter ();
}

void
plug_gui_unlock (void) {
    gdk_threads_leave ();
}

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
    DB_callback_t callback;
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
plug_ev_subscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data) {
    assert (ev < DB_EV_MAX && ev >= 0);
    int i;
    mutex_lock (mutex);
    for (i = 0; i < MAX_HANDLERS; i++) {
        if (!handlers[ev][i].plugin) {
            handlers[ev][i].plugin = plugin;
            handlers[ev][i].callback = callback;
            handlers[ev][i].data = data;
            break;
        }
    }
    mutex_unlock (mutex);
    if (i == MAX_HANDLERS) {
        fprintf (stderr, "failed to subscribe plugin %s to event %d (too many event handlers)\n", plugin->name, ev);
    }
}

void
plug_ev_unsubscribe (DB_plugin_t *plugin, int ev, DB_callback_t callback, uintptr_t data) {
    assert (ev < DB_EV_MAX && ev >= 0);
    mutex_lock (mutex);
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handlers[ev][i].plugin == plugin) {
            handlers[ev][i].plugin = NULL;
            handlers[ev][i].callback = NULL;
            handlers[ev][i].data = 0;
            break;
        }
    }
    mutex_unlock (mutex);
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
plug_playback_random (void) {
    messagepump_push (M_PLAYRANDOM, 0, 0, 0);
}

float
plug_playback_get_pos (void) {
    if (str_playing_song._duration <= 0) {
        return 0;
    }
    return streamer_get_playpos () * 100 / str_playing_song._duration;
}

void
plug_playback_set_pos (float pos) {
    if (str_playing_song._duration <= 0) {
        return;
    }
    float t = pos * str_playing_song._duration / 100.f;
    streamer_set_seek (t);
}

void 
plug_quit (void) {
    progress_abort ();
    messagepump_push (M_TERMINATE, 0, 0, 0);
}

/////// non-api functions (plugin support)
void
plug_trigger_event (int ev, uintptr_t param) {
    mutex_lock (mutex);
    DB_event_t *event;
    switch (ev) {
    case DB_EV_SONGSTARTED:
    case DB_EV_SONGFINISHED:
        {
        DB_event_song_t *pev = malloc (sizeof (DB_event_song_t));
        pev->song = DB_PLAYITEM (&str_playing_song);
        event = DB_EVENT (pev);
        }
        break;
    case DB_EV_TRACKDELETED:
        {
            DB_event_song_t *pev = malloc (sizeof (DB_event_song_t));
            pev->song = DB_PLAYITEM (param);
            event = DB_EVENT (pev);
        }
        break;
    default:
        event = malloc (sizeof (DB_event_t));
    }
    event->event = ev;
    event->time = (double)clock () / CLOCKS_PER_SEC;
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handlers[ev][i].plugin && !handlers[ev][i].plugin->inactive) {
            handlers[ev][i].callback (event, handlers[ev][i].data);
        }
    }
    free (event);
    mutex_unlock (mutex);
}

int
plug_init_plugin (DB_plugin_t* (*loadfunc)(DB_functions_t *), void *handle) {
    DB_plugin_t *plugin_api = loadfunc (&deadbeef_api);
    if (!plugin_api) {
        return -1;
    }
#if !DISABLE_VERSIONCHECK
    if (plugin_api->api_vmajor != 0 || plugin_api->api_vminor != 0) {
        // version check enabled
        if (plugin_api->api_vmajor != DB_API_VERSION_MAJOR || plugin_api->api_vminor != DB_API_VERSION_MINOR) {
            fprintf (stderr, "\033[0;31mWARNING: plugin \"%s\" wants API v%d.%d (got %d.%d), will not be loaded\033[0;m\n", plugin_api->name, plugin_api->api_vmajor, plugin_api->api_vminor, DB_API_VERSION_MAJOR, DB_API_VERSION_MINOR);
            return -1;
        }
    }
    else {
            fprintf (stderr, "\033[0;31mWARNING: plugin \"%s\" has disabled version check. do not distribute!\033[0;m\n", plugin_api->name);
    }
#endif
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
    return 0;
}

void
plug_load_all (void) {
#if DISABLE_VERSIONCHECK
    fprintf (stderr, "\033[0;31mDISABLE_VERSIONCHECK=1! do not distribute!\033[0;m\n");
#endif
    const char *conf_blacklist_plugins = conf_get_str ("blacklist_plugins", "");
    mutex = mutex_create ();
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
                char d_name[256];
                memcpy (d_name, namelist[i]->d_name, l+1);
                // no blacklisted
                const uint8_t *p = conf_blacklist_plugins;
                while (*p) {
                    const uint8_t *e = p;
                    while (*e && *e > 0x20) {
                        e++;
                    }
                    if (l-3 == e-p) {
                        if (!strncmp (p, d_name, e-p)) {
                            p = NULL;
                            break;
                        }
                    }
                    p = e;
                    while (*p && *p <= 0x20) {
                        p++;
                    }
                }
                if (!p) {
                    fprintf (stderr, "plugin %s is blacklisted in config file\n", d_name);
                    continue;
                }
                char fullname[1024];
                strcpy (fullname, dirname);
                strncat (fullname, "/", 1024);
                strncat (fullname, d_name, 1024);
                printf ("loading plugin %s\n", d_name);
                void *handle = dlopen (fullname, RTLD_NOW);
                if (!handle) {
                    fprintf (stderr, "dlopen error: %s\n", dlerror ());
                    continue;
                }
                d_name[l-3] = 0;
                printf ("module name is %s\n", d_name);
                strcat (d_name, "_load");
                DB_plugin_t *(*plug_load)(DB_functions_t *api) = dlsym (handle, d_name);
                if (!plug_load) {
                    fprintf (stderr, "dlsym error: %s\n", dlerror ());
                    dlclose (handle);
                    continue;
                }
                if (plug_init_plugin (plug_load, handle) < 0) {
                    d_name[l-3] = 0;
                    fprintf (stderr, "plugin %s is incompatible with current version of deadbeef, please upgrade the plugin\n", d_name);
                    dlclose (handle);
                    continue;
                }
            }
            free (namelist[i]);
        }
        free (namelist);
    }
// load all compiled-in modules
#define PLUG(n) extern DB_plugin_t * n##_load (DB_functions_t *api);
#include "moduleconf.h"
#undef PLUG
#define PLUG(n) plug_init_plugin (n##_load, NULL);
#include "moduleconf.h"
#undef PLUG

    // categorize plugins
    int numplugins = 0;
    int numdecoders = 0;
    int numvfs = 0;
    for (plugin_t *plug = plugins; plug; plug = plug->next) {
        g_plugins[numplugins++] = plug->plugin;
        if (plug->plugin->type == DB_PLUGIN_DECODER) {
            fprintf (stderr, "found decoder plugin %s\n", plug->plugin->name);
            if (numdecoders >= MAX_DECODER_PLUGINS) {
                break;
            }
            g_decoder_plugins[numdecoders++] = (DB_decoder_t *)plug->plugin;
        }
        else if (plug->plugin->type == DB_PLUGIN_VFS) {
            fprintf (stderr, "found vfs plugin %s\n", plug->plugin->name);
            if (numvfs >= MAX_VFS_PLUGINS) {
                break;
            }
            g_vfs_plugins[numvfs++] = (DB_vfs_t *)plug->plugin;
        }
    }
    g_plugins[numplugins] = NULL;
    g_decoder_plugins[numdecoders] = NULL;
    g_vfs_plugins[numvfs] = NULL;
}

void
plug_unload_all (void) {
    while (plugins) {
        plugin_t *next = plugins->next;
        if (plugins->plugin->stop) {
            plugins->plugin->stop ();
        }
        if (plugins->handle) {
            dlclose (plugins->handle);
        }
        plugins = next;
    }
    mutex_free (mutex);
}

struct DB_decoder_s **
plug_get_decoder_list (void) {
    return g_decoder_plugins;
}

struct DB_vfs_s **
plug_get_vfs_list (void) {
    return g_vfs_plugins;
}

struct DB_plugin_s **
plug_get_list (void) {
    return g_plugins;
}
