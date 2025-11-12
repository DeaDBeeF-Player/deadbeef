/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Oleksiy Yakovenko and other contributors

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

#include "pango/pango-font.h"
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <assert.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
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
#include "../../src/utf8.h"

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define min(x,y) ((x)<(y)?(x):(y))

static GtkWidget *trackproperties;
static GtkCellRenderer *rend_text2;
static GtkListStore *store;
static GtkListStore *propstore;
int trkproperties_modified;
static DB_playItem_t **tracks; // This is a copy to act as the model while editing and saving
static DB_playItem_t **orig_tracks; // This is a reference to the original tracks in the playlist
static int numtracks;
static GtkWidget *progressdlg;
static int progress_aborted;
static int last_ctx;
static ddb_playlist_t *last_plt;
static trkproperties_delegate_t *_delegate;


// Max length of a string displayed in the TableView
// If a string is longer -- it gets clipped, and appended with " (…)", like with linebreaks
#define MAX_GUI_FIELD_LEN 500

static void
_iterate_semicolon_separated_substrings(const char *svalue, void (^completion_block)(const char *item));

static void
_cleanup_track_list (void);

static void
_set_metadata_row(GtkListStore *store, GtkTreeIter *iter, const char *key, int is_mult, const char *title, char *value);

static void
_remove_field(GtkListStore *model, GtkTreeIter *iter, const char *skey);

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

    _cleanup_track_list();

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
    else if (gtk_widget_is_focus(lookup_widget (trackproperties, "metalist"))) {
        if (event->keyval == GDK_Delete) {
            on_trkproperties_remove_activate (NULL, NULL);
            return TRUE;
        }
        else if (event->keyval == GDK_Insert) {
            on_trkproperties_add_new_field_activate (NULL, NULL);
            return TRUE;
        }
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

static void _apply_field_to_track(ddb_playItem_t *track, const char *key, const char *new_text) {
    deadbeef->pl_delete_meta(track, key);

    static const char *special_fields[] = {
        "comment",
        "lyrics",
        NULL
    };

    int should_split = 1;
    for (int n = 0; special_fields[n] != NULL; n++) {
        if (!strcasecmp(key, special_fields[n])) {
            should_split = 0;
            break;
        }
    }

    if (!should_split) {
        deadbeef->pl_append_meta(track, key, new_text);
        return;
    }

    _iterate_semicolon_separated_substrings(new_text, ^(const char *item) {
        deadbeef->pl_append_meta(track, key, item);
    });
}

static void
_apply_field_to_all_tracks (const char *key, const char *new_text) {
    for (int i = 0; i < numtracks; i++) {
        _apply_field_to_track(tracks[i], key, new_text);
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

    GValue title = {};
    GValue key = {};
    GValue value = {};
    GValue mult = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 0, &title);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &key);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 4, &value);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 3, &mult);
    const char *stitle = g_value_get_string (&title);
    const char *skey = g_value_get_string (&key);
    const char *svalue = g_value_get_string (&value);
    if (!svalue) {
        svalue = "";
    }

    // The multiple values case gets cleared on attempt to edit,
    // that's why the change gets applied unconditionally for multivalue case
    int imult = g_value_get_int (&mult);
    if (strcmp (svalue, new_text) || imult) {
        _apply_field_to_all_tracks (skey, new_text);
        _set_metadata_row(store, &iter, skey, 0, stitle, new_text);
        trkproperties_modified = 1;
    }

    G_IS_VALUE (&title) ? ((void)(g_value_unset (&title)), NULL) : NULL;
    G_IS_VALUE (&key) ? ((void)(g_value_unset (&key)), NULL) : NULL;
    G_IS_VALUE (&value) ? ((void)(g_value_unset (&value)), NULL) : NULL;
    G_IS_VALUE (&mult) ? ((void)(g_value_unset (&mult)), NULL) : NULL;
    trkproperties_block_keyhandler = 0;
}

static void
_set_metadata_row(GtkListStore *store, GtkTreeIter *iter, const char *key, int is_mult, const char *title, char *value) {
    char *clipped_val = clip_multiline_value (value);
    char *display_val = clipped_val ?: value;
    gtk_list_store_set (store, iter, META_COL_TITLE, title, META_COL_DISPLAY_VAL, display_val, META_COL_KEY, key, META_COL_IS_MULT, is_mult ? 1 : 0, META_COL_VALUE, value, META_COL_PANGO_WEIGHT, PANGO_WEIGHT_NORMAL, -1);
    free (clipped_val);
}

