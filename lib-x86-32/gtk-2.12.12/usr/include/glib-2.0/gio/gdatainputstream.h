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

#ifndef __G_DATA_INPUT_STREAM_H__
#define __G_DATA_INPUT_STREAM_H__

#include <glib-object.h>
#include <gio/gbufferedinputstream.h>

G_BEGIN_DECLS

#define G_TYPE_DATA_INPUT_STREAM         (g_data_input_stream_get_type ())
#define G_DATA_INPUT_STREAM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_DATA_INPUT_STREAM, GDataInputStream))
#define G_DATA_INPUT_STREAM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_DATA_INPUT_STREAM, GDataInputStreamClass))
#define G_IS_DATA_INPUT_STREAM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_DATA_INPUT_STREAM))
#define G_IS_DATA_INPUT_STREAM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_DATA_INPUT_STREAM))
#define G_DATA_INPUT_STREAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_DATA_INPUT_STREAM, GDataInputStreamClass))

/**
 * GDataInputStream:
 * @parent: a #GBufferedInputStream.
 *
 * An implementation of #GBufferedInputStream that allows for high-level 
 * data manipulation of arbitrary data (including binary operations).
 **/
typedef struct _GDataInputStream         GDataInputStream;
typedef struct _GDataInputStreamClass    GDataInputStreamClass;
typedef struct _GDataInputStreamPrivate  GDataInputStreamPrivate;

struct _GDataInputStream
{
  GBufferedInputStream parent_instance;

  /*< private >*/
  GDataInputStreamPrivate *priv;
};

struct _GDataInputStreamClass
{
 GBufferedInputStreamClass parent_class;
  /*< private >*/
  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);
};

/**
 * GDataStreamByteOrder:
 * @G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN: Selects Big Endian byte order. 
 * @G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN: Selects Little Endian byte order.
 * @G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN: Selects endianness based on host machine's architecture.
 *
 * #GDataStreamByteOrder is used to ensure proper endianness of streaming data sources
 * across various machine architectures.
 *
 **/
typedef enum  {
  G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN,
  G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN,
  G_DATA_STREAM_BYTE_ORDER_HOST_ENDIAN
} GDataStreamByteOrder;

/** 
 * GDataStreamNewlineType:
 * @G_DATA_STREAM_NEWLINE_TYPE_LF: Selects "LF" line endings, common on most modern UNIX platforms.
 * @G_DATA_STREAM_NEWLINE_TYPE_CR: Selects "CR" line endings.
 * @G_DATA_STREAM_NEWLINE_TYPE_CR_LF: Selects "CR, LF" line ending, common on Microsoft Windows.
 * @G_DATA_STREAM_NEWLINE_TYPE_ANY: Automatically try to handle any line ending type.
 *
 * #GDataStreamNewlineType is used when checking for or setting the line endings for a given file.
 **/
typedef enum  {
  G_DATA_STREAM_NEWLINE_TYPE_LF,
  G_DATA_STREAM_NEWLINE_TYPE_CR,
  G_DATA_STREAM_NEWLINE_TYPE_CR_LF,
  G_DATA_STREAM_NEWLINE_TYPE_ANY
} GDataStreamNewlineType;

GType          g_data_input_stream_get_type   (void) G_GNUC_CONST;
GDataInputStream*  g_data_input_stream_new        (GInputStream *base_stream);

void                   g_data_input_stream_set_byte_order   (GDataInputStream        *stream,
							     GDataStreamByteOrder     order);
GDataStreamByteOrder   g_data_input_stream_get_byte_order   (GDataInputStream        *stream);
void                   g_data_input_stream_set_newline_type (GDataInputStream        *stream,
							     GDataStreamNewlineType   type);
GDataStreamNewlineType g_data_input_stream_get_newline_type (GDataInputStream        *stream);
guchar                 g_data_input_stream_read_byte        (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
gint16                 g_data_input_stream_read_int16       (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
guint16                g_data_input_stream_read_uint16      (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
gint32                 g_data_input_stream_read_int32       (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
guint32                g_data_input_stream_read_uint32      (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
gint64                 g_data_input_stream_read_int64       (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
guint64                g_data_input_stream_read_uint64      (GDataInputStream        *stream,
							     GCancellable            *cancellable,
							     GError                 **error);
char *                 g_data_input_stream_read_line        (GDataInputStream        *stream,
							     gsize                   *length,
							     GCancellable            *cancellable,
							     GError                 **error);
char *                 g_data_input_stream_read_until       (GDataInputStream        *stream,
							     const gchar             *stop_chars,
							     gsize                   *length,
							     GCancellable            *cancellable,
							     GError                 **error);

G_END_DECLS

#endif /* __G_DATA_INPUT_STREAM_H__ */
