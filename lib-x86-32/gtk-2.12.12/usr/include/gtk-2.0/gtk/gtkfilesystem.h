/* GTK - The GIMP Toolkit
 * gtkfilesystem.h: Abstract file system interfaces
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

#ifndef __GTK_FILE_SYSTEM_H__
#define __GTK_FILE_SYSTEM_H__

/* This is a "semi-private" header; it is meant only for
 * alternate GtkFileChooser backend modules; no stability guarantees 
 * are made at this point
 */
#ifndef GTK_FILE_SYSTEM_ENABLE_UNSUPPORTED
#error "GtkFileSystem is not supported API for general use"
#endif

#include <glib-object.h>
#include <gtk/gtkwidget.h>	/* For icon handling */

G_BEGIN_DECLS

typedef gint64 GtkFileTime;

typedef struct _GtkFileFolder       GtkFileFolder;
typedef struct _GtkFileFolderIface  GtkFileFolderIface;
typedef struct _GtkFileInfo         GtkFileInfo;
typedef struct _GtkFileSystem       GtkFileSystem;
typedef struct _GtkFileSystemIface  GtkFileSystemIface;
typedef struct _GtkFileSystemVolume GtkFileSystemVolume;

typedef struct _GtkFilePath        GtkFilePath;

/* Mask of information about a file, for monitoring and
 * gtk_file_system_get_info()
 */
typedef enum {
  GTK_FILE_INFO_DISPLAY_NAME      = 1 << 0,
  GTK_FILE_INFO_IS_FOLDER         = 1 << 1,
  GTK_FILE_INFO_IS_HIDDEN         = 1 << 2,
  GTK_FILE_INFO_MIME_TYPE         = 1 << 3,
  GTK_FILE_INFO_MODIFICATION_TIME = 1 << 4,
  GTK_FILE_INFO_SIZE              = 1 << 5,
  GTK_FILE_INFO_ICON              = 1 << 6,
  GTK_FILE_INFO_ALL               = (1 << 7) - 1
} GtkFileInfoType;

/* GError enumeration for GtkFileSystem
 */

#define GTK_FILE_SYSTEM_ERROR (gtk_file_system_error_quark ())

typedef enum
{
  GTK_FILE_SYSTEM_ERROR_NONEXISTENT,
  GTK_FILE_SYSTEM_ERROR_NOT_FOLDER,
  GTK_FILE_SYSTEM_ERROR_INVALID_URI,
  GTK_FILE_SYSTEM_ERROR_BAD_FILENAME,
  GTK_FILE_SYSTEM_ERROR_FAILED,
  GTK_FILE_SYSTEM_ERROR_ALREADY_EXISTS
} GtkFileSystemError;

GQuark     gtk_file_system_error_quark      (void);

/* Boxed-type for gtk_file_folder_get_info() results
 */
#define GTK_TYPE_FILE_INFO (gtk_file_info_get_type ())

GType       gtk_file_info_get_type (void) G_GNUC_CONST; 

GtkFileInfo *gtk_file_info_new  (void);
GtkFileInfo *gtk_file_info_copy (GtkFileInfo *info);
void         gtk_file_info_free (GtkFileInfo *info);


G_CONST_RETURN gchar *gtk_file_info_get_display_name      (const GtkFileInfo *info);
G_CONST_RETURN gchar *gtk_file_info_get_display_key       (const GtkFileInfo *info);
void                  gtk_file_info_set_display_name      (GtkFileInfo       *info,
							   const gchar       *display_name);
gboolean              gtk_file_info_get_is_folder         (const GtkFileInfo *info);
void                  gtk_file_info_set_is_folder         (GtkFileInfo       *info,
							   gboolean           is_folder);
gboolean              gtk_file_info_get_is_hidden         (const GtkFileInfo *info);
void                  gtk_file_info_set_is_hidden         (GtkFileInfo       *info,
							   gboolean           is_hidden);
