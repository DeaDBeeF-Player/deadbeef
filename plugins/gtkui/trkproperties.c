/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "../../gettext.h"
#include "ddblistview.h"
#include "trkproperties.h"
#include "interface.h"
#include "support.h"
#include "../../deadbeef.h"
#include "gtkui.h"
#include "mainplaylist.h"
#include "search.h"
#include "ddbcellrenderertextmultiline.h"
#include "tagwritersettings.h"
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

static int
build_key_list (const char ***pkeys, int props) {
    int sz = 20;
    const char **keys = malloc (sizeof (const char *) * sz);
    if (!keys) {
        fprintf (stderr, "fatal: out of memory allocating key list\n");
        assert (0);
        return 0;
    }

    int n = 0;

    for (int i = 0; i < numtracks; i++) {
        DB_metaInfo_t *meta = deadbeef->pl_get_metadata_head (tracks[i]);
        while (meta) {
            if (meta->key[0] != '!' && ((props && meta->key[0] == ':') || (!props && meta->key[0] != ':'))) {
                int k = 0;
                for (; k < n; k++) {
                    if (meta->key == keys[k]) {
                        break;
                    }
                }
                if (k == n) {
                    if (n >= sz) {
                        sz *= 2;
                        keys = realloc (keys, sizeof (const char *) * sz);
                        if (!keys) {
                            fprintf (stderr, "fatal: out of memory reallocating key list (%d keys)\n", sz);
                            assert (0);
                        }
                    }
                    keys[n++] = meta->key;
                }
            }
            meta = meta->next;
        }
    }

    *pkeys = keys;
    return n;
}

static int
equals_ptr (const char *a, const char *b) {
    return a == b;
}

static int
get_field_value (char *out, int size, const char *key, const char *(*getter)(DB_playItem_t *it, const char *key), int (*equals)(const char *a, const char *b)) {
    int multiple = 0;
    *out = 0;
    if (numtracks == 0) {
        return 0;
    }
    char *p = out;
    deadbeef->pl_lock ();
    const char **prev = malloc (sizeof (const char *) * numtracks);
    memset (prev, 0, sizeof (const char *) * numtracks);
    for (int i = 0; i < numtracks; i++) {
        const char *val = getter (tracks[i], key);
        if (val && val[0] == 0) {
            val = NULL;
        }
        if (i > 0) {
            int n = 0;
            for (; n < i; n++) {
                if (equals (prev[n], val)) {
                    break;
                }
            }
            if (n == i) {
                multiple = 1;
                if (val) {
                    size_t l = snprintf (out, size, out == p ? "%s" : "; %s", val ? val : "");
                    l = min (l, size);
                    out += l;
                    size -= l;
                }
            }
        }
        else if (val) {
            size_t l = snprintf (out, size, "%s", val ? val : "");
            l = min (l, size);
            out += l;
            size -= l;
        }
        prev[i] = val;
        if (size <= 1) {
            break;
        }
    }
    deadbeef->pl_unlock ();
    if (size <= 1) {
        gchar *prev = g_utf8_prev_char (out-4);
        strcpy (prev, "...");
    }
    free (prev);
    return multiple;
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

void
on_remove_field_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_add_field_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_trackproperties_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
#if 0
    if (event->keyval == GDK_Escape) {
        printf ("trkproperties esc\n");
        on_trackproperties_delete_event (trackproperties, NULL, NULL);
        return TRUE;
    }
    else if (event->keyval == GDK_Delete) {
        printf ("trkproperties del\n");
        on_remove_field_activate (NULL, NULL);
        return FALSE;
    }
    else if (event->keyval == GDK_Insert) {
        printf ("trkproperties ins\n");
        on_add_field_activate (NULL, NULL);
        return TRUE;
    }
#endif
    return FALSE;
}

void
trkproperties_destroy (void) {
    if (trackproperties) {
        on_trackproperties_delete_event (trackproperties, NULL, NULL);
    }
}

void
on_closebtn_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{
    trkproperties_destroy ();
}

void
on_metadata_edited (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
    GtkListStore *store = GTK_LIST_STORE (user_data);
    GtkTreePath *treepath = gtk_tree_path_new_from_string (path);
    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, treepath);
    gtk_tree_path_free (treepath);
    GValue value = {0,};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 1, &value);
    const char *svalue = g_value_get_string (&value);
    if (strcmp (svalue, new_text)) {
        gtk_list_store_set (store, &iter, 1, new_text, 3, 0, -1);
        trkproperties_modified = 1;
    }
}

