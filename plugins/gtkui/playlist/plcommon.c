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

#include <jansson.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gdk/gdkkeysyms.h>
#include "gtkui.h"
#include <deadbeef/strdupa.h>
#include "../../libparser/parser.h"
#include "../../../shared/deletefromdisk.h"
#include "actions.h"
#include "actionhandlers.h"
#include "clipboard.h"
#include "drawing.h"
#include "../interface.h"
#include "plcommon.h"
#include "../support.h"
#include "trkproperties.h"
#include "../../../shared/tftintutil.h"

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

//#define trace(...) { fprintf(stderr, __VA_ARGS__); }
#define trace(fmt,...)

#define SUBGROUP_DELIMITER "|||"

// playlist theming
GtkWidget *theme_button;
GtkWidget *theme_treeview;
GdkPixbuf *play16_pixbuf;
GdkPixbuf *pause16_pixbuf;
GdkPixbuf *buffering16_pixbuf;

struct pl_preset_column_format {
    enum pl_column_t id;
    char *title;
    char *format;
};

#define PRESET_COLUMN_NUMITEMS 14

static struct pl_preset_column_format pl_preset_column_formats[PRESET_COLUMN_NUMITEMS];

static void
init_preset_column_struct(void) {
    // There can only be one instance of columns of type DB_COLUMN_FILENUMBER, DB_COLUMN_PLAYING, DB_COLUMN_ALBUM_ART and DB_COLUMN_CUSTOM
    struct pl_preset_column_format data[] = {
        {DB_COLUMN_FILENUMBER, _("Item Index"), NULL},
        {DB_COLUMN_PLAYING, _("Playing"), NULL},
        {DB_COLUMN_ALBUM_ART, _("Album Art"), NULL},
        {DB_COLUMN_STANDARD, _("Artist - Album"), COLUMN_FORMAT_ARTISTALBUM},
        {DB_COLUMN_STANDARD, _("Artist"), COLUMN_FORMAT_ARTIST},
        {DB_COLUMN_STANDARD, _("Album"), COLUMN_FORMAT_ALBUM},
        {DB_COLUMN_STANDARD, _("Title"), COLUMN_FORMAT_TITLE},
        {DB_COLUMN_STANDARD, _("Year"), COLUMN_FORMAT_YEAR},
        {DB_COLUMN_STANDARD, _("Duration"), COLUMN_FORMAT_LENGTH},
        {DB_COLUMN_STANDARD, _("Track Number"), COLUMN_FORMAT_TRACKNUMBER},
        {DB_COLUMN_STANDARD, _("Band / Album Artist"), COLUMN_FORMAT_BAND},
        {DB_COLUMN_STANDARD, _("Codec"), COLUMN_FORMAT_CODEC},
        {DB_COLUMN_STANDARD, _("Bitrate"), COLUMN_FORMAT_BITRATE},
        {DB_COLUMN_CUSTOM, _("Custom"), NULL}
    };
    memcpy(pl_preset_column_formats, data, sizeof(pl_preset_column_formats));
}

int
find_first_preset_column_type (int type) {
    int idx = -1;
    for (int i = 0; i < PRESET_COLUMN_NUMITEMS; i++) {
        if (pl_preset_column_formats[i].id == type) {
            idx = i;
            break;
        }
    }
    return idx;
}

void
pl_common_init(void) {
    play16_pixbuf = create_pixbuf("play_16.png");
    g_object_ref_sink(play16_pixbuf);
    pause16_pixbuf = create_pixbuf("pause_16.png");
    g_object_ref_sink(pause16_pixbuf);
    buffering16_pixbuf = create_pixbuf("buffering_16.png");
    g_object_ref_sink(buffering16_pixbuf);

    theme_treeview = gtk_tree_view_new();
    gtk_widget_show(theme_treeview);
    gtk_widget_set_can_focus(theme_treeview, FALSE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(theme_treeview), TRUE);
    gtk_box_pack_start(GTK_BOX(gtk_bin_get_child(GTK_BIN(mainwin))), theme_treeview, FALSE, FALSE, 0);
#if GTK_CHECK_VERSION(3,0,0)
    GtkStyleContext *context = gtk_widget_get_style_context(theme_treeview);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_CELL);
    gtk_style_context_add_class(context, GTK_STYLE_CLASS_VIEW);
#endif
    theme_button = mainwin;

    init_preset_column_struct();
}

void
pl_common_free (void) {
    if (play16_pixbuf) {
        g_object_unref(play16_pixbuf);
        play16_pixbuf = NULL;
    }
    if (pause16_pixbuf) {
        g_object_unref(pause16_pixbuf);
        pause16_pixbuf = NULL;
    }
    if (buffering16_pixbuf) {
        g_object_unref(buffering16_pixbuf);
        buffering16_pixbuf = NULL;
    }
}

