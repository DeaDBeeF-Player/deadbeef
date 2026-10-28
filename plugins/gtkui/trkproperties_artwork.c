/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2026 Oleksiy Yakovenko and other contributors

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
#    include <config.h>
#endif
#include <dispatch/dispatch.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <deadbeef/deadbeef.h>
#include "../../gettext.h"
#include "../artwork/artwork.h"
#include "gtkui.h"
#include "trkproperties.h"
#include "trkproperties_artwork.h"

// The pictures are scaled down to fit this size
#define PICTURE_MAX_HEIGHT 96
#define PICTURE_MAX_WIDTH 384

enum {
    COL_NAME,
    COL_PICTURE,
    COL_COUNT,
};

// The state of the (single) track properties dialog
static struct {
    GtkWidget *scrolled;
    GtkListStore *store;
    GtkWidget *message_label;
    GtkWidget *spinner;

    // Incremented to invalidate the pending loads
    unsigned generation;

    // Identifies the artwork queries of this dialog, so that they can be cancelled in bulk
    int64_t source_id;
} _ui;

typedef struct {
    char *name;
    GdkPixbuf *pixbuf;
} cover_result_t;

// All the queries of a single load share this. It's only accessed on the main thread.
typedef struct {
    unsigned generation;
    int pending;
    int count;
    cover_result_t *results;
} load_context_t;

typedef struct {
    load_context_t *context;
    int index;
} query_data_t;

static const char *
_name_for_cover_type (ddb_artwork_cover_type_t cover_type) {
    switch (cover_type) {
    case DDB_ARTWORK_COVER_TYPE_OTHER:
        return _("Other");
    case DDB_ARTWORK_COVER_TYPE_FILE_ICON:
        return _("File Icon (32x32 PNG)");
    case DDB_ARTWORK_COVER_TYPE_OTHER_FILE_ICON:
        return _("Other File Icon");
    case DDB_ARTWORK_COVER_TYPE_FRONT:
        return _("Front Cover");
    case DDB_ARTWORK_COVER_TYPE_BACK:
        return _("Back Cover");
    case DDB_ARTWORK_COVER_TYPE_LEAFLET_PAGE:
        return _("Leaflet Page");
    case DDB_ARTWORK_COVER_TYPE_MEDIA:
        return _("Media");
    case DDB_ARTWORK_COVER_TYPE_LEAD_ARTIST:
        return _("Lead Artist");
    case DDB_ARTWORK_COVER_TYPE_ARTIST:
        return _("Artist");
    case DDB_ARTWORK_COVER_TYPE_CONDUCTOR:
        return _("Conductor");
    case DDB_ARTWORK_COVER_TYPE_BAND:
        return _("Band / Orchestra");
    case DDB_ARTWORK_COVER_TYPE_COMPOSER:
        return _("Composer");
    case DDB_ARTWORK_COVER_TYPE_LYRICIST:
        return _("Lyricist");
    case DDB_ARTWORK_COVER_TYPE_RECORDING_LOCATION:
        return _("Recording Location");
    case DDB_ARTWORK_COVER_TYPE_DURING_RECORDING:
        return _("During Recording");
    case DDB_ARTWORK_COVER_TYPE_DURING_PERFORMANCE:
        return _("During Performance");
    case DDB_ARTWORK_COVER_TYPE_VIDEO_SCREEN_CAPTURE:
        return _("Video Screen Capture");
    case DDB_ARTWORK_COVER_TYPE_BRIGHT_FISH:
        return _("A Bright Coloured Fish");
    case DDB_ARTWORK_COVER_TYPE_ILLUSTRATION:
        return _("Illustration");
    case DDB_ARTWORK_COVER_TYPE_BAND_LOGO:
        return _("Band / Artist Logotype");
    case DDB_ARTWORK_COVER_TYPE_PUBLISHER_LOGO:
        return _("Publisher / Studio Logotype");
    case DDB_ARTWORK_COVER_TYPE_DEFAULT:
        break;
    }
    return _("Cover");
}

#pragma mark - Visible state

static void
_stop_progress (void) {
    if (_ui.spinner == NULL) {
        return;
    }
    gtk_spinner_stop (GTK_SPINNER (_ui.spinner));
    gtk_widget_hide (_ui.spinner);
}

static void
_show_progress (void) {
    if (_ui.spinner == NULL) {
        return;
    }
    gtk_widget_hide (_ui.message_label);
    gtk_widget_hide (_ui.scrolled);
    gtk_widget_show (_ui.spinner);
    gtk_spinner_start (GTK_SPINNER (_ui.spinner));
}

