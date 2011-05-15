/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
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
 */
#if !defined (__GLIB_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#ifndef __G_BOXED_H__
#define __G_BOXED_H__

#include        <gobject/gtype.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define G_TYPE_IS_BOXED(type)	   (G_TYPE_FUNDAMENTAL (type) == G_TYPE_BOXED)
#define G_VALUE_HOLDS_BOXED(value) (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_BOXED))


/* --- typedefs --- */
typedef gpointer (*GBoxedCopyFunc)	(gpointer	 boxed);
typedef void     (*GBoxedFreeFunc)	(gpointer	 boxed);


/* --- prototypes --- */
gpointer	g_boxed_copy			(GType		 boxed_type,
						 gconstpointer	 src_boxed);
void		g_boxed_free			(GType		 boxed_type,
						 gpointer	 boxed);
void		g_value_set_boxed		(GValue		*value,
						 gconstpointer	 v_boxed);
void		g_value_set_static_boxed	(GValue		*value,
						 gconstpointer	 v_boxed);
gpointer	g_value_get_boxed		(const GValue	*value);
gpointer	g_value_dup_boxed		(const GValue	*value);


/* --- convenience --- */
GType	g_boxed_type_register_static		(const gchar	*name,
						 GBoxedCopyFunc	 boxed_copy,
						 GBoxedFreeFunc	 boxed_free);


/* --- GLib boxed types --- */
#define	G_TYPE_CLOSURE		(g_closure_get_type ())
#define	G_TYPE_VALUE		(g_value_get_type ())
#define	G_TYPE_VALUE_ARRAY	(g_value_array_get_type ())
#define	G_TYPE_DATE	        (g_date_get_type ())
#define	G_TYPE_STRV	        (g_strv_get_type ())
#define	G_TYPE_GSTRING		(g_gstring_get_type ())
#define	G_TYPE_HASH_TABLE	(g_hash_table_get_type ())
#define	G_TYPE_REGEX		(g_regex_get_type ())


void    g_value_take_boxed      (GValue		*value,
				 gconstpointer	 v_boxed);
#ifndef G_DISABLE_DEPRECATED
void	g_value_set_boxed_take_ownership	(GValue		*value,
						 gconstpointer	 v_boxed);
#endif
GType	g_closure_get_type	(void)	G_GNUC_CONST;
GType	g_value_get_type	(void)	G_GNUC_CONST;
GType	g_value_array_get_type	(void)	G_GNUC_CONST;
GType	g_date_get_type	        (void)	G_GNUC_CONST;
GType	g_strv_get_type	        (void)	G_GNUC_CONST;
GType	g_gstring_get_type      (void)	G_GNUC_CONST;
GType   g_hash_table_get_type   (void)  G_GNUC_CONST;
GType   g_regex_get_type        (void)  G_GNUC_CONST;

typedef gchar** GStrv;
     
G_END_DECLS

#endif	/* __G_BOXED_H__ */
