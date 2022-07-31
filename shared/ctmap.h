/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2019 Oleksiy Yakovenko and other contributors

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

#ifndef ctmapping_h
#define ctmapping_h

#define DDB_CTMAP_MAX_PLUGINS 5

typedef struct ddb_ctmap_s {
    char *ct;
    char *plugins[DDB_CTMAP_MAX_PLUGINS];
    struct ddb_ctmap_s *next;
} ddb_ctmap_t;

ddb_ctmap_t *
ddb_ctmap_init_from_string (const char *mapstr);

void
ddb_ctmap_free (ddb_ctmap_t *ctmap);

#endif /* ctmapping_h */