static void
_show_message (const char *message) {
    if (_ui.message_label == NULL) {
        return;
    }
    _stop_progress ();
    gtk_widget_hide (_ui.scrolled);
    gtk_label_set_text (GTK_LABEL (_ui.message_label), message);
    gtk_widget_show (_ui.message_label);
}

static void
_show_covers (void) {
    if (_ui.scrolled == NULL) {
        return;
    }
    _stop_progress ();
    gtk_widget_hide (_ui.message_label);
    gtk_widget_show (_ui.scrolled);
}

#pragma mark - Loading

static GdkPixbuf *
_scale_to_fit (GdkPixbuf *pixbuf) {
    const int width = gdk_pixbuf_get_width (pixbuf);
    const int height = gdk_pixbuf_get_height (pixbuf);
    if (width <= 0 || height <= 0 || (width <= PICTURE_MAX_WIDTH && height <= PICTURE_MAX_HEIGHT)) {
        return pixbuf;
    }

    double scale = (double)PICTURE_MAX_HEIGHT / height;
    if ((double)PICTURE_MAX_WIDTH / width < scale) {
        scale = (double)PICTURE_MAX_WIDTH / width;
    }
    int new_width = (int)(width * scale);
    int new_height = (int)(height * scale);
    if (new_width < 1) {
        new_width = 1;
    }
    if (new_height < 1) {
        new_height = 1;
    }

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf, new_width, new_height, GDK_INTERP_BILINEAR);
    if (scaled == NULL) {
        return pixbuf;
    }
    g_object_unref (pixbuf);
    return scaled;
}

// Can be called on any thread. Returns a new reference, or NULL.
static GdkPixbuf *
_decode_picture (const void *data, size_t size, const char *filename) {
    GdkPixbuf *pixbuf = NULL;

    if (data != NULL && size != 0) {
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
        if (loader != NULL) {
            gdk_pixbuf_loader_write (loader, data, size, NULL);
            gdk_pixbuf_loader_close (loader, NULL);
            pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
            if (pixbuf != NULL) {
                g_object_ref (pixbuf);
            }
            g_object_unref (loader);
        }
    }
    else if (filename != NULL) {
        pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
    }

    if (pixbuf == NULL) {
        return NULL;
    }
    return _scale_to_fit (pixbuf);
}

static void
_free_load_context (load_context_t *context) {
    for (int i = 0; i < context->count; i++) {
        free (context->results[i].name);
        if (context->results[i].pixbuf != NULL) {
            g_object_unref (context->results[i].pixbuf);
        }
    }
    free (context->results);
    free (context);
}

// All the queries of the load are done: fill the table
static void
_finish_load (load_context_t *context) {
    gtk_list_store_clear (_ui.store);

    int rows = 0;
    for (int i = 0; i < context->count; i++) {
        if (context->results[i].pixbuf == NULL) {
            continue;
        }
        GtkTreeIter iter;
        gtk_list_store_append (_ui.store, &iter);
        gtk_list_store_set (
            _ui.store,
            &iter,
            COL_NAME,
            context->results[i].name,
            COL_PICTURE,
            context->results[i].pixbuf,
            -1);
        rows++;
    }

    if (rows > 0) {
        _show_covers ();
    }
    else {
        _show_message (_("No artwork found."));
    }
}

// Called on the main thread, takes the ownership of @c query_data and @c pixbuf
static void
_cover_loaded (query_data_t *query_data, GdkPixbuf *pixbuf, ddb_artwork_cover_type_t cover_type) {
    load_context_t *context = query_data->context;
    const int is_current = context->generation == _ui.generation && _ui.store != NULL;

    if (is_current && pixbuf != NULL) {
        context->results[query_data->index].name = strdup (_name_for_cover_type (cover_type));
        context->results[query_data->index].pixbuf = pixbuf;
    }
    else if (pixbuf != NULL) {
        g_object_unref (pixbuf);
    }
    free (query_data);

    context->pending--;
    if (context->pending != 0) {
        return;
    }

    if (is_current) {
        _finish_load (context);
    }
    _free_load_context (context);
}

