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

#ifndef gtkScriptableSelectViewController_h
#define gtkScriptableSelectViewController_h

#include <gtk/gtk.h>
#include "scriptable/scriptable.h"
#include "gtkScriptable.h"

// ViewController representing a Dropdown box + edit button
// (E.g. choose from a list of presets / edit the list)

struct gtkScriptableSelectViewController_t;
typedef struct gtkScriptableSelectViewController_t gtkScriptableSelectViewController_t;

typedef struct {
    void (*selection_did_change) (gtkScriptableSelectViewController_t *vc, scriptableItem_t *item, void *context);
    void (*scriptable_did_change) (
        gtkScriptableSelectViewController_t *view_controller,
        gtkScriptableChange_t change_type,
        void *context);
} gtkScriptableSelectViewControllerDelegate_t;

gtkScriptableSelectViewController_t *
gtkScriptableSelectViewControllerNew (void);

void
gtkScriptableSelectViewControllerFree (gtkScriptableSelectViewController_t *self);

void
gtkScriptableSelectViewControllerSetScriptable (
    gtkScriptableSelectViewController_t *self,
    scriptableItem_t *scriptable);

void
gtkScriptableSelectViewControllerSetDelegate (
    gtkScriptableSelectViewController_t *self,
    gtkScriptableSelectViewControllerDelegate_t *delegate,
    void *context);

void
gtkScriptableSelectViewControllerLoad (gtkScriptableSelectViewController_t *self);

GtkWidget *
gtkScriptableSelectViewControllerGetView (gtkScriptableSelectViewController_t *self);

void
gtkScriptableSelectViewControllerSelectItem (gtkScriptableSelectViewController_t *self, scriptableItem_t *item);

int
gtkScriptableSelectViewControllerIndexOfSelectedItem (gtkScriptableSelectViewController_t *self);

#endif /* gtkScriptableSelectViewController_h */
