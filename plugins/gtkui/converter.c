/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2010 Alexey Yakovenko <waker@users.sourceforge.net>

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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include "converter.h"
#include "support.h"
#include "interface.h"
#include "gtkui.h"

enum {
    DDB_ENCODER_METHOD_PIPE = 0,
    DDB_ENCODER_METHOD_FILE = 1,
};

enum {
    DDB_ENCODER_FMT_8BIT = 0x1,
    DDB_ENCODER_FMT_16BIT = 0x2,
    DDB_ENCODER_FMT_24BIT = 0x4,
    DDB_ENCODER_FMT_32BIT = 0x8,
    DDB_ENCODER_FMT_32BITFLOAT = 0x10,
};

typedef struct ddb_encoder_preset_s {
    char *title;
    char *fname;
    char *encoder;
    int method; // pipe or file
    uint32_t formats; // combination of supported flags (FMT_*)
    struct ddb_encoder_preset_s *next;
} ddb_encoder_preset_t;

ddb_encoder_preset_t *encoder_presets;

ddb_encoder_preset_t *
ddb_encoder_preset_alloc (void) {
    ddb_encoder_preset_t *p = malloc (sizeof (ddb_encoder_preset_t));
    if (!p) {
        fprintf (stderr, "failed to alloc ddb_encoder_preset_t\n");
        return NULL;
    }
    memset (p, 0, sizeof (ddb_encoder_preset_t));
    return p;
}

void
ddb_encoder_preset_free (ddb_encoder_preset_t *p) {
    if (p) {
        if (p->title) {
            free (p->title);
        }
        if (p->fname) {
            free (p->fname);
        }
        if (p->encoder) {
            free (p->encoder);
        }
        free (p);
    }
}

ddb_encoder_preset_t *
ddb_encoder_preset_load (const char *fname) {
    int err = 1;
    FILE *fp = fopen (fname, "rt");
    if (!fp) {
        return NULL;
    }
    ddb_encoder_preset_t *p = ddb_encoder_preset_alloc ();

    char str[1024];

    if (1 != fscanf (fp, "title %1024[^\n]\n", str)) {
        goto error;
    }
    p->title = strdup (str);

    if (1 != fscanf (fp, "fname %1024[^\n]\n", str)) {
        goto error;
    }
    p->fname = strdup (str);

    if (1 != fscanf (fp, "encoder %1024[^\n]\n", str)) {
        goto error;
    }
    p->encoder = strdup (str);

    if (1 != fscanf (fp, "method %d\n", &p->method)) {
        goto error;
    }

    if (1 != fscanf (fp, "formats %X\n", &p->formats)) {
        goto error;
    }

    err = 0;
error:
    if (err) {
        ddb_encoder_preset_free (p);
        p = NULL;
    }
    if (fp) {
        fclose (fp);
    }
    return p;
}

// @return -1 on path/write error, -2 if file already exists
int
ddb_encoder_preset_save (ddb_encoder_preset_t *p, int overwrite) {
    const char *confdir = deadbeef->get_config_dir ();
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets", confdir) < 0) {
        return -1;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/encoders", confdir) < 0) {
        return -1;
    }
    mkdir (path, 0755);
    if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", confdir, p->title) < 0) {
        return -1;
    }

    if (!overwrite) {
        FILE *fp = fopen (path, "rb");
        if (fp) {
            fclose (fp);
            return -2; 
        }
    }

    FILE *fp = fopen (path, "w+b");
    if (!fp) {
        return -1;
    }

    fprintf (fp, "title %s\n", p->title);
    fprintf (fp, "fname %s\n", p->fname);
    fprintf (fp, "encoder %s\n", p->encoder);
    fprintf (fp, "method %d\n", p->method);
    fprintf (fp, "formats %08X\n", p->formats);

    fclose (fp);
    return 0;
}

static int dirent_alphasort (const struct dirent **a, const struct dirent **b) {
    return strcmp ((*a)->d_name, (*b)->d_name);
}

int
enc_preset_filter (const struct dirent *ent) {
    char *ext = strrchr (ent->d_name, '.');
    if (ext && !strcasecmp (ext, ".txt")) {
        return 1;
    }
    return 0;
}

