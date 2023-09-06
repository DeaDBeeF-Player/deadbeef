/*
    Media Library plugin for DeaDBeeF Player
    Copyright (C) 2009-2017 Oleksiy Yakovenko

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

#ifndef medialib_h
#define medialib_h

#include <deadbeef/deadbeef.h>

#define DDB_MEDIALIB_VERSION_MAJOR 1
#define DDB_MEDIALIB_VERSION_MINOR 0

typedef enum {
    DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE = DDB_MEDIASOURCE_EVENT_MAX+1,
} ddb_medialib_mediasource_event_type_t;

typedef struct ddb_medialib_plugin_priv_s ddb_medialib_plugin_priv_t;

typedef struct ddb_medialib_plugin_api_s {
    // Size of this structure
    int _size;

    /// Primarily for debugging and testing, enable or disable reading or writing the database files.
    /// Default is Enabled.
    void (*enable_file_operations)(ddb_mediasource_source_t *source, int enable);

    // The mediasource must be treated as the source of truth for the folders configuration.
    // Use DDB_MEDIALIB_MEDIASOURCE_EVENT_FOLDERS_DID_CHANGE event to know when the folders change.
    unsigned (*folder_count)(ddb_mediasource_source_t *source);

    void (*folder_at_index)(ddb_mediasource_source_t *source, int index, char *folder, size_t size);

    void (*set_folders) (ddb_mediasource_source_t *source, const char **folders, size_t count);

    char **(*get_folders) (ddb_mediasource_source_t *source, /* out */ size_t *count);

    void (*free_folders) (ddb_mediasource_source_t *source, char **folders, size_t count);

    void (*insert_folder_at_index) (ddb_mediasource_source_t *source, const char *folder, int index);

    void (*remove_folder_at_index) (ddb_mediasource_source_t *source, int index);

    void (*append_folder) (ddb_mediasource_source_t *source, const char *folder);
} ddb_medialib_plugin_api_t;

#endif /* medialib_h */
