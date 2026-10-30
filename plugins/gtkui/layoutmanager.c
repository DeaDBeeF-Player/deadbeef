/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include "layoutmanager.h"

#define LAYOUT_MAGIC "ddblayout"
#define LAYOUT_KEY "layout_gtkui"

static int
_make_directory (const char *path) {
    // on Windows, mkdir is redefined by shared/windows/mingw32_layer.h
    return mkdir (path, 0755);
}

int
layoutmanager_create_directory (const char *path) {
    if (path == NULL || path[0] == 0) {
        return -1;
    }

    char *tmp = strdup (path);
    if (tmp == NULL) {
        return -1;
    }

    // create the parents, ignore failures: these may already exist, or be unwritable but traversable
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/'
#ifdef _WIN32
            || *p == '\\'
#endif
        ) {
            char c = *p;
            *p = 0;
            _make_directory (tmp);
            *p = c;
        }
    }

    int res = -1;
    if (_make_directory (tmp) == 0 || errno == EEXIST) {
        struct stat st;
        if (stat (tmp, &st) == 0 && S_ISDIR (st.st_mode)) {
            res = 0;
        }
    }

    free (tmp);
    return res;
}

static layoutmanager_entry_t *
_entry_create (const char *dir, const char *file_name, size_t name_len) {
    layoutmanager_entry_t *entry = calloc (1, sizeof (layoutmanager_entry_t));
    if (entry == NULL) {
        return NULL;
    }

    entry->name = malloc (name_len + 1);
    size_t path_size = strlen (dir) + 1 + strlen (file_name) + 1;
    entry->path = malloc (path_size);
    if (entry->name == NULL || entry->path == NULL) {
        layoutmanager_entries_free (entry);
        return NULL;
    }
    memcpy (entry->name, file_name, name_len);
    entry->name[name_len] = 0;
    snprintf (entry->path, path_size, "%s/%s", dir, file_name);
    return entry;
}

layoutmanager_entry_t *
layoutmanager_scan_directory (const char *dir) {
    if (dir == NULL) {
        return NULL;
    }

    DIR *d = opendir (dir);
    if (d == NULL) {
        return NULL;
    }

    static const char ext[] = "." LAYOUTMANAGER_FILE_EXTENSION;
    const size_t ext_len = sizeof (ext) - 1;

    layoutmanager_entry_t *head = NULL;
    struct dirent *ent;
    while ((ent = readdir (d)) != NULL) {
        const char *file_name = ent->d_name;
        size_t len = strlen (file_name);
        if (file_name[0] == '.' || len <= ext_len || strcasecmp (file_name + len - ext_len, ext) != 0) {
            continue;
        }

        layoutmanager_entry_t *entry = _entry_create (dir, file_name, len - ext_len);
        if (entry == NULL) {
            continue;
        }

        // sorted insert
        layoutmanager_entry_t **pos = &head;
        while (*pos != NULL && strcasecmp ((*pos)->name, entry->name) <= 0) {
            pos = &(*pos)->next;
        }
        entry->next = *pos;
        *pos = entry;
    }
    closedir (d);

    return head;
}

void
layoutmanager_entries_free (layoutmanager_entry_t *entries) {
    while (entries != NULL) {
        layoutmanager_entry_t *next = entries->next;
        free (entries->name);
        free (entries->path);
        free (entries);
        entries = next;
    }
}

// Parses "major.minor.point", each component being a non-negative number. Returns 0 on success.
static int
_parse_version (const char *str, long version[3]) {
    if (str == NULL) {
        return -1;
    }

    const char *p = str;
    for (int i = 0; i < 3; i++) {
        if (*p < '0' || *p > '9') {
            return -1;
        }
        long n = 0;
        while (*p >= '0' && *p <= '9') {
            n = n * 10 + (*p - '0');
            if (n > 1000000) {
                return -1;
            }
            p++;
        }
        version[i] = n;

        if (i < 2) {
            if (*p != '.') {
                return -1;
            }
            p++;
        }
    }
    return *p == 0 ? 0 : -1;
}

// Returns <0, 0, >0 like strcmp
static int
_compare_versions (const long a[3], const long b[3]) {
    for (int i = 0; i < 3; i++) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

json_t *
layoutmanager_load_file (const char *path, const char *current_version, layoutmanager_error_t *error) {
    layoutmanager_error_t err = LAYOUTMANAGER_OK;
    json_t *root = NULL;
    json_t *layout = NULL;

    FILE *fp = fopen (path, "rb");
    if (fp == NULL) {
        err = LAYOUTMANAGER_ERROR_IO;
        goto error;
    }
    root = json_loadf (fp, 0, NULL);
    fclose (fp);

    if (!json_is_object (root)) {
        err = LAYOUTMANAGER_ERROR_INVALID_FILE;
        goto error;
    }

    const char *magic = json_string_value (json_object_get (root, "magic"));
    if (magic == NULL || strcmp (magic, LAYOUT_MAGIC) != 0) {
        err = LAYOUTMANAGER_ERROR_WRONG_MAGIC;
        goto error;
    }

    long file_version[3];
    long supported_version[3];
    if (_parse_version (json_string_value (json_object_get (root, "version")), file_version) != 0
        || _parse_version (current_version, supported_version) != 0) {
        err = LAYOUTMANAGER_ERROR_INVALID_FILE;
        goto error;
    }
    if (_compare_versions (file_version, supported_version) > 0) {
        err = LAYOUTMANAGER_ERROR_UNSUPPORTED_VERSION;
        goto error;
    }

    layout = json_object_get (root, LAYOUT_KEY);
    if (!json_is_object (layout)) {
        layout = NULL;
        err = LAYOUTMANAGER_ERROR_INVALID_FILE;
        goto error;
    }
    json_incref (layout);
    json_decref (root);

    if (error != NULL) {
        *error = LAYOUTMANAGER_OK;
    }
    return layout;

error:
    if (root != NULL) {
        json_decref (root);
    }
    if (error != NULL) {
        *error = err;
    }
    return NULL;
}

layoutmanager_error_t
layoutmanager_save_file (const char *path, const char *version, json_t *layout) {
    json_t *root = json_object ();
    if (root == NULL) {
        return LAYOUTMANAGER_ERROR_IO;
    }
    json_object_set_new (root, "magic", json_string (LAYOUT_MAGIC));
    json_object_set_new (root, "version", json_string (version));
    json_object_set (root, LAYOUT_KEY, layout);

    int res = json_dump_file (root, path, JSON_INDENT (2) | JSON_SORT_KEYS);
    json_decref (root);

    return res == 0 ? LAYOUTMANAGER_OK : LAYOUTMANAGER_ERROR_IO;
}
