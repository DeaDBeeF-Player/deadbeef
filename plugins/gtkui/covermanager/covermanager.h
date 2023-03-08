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

#ifndef covermanager_h
#define covermanager_h

#include <gtk/gtk.h>
#include <stdint.h>
#include <dispatch/dispatch.h>
#include <deadbeef/deadbeef.h>

typedef struct covermanager_s covermanager_t;

/// Called by @c covermanager_cover_for_track when the cover is ready.
/// The @c img argument is not retained, and will be released after the block completes.
typedef void (^covermanager_completion_block_t)(GdkPixbuf *img);

covermanager_t *
covermanager_shared(void);

void
covermanager_shared_free(void);

covermanager_t *
covermanager_new(void);

void
covermanager_free (covermanager_t *manager);

/// Gets the cover from in-memory cache, or initiates asynchronous request to cache it.
///
/// If the cover is immediately available -- it will be returned (retained), and the @c completion_block will not be called.
/// Otherwise the @c completion_block will be called when the requests completes.
GdkPixbuf *
covermanager_cover_for_track(covermanager_t *manager, DB_playItem_t *track, int64_t source_id, covermanager_completion_block_t completion_block);

/// Create scaled image with specified dimensions. Returns retained object.
GdkPixbuf *
covermanager_create_scaled_image (covermanager_t *manager, GdkPixbuf *image, GtkAllocation size);

/// Calculate the desired image size for specified available size.
GtkAllocation
covermanager_desired_size_for_image_size (covermanager_t *manager, GtkAllocation image_size, GtkAllocation availableSize);

void
covermanager_terminate(covermanager_t *manager);

#endif /* covermanager_h */
