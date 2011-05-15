/* GTK - The GIMP Toolkit
 * gtkfilechooserprivate.h: Interface definition for file selector GUIs
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

#ifndef __GTK_FILE_CHOOSER_PRIVATE_H__
#define __GTK_FILE_CHOOSER_PRIVATE_H__

/* This is a "semi-private" header; it is meant only for
 * alternate GtkFileChooser implementations; no stability guarantees 
 * are made at this point
 */
#ifndef GTK_FILE_CHOOSER_ENABLE_UNSUPPORTED
#error "gtkfilechooserprivate.h is not supported API for general use"
#endif

#include "gtkfilechooser.h"
#include "gtkfilesystem.h"
#include "gtkfilesystemmodel.h"
#include "gtkliststore.h"
#include "gtkrecentmanager.h"
#include "gtksearchengine.h"
#include "gtkquery.h"
#include "gtktooltips.h"
#include "gtktreemodelsort.h"
#include "gtktreestore.h"
#include "gtktreeview.h"
#include "gtkvbox.h"

G_BEGIN_DECLS

#define GTK_FILE_CHOOSER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GTK_TYPE_FILE_CHOOSER, GtkFileChooserIface))

typedef struct _GtkFileChooserIface GtkFileChooserIface;

struct _GtkFileChooserIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  gboolean       (*set_current_folder) 	   (GtkFileChooser    *chooser,
		 		       	    const GtkFilePath *path,
					    GError           **error);
  GtkFilePath *  (*get_current_folder) 	   (GtkFileChooser    *chooser);
  void           (*set_current_name)   	   (GtkFileChooser    *chooser,
					    const gchar       *name);
  gboolean       (*select_path)        	   (GtkFileChooser    *chooser,
		 		       	    const GtkFilePath *path,
					    GError           **error);
  void           (*unselect_path)      	   (GtkFileChooser    *chooser,
		 		       	    const GtkFilePath *path);
  void           (*select_all)         	   (GtkFileChooser    *chooser);
  void           (*unselect_all)       	   (GtkFileChooser    *chooser);
  GSList *       (*get_paths)          	   (GtkFileChooser    *chooser);
  GtkFilePath *  (*get_preview_path)   	   (GtkFileChooser    *chooser);
  GtkFileSystem *(*get_file_system)    	   (GtkFileChooser    *chooser);
  void           (*add_filter)         	   (GtkFileChooser    *chooser,
					    GtkFileFilter     *filter);
  void           (*remove_filter)      	   (GtkFileChooser    *chooser,
					    GtkFileFilter     *filter);
  GSList *       (*list_filters)       	   (GtkFileChooser    *chooser);
  gboolean       (*add_shortcut_folder)    (GtkFileChooser    *chooser,
					    const GtkFilePath *path,
					    GError           **error);
  gboolean       (*remove_shortcut_folder) (GtkFileChooser    *chooser,
					    const GtkFilePath *path,
					    GError           **error);
  GSList *       (*list_shortcut_folders)  (GtkFileChooser    *chooser);
  
  /* Signals
   */
  void (*current_folder_changed) (GtkFileChooser *chooser);
  void (*selection_changed)      (GtkFileChooser *chooser);
  void (*update_preview)         (GtkFileChooser *chooser);
  void (*file_activated)         (GtkFileChooser *chooser);
  GtkFileChooserConfirmation (*confirm_overwrite) (GtkFileChooser *chooser);
};

GtkFileSystem *_gtk_file_chooser_get_file_system         (GtkFileChooser    *chooser);
gboolean       _gtk_file_chooser_set_current_folder_path (GtkFileChooser    *chooser,
							  const GtkFilePath *path,
							  GError           **error);
GtkFilePath *  _gtk_file_chooser_get_current_folder_path (GtkFileChooser    *chooser);
gboolean       _gtk_file_chooser_select_path             (GtkFileChooser    *chooser,
							  const GtkFilePath *path,
							  GError           **error);
void           _gtk_file_chooser_unselect_path           (GtkFileChooser    *chooser,
							  const GtkFilePath *path);