// Called by the artwork plugin on a non-main thread
static void
_cover_callback (int error, ddb_cover_query_t *query, ddb_cover_info_t *cover) {
    query_data_t *query_data = query->user_data;
    const int cancelled = (query->flags & DDB_ARTWORK_FLAG_CANCELLED) != 0;

    // The covers loaded with DDB_ARTWORK_FLAG_SKIP_CACHE are in memory.
    // cover_type, image_data and image_data_size were added in API 2.1
    uint8_t *data = NULL;
    size_t data_size = 0;
    char *filename = NULL;
    ddb_artwork_cover_type_t cover_type = DDB_ARTWORK_COVER_TYPE_DEFAULT;
    if (!cancelled && error == 0 && cover != NULL && cover->cover_found) {
        if (cover->image_filename != NULL) {
            filename = strdup (cover->image_filename);
        }
        else if (
            cover->_size >= offsetof (ddb_cover_info_t, image_data_size) + sizeof (cover->image_data_size)
            && cover->image_data != NULL && cover->image_data_size != 0) {
            data = malloc (cover->image_data_size);
            if (data != NULL) {
                memcpy (data, cover->image_data, cover->image_data_size);
                data_size = cover->image_data_size;
            }
        }
        if (cover->_size >= offsetof (ddb_cover_info_t, cover_type) + sizeof (cover->cover_type)) {
            cover_type = cover->cover_type;
        }
    }

    deadbeef->pl_item_unref (query->track);
    free (query);

    // Decode the picture in the background, and release the cover after that
    dispatch_async (dispatch_get_global_queue (DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        GdkPixbuf *pixbuf = cancelled ? NULL : _decode_picture (data, data_size, filename);
        free (data);
        free (filename);

        if (cover != NULL) {
            ddb_artwork_plugin_t *plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
            if (plugin != NULL) {
                plugin->cover_info_release (cover);
            }
        }

        gtkui_dispatch_on_main (^{
            _cover_loaded (query_data, pixbuf, cover_type);
        });
    });
}

// Issues the queries for all the covers in the track, @c count of them.
static void
_request_covers (ddb_playItem_t *track, ddb_artwork_plugin_t *plugin, int count) {
    if (count <= 0) {
        _show_message (_("No artwork found."));
        return;
    }

    load_context_t *context = calloc (1, sizeof (load_context_t));
    context->generation = _ui.generation;
    context->count = count;
    context->pending = count;
    context->results = calloc (count, sizeof (cover_result_t));

    for (int i = 0; i < count; i++) {
        query_data_t *query_data = calloc (1, sizeof (query_data_t));
        query_data->context = context;
        query_data->index = i;

        // Only the covers embedded in the file are shown: the queries don't search the folders and online services,
        // and don't use the cache, so that the images are returned in memory.
        ddb_cover_query_t *query = calloc (1, sizeof (ddb_cover_query_t));
        query->_size = sizeof (ddb_cover_query_t);
        query->user_data = query_data;
        query->flags =
            DDB_ARTWORK_FLAG_ONLY_EMBEDDED | DDB_ARTWORK_FLAG_USE_COVER_INDEX | DDB_ARTWORK_FLAG_SKIP_CACHE;
        query->track = track;
        deadbeef->pl_item_ref (track);
        query->source_id = _ui.source_id;
        query->cover_index = i;
        plugin->cover_get (query, _cover_callback);
    }
}

#pragma mark - Public

void
trkproperties_artwork_cancel (void) {
    _ui.generation++;
    if (_ui.source_id != 0) {
        ddb_artwork_plugin_t *plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
        if (plugin != NULL) {
            plugin->cancel_queries_with_source_id (_ui.source_id);
        }
    }
    _stop_progress ();
}

void
trkproperties_artwork_reload (ddb_playItem_t **tracks, int count) {
    if (_ui.store == NULL) {
        return;
    }

    // Cancel and invalidate any pending load
    trkproperties_artwork_cancel ();
    gtk_list_store_clear (_ui.store);

    if (count > 1) {
        _show_message (_("Artwork can't be shown for multiple tracks. Select a single track."));
        return;
    }

    ddb_artwork_plugin_t *plugin = (ddb_artwork_plugin_t *)deadbeef->plug_get_for_id ("artwork2");
    if (count != 1 || plugin == NULL) {
        _show_message (_("No artwork found."));
        return;
    }

    if (_ui.source_id == 0) {
        _ui.source_id = plugin->allocate_source_id ();
    }

    _show_progress ();

    ddb_playItem_t *track = tracks[0];
    const unsigned generation = _ui.generation;

    // Counting the covers reads the file, so it's done in the background.
    // The track is referenced, because the track list can be replaced in the meantime.
    deadbeef->pl_item_ref (track);
    dispatch_async (dispatch_get_global_queue (DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        int cover_count = 0;
        if (plugin->plugin.plugin.version_minor >= 1) {
            cover_count = plugin->cover_count_in_track (track);
        }

        gtkui_dispatch_on_main (^{
            if (generation == _ui.generation && _ui.store != NULL) {
                _request_covers (track, plugin, cover_count);
            }
            // the queries hold their own references
            deadbeef->pl_item_unref (track);
        });
    });
}

