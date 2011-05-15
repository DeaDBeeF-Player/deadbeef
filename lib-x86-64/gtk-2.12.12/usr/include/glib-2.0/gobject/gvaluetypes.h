/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * gvaluetypes.h: GLib default values
 */
#if !defined (__GLIB_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#ifndef __G_VALUETYPES_H__
#define __G_VALUETYPES_H__

#include	<gobject/gvalue.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define G_VALUE_HOLDS_CHAR(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_CHAR))
#define G_VALUE_HOLDS_UCHAR(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_UCHAR))
#define G_VALUE_HOLDS_BOOLEAN(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOOLEAN))
#define G_VALUE_HOLDS_INT(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_INT))
#define G_VALUE_HOLDS_UINT(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_UINT))
#define G_VALUE_HOLDS_LONG(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_LONG))
#define G_VALUE_HOLDS_ULONG(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_ULONG))
#define G_VALUE_HOLDS_INT64(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_INT64))
#define G_VALUE_HOLDS_UINT64(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_UINT64))
#define G_VALUE_HOLDS_FLOAT(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_FLOAT))
#define G_VALUE_HOLDS_DOUBLE(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_DOUBLE))
#define G_VALUE_HOLDS_STRING(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_STRING))
#define G_VALUE_HOLDS_POINTER(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_POINTER))
#define	G_TYPE_GTYPE			 (g_gtype_get_type())
#define G_VALUE_HOLDS_GTYPE(value)	 (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_GTYPE))


/* --- prototypes --- */
void		      g_value_set_char		(GValue	      *value,
						 gchar	       v_char);
gchar		      g_value_get_char		(const GValue *value);
void		      g_value_set_uchar		(GValue	      *value,
						 guchar	       v_uchar);
guchar		      g_value_get_uchar		(const GValue *value);
void		      g_value_set_boolean	(GValue	      *value,
						 gboolean      v_boolean);
gboolean	      g_value_get_boolean	(const GValue *value);
void		      g_value_set_int		(GValue	      *value,
						 gint	       v_int);
gint		      g_value_get_int		(const GValue *value);
void		      g_value_set_uint		(GValue	      *value,
						 guint	       v_uint);
guint		      g_value_get_uint		(const GValue *value);
void		      g_value_set_long		(GValue	      *value,
						 glong	       v_long);
glong		      g_value_get_long		(const GValue *value);
void		      g_value_set_ulong		(GValue	      *value,
						 gulong	       v_ulong);
gulong		      g_value_get_ulong		(const GValue *value);
void		      g_value_set_int64		(GValue	      *value,
						 gint64	       v_int64);
gint64		      g_value_get_int64		(const GValue *value);
void		      g_value_set_uint64	(GValue	      *value,
						 guint64      v_uint64);
guint64		      g_value_get_uint64	(const GValue *value);
void		      g_value_set_float		(GValue	      *value,
						 gfloat	       v_float);
gfloat		      g_value_get_float		(const GValue *value);
void		      g_value_set_double	(GValue	      *value,
						 gdouble       v_double);
gdouble		      g_value_get_double	(const GValue *value);
void		      g_value_set_string	(GValue	      *value,
						 const gchar  *v_string);
void		      g_value_set_static_string (GValue	      *value,
						 const gchar  *v_string);
G_CONST_RETURN gchar* g_value_get_string	(const GValue *value);
gchar*		      g_value_dup_string	(const GValue *value);
void		      g_value_set_pointer	(GValue	      *value,
						 gpointer      v_pointer);
gpointer	      g_value_get_pointer	(const GValue *value);
GType		      g_gtype_get_type		(void);
void		      g_value_set_gtype	        (GValue	      *value,
						 GType         v_gtype);
GType	              g_value_get_gtype	        (const GValue *value);


/* Convenience for registering new pointer types */
GType                 g_pointer_type_register_static (const gchar *name);

/* debugging aid, describe value contents as string */
gchar*                g_strdup_value_contents   (const GValue *value);


void g_value_take_string		        (GValue		   *value,
						 gchar		   *v_string);
#ifndef G_DISABLE_DEPRECATED
void g_value_set_string_take_ownership		(GValue		   *value,
						 gchar		   *v_string);
#endif


/* humpf, need a C representable type name for G_TYPE_STRING */
typedef gchar* gchararray;


G_END_DECLS

#endif /* __G_VALUETYPES_H__ */