G_CONST_RETURN gchar *gtk_file_info_get_mime_type         (const GtkFileInfo *info);
void                  gtk_file_info_set_mime_type         (GtkFileInfo       *info,
							   const gchar       *mime_type);
GtkFileTime           gtk_file_info_get_modification_time (const GtkFileInfo *info);
void                  gtk_file_info_set_modification_time (GtkFileInfo       *info,
							   GtkFileTime        modification_time);
gint64                gtk_file_info_get_size              (const GtkFileInfo *info);
void                  gtk_file_info_set_size              (GtkFileInfo       *info,
							   gint64             size);

void                  gtk_file_info_set_icon_name         (GtkFileInfo       *info,
							   const gchar       *con_name);
G_CONST_RETURN gchar *gtk_file_info_get_icon_name         (const GtkFileInfo *info);
GdkPixbuf            *gtk_file_info_render_icon           (const GtkFileInfo *info,
							   GtkWidget         *widget,
							   gint               pixel_size,
							   GError           **error);

/* GtkFileSystemHandle
 */

#define GTK_TYPE_FILE_SYSTEM_HANDLE            (gtk_file_system_handle_get_type ())
#define GTK_FILE_SYSTEM_HANDLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_SYSTEM_HANDLE, GtkFileSystemHandle))
#define GTK_IS_FILE_SYSTEM_HANDLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_SYSTEM_HANDLE))
#define GTK_FILE_SYSTEM_HANDLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_FILE_SYSTEM_HANDLE, GtkFileSystemHandleUnixClass))
#define GTK_IS_FILE_SYSTEM_HANDLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FILE_SYSTEM_HANDLE))
#define GTK_FILE_SYSTEM_HANDLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_FILE_SYSTEM_HANDLE, GtkFileSystemHandleClass))

typedef struct _GtkFileSystemHandle       GtkFileSystemHandle;
typedef struct _GtkFileSystemHandleClass  GtkFileSystemHandleClass;

struct _GtkFileSystemHandle
{
  GObject parent_instance;

  GtkFileSystem *file_system;

  guint cancelled : 1;
};

struct _GtkFileSystemHandleClass
{
  GObjectClass parent_class;
};

GType gtk_file_system_handle_get_type (void);

/* The base GtkFileSystem interface
 */
#define GTK_TYPE_FILE_SYSTEM             (gtk_file_system_get_type ())
#define GTK_FILE_SYSTEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_SYSTEM, GtkFileSystem))
#define GTK_IS_FILE_SYSTEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_SYSTEM))
#define GTK_FILE_SYSTEM_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GTK_TYPE_FILE_SYSTEM, GtkFileSystemIface))

/* Callbacks for the asynchronous GtkFileSystem operations
 */

typedef void (* GtkFileSystemGetInfoCallback) (GtkFileSystemHandle *handle,
					       const GtkFileInfo   *file_info,
					       const GError        *error,
					       gpointer             data);
typedef void (* GtkFileSystemGetFolderCallback) (GtkFileSystemHandle *handle,
						 GtkFileFolder       *folder,
						 const GError        *error,
						 gpointer             data);
typedef void (* GtkFileSystemCreateFolderCallback) (GtkFileSystemHandle *handle,
						    const GtkFilePath   *path,
						    const GError        *error,
						    gpointer             data);
typedef void (* GtkFileSystemVolumeMountCallback) (GtkFileSystemHandle *handle,
						   GtkFileSystemVolume *volume,
						   const GError        *error,
						   gpointer             data);

/*
 */