void
add_field (GtkListStore *store, const char *key, const char *title, int is_prop, DB_playItem_t **tracks, int numtracks) {
    size_t val_len = 5000;
    char *val = malloc(val_len);

    // get value to edit
    const char *mult = is_prop ? "" : _("[Multiple values] ");
    size_t ml = strlen (mult);
    memcpy (val, mult, ml+1);
    int n = trkproperties_get_field_value (val + ml, (int)(val_len - ml), key, tracks, numtracks);

    GtkTreeIter iter;
    gtk_list_store_append (store, &iter);
    if (!is_prop) {
        char *v = val;
        if (!n) {
            v += ml;
        }

        _set_metadata_row(store, &iter, key, n, title, v);
    }
    else {
        gtk_list_store_set (store, &iter, META_COL_TITLE, title, META_COL_DISPLAY_VAL, n ? val : val + ml, META_COL_PANGO_WEIGHT, PANGO_WEIGHT_NORMAL, -1);
    }

    free (val);
}

void
add_field_section(GtkListStore *store, const char *title, const char *value) {
	GtkTreeIter iter;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set(store, &iter, META_COL_TITLE, title, META_COL_DISPLAY_VAL, value, META_COL_PANGO_WEIGHT, PANGO_WEIGHT_BOLD, -1);
}

static char *
_formatted_title_for_unknown_key(const char *key) {
    size_t l = strlen (key);
    char *title = malloc(l*4);
    title[0] = '<';
    char *t = title + 1;
    const char *p = key;
    while (*p) {
        int32_t size = 0;
        u8_nextchar (p, &size);
        int outsize = u8_toupper((const signed char *)p, size, t);
        t += outsize;
        p += size;
    }
    *t++ = '>';
    *t++ = 0;
    return title;
}

void
trkproperties_fill_meta (GtkListStore *store, DB_playItem_t **tracks, int numtracks) {
    // no clear here
    // gtk_list_store_clear (store);
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

        char *title = _formatted_title_for_unknown_key(keys[k]);
        add_field (store, keys[k], title, 0, tracks, numtracks);
        free (title);
        title = NULL;
    }
    if (keys) {
        free (keys);
    }
}

void
trkproperties_fill_prop (GtkListStore *store, DB_playItem_t **tracks, int numtracks) {
    // no clear here
    // gtk_list_store_clear (propstore);
    if (!tracks) {
        return;
    }

    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 1, tracks, numtracks);

    // add "standard" fields
    for (int i = 0; trkproperties_hc_props[i]; i += 2) {
        add_field (store, trkproperties_hc_props[i], _(trkproperties_hc_props[i+1]), 1, tracks, numtracks);
    }

    // add all other fields
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

        char *title = _formatted_title_for_unknown_key(keys[k] + 1);
        add_field (store, keys[k], title, 1, tracks, numtracks);
        free (title);
        title = NULL;
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

    gtk_list_store_clear (store);
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
        char *title = _formatted_title_for_unknown_key(keys[k] + 1);
        add_field (propstore, keys[k], title, 1, tracks, numtracks);
        free (title);
        title = NULL;
    }
    if (keys) {
        free (keys);
    }
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
        store = gtk_list_store_new (6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
        gtk_tree_view_set_model (tree, GTK_TREE_MODEL (store));
        GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
        rend_text2 = GTK_CELL_RENDERER (ddb_cell_renderer_text_multiline_new ());
        g_object_set (G_OBJECT (rend_text2), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

        g_signal_connect ((gpointer)rend_text2, "edited", G_CALLBACK (on_metadata_edited), store);

        GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes (_("Name"), rend_text, "text", META_COL_TITLE, NULL);
        GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_text2, "text", META_COL_DISPLAY_VAL, NULL);

        gtk_tree_view_append_column (tree, col1);
        gtk_tree_view_append_column (tree, col2);

        // properties tree
        proptree = GTK_TREE_VIEW (lookup_widget (trackproperties, "properties"));
        propstore = gtk_list_store_new (6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);
        gtk_tree_view_set_model (proptree, GTK_TREE_MODEL (propstore));
        GtkCellRenderer *rend_propkey = gtk_cell_renderer_text_new ();
        GtkCellRenderer *rend_propvalue = gtk_cell_renderer_text_new ();
        g_object_set (G_OBJECT (rend_propvalue), "editable", FALSE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
        col1 = gtk_tree_view_column_new_with_attributes (_("Key"), rend_propkey, "text", META_COL_TITLE, NULL);
        col2 = gtk_tree_view_column_new_with_attributes (_("Value"), rend_propvalue, "text", META_COL_DISPLAY_VAL, NULL);
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
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (trackproperties, "filename")), deadbeef->pl_find_meta_raw (tracks[0], ":URI"));
    }
    else {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (trackproperties, "filename")), _("[Multiple values]"));
    }

    GtkWidget *widget = trackproperties;
    trkproperties_fill_metadata ();

    gtk_widget_set_sensitive (lookup_widget (widget, "write_tags"), TRUE);

    gtk_widget_show (widget);
    gtk_window_present (GTK_WINDOW (widget));
}