GSList *       _gtk_file_chooser_get_paths               (GtkFileChooser    *chooser);
GtkFilePath *  _gtk_file_chooser_get_preview_path        (GtkFileChooser    *chooser);
gboolean       _gtk_file_chooser_add_shortcut_folder     (GtkFileChooser    *chooser,
							  const GtkFilePath *path,
							  GError           **error);
gboolean       _gtk_file_chooser_remove_shortcut_folder  (GtkFileChooser    *chooser,
							  const GtkFilePath *path,
							  GError           **error);

/* GtkFileChooserDialog private */

struct _GtkFileChooserDialogPrivate
{
  GtkWidget *widget;
  
  char *file_system;

  /* for use with GtkFileChooserEmbed */
  gboolean response_requested;
};


/* GtkFileChooserWidget private */

struct _GtkFileChooserWidgetPrivate
{
  GtkWidget *impl;

  char *file_system;
};


/* GtkFileChooserDefault private */

typedef enum {
  LOAD_EMPTY,			/* There is no model */
  LOAD_PRELOAD,			/* Model is loading and a timer is running; model isn't inserted into the tree yet */
  LOAD_LOADING,			/* Timeout expired, model is inserted into the tree, but not fully loaded yet */
  LOAD_FINISHED			/* Model is fully loaded and inserted into the tree */
} LoadState;

typedef enum {
  RELOAD_EMPTY,			/* No folder has been set */
  RELOAD_HAS_FOLDER,		/* We have a folder, although it may not be completely loaded yet; no need to reload */
  RELOAD_WAS_UNMAPPED		/* We had a folder but got unmapped; reload is needed */
} ReloadState;

typedef enum {
  LOCATION_MODE_PATH_BAR,
  LOCATION_MODE_FILENAME_ENTRY
} LocationMode;

typedef enum {
  OPERATION_MODE_BROWSE,
  OPERATION_MODE_SEARCH,
  OPERATION_MODE_RECENT
} OperationMode;

struct _GtkFileChooserDefault
{
  GtkVBox parent_instance;

  GtkFileChooserAction action;

  GtkFileSystem *file_system;

  /* Save mode widgets */
  GtkWidget *save_widgets;

  GtkWidget *save_folder_label;
  GtkWidget *save_folder_combo;
  GtkWidget *save_expander;

  /* The file browsing widgets */
  GtkWidget *browse_widgets;
  GtkWidget *browse_shortcuts_tree_view;
  GtkWidget *browse_shortcuts_add_button;
  GtkWidget *browse_shortcuts_remove_button;
  GtkWidget *browse_shortcuts_popup_menu;
  GtkWidget *browse_shortcuts_popup_menu_remove_item;
  GtkWidget *browse_shortcuts_popup_menu_rename_item;
  GtkWidget *browse_files_tree_view;
  GtkWidget *browse_files_popup_menu;
  GtkWidget *browse_files_popup_menu_add_shortcut_item;
  GtkWidget *browse_files_popup_menu_hidden_files_item;
  GtkWidget *browse_new_folder_button;
  GtkWidget *browse_path_bar_hbox;
  GtkWidget *browse_path_bar;

  GtkFileSystemModel *browse_files_model;
  char *browse_files_last_selected_name;

  /* OPERATION_MODE_SEARCH */
  GtkWidget *search_hbox;
  GtkWidget *search_entry;
  GtkSearchEngine *search_engine;
  GtkQuery *search_query;
  GtkListStore *search_model;
  GtkTreeModelFilter *search_model_filter;
  GtkTreeModelSort *search_model_sort;

  /* OPERATION_MODE_RECENT */
  GtkRecentManager *recent_manager;
  GtkListStore *recent_model;
  guint load_recent_id;
  GtkTreeModelFilter *recent_model_filter;
  GtkTreeModelSort *recent_model_sort;

  GtkWidget *filter_combo_hbox;
  GtkWidget *filter_combo;
  GtkWidget *preview_box;
  GtkWidget *preview_label;
  GtkWidget *preview_widget;
  GtkWidget *extra_align;
  GtkWidget *extra_widget;

  GtkWidget *location_button;
  GtkWidget *location_entry_box;
  GtkWidget *location_label;
  GtkWidget *location_entry;
  LocationMode location_mode;

