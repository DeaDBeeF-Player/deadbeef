/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

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

#ifndef __ARTWORK_H
#define __ARTWORK_H

#include <limits.h>
#include <stdint.h>
#include <time.h>
#include "../../deadbeef.h"

#define DDB_ARTWORK_MAJOR_VERSION 2
#define DDB_ARTWORK_MINOR_VERSION 0

/// The flags below can be used in the `flags` member of the `ddb_cover_query_t` structure,
/// and can be OR'ed together.
///
/// Example usage: `DDB_ARTWORK_FLAG_NO_FILENAME | DDB_ARTWORK_FLAG_LOAD_BLOB`
/// This indicates that all results must be loaded into memory, and returned as blob.
///
/// Another example: `DDB_ARTWORK_FLAG_LOAD_BLOB`
/// This indicates that both blob and filename must be returned.
/// However, in some cases filenames are not available, e.g. when loading from tags.
/// In this case filename will be set to NULL.
enum {
    /// Tells that filenames should not be returned
    DDB_ARTWORK_FLAG_NO_FILENAME = (1<<0),

    /// Returned artwork can be a blob, i.e. a memory block - that is, entire cover image in memory
    DDB_ARTWORK_FLAG_LOAD_BLOB = (1<<1),

    /// Loading of the cover was cancelled, and result should be ignored
    DDB_ARTWORK_FLAG_CANCELLED = (1<<2),
};

/// This structure needs to be passed to cover_get.
/// It must remain in memory until the callback is called.
typedef struct ddb_cover_query_s {
    /// Must be set to sizeof(ddb_cover_query_t)
    uint32_t _size;

    // Arbitrary user-defined pointer
    void *user_data;

    /// DDB_ARTWORK_FLAG_*; When 0 is passed, it will use the global settings.
    /// By default, it means that the files can be stored in disk cache,
    /// and returned result is always a filename.
    uint32_t flags;
    /// The track to load artwork for
    ddb_playItem_t *track;

    /// A unique ID identifying the source of the query. This allows to cancel all queries for a single source.
    int64_t source_id;
} ddb_cover_query_t;

/// This structure is passed to the callback, when the artwork query has been processed.
/// It doesn't need to be freed by the caller
typedef struct ddb_cover_info_s {
    // query info
    time_t timestamp; // Last time when the info was used last time
    char filepath[PATH_MAX];
    char album[1000];
    char artist[1000];
    char title[1000];
    int is_compilation;

    int cover_found; // set to 1 if the cover was found

    char *type; // A type of image, e.g. "front" or "back" (can be NULL)

    char *image_filename; // A name of file with the image

    char *blob; // A blob with the image data, or NULL
    uint64_t blob_size; // Size of the blob
    uint64_t blob_image_offset; // offset where the image data starts in the blob
    uint64_t blob_image_size; // size of the image at offset

    struct ddb_cover_info_s *next; // The next image in the chain, or NULL

    // FIXME: this struct is inheritable, add padding and size
} ddb_cover_info_t;

/// The `error` is 0 on success, or negative value on failure.
/// The `query` will be the same pointer, as passed to `cover_get`,
/// remember to free it when done with it.
/// The `cover` is a artwork information, e.g. a filename, or a blob,
/// remember for call artwork_plugin->cover_info_free (cover) when done with it.
typedef void (*ddb_cover_callback_t) (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover);

typedef enum {
    /// The listener should reset its artwork cache and redraw.
    /// If p1 is 0, the entire cache did reset,
    /// otherwise p1 is ddb_playItem_t pointer,
    /// and only this one specific track needs to be invalidated.
    DDB_ARTWORK_SETTINGS_DID_CHANGE = 1,
} ddb_artwork_listener_event_t;

typedef void (*ddb_artwork_listener_t) (ddb_artwork_listener_event_t event, void *user_data, int64_t p1, int64_t p2);

typedef struct {
    DB_misc_t plugin;

    /// The @c cover_get function adds the query into an internal queue,
    /// then the queue gets processed on another thread, i.e. asynchronously.
    ///
    /// When the query is processed, the supplied @c callback is called
    /// with the results in @c ddb_cover_info_t structure.
    ///
    /// The callback is guaranteed to be called,
    /// because the caller is responsible for memory management of the @c query argument.
    ///
    /// The callback is not executed on the same thread, as cover_get.
    /// Avoid running slow blocking code in the callbacks.
    void
    (*cover_get) (ddb_cover_query_t *query, ddb_cover_callback_t callback);

    /// Clears the current queue, calling the callback with no results for each item.
    void
    (*reset) (void);

    /// Release the dynamically allocated data pointed by @c cover.
    void
    (*cover_info_release) (ddb_cover_info_t *cover);

    void
    (*add_listener) (ddb_artwork_listener_t listener, void *user_data);

    void
    (*remove_listener) (ddb_artwork_listener_t listener, void *user_data);

    /// Get the default image path to use when the cover is not available
    ///
    /// @c path needs to point to a sufficiently large buffer to contain the result.
    /// Result can be an empty string.
    void
    (*default_image_path) (char *path, size_t size);

    /// Returns a new unique ID, which can be used to set @c source_id of the queries, and cancel queries in bulk.
    /// Deallocation is not required, the function will return a new unique value every time.
    int64_t
    (*allocate_source_id) (void);

    /// Cancel all queries with the specified source_id
    void
    (*cancel_queries_with_source_id) (int64_t source_id);
} ddb_artwork_plugin_t;

#endif /*__ARTWORK_H*/

