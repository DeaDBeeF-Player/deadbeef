/*
    Album Art plugin for DeaDBeeF
    Copyright (C) 2009-2011 Viktor Semykin <thesame.ml@gmail.com>
    Copyright (C) 2009-2013 Alexey Yakovenko <waker@users.sourceforge.net>

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
#ifndef __ARTWORK_INTERNAL_H
#define __ARTWORK_INTERNAL_H

#include "../../deadbeef.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define FETCH_CONCURRENT_LIMIT 1

size_t artwork_http_request(const char *url, char *buffer, const size_t max_bytes);

void artwork_abort_all_http_requests (void);

int ensure_dir(const char *path);
int copy_file (const char *in, const char *out);
int write_file(const char *out, const char *data, const size_t data_length);

#endif /*__ARTWORK_INTERNAL_H*/