  GtkListStore *shortcuts_model;

  /* Filter for the shortcuts pane.  We filter out the "current folder" row and
   * the separator that we use for the "Save in folder" combo.
   */
  GtkTreeModel *shortcuts_pane_filter_model;
  
  /* Filter for the "Save in folder" combo.  We filter out the Search row and
   * its separator.
   */
  GtkTreeModel *shortcuts_combo_filter_model;

  GtkTreeModelSort *sort_model;

  /* Handles */
  GSList *loading_shortcuts;
  GSList *reload_icon_handles;
  GtkFileSystemHandle *file_list_drag_data_received_handle;
  GtkFileSystemHandle *update_current_folder_handle;
  GtkFileSystemHandle *show_and_select_paths_handle;
  GtkFileSystemHandle *should_respond_get_info_handle;
  GtkFileSystemHandle *file_exists_get_info_handle;
  GtkFileSystemHandle *update_from_entry_handle;
  GtkFileSystemHandle *shortcuts_activate_iter_handle;
  GSList *pending_handles;

  LoadState load_state;
  ReloadState reload_state;
  guint load_timeout_id;

  OperationMode operation_mode;

  GSList *pending_select_paths;

  GtkFileFilter *current_filter;
  GSList *filters;

  GtkTooltips *tooltips;

  int num_volumes;
  int num_shortcuts;
  int num_bookmarks;

  gulong volumes_changed_id;
  gulong bookmarks_changed_id;

  GtkFilePath *current_volume_path;
  GtkFilePath *current_folder;
  GtkFilePath *preview_path;
  char *preview_display_name;

  GtkTreeViewColumn *list_name_column;
  GtkCellRenderer *list_name_renderer;
  GtkTreeViewColumn *list_mtime_column;

  GSource *edited_idle;
  char *edited_new_text;

  gulong settings_signal_id;
  int icon_size;

  gulong toplevel_set_focus_id;
  GtkWidget *toplevel_last_focus_widget;

#if 0
  GdkDragContext *shortcuts_drag_context;
  GSource *shortcuts_drag_outside_idle;
#endif

  gint default_width;
  gint default_height;

  /* Flags */

  guint local_only : 1;
  guint preview_widget_active : 1;
  guint use_preview_label : 1;
  guint select_multiple : 1;
  guint show_hidden : 1;
  guint do_overwrite_confirmation : 1;
  guint list_sort_ascending : 1;
  guint changing_folder : 1;
  guint shortcuts_current_folder_active : 1;
  guint expand_folders : 1;
  guint has_home : 1;
  guint has_desktop : 1;
  guint has_search : 1;
  guint has_recent : 1;

#if 0
  guint shortcuts_drag_outside : 1;
#endif
};


/* GtkFileSystemModel private */

typedef struct _FileModelNode           FileModelNode;

struct _GtkFileSystemModel
{
  GObject parent_instance;

  GtkFileSystem  *file_system;
  GtkFileInfoType types;
  FileModelNode  *roots;
  GtkFileFolder  *root_folder;
  GtkFilePath    *root_path;

  GtkFileSystemModelFilter filter_func;
  gpointer filter_data;

  GSList *idle_clears;
  GSource *idle_clear_source;

  gushort max_depth;

  GSList *pending_handles;
  
  guint show_hidden : 1;
  guint show_folders : 1;
  guint show_files : 1;
  guint folders_only : 1;
  guint has_editable : 1;
};

struct _FileModelNode
{
  GtkFilePath *path;
  FileModelNode *next;

  GtkFileInfo *info;
  GtkFileFolder *folder;
  
  FileModelNode *children;
  FileModelNode *parent;
  GtkFileSystemModel *model;

  guint ref_count;
  guint n_referenced_children;

  gushort depth;

  guint has_dummy : 1;
  guint is_dummy : 1;
  guint is_visible : 1;
  guint loaded : 1;
  guint idle_clear : 1;
  guint load_pending : 1;
};


G_END_DECLS

#endif /* __GTK_FILE_CHOOSER_PRIVATE_H__ */
