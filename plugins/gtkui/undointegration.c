/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2024 Oleksiy Yakovenko and other contributors

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

#include "gtkui.h"
#include "support.h"
#include "undointegration.h"
#include "undostack.h"

extern DB_functions_t *deadbeef;

ddb_undo_interface_t *ddb_undo;

static void
_undo_initialize (ddb_undo_interface_t *interface) {
    ddb_undo = interface;
}

static int
_undo_process_action (struct ddb_undobuffer_s *undobuffer, const char *action_name) {
    gtkui_undostack_append_buffer (undobuffer, action_name);
    refresh_undo_redo_menu ();
    return 0;
}

static ddb_undo_hooks_t _undo_hooks = {
    ._size = sizeof (ddb_undo_hooks_t),
    .initialize = _undo_initialize,
    .process_action = _undo_process_action,
};

void undo_integration_init (void) {
    deadbeef->register_for_undo (&_undo_hooks);
}

void
refresh_undo_redo_menu (void) {
    GtkWidget *undo = lookup_widget (mainwin, "undo");
    GtkWidget *redo = lookup_widget (mainwin, "redo");

    int has_undo = gtkui_undostack_has_undo ();
    int has_redo = gtkui_undostack_has_redo ();
    gtk_widget_set_sensitive (undo, has_undo);
    gtk_widget_set_sensitive (redo, has_redo);

    const char *undo_action_name = gtkui_undostack_get_undo_action_name ();
    const char *redo_action_name = gtkui_undostack_get_redo_action_name ();

    char text[100];
    if (has_undo && undo_action_name != NULL) {
        snprintf (text, sizeof (text), _("Undo %s"), undo_action_name);
        gtk_menu_item_set_label (GTK_MENU_ITEM (undo), text);
    }
    else {
        gtk_menu_item_set_label (GTK_MENU_ITEM (undo), _("Undo"));
    }
    if (has_redo && redo_action_name != NULL) {
        snprintf (text, sizeof (text), _("Redo %s"), redo_action_name);
        gtk_menu_item_set_label (GTK_MENU_ITEM (redo), text);
    }
    else {
        gtk_menu_item_set_label (GTK_MENU_ITEM (redo), _("Redo"));
    }
}