struct _GtkFileSystemIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  GSList *              (*list_volumes)        (GtkFileSystem     *file_system);
  GtkFileSystemVolume * (*get_volume_for_path) (GtkFileSystem     *file_system,
						const GtkFilePath *path);

  GtkFileSystemHandle * (*get_folder)  (GtkFileSystem                  *file_system,
					const GtkFilePath              *path,
					GtkFileInfoType                 types,
					GtkFileSystemGetFolderCallback  callback,
					gpointer                        data);
  GtkFileSystemHandle * (*get_info) (GtkFileSystem                *file_system,
				     const GtkFilePath            *path,
				     GtkFileInfoType               types,
				     GtkFileSystemGetInfoCallback  callback,
				     gpointer                      data);
  GtkFileSystemHandle * (*create_folder)  (GtkFileSystem                     *file_system,
					   const GtkFilePath                 *path,
					   GtkFileSystemCreateFolderCallback  callback,
					   gpointer                           data);

  void               (*cancel_operation) (GtkFileSystemHandle *handle);

  /* Volumes
   */
  void          (*volume_free)             (GtkFileSystem        *file_system,
					    GtkFileSystemVolume  *volume);
  GtkFilePath * (*volume_get_base_path)    (GtkFileSystem        *file_system,
					    GtkFileSystemVolume  *volume);
  gboolean      (*volume_get_is_mounted)   (GtkFileSystem        *file_system,
					    GtkFileSystemVolume  *volume);
  GtkFileSystemHandle * (*volume_mount)    (GtkFileSystem                    *file_system,
					    GtkFileSystemVolume              *volume,
					    GtkFileSystemVolumeMountCallback  callback,
					    gpointer                          data);
  char *        (*volume_get_display_name) (GtkFileSystem        *file_system,
					    GtkFileSystemVolume  *volume);
  gchar *       (*volume_get_icon_name)    (GtkFileSystem        *file_system,
					    GtkFileSystemVolume  *volume,
					    GError              **error);

  /* Path Manipulation
   */
  gboolean      (*get_parent)      (GtkFileSystem      *file_system,
				    const GtkFilePath  *path,
				    GtkFilePath       **parent,
				    GError            **error);
  GtkFilePath * (*make_path)        (GtkFileSystem     *file_system,
				     const GtkFilePath *base_path,
				     const gchar       *display_name,
				     GError           **error);
  gboolean      (*parse)            (GtkFileSystem     *file_system,
				     const GtkFilePath *base_path,
				     const gchar       *str,
				     GtkFilePath      **folder,
				     gchar            **file_part,
				     GError           **error);
  gchar *      (*path_to_uri)      (GtkFileSystem      *file_system,
				    const GtkFilePath  *path);
  gchar *      (*path_to_filename) (GtkFileSystem      *file_system,
				    const GtkFilePath  *path);
  GtkFilePath *(*uri_to_path)      (GtkFileSystem      *file_system,
				    const gchar        *uri);
  GtkFilePath *(*filename_to_path) (GtkFileSystem      *file_system,
				    const gchar        *path);

  /* Bookmarks 
   */
  gboolean       (*insert_bookmark)        (GtkFileSystem     *file_system,
					    const GtkFilePath *path,
					    gint               position,
					    GError           **error);
  gboolean       (*remove_bookmark)        (GtkFileSystem     *file_system,
					    const GtkFilePath *path,
					    GError           **error);
  GSList *       (*list_bookmarks)         (GtkFileSystem     *file_system);

  /* Signals 
   */
  void (*volumes_changed)   (GtkFileSystem *file_system);
  void (*bookmarks_changed) (GtkFileSystem *file_system);

  /* Bookmark labels 
   */
  gchar *        (*get_bookmark_label)     (GtkFileSystem     *file_system,
					    const GtkFilePath *path);
  void           (*set_bookmark_label)     (GtkFileSystem     *file_system,
					    const GtkFilePath *path,
					    const gchar       *label);
};

GType             gtk_file_system_get_type       (void) G_GNUC_CONST;

GSList *          gtk_file_system_list_volumes   (GtkFileSystem     *file_system);

GtkFileSystemVolume *gtk_file_system_get_volume_for_path (GtkFileSystem     *file_system,
							  const GtkFilePath *path);

