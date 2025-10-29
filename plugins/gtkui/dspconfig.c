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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <deadbeef/deadbeef.h>
#include "gtkui.h"
#include "pluginconf.h"

#include "../../shared/scriptable/scriptable.h"
#include "../../shared/scriptable/scriptable_dsp.h"
#include "scriptable/gtkScriptableListEditViewController.h"
#include "scriptable/gtkScriptableSelectViewController.h"

static GtkWidget *_prefwin;
static gtkScriptableListEditViewController_t *_listView;
static gtkScriptableListEditViewControllerDelegate_t _listViewDelegate;

static gtkScriptableSelectViewController_t *_selectView;
static scriptableModel_t *_selectViewScriptableModel;
static gtkScriptableSelectViewControllerDelegate_t _selectViewDelegate;

static scriptableItem_t *_current_dsp_chain;
static gboolean _is_saving_new_preset;

static void
_update_current_dsp_chain(ddb_dsp_context_t *chain) {
    if (_current_dsp_chain != NULL) {
        scriptableItemFree(_current_dsp_chain);
        _current_dsp_chain = NULL;
    }
    _current_dsp_chain = scriptableDspPresetFromDspChain (chain);
}

void
_save_as_preset(GtkButton *button,
                gpointer user_data) {
    if (_current_dsp_chain == NULL) {
        return;
    }
    GtkWidget *dlg = create_entrydialog ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _ ("New preset"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL (e), _ ("Name:"));
    e = lookup_widget (dlg, "title");
    gtk_entry_set_text (GTK_ENTRY (e), _("New Preset"));
    int res = gtk_dialog_run (GTK_DIALOG (dlg));
    if (res == GTK_RESPONSE_OK) {
        const char *name = gtk_entry_get_text (GTK_ENTRY (e));
        _is_saving_new_preset = TRUE;
        scriptableItem_t *preset = scriptableItemClone (_current_dsp_chain);
        scriptableItem_t *presetList = scriptableDspRoot(deadbeef->get_shared_scriptable_root());
        scriptableItemSetUniqueNameUsingPrefixAndRoot(preset, name, presetList);
        scriptableItemAddSubItem(presetList, preset);
        gtkScriptableSelectViewControllerSetScriptable(_selectView, presetList);
        gtkScriptableSelectViewControllerSelectItem(_selectView, preset);
        _is_saving_new_preset = FALSE;
    }
    gtk_widget_destroy (dlg);
}

static void _add_buttons (gtkScriptableListEditViewController_t *view_controller, GtkBox *button_box, void *context) {
    GtkWidget *button = gtk_button_new_with_label(_("Save as preset"));
    gtk_box_pack_end(button_box, button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    g_signal_connect (button, "clicked", G_CALLBACK (_save_as_preset), NULL);
}

static void _list_scriptable_did_change (gtkScriptableListEditViewController_t *view_controller, gtkScriptableChange_t change_type, void *context) {
    // create dsp chain from the new state

    scriptableItem_t *item = gtkScriptableListEditViewControllerGetScriptable(view_controller);
    ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (item);
    deadbeef->streamer_set_dsp_chain (chain);
    deadbeef->dsp_preset_free (chain);
}

static void _selection_did_change (gtkScriptableSelectViewController_t *vc, scriptableItem_t *item, void *context) {
    if (_is_saving_new_preset) {
        return;
    }
    ddb_dsp_context_t *chain = scriptableDspConfigToDspChain (item);
    deadbeef->streamer_set_dsp_chain (chain);

    _update_current_dsp_chain(chain);

    gtkScriptableListEditViewControllerSetScriptable(_listView, _current_dsp_chain);

    deadbeef->dsp_preset_free (chain);
}

static void _select_scriptable_did_change (
                               gtkScriptableSelectViewController_t *view_controller,
                               gtkScriptableChange_t change_type,
                                    void *context) {
    // unused / no need to do anything
}

void
dsp_setup_chain_changed (void) {
    // The quirk here is that we can't replace current scriptable(s) here,
    // since it can be in use by multiple nested dialogs.
    // So we do nothing, and reload only when config dialog reopens.
    //
    // Example what this would break:
    // * Open DSP node editor, then click reset
    // * This would update streamer DSP chain
    // * This would in turn call this method
    // * This would delete old scriptable, and corrupt current editor
}

void
dsp_setup_init (GtkWidget *prefwin) {
    _prefwin = prefwin;

    GtkWidget *vbox = lookup_widget(prefwin, "prefwin_dsp_vbox");

    // Stop listening
    _listViewDelegate.scriptable_did_change = NULL;
    _selectViewDelegate.scriptable_did_change = NULL;
    _selectViewDelegate.selection_did_change = NULL;


    // Editor for current dsp chain

    _listView = gtkScriptableListEditViewControllerNew();

    _listViewDelegate.add_buttons = _add_buttons;

    gtkScriptableListEditViewControllerSetDelegate(_listView, &_listViewDelegate, NULL);

    ddb_dsp_context_t *chain = deadbeef->streamer_get_dsp_chain ();
    _update_current_dsp_chain(chain);

    gtkScriptableListEditViewControllerLoad(_listView);
    gtkScriptableListEditViewControllerSetScriptable(_listView, _current_dsp_chain);

    GtkWidget *listViewWidget = gtkScriptableListEditViewControllerGetView(_listView);

    gtk_box_pack_start(GTK_BOX(vbox), listViewWidget, TRUE, TRUE, 0);

    // Preset selector

    _selectView = gtkScriptableSelectViewControllerNew();

    gtkScriptableSelectViewControllerSetDelegate(_selectView, &_selectViewDelegate, NULL);

    scriptableItem_t *presetList = scriptableDspRoot(deadbeef->get_shared_scriptable_root());
    gtkScriptableSelectViewControllerSetScriptable(_selectView, presetList);

    _selectViewScriptableModel = scriptableModelInit (scriptableModelAlloc (), deadbeef, "dsp.preset");
    gtkScriptableSelectViewControllerSetModel (_selectView, _selectViewScriptableModel);

    GtkWidget *selectViewWidget = gtkScriptableSelectViewControllerGetView(_selectView);

    GtkWidget *selectorHBox = gtk_hbox_new(FALSE, 8);
    gtk_widget_show(selectorHBox);

    GtkWidget *label = gtk_label_new(_("DSP Preset:"));
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(selectorHBox), label, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(selectorHBox), selectViewWidget, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), selectorHBox, FALSE, TRUE, 0);

    // Start listening to changes
    _listViewDelegate.scriptable_did_change = _list_scriptable_did_change;
    _selectViewDelegate.scriptable_did_change = _select_scriptable_did_change;
    _selectViewDelegate.selection_did_change = _selection_did_change;
}

void
dsp_setup_free (void) {
    if (_listView != NULL) {
        gtkScriptableListEditViewControllerFree(_listView);
        _listView = NULL;
    }
    if (_selectView != NULL) {
        gtkScriptableSelectViewControllerFree(_selectView);
        _selectView = NULL;
    }
    if (_selectViewScriptableModel != NULL) {
        scriptableModelFree(_selectViewScriptableModel);
        _selectViewScriptableModel = NULL;
    }
}
