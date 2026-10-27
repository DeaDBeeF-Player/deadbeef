/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2025 Oleksiy Yakovenko and other contributors

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

#ifndef ogg_shared_h
#define ogg_shared_h

#include <deadbeef/deadbeef.h>
#include "artwork.h"

/// Quickly checks if the comment is a METADATA_BLOCK_PICTURE with a supported image type.
/// It only decodes the beginning of the picture block, so it's cheap to use for counting the pictures.
/// Returns 1 if it is.
int
ogg_artwork_comment_is_picture (const char *comment, int comment_length);

/// Decodes a METADATA_BLOCK_PICTURE comment into @c cover. Returns 0 on success.
int
ogg_parse_artwork_comment (const char *comment, int comment_length, ddb_cover_info_t *cover);

#endif /* ogg_shared_h */