static col_info_t *
create_col_info (DdbListview *listview, int id) {
    col_info_t *info = calloc(1, sizeof(col_info_t));
    info->id = id;
    info->listview = listview;
    return info;
}

void
pl_common_free_col_info (void *data) {
    if (!data) {
        return;
    }

    col_info_t *info = data;
    if (info->format) {
        free (info->format);
    }
    if (info->bytecode) {
        free (info->bytecode);
    }
    if (info->sort_format) {
        free (info->sort_format);
    }
    if (info->sort_bytecode) {
        free (info->sort_bytecode);
    }
    free (info);
}

#define COL_CONF_BUFFER_SIZE 10000

int
pl_common_rewrite_column_config (DdbListview *listview, const char *name) {
    char *buffer = malloc (COL_CONF_BUFFER_SIZE);
    strcpy (buffer, "[");
    char *p = buffer+1;
    int n = COL_CONF_BUFFER_SIZE-3;

    int cnt = ddb_listview_column_get_count (listview);
    for (int i = 0; i < cnt; i++) {
        const char *title;
        int width;
        int align;
        col_info_t *info;
        int color_override;
        GdkColor color;
        ddb_listview_column_get_info (listview, i, &title, &width, &align, NULL, NULL, &color_override, &color, (void **)&info);

        char *esctitle = parser_escape_string (title);
        char *escformat = info->format ? parser_escape_string (info->format) : NULL;
        char *escsortformat = info->sort_format ? parser_escape_string (info->sort_format) : NULL;

        size_t written = snprintf (p, n, "{\"title\":\"%s\",\"id\":\"%d\",\"format\":\"%s\",\"sort_format\":\"%s\",\"size\":\"%d\",\"align\":\"%d\",\"color_override\":\"%d\",\"color\":\"#ff%02x%02x%02x\"}%s", esctitle, info->id, escformat ? escformat : "", escsortformat ? escsortformat : "", width, align, color_override, color.red>>8, color.green>>8, color.blue>>8, i < cnt-1 ? "," : "");
        free (esctitle);
        if (escformat) {
            free (escformat);
        }
        if (escsortformat) {
            free (escsortformat);
        }
        p += written;
        n -= written;
        if (n <= 0) {
            fprintf (stderr, "Column configuration is too large, doesn't fit in the buffer. Won't be written.\n");
            free (buffer);
            return -1;
        }
    }
    strcpy (p, "]");
    deadbeef->conf_set_str (name, buffer);
    deadbeef->conf_save ();
    free (buffer);
    return 0;
}

static int
min_group_height(void *user_data, int width) {
    col_info_t *info = (col_info_t *)user_data;
    if (info->id == DB_COLUMN_ALBUM_ART) {
        return width - 2*ART_PADDING_HORZ + 2*ART_PADDING_VERT;
    }
    return 0;
}

///// cover art display
int
pl_common_is_album_art_column (void *user_data) {
    col_info_t *info = (col_info_t *)user_data;
    return info->id == DB_COLUMN_ALBUM_ART;
}


#define CHANNEL_BLENDR(CHANNELA,CHANNELB,BLEND) ( \
	sqrt(((1.0 - BLEND) * (CHANNELA * CHANNELA)) + (BLEND * (CHANNELB * CHANNELB))) \
)



PangoAttrList *
convert_escapetext_to_pango_attrlist (char *text, char **plainString, float *fg, float *bg, float *highlight) {
    const int maxTintStops = 100;
    tint_stop_t tintStops[maxTintStops];
    int numTintStops;

    numTintStops = calculate_tint_stops_from_string (text, tintStops, maxTintStops, plainString);

    size_t strLength = strlen(*plainString);

    PangoAttrList *lst = pango_attr_list_new ();
    PangoAttribute *attr = NULL;

    // add attributes
    for (guint i = 0; i < numTintStops; i++) {
        int index0 = tintStops[i].byteindex;
        size_t len = strLength - index0;

        GdkColor finalColor = { .red = fg[0]*65535, .green = fg[1]*65535, .blue = fg[2]*65535};
        if (tintStops[i].has_rgb) {
            finalColor.red = tintStops[i].r * 65535/255.;
            finalColor.green = tintStops[i].g * 65535/255.;
            finalColor.blue = tintStops[i].b * 65535/255.;
        }

        int tint = tintStops[i].tint;

        if (tint >= 1 && tint <= 3) {
            const float blend[] = {.50f, .25f, 0};
            finalColor.red   = CHANNEL_BLENDR(highlight[0], finalColor.red/65535., blend[tint-1]) * 65535;
            finalColor.green = CHANNEL_BLENDR(highlight[1], finalColor.green/65535., blend[tint-1]) * 65535;
            finalColor.blue  = CHANNEL_BLENDR(highlight[2], finalColor.blue/65535., blend[tint-1]) * 65535;

        } else if (tint >= -3 && tint <= -1) {
            const float blend[] = {.30f, .60f, .80f};
            finalColor.red   = CHANNEL_BLENDR(finalColor.red/65535., bg[0], blend[-tint-1]) * 65535;
            finalColor.green = CHANNEL_BLENDR(finalColor.green/65535., bg[1], blend[-tint-1]) * 65535;
            finalColor.blue  = CHANNEL_BLENDR(finalColor.blue/65535., bg[2], blend[-tint-1]) * 65535;
        }

        attr = pango_attr_foreground_new (finalColor.red, finalColor.green, finalColor.blue);
        attr->start_index = index0;
        attr->end_index = (guint)(index0 + len);
        pango_attr_list_insert (lst, attr);
    }

    return lst;
}

