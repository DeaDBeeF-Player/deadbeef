/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

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
#include <assert.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include "../../deadbeef.h"
#include "../../gettext.h"
#include "../../shared/trkproperties_shared.h"
#include "callbacks.h"
#include "ddbcellrenderertextmultiline.h"
#include "gtkui.h"
#include "interface.h"
#include "playlist/ddblistview.h"
#include "playlist/mainplaylist.h"
#include "search.h"
#include "support.h"
#include "tagwritersettings.h"
#include "trkproperties.h"
#include "wingeom.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

static GtkWidget *trackproperties;
static GtkCellRenderer *rend_text2;
static GtkListStore *store;
static GtkListStore *propstore;
int trkproperties_modified;
static DB_playItem_t **tracks;
static int numtracks;
static GtkWidget *progressdlg;
static int progress_aborted;
static int last_ctx;
static ddb_playlist_t *last_plt;
static trkproperties_delegate_t *_delegate;

// Max length of a string displayed in the TableView
// If a string is longer -- it gets clipped, and appended with " (…)", like with linebreaks
#define MAX_GUI_FIELD_LEN 500

static char *
clip_multiline_value (const char *v) {
    char *clipped_val = NULL;
    size_t l = strlen (v);
    const char multiline_ellipsis[] = " (…)";
    int i;
    for (i = 0; i < l; i++) {
        if (v[i] == '\r' || v[i] == '\n') {
            break;
        }
    }

    if (l >= MAX_GUI_FIELD_LEN && (i == l || i >= MAX_GUI_FIELD_LEN)) {
        i = MAX_GUI_FIELD_LEN;
    }

    if (i != l) {
        clipped_val = malloc (i + sizeof (multiline_ellipsis));
        memcpy (clipped_val, v, i);
        memcpy (clipped_val + i, multiline_ellipsis, sizeof (multiline_ellipsis));
    }
    return clipped_val;
}


gboolean
on_trackproperties_delete_event        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    if (trkproperties_modified) {
        GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (trackproperties), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("You've modified data for this track."));
        gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (trackproperties));
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("Really close the window?"));
        gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

        int response = gtk_dialog_run (GTK_DIALOG (dlg));
        gtk_widget_destroy (dlg);
        if (response != GTK_RESPONSE_YES) {
            return TRUE;
        }
    }
    gtk_widget_destroy (widget);
    rend_text2 = NULL;
    trackproperties = NULL;
    if (tracks) {
        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }
        free (tracks);
        tracks = NULL;
        numtracks = 0;
    }
    return TRUE;
}

int trkproperties_block_keyhandler = 0;

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
    if (trkproperties_block_keyhandler) {
        return FALSE;
    }
    if (event->keyval == GDK_Escape) {
        on_trackproperties_delete_event (trackproperties, NULL, NULL);
        return TRUE;
    }
    else if (event->keyval == GDK_Delete) {
        on_trkproperties_remove_activate (NULL, NULL);
        return TRUE;
    }
    else if (event->keyval == GDK_Insert) {
        on_trkproperties_add_new_field_activate (NULL, NULL);
        return TRUE;
    }
    return FALSE;
}

void
trkproperties_destroy (void) {
    if (trackproperties) {
        on_trackproperties_delete_event (trackproperties, NULL, NULL);
    }
    if (last_plt) {
        deadbeef->plt_unref (last_plt);
        last_plt = NULL;
    }
    last_ctx = -1;
}

void
on_closebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    trkproperties_destroy ();
}

static void
update_meta_iter_with_edited_value (GtkTreeIter *iter, const char *new_text) {
    char *clipped_val = clip_multiline_value (new_text);
    if (!clipped_val) {
        gtk_list_store_set (store, iter, 1, new_text, 3, 0, 4, new_text, -1);
    }
    else {
        gtk_list_store_set (store, iter, 1, clipped_val, 3, 0, 4, new_text, -1);
        free (clipped_val);
    }
}


