/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1998-1999, 2000-2001 Tim Janik and Red Hat, Inc.
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

#ifndef __G_OBJECT_H__
#define __G_OBJECT_H__

#include        <gobject/gtype.h>
#include        <gobject/gvalue.h>
#include        <gobject/gparam.h>
#include        <gobject/gclosure.h>
#include        <gobject/gsignal.h>

G_BEGIN_DECLS

/* --- type macros --- */
#define G_TYPE_IS_OBJECT(type)      (G_TYPE_FUNDAMENTAL (type) == G_TYPE_OBJECT)
#define G_OBJECT(object)            (G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_OBJECT, GObject))
#define G_OBJECT_CLASS(class)       (G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_OBJECT, GObjectClass))
#define G_IS_OBJECT(object)         (G_TYPE_CHECK_INSTANCE_TYPE ((object), G_TYPE_OBJECT))
#define G_IS_OBJECT_CLASS(class)    (G_TYPE_CHECK_CLASS_TYPE ((class), G_TYPE_OBJECT))
#define G_OBJECT_GET_CLASS(object)  (G_TYPE_INSTANCE_GET_CLASS ((object), G_TYPE_OBJECT, GObjectClass))
#define G_OBJECT_TYPE(object)       (G_TYPE_FROM_INSTANCE (object))
#define G_OBJECT_TYPE_NAME(object)  (g_type_name (G_OBJECT_TYPE (object)))
#define G_OBJECT_CLASS_TYPE(class)  (G_TYPE_FROM_CLASS (class))
#define G_OBJECT_CLASS_NAME(class)  (g_type_name (G_OBJECT_CLASS_TYPE (class)))
#define G_VALUE_HOLDS_OBJECT(value) (G_TYPE_CHECK_VALUE_TYPE ((value), G_TYPE_OBJECT))

/* --- type macros --- */
#define G_TYPE_INITIALLY_UNOWNED	      (g_initially_unowned_get_type())
#define G_INITIALLY_UNOWNED(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_INITIALLY_UNOWNED, GInitiallyUnowned))
#define G_INITIALLY_UNOWNED_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_INITIALLY_UNOWNED, GInitiallyUnownedClass))
#define G_IS_INITIALLY_UNOWNED(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), G_TYPE_INITIALLY_UNOWNED))
#define G_IS_INITIALLY_UNOWNED_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), G_TYPE_INITIALLY_UNOWNED))
#define G_INITIALLY_UNOWNED_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), G_TYPE_INITIALLY_UNOWNED, GInitiallyUnownedClass))
/* GInitiallyUnowned ia a GObject with initially floating reference count */