// FIXME: duplicate with plmenu.c
static GtkWidget*
find_popup (GtkWidget *widget) {
    GtkWidget *parent = widget;
    do {
        widget = parent;
        if (GTK_IS_MENU (widget))
            parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
        else
            parent = gtk_widget_get_parent (widget);
        if (!parent)
            parent = (GtkWidget*) g_object_get_data (G_OBJECT (widget), "GladeParentKey");
    } while (parent);

    return widget;
}

static DdbListview *
get_context_menu_listview (GtkMenuItem *menuitem) {
    return DDB_LISTVIEW (g_object_get_data (G_OBJECT (find_popup (GTK_WIDGET (menuitem))), "ps"));
}

static int
get_context_menu_column (GtkMenuItem *menuitem) {
    return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (gtk_widget_get_parent (GTK_WIDGET (menuitem))), "column"));
}

static char *
strtok_stringdelim_r (char *str, const char *delim,  char **next_start) {
    if (*next_start) {
        str = *next_start;
    }

    if (!str || str[0] == 0) {
        *next_start = NULL;
        return NULL;
    }

    char *next = strstr(str, delim);

    if (next) {
        *next = 0;
        *next_start = next + strlen(delim);
    }
    else {
        *next_start = str + strlen(str);
    }

    return str;
}

gboolean
list_handle_keypress (DdbListview *ps, int keyval, int state, int iter) {
    int prev = ps->datasource->cursor ();
    int cursor = prev;
    GtkWidget *range = ps->scrollbar;
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (range));

    state &= (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD4_MASK);

    if (state & GDK_CONTROL_MASK) {
        int res = 0;
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            if (keyval == GDK_c) {
                clipboard_copy_selection (plt, DDB_ACTION_CTX_SELECTION);
                res = 1;
            }
            else if (keyval == GDK_v && iter != PL_SEARCH) {
                clipboard_paste_selection (plt, DDB_ACTION_CTX_SELECTION);
                res = 1;
            }
            else if (keyval == GDK_x) {
                clipboard_cut_selection (plt, DDB_ACTION_CTX_SELECTION);
                res = 1;
            }
            deadbeef->plt_unref (plt);
            return res;
        }
    }

    if (state & ~GDK_SHIFT_MASK) {
        return FALSE;
    }

    if (keyval == GDK_Down) {
        if (cursor < ps->datasource->count () - 1) {
            cursor++;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
        }
    }
    else if (keyval == GDK_Up) {
        if (cursor > 0) {
            cursor--;
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
            if (cursor < 0 && ps->datasource->count () > 0) {
                cursor = 0;
            }
        }
    }
    else if (keyval == GDK_Page_Down) {
        if (cursor < ps->datasource->count () - 1) {
            cursor += 10;
            if (cursor >= ps->datasource->count ()) {
                cursor = ps->datasource->count () - 1;
            }
        }
        else {
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
        }
    }
    else if (keyval == GDK_Page_Up) {
        if (cursor > 0) {
            cursor -= 10;
            if (cursor < 0) {
                gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
                cursor = 0;
            }
        }
        else {
            if (cursor < 0 && ps->datasource->count () > 0) {
                cursor = 0;
            }
            gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
        }
    }
    else if (keyval == GDK_End) {
        cursor = ps->datasource->count () - 1;
        gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_upper (adj));
    }
    else if (keyval == GDK_Home) {
        cursor = 0;
        gtk_range_set_value (GTK_RANGE (range), gtk_adjustment_get_lower (adj));
    }
    else {
        return FALSE;
    }

    if (state & GDK_SHIFT_MASK) {
        if (cursor != prev) {
            int scrollpos = ddb_listview_get_scroll_pos(ps);
            int list_height = ddb_listview_get_list_height(ps);
            int newscroll = scrollpos;
            int cursor_scroll = ddb_listview_get_row_pos (ps, cursor, NULL);
            if (cursor_scroll < scrollpos) {
                newscroll = cursor_scroll;
            }
            else if (cursor_scroll >= scrollpos + list_height) {
                newscroll = cursor_scroll - list_height + 1;
                if (newscroll < 0) {
                    newscroll = 0;
                }
            }
            if (scrollpos != newscroll) {
                GtkWidget *range = ps->scrollbar;
                gtk_range_set_value (GTK_RANGE (range), newscroll);
            }

            // select all between shift_sel_anchor and deadbeef->pl_get_cursor (ps->iterator)
            int shift_sel_anchor = ddb_listview_get_shift_sel_anchor(ps);
            int start = min (cursor, shift_sel_anchor);
            int end = max (cursor, shift_sel_anchor);

            ddb_listview_select_range (ps, start, end);
            ddb_listview_update_cursor (ps, cursor);
        }
    }
    else {
        ddb_listview_set_shift_sel_anchor(ps, cursor);
        ddb_listview_set_cursor_and_scroll (ps, cursor);
    }

    return TRUE;
}

