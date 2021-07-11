/*
    Media Library plugin for DeaDBeeF Player
    Copyright (C) 2009-2017 Alexey Yakovenko

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

#include "../../deadbeef.h"

#define DDB_MEDIALIB_VERSION_MAJOR 1
#define DDB_MEDIALIB_VERSION_MINOR 0

typedef struct ddb_medialib_plugin_s {
    DB_mediasource_t plugin;
#pragma mark - Configuration

    /// Primarily for debugging and testing, enable or disable reading or writing the database files.
    /// Default is Enabled.
    void (*enable_file_operations)(ddb_mediasource_source_t source, int enable);

    unsigned (*folder_count)(ddb_mediasource_source_t source);

    void (*folder_at_index)(ddb_mediasource_source_t source, int index, char *folder, size_t size);

    void (*set_folders) (ddb_mediasource_source_t source, const char **folders, size_t count);
} ddb_medialib_plugin_t;

#endif /* medialib_h */