int
load_encoder_presets (void) {
    ddb_encoder_preset_t *tail = NULL;
    char path[1024];
    if (snprintf (path, sizeof (path), "%s/presets/encoders", deadbeef->get_config_dir ()) < 0) {
        return -1;
    }
    struct dirent **namelist = NULL;
    int n = scandir (path, &namelist, enc_preset_filter, dirent_alphasort);
    int i;
    for (i = 0; i < n; i++) {
        char s[1024];
        if (snprintf (s, sizeof (s), "%s/%s", path, namelist[i]->d_name) > 0){
            ddb_encoder_preset_t *p = ddb_encoder_preset_load (s);
            if (p) {
                if (tail) {
                    tail->next = p;
                    tail = p;
                }
                else {
                    encoder_presets = tail = p;
                }
            }
        }
        free (namelist[i]);
    }
    free (namelist);
    return 0;
}

static GtkWidget *converter;

void
fill_encoder_presets (GtkListStore *mdl) {
    ddb_encoder_preset_t *p = encoder_presets;
    while (p) {
        GtkTreeIter iter;
        gtk_list_store_append (mdl, &iter);
        gtk_list_store_set (mdl, &iter, 0, p->title, -1);
        p = p->next;
    }
}

void
converter_show (void) {
    if (!converter) {
        if (!encoder_presets) {
            load_encoder_presets ();
        }

        converter = create_converterdlg ();
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (converter, "output_folder")), deadbeef->conf_get_str ("converter.output_folder", ""));

        GtkComboBox *combo;
        // fill encoder presets
        combo = GTK_COMBO_BOX (lookup_widget (converter, "encoder"));
        GtkListStore *mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
        fill_encoder_presets (mdl);
        gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.encoder_preset", 0));

        // fill dsp presets
        combo = GTK_COMBO_BOX (lookup_widget (converter, "dsp_preset"));
        gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.dsp_preset", 0));
        
        // fill channel maps
        combo = GTK_COMBO_BOX (lookup_widget (converter, "channelmap"));
        gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.channelmap_preset", 0));

        // select output format
        combo = GTK_COMBO_BOX (lookup_widget (converter, "output_format"));
        gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.output_format", 0));

    }
    gtk_widget_show (converter);
}

void
on_converter_encoder_changed           (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (converter, "encoder"));
    int act = gtk_combo_box_get_active (combo);
    deadbeef->conf_set_int ("converter.encoder_preset", act);
}

void
on_converter_output_browse_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Select folder..."), GTK_WINDOW (converter), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (converter));

    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dlg), FALSE);
    // restore folder
    gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg), deadbeef->conf_get_str ("filechooser.lastdir", ""));
    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    // store folder
    gchar *folder = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (dlg));
    if (folder) {
        deadbeef->conf_set_str ("filechooser.lastdir", folder);
        g_free (folder);
        deadbeef->sendmessage (M_CONFIGCHANGED, 0, 0, 0);
    }
    if (response == GTK_RESPONSE_OK) {
        folder = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dlg));
        gtk_widget_destroy (dlg);
        if (folder) {
            GtkWidget *entry = lookup_widget (converter, "output_folder");
            gtk_entry_set_text (GTK_ENTRY (entry), folder);
            g_free (folder);
        }
    }
    else {
        gtk_widget_destroy (dlg);
    }
}


void
on_converter_cancel_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
    gtk_widget_destroy (converter);
    converter = NULL;
}

DB_decoder_t *
plug_get_decoder_for_id (const char *id) {
    DB_decoder_t **plugins = deadbeef->plug_get_decoder_list ();
    for (int c = 0; plugins[c]; c++) {
        if (!strcmp (id, plugins[c]->plugin.id)) {
            return plugins[c];
        }
    }
    return NULL;
}