static void
on_group_by_none_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    pl_common_set_group_format (get_context_menu_listview (menuitem) , "");
}

static void
on_pin_groups_active                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    int old_val = deadbeef->conf_get_int ("playlist.pin.groups", 0);
    deadbeef->conf_set_int ("playlist.pin.groups", old_val ? 0 : 1);
    deadbeef->sendmessage (DB_EV_CONFIGCHANGED, (uintptr_t)"playlist.pin.groups", 0, 0);
    gtk_check_menu_item_toggled(GTK_CHECK_MENU_ITEM(menuitem));
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref(plt);
    }
}

static void
on_group_by_artist_date_album_activate (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    pl_common_set_group_format (get_context_menu_listview (menuitem), "%album artist% - ['['$year(%date%)']' ]%album%");
}

static void
on_group_by_artist_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    if (plt) {
        deadbeef->plt_modified (plt);
        deadbeef->plt_unref (plt);
    }
    pl_common_set_group_format (get_context_menu_listview (menuitem), "%artist%");
}


static void
_make_format (char *format, size_t size, const DdbListviewGroupFormat *fmt) {
    format[0] = 0;
    while (fmt) {
        if (format[0] != 0) {
            strncat(format, SUBGROUP_DELIMITER, size - strlen(format) - 1);
        }
        strncat(format, fmt->format, size - strlen(format) - 1);
        fmt = fmt->next;
    }
}

static void
on_group_by_custom_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    GtkWidget *dlg = create_groupbydlg ();

    DdbListview *listview = get_context_menu_listview (menuitem);
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    GtkWidget *entry = lookup_widget (dlg, "format");
    char format[1024];

    const DdbListviewGroupFormat *group_formats = ddb_listview_get_group_formats (listview);
    _make_format(format, sizeof (format), group_formats);

    gtk_entry_set_text (GTK_ENTRY (entry), format);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));

    if (response == GTK_RESPONSE_OK) {
        const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
        ddb_playlist_t *plt = deadbeef->plt_get_curr ();
        if (plt) {
            deadbeef->plt_modified (plt);
            deadbeef->plt_unref (plt);
        }
        pl_common_set_group_format (listview, text);
    }
    gtk_widget_destroy (dlg);
}

int
pl_common_load_column_config (DdbListview *listview, const char *key) {
    deadbeef->conf_lock ();
    const char *json = deadbeef->conf_get_str_fast (key, NULL);
    json_error_t error;
    if (!json) {
        deadbeef->conf_unlock ();
        return -1;
    }
    json_t *root = json_loads (json, 0, &error);
    deadbeef->conf_unlock ();
    if(!root) {
        printf ("json parse error for config variable %s\n", key);
        return -1;
    }
    if (!json_is_array (root))
    {
        goto error;
    }
    for (int i = 0; i < json_array_size(root); i++) {
        json_t *title, *align, *id, *format, *sort_format, *width, *color_override, *color;
        json_t *data = json_array_get (root, i);
        if (!json_is_object (data)) {
            goto error;
        }
        title = json_object_get (data, "title");
        align = json_object_get (data, "align");
        id = json_object_get (data, "id");
        format = json_object_get (data, "format");
        sort_format = json_object_get (data, "sort_format");
        width = json_object_get (data, "size");
        color_override = json_object_get (data, "color_override");
        color = json_object_get (data, "color");
        if (!json_is_string (title)
                || !json_is_string (id)
                || !json_is_string (width)
           ) {
            goto error;
        }
        const char *stitle = NULL;
        int ialign = -1;
        int iid = -1;
        const char *sformat = NULL;
        const char *ssort_format = NULL;
        int iwidth = 0;
        int icolor_override = 0;
        const char *scolor = NULL;
        GdkColor gdkcolor = {0};
        stitle = json_string_value (title);
        if (json_is_string (align)) {
            ialign = atoi (json_string_value (align));
        }
        if (json_is_string (id)) {
            iid = atoi (json_string_value (id));
        }
        if (json_is_string (format)) {
            sformat = json_string_value (format);
            if (!sformat[0]) {
                sformat = NULL;
            }
        }
        if (json_is_string (sort_format)) {
            ssort_format = json_string_value (sort_format);
            if (!ssort_format[0]) {
                ssort_format = NULL;
            }
        }
        if (json_is_string (width)) {
            iwidth = atoi (json_string_value (width));
            if (iwidth < 0) {
                iwidth = 50;
            }
        }
        if (json_is_string (color_override)) {
            icolor_override = atoi (json_string_value (color_override));
        }
        if (json_is_string (color)) {
            scolor = json_string_value (color);
            int r, g, b, a;
            if (4 == sscanf (scolor, "#%02x%02x%02x%02x", &a, &r, &g, &b)) {
                gdkcolor.red = r<<8;
                gdkcolor.green = g<<8;
                gdkcolor.blue = b<<8;
            }
            else {
                icolor_override = 0;
            }
        }

        col_info_t *inf = create_col_info(listview, iid);
        if (sformat) {
            inf->format = strdup (sformat);
            inf->bytecode = deadbeef->tf_compile (inf->format);
        }
        if (ssort_format) {
            inf->sort_format = strdup (ssort_format);
            inf->sort_bytecode = deadbeef->tf_compile (inf->sort_format);
        }
        ddb_listview_column_append (listview, stitle, iwidth, ialign, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, icolor_override, gdkcolor, inf);
    }
    json_decref(root);
    return 0;
error:
    fprintf (stderr, "%s config variable contains invalid data, ignored\n", key);
    json_decref(root);
    return -1;
}

