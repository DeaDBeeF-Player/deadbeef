/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2023 Oleksiy Yakovenko and other contributors

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

#ifndef scriptable_model_h
#define scriptable_model_h

#include "scriptable.h"
#include <deadbeef/deadbeef.h>

// Observable model interface for scriptables

struct scriptableModel_t;
typedef struct scriptableModel_t scriptableModel_t;

typedef struct scriptableModelAPI_t {
    char *(*get_active_name) (scriptableModel_t *model);
    void (*set_active_name) (scriptableModel_t *model, const char *active_name);
    int64_t (*add_listener) (
        scriptableModel_t *model,
        void (*listener) (scriptableModel_t *model, void *user_data),
        void *user_data);
    void (*remove_listener) (scriptableModel_t *model, int64_t listener);
} scriptableModelAPI_t;

scriptableModel_t *
scriptableModelAlloc (void);

scriptableModel_t *
scriptableModelInit (scriptableModel_t *model, DB_functions_t *deadbeef, const char *config_name_active);

void
scriptableModelFree (scriptableModel_t *model);

scriptableModelAPI_t *
scriptableModelGetAPI (scriptableModel_t *model);

#endif /* scriptable_model_h */