static void
_cleanup_track_list (void) {
    for (int i = 0; i < numtracks; i++) {
        deadbeef->pl_item_unref (orig_tracks[i]);
    }
    free (orig_tracks);
    orig_tracks = NULL;
    trkproperties_free_track_list (&tracks, &numtracks);
}

void
show_track_properties_dlg_with_track_list (ddb_playItem_t **track_list, int count) {
    _cleanup_track_list ();

    if (count == 0) {
        return;
    }

    orig_tracks = calloc (count, sizeof (ddb_playItem_t *));
    tracks = calloc (count, sizeof (ddb_playItem_t *));
    for (int i = 0; i < count; i++) {
        orig_tracks[i] = track_list[i];
        deadbeef->pl_item_ref(track_list[i]);
        tracks[i] = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy(tracks[i], track_list[i]);
    }
    numtracks = count;
    show_track_properties_dlg_with_current_track_list();
    _delegate = NULL;
}

void
show_track_properties_dlg (int ctx, ddb_playlist_t *plt) {
    _cleanup_track_list();

    last_ctx = ctx;
    deadbeef->plt_ref (plt);
    if (last_plt) {
        deadbeef->plt_unref (last_plt);
    }
    last_plt = plt;

    trkproperties_build_track_list_for_ctx (plt, ctx, &orig_tracks, &numtracks);

    if (numtracks == 0) {
        return;
    }

    tracks = calloc (numtracks, sizeof (ddb_playItem_t *));
    for (int i = 0; i < numtracks; i++) {
        tracks[i] = deadbeef->pl_item_alloc();
        deadbeef->pl_item_copy(tracks[i], orig_tracks[i]);
    }

    show_track_properties_dlg_with_current_track_list();
    _delegate = NULL;
}

void
trkproperties_set_delegate (trkproperties_delegate_t *delegate) {
    _delegate = delegate;
}

static gboolean
write_finished_cb (void *ctx) {
    deadbeef->pl_lock ();

    const char **keys = NULL;
    int nkeys = trkproperties_build_key_list (&keys, 0, orig_tracks, numtracks);

    for (int i = 0 ; i < numtracks; i++) {
        // Remove all existing affected fields before adding the new ones
        for (int j = 0; j < nkeys; j++) {
            deadbeef->pl_delete_meta(orig_tracks[i], keys[j]);
        }

        // Update the original items
        deadbeef->pl_item_copy (orig_tracks[i], tracks[i]);
    }

    free (keys);

    deadbeef->pl_unlock();

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
    gtk_entry_set_text (GTK_ENTRY (progressitem), deadbeef->pl_find_meta_raw (track, ":URI"));
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
        const char *dec = deadbeef->pl_find_meta_raw (track, ":DECODER");
        char decoder_id[100];
        if (dec) {
            strncpy (decoder_id, dec, sizeof (decoder_id));
        }
        int match = track && dec;
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
                                        gpointer         user_data) {
    progress_aborted = 1;
}