static void
init_column (col_info_t *inf, int id, const char *format, const char *sort_format) {
    if (inf->format) {
        free (inf->format);
        inf->format = NULL;
    }
    if (inf->sort_format) {
        free (inf->sort_format);
        inf->sort_format = NULL;
    }
    if (inf->bytecode) {
        deadbeef->tf_free (inf->bytecode);
        inf->bytecode = NULL;
    }
    if (inf->sort_bytecode) {
        deadbeef->tf_free (inf->sort_bytecode);
        inf->sort_bytecode = NULL;
    }

    inf->id = pl_preset_column_formats[id].id;

    if (pl_preset_column_formats[id].id == DB_COLUMN_CUSTOM) {
        inf->format = strdup (format);
    } else if (pl_preset_column_formats[id].id == DB_COLUMN_STANDARD && pl_preset_column_formats[id].format) {
        inf->format = strdup (pl_preset_column_formats[id].format);
    }

    if (inf->format) {
        inf->bytecode = deadbeef->tf_compile (inf->format);
    }

    if (sort_format) {
        inf->sort_format = strdup(sort_format);
        inf->sort_bytecode = deadbeef->tf_compile (inf->sort_format);
    }
}

static void
populate_column_id_combo_box(GtkComboBoxText *combo_box) {
    for (int i = 0; i < PRESET_COLUMN_NUMITEMS; i++) {
        gtk_combo_box_text_append_text (combo_box, pl_preset_column_formats[i].title);
    }
}

int editcolumn_title_changed = 0;

static void
on_add_column_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    editcolumn_title_changed = 0;
    GdkColor color;
    gtkui_get_listview_text_color (&color);

    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Add column"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    populate_column_id_combo_box (GTK_COMBO_BOX_TEXT (lookup_widget (dlg, "id")));
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), 0);
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")), 0);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &color);
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        const gchar *sort_format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "sort_format")));
        int sel = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));

        int clr_override = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")));
        GdkColor clr;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &clr);

        col_info_t *inf = create_col_info(user_data, 0);
        init_column (inf, sel, format, sort_format);

        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));
        DdbListview *listview = get_context_menu_listview (menuitem);
        int before = get_context_menu_column (menuitem);
        ddb_listview_column_insert (listview, before, title, 100, align, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, clr_override, clr, inf);
        ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL);
    }
    gtk_widget_destroy (dlg);
}