void
on_metadata_edited (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
    GtkTreeIter iter;

    if (!treepath) {
        return;
    }

    gboolean valid = gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, treepath);
    gtk_tree_path_free (treepath);

    if (!valid) {
        return;
    }

    GValue value = {0,};
    GValue mult = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 4, &value);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 3, &mult);
    const char *svalue = g_value_get_string (&value);
    if (!svalue) {
        svalue = "";
    }

    // The multiple values case gets cleared on attempt to edit,
    // that's why the change gets applied unconditionally for multivalue case
    int imult = g_value_get_int (&mult);
    if (strcmp (svalue, new_text) || imult) {
        update_meta_iter_with_edited_value (&iter, new_text);
        trkproperties_modified = 1;
    }

    G_IS_VALUE (&value) ? ((void)(g_value_unset (&value)), NULL) : NULL;
    G_IS_VALUE (&mult) ? ((void)(g_value_unset (&mult)), NULL) : NULL;
    trkproperties_block_keyhandler = 0;
}

void
add_field (GtkListStore *store, const char *key, const char *title, int is_prop, DB_playItem_t **tracks, int numtracks) {
    // get value to edit
    const char *mult = is_prop ? "" : _("[Multiple values] ");
    char val[5000];
    size_t ml = strlen (mult);
    memcpy (val, mult, ml+1);
    int n = trkproperties_get_field_value (val + ml, (int)(sizeof (val) - ml), key, tracks, numtracks);

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    if (!is_prop) {
        if (n) {
            char *clipped_val = clip_multiline_value (val);
            if (!clipped_val) {
                gtk_list_store_set (store, &iter, 0, title, 1, val, 2, key, 3, n ? 1 : 0, 4, val, -1);
            }
            else {
                gtk_list_store_set (store, &iter, 0, title, 1, clipped_val, 2, key, 3, n ? 1 : 0, 4, val, -1);
                free (clipped_val);
            }
        }
        else {
            char *v = val + ml;
            char *clipped_val = clip_multiline_value (v);
            if (!clipped_val) {
                gtk_list_store_set (store, &iter, 0, title, 1, v,  2, key, 3, n ? 1 : 0, 4, v, -1);
            }
            else {
                gtk_list_store_set (store, &iter, 0, title, 1, clipped_val, 2, key, 3, n ? 1 : 0, 4, v, -1);
                free (clipped_val);
            }
        }
    }
    else {
        gtk_list_store_set (store, &iter, 0, title, 1, n ? val : val + ml, -1);
    }
}

void
trkproperties_fill_meta (GtkListStore *store, DB_playItem_t **tracks, int numtracks) {
    gtk_list_store_clear (store);
    if (!tracks) {
        return;
    }

    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 0, tracks, numtracks);

    // add "standard" fields
    for (int i = 0; trkproperties_types[i]; i += 2) {
        add_field (store, trkproperties_types[i], _(trkproperties_types[i+1]), 0, tracks, numtracks);
    }

    // add all other fields
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; trkproperties_types[i]; i += 2) {
            if (!strcasecmp (keys[k], trkproperties_types[i])) {
                break;
            }
        }
        if (trkproperties_types[i]) {
            continue;
        }

        size_t l = strlen (keys[k]);
        char title[l + 3];
        snprintf (title, sizeof (title), "<%s>", keys[k]);
        add_field (store, keys[k], title, 0, tracks, numtracks);
    }
    if (keys) {
        free (keys);
    }
}