void
on_write_tags_clicked                  (GtkButton       *button,
                                        gpointer         user_data) {
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

static void _update_single_value(GtkTextBuffer *buffer, GtkTreeIter *iter, const char *skey, const char *stitle) {
    GtkTextIter begin, end;

    gtk_text_buffer_get_start_iter (buffer, &begin);
    gtk_text_buffer_get_end_iter (buffer, &end);

    char *new_text = gtk_text_buffer_get_text (buffer, &begin, &end, TRUE);

    for (int i = 0; i < numtracks; i++) {
        _apply_field_to_track(tracks[i], skey, new_text);
    }
    free (new_text);

    size_t val_len = 5000;
    char *val = malloc(val_len);

    // get value to edit
    trkproperties_get_field_value (val, (int)val_len, skey, tracks, numtracks);

    _set_metadata_row(store, iter, skey, 0, stitle, val);

    free (val);

    trkproperties_modified = 1;
}

static void
_edit_field_single_track (void) {
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
    for (GList *l = lst; l; l = l->next) {
        gtk_tree_path_free (l->data);
    }
    g_list_free (lst);
    path = NULL;
    lst = NULL;

    GValue title = {};
    GValue key = {};
    GValue value = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 0, &title);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &key);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 4, &value);
    const char *skey = g_value_get_string (&key);
    const char *svalue = g_value_get_string (&value);
    const char *stitle = g_value_get_string (&title);

    char *uppercase_key = strdup (skey);
    for (char *p = uppercase_key; *p; p++) {
        *p = toupper (*p);
    }

    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "field_name")), uppercase_key);

    free (uppercase_key);

    GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
    gtk_text_buffer_set_text (buffer, svalue, (gint)strlen (svalue));
    gtk_text_view_set_buffer (GTK_TEXT_VIEW (lookup_widget (dlg, "field_value")), buffer);

    g_value_unset (&value);

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        _update_single_value(buffer, &iter, skey, stitle);
    }
    g_value_unset (&key);
    g_value_unset (&title);
    g_object_unref (buffer);
    gtk_widget_destroy (dlg);
}

void
on_individual_field_edited (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data) {
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

    GValue value = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 4, &value);
    const char *svalue = g_value_get_string (&value);
    if (!svalue) {
        svalue = "";
    }

    // The multiple values case gets cleared on attempt to edit,
    // that's why the change gets applied unconditionally for multivalue case
    if (strcmp (svalue, new_text)) {
        gtk_list_store_set (store, &iter, META_COL_KEY, new_text, META_COL_IS_MULT, 0, META_COL_VALUE, new_text, -1);
    }

    G_IS_VALUE (&value) ? ((void)(g_value_unset (&value)), NULL) : NULL;
}


static char *
_semicolon_separated_string_for_meta(DB_metaInfo_t *meta) {
    if (meta == NULL) {
        return strdup("");
    }

    char *field = NULL;
    size_t valuelen = 0;
    size_t len = 0;
    const char *value = meta->value;
    while (len < meta->valuesize) {
        size_t metalen = strlen (value);
        if (metalen == 0) {
            break;
        }

        size_t newlen = valuelen + metalen;
        int add_semicolon = 0;
        if (field != NULL) {
            newlen += 2;
            add_semicolon = 1;
        }

        field = realloc (field, newlen+1);

        if (add_semicolon) {
            memcpy (field + valuelen, "; ", 2);
            valuelen += 2;
        }

        memcpy (field + valuelen, value, metalen);
        valuelen = newlen;

        len += metalen + 1;
        value += metalen + 1;
    }

    if (field != NULL) {
        field[valuelen] = 0;
    }
    else {
        field = strdup("");
    }

    return field;
}

static void
_iterate_semicolon_separated_substrings(const char *svalue, void (^completion_block)(const char *item)) {
    while (*svalue) {
        char *semicolon = strchr(svalue, ';');

        size_t len;
        if (semicolon == NULL) {
            len = strlen(svalue);
        }
        else {
            len = semicolon - svalue;
        }

        char *item = malloc (len + 1);
        memcpy (item, svalue, len);
        item[len] = 0;

        char *trimmed_item = gtkui_trim_whitespace(item, len);

        if (*trimmed_item) {
            completion_block(trimmed_item);
        }

        free (item);

        if (semicolon != NULL) {
            len += 1;
        }
        svalue += len;
    }
}

