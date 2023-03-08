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

#ifndef __ACTIONS_H
#define __ACTIONS_H

#include <gtk/gtk.h>
#include <deadbeef/deadbeef.h>

void
add_mainmenu_actions (void);

typedef void (* menu_action_activate_callback_t) (GtkMenuItem *menuitem, DB_plugin_action_t *action);

/// Add plugin action items to the existing menu.
/// @return The number of items added
int
menu_add_action_items(GtkWidget *menu, int selected_count, ddb_playItem_t *selected_track, ddb_action_context_t action_context, menu_action_activate_callback_t activate_callback);

void
gtkui_exec_action_14 (DB_plugin_action_t *action, int cursor);

#endif