void
trkproperties_fill_metadata (void) {
    if (!trackproperties) {
        return;
    }
    trkproperties_modified = 0;
    deadbeef->pl_lock ();

    trkproperties_fill_meta (store, tracks, numtracks);
    gtk_list_store_clear (propstore);

    // hardcoded properties
    for (int i = 0; trkproperties_hc_props[i]; i += 2) {
        add_field (propstore, trkproperties_hc_props[i], _(trkproperties_hc_props[i+1]), 1, tracks, numtracks);
    }
    // properties
    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 1, tracks, numtracks);
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; trkproperties_hc_props[i]; i += 2) {
            if (!strcasecmp (keys[k], trkproperties_hc_props[i])) {
                break;
            }
        }
        if (trkproperties_hc_props[i]) {
            continue;
        }
        size_t l = strlen (keys[k]);
        char title[l + 3];
        snprintf (title, sizeof (title), "<%s>", keys[k]+1);
        add_field (propstore, keys[k], title, 1, tracks, numtracks);
    }
    if (keys) {
        free (keys);
    }

    deadbeef->pl_unlock ();
}

void
meta_value_transform_func (GtkTreeViewColumn *tree_column,
        GtkCellRenderer *cell,
        GtkTreeModel *tree_model,
        GtkTreeIter *iter,
        gpointer data) {
}

void
show_track_properties_dlg_with_current_track_list (void) {
    GtkTreeView *tree;
    GtkTreeView *proptree;
    if (!trackproperties) {
        trackproperties = create_trackproperties ();
        gtk_window_set_transient_for (GTK_WINDOW (trackproperties), GTK_WINDOW (mainwin));
        wingeom_restore (trackproperties, "trkproperties", -1, -1, 300, 400, 0);

        // metadata tree
        tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
        store = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING);
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
        GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
        rend_text2 = GTK_CELL_RENDERER (ddb_cell_renderer_text_multiline_new ());
        g_object_set (G_OBJECT (rend_text2), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        g_signal_connect ((gpointer)rend_text2, "edited",
                          G_CALLBACK (on_metadata_edited),
                          store);
        GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend_text, "text", 0, NULL);
        GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_text2, "text", 1, NULL);

        //gtk_tree_view_column_set_cell_data_func (col2, rend_text2, meta_value_transform_func, NULL, NULL);

        gtk_tree_view_append_column (tree, col1);
        gtk_tree_view_append_column (tree, col2);

        // properties tree
        proptree = GTK_TREE_VIEW (lookup_widget (trackproperties, "properties"));
        propstore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        gtk_tree_view_set_model (proptree, GTK_TREE_MODEL (propstore));
        GtkCellRenderer *rend_propkey = gtk_cell_renderer_text_new ();
        GtkCellRenderer *rend_propvalue = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (rend_propvalue), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend_propkey, "text", 0, NULL);
        col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_propvalue, "text", 1, NULL);
        gtk_tree_view_append_column (proptree, col1);
        gtk_tree_view_append_column (proptree, col2);
    }
    else {
        tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
        store = GTK_LIST_STORE (gtk_tree_view_get_model (tree));
        gtk_list_store_clear (store);
        proptree = GTK_TREE_VIEW (lookup_widget (trackproperties, "properties"));
        propstore = GTK_LIST_STORE (gtk_tree_view_get_model (proptree));
        gtk_list_store_clear (propstore);
    }

    if (numtracks == 1) {
        deadbeef->pl_lock ();
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (trackproperties, "filename")), deadbeef->pl_find_meta_raw (tracks[0], ":URI"));
        deadbeef->pl_unlock ();
    }
    else {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (trackproperties, "filename")), _("[Multiple values]"));
    }

    g_object_set (G_OBJECT (rend_text2), "editable", TRUE, NULL);

    GtkWidget *widget = trackproperties;
    trkproperties_fill_metadata ();

    gtk_widget_set_sensitive (lookup_widget (widget, "write_tags"), TRUE);

    gtk_widget_show (widget);
    gtk_window_present (GTK_WINDOW (widget));
}

void
show_track_properties_dlg_with_track_list (ddb_playItem_t **track_list, int count) {
    trkproperties_free_track_list (&tracks, &numtracks);
    tracks = calloc (count, sizeof (ddb_playItem_t *));
    for (int i = 0; i < count; i++) {
        tracks[i] = track_list[i];
        deadbeef->pl_item_ref (tracks[i]);
    }
    numtracks = count;
    show_track_properties_dlg_with_current_track_list();
    _delegate = NULL;
}

