/* eggdesktopfile.h - Freedesktop.Org Desktop Files
 * Copyright (C) 2007 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING.LIB. If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place -
 * Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __EGG_DESKTOP_FILE_H__
#define __EGG_DESKTOP_FILE_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct EggDesktopFile EggDesktopFile;

typedef enum {
	EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED,

	EGG_DESKTOP_FILE_TYPE_APPLICATION,
	EGG_DESKTOP_FILE_TYPE_LINK,
	EGG_DESKTOP_FILE_TYPE_DIRECTORY
} EggDesktopFileType;

EggDesktopFile     *egg_desktop_file_new                (const char   *desktop_file_path,
							 GError      **error);

EggDesktopFile     *egg_desktop_file_new_from_data_dirs (const char   *desktop_file_path,
							 GError      **error);
EggDesktopFile     *egg_desktop_file_new_from_dirs      (const char   *desktop_file_path,
							 const char  **search_dirs,
							 GError      **error);
EggDesktopFile     *egg_desktop_file_new_from_key_file  (GKeyFile     *key_file,
							 const char   *source,
							 GError      **error);

void                egg_desktop_file_free               (EggDesktopFile  *desktop_file);

const char         *egg_desktop_file_get_source         (EggDesktopFile  *desktop_file);

EggDesktopFileType  egg_desktop_file_get_desktop_file_type (EggDesktopFile  *desktop_file);

const char         *egg_desktop_file_get_name           (EggDesktopFile  *desktop_file);
const char         *egg_desktop_file_get_icon           (EggDesktopFile  *desktop_file);

gboolean            egg_desktop_file_can_launch         (EggDesktopFile  *desktop_file,
							 const char      *desktop_environment);

gboolean            egg_desktop_file_accepts_documents  (EggDesktopFile  *desktop_file);
gboolean            egg_desktop_file_accepts_multiple   (EggDesktopFile  *desktop_file);
gboolean            egg_desktop_file_accepts_uris       (EggDesktopFile  *desktop_file);

char               *egg_desktop_file_parse_exec         (EggDesktopFile  *desktop_file,
							 GSList          *documents,
							 GError         **error);

gboolean            egg_desktop_file_launch             (EggDesktopFile  *desktop_file,
							 GSList          *documents,
							 GError         **error,
							 ...) G_GNUC_NULL_TERMINATED;

typedef enum {
	EGG_DESKTOP_FILE_LAUNCH_CLEARENV = 1,
	EGG_DESKTOP_FILE_LAUNCH_PUTENV,
	EGG_DESKTOP_FILE_LAUNCH_SCREEN,
	EGG_DESKTOP_FILE_LAUNCH_WORKSPACE,
	EGG_DESKTOP_FILE_LAUNCH_DIRECTORY,
	EGG_DESKTOP_FILE_LAUNCH_TIME,
	EGG_DESKTOP_FILE_LAUNCH_FLAGS,
	EGG_DESKTOP_FILE_LAUNCH_SETUP_FUNC,
	EGG_DESKTOP_FILE_LAUNCH_RETURN_PID,
	EGG_DESKTOP_FILE_LAUNCH_RETURN_STDIN_PIPE,
	EGG_DESKTOP_FILE_LAUNCH_RETURN_STDOUT_PIPE,
	EGG_DESKTOP_FILE_LAUNCH_RETURN_STDERR_PIPE,
	EGG_DESKTOP_FILE_LAUNCH_RETURN_STARTUP_ID
} EggDesktopFileLaunchOption;

/* Standard Keys */
#define EGG_DESKTOP_FILE_GROUP			"Desktop Entry"

#define EGG_DESKTOP_FILE_KEY_TYPE		"Type"
#define EGG_DESKTOP_FILE_KEY_VERSION		"Version"
#define EGG_DESKTOP_FILE_KEY_NAME		"Name"
#define EGG_DESKTOP_FILE_KEY_GENERIC_NAME	"GenericName"
#define EGG_DESKTOP_FILE_KEY_NO_DISPLAY		"NoDisplay"
#define EGG_DESKTOP_FILE_KEY_COMMENT		"Comment"
#define EGG_DESKTOP_FILE_KEY_ICON		"Icon"
#define EGG_DESKTOP_FILE_KEY_HIDDEN		"Hidden"
#define EGG_DESKTOP_FILE_KEY_ONLY_SHOW_IN	"OnlyShowIn"
#define EGG_DESKTOP_FILE_KEY_NOT_SHOW_IN	"NotShowIn"
#define EGG_DESKTOP_FILE_KEY_TRY_EXEC		"TryExec"
#define EGG_DESKTOP_FILE_KEY_EXEC		"Exec"
#define EGG_DESKTOP_FILE_KEY_PATH		"Path"
#define EGG_DESKTOP_FILE_KEY_TERMINAL		"Terminal"
#define EGG_DESKTOP_FILE_KEY_MIME_TYPE		"MimeType"
#define EGG_DESKTOP_FILE_KEY_CATEGORIES		"Categories"
#define EGG_DESKTOP_FILE_KEY_STARTUP_NOTIFY	"StartupNotify"
#define EGG_DESKTOP_FILE_KEY_STARTUP_WM_CLASS	"StartupWMClass"
#define EGG_DESKTOP_FILE_KEY_URL		"URL"

/* Accessors */
gboolean  egg_desktop_file_has_key                (EggDesktopFile  *desktop_file,
						   const char      *key,
						   GError         **error);
char     *egg_desktop_file_get_string             (EggDesktopFile  *desktop_file,
						   const char      *key,
						   GError         **error) G_GNUC_MALLOC;
char     *egg_desktop_file_get_locale_string      (EggDesktopFile  *desktop_file,
						   const char      *key,
						   const char      *locale,
						   GError         **error) G_GNUC_MALLOC;
gboolean  egg_desktop_file_get_boolean            (EggDesktopFile  *desktop_file,
						   const char      *key,
						   GError         **error);
double    egg_desktop_file_get_numeric            (EggDesktopFile  *desktop_file,
						   const char      *key,
						   GError         **error);
int       egg_desktop_file_get_integer            (EggDesktopFile  *desktop_file,
						   const char      *key,
						   GError         **error);
char    **egg_desktop_file_get_string_list        (EggDesktopFile  *desktop_file,
						   const char      *key,
						   gsize           *length,
						   GError         **error) G_GNUC_MALLOC;
char    **egg_desktop_file_get_locale_string_list (EggDesktopFile  *desktop_file,
						   const char      *key,
						   const char      *locale,
						   gsize           *length,
						   GError         **error) G_GNUC_MALLOC;


/* Errors */
#define EGG_DESKTOP_FILE_ERROR egg_desktop_file_error_quark()

GQuark egg_desktop_file_error_quark (void);

typedef enum {
	EGG_DESKTOP_FILE_ERROR_INVALID,
	EGG_DESKTOP_FILE_ERROR_NOT_LAUNCHABLE,
	EGG_DESKTOP_FILE_ERROR_UNRECOGNIZED_OPTION
} EggDesktopFileError;

/* Global application desktop file */
void            egg_set_desktop_file                  (const char *desktop_file_path);
void            egg_set_desktop_file_without_defaults (const char *desktop_file_path);
EggDesktopFile *egg_get_desktop_file                  (void);


G_END_DECLS

#endif /* __EGG_DESKTOP_FILE_H__ */
