/*
    Media Library plugin for DeaDBeeF Player
    Copyright (C) 2009-2021 Oleksiy Yakovenko

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <deadbeef/deadbeef.h>
#include <jansson.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "medialib.h"
#include "medialibcommon.h"
#include "medialibfilesystem.h"
#include "medialibscanner.h"
#include "medialibsource.h"
#include "medialibtree.h"
#include "scriptable_tfquery.h"

static DB_functions_t *deadbeef;
static DB_mediasource_t plugin;

static int
ml_connect (void) {
    return 0;
}

static int
ml_start (void) {
    ml_source_init(deadbeef);
    ml_db_init(deadbeef);
    ml_scanner_init(&plugin, deadbeef);
    ml_tree_init(deadbeef);
    ml_item_state_init(deadbeef);

    scriptableInitShared(); // FIXME: stop using shared
    ml_scriptable_init(deadbeef);

    return 0;
}

static int
ml_stop (void) {
    ml_scanner_free();
    ml_tree_free();

    scriptableDeinitShared();
    printf ("medialib cleanup done\n");

    return 0;
}

static int
ml_add_listener (ddb_mediasource_source_t *_source, ddb_medialib_listener_t listener, void *user_data) {
    medialib_source_t *source = (medialib_source_t *)_source;

    __block int result = -1;
    dispatch_sync(source->sync_queue, ^{
        for (int i = 0; i < MAX_LISTENERS; i++) {
            if (!source->ml_listeners[i]) {
                source->ml_listeners[i] = listener;
                source->ml_listeners_userdatas[i] = user_data;
                result = i;
                return;
            }
        }
    });
    return result;
}

static void
ml_remove_listener (ddb_mediasource_source_t *_source, int listener_id) {
    medialib_source_t *source = (medialib_source_t *)_source;

    dispatch_sync(source->sync_queue, ^{
        source->ml_listeners[listener_id] = NULL;
        source->ml_listeners_userdatas[listener_id] = NULL;
    });
}


static ddb_medialib_item_t *
ml_create_item_tree (ddb_mediasource_source_t *_source, ddb_mediasource_list_selector_t selector, const char *filter) {
    medialib_source_t *source = (medialib_source_t *)_source;

    __block ml_tree_item_t *root = NULL;

    dispatch_sync(source->sync_queue, ^{
        if (!source->enabled) {
            return;
        }

        medialibSelector_t index = (medialibSelector_t)selector;

        root = _create_item_tree_from_collection(filter, index, source);
    });

    return (ddb_medialib_item_t *)root;
}

#pragma mark - Select / Expand

static int
ml_is_tree_item_selected (ddb_mediasource_source_t *_source, const ddb_medialib_item_t *_item) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    const char *path = item->path;
    __block ml_collection_item_state_t state;
    dispatch_sync(source->sync_queue, ^{
        state = ml_item_state_get (&source->db.state, path);
    });
    return state.selected;
}

static void
ml_set_tree_item_selected (ddb_mediasource_source_t *_source, const ddb_medialib_item_t *_item, int selected) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    const char *path = item->path;
    dispatch_sync(source->sync_queue, ^{
        ml_collection_item_state_t *prev = NULL;
        ml_collection_item_state_t *state = ml_item_state_find (&source->db.state, path, &prev);
        int expanded = 0;
        if (state != NULL) {
            expanded = state->expanded;
        }
        ml_item_state_update (&source->db.state, path, state, prev, selected, expanded);
    });
}

static int
ml_is_tree_item_expanded (ddb_mediasource_source_t *_source, const ddb_medialib_item_t *_item) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    const char *path = item->path;
    __block ml_collection_item_state_t state;
    dispatch_sync(source->sync_queue, ^{
        state = ml_item_state_get (&source->db.state, path);
    });
    return state.expanded;
}

static void
ml_set_tree_item_expanded (ddb_mediasource_source_t *_source, const ddb_medialib_item_t *_item, int expanded) {
    medialib_source_t *source = (medialib_source_t *)_source;
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    const char *path = item->path;
    if (path == NULL) {
        return;
    }
    dispatch_sync(source->sync_queue, ^{
        ml_collection_item_state_t *prev = NULL;
        ml_collection_item_state_t *state = ml_item_state_find (&source->db.state, path, &prev);
        int selected = 0;
        if (state != NULL) {
            selected = state->selected;
        }
        ml_item_state_update (&source->db.state, path, state, prev, selected, expanded);
    });
}

#pragma mark -

#if 0
static ddb_playItem_t *
ml_find_track (medialib_source_t *source, ddb_playItem_t *it) {
    char track_uri[PATH_MAX];
    const char *uri = deadbeef->pl_find_meta (it, ":URI");

    const char *subsong = NULL;
    if (deadbeef->pl_get_item_flags (it) & DDB_IS_SUBTRACK) {
        subsong = deadbeef->pl_find_meta (it, ":TRACKNUM");
    }
    printf ("find lib track for key: %s\n", track_uri);
    const char *key = deadbeef->metacache_add_string (track_uri);

    ml_string_t *s = hash_find (source->db.track_uris.hash, key);

    ml_collection_item_t *item = NULL;
    if (s) {
        // find the one with correct subsong
        item = s->items;
        while (item) {
            if (!subsong) {
                break;
            }
            const char *item_subsong = deadbeef->pl_find_meta (it, ":TRACKNUM");
            if (item_subsong == subsong) {
                break;
            }
            item = item->next;
        }
    }
    if (item) {
        uri = deadbeef->pl_find_meta (item->it, ":URI");
        printf ("Found lib track %p (%s) for input track %p\n", item->it, uri, it);
    }
    else {
        printf ("Track not found in lib: %s\n", key);
    }
    deadbeef->metacache_remove_string (key);

    if (s) {
        deadbeef->pl_item_ref (s->items->it);
        return s->items->it;
    }

    return NULL;
}
#endif

static void *
ml_get_queries_scriptable(ddb_mediasource_source_t *_source) {
    return scriptableTFQueryRoot(scriptableRootShared());
}

static ddb_mediasource_state_t
ml_scanner_state (ddb_mediasource_source_t *_source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    return source->_ml_state;
}

static int
ml_message (uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    return 0;
}

#pragma mark - folder access

static void
ml_enable_saving(ddb_mediasource_source_t *_source, int enable) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        source->disable_file_operations = !enable;
    });
}

static unsigned
ml_folder_count (ddb_mediasource_source_t *_source) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block unsigned res = 0;
    dispatch_sync(source->sync_queue, ^{
        res = (unsigned)json_array_size(source->musicpaths_json);
    });
    return res;
}

static void
ml_folder_at_index (ddb_mediasource_source_t *_source, int index, char *folder, size_t size) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        json_t *data = json_array_get (source->musicpaths_json, index);
        *folder = 0;
        if (json_is_string (data)) {
            const char *musicdir = json_string_value (data);
            strncat(folder, musicdir, size);
        }
    });
}


static void
_save_folders_config (medialib_source_t *source) {
    char *dump = json_dumps(source->musicpaths_json, JSON_COMPACT);
    if (dump) {
        char conf_name[200];
        snprintf (conf_name, sizeof (conf_name), "%spaths", source->source_conf_prefix);
        deadbeef->conf_set_str (conf_name, dump);
        free (dump);
        dump = NULL;
        deadbeef->conf_save();
    }
}

static void
ml_set_folders (ddb_mediasource_source_t *_source, const char **folders, size_t count) {
    medialib_source_t *source = (medialib_source_t *)_source;
    dispatch_sync(source->sync_queue, ^{
        if (!source->musicpaths_json) {
            source->musicpaths_json = json_array();
        }

        json_array_clear(source->musicpaths_json);
        for (int i = 0; i < count; i++) {
            json_t *value = json_string(folders[i]);
            json_array_append(source->musicpaths_json, value);
            json_decref(value);
        }

        _save_folders_config(source);
    });
}

#pragma mark - Access to configured music folders
static char **
ml_get_folders (ddb_mediasource_source_t *_source, /* out */ size_t *_count) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block char **folders = NULL;
    __block size_t count = 0;
    dispatch_sync(source->sync_queue, ^{
        count = json_array_size(source->musicpaths_json);
        folders = calloc (count, sizeof (char *));
        for (int i = 0; i < count; i++) {
            json_t *data = json_array_get (source->musicpaths_json, i);
            if (json_is_string (data)) {
                folders[i] = strdup (json_string_value (data));
            }
        }
    });

    *_count = count;
    return folders;
}