static void
_edit_field_multiple_tracks (void) {
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    GtkTreeSelection *sel = gtk_tree_view_get_selection (treeview);
    int count = gtk_tree_selection_count_selected_rows (sel);
    if (count != 1) {
        return; // multiple fields can't be edited at the same time
    }

    GtkWidget *dlg = create_edit_multiple_tracks_dialog();
    gtk_widget_set_size_request(dlg, 400, 300);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (trackproperties));
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    GList *lst = gtk_tree_selection_get_selected_rows (sel, NULL);

    GtkTreePath *path = lst->data;

    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
    for (GList *l = lst; l; l = l->next) {
        gtk_tree_path_free (l->data);
    }
    g_list_free (lst);
    path = NULL;
    lst = NULL;

    GValue title = {};
    GValue key = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 0, &title);
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &key);

    const char *stitle = g_value_get_string (&title);
    const char *skey = g_value_get_string (&key);

    char *uppercase_key = strdup (skey);
    for (char *p = uppercase_key; *p; p++) {
        *p = toupper (*p);
    }

    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "entry_field_name")), uppercase_key);
    free (uppercase_key);

    // Initialize the single value tab

    // Allow editing the value if it's the same on all tracks
    GValue mult = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 3, &mult);
    int imult = g_value_get_int (&mult);

    if (!imult) {
        GValue value = {};
        gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 4, &value);
        const char *svalue = g_value_get_string (&value);

        GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);
        gtk_text_buffer_set_text (buffer, svalue, (gint)strlen (svalue));
        g_value_unset (&value);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (lookup_widget (dlg, "textview_single_value")), buffer);
    }

    // Initialize the individual values tab
    GtkListStore *list_store = gtk_list_store_new(META_COL_COUNT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);

    GtkTreeView *tree = GTK_TREE_VIEW (lookup_widget (dlg, "treeview_individual_values"));
    gtk_tree_view_set_model (tree, GTK_TREE_MODEL (list_store));

    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new ();
    GtkCellRenderer *rend_text2 = gtk_cell_renderer_text_new();
    GtkCellRenderer *rend_text3 = GTK_CELL_RENDERER (ddb_cell_renderer_text_multiline_new ());

    g_object_set (G_OBJECT (rend_text3), "editable", TRUE, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    g_signal_connect ((gpointer)rend_text3, "edited", G_CALLBACK (on_individual_field_edited), list_store);

    GtkTreeViewColumn *col1 = gtk_tree_view_column_new_with_attributes ("#", rend_text, "text", META_COL_TITLE, NULL);
    GtkTreeViewColumn *col2 = gtk_tree_view_column_new_with_attributes (_("Item"), rend_text2, "text", META_COL_DISPLAY_VAL, NULL);
    GtkTreeViewColumn *col3 = gtk_tree_view_column_new_with_attributes (_("Field"), rend_text3, "text", META_COL_KEY, NULL);

    gtk_tree_view_column_set_resizable(col2, TRUE);
    gtk_tree_view_column_set_resizable(col3, TRUE);

    gtk_tree_view_append_column (tree, col1);
    gtk_tree_view_append_column (tree, col2);
    gtk_tree_view_append_column (tree, col3);

    char *item_tf = deadbeef->tf_compile ("%title%[ // %track artist%]");

    ddb_tf_context_t ctx;
    memset (&ctx, 0, sizeof (ctx));

    ctx._size = sizeof (ctx);
    ctx.plt = NULL;
    ctx.idx = -1;
    ctx.id = -1;

    for (int i = 0; i < numtracks; i++) {
        char item[1000];
        ctx.it = tracks[i];
        deadbeef->tf_eval(&ctx, item_tf, item, sizeof (item));

        char idx[10];
        snprintf (idx, sizeof (idx), "%d", i+1);
        DB_metaInfo_t *meta = deadbeef->pl_meta_for_key(tracks[i], skey);
        char *field = _semicolon_separated_string_for_meta(meta);

        GtkTreeIter new_iter;
        gtk_list_store_append(list_store, &new_iter);
        gtk_list_store_set (list_store, &new_iter, META_COL_TITLE, idx, META_COL_DISPLAY_VAL, item, META_COL_KEY, field, META_COL_IS_MULT, 0, META_COL_VALUE, field, -1);


        free (field);
    }

    deadbeef->tf_free (item_tf);

    gint response = gtk_dialog_run (GTK_DIALOG (dlg));

    if (response == GTK_RESPONSE_OK) {
        GtkWidget *tabs = lookup_widget(dlg, "notebook");
        gint tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(tabs));

        if (tab == 0) {
            GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(lookup_widget (dlg, "textview_single_value")));
            _update_single_value(buffer, &iter, skey, stitle);
        }
        else if (tab == 1) {
            GtkTreeIter curr_item_iter;
            gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &curr_item_iter);
            for (int i = 0; i < numtracks; i++) {
                GValue value = {};
                gtk_tree_model_get_value(GTK_TREE_MODEL(list_store), &curr_item_iter, 2, &value);
                const char *svalue = g_value_get_string (&value);

                _apply_field_to_track(tracks[i], skey, svalue);

                g_value_unset (&value);

                gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &curr_item_iter);
            }

            // create new text from tracks
            size_t val_len = 5000;
            char *val = malloc(val_len);

            // get value to edit
            const char *mult = _("[Multiple values] ");
            size_t ml = strlen (mult);
            memcpy (val, mult, ml+1);
            int n = trkproperties_get_field_value (val + ml, (int)(val_len - ml), skey, tracks, numtracks);

            char *display_val = val;
            if (!n) {
                display_val += ml;
            }

            _set_metadata_row(store, &iter, skey, 0, stitle, display_val);

            free (val);

            trkproperties_modified = 1;
        }
    }

    g_value_unset (&title);
    g_value_unset (&key);
    gtk_widget_destroy (dlg);
}