void
on_converter_ok_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    char *outfolder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (converter, "output_folder"))));
    deadbeef->conf_set_str ("converter.output_folder", outfolder);
    deadbeef->conf_save ();

    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (converter, "encoder"));
    int enc_preset = gtk_combo_box_get_active (combo);

    ddb_encoder_preset_t *p = encoder_presets;
    while (enc_preset--) {
        p = p->next;
    }
    if (!p) {
        return;
    }

    gtk_widget_destroy (converter);
    converter = NULL;

    deadbeef->pl_lock ();

    // copy list
    int nsel = deadbeef->pl_getselcount ();
    if (0 < nsel) {
        DB_playItem_t **items = malloc (sizeof (DB_playItem_t *) * nsel);
        if (items) {
            int n = 0;
            DB_playItem_t *it = deadbeef->pl_get_first (PL_MAIN);
            while (it) {
                if (deadbeef->pl_is_selected (it)) {
                    assert (n < nsel);
                    deadbeef->pl_item_ref (it);
                    items[n++] = it;
                }
                DB_playItem_t *next = deadbeef->pl_get_next (it, PL_MAIN);
                deadbeef->pl_item_unref (it);
                it = next;
            }
            deadbeef->pl_unlock ();

            // ... convert ...
            for (n = 0; n < nsel; n++) {
                it = items[n];
                DB_decoder_t *dec = NULL;
                dec = plug_get_decoder_for_id (items[n]->decoder_id);
                if (dec) {
                    DB_fileinfo_t *fileinfo = dec->open (0);
                    if (fileinfo && dec->init (fileinfo, DB_PLAYITEM (it)) != 0) {
                        dec->free (fileinfo);
                        fileinfo = NULL;
                    }
                    if (fileinfo) {
                        char fname[1024];
#if 0
                        char *fname = strrchr (it->fname, '/');
                        if (!fname) {
                            fname = it->fname;
                        }
                        else {
                            fname++;
                        }
#endif
                        int idx = deadbeef->pl_get_idx_of (it);
                        deadbeef->pl_format_title (it, idx, fname, sizeof (fname), -1, p->fname);
                        char out[1024];
                        snprintf (out, sizeof (out), "%s/%s", outfolder, fname);
                        char enc[1024];
                        snprintf (enc, sizeof (enc), p->encoder, out);
                        fprintf (stderr, "executing: %s\n", enc);
                        FILE *fp = popen (enc, "w");
                        if (!fp) {
                            fprintf (stderr, "converter: failed to open encoder\n");
                        }
                        else {
                            // write wave header
                            char wavehdr[] = {
                                0x52, 0x49, 0x46, 0x46, 0x24, 0x70, 0x0d, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xac, 0x00, 0x00, 0x10, 0xb1, 0x02, 0x00, 0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61
                            };
                            fwrite (wavehdr, 1, sizeof (wavehdr), fp);
                            uint32_t size = (it->endsample-it->startsample) * fileinfo->fmt.channels * fileinfo->fmt.bps / 8;
                            fwrite (&size, 1, sizeof (size), fp);

                            int bs = 8192;
                            char buffer[bs];
                            for (;;) {
                                int sz = dec->read (fileinfo, buffer, bs);
                                fwrite (buffer, 1, sz, fp);
                                if (sz != bs) {
                                    break;
                                }
                            }

                            pclose (fp);
                        }
                        dec->free (fileinfo);
                    }
                }
            }

            for (n = 0; n < nsel; n++) {
                deadbeef->pl_item_unref (items[n]);
            }
            free (items);
        }
        else {
            fprintf (stderr, "converter: error allocating memory to store %d tracks\n", nsel);
            deadbeef->pl_unlock ();
        }
    }


    free (outfolder);
}

gboolean
on_converterdlg_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    converter = NULL;
    return FALSE;
}

void
init_encoder_preset_from_dlg (GtkWidget *dlg, ddb_encoder_preset_t *p) {
    p->title = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "title"))));
    p->fname = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "fname"))));
    p->encoder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (dlg, "encoder"))));
    int method_idx = gtk_combo_box_get_active (GTK_COMBO_BOX (lookup_widget (dlg, "method")));
    switch (method_idx) {
    case 0:
        p->method = DDB_ENCODER_METHOD_PIPE;
        break;
    case 1:
        p->method = DDB_ENCODER_METHOD_FILE;
        break;
    }

    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_8bit")))) {
        p->formats |= DDB_ENCODER_FMT_8BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_16bit")))) {
        p->formats |= DDB_ENCODER_FMT_16BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_24bit")))) {
        p->formats |= DDB_ENCODER_FMT_24BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bit")))) {
        p->formats |= DDB_ENCODER_FMT_32BIT;
    }
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bitfloat")))) {
        p->formats |= DDB_ENCODER_FMT_32BITFLOAT;
    }
}