void
show_track_properties_dlg (int ctx, ddb_playlist_t *plt) {
    last_ctx = ctx;
    deadbeef->plt_ref (plt);
    if (last_plt) {
        deadbeef->plt_unref (last_plt);
    }
    last_plt = plt;

    trkproperties_free_track_list (&tracks, &numtracks);

    trkproperties_build_track_list_for_ctx (plt, ctx, &tracks, &numtracks);

    show_track_properties_dlg_with_current_track_list();
    _delegate = NULL;
}

void
trkproperties_set_delegate (trkproperties_delegate_t *delegate) {
    _delegate = delegate;
}


static gboolean
set_metadata_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    GValue mult = {0,};
    gtk_tree_model_get_value (model, iter, 3, &mult);
    int smult = g_value_get_int (&mult);
    if (!smult) {
        GValue key = {0,}, value = {0,};
        gtk_tree_model_get_value (model, iter, 2, &key);
        gtk_tree_model_get_value (model, iter, 4, &value);
        const char *skey = g_value_get_string (&key);
        const char *svalue = g_value_get_string (&value);

        int num_values = 1;
        for (int i = 0; svalue[i]; i++) {
            if (svalue[i] == ';') {
                num_values++;
            }
        }

        char **values = calloc (num_values, sizeof (char *));
        const char *p = svalue;
        const char *e = p;
        int n = 0;
        do {
            while (*e && *e != ';') {
                e++;
            }

            // trim
            const char *rb = p;
            const char *re = e-1;
            while (rb <= re) {
                if ((uint8_t)(*rb) > 0x20) {
                    break;
                }
                rb++;
            }
            while (re >= rb) {
                if ((uint8_t)(*re) > 0x20) {
                    break;
                }
                re--;
            }

            if (rb <= re) {
                re++;
                char *v = malloc (re-rb+1);
                memcpy (v, rb, re-rb);
                v[re-rb] = 0;
                values[n++] = v;
            }

            if (!*e) {
                break;
            }
            p = ++e;
        } while (*p);

        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_delete_meta (tracks[i], skey);
            if (*svalue) {
                for (int k = 0; k < n; k++) {
                    if (values[k] && *values[k]) {
                        deadbeef->pl_append_meta (tracks[i], skey, values[k]);
                    }
                }
            }
        }

        for (int k = 0; k < n; k++) {
            if (values[k]) {
                free (values[k]);
            }
        }
        free (values);
    }

    return FALSE;
}

static gboolean
write_finished_cb (void *ctx) {
    gtk_widget_destroy (progressdlg);
    progressdlg = NULL;
    trkproperties_modified = 0;
    if (last_plt) {
        deadbeef->plt_modified (last_plt);
    }
    if (_delegate != NULL) {
        _delegate->trkproperties_did_update_tracks (_delegate->user_data);
    }

    show_track_properties_dlg_with_current_track_list();

    return FALSE;
}

static gboolean
set_progress_cb (void *ctx) {
    DB_playItem_t *track = ctx;
    GtkWidget *progressitem = lookup_widget (progressdlg, "progresstitle");
    deadbeef->pl_lock ();
    gtk_entry_set_text (GTK_ENTRY (progressitem), deadbeef->pl_find_meta_raw (track, ":URI"));
    deadbeef->pl_unlock ();
    deadbeef->pl_item_unref (track);
    return FALSE;
}