static void
on_edit_column_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    int active_column = get_context_menu_column (menuitem);
    if (active_column == -1)
        return;

    DdbListview *listview = get_context_menu_listview (menuitem);
    GtkWidget *dlg = create_editcolumndlg ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Edit column"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (mainwin));
    populate_column_id_combo_box (GTK_COMBO_BOX_TEXT (lookup_widget (dlg, "id")));

    const char *title;
    int width;
    int align_right;
    col_info_t *inf;
    int color_override;
    GdkColor color;
    int res = ddb_listview_column_get_info (listview, active_column, &title, &width, &align_right, NULL, NULL, &color_override, &color, (void **)&inf);
    if (res == -1) {
        trace ("attempted to edit non-existing column\n");
        return;
    }

    int idx = -1;

    // get index for id
    if (inf->id == DB_COLUMN_STANDARD) {
        for (int i = 0; i < PRESET_COLUMN_NUMITEMS; i++) {
            if (pl_preset_column_formats[i].id == DB_COLUMN_STANDARD && inf->format &&
                pl_preset_column_formats[i].format && !strcmp (inf->format, pl_preset_column_formats[i].format)) {
                idx = i;
                break;
            }
        }
        // old style custom column
        if (idx == -1) {
            idx = find_first_preset_column_type(DB_COLUMN_CUSTOM);
        }
    } else {
        idx = find_first_preset_column_type(inf->id);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")), idx);
    if (idx == PRESET_COLUMN_NUMITEMS-1) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "format")), inf->format);
    }
    if (inf->sort_format) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "sort_format")), inf->sort_format);
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")), align_right);
    gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), title);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")), color_override);

    gtk_color_button_set_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &color);
    editcolumn_title_changed = 0;
    gint response = gtk_dialog_run (GTK_DIALOG (dlg));
    if (response == GTK_RESPONSE_OK) {
        const gchar *title = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title")));
        const gchar *format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "format")));
        const gchar *sort_format = gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "sort_format")));
        int id = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "id")));
        int align = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "align")));

        int clr_override = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "color_override")));
        GdkColor clr;
        gtk_color_button_get_color (GTK_COLOR_BUTTON (lookup_widget (dlg, "color")), &clr);

        init_column (inf, id, format, sort_format);
        ddb_listview_column_set_info (listview, active_column, title, width, align, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, clr_override, clr, inf);

        ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST);
    }
    gtk_widget_destroy (dlg);
}


static void
on_remove_column_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data) {
    int active_column = get_context_menu_column (menuitem);
    if (active_column == -1)
        return;

    DdbListview *listview = get_context_menu_listview (menuitem);
    ddb_listview_column_remove (listview, active_column);
    ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_COLUMNS | DDB_REFRESH_LIST | DDB_REFRESH_HSCROLL);
}

static GtkWidget*
create_headermenu (DdbListview *listview, int column, int groupby) {
    GtkWidget *headermenu;
    GtkWidget *add_column;
    GtkWidget *edit_column;
    GtkWidget *remove_column;
    GtkWidget *separator;
    GtkWidget *group_by;
    GtkWidget *pin_groups;
    GtkWidget *group_by_menu;
    GtkWidget *none;
    GtkWidget *artist_date_album;
    GtkWidget *artist;
    GtkWidget *custom;

    headermenu = gtk_menu_new ();

    add_column = gtk_menu_item_new_with_mnemonic (_("Add column"));
    gtk_widget_show (add_column);
    gtk_container_add (GTK_CONTAINER (headermenu), add_column);

    edit_column = gtk_menu_item_new_with_mnemonic (_("Edit column"));
    gtk_widget_show (edit_column);
    gtk_container_add (GTK_CONTAINER (headermenu), edit_column);

    remove_column = gtk_menu_item_new_with_mnemonic (_("Remove column"));
    gtk_widget_show (remove_column);
    gtk_container_add (GTK_CONTAINER (headermenu), remove_column);

    if (column == -1) {
        gtk_widget_set_sensitive (edit_column, FALSE);
        gtk_widget_set_sensitive (remove_column, FALSE);
    }

    if (groupby) {
        separator = gtk_separator_menu_item_new ();
        gtk_widget_show (separator);
        gtk_container_add (GTK_CONTAINER (headermenu), separator);
        gtk_widget_set_sensitive (separator, FALSE);

        pin_groups = gtk_check_menu_item_new_with_mnemonic(_("Pin groups when scrolling"));
        gtk_widget_show (pin_groups);
        gtk_container_add (GTK_CONTAINER (headermenu), pin_groups);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (pin_groups), (gboolean)deadbeef->conf_get_int("playlist.pin.groups",0));

        group_by = gtk_menu_item_new_with_mnemonic (_("Group by"));
        gtk_widget_show (group_by);
        gtk_container_add (GTK_CONTAINER (headermenu), group_by);

        group_by_menu = gtk_menu_new ();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (group_by), group_by_menu);

        none = gtk_menu_item_new_with_mnemonic (_("None"));
        gtk_widget_show (none);
        gtk_container_add (GTK_CONTAINER (group_by_menu), none);

        artist_date_album = gtk_menu_item_new_with_mnemonic (_("Artist/Date/Album"));
        gtk_widget_show (artist_date_album);
        gtk_container_add (GTK_CONTAINER (group_by_menu), artist_date_album);

        artist = gtk_menu_item_new_with_mnemonic (_("Artist"));
        gtk_widget_show (artist);
        gtk_container_add (GTK_CONTAINER (group_by_menu), artist);

        custom = gtk_menu_item_new_with_mnemonic (_("Custom"));
        gtk_widget_show (custom);
        gtk_container_add (GTK_CONTAINER (group_by_menu), custom);

        g_signal_connect ((gpointer) none, "activate",
              G_CALLBACK (on_group_by_none_activate),
              NULL);

        g_signal_connect ((gpointer) pin_groups, "activate",
              G_CALLBACK (on_pin_groups_active),
              NULL);

        g_signal_connect ((gpointer) artist_date_album, "activate",
              G_CALLBACK (on_group_by_artist_date_album_activate),
              NULL);

        g_signal_connect ((gpointer) artist, "activate",
              G_CALLBACK (on_group_by_artist_activate),
              NULL);

        g_signal_connect ((gpointer) custom, "activate",
              G_CALLBACK (on_group_by_custom_activate),
              NULL);
    }

    g_signal_connect ((gpointer) add_column, "activate",
                      G_CALLBACK (on_add_column_activate),
                      listview);
    g_signal_connect ((gpointer) edit_column, "activate",
                      G_CALLBACK (on_edit_column_activate),
                      listview);
    g_signal_connect ((gpointer) remove_column, "activate",
                      G_CALLBACK (on_remove_column_activate),
                      listview);

    return headermenu;
}