// full metadata
static const char *types[] = {
    "artist", "Artist",
    "title", "Track Title",
    "album", "Album",
    "year", "Date",
    "track", "Track Number",
    "numtracks", "Total Tracks",
    "genre", "Genre",
    "composer", "Composer",
    "disc", "Disc Number",
    "comment", "Comment",
    NULL
};

static const char *hc_props[] = {
    ":URI", "Location",
    ":TRACKNUM", "Subtrack Index",
    ":DURATION", "Duration",
    ":TAGS", "Tag Type(s)",
    ":HAS_EMBEDDED_CUESHEET", "Embedded Cuesheet",
    ":DECODER", "Codec",
    NULL
};

void
add_field (GtkListStore *store, const char *key, const char *title, int is_prop) {
    // get value to edit
    const char *mult = is_prop ? "" : _("[Multiple values] ");
    char val[1000];
    size_t ml = strlen (mult);
    memcpy (val, mult, ml+1);
    int n = get_field_value (val + ml, sizeof (val) - ml, key, deadbeef->pl_find_meta_raw, equals_ptr);

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    if (!is_prop) {
        if (n) {
            gtk_list_store_set (store, &iter, 0, title, 1, val, 2, key, 3, n ? 1 : 0, -1);
        }
        else {
            deadbeef->pl_lock ();
            const char *val = deadbeef->pl_find_meta_raw (tracks[0], key);
            if (!val) {
                val = "";
            }
            gtk_list_store_set (store, &iter, 0, title, 1, val, 2, key, 3, n ? 1 : 0, -1);
            deadbeef->pl_unlock ();
        }
    }
    else {
        gtk_list_store_set (store, &iter, 0, title, 1, n ? val : val + ml, -1);
    }
}

void
trkproperties_fill_metadata (void) {
    if (!trackproperties) {
        return;
    }
    trkproperties_modified = 0;
    gtk_list_store_clear (store);
    gtk_list_store_clear (propstore);
    deadbeef->pl_lock ();

    const char **keys = NULL;
    int nkeys = build_key_list (&keys, 0);

    int k;

    // add "standard" fields
    for (int i = 0; types[i]; i += 2) {
        add_field (store, types[i], _(types[i+1]), 0);
    }

    // add all other fields
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; types[i]; i += 2) {
            if (!strcasecmp (keys[k], types[i])) {
                break;
            }
        }
        if (types[i]) {
            continue;
        }

        char title[1000];
        if (!types[i]) {
            snprintf (title, sizeof (title), "<%s>", keys[k]);
        }
        add_field (store, keys[k], title, 0);
    }
    if (keys) {
        free (keys);
    }

    // hardcoded properties
    for (int i = 0; hc_props[i]; i += 2) {
        add_field (propstore, hc_props[i], _(hc_props[i+1]), 1);
    }
    // properties
    keys = NULL;
    nkeys = build_key_list (&keys, 1);
    for (int k = 0; k < nkeys; k++) {
        int i;
        for (i = 0; hc_props[i]; i += 2) {
            if (!strcasecmp (keys[k], hc_props[i])) {
                break;
            }
        }
        if (hc_props[i]) {
            continue;
        }
        char title[1000];
        snprintf (title, sizeof (title), "<%s>", keys[k]+1);
        add_field (propstore, keys[k], title, 1);
    }
    if (keys) {
        free (keys);
    }

    deadbeef->pl_unlock ();
}

void
show_track_properties_dlg (void) {

    deadbeef->pl_lock ();

    if (tracks) {
        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_item_unref (tracks[i]);
        }
        free (tracks);
        tracks = NULL;
        numtracks = 0;
    }

    int nsel = deadbeef->pl_getselcount ();
    if (0 < nsel) {
        tracks = malloc (sizeof (DB_playItem_t *) * nsel);
        if (tracks) {
            int n = 0;
            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    assert (n < nsel);
                    deadbeef->pl_item_ref (it);
                    tracks[n++] = it;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            numtracks = nsel;
        }
        else {
            deadbeef->pl_unlock ();
            return;
        }
    }

    deadbeef->pl_unlock ();

    GtkTreeView *tree;
    GtkTreeView *proptree;
    if (!trackproperties) {
        trackproperties = create_trackproperties ();
        gtk_window_set_transient_for (GTK_WINDOW (trackproperties), GTK_WINDOW (mainwin));
        wingeom_restore (trackproperties, "trkproperties", -1, -1, 300, 400, 0);

        // metadata tree
        tree = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
        store = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
        GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
        rend_text2 = GTK_CELL_RENDERER (ddb_cell_renderer_text_multiline_new ());
        g_signal_connect ((gpointer)rend_text2, "edited",
                G_CALLBACK (on_metadata_edited),
                store);
        GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend_text, "text", 0, NULL);
        GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_text2, "text", 1, NULL);
        gtk_tree_view_append_column (tree, col1);
        gtk_tree_view_append_column (tree, col2);

        // properties tree
        proptree = GTK_TREE_VIEW (lookup_widget (trackproperties, "properties"));
        propstore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
        gtk_tree_view_set_model (proptree, GTK_TREE_MODEL (propstore));
        GtkCellRenderer *rend_propkey = gtk_cell_renderer_text_new ();
        GtkCellRenderer *rend_propvalue = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (rend_propvalue), "editable", TRUE, NULL);
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

    g_object_set (G_OBJECT (rend_text2), "editable", TRUE, NULL);

    GtkWidget *widget = trackproperties;
    GtkWidget *w;
    const char *meta;

    trkproperties_fill_metadata ();

    gtk_widget_set_sensitive (lookup_widget (widget, "write_tags"), TRUE);

    gtk_widget_show (widget);
    gtk_window_present (GTK_WINDOW (widget));
}