/* --- typedefs & structures --- */
typedef struct _GObject                  GObject;
typedef struct _GObjectClass             GObjectClass;
typedef struct _GObject                  GInitiallyUnowned;
typedef struct _GObjectClass             GInitiallyUnownedClass;
typedef struct _GObjectConstructParam    GObjectConstructParam;
typedef void (*GObjectGetPropertyFunc)  (GObject      *object,
                                         guint         property_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
typedef void (*GObjectSetPropertyFunc)  (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
typedef void (*GObjectFinalizeFunc)     (GObject      *object);
typedef void (*GWeakNotify)		(gpointer      data,
					 GObject      *where_the_object_was);
struct  _GObject
{
  GTypeInstance  g_type_instance;
  
  /*< private >*/
  volatile guint ref_count;
  GData         *qdata;
};
struct  _GObjectClass
{
  GTypeClass   g_type_class;

  /*< private >*/
  GSList      *construct_properties;

  /*< public >*/
  /* seldomly overidden */
  GObject*   (*constructor)     (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties);
  /* overridable methods */
  void       (*set_property)		(GObject        *object,
                                         guint           property_id,
                                         const GValue   *value,
                                         GParamSpec     *pspec);
  void       (*get_property)		(GObject        *object,
                                         guint           property_id,
                                         GValue         *value,
                                         GParamSpec     *pspec);
  void       (*dispose)			(GObject        *object);
  void       (*finalize)		(GObject        *object);
  /* seldomly overidden */
  void       (*dispatch_properties_changed) (GObject      *object,
					     guint	   n_pspecs,
					     GParamSpec  **pspecs);
  /* signals */
  void	     (*notify)			(GObject	*object,
					 GParamSpec	*pspec);

  /* called when done constructing */
  void	     (*constructed)		(GObject	*object);

  /*< private >*/
  /* padding */
  gpointer	pdummy[7];
};
struct _GObjectConstructParam
{
  GParamSpec *pspec;
  GValue     *value;
};


/* --- prototypes --- */
GType       g_initially_unowned_get_type      (void);
void        g_object_class_install_property   (GObjectClass   *oclass,
					       guint           property_id,
					       GParamSpec     *pspec);
GParamSpec* g_object_class_find_property      (GObjectClass   *oclass,
					       const gchar    *property_name);
GParamSpec**g_object_class_list_properties    (GObjectClass   *oclass,
					       guint	      *n_properties);
void        g_object_class_override_property  (GObjectClass   *oclass,
					       guint           property_id,
					       const gchar    *name);

void        g_object_interface_install_property (gpointer     g_iface,
						 GParamSpec  *pspec);
GParamSpec* g_object_interface_find_property    (gpointer     g_iface,
						 const gchar *property_name);
GParamSpec**g_object_interface_list_properties  (gpointer     g_iface,
						 guint       *n_properties_p);

gpointer    g_object_new                      (GType           object_type,
					       const gchar    *first_property_name,
					       ...);
gpointer    g_object_newv		      (GType           object_type,
					       guint	       n_parameters,
					       GParameter     *parameters);
GObject*    g_object_new_valist               (GType           object_type,
					       const gchar    *first_property_name,
					       va_list         var_args);
void	    g_object_set                      (gpointer	       object,
					       const gchar    *first_property_name,
					       ...) G_GNUC_NULL_TERMINATED;
void        g_object_get                      (gpointer        object,
					       const gchar    *first_property_name,
					       ...) G_GNUC_NULL_TERMINATED;
gpointer    g_object_connect                  (gpointer	       object,
					       const gchar    *signal_spec,
					       ...) G_GNUC_NULL_TERMINATED;
void	    g_object_disconnect               (gpointer	       object,
					       const gchar    *signal_spec,
					       ...) G_GNUC_NULL_TERMINATED;
void        g_object_set_valist               (GObject        *object,
					       const gchar    *first_property_name,
					       va_list         var_args);
void        g_object_get_valist               (GObject        *object,
					       const gchar    *first_property_name,
					       va_list         var_args);
void        g_object_set_property             (GObject        *object,
					       const gchar    *property_name,
					       const GValue   *value);
void        g_object_get_property             (GObject        *object,
					       const gchar    *property_name,
					       GValue         *value);
void        g_object_freeze_notify            (GObject        *object);
void        g_object_notify                   (GObject        *object,
					       const gchar    *property_name);
void        g_object_thaw_notify              (GObject        *object);
gboolean    g_object_is_floating    	      (gpointer        object);
gpointer    g_object_ref_sink       	      (gpointer	       object);
gpointer    g_object_ref                      (gpointer        object);
void        g_object_unref                    (gpointer        object);
void	    g_object_weak_ref		      (GObject	      *object,
					       GWeakNotify     notify,
					       gpointer	       data);
void	    g_object_weak_unref		      (GObject	      *object,
					       GWeakNotify     notify,
					       gpointer	       data);
void        g_object_add_weak_pointer         (GObject        *object, 
                                               gpointer       *weak_pointer_location);
void        g_object_remove_weak_pointer      (GObject        *object, 
                                               gpointer       *weak_pointer_location);

typedef void (*GToggleNotify) (gpointer      data,
			       GObject      *object,
			       gboolean      is_last_ref);

void g_object_add_toggle_ref    (GObject       *object,
				 GToggleNotify  notify,
				 gpointer       data);
void g_object_remove_toggle_ref (GObject       *object,
				 GToggleNotify  notify,
				 gpointer       data);

gpointer    g_object_get_qdata                (GObject        *object,
					       GQuark          quark);
void        g_object_set_qdata                (GObject        *object,
					       GQuark          quark,
					       gpointer        data);
void        g_object_set_qdata_full           (GObject        *object,
					       GQuark          quark,
					       gpointer        data,
					       GDestroyNotify  destroy);
gpointer    g_object_steal_qdata              (GObject        *object,
					       GQuark          quark);
gpointer    g_object_get_data                 (GObject        *object,
					       const gchar    *key);
void        g_object_set_data                 (GObject        *object,
					       const gchar    *key,
					       gpointer        data);
void        g_object_set_data_full            (GObject        *object,
					       const gchar    *key,
					       gpointer        data,
					       GDestroyNotify  destroy);
gpointer    g_object_steal_data               (GObject        *object,
					       const gchar    *key);
void        g_object_watch_closure            (GObject        *object,
					       GClosure       *closure);
GClosure*   g_cclosure_new_object             (GCallback       callback_func,
					       GObject	      *object);
GClosure*   g_cclosure_new_object_swap        (GCallback       callback_func,
					       GObject	      *object);
GClosure*   g_closure_new_object              (guint           sizeof_closure,
					       GObject        *object);
void        g_value_set_object                (GValue         *value,
					       gpointer        v_object);
gpointer    g_value_get_object                (const GValue   *value);
gpointer    g_value_dup_object                (const GValue   *value);
gulong	    g_signal_connect_object           (gpointer	       instance,
					       const gchar    *detailed_signal,
					       GCallback       c_handler,
					       gpointer	       gobject,
					       GConnectFlags   connect_flags);

/*< protected >*/
void        g_object_force_floating           (GObject        *object);
void        g_object_run_dispose	      (GObject	      *object);


void        g_value_take_object               (GValue         *value,
					       gpointer        v_object);
#ifndef G_DISABLE_DEPRECATED
void        g_value_set_object_take_ownership (GValue         *value,
					       gpointer        v_object);
#endif

#if !defined(G_DISABLE_DEPRECATED) || defined(GTK_COMPILATION)
gsize	    g_object_compat_control	      (gsize	       what,
					       gpointer	       data);
#endif

/* --- implementation macros --- */
#define G_OBJECT_WARN_INVALID_PSPEC(object, pname, property_id, pspec) \
G_STMT_START { \
  GObject *_object = (GObject*) (object); \
  GParamSpec *_pspec = (GParamSpec*) (pspec); \
  guint _property_id = (property_id); \
  g_warning ("%s: invalid %s id %u for \"%s\" of type `%s' in `%s'", \
             G_STRLOC, \
             (pname), \
             _property_id, \
             _pspec->name, \
             g_type_name (G_PARAM_SPEC_TYPE (_pspec)), \
             G_OBJECT_TYPE_NAME (_object)); \
} G_STMT_END
#define G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec) \
    G_OBJECT_WARN_INVALID_PSPEC ((object), "property", (property_id), (pspec))

G_END_DECLS

#endif /* __G_OBJECT_H__ */