static void
_close_clicked (GtkButton *button, gpointer user_data) {
    trkproperties_destroy ();
}

static void
_tab_destroyed (GtkWidget *widget, gpointer user_data) {
    trkproperties_artwork_cancel ();
    _ui.scrolled = NULL;
    _ui.store = NULL;
    _ui.message_label = NULL;
    _ui.spinner = NULL;
}

void
trkproperties_artwork_create_tab (GtkNotebook *notebook) {
    GtkWidget *page = gtk_vbox_new (FALSE, 8);
    gtk_container_set_border_width (GTK_CONTAINER (page), 12);

    GtkListStore *store = gtk_list_store_new (COL_COUNT, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    GtkWidget *tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
    // the tree view holds the reference to the store
    g_object_unref (store);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), TRUE);
    // The single selection mode allows an empty selection
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (tree)), GTK_SELECTION_SINGLE);

    GtkCellRenderer *name_renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *name_column =
        gtk_tree_view_column_new_with_attributes (_("Name"), name_renderer, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_resizable (name_column, TRUE);
    gtk_tree_view_column_set_min_width (name_column, 120);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), name_column);

    GtkCellRenderer *picture_renderer = gtk_cell_renderer_pixbuf_new ();
    // the pictures are left-aligned in the cell
    g_object_set (picture_renderer, "xalign", 0.0f, NULL);
    GtkTreeViewColumn *picture_column =
        gtk_tree_view_column_new_with_attributes (_("Picture"), picture_renderer, "pixbuf", COL_PICTURE, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), picture_column);

    GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (scrolled), tree);
    gtk_box_pack_start (GTK_BOX (page), scrolled, TRUE, TRUE, 0);

    GtkWidget *message_label = gtk_label_new ("");
    gtk_label_set_line_wrap (GTK_LABEL (message_label), TRUE);
    gtk_label_set_justify (GTK_LABEL (message_label), GTK_JUSTIFY_CENTER);
    gtk_misc_set_alignment (GTK_MISC (message_label), 0.5f, 0.5f);
    gtk_box_pack_start (GTK_BOX (page), message_label, TRUE, TRUE, 0);

    GtkWidget *spinner = gtk_spinner_new ();
    gtk_widget_set_size_request (spinner, 32, 32);
    gtk_box_pack_start (GTK_BOX (page), spinner, TRUE, FALSE, 0);

    // The Close button at the bottom, like in the other tabs. The dialog can have no title bar, e.g. in tiling window
    // managers.
    GtkWidget *button_box = gtk_hbutton_box_new ();
    gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
    gtk_box_pack_start (GTK_BOX (page), button_box, FALSE, FALSE, 0);

    GtkWidget *close_button = gtk_button_new ();
    gtk_widget_set_can_default (close_button, TRUE);
    gtk_container_add (GTK_CONTAINER (button_box), close_button);

    GtkWidget *close_alignment = gtk_alignment_new (0.5f, 0.5f, 0, 0);
    gtk_container_add (GTK_CONTAINER (close_button), close_alignment);

    GtkWidget *close_box = gtk_hbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (close_alignment), close_box);

    GtkWidget *close_image = gtk_image_new_from_stock ("gtk-close", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start (GTK_BOX (close_box), close_image, FALSE, FALSE, 0);

    GtkWidget *close_label = gtk_label_new_with_mnemonic (_("_Close"));
    gtk_box_pack_start (GTK_BOX (close_box), close_label, FALSE, FALSE, 0);

    g_signal_connect (close_button, "clicked", G_CALLBACK (_close_clicked), NULL);

    // The scrolled window, the label, and the spinner only appear when they're needed
    gtk_widget_show (tree);
    gtk_widget_show_all (button_box);
    gtk_widget_show (page);

    GtkWidget *tab_label = gtk_label_new (_("Artwork"));
    gtk_widget_show (tab_label);
    gtk_notebook_append_page (notebook, page, tab_label);

    _ui.scrolled = scrolled;
    _ui.store = store;
    _ui.message_label = message_label;
    _ui.spinner = spinner;

    g_signal_connect (page, "destroy", G_CALLBACK (_tab_destroyed), NULL);
}
