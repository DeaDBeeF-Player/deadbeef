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

#ifndef layoutmanager_h
#define layoutmanager_h

#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif

// Reading, writing and listing of .ddblayout files.
// File format: {"magic":"ddblayout","version":"x.y.z","layout_gtkui":{...}}
// This file doesn't depend on GTK.

#define LAYOUTMANAGER_FILE_EXTENSION "ddblayout"

typedef enum {
    LAYOUTMANAGER_OK = 0,
    LAYOUTMANAGER_ERROR_IO,
    LAYOUTMANAGER_ERROR_INVALID_FILE,
    LAYOUTMANAGER_ERROR_WRONG_MAGIC,
    LAYOUTMANAGER_ERROR_UNSUPPORTED_VERSION,
} layoutmanager_error_t;

typedef struct layoutmanager_entry_s {
    char *name; // file name without extension
    char *path;
    struct layoutmanager_entry_s *next;
} layoutmanager_entry_t;

// Returns a list of .ddblayout files in the directory, sorted by name. Returns NULL if there are none,
// or the directory doesn't exist. Free with layoutmanager_entries_free.
layoutmanager_entry_t *
layoutmanager_scan_directory (const char *dir);

void
layoutmanager_entries_free (layoutmanager_entry_t *entries);

// Creates the directory with all missing parents. Returns 0 on success.
int
layoutmanager_create_directory (const char *path);

// Loads the file, checks the magic, and checks that the file version (major.minor.point) is not higher than
// current_version. Returns a new reference to the layout object, or NULL on error.
json_t *
layoutmanager_load_file (const char *path, const char *current_version, layoutmanager_error_t *error);

// Writes the layout (the reference is not stolen) into the file. Returns LAYOUTMANAGER_OK on success.
layoutmanager_error_t
layoutmanager_save_file (const char *path, const char *version, json_t *layout);

#ifdef __cplusplus
}
#endif

#endif /* layoutmanager_h */
