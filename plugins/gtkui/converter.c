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
#include "converter.h"
#include "support.h"
#include "interface.h"
#include "gtkui.h"

static GtkWidget *converter;

void
converter_show (void) {
    if (!converter) {
        converter = create_converterdlg ();
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (converter, "output_folder")), deadbeef->conf_get_str ("converter.output_folder", ""));
        gtk_entry_set_text (GTK_ENTRY (lookup_widget (converter, "encoder_cmd_line")), deadbeef->conf_get_str ("converter.encoder", ""));
    }
    gtk_widget_show (converter);
}

void
on_converter_output_browse_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
    GtkWidget *dlg = gtk_file_chooser_dialog_new (_("Select folder..."), GTK_WINDOW (converter), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_OK, NULL);

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
    char *encoder = strdup (gtk_entry_get_text (GTK_ENTRY (lookup_widget (converter, "encoder_cmd_line"))));
    deadbeef->conf_set_str ("converter.encoder", encoder);
    deadbeef->conf_save ();
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
                        char *fname = strrchr (it->fname, '/');
                        if (!fname) {
                            fname = it->fname;
                        }
                        else {
                            fname++;
                        }
                        char out[1024];
                        snprintf (out, sizeof (out), "%s/%s", outfolder, fname);
                        char enc[1024];
                        snprintf (enc, sizeof (enc), encoder, out);
                        printf ("executing: %s\n", enc);
                        FILE *fp = popen (enc, "w");
                        if (!fp) {
                            fprintf (stderr, "converter: failed to open encoder: %s\n", encoder);
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
    free (encoder);
}

gboolean
on_converterdlg_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
    converter = NULL;
    return FALSE;
}

