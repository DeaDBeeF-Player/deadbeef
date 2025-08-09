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

#ifndef gtkScriptableListEditViewController_h
#define gtkScriptableListEditViewController_h

#include <gtk/gtk.h>
#include "scriptable/scriptable.h"
#include "gtkScriptable.h"

// Represents an editor view for a list of scriptables:
// list of presets,
// button bar: add, remove, duplicate, configure,
// optionally "Save as preset" button.

// Lifecycle:
// New
// [optional] SetDelegate
// Load
// Free

// SetScriptable can be called at any point after New

struct gtkScriptableListEditViewController_t;
typedef struct gtkScriptableListEditViewController_t gtkScriptableListEditViewController_t;

typedef struct {
    void (*add_buttons)(gtkScriptableListEditViewController_t *view_controller, GtkBox *button_box, void *context);
    void (*scriptable_did_change)(gtkScriptableListEditViewController_t *view_controller, gtkScriptableChange_t change_type, void *context);
} gtkScriptableListEditViewControllerDelegate_t;

gtkScriptableListEditViewController_t *
gtkScriptableListEditViewControllerNew (void);

void
gtkScriptableListEditViewControllerFree (gtkScriptableListEditViewController_t *self);

void
gtkScriptableListEditViewControllerSetDelegate(gtkScriptableListEditViewController_t *self, gtkScriptableListEditViewControllerDelegate_t *delegate, void *context);

void
gtkScriptableListEditViewControllerLoad (gtkScriptableListEditViewController_t *self);

GtkWidget *
gtkScriptableListEditViewControllerGetView(gtkScriptableListEditViewController_t *self);

void
gtkScriptableListEditViewControllerSetScriptable(gtkScriptableListEditViewController_t *self, scriptableItem_t *scriptable);

scriptableItem_t *
gtkScriptableListEditViewControllerGetScriptable(gtkScriptableListEditViewController_t *self);

#endif /* gtkScriptableListEditViewController_h */