static void
write_meta_worker (void *ctx) {
    for (int t = 0; t < numtracks; t++) {
        if (progress_aborted) {
            break;
        }
        DB_playItem_t *track = tracks[t];
        deadbeef->pl_lock ();
        const char *dec = deadbeef->pl_find_meta_raw (track, ":DECODER");
        char decoder_id[100];
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = track && dec;
        deadbeef->pl_unlock ();
        if (match) {
            int is_subtrack = deadbeef->pl_get_item_flags (track) & DDB_IS_SUBTRACK;
            if (is_subtrack) {
                continue;
            }
            deadbeef->pl_item_ref (track);
            g_idle_add (set_progress_cb, track);
            // find decoder
            DB_decoder_t *dec = NULL;
            DB_decoder_t **decoders = deadbeef->plug_get_decoder_list ();
            for (int i = 0; decoders[i]; i++) {
                if (!strcmp (decoders[i]->plugin.id, decoder_id)) {
                    dec = decoders[i];
                    if (dec->write_metadata) {
                        dec->write_metadata (track);
                    }
                    break;
                }
            }
        }
    }
    g_idle_add (write_finished_cb, ctx);
}

static gboolean
on_progress_delete_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    progress_aborted = 1;
    return gtk_widget_hide_on_delete (widget);
}

static void
on_progress_abort                      (GtkButton       *button,
                                        gpointer         user_data)
{
    progress_aborted = 1;
}

void
on_write_tags_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
    deadbeef->pl_lock ();
    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    GtkTreeModel *model = GTK_TREE_MODEL (gtk_tree_view_get_model (tree));

    // delete all metadata properties that are not in the listview
    for (int i = 0; i < numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (tracks[i]);
        while (meta) {
            DB_metaInfo_t *next = meta->next;
            if (meta->key[0] != ':' && meta->key[0] != '!' && meta->key[0] != '_') {
                GtkTreeIter iter;
                gboolean res = gtk_tree_model_get_iter_first (model, &iter);
                while (res) {
                    GValue key = {0,};
                    gtk_tree_model_get_value (model, &iter, 2, &key);
                    const char *skey = g_value_get_string (&key);

                    if (!strcasecmp (skey, meta->key)) {
                        // field found, don't delete
                        break;
                    }
                    res = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
                }
                if (!res) {
                    // field not found, delete
                    deadbeef->pl_delete_metadata (tracks[i], meta);
                }
            }
            meta = next;
        }
    }
    // put all metainfo into track
    gtk_tree_model_foreach (model, set_metadata_cb, NULL);
    deadbeef->pl_unlock ();

    if (numtracks > 25) {
        deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
    }
    else {
        for (int i = 0; i < numtracks; i++) {
            ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
            ev->track = tracks[i];
            deadbeef->pl_item_ref (ev->track);
            deadbeef->event_send ((ddb_event_t*)ev, 0, 0);
        }
    }

    progress_aborted = 0;
    progressdlg = create_progressdlg ();
    gtk_window_set_title (GTK_WINDOW (progressdlg), _("Writing tags..."));

    g_signal_connect ((gpointer) progressdlg, "delete_event",
            G_CALLBACK (on_progress_delete_event),
            NULL);
    GtkWidget *cancelbtn = lookup_widget (progressdlg, "cancelbtn");
    g_signal_connect ((gpointer) cancelbtn, "clicked",
            G_CALLBACK (on_progress_abort),
            NULL);

    gtk_widget_show_all (progressdlg);
    gtk_window_present (GTK_WINDOW (progressdlg));
    gtk_window_set_transient_for (GTK_WINDOW (progressdlg), GTK_WINDOW (trackproperties));

    // start new thread for writing metadata
    intptr_t tid = deadbeef->thread_start (write_meta_worker, NULL);
    deadbeef->thread_detach (tid);
}

gboolean
on_metalist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 3) {
        GtkWidget *menu = create_trkproperties_popup_menu ();
        if (numtracks != 1) {
            gtk_widget_set_sensitive (lookup_widget (menu, "trkproperties_edit"), FALSE);
        }
        gtk_menu_attach_to_widget (GTK_MENU (menu), GTK_WIDGET (widget), NULL);
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, gtk_get_current_event_time());
    }
    return FALSE;
}

void
on_tagwriter_settings_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
    run_tagwriter_settings (trackproperties);
}

gboolean
on_trackproperties_configure_event     (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data)
{
    wingeom_save (widget, "trkproperties");
    return FALSE;
}


