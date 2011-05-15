/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef __GTK_STOCK_H__
#define __GTK_STOCK_H__


#include <gdk/gdk.h>
#include <gtk/gtkitemfactory.h> /* for GtkTranslateFunc */

G_BEGIN_DECLS

typedef struct _GtkStockItem GtkStockItem;

struct _GtkStockItem
{
  gchar *stock_id;
  gchar *label;
  GdkModifierType modifier;
  guint keyval;
  gchar *translation_domain;
};

void     gtk_stock_add        (const GtkStockItem  *items,
                               guint                n_items);
void     gtk_stock_add_static (const GtkStockItem  *items,
                               guint                n_items);
gboolean gtk_stock_lookup     (const gchar         *stock_id,
                               GtkStockItem        *item);

/* Should free the list (and free each string in it also).
 * This function is only useful for GUI builders and such.
 */
GSList*  gtk_stock_list_ids  (void);

GtkStockItem *gtk_stock_item_copy (const GtkStockItem *item);
void          gtk_stock_item_free (GtkStockItem       *item);

void          gtk_stock_set_translate_func (const gchar      *domain,
					    GtkTranslateFunc  func,
					    gpointer          data,
					    GtkDestroyNotify  notify);

/* Stock IDs (not all are stock items; some are images only) */
#define GTK_STOCK_DIALOG_AUTHENTICATION \
                                   "gtk-dialog-authentication"
#define GTK_STOCK_DIALOG_INFO      "gtk-dialog-info"
#define GTK_STOCK_DIALOG_WARNING   "gtk-dialog-warning"
#define GTK_STOCK_DIALOG_ERROR     "gtk-dialog-error"
#define GTK_STOCK_DIALOG_QUESTION  "gtk-dialog-question"

#define GTK_STOCK_DND              "gtk-dnd"
#define GTK_STOCK_DND_MULTIPLE     "gtk-dnd-multiple"

