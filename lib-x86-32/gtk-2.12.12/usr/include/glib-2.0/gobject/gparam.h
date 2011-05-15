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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * gparam.h: GParamSpec base class implementation
 */
#if !defined (__GLIB_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#ifndef __G_PARAM_H__
#define __G_PARAM_H__

#include	<gobject/gvalue.h>

G_BEGIN_DECLS

/* --- standard type macros --- */
#define G_TYPE_IS_PARAM(type)		(G_TYPE_FUNDAMENTAL (type) == G_TYPE_PARAM)
#define G_PARAM_SPEC(pspec)		(G_TYPE_CHECK_INSTANCE_CAST ((pspec), G_TYPE_PARAM, GParamSpec))
#define G_IS_PARAM_SPEC(pspec)		(G_TYPE_CHECK_INSTANCE_TYPE ((pspec), G_TYPE_PARAM))
#define G_PARAM_SPEC_CLASS(pclass)      (G_TYPE_CHECK_CLASS_CAST ((pclass), G_TYPE_PARAM, GParamSpecClass))
#define G_IS_PARAM_SPEC_CLASS(pclass)   (G_TYPE_CHECK_CLASS_TYPE ((pclass), G_TYPE_PARAM))
#define G_PARAM_SPEC_GET_CLASS(pspec)	(G_TYPE_INSTANCE_GET_CLASS ((pspec), G_TYPE_PARAM, GParamSpecClass))


/* --- convenience macros --- */
#define G_PARAM_SPEC_TYPE(pspec)	(G_TYPE_FROM_INSTANCE (pspec))
#define G_PARAM_SPEC_TYPE_NAME(pspec)	(g_type_name (G_PARAM_SPEC_TYPE (pspec)))
#define	G_PARAM_SPEC_VALUE_TYPE(pspec)	(G_PARAM_SPEC (pspec)->value_type)
#define G_VALUE_HOLDS_PARAM(value)	(G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_PARAM))
       

/* --- flags --- */
typedef enum
{
  G_PARAM_READABLE            = 1 << 0,
  G_PARAM_WRITABLE            = 1 << 1,
  G_PARAM_CONSTRUCT	      = 1 << 2,
  G_PARAM_CONSTRUCT_ONLY      = 1 << 3,
  G_PARAM_LAX_VALIDATION      = 1 << 4,
  G_PARAM_STATIC_NAME	      = 1 << 5,
#ifndef G_DISABLE_DEPRECATED
  G_PARAM_PRIVATE	      = G_PARAM_STATIC_NAME,
#endif
  G_PARAM_STATIC_NICK	      = 1 << 6,
  G_PARAM_STATIC_BLURB	      = 1 << 7
} GParamFlags;
#define	G_PARAM_READWRITE	(G_PARAM_READABLE | G_PARAM_WRITABLE)
#define	G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#define	G_PARAM_MASK		(0x000000ff)
/* bits in the range 0xffffff00 are reserved for 3rd party usage */
#define	G_PARAM_USER_SHIFT	(8)


/* --- typedefs & structures --- */
typedef struct _GParamSpec      GParamSpec;
typedef struct _GParamSpecClass GParamSpecClass;
typedef struct _GParameter	GParameter;
typedef struct _GParamSpecPool  GParamSpecPool;
struct _GParamSpec
{
  GTypeInstance  g_type_instance;

  gchar         *name;
  GParamFlags    flags;
  GType		 value_type;
  GType		 owner_type;	/* class or interface using this property */

  /*< private >*/
  gchar         *_nick;
  gchar         *_blurb;
  GData		*qdata;
  guint          ref_count;
  guint		 param_id;	/* sort-criteria */
};
struct _GParamSpecClass
{
  GTypeClass      g_type_class;

  GType		  value_type;

  void	        (*finalize)		(GParamSpec   *pspec);

  /* GParam methods */
  void          (*value_set_default)    (GParamSpec   *pspec,
					 GValue       *value);
  gboolean      (*value_validate)       (GParamSpec   *pspec,
					 GValue       *value);
  gint          (*values_cmp)           (GParamSpec   *pspec,
					 const GValue *value1,
					 const GValue *value2);
  /*< private >*/
  gpointer	  dummy[4];
};
struct _GParameter /* auxillary structure for _setv() variants */
{
  const gchar *name;
  GValue       value;
};


/* --- prototypes --- */
GParamSpec*	g_param_spec_ref		(GParamSpec    *pspec);
void		g_param_spec_unref		(GParamSpec    *pspec);
void		g_param_spec_sink		(GParamSpec    *pspec);
GParamSpec*	g_param_spec_ref_sink   	(GParamSpec    *pspec);
gpointer        g_param_spec_get_qdata		(GParamSpec    *pspec,
						 GQuark         quark);
