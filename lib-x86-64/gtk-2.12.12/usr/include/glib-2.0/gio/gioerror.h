/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_IO_ERROR_H__
#define __G_IO_ERROR_H__

#include <glib/gerror.h>

G_BEGIN_DECLS

GQuark          g_io_error_quark      (void);

/**
 * G_IO_ERROR:
 * 
 * Error domain for GIO. Errors in this domain will be from the #GIOErrorEnum enumeration.
 * See #GError for more information on error domains.
 **/
#define G_IO_ERROR g_io_error_quark()

/* This enumeration conflicts with GIOError in giochannel.h. However,
 * that is only used as a return value in some deprecated functions.
 * So, we reuse the same prefix for the enumeration values, but call
 * the actual enumeration (which is rarely used) GIOErrorEnum.
 */

/**
 * GIOErrorEnum:
 * @G_IO_ERROR_FAILED: Generic error condition for when any operation fails.
 * @G_IO_ERROR_NOT_FOUND: File not found error.
 * @G_IO_ERROR_EXISTS: File already exists error.
 * @G_IO_ERROR_IS_DIRECTORY: File is a directory error.
 * @G_IO_ERROR_NOT_DIRECTORY: File is not a directory.
 * @G_IO_ERROR_NOT_EMPTY: File is a directory that isn't empty.
 * @G_IO_ERROR_NOT_REGULAR_FILE: File is not a regular file.
 * @G_IO_ERROR_NOT_SYMBOLIC_LINK: File is not a symbolic link.
 * @G_IO_ERROR_NOT_MOUNTABLE_FILE: File cannot be mounted.
 * @G_IO_ERROR_FILENAME_TOO_LONG: Filename is too many characters.
 * @G_IO_ERROR_INVALID_FILENAME: Filename is invalid or contains invalid characters.
 * @G_IO_ERROR_TOO_MANY_LINKS: File contains too many symbolic links.
 * @G_IO_ERROR_NO_SPACE: No space left on drive.
 * @G_IO_ERROR_INVALID_ARGUMENT: Invalid argument.
 * @G_IO_ERROR_PERMISSION_DENIED: Permission denied.
 * @G_IO_ERROR_NOT_SUPPORTED: Operation not supported for the current backend.
 * @G_IO_ERROR_NOT_MOUNTED: File isn't mounted.
 * @G_IO_ERROR_ALREADY_MOUNTED: File is already mounted.
 * @G_IO_ERROR_CLOSED: File was closed.
 * @G_IO_ERROR_CANCELLED: Operation was cancelled. See #GCancellable.
 * @G_IO_ERROR_PENDING: Operations are still pending.
 * @G_IO_ERROR_READ_ONLY: File is read only.
 * @G_IO_ERROR_CANT_CREATE_BACKUP: Backup couldn't be created.
 * @G_IO_ERROR_WRONG_ETAG: File's Entity Tag was incorrect.
 * @G_IO_ERROR_TIMED_OUT: Operation timed out.
 * @G_IO_ERROR_WOULD_RECURSE: Operation would be recursive.
 * @G_IO_ERROR_BUSY: File is busy.
 * @G_IO_ERROR_WOULD_BLOCK: Operation would block.
 * @G_IO_ERROR_HOST_NOT_FOUND: Host couldn't be found (remote operations).
 * @G_IO_ERROR_WOULD_MERGE: Operation would merge files.
 * @G_IO_ERROR_FAILED_HANDLED: Operation failed and a helper program has already interacted with the user. Do not display any error dialog.
 *
 * Error codes returned by GIO functions.
 * 
 **/
typedef enum
{
  G_IO_ERROR_FAILED,
  G_IO_ERROR_NOT_FOUND,
  G_IO_ERROR_EXISTS,
  G_IO_ERROR_IS_DIRECTORY,
  G_IO_ERROR_NOT_DIRECTORY,
  G_IO_ERROR_NOT_EMPTY,
  G_IO_ERROR_NOT_REGULAR_FILE,
  G_IO_ERROR_NOT_SYMBOLIC_LINK,
  G_IO_ERROR_NOT_MOUNTABLE_FILE,
  G_IO_ERROR_FILENAME_TOO_LONG,
  G_IO_ERROR_INVALID_FILENAME,
  G_IO_ERROR_TOO_MANY_LINKS,
  G_IO_ERROR_NO_SPACE,
  G_IO_ERROR_INVALID_ARGUMENT,
  G_IO_ERROR_PERMISSION_DENIED,
  G_IO_ERROR_NOT_SUPPORTED,
  G_IO_ERROR_NOT_MOUNTED,
  G_IO_ERROR_ALREADY_MOUNTED,
  G_IO_ERROR_CLOSED,
  G_IO_ERROR_CANCELLED,
  G_IO_ERROR_PENDING,
  G_IO_ERROR_READ_ONLY,
  G_IO_ERROR_CANT_CREATE_BACKUP,
  G_IO_ERROR_WRONG_ETAG,
  G_IO_ERROR_TIMED_OUT,
  G_IO_ERROR_WOULD_RECURSE,
  G_IO_ERROR_BUSY,
  G_IO_ERROR_WOULD_BLOCK,
  G_IO_ERROR_HOST_NOT_FOUND,
  G_IO_ERROR_WOULD_MERGE,
  G_IO_ERROR_FAILED_HANDLED
} GIOErrorEnum;

GIOErrorEnum g_io_error_from_errno (gint err_no);

G_END_DECLS

#endif /* __G_IO_ERROR_H__ */