void
on_trkproperties_edit_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    if (numtracks == 1) {
        _edit_field_single_track();
    }
    else if (numtracks > 1) {
        _edit_field_multiple_tracks();
    }
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
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (treeview, &path, &col);
    if (!path || !col) {
        return;
    }

    GtkTreeIter iter;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
    GValue key = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &key);
    const char *skey = g_value_get_string (&key);

    for (int i = 0; i < numtracks; i++) {
        deadbeef->pl_delete_meta(tracks[i], skey);
    }

    _remove_field(store, &iter, skey);
    g_value_unset (&key);

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
                GValue value = {0};
                gtk_tree_model_get_value (GTK_TREE_MODEL (store), &iter, 2, &value);
                const char *svalue = g_value_get_string (&value);
                if (!strcasecmp (svalue, text)) {
                    g_value_unset(&value);
                    dup = 1;
                    break;
                }
                g_value_unset(&value);
                res = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
            }

            if (!dup) {
                char *title = _formatted_title_for_unknown_key(text);
                const char *value = "";
                const char *key = text;

                gtk_list_store_append (store, &iter);
                gtk_list_store_set (store, &iter, META_COL_TITLE, title, META_COL_DISPLAY_VAL, value, META_COL_KEY, key, META_COL_IS_MULT, 0, META_COL_VALUE, value, -1);
                free (title);
                title = NULL;

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

// predefined keys clear, otherwise remove
static void
_remove_field(GtkListStore *model, GtkTreeIter *iter, const char *skey) {
    GValue title = {};
    gtk_tree_model_get_value (GTK_TREE_MODEL(model), iter, 0, &title);
    const char *stitle = g_value_get_string (&title);
    int cleared = 0;
    for (int i = 0; trkproperties_types[i] != NULL; i += 2) {
        if (!strcasecmp (trkproperties_types[i], skey)) {
            _set_metadata_row(store, iter, skey, 0, stitle, "");
            cleared = 1;
            break;
        }
    }

    if (!cleared) {
        gtk_list_store_remove(store, iter);
    }
    g_value_unset(&title);
}

void
on_trkproperties_crop_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    GtkTreeView *treeview = GTK_TREE_VIEW (lookup_widget (trackproperties, "metalist"));
    GtkTreeModel *model = gtk_tree_view_get_model (treeview);

    GtkTreePath *path;
    gtk_tree_view_get_cursor (treeview, &path, NULL);
    if (!path) {
        return;
    }

    GtkTreeIter iter_curr;
    gtk_tree_model_get_iter (model, &iter_curr, path);

    // Create a list of iters to remove
    GtkTreeIter **iters_to_remove = calloc (gtk_tree_model_iter_n_children(model, NULL), sizeof (GtkTreeIter *));
    int count = 0;

    GtkTreeIter iter;
    gboolean res = gtk_tree_model_get_iter_first (model, &iter);
    while (res) {
        int getnext = 1;
        GtkTreePath *iter_path = gtk_tree_model_get_path (model, &iter);

        if (gtk_tree_path_compare (path, iter_path)) {
            iters_to_remove[count++] = gtk_tree_iter_copy(&iter);
        }

        gtk_tree_path_free (iter_path);
        if (getnext) {
            res = gtk_tree_model_iter_next (model, &iter);
        }
    }

    // Remove the collected iters
    for (int i = 0; i < count; i++) {
        GValue key = {};
        gtk_tree_model_get_value (model, iters_to_remove[i], 2, &key);
        const char *skey = g_value_get_string (&key);

        for (int i = 0; i < numtracks; i++) {
            deadbeef->pl_delete_meta(tracks[i], skey);
        }

        _remove_field(store, iters_to_remove[i], skey);
        g_value_unset(&key);

        gtk_tree_iter_free(iters_to_remove[i]);
    }
    free (iters_to_remove);
    iters_to_remove = NULL;

    gtk_tree_view_set_cursor (treeview, path, NULL, FALSE); // restore cursor after deletion
    gtk_tree_path_free (path);
    trkproperties_modified = 1;
}