void            g_param_spec_set_qdata		(GParamSpec    *pspec,
						 GQuark         quark,
						 gpointer       data);
void            g_param_spec_set_qdata_full	(GParamSpec    *pspec,
						 GQuark         quark,
						 gpointer       data,
						 GDestroyNotify destroy);
gpointer        g_param_spec_steal_qdata	(GParamSpec    *pspec,
						 GQuark         quark);
GParamSpec*     g_param_spec_get_redirect_target (GParamSpec   *pspec);

void		g_param_value_set_default	(GParamSpec    *pspec,
						 GValue	       *value);
gboolean	g_param_value_defaults		(GParamSpec    *pspec,
						 GValue	       *value);
gboolean	g_param_value_validate		(GParamSpec    *pspec,
						 GValue	       *value);
gboolean	g_param_value_convert		(GParamSpec    *pspec,
						 const GValue  *src_value,
						 GValue	       *dest_value,
						 gboolean	strict_validation);
gint		g_param_values_cmp		(GParamSpec    *pspec,
						 const GValue  *value1,
						 const GValue  *value2);
G_CONST_RETURN gchar*	g_param_spec_get_name	(GParamSpec    *pspec);
G_CONST_RETURN gchar*	g_param_spec_get_nick	(GParamSpec    *pspec);
G_CONST_RETURN gchar*	g_param_spec_get_blurb	(GParamSpec    *pspec);
void            g_value_set_param               (GValue	       *value,
						 GParamSpec    *param);
GParamSpec*     g_value_get_param               (const GValue  *value);
GParamSpec*     g_value_dup_param               (const GValue  *value);


void           g_value_take_param               (GValue        *value,
					         GParamSpec    *param);
#ifndef G_DISABLE_DEPRECATED
void           g_value_set_param_take_ownership (GValue        *value,
					         GParamSpec    *param);
#endif

/* --- convenience functions --- */
typedef struct _GParamSpecTypeInfo GParamSpecTypeInfo;
struct _GParamSpecTypeInfo
{
  /* type system portion */
  guint16         instance_size;                               /* obligatory */
  guint16         n_preallocs;                                 /* optional */
  void		(*instance_init)	(GParamSpec   *pspec); /* optional */

  /* class portion */
  GType           value_type;				       /* obligatory */
  void          (*finalize)             (GParamSpec   *pspec); /* optional */
  void          (*value_set_default)    (GParamSpec   *pspec,  /* recommended */
					 GValue       *value);
  gboolean      (*value_validate)       (GParamSpec   *pspec,  /* optional */
					 GValue       *value);
  gint          (*values_cmp)           (GParamSpec   *pspec,  /* recommended */
					 const GValue *value1,
					 const GValue *value2);
};
GType	g_param_type_register_static	(const gchar		  *name,
					 const GParamSpecTypeInfo *pspec_info);

/* For registering builting types */
GType  _g_param_type_register_static_constant (const gchar              *name,
					       const GParamSpecTypeInfo *pspec_info,
					       GType                     opt_type);


/* --- protected --- */
gpointer	g_param_spec_internal		(GType	        param_type,
						 const gchar   *name,
						 const gchar   *nick,
						 const gchar   *blurb,
						 GParamFlags    flags);
GParamSpecPool* g_param_spec_pool_new		(gboolean	type_prefixing);
void		g_param_spec_pool_insert	(GParamSpecPool	*pool,
						 GParamSpec	*pspec,
						 GType		 owner_type);
void		g_param_spec_pool_remove	(GParamSpecPool	*pool,
						 GParamSpec	*pspec);
GParamSpec*	g_param_spec_pool_lookup	(GParamSpecPool	*pool,
						 const gchar	*param_name,
						 GType		 owner_type,
						 gboolean	 walk_ancestors);
GList*		g_param_spec_pool_list_owned	(GParamSpecPool	*pool,
						 GType		 owner_type);
GParamSpec**	g_param_spec_pool_list		(GParamSpecPool	*pool,
						 GType		 owner_type,
						 guint		*n_pspecs_p);



/* contracts:
 *
 * gboolean value_validate (GParamSpec *pspec,
 *                          GValue     *value):
 *	modify value contents in the least destructive way, so
 *	that it complies with pspec's requirements (i.e.
 *	according to minimum/maximum ranges etc...). return
 *	whether modification was necessary.
 *
 * gint values_cmp (GParamSpec   *pspec,
 *                  const GValue *value1,
 *                  const GValue *value2):
 *	return value1 - value2, i.e. (-1) if value1 < value2,
 *	(+1) if value1 > value2, and (0) otherwise (equality)
 */

G_END_DECLS

#endif /* __G_PARAM_H__ */