static gboolean
set_metadata_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
    GValue mult = {0,};
    gtk_tree_model_get_value (model, iter, 3, &mult);
    int smult = g_value_get_int (&mult);
    if (!smult) {
        GValue key = {0,}, value = {0,};
        gtk_tree_model_get_value (model, iter, 2, &key);
        gtk_tree_model_get_value (model, iter, 1, &value);
        const char *skey = g_value_get_string (&key);
        const char *svalue = g_value_get_string (&value);
        if (*svalue) {
            for (int i = 0; i < numtracks; i++) {
                deadbeef->pl_replace_meta (tracks[i], skey, svalue);
            }
        }
        else {
            for (int i = 0; i < numtracks; i++) {
                deadbeef->pl_delete_meta (tracks[i], skey);
            }
        }
    }

    return FALSE;
}

static gboolean
write_finished_cb (void *ctx) {
    gtk_widget_destroy (progressdlg);
    progressdlg = NULL;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    main_refresh ();
    search_refresh ();
    trkproperties_modified = 0;
    show_track_properties_dlg ();

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
                int mult = 0;
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

    for (int i = 0; i < numtracks; i++) {
        ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc (DB_EV_TRACKINFOCHANGED);
        ev->track = tracks[i];
        deadbeef->pl_item_ref (ev->track);
        deadbeef->event_send ((ddb_event_t*)ev, 0, 0);
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

void
on_add_field_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    if (!gtk_widget_is_focus(GTK_WIDGET (treeview))) {
        return; // do not add field if Metadata tab is not focused
    }
    GtkWidget *dlg = create_entrydialog ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Field name"));
    GtkWidget *e;
    e = lookup_widget (dlg, "title_label");
    gtk_label_set_text (GTK_LABEL(e), _("Name:"));
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
                int l = strlen (text);
                char title[l+3];
                snprintf (title, sizeof (title), "<%s>", text);
                const char *value = "";
                const char *key = text;

                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter, 0, title, 1, value, 2, key, -1);
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
}

void
on_remove_field_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {

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
    for (; types[i]; i += 2) {
        if (!strcasecmp (svalue, types[i])) {
            break;
        }
    }
    if (types[i]) { // known val, clear
        gtk_list_store_set (store, &iter, 1, "", 3, 0, -1);
    }
    else {
        gtk_list_store_remove (store, &iter);
    }
    gtk_tree_view_set_cursor (treeview, path, NULL, FALSE); // restore cursor after deletion
    gtk_tree_path_free (path);
    trkproperties_modified = 1;
}

gboolean
on_metalist_button_press_event         (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
    if (event->button == 3) {
        GtkWidget *menu;
        GtkWidget *add;
        GtkWidget *remove;
        menu = gtk_menu_new ();
        add = gtk_menu_item_new_with_mnemonic (_("Add field"));
        gtk_widget_show (add);
        gtk_container_add (GTK_CONTAINER (menu), add);
        remove = gtk_menu_item_new_with_mnemonic (_("Remove field"));
        gtk_widget_show (remove);
        gtk_container_add (GTK_CONTAINER (menu), remove);

        g_signal_connect ((gpointer) add, "activate",
                G_CALLBACK (on_add_field_activate),
                NULL);

        g_signal_connect ((gpointer) remove, "activate",
                G_CALLBACK (on_remove_field_activate),
                NULL);

        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, widget, event->button, gtk_get_current_event_time());
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