void              gtk_file_system_volume_free             (GtkFileSystem        *file_system,
							   GtkFileSystemVolume  *volume);
GtkFilePath *     gtk_file_system_volume_get_base_path    (GtkFileSystem        *file_system,
							   GtkFileSystemVolume  *volume);
gboolean          gtk_file_system_volume_get_is_mounted   (GtkFileSystem        *file_system,
							   GtkFileSystemVolume  *volume);
GtkFileSystemHandle *gtk_file_system_volume_mount         (GtkFileSystem                    *file_system,
							   GtkFileSystemVolume              *volume,
							   GtkFileSystemVolumeMountCallback  callback,
							   gpointer                          data);
char *            gtk_file_system_volume_get_display_name (GtkFileSystem        *file_system, 
							   GtkFileSystemVolume  *volume);
GdkPixbuf *       gtk_file_system_volume_render_icon      (GtkFileSystem        *file_system,
							   GtkFileSystemVolume  *volume,
							   GtkWidget            *widget,
							   gint                  pixel_size,
							   GError              **error);
gchar *           gtk_file_system_volume_get_icon_name    (GtkFileSystem        *file_system,
							   GtkFileSystemVolume  *volume,
							   GError              **error);

gboolean          gtk_file_system_get_parent     (GtkFileSystem     *file_system,
						  const GtkFilePath *path,
						  GtkFilePath      **parent,
						  GError           **error);
GtkFileSystemHandle *gtk_file_system_get_folder  (GtkFileSystem                  *file_system,
						  const GtkFilePath              *path,
						  GtkFileInfoType                 types,
						  GtkFileSystemGetFolderCallback  callback,
						  gpointer                        data);
GtkFileSystemHandle *gtk_file_system_get_info    (GtkFileSystem                  *file_system,
						  const GtkFilePath              *path,
						  GtkFileInfoType                 types,
						  GtkFileSystemGetInfoCallback    callback,
						  gpointer                        data);
GtkFileSystemHandle *gtk_file_system_create_folder (GtkFileSystem                     *file_system,
						    const GtkFilePath                 *path,
						    GtkFileSystemCreateFolderCallback  callback,
						    gpointer                           data);
void              gtk_file_system_cancel_operation (GtkFileSystemHandle *handle);
GtkFilePath *     gtk_file_system_make_path      (GtkFileSystem     *file_system,
						  const GtkFilePath *base_path,
						  const gchar       *display_name,
						  GError           **error);
gboolean          gtk_file_system_parse          (GtkFileSystem     *file_system,
						  const GtkFilePath *base_path,
						  const gchar       *str,
						  GtkFilePath      **folder,
						  gchar            **file_part,
						  GError           **error);

gchar *      gtk_file_system_path_to_uri      (GtkFileSystem     *file_system,
					       const GtkFilePath *path);
gchar *      gtk_file_system_path_to_filename (GtkFileSystem     *file_system,
					       const GtkFilePath *path);
GtkFilePath *gtk_file_system_uri_to_path      (GtkFileSystem     *file_system,
					       const gchar       *uri);
GtkFilePath *gtk_file_system_filename_to_path (GtkFileSystem     *file_system,
					       const gchar       *filename);

gboolean     gtk_file_system_path_is_local    (GtkFileSystem     *filesystem,
					       const GtkFilePath *path);

gboolean gtk_file_system_insert_bookmark (GtkFileSystem     *file_system,
					  const GtkFilePath *path,
					  gint               position,
					  GError           **error);
gboolean gtk_file_system_remove_bookmark (GtkFileSystem     *file_system,
					  const GtkFilePath *path,
					  GError           **error);
GSList  *gtk_file_system_list_bookmarks  (GtkFileSystem     *file_system);

gchar   *gtk_file_system_get_bookmark_label (GtkFileSystem     *file_system,
					     const GtkFilePath *path);
void     gtk_file_system_set_bookmark_label (GtkFileSystem     *file_system,
					     const GtkFilePath *path,
					     const gchar       *label);

