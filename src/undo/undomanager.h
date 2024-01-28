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

#ifndef undomanager_h
#define undomanager_h

#include "undobuffer.h"

struct undomanager_s;

typedef struct undomanager_s undomanager_t;

undomanager_t *
undomanager_alloc (void);

void
undomanager_free (undomanager_t *undomanager);

undobuffer_t *
undomanager_get_buffer (undomanager_t *undomanager);

undobuffer_t *
undomanager_consume_buffer (undomanager_t *undomanager);

// Send the accumulated undo buffer to the UI for registration
void
undomanager_flush(undomanager_t *undomanager);

void
undomanager_set_action_name (undomanager_t *undomanager, const char *name);

const char *
undomanager_get_action_name (undomanager_t *undomanager);


#pragma mark - Shared instance

undomanager_t *
undomanager_shared (void);

void
undomanager_shared_init (void);

#endif /* undomanager_h */