gboolean
on_trackproperties_window_state_event  (GtkWidget       *widget,
                                        GdkEventWindowState *event,
                                        gpointer         user_data)
{
    wingeom_save_max (event, widget, "trkproperties");
    return FALSE;
}

void
on_trkproperties_edit_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (numtracks != 1) {
        return; // TODO: multiple track editing support
    }

    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    GtkTreeSelection *sel = gtk_tree_view_get_selection (treeview);
    int count = gtk_tree_selection_count_selected_rows (sel);
    if (count != 1) {
        return; // multiple fields can't be edited at the same time
    }


    GtkWidget *dlg = create_edit_tag_value_dlg ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (trackproperties));
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    GList *lst = gtk_tree_selection_get_selected_rows (sel, NULL);

    GtkTreePath *path = lst->data;

    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
    GValue key = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &key);
    GValue value = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 4, &value);
    const char *skey = g_value_get_string (&key);
    const char *svalue = g_value_get_string (&value);

    char *uppercase_key = strdup (skey);
    for (char *p = uppercase_key; *p; p++) {
        *p = toupper (*p);
    }

    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "field_name")), uppercase_key);

    free (uppercase_key);

    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
    gtk_text_buffer_set_text (buffer, svalue, (gint)strlen (svalue));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (lookup_widget (dlg, "field_value")), buffer);

    g_value_unset (&key);
    g_value_unset (&value);

    for (GList *l = lst; l; l = l->next) {
        gtk_tree_path_free (l->data);
    }
    g_list_free (lst);

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        GtkTextIter begin, end;

        gtk_text_buffer_get_start_iter (buffer, &begin);
        gtk_text_buffer_get_end_iter (buffer, &end);

        char *new_text = gtk_text_buffer_get_text (buffer, &begin, &end, TRUE);

        update_meta_iter_with_edited_value (&iter, new_text);

        free (new_text);

        trkproperties_modified = 1;
    }
    g_object_unref (buffer);
    gtk_widget_destroy (dlg);
}


void
on_trkproperties_edit_in_place_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    GtkTreePath *path;
    gtk_tree_view_get_cursor (treeview, &path, NULL);
    if (!path) {
        return;
    }

    GtkTreeViewColumn *col = gtk_tree_view_get_column (treeview, 1);

    gtk_tree_view_set_cursor (treeview, path, col, TRUE); // set cursor onto new field
    gtk_tree_path_free(path);
}


void
on_trkproperties_remove_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    if (!gtk_widget_is_focus(GTK_WIDGET (treeview))) {
        return; // do not remove field if Metadata tab is not focused
    }
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        return;
    }

    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
    GValue value = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &value);
    const char *svalue = g_value_get_string (&value);

    // delete unknown fields completely; otherwise just clear
    int i = 0;
    for (; trkproperties_types[i]; i += 2) {
        if (!strcasecmp (svalue, trkproperties_types[i])) {
            break;
        }
    }
    if (trkproperties_types[i]) { // known val, clear
        gtk_list_store_set (store, &iter, 1, "", 3, 0, 4, "", -1);
    }
    else {
        gtk_list_store_remove (store, &iter);
    }
    gtk_tree_view_set_cursor (treeview, path, NULL, FALSE); // restore cursor after deletion
    gtk_tree_path_free (path);
    trkproperties_modified = 1;
}