/*
 * Detailed information about a particular folder
 */
#define GTK_TYPE_FILE_FOLDER             (gtk_file_folder_get_type ())
#define GTK_FILE_FOLDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_FILE_FOLDER, GtkFileFolder))
#define GTK_IS_FILE_FOLDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_FILE_FOLDER))
#define GTK_FILE_FOLDER_GET_IFACE(inst)  (G_TYPE_INSTANCE_GET_INTERFACE ((inst), GTK_TYPE_FILE_FOLDER, GtkFileFolderIface))

struct _GtkFileFolderIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  GtkFileInfo *      (*get_info)       (GtkFileFolder     *folder,
					const GtkFilePath *path,
				        GError           **error);
  gboolean           (*list_children)  (GtkFileFolder     *folder,
				        GSList           **children,
				        GError           **error);

  /* ??? refresh() ??? */

  /* Signals
   */
  void (*deleted)       (GtkFileFolder *monitor);
  void (*files_added)   (GtkFileFolder *monitor,
			 GSList        *paths);
  void (*files_changed) (GtkFileFolder *monitor,
			 GSList        *paths);
  void (*files_removed) (GtkFileFolder *monitor,
			 GSList        *paths);

  /* Method / signal */
  gboolean (*is_finished_loading) (GtkFileFolder *folder);
  void     (*finished_loading)    (GtkFileFolder *folder);
};

GType        gtk_file_folder_get_type      (void) G_GNUC_CONST;
gboolean     gtk_file_folder_list_children (GtkFileFolder      *folder,
					    GSList            **children,
					    GError            **error);
GtkFileInfo *gtk_file_folder_get_info      (GtkFileFolder      *folder,
					    const GtkFilePath  *path,
					    GError            **error);

gboolean     gtk_file_folder_is_finished_loading (GtkFileFolder *folder);


/* GtkFilePath */
#define GTK_TYPE_FILE_PATH             (gtk_file_path_get_type ())

GType   gtk_file_path_get_type (void) G_GNUC_CONST;
#ifdef __GNUC__
#define gtk_file_path_new_dup(str) \
 ({ const gchar *__s = (str); (GtkFilePath *)g_strdup(__s); })
#define gtk_file_path_new_steal(str) \
 ({ gchar *__s = (str); (GtkFilePath *)__s; })
#define gtk_file_path_get_string(path) \
 ({ const GtkFilePath *__p = (path); (const gchar *)__p; })
#define gtk_file_path_free(path) \
 ({ GtkFilePath *__p = (path); g_free (__p); })
#else /* __GNUC__ */
#define gtk_file_path_new_dup(str)     ((GtkFilePath *)g_strdup(str))
#define gtk_file_path_new_steal(str)   ((GtkFilePath *)(str))
#define gtk_file_path_get_string(str) ((const gchar *)(str))
#define gtk_file_path_free(path)       g_free (path)
#endif/* __GNUC__ */

#define gtk_file_path_copy(path)       gtk_file_path_new_dup (gtk_file_path_get_string(path))
#ifdef G_OS_WIN32
int _gtk_file_system_win32_path_compare (const gchar *path1,
					 const gchar *path2);
#define gtk_file_path_compare(path1,path2) \
  _gtk_file_system_win32_path_compare (gtk_file_path_get_string (path1), \
	                               gtk_file_path_get_string (path2))
#else
#define gtk_file_path_compare(path1,path2) strcmp (gtk_file_path_get_string (path1), \
						   gtk_file_path_get_string (path2))
#endif

GSList *gtk_file_paths_sort (GSList *paths);
GSList *gtk_file_paths_copy (GSList *paths);
void    gtk_file_paths_free (GSList *paths);

/* GtkFileSystem modules support */

GtkFileSystem  *gtk_file_system_create (const char *file_system_name);

G_END_DECLS

#endif /* __GTK_FILE_SYSTEM_H__ */
