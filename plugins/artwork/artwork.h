/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Oleksiy Yakovenko and other contributors

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

#include <stdint.h>
#include <time.h>

#define DDB_ARTWORK_MAJOR_VERSION 2
#define DDB_ARTWORK_MINOR_VERSION 1

/// The flags below can be used in the `flags` member of the `ddb_cover_query_t` structure,
/// and can be OR'ed together.
enum {
    /// Loading of the cover was cancelled, and result should be ignored
    DDB_ARTWORK_FLAG_CANCELLED = (1<<0),

    /// Only look for the cover embedded in the track's file: skip local image files and online services.
    /// Added in API 2.1.
    DDB_ARTWORK_ONLY_EMBEDDED = (1<<1),

    /// Load the cover at the index specified in `ddb_cover_query_t.cover_index`.
    /// Implies @c DDB_ARTWORK_ONLY_EMBEDDED.
    /// Without this flag the @c cover_index is ignored, and the best cover is searched using all the enabled sources.
    /// Added in API 2.1.
    DDB_ARTWORK_USE_COVER_INDEX = (1<<2),

    /// Don't use the cache at all: the cached covers are not read, and the loaded covers are not written to the cache.
    /// The covers embedded in the tags are returned in memory (@c image_data), instead of a cache file.
    /// The covers found in local folders are returned as is, without copying them to the cache.
    /// The online services are not queried, because they rely on the cache.
    /// Added in API 2.1.
    DDB_ARTWORK_SKIP_CACHE = (1<<3),
};

/// Cover types, reported in `ddb_cover_info_t.cover_type`.
/// The types other than @c DDB_ARTWORK_COVER_TYPE_DEFAULT correspond to the ID3v2 APIC picture types,
/// which are also used by FLAC, Ogg Vorbis, Opus, and WMA.
/// Added in API 2.1.
typedef enum {
    /// The type is not known, e.g. the cover was found in a local folder, or in a tag format without cover types
    DDB_ARTWORK_COVER_TYPE_DEFAULT = 0,
    DDB_ARTWORK_COVER_TYPE_OTHER = 1,
    /// 32x32 PNG file icon
    DDB_ARTWORK_COVER_TYPE_FILE_ICON = 2,
    DDB_ARTWORK_COVER_TYPE_OTHER_FILE_ICON = 3,
    DDB_ARTWORK_COVER_TYPE_FRONT = 4,
    DDB_ARTWORK_COVER_TYPE_BACK = 5,
    DDB_ARTWORK_COVER_TYPE_LEAFLET_PAGE = 6,
    /// e.g. label side of CD
    DDB_ARTWORK_COVER_TYPE_MEDIA = 7,
    /// Lead artist / lead performer / soloist
    DDB_ARTWORK_COVER_TYPE_LEAD_ARTIST = 8,
    /// Artist / performer
    DDB_ARTWORK_COVER_TYPE_ARTIST = 9,
    DDB_ARTWORK_COVER_TYPE_CONDUCTOR = 10,
    /// Band / orchestra
    DDB_ARTWORK_COVER_TYPE_BAND = 11,
    DDB_ARTWORK_COVER_TYPE_COMPOSER = 12,
    /// Lyricist / text writer
    DDB_ARTWORK_COVER_TYPE_LYRICIST = 13,
    DDB_ARTWORK_COVER_TYPE_RECORDING_LOCATION = 14,
    DDB_ARTWORK_COVER_TYPE_DURING_RECORDING = 15,
    DDB_ARTWORK_COVER_TYPE_DURING_PERFORMANCE = 16,
    /// Movie / video screen capture
    DDB_ARTWORK_COVER_TYPE_VIDEO_SCREEN_CAPTURE = 17,
    /// A bright coloured fish
    DDB_ARTWORK_COVER_TYPE_BRIGHT_FISH = 18,
    DDB_ARTWORK_COVER_TYPE_ILLUSTRATION = 19,
    /// Band / artist logotype
    DDB_ARTWORK_COVER_TYPE_BAND_LOGO = 20,
    /// Publisher / studio logotype
    DDB_ARTWORK_COVER_TYPE_PUBLISHER_LOGO = 21,
} ddb_artwork_cover_type_t;

/// This structure needs to be passed to cover_get.
/// It must remain in memory until the callback is called.
typedef struct ddb_cover_query_s {
    /// Size of this struct
    uint32_t _size;

    // Arbitrary user-defined pointer
    void *user_data;

    /// A combination of DDB_ARTWORK_FLAG_* flags
    /// Pass 0 here.
    /// The DDB_ARTWORK_FLAG_CANCELLED flag may be added after the query was processed.
    uint32_t flags;

    /// The track to load artwork for
    ddb_playItem_t *track;

    /// A unique ID identifying the source of the query. This allows to cancel all queries for a single source.
    int64_t source_id;

    /// The index of the cover to load, from 0 to the number returned by @c cover_count_in_track minus 1.
    /// Only used when the @c DDB_ARTWORK_USE_COVER_INDEX flag is set, in which case the index must be valid:
    /// the query fails otherwise, including when @c _size doesn't cover this field.
    /// Supported for covers embedded in FLAC, ID3v2, APEv2, MP4, Ogg Vorbis and Opus tags.
    /// Added in API 2.1.
    int cover_index;
} ddb_cover_query_t;

typedef struct ddb_cover_info_priv_s ddb_cover_info_priv_t;

/// This structure is passed to the callback, when the artwork query has been processed.
/// It doesn't need to be freed by the caller
typedef struct ddb_cover_info_s {
    /// Size of this struct
    size_t _size;

    ddb_cover_info_priv_t *priv;

    int cover_found; // set to 1 if the cover was found

    char *image_filename; // A name of file with the image, can be NULL if @c image_data is set

    /// The type of the cover, if known; @c DDB_ARTWORK_COVER_TYPE_DEFAULT otherwise.
    /// Added in API 2.1: only access it if @c _size covers this field.
    ddb_artwork_cover_type_t cover_type;

    /// The image data, set instead of @c image_filename when the cover is loaded in memory,
    /// i.e. when the @c DDB_ARTWORK_SKIP_CACHE flag is used.
    /// It remains valid until the @c cover is released.
    /// Added in API 2.1: only access it if @c _size covers this field.
    const void *image_data;
    size_t image_data_size;
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

    // Added in API 2.1: check that `plugin.version_minor` is at least 1 before calling.

    /// Returns the number of covers embedded in the file of the @c track.
    /// The covers can be loaded by passing their index in `ddb_cover_query_t.cover_index`.
    /// This function is synchronous, and reads the file.
    /// If the file has covers in multiple tag formats, the first format in this order is used:
    /// FLAC, ID3v2, APEv2, MP4, Ogg Vorbis, Opus.
    int
    (*cover_count_in_track) (ddb_playItem_t *track);
} ddb_artwork_plugin_t;

#endif /*__ARTWORK_H*/

