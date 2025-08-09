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

#ifndef gtkScriptablePropertySheetEditWindowController_h
#define gtkScriptablePropertySheetEditWindowController_h

#include <gtk/gtk.h>
#include "scriptable/scriptable.h"
#include "gtkScriptable.h"

// A window displaying a property sheet editor of a scriptable.
// Titlebar,
// properties (property sheet view controller),
// optional reset button,
// close button

struct gtkScriptablePropertySheetEditWindowController_t;
typedef struct gtkScriptablePropertySheetEditWindowController_t gtkScriptablePropertySheetEditWindowController_t;

typedef struct {
    void (*window_did_close)(gtkScriptablePropertySheetEditWindowController_t *controller, void *context);
    void (*scriptable_did_change)(gtkScriptablePropertySheetEditWindowController_t *view_controller, gtkScriptableChange_t change_type, void *context);
} gtkScriptablePropertySheetEditWindowControllerDelegate_t;

gtkScriptablePropertySheetEditWindowController_t *
gtkScriptablePropertySheetEditWindowControllerNew (void);

void
gtkScriptablePropertySheetEditWindowControllerFree (gtkScriptablePropertySheetEditWindowController_t *self);

void
gtkScriptablePropertySheetEditWindowControllerRunModal(gtkScriptablePropertySheetEditWindowController_t *self, GtkWindow *modal_parent);

void
gtkScriptablePropertySheetEditWindowControllerSetScriptable(gtkScriptablePropertySheetEditWindowController_t *self, scriptableItem_t *scriptable);

void
gtkScriptablePropertySheetEditWindowControllerSetTitle(gtkScriptablePropertySheetEditWindowController_t *self, const char *title);

void
gtkScriptablePropertySheetEditWindowControllerSetDelegate(gtkScriptablePropertySheetEditWindowController_t *self, gtkScriptablePropertySheetEditWindowControllerDelegate_t *delegate, void *context);

#endif /* gtkScriptablePropertySheetEditWindowController_h */
