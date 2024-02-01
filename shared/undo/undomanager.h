/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2022 Oleksiy Yakovenko and other contributors

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

#ifndef ddb_undomanager_h
#define ddb_undomanager_h

#include "undobuffer.h"

struct ddb_undomanager_s;

typedef struct ddb_undomanager_s ddb_undomanager_t;

ddb_undomanager_t *
ddb_undomanager_alloc (void);

void
ddb_undomanager_free (ddb_undomanager_t *undomanager);

ddb_undobuffer_t *
ddb_undomanager_get_buffer (ddb_undomanager_t *undomanager);

ddb_undobuffer_t *
ddb_undomanager_consume_buffer (ddb_undomanager_t *undomanager);

void
ddb_undomanager_set_action_name (ddb_undomanager_t *undomanager, const char *name);

const char *
ddb_undomanager_get_action_name (ddb_undomanager_t *undomanager);

#pragma mark - Shared instance

ddb_undomanager_t *
ddb_undomanager_shared (void);

void
ddb_undomanager_shared_init (ddb_undomanager_t *undomanager);

#endif /* ddb_undomanager_h */
