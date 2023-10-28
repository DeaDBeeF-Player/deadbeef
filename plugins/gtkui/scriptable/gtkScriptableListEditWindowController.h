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

#ifndef gtkScriptableListEditWindowController_h
#define gtkScriptableListEditWindowController_h

#include <gtk/gtk.h>
#include "scriptable/scriptable.h"

// A window displaying a list of items from a scriptable.
// Titlebar,
// list (edit list viewcontroller)
// optional reset button,
// close button

struct gtkScriptableListEditWindowController_t;
typedef struct gtkScriptableListEditWindowController_t gtkScriptableListEditWindowController_t;

typedef struct {
    void (*window_did_close)(gtkScriptableListEditWindowController_t *controller, void *context);
} gtkScriptableListEditWindowControllerDelegate_t;

gtkScriptableListEditWindowController_t *
gtkScriptableListEditWindowControllerNew (void);

void
gtkScriptableListEditWindowControllerFree (gtkScriptableListEditWindowController_t *self);

void
gtkScriptableListEditWindowControllerRunModal(gtkScriptableListEditWindowController_t *self, GtkWindow *modal_parent);

void
gtkScriptableListEditWindowControllerSetScriptable(gtkScriptableListEditWindowController_t *self, scriptableItem_t *scriptable);

void
gtkScriptableListEditWindowControllerSetTitle(gtkScriptableListEditWindowController_t *self, const char *title);

void
gtkScriptableListEditWindowControllerSetDelegate(gtkScriptableListEditWindowController_t *self, gtkScriptableListEditWindowControllerDelegate_t *delegate, void *context);

#endif /* gtkScriptableListEditWindowController_h */