void
on_trkproperties_cut_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_copy_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_paste_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_capitalize_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_clean_up_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_format_from_other_fields_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_add_new_field_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkNotebook *notebook = GTK_NOTEBOOK(lookup_widget(trackproperties, "trkproperties_notebook"));
    if (gtk_notebook_get_current_page(notebook) != 0) {
        return; // do not add field if Metadata tab is not focused
    }
    GtkWidget *dlg = create_entrydialog ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (trackproperties));
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Field name"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL(e), _("Name:"));
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    for (;;) {
        int res = gtk_dialog_run (GTK_DIALOG (dlg));
        if (res == GTK_RESPONSE_OK) {
            e = lookup_widget (dlg, "title");

            const char *text = gtk_entry_get_text (GTK_ENTRY(e));

            GtkTreeIter iter;

            // check for _ and :
            if (text[0] == '_' || text[0] == ':' || text[0] == '!') {
                GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW (dlg), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Field names must not start with : or _"));
                gtk_window_set_title (GTK_WINDOW (d), _("Cannot add field"));

                gtk_dialog_run (GTK_DIALOG (d));
                gtk_widget_destroy (d);
                continue;
            }

            // check if a field with the same name already exists
            int dup = 0;
            gboolean res = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
            while (res) {
                GValue value = {0,};
                gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &value);
                const char *svalue = g_value_get_string (&value);
                if (!strcasecmp (svalue, text)) {
                    dup = 1;
                    break;
                }
                res = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
            }

            if (!dup) {
                size_t l = strlen (text);
                char title[l+3];
                snprintf (title, sizeof (title), "<%s>", text);
                const char *value = "";
                const char *key = text;

                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter, 0, title, 1, value, 2, key, 3, 0, 4, value, -1);
                GtkTreePath *path;
                gint rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL);
                path = gtk_tree_path_new_from_indices (rows - 1, -1);
                gtk_tree_view_set_cursor (treeview, path, NULL, TRUE); // set cursor onto new field
                gtk_tree_path_free(path);
                trkproperties_modified = 1;
            }
            else {
                GtkWidget *d = gtk_message_dialog_new (GTK_WINDOW (dlg), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Field with such name already exists, please try different name."));
                gtk_window_set_title (GTK_WINDOW (d), _("Cannot add field"));

                gtk_dialog_run (GTK_DIALOG (d));
                gtk_widget_destroy (d);
                continue;
            }
        }
        break;
    }
    gtk_widget_destroy (dlg);
    gtk_window_present (GTK_WINDOW (trackproperties));
}


void
on_trkproperties_paste_fields_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_automatically_fill_values_activate
                                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_trkproperties_crop_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    GtkNotebook *notebook = GTK_NOTEBOOK(lookup_widget(trackproperties, "trkproperties_notebook"));
    if (gtk_notebook_get_current_page(notebook) != 0) {
        return; // do not remove field if Metadata tab is not focused
    }

    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    GtkTreePath *path;
    gtk_tree_view_get_cursor (treeview, &path, NULL);
    if (!path) {
        return;
    }

    GtkTreeIter iter_curr;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter_curr, path);

    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    GtkTreeIter iter;
    gboolean res = gtk_tree_model_get_iter_first (model, &iter);
    while (res) {
        int getnext = 1;
        GtkTreePath *iter_path = gtk_tree_model_get_path (model, &iter);
        if (gtk_tree_path_compare (path, iter_path)) {
            GValue key = {0,};
            gtk_tree_model_get_value (model, &iter, 2, &key);

            GValue value = {0,};
            gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &value);
            const char *svalue = g_value_get_string (&value);

            // delete unknown fields completely; otherwise just clear
            int i = 0;
            for (; trkproperties_types[i]; i += 2) {
                if (!strcasecmp (svalue, trkproperties_types[i])) {
                    break;
                }
            }
            if (trkproperties_types[i]) { // known val, clear
                gtk_list_store_set (store, &iter, 1, "", 3, 0, 4, "", -1);
            }
            else {
                gtk_list_store_remove (store, &iter);
                getnext = 0;
                if (!gtk_list_store_iter_is_valid (GTK_LIST_STORE (model), &iter)) {
                    res = 0;
                }
            }
        }
        gtk_tree_path_free (iter_path);
        if (getnext) {
            res = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
        }
    }

    gtk_tree_view_set_cursor (treeview, path, NULL, FALSE); // restore cursor after deletion
    gtk_tree_path_free (path);
    trkproperties_modified = 1;
}

