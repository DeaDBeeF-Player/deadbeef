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

#ifndef __G_FILE_ATTRIBUTE_H__
#define __G_FILE_ATTRIBUTE_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GFileAttributeType:
 * @G_FILE_ATTRIBUTE_TYPE_INVALID: indicates an invalid or uninitalized type.
 * @G_FILE_ATTRIBUTE_TYPE_STRING: a null terminated UTF8 string.
 * @G_FILE_ATTRIBUTE_TYPE_BYTE_STRING: a zero terminated string of non-zero bytes.
 * @G_FILE_ATTRIBUTE_TYPE_BOOLEAN: a boolean value.
 * @G_FILE_ATTRIBUTE_TYPE_UINT32: an unsigned 4-byte/32-bit integer.
 * @G_FILE_ATTRIBUTE_TYPE_INT32: a signed 4-byte/32-bit integer.
 * @G_FILE_ATTRIBUTE_TYPE_UINT64: an unsigned 8-byte/64-bit integer.
 * @G_FILE_ATTRIBUTE_TYPE_INT64: a signed 8-byte/64-bit integer.
 * @G_FILE_ATTRIBUTE_TYPE_OBJECT: a #GObject.
 * 
 * The data types for file attributes.
 **/ 
typedef enum {
  G_FILE_ATTRIBUTE_TYPE_INVALID = 0,
  G_FILE_ATTRIBUTE_TYPE_STRING,
  G_FILE_ATTRIBUTE_TYPE_BYTE_STRING, /* zero terminated string of non-zero bytes */
  G_FILE_ATTRIBUTE_TYPE_BOOLEAN,
  G_FILE_ATTRIBUTE_TYPE_UINT32,
  G_FILE_ATTRIBUTE_TYPE_INT32,
  G_FILE_ATTRIBUTE_TYPE_UINT64,
  G_FILE_ATTRIBUTE_TYPE_INT64,
  G_FILE_ATTRIBUTE_TYPE_OBJECT
} GFileAttributeType;

/**
 * GFileAttributeInfoFlags:
 * @G_FILE_ATTRIBUTE_INFO_NONE: no flags set.
 * @G_FILE_ATTRIBUTE_INFO_COPY_WITH_FILE: copy the attribute values when the file is copied.
 * @G_FILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED: copy the attribute values when the file is moved.
 * 
 * Flags specifying the behaviour of an attribute.
 **/
typedef enum {
  G_FILE_ATTRIBUTE_INFO_NONE = 0,
  G_FILE_ATTRIBUTE_INFO_COPY_WITH_FILE = 1 << 0,
  G_FILE_ATTRIBUTE_INFO_COPY_WHEN_MOVED = 1 << 1
} GFileAttributeInfoFlags;

/**
 * GFileAttributeStatus:
 * @G_FILE_ATTRIBUTE_STATUS_UNSET: Attribute value is unset (empty).
 * @G_FILE_ATTRIBUTE_STATUS_SET: Attribute value is set.
 * @G_FILE_ATTRIBUTE_STATUS_ERROR_SETTING: Indicates an error in setting the value.
 * 
 * Used by g_file_set_attributes_from_info() when setting file attributes.
 **/
typedef enum {
  G_FILE_ATTRIBUTE_STATUS_UNSET = 0,
  G_FILE_ATTRIBUTE_STATUS_SET,
  G_FILE_ATTRIBUTE_STATUS_ERROR_SETTING
} GFileAttributeStatus;

/**
 * GFileAttributeInfo:
 * @name: the name of the attribute.
 * @type: the #GFileAttributeType type of the attribute.
 * @flags: a set of #GFileAttributeInfoFlags.
 * 
 * Information about a specific attribute. 
 **/
typedef struct {
  char *name;
  GFileAttributeType type;
  GFileAttributeInfoFlags flags;
} GFileAttributeInfo;

/**
 * GFileAttributeInfoList:
 * @infos: an array of #GFileAttributeInfo<!-- -->s.
 * @n_infos: the number of values in the array.
 * 
 * Acts as a lightweight registry for possible valid file attributes.
 * The registry stores Key-Value pair formats as #GFileAttributeInfo<!-- -->s.
 **/
typedef struct {
  GFileAttributeInfo *infos;
  int n_infos;
} GFileAttributeInfoList;

GFileAttributeInfoList *  g_file_attribute_info_list_new    (void);
GFileAttributeInfoList *  g_file_attribute_info_list_ref    (GFileAttributeInfoList *list);
void                      g_file_attribute_info_list_unref  (GFileAttributeInfoList *list);
GFileAttributeInfoList *  g_file_attribute_info_list_dup    (GFileAttributeInfoList *list);
const GFileAttributeInfo *g_file_attribute_info_list_lookup (GFileAttributeInfoList *list,
							     const char             *name);
void                      g_file_attribute_info_list_add    (GFileAttributeInfoList *list,
							     const char             *name,
							     GFileAttributeType      type,
							     GFileAttributeInfoFlags flags);

G_END_DECLS


#endif /* __G_FILE_INFO_H__ */
