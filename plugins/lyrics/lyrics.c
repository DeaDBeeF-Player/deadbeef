/*
    Lyrics widget plugin for DeaDBeeF Player
    Copyright (C) 2009-2025 Oleksiy Yakovenko

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
#include <deadbeef/deadbeef.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif
#include "../gtkui/gtkui_api.h"
#include "../../gettext.h"
#include "support.h"
#include <stdbool.h>

DB_functions_t *deadbeef;
static ddb_gtkui_t *gtkui_plugin;

typedef struct {
    ddb_gtkui_widget_t base;
    GtkWidget *text_view;
    ddb_playItem_t *track;
} w_lyrics_t;

static void _apply_tags(GtkTextBuffer *buffer) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    // Apply heading tag to the first line
    GtkTextIter first_line_end = start;
    gtk_text_iter_forward_to_line_end(&first_line_end);
    gtk_text_buffer_apply_tag_by_name(buffer, "heading", &start, &first_line_end);
}

static void
_update_ui(w_lyrics_t *lyrics) {
    const char *not_avail = _("Lyrics Not Available");
    char *artist = NULL;
    char *title = NULL;
    char *lyrics_text = NULL;
    if (lyrics->track != NULL) {
        // get lyrics from tags
        deadbeef->pl_lock();
        size_t buffer_size = 100000;
        char *lyrics_buffer = malloc(100000);
        *lyrics_buffer = 0;
        // A bit of a hack since deadbeef stores multiline values as 0-separated lines.
        DB_metaInfo_t *meta = deadbeef->pl_meta_for_key(lyrics->track, "lyrics");
        if (meta != NULL) {
            size_t value_size = meta->valuesize;
            if (value_size > buffer_size - 1) {
                value_size = buffer_size - 1;
            }
            memcpy(lyrics_buffer, meta->value, value_size);
            char *p = lyrics_buffer;
            for (size_t i = 0; i <= value_size; i++, p++) {
                if (*p == 0) {
                    *p = '\n';
                }
            }
            lyrics_buffer[value_size] = 0;
            lyrics_text = lyrics_buffer;
        }
        else {
            free (lyrics_buffer);
            lyrics_buffer = NULL;
        }
        const char *str = deadbeef->pl_find_meta (lyrics->track, "artist");
        if (str != NULL) {
            artist = strdup(str);
        }
        str = deadbeef->pl_find_meta (lyrics->track, "title");
        if (str != NULL) {
            title = strdup(str);
        }

        deadbeef->pl_unlock();
    }

    bool have_heading = false;
    if (artist != NULL && title != NULL && lyrics_text != NULL) {
        size_t new_len = strlen (lyrics_text) + 1000;
        char *with_heading = malloc (new_len);
        snprintf (with_heading, new_len, "%s - %s\n%s", artist, title, lyrics_text);
        free (lyrics_text);
        lyrics_text = with_heading;
        have_heading = true;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (lyrics->text_view));
    GtkTextTagTable *tag_table = gtk_text_buffer_get_tag_table (buffer);
    GtkTextTag *heading_tag = gtk_text_tag_new ("heading");
    g_object_set (heading_tag, "weight", PANGO_WEIGHT_BOLD, "size", 16 * PANGO_SCALE, NULL);
    gtk_text_tag_table_add (tag_table, heading_tag);

    gtk_text_buffer_set_text (buffer, lyrics_text ?: not_avail, -1);

    if (lyrics_text == NULL || have_heading) {
        _apply_tags (buffer);
    }

    free (artist);
    free (title);
    free (lyrics_text);
}

static gboolean
_update(gpointer w) {
    w_lyrics_t *lyrics = w;
    int cursor = deadbeef->pl_get_cursor(PL_MAIN);
    if (cursor == -1) {
        lyrics->track = NULL;
    }
    else {
        ddb_playlist_t *plt = deadbeef->plt_get_curr();
        if (plt) {
            ddb_playItem_t *track = deadbeef->plt_get_item_for_idx(plt, cursor, PL_MAIN);

            if (track != NULL) {
                lyrics->track = track;
                deadbeef->pl_item_unref (track);
            }

            deadbeef->plt_unref (plt);
        }
    }

    _update_ui(w);

    return FALSE;
}

static int
lyrics_message (ddb_gtkui_widget_t *w, uint32_t id, uintptr_t ctx, uint32_t p1, uint32_t p2) {
    switch (id) {
    case DB_EV_CURSOR_MOVED:
        g_idle_add (_update, w);
        break;
    }
    return 0;
}

static void
w_lyrics_init (struct ddb_gtkui_widget_s *w) {
}


static ddb_gtkui_widget_t *
w_lyrics_create (void) {
    w_lyrics_t *w = calloc (1, sizeof (w_lyrics_t));

    w->base.widget = gtk_event_box_new ();
    w->base.init = w_lyrics_init;
    w->base.message = lyrics_message;

    gtk_widget_set_can_focus (w->base.widget, FALSE);

    GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_set_can_focus (scroll, FALSE);
    gtk_widget_show (scroll);
    gtk_container_add (GTK_CONTAINER (w->base.widget), scroll);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
    w->text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(w->text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(w->text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(w->text_view), GTK_WRAP_WORD);
    gtk_text_view_set_justification(GTK_TEXT_VIEW(w->text_view), GTK_JUSTIFY_CENTER);
    gtk_text_view_set_pixels_above_lines(GTK_TEXT_VIEW(w->text_view), 5);
    gtk_text_view_set_pixels_below_lines(GTK_TEXT_VIEW(w->text_view), 5);
    gtk_text_view_set_pixels_inside_wrap(GTK_TEXT_VIEW(w->text_view), 5);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(w->text_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(w->text_view), 8);
    gtk_container_set_border_width(GTK_CONTAINER(w->text_view), 8);
    gtk_widget_show(w->text_view);
    gtk_container_add (GTK_CONTAINER (scroll), w->text_view);

    gtkui_plugin->w_override_signals (w->base.widget, w);

    _update (w);

    return (ddb_gtkui_widget_t *)w;
}

static int
lyrics_connect (void) {
    gtkui_plugin = (ddb_gtkui_t *)deadbeef->plug_get_for_id (DDB_GTKUI_PLUGIN_ID);
    if(!gtkui_plugin) {
        return -1;
    }
    gtkui_plugin->w_reg_widget (_("Lyrics"), 0, w_lyrics_create, "lyrics", NULL);

    return 0;
}

static int
lyrics_disconnect (void) {
    if (gtkui_plugin) {
        gtkui_plugin->w_unreg_widget ("lyrics");
    }
    return 0;
}

static DB_misc_t plugin = {
    DDB_PLUGIN_SET_API_VERSION
    .plugin.version_major = 1,
    .plugin.version_minor = 0,
    .plugin.type = DB_PLUGIN_MISC,
#if GTK_CHECK_VERSION(3,0,0)
    .plugin.id = "lyrics_gtk3",
#else
    .plugin.id = "lyrics_gtk2",
#endif
    .plugin.name = "Lyrics Viewer",
    .plugin.descr = "Use View -> Design Mode to add the Lyrics Viewer into main window",
    .plugin.copyright =
        "Lyrics widget plugin for DeaDBeeF Player\n"
        "Copyright (C) 2009-2025 Oleksiy Yakovenko\n"
        "\n"
        "This software is provided 'as-is', without any express or implied\n"
        "warranty.  In no event will the authors be held liable for any damages\n"
        "arising from the use of this software.\n"
        "\n"
        "Permission is granted to anyone to use this software for any purpose,\n"
        "including commercial applications, and to alter it and redistribute it\n"
        "freely, subject to the following restrictions:\n"
        "\n"
        "1. The origin of this software must not be misrepresented; you must not\n"
        " claim that you wrote the original software. If you use this software\n"
        " in a product, an acknowledgment in the product documentation would be\n"
        " appreciated but is not required.\n"
        "\n"
        "2. Altered source versions must be plainly marked as such, and must not be\n"
        " misrepresented as being the original software.\n"
        "\n"
        "3. This notice may not be removed or altered from any source distribution.\n"
    ,
    .plugin.website = "http://deadbeef.sf.net",
    .plugin.connect = lyrics_connect,
    .plugin.disconnect = lyrics_disconnect
};

DB_plugin_t *
#if GTK_CHECK_VERSION(3,0,0)
lyrics_gtk3_load (DB_functions_t *api) {
#else
lyrics_gtk2_load (DB_functions_t *api) {
#endif
    deadbeef = api;
    return DB_PLUGIN(&plugin);
}
