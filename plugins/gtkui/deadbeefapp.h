/*
    DeaDBeeF -- the music player
    GtkApplication implementation
    Copyright (C) 2017 Nicolai Syvertsen

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
#ifndef __GTKUI_DEADBEEFAPP_H
#define __GTKUI_DEADBEEFAPP_H

#include <gtk/gtk.h>

#define DEADBEEF_APP_TYPE (deadbeef_app_get_type ())
GType deadbeef_app_get_type (void);
typedef struct _DeadbeefApp DeadbeefApp;
typedef struct { GtkApplicationClass parent_class; } DeadbeefAppClass;

static inline DeadbeefApp * DEADBEEF_APP(gpointer ptr) {
    return G_TYPE_CHECK_INSTANCE_CAST (ptr, deadbeef_app_get_type (), DeadbeefApp);
}
// Only available since GLib 2.44:
//G_DECLARE_FINAL_TYPE (DeadbeefApp, deadbeef_app, DEADBEEF, APP, GtkApplication)


DeadbeefApp *
deadbeef_app_new (void);

GSimpleAction *
deadbeef_app_get_log_action(DeadbeefApp *application);

#endif /*__GTKUI_DEADBEEFAPP_H*/