void
on_encoder_preset_add                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_convpreset_editor ();
    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "method")), 0);
    gtk_window_set_title (GTK_WINDOW (dlg), _("Add new encoder preset"));
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    for (;;) {
        int r = gtk_dialog_run (GTK_DIALOG (dlg));
        if (r == GTK_RESPONSE_OK) {
            ddb_encoder_preset_t *p = ddb_encoder_preset_alloc ();
            if (p) {
                init_encoder_preset_from_dlg (dlg, p);
                int err = ddb_encoder_preset_save (p, 0);
                if (!err) {
                    p->next = encoder_presets;
                    encoder_presets = p;
                }
                else {
                    GtkWidget *warndlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Failed to save preset"));
                    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (warndlg), err == -1 ? _("Check preset folder permissions, try to pick different title, or free up some disk space") : _("Preset with the same name already exists. Try to pick another title."));
                    gtk_window_set_title (GTK_WINDOW (warndlg), _("Error"));

                    gtk_window_set_transient_for (GTK_WINDOW (warndlg), GTK_WINDOW (dlg));
                    int response = gtk_dialog_run (GTK_DIALOG (warndlg));
                    gtk_widget_destroy (dlg);
                    continue;
                }
            }
        }
        break;
    }
    
    gtk_widget_destroy (dlg);

    // presets list view
    GtkWidget *list = lookup_widget (toplevel, "presets");
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));
    gtk_list_store_clear (mdl);
    fill_encoder_presets (mdl);

    // presets combo box
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (converter, "encoder"));
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    gtk_list_store_clear (mdl);
    fill_encoder_presets (mdl);
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.encoder_preset", 0));
}

void
on_encoder_preset_edit                     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_convpreset_editor ();
    gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);

    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));

    GtkWidget *list = lookup_widget (toplevel, "presets");
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (!path || !col) {
        // nothing selected
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);

    ddb_encoder_preset_t *p = encoder_presets;
    while (idx--) {
        p = p->next;
    }

    if (p) {
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "title")), p->title);
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "fname")), p->fname);
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (dlg, "encoder")), p->encoder);
        gtk_combo_box_set_active (GTK_COMBO_BOX (lookup_widget (dlg, "method")), p->method);
        if (p->formats & DDB_ENCODER_FMT_8BIT) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_8bit")), 1);
        }
        if (p->formats & DDB_ENCODER_FMT_16BIT) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_16bit")), 1);
        }
        if (p->formats & DDB_ENCODER_FMT_24BIT) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_24bit")), 1);
        }
        if (p->formats & DDB_ENCODER_FMT_32BIT) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bit")), 1);
        }
        if (p->formats & DDB_ENCODER_FMT_32BITFLOAT) {
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lookup_widget (dlg, "_32bitfloat")), 1);
        }

    }

    ddb_encoder_preset_t *old = p;
    for (;;) {
        int r = gtk_dialog_run (GTK_DIALOG (dlg));
        if (r == GTK_RESPONSE_OK) {
            ddb_encoder_preset_t *p = ddb_encoder_preset_alloc ();
            if (p) {
                init_encoder_preset_from_dlg (dlg, p);
                int err = ddb_encoder_preset_save (p, 1);
                if (!err) {
                    if (strcmp (p->title, old->title)) {
                        char path[1024];
                        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_config_dir (), old->title) > 0) {
                            unlink (path);
                        }
                    }
                    free (old->title);
                    free (old->fname);
                    free (old->encoder);
                    old->title = p->title;
                    old->fname = p->fname;
                    old->encoder = p->encoder;
                    old->method = p->method;
                    old->formats = p->formats;
                    free (p);
                }
                else {
                    GtkWidget *warndlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, _("Failed to save preset"));
                    gtk_window_set_transient_for (GTK_WINDOW (warndlg), GTK_WINDOW (dlg));
                    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (warndlg), err == -1 ? _("Check preset folder permissions, try to pick different title, or free up some disk space") : _("Preset with the same name already exists. Try to pick another title."));
                    gtk_window_set_title (GTK_WINDOW (warndlg), _("Error"));

                    int response = gtk_dialog_run (GTK_DIALOG (warndlg));
                    gtk_widget_destroy (warndlg);
                    continue;
                }
            }
        }
        break;
    }
    
    gtk_widget_destroy (dlg);

    // presets list view
    GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));
    gtk_list_store_clear (mdl);
    fill_encoder_presets (mdl);

    // presets combo box
    GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (converter, "encoder"));
    mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
    gtk_list_store_clear (mdl);
    fill_encoder_presets (mdl);
    gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.encoder_preset", 0));
}