#define GTK_STOCK_ABOUT            "gtk-about"
#define GTK_STOCK_ADD              "gtk-add"
#define GTK_STOCK_APPLY            "gtk-apply"
#define GTK_STOCK_BOLD             "gtk-bold"
#define GTK_STOCK_CANCEL           "gtk-cancel"
#define GTK_STOCK_CDROM            "gtk-cdrom"
#define GTK_STOCK_CLEAR            "gtk-clear"
#define GTK_STOCK_CLOSE            "gtk-close"
#define GTK_STOCK_COLOR_PICKER     "gtk-color-picker"
#define GTK_STOCK_CONVERT          "gtk-convert"
#define GTK_STOCK_CONNECT          "gtk-connect"
#define GTK_STOCK_COPY             "gtk-copy"
#define GTK_STOCK_CUT              "gtk-cut"
#define GTK_STOCK_DELETE           "gtk-delete"
#define GTK_STOCK_DIRECTORY        "gtk-directory"
#define GTK_STOCK_DISCARD          "gtk-discard"
#define GTK_STOCK_DISCONNECT       "gtk-disconnect"
#define GTK_STOCK_EDIT             "gtk-edit"
#define GTK_STOCK_EXECUTE          "gtk-execute"
#define GTK_STOCK_FILE             "gtk-file"
#define GTK_STOCK_FIND             "gtk-find"
#define GTK_STOCK_FIND_AND_REPLACE "gtk-find-and-replace"
#define GTK_STOCK_FLOPPY           "gtk-floppy"
#define GTK_STOCK_FULLSCREEN       "gtk-fullscreen"
#define GTK_STOCK_GOTO_BOTTOM      "gtk-goto-bottom"
#define GTK_STOCK_GOTO_FIRST       "gtk-goto-first"
#define GTK_STOCK_GOTO_LAST        "gtk-goto-last"
#define GTK_STOCK_GOTO_TOP         "gtk-goto-top"
#define GTK_STOCK_GO_BACK          "gtk-go-back"
#define GTK_STOCK_GO_DOWN          "gtk-go-down"
#define GTK_STOCK_GO_FORWARD       "gtk-go-forward"
#define GTK_STOCK_GO_UP            "gtk-go-up"
#define GTK_STOCK_HARDDISK         "gtk-harddisk"
#define GTK_STOCK_HELP             "gtk-help"
#define GTK_STOCK_HOME             "gtk-home"
#define GTK_STOCK_INDEX            "gtk-index"
#define GTK_STOCK_INDENT           "gtk-indent"
#define GTK_STOCK_INFO             "gtk-info"
#define GTK_STOCK_UNINDENT         "gtk-unindent"
#define GTK_STOCK_ITALIC           "gtk-italic"
#define GTK_STOCK_JUMP_TO          "gtk-jump-to"
#define GTK_STOCK_JUSTIFY_CENTER   "gtk-justify-center"
#define GTK_STOCK_JUSTIFY_FILL     "gtk-justify-fill"
#define GTK_STOCK_JUSTIFY_LEFT     "gtk-justify-left"
#define GTK_STOCK_JUSTIFY_RIGHT    "gtk-justify-right"
#define GTK_STOCK_LEAVE_FULLSCREEN "gtk-leave-fullscreen"
#define GTK_STOCK_MISSING_IMAGE    "gtk-missing-image"
#define GTK_STOCK_MEDIA_FORWARD    "gtk-media-forward"
#define GTK_STOCK_MEDIA_NEXT       "gtk-media-next"
#define GTK_STOCK_MEDIA_PAUSE      "gtk-media-pause"
#define GTK_STOCK_MEDIA_PLAY       "gtk-media-play"
#define GTK_STOCK_MEDIA_PREVIOUS   "gtk-media-previous"
#define GTK_STOCK_MEDIA_RECORD     "gtk-media-record"
#define GTK_STOCK_MEDIA_REWIND     "gtk-media-rewind"
#define GTK_STOCK_MEDIA_STOP       "gtk-media-stop"
#define GTK_STOCK_NETWORK          "gtk-network"
#define GTK_STOCK_NEW              "gtk-new"
#define GTK_STOCK_NO               "gtk-no"
#define GTK_STOCK_OK               "gtk-ok"
#define GTK_STOCK_OPEN             "gtk-open"
#define GTK_STOCK_ORIENTATION_PORTRAIT "gtk-orientation-portrait"
#define GTK_STOCK_ORIENTATION_LANDSCAPE "gtk-orientation-landscape"
#define GTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE "gtk-orientation-reverse-landscape"
#define GTK_STOCK_ORIENTATION_REVERSE_PORTRAIT "gtk-orientation-reverse-portrait"
#define GTK_STOCK_PASTE            "gtk-paste"
#define GTK_STOCK_PREFERENCES      "gtk-preferences"
#define GTK_STOCK_PRINT            "gtk-print"
#define GTK_STOCK_PRINT_PREVIEW    "gtk-print-preview"
#define GTK_STOCK_PROPERTIES       "gtk-properties"
#define GTK_STOCK_QUIT             "gtk-quit"
#define GTK_STOCK_REDO             "gtk-redo"
#define GTK_STOCK_REFRESH          "gtk-refresh"
#define GTK_STOCK_REMOVE           "gtk-remove"
#define GTK_STOCK_REVERT_TO_SAVED  "gtk-revert-to-saved"
#define GTK_STOCK_SAVE             "gtk-save"
#define GTK_STOCK_SAVE_AS          "gtk-save-as"
#define GTK_STOCK_SELECT_ALL       "gtk-select-all"
#define GTK_STOCK_SELECT_COLOR     "gtk-select-color"
#define GTK_STOCK_SELECT_FONT      "gtk-select-font"
#define GTK_STOCK_SORT_ASCENDING   "gtk-sort-ascending"
#define GTK_STOCK_SORT_DESCENDING  "gtk-sort-descending"
#define GTK_STOCK_SPELL_CHECK      "gtk-spell-check"
#define GTK_STOCK_STOP             "gtk-stop"
#define GTK_STOCK_STRIKETHROUGH    "gtk-strikethrough"
#define GTK_STOCK_UNDELETE         "gtk-undelete"
#define GTK_STOCK_UNDERLINE        "gtk-underline"
#define GTK_STOCK_UNDO             "gtk-undo"
#define GTK_STOCK_YES              "gtk-yes"
#define GTK_STOCK_ZOOM_100         "gtk-zoom-100"
#define GTK_STOCK_ZOOM_FIT         "gtk-zoom-fit"
#define GTK_STOCK_ZOOM_IN          "gtk-zoom-in"
#define GTK_STOCK_ZOOM_OUT         "gtk-zoom-out"

G_END_DECLS

#endif /* __GTK_STOCK_H__ */