static void
ml_free_folders (ddb_mediasource_source_t *_source, char **folders, size_t count) {
    for (int i = 0; i < count; i++) {
        free (folders[i]);
    }
    free (folders);
}

#pragma mark - Setting / changing music folders

static void
ml_insert_folder_at_index (ddb_mediasource_source_t *_source, const char *folder, int index) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int notify = 0;
    dispatch_sync(source->sync_queue, ^{
        json_t *value = json_string(folder);
        if (-1 != json_array_insert(source->musicpaths_json, index, value)) {
            notify = 1;
        }
        json_decref(value);
        _save_folders_config(source);
        ml_watch_fs_start(source);
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE);
    }
}

static void
ml_remove_folder_at_index (ddb_mediasource_source_t *_source, int index) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int notify = 0;
    dispatch_sync(source->sync_queue, ^{
        if (-1 != json_array_remove(source->musicpaths_json, index)) {
            notify = 1;
        }
        _save_folders_config(source);
        ml_watch_fs_start(source);
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE);
    }
}

static void
ml_append_folder (ddb_mediasource_source_t *_source, const char *folder) {
    medialib_source_t *source = (medialib_source_t *)_source;
    __block int notify = 0;
    dispatch_sync(source->sync_queue, ^{
        json_t *value = json_string(folder);
        if (-1 != json_array_append(source->musicpaths_json, value)) {
            notify = 1;
        }
        json_decref(value);
        _save_folders_config(source);
        ml_watch_fs_start(source);
    });
    if (notify) {
        ml_notify_listeners (source, DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE);
    }
}

