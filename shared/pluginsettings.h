/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#ifndef __deadbeef__pluginsettings__
#define __deadbeef__pluginsettings__

#define MAX_SETTINGS_ITEMS 50

// types
enum {
    PROP_GROUP,
    PROP_SUBMENU,
    PROP_ACTION,
    PROP_ENTRY,
    PROP_PASSWORD,
    PROP_CHECKBOX,
    PROP_FILE,
    PROP_DIR,
    PROP_SELECT,
    PROP_SLIDER, // hscale, vscale, spinbtn
    PROP_ITEMLIST, // abstract list of items, with a type: List<Type>
    PROP_ITEMSELECT,
    PROP_HBOX,
    PROP_VBOX,
};

typedef struct {
    int type;
    char *key;
    char *title;
    char *def;

    // for "select": list separated with spaces, ends with ';'
    // for vscale, hscale, spinbtn: contents of the [] markers, ends with ']'
    const char *select_options;
    // always ends with >, e.g. "DSPPreset>"
    char *itemlist_type;
} settings_property_t;

typedef struct {
    const char *title;
    settings_property_t props[MAX_SETTINGS_ITEMS];
    int nprops;
} settings_data_t;

int
settings_data_init (settings_data_t *settings_data, const char *layout);

void
settings_data_add_property (settings_data_t *settings_data, int type, const char *key, const char *title, const char *def);

void
settings_data_free (settings_data_t *settings_data);

#endif /* defined(__deadbeef__pluginsettings__) */
