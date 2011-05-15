/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000 Red Hat, Inc.
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
#if !defined (__GLIB_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#ifndef __G_TYPE_MODULE_H__
#define __G_TYPE_MODULE_H__

#include <gobject/gobject.h>
#include <gobject/genums.h>

G_BEGIN_DECLS

typedef struct _GTypeModule      GTypeModule;
typedef struct _GTypeModuleClass GTypeModuleClass;

#define G_TYPE_TYPE_MODULE              (g_type_module_get_type ())
#define G_TYPE_MODULE(module)           (G_TYPE_CHECK_INSTANCE_CAST ((module), G_TYPE_TYPE_MODULE, GTypeModule))
#define G_TYPE_MODULE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), G_TYPE_TYPE_MODULE, GTypeModuleClass))
#define G_IS_TYPE_MODULE(module)        (G_TYPE_CHECK_INSTANCE_TYPE ((module), G_TYPE_TYPE_MODULE))
#define G_IS_TYPE_MODULE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), G_TYPE_TYPE_MODULE))
#define G_TYPE_MODULE_GET_CLASS(module) (G_TYPE_INSTANCE_GET_CLASS ((module), G_TYPE_TYPE_MODULE, GTypeModuleClass))

struct _GTypeModule 
{
  GObject parent_instance;

  guint use_count;
  GSList *type_infos;
  GSList *interface_infos;

  /*< public >*/
  gchar *name;
};

struct _GTypeModuleClass
{
  GObjectClass parent_class;

  /*< public >*/
  gboolean (* load)   (GTypeModule *module);
  void     (* unload) (GTypeModule *module);

  /*< private >*/
  /* Padding for future expansion */
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
};

#define G_DEFINE_DYNAMIC_TYPE(TN, t_n, T_P)          G_DEFINE_DYNAMIC_TYPE_EXTENDED (TN, t_n, T_P, 0, {})
#define G_DEFINE_DYNAMIC_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE) \
static void     type_name##_init              (TypeName        *self); \
static void     type_name##_class_init        (TypeName##Class *klass); \
static void     type_name##_class_finalize    (TypeName##Class *klass); \
static gpointer type_name##_parent_class = NULL; \
static GType    type_name##_type_id = 0; \
static void     type_name##_class_intern_init (gpointer klass) \
{ \
  type_name##_parent_class = g_type_class_peek_parent (klass); \
  type_name##_class_init ((TypeName##Class*) klass); \
} \
GType \
type_name##_get_type (void) \
{ \
  return type_name##_type_id; \
} \
static void \
type_name##_register_type (GTypeModule *type_module) \
{ \
  GType g_define_type_id; \
  const GTypeInfo g_define_type_info = { \
    sizeof (TypeName##Class), \
    (GBaseInitFunc) NULL, \
    (GBaseFinalizeFunc) NULL, \
    (GClassInitFunc) type_name##_class_intern_init, \
    (GClassFinalizeFunc) type_name##_class_finalize, \
    NULL,   /* class_data */ \
    sizeof (TypeName), \
    0,      /* n_preallocs */ \
    (GInstanceInitFunc) type_name##_init, \
    NULL    /* value_table */ \
  }; \
  type_name##_type_id = g_type_module_register_type (type_module, \
						     TYPE_PARENT, \
						     #TypeName, \
						     &g_define_type_info, \
						     (GTypeFlags) flags); \
  g_define_type_id = type_name##_type_id; \
  { CODE ; } \
}


GType    g_type_module_get_type       (void) G_GNUC_CONST;
gboolean g_type_module_use            (GTypeModule          *module);
void     g_type_module_unuse          (GTypeModule          *module);
void     g_type_module_set_name       (GTypeModule          *module,
                                       const gchar          *name);
GType    g_type_module_register_type  (GTypeModule          *module,
                                       GType                 parent_type,
                                       const gchar          *type_name,
                                       const GTypeInfo      *type_info,
                                       GTypeFlags            flags);
void     g_type_module_add_interface  (GTypeModule          *module,
                                       GType                 instance_type,
                                       GType                 interface_type,
                                       const GInterfaceInfo *interface_info);
GType    g_type_module_register_enum  (GTypeModule          *module,
                                       const gchar          *name,
                                       const GEnumValue     *const_static_values);
GType    g_type_module_register_flags (GTypeModule          *module,
                                       const gchar          *name,
                                       const GFlagsValue    *const_static_values);

G_END_DECLS

#endif /* __G_TYPE_MODULE_H__ */
