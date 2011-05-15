/* GTK - The GIMP Toolkit
 * gtkfilesystemmodel.h: GtkTreeModel wrapping a GtkFileSystem
 * Copyright (C) 2003, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GTK_FILE_SYSTEM_MODEL_H__
#define __GTK_FILE_SYSTEM_MODEL_H__

#ifndef GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#error "GtkFileSystemModel is not supported API for general use"
#endif

#include <glib-object.h>
#include "gtkfilesystem.h"
#include <gtk/gtktreemodel.h>

G_BEGIN_DECLS

#define GTK_TYPE_FILE_SYSTEM_MODEL             (_gtk_file_system_model_get_type ())
#define GTK_FILE_SYSTEM_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_SYSTEM_MODEL, GtkFileSystemModel))
#define GTK_IS_FILE_SYSTEM_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_SYSTEM_MODEL))

typedef struct _GtkFileSystemModel      GtkFileSystemModel;

GType _gtk_file_system_model_get_type (void) G_GNUC_CONST;

typedef enum {
  GTK_FILE_SYSTEM_MODEL_INFO,
  GTK_FILE_SYSTEM_MODEL_DISPLAY_NAME,
  GTK_FILE_SYSTEM_MODEL_N_COLUMNS
} GtkFileSystemModelColumns;

GtkFileSystemModel *_gtk_file_system_model_new              (GtkFileSystem      *file_system,
							     const GtkFilePath  *root_path,
							     gint                max_depth,
							     GtkFileInfoType     types,
							     GError            **error);
const GtkFileInfo * _gtk_file_system_model_get_info         (GtkFileSystemModel *model,
							     GtkTreeIter        *iter);
const GtkFilePath * _gtk_file_system_model_get_path          (GtkFileSystemModel *model,
							     GtkTreeIter        *iter);
void                _gtk_file_system_model_set_show_hidden  (GtkFileSystemModel *model,
							     gboolean            show_hidden);
void                _gtk_file_system_model_set_show_folders (GtkFileSystemModel *model,
							     gboolean            show_folders);
void                _gtk_file_system_model_set_show_files   (GtkFileSystemModel *model,
							     gboolean            show_files);

typedef gboolean (*GtkFileSystemModelFilter) (GtkFileSystemModel *model,
					      GtkFilePath        *path,
					      const GtkFileInfo  *info,
					      gpointer            user_data);

void     _gtk_file_system_model_set_filter (GtkFileSystemModel      *model,
					    GtkFileSystemModelFilter filter,
					    gpointer                 user_data);

typedef void (*GtkFileSystemModelPathFunc) (GtkFileSystemModel *model,
					    GtkTreePath        *path,
					    GtkTreeIter        *iter,
					    gpointer            user_data);

void     _gtk_file_system_model_path_do (GtkFileSystemModel        *model,
					 const GtkFilePath         *path,
					 GtkFileSystemModelPathFunc func,
					 gpointer                   user_data);

void _gtk_file_system_model_add_editable    (GtkFileSystemModel *model,
					     GtkTreeIter        *iter);
void _gtk_file_system_model_remove_editable (GtkFileSystemModel *model);

G_END_DECLS

#endif /* __GTK_FILE_SYSTEM_MODEL_H__ */