void
pl_common_header_context_menu (DdbListview *ps, int column) {
    GtkWidget *menu = create_headermenu (ps, column, 1);
    g_object_set_data (G_OBJECT (menu), "ps", ps);
    g_object_set_data (G_OBJECT (menu), "column", GINT_TO_POINTER (column));

    gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

void
pl_common_add_column_helper (DdbListview *listview, const char *title, int width, int id, const char *format, const char *sort_format, int align_right) {
    if (!format) {
        format = "";
    }
    if (!sort_format) {
        sort_format = "";
    }
    col_info_t *inf = create_col_info(listview, id);
    inf->format = strdup (format);
    inf->bytecode = deadbeef->tf_compile (inf->format);
    inf->sort_format = strdup (sort_format);
    inf->sort_bytecode = deadbeef->tf_compile (inf->sort_format);
    GdkColor color = { 0, 0, 0, 0 };
    ddb_listview_column_append (listview, title, width, align_right, inf->id == DB_COLUMN_ALBUM_ART ? min_group_height : NULL, inf->id == DB_COLUMN_ALBUM_ART, 0, color, inf);
}

int
pl_common_get_group_text (DdbListview *listview, DdbListviewIter it, char *str, int size, int index) {
    const DdbListviewGroupFormat *fmt = ddb_listview_get_group_formats(listview);
    if (!fmt->format || !fmt->format[0]) {
        return -1;
    }
    while (index > 0) {
        index--;
        fmt = fmt->next;
        if (fmt == NULL) {
            return -1;
        }
    }
    if (fmt->bytecode) {
        ddb_tf_context_t ctx = {
            ._size = sizeof (ddb_tf_context_t),
            .it = it,
            .plt = deadbeef->plt_get_curr (),
            .flags = DDB_TF_CONTEXT_NO_DYNAMIC,
        };
        deadbeef->tf_eval (&ctx, fmt->bytecode, str, size);
        if (ctx.plt) {
            deadbeef->plt_unref (ctx.plt);
            ctx.plt = NULL;
        }

        char *lb = strchr (str, '\r');
        if (lb) {
            *lb = 0;
        }
        lb = strchr (str, '\n');
        if (lb) {
            *lb = 0;
        }
    }
    return fmt->next == NULL ? 0 : 1;
}

void
pl_common_draw_group_title (DdbListview *listview, cairo_t *drawable, DdbListviewIter it, int iter, int x, int y, int width, int height, int group_depth) {
    const DdbListviewGroupFormat *fmt = ddb_listview_get_group_formats(listview);
    if (fmt->format && fmt->format[0]) {
        char str[1024] = "";
        while (group_depth--) {
            fmt = fmt->next;
        }
        int isdimmed=0;
        if (fmt->bytecode) {
            ddb_tf_context_t ctx = {
                ._size = sizeof (ddb_tf_context_t),
                .it = it,
                .plt = deadbeef->plt_get_curr (),
                .flags = DDB_TF_CONTEXT_NO_DYNAMIC | DDB_TF_CONTEXT_TEXT_DIM,
                .iter = iter,
            };
            deadbeef->tf_eval (&ctx, fmt->bytecode, str, sizeof (str));
            if (ctx.plt) {
                deadbeef->plt_unref (ctx.plt);
                ctx.plt = NULL;
            }
            isdimmed = ctx.dimmed;


            char *lb = strchr (str, '\r');
            if (lb) {
                *lb = 0;
            }
            lb = strchr (str, '\n');
            if (lb) {
                *lb = 0;
            }
        }

        int theming = !gtkui_override_listview_colors ();
        GdkColor clr;
        if (theming) {
            clr = gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
        }
        else {
            gtkui_get_listview_group_text_color (&clr);
        }

        drawctx_t * const ctx = ddb_listview_get_grpctx(listview);

        float rgb[] = {clr.red/65535., clr.green/65535., clr.blue/65535.};
        draw_set_fg_color (ctx, rgb);

        int ew;
        int text_width = width-x-10;
        if (text_width > 0) {
            if (isdimmed) {
                GdkColor *highlight_color;
                GdkColor hlclr;
                if (!gtkui_override_listview_colors ()) {
                    highlight_color = &gtk_widget_get_style(theme_treeview)->fg[GTK_STATE_NORMAL];
                }
                else {
                    highlight_color = ((void)(gtkui_get_listview_group_text_color (&hlclr)), &hlclr);
                }

                float highlight[] = {highlight_color->red/65535., highlight_color->green/65535., highlight_color->blue/65535.};

                GdkColor *background_color;
                GdkColor bgclr;

                if (!gtkui_override_listview_colors ()) {
                    background_color = &gtk_widget_get_style(theme_treeview)->bg[GTK_STATE_NORMAL];
                } else {
                    gtkui_get_listview_odd_row_color (&bgclr);
                    background_color = &bgclr;
                }
                float bg[] = {background_color->red/65535., background_color->green/65535., background_color->blue/65535.};

                PangoAttrList *attrs;
                char *plainString;

                attrs = convert_escapetext_to_pango_attrlist(str, &plainString, rgb, bg, highlight);
                pango_layout_set_attributes (ctx->pangolayout, attrs);
                pango_attr_list_unref(attrs);
                draw_text_custom (ctx, x + 5, y + height/2 - draw_get_listview_rowheight (ctx)/2 + 3, text_width, 0, DDB_GROUP_FONT, 0, 0, plainString);
                free (plainString);
                pango_layout_set_attributes (ctx->pangolayout, NULL);
            } else {
                draw_text_custom (ctx, x + 5, y + height/2 - draw_get_listview_rowheight (ctx)/2 + 3, text_width, 0, DDB_GROUP_FONT, 0, 0, str);
            }
            draw_get_layout_extents (ctx, &ew, NULL);

            size_t len = strlen (str);
            int line_x = x + 10 + ew + (len ? ew / len / 2 : 0);
            if (line_x+20 < x + width) {
                draw_line (ctx, line_x, y+height/2, x+width, y+height/2);
            }
        }
    }
}

void
pl_common_selection_changed (DdbListview *ps, int iter, DB_playItem_t *it) {
    if (it) {
        ddb_event_track_t *ev = (ddb_event_track_t *)deadbeef->event_alloc(DB_EV_TRACKINFOCHANGED);
        ev->track = it;
        deadbeef->pl_item_ref(ev->track);
        deadbeef->event_send((ddb_event_t *)ev, DDB_PLAYLIST_CHANGE_SELECTION, iter);
    }
    else {
        deadbeef->sendmessage(DB_EV_PLAYLISTCHANGED, (uintptr_t)ps, DDB_PLAYLIST_CHANGE_SELECTION, iter);
    }
}

void
pl_common_col_sort (DdbListviewColumnSortOrder sort_order, int iter, void *user_data) {
    col_info_t *c = (col_info_t*)user_data;
    ddb_playlist_t *plt = deadbeef->plt_get_curr ();
    char *format = (c->sort_format && strlen(c->sort_format)) ? c->sort_format : c->format;
    deadbeef->plt_sort_v2 (plt, iter, c->id, format, sort_order == DdbListviewColumnSortOrderDescending ? DDB_SORT_DESCENDING : DDB_SORT_ASCENDING);
    deadbeef->plt_unref (plt);
    deadbeef->sendmessage (DB_EV_PLAYLISTCHANGED, 0, DDB_PLAYLIST_CHANGE_CONTENT, 0);
}

void
pl_common_set_group_format (DdbListview *listview, const char *_format) {
    char *format = strdup (_format);
    parser_unescape_quoted_string (format);

    DdbListviewGroupFormat *group_formats = NULL;
    DdbListviewGroupFormat *fmt = NULL;
    char *saveptr = NULL;
    char *token;
    while ((token = strtok_stringdelim_r(format, SUBGROUP_DELIMITER, &saveptr)) != NULL) {
        if (strlen(token) > 0) {
            DdbListviewGroupFormat *new_fmt = calloc(1, sizeof(DdbListviewGroupFormat));
            if (!fmt) {
                group_formats = new_fmt;
                fmt = group_formats;
            }
            else {
                fmt->next = new_fmt;
                fmt = fmt->next;
            }
            fmt->format = strdup (token);
            fmt->bytecode = deadbeef->tf_compile (fmt->format);
        }
    }

    free (format);

    // always have at least one
    if (!group_formats) {
        group_formats = calloc(1, sizeof(DdbListviewGroupFormat));
        fmt = group_formats;
        fmt->format = strdup ("");
        fmt->bytecode = deadbeef->tf_compile (fmt->format);
    }

    listview->delegate->groups_changed(_format);

    ddb_listview_set_group_formats(listview, group_formats);
    ddb_listview_refresh (listview, DDB_LIST_CHANGED | DDB_REFRESH_LIST);
}
