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

#ifndef gtkScriptablePropertySheetEditViewController_h
#define gtkScriptablePropertySheetEditViewController_h

#include <gtk/gtk.h>
#include "scriptable/scriptable.h"
#include "scriptable/scriptable_model.h"
#include "gtkScriptable.h"

// ViewController representing a property sheet editor of a single scriptable (e.g. DSP preset)

struct gtkScriptablePropertySheetEditViewController_t;
typedef struct gtkScriptablePropertySheetEditViewController_t gtkScriptablePropertySheetEditViewController_t;

typedef struct {
    void (*scriptable_did_change)(gtkScriptablePropertySheetEditViewController_t *view_controller, gtkScriptableChange_t change_type, void *context);
} gtkScriptablePropertySheetEditViewControllerDelegate_t;

gtkScriptablePropertySheetEditViewController_t *
gtkScriptablePropertySheetEditViewControllerNew (void);

void
gtkScriptablePropertySheetEditViewControllerFree (gtkScriptablePropertySheetEditViewController_t *self);

void
gtkScriptablePropertySheetEditViewControllerSetScriptable (
                                                gtkScriptablePropertySheetEditViewController_t *self,
                                                scriptableItem_t *scriptable);

void
gtkScriptablePropertySheetEditViewControllerSetDelegate (
                                              gtkScriptablePropertySheetEditViewController_t *self,
                                              gtkScriptablePropertySheetEditViewControllerDelegate_t *delegate,
                                              void *context);

GtkWidget *
gtkScriptablePropertySheetEditViewControllerGetView (gtkScriptablePropertySheetEditViewController_t *self);

#endif /* gtkScriptablePropertySheetEditViewController_h */