#pragma mark -
static const char *
ml_tree_item_get_text (const ddb_medialib_item_t *_item) {
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    return item->text;
}

static ddb_playItem_t *
ml_tree_item_get_track (const ddb_medialib_item_t *_item) {
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    return item->track;
}

static const ddb_medialib_item_t *
ml_tree_item_get_next (const ddb_medialib_item_t *_item) {
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    return (ddb_medialib_item_t *)item->next;
}

static const ddb_medialib_item_t *
ml_tree_item_get_children (const ddb_medialib_item_t *_item) {
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    return (ddb_medialib_item_t *)item->children;
}

static int
ml_tree_item_get_children_count (const ddb_medialib_item_t *_item) {
    ml_tree_item_t *item = (ml_tree_item_t *)_item;
    return item->num_children;
}

#pragma mark -

ddb_medialib_plugin_api_t api = {
    ._size = sizeof(ddb_medialib_plugin_api_t),
    .enable_file_operations = ml_enable_saving,
    .folder_count = ml_folder_count,
    .folder_at_index = ml_folder_at_index,
    .set_folders = ml_set_folders,
    .get_folders = ml_get_folders,
    .free_folders = ml_free_folders,
    .insert_folder_at_index = ml_insert_folder_at_index,
    .remove_folder_at_index = ml_remove_folder_at_index,
    .append_folder = ml_append_folder,
};

static ddb_mediasource_api_t *
ml_get_api (void) {
    return (ddb_mediasource_api_t *)&api;
}

// define plugin interface
static DB_mediasource_t plugin = {
    .plugin.api_vmajor = DB_API_VERSION_MAJOR,
    .plugin.api_vminor = DB_API_VERSION_MINOR,
    .plugin.version_major = DDB_MEDIALIB_VERSION_MAJOR,
    .plugin.version_minor = DDB_MEDIALIB_VERSION_MINOR,
    .plugin.type = DB_PLUGIN_MEDIASOURCE,
    .plugin.id = "medialib",
    .plugin.name = "Media Library",
    .plugin.descr = "Scans disk for music files and manages them as database",
    .plugin.copyright =
        "Media Library plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2020 Oleksiy Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.connect = ml_connect,
    .plugin.start = ml_start,
    .plugin.stop = ml_stop,
    .plugin.message = ml_message,
    .get_extended_api = ml_get_api,
    .create_source = ml_create_source,
    .free_source = ml_free_source,
    .set_source_enabled = ml_set_source_enabled,
    .is_source_enabled = ml_is_source_enabled,
    .refresh = ml_refresh,
    .get_selectors_list = ml_get_selectors,
    .free_selectors_list = ml_free_selectors,
    .selector_name = ml_get_name_for_selector,
    .add_listener = ml_add_listener,
    .remove_listener = ml_remove_listener,
    .create_item_tree = ml_create_item_tree,
    .is_tree_item_selected = ml_is_tree_item_selected,
    .set_tree_item_selected = ml_set_tree_item_selected,
    .is_tree_item_expanded = ml_is_tree_item_expanded,
    .set_tree_item_expanded = ml_set_tree_item_expanded,
    .free_item_tree = ml_free_list,
    .get_queries_scriptable = ml_get_queries_scriptable,
    .scanner_state = ml_scanner_state,
    .tree_item_get_text = ml_tree_item_get_text,
    .tree_item_get_track = ml_tree_item_get_track,
    .tree_item_get_next = ml_tree_item_get_next,
    .tree_item_get_children = ml_tree_item_get_children,
    .tree_item_get_children_count = ml_tree_item_get_children_count,
};

DB_plugin_t *
medialib_load (DB_functions_t *api) {
    deadbeef = api;

    return DB_PLUGIN (&plugin);
}