void
on_encoder_preset_remove                     (GtkButton       *button,
                                        gpointer         user_data)
{

    GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (button));
    GtkWidget *list = lookup_widget (toplevel, "presets");
    GtkTreePath *path;
    GtkTreeViewColumn *col;
    gtk_tree_view_get_cursor (GTK_TREE_VIEW (list), &path, &col);
    if (!path || !col) {
        // nothing selected
        return;
    }
    int *indices = gtk_tree_path_get_indices (path);
    int idx = *indices;
    g_free (indices);

    ddb_encoder_preset_t *p = encoder_presets;
    ddb_encoder_preset_t *prev = NULL;
    while (idx--) {
        prev = p;
        p = p->next;
    }

    if (!p) {
        return;
    }

    GtkWidget *dlg = gtk_message_dialog_new (GTK_WINDOW (mainwin), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO, _("Remove preset"));
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (toplevel));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg), _("This action will delete the selected preset. Are you sure?"));
    gtk_window_set_title (GTK_WINDOW (dlg), _("Warning"));

    int response = gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
    if (response == GTK_RESPONSE_YES) {
        char path[1024];
        if (snprintf (path, sizeof (path), "%s/presets/encoders/%s.txt", deadbeef->get_config_dir (), p->title) > 0) {
            unlink (path);
        }

        ddb_encoder_preset_t *next = p->next;
        if (prev) {
            prev->next = next;
        }
        else {
            encoder_presets = next;
        }
        ddb_encoder_preset_free (p);

        // presets list view
        GtkListStore *mdl = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (list)));
        gtk_list_store_clear (mdl);
        fill_encoder_presets (mdl);

        // presets combo box
        GtkComboBox *combo = GTK_COMBO_BOX (lookup_widget (converter, "encoder"));
        mdl = GTK_LIST_STORE (gtk_combo_box_get_model (combo));
        gtk_list_store_clear (mdl);
        fill_encoder_presets (mdl);
        gtk_combo_box_set_active (combo, deadbeef->conf_get_int ("converter.encoder_preset", 0));
    }
}

void
on_edit_encoder_presets_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = create_preset_list ();
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (converter));
    g_signal_connect ((gpointer)lookup_widget (dlg, "add"), "clicked", G_CALLBACK (on_encoder_preset_add), NULL);
    g_signal_connect ((gpointer)lookup_widget (dlg, "remove"), "clicked", G_CALLBACK (on_encoder_preset_remove), NULL);
    g_signal_connect ((gpointer)lookup_widget (dlg, "edit"), "clicked", G_CALLBACK (on_encoder_preset_edit), NULL);

    GtkWidget *list = lookup_widget (dlg, "presets");
    GtkCellRenderer *title_cell = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (_("Title"), title_cell, "text", 0, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (list), GTK_TREE_VIEW_COLUMN (col));
    GtkListStore *mdl = gtk_list_store_new (1, G_TYPE_STRING);
    gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (mdl));
    fill_encoder_presets (mdl);
    int curr = deadbeef->conf_get_int ("converter.encoder_preset", 0);
    GtkTreePath *path = gtk_tree_path_new_from_indices (curr, -1);
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (list), path, col, FALSE);
    gtk_tree_path_free (path);
    gtk_dialog_run (GTK_DIALOG (dlg));
    gtk_widget_destroy (dlg);
}


void
on_edit_dsp_presets_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
}


void
on_edit_channel_maps_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
}

