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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __G_SIGNAL_H__
#define __G_SIGNAL_H__

#include	<gobject/gclosure.h>
#include	<gobject/gvalue.h>
#include	<gobject/gparam.h>
#include	<gobject/gmarshal.h>

G_BEGIN_DECLS

/* --- typedefs --- */
typedef struct _GSignalQuery		 GSignalQuery;
typedef struct _GSignalInvocationHint	 GSignalInvocationHint;
typedef GClosureMarshal			 GSignalCMarshaller;
typedef gboolean (*GSignalEmissionHook) (GSignalInvocationHint *ihint,
					 guint			n_param_values,
					 const GValue	       *param_values,
					 gpointer		data);
typedef gboolean (*GSignalAccumulator)	(GSignalInvocationHint *ihint,
					 GValue		       *return_accu,
					 const GValue	       *handler_return,
					 gpointer               data);


/* --- run, match and connect types --- */
typedef enum
{
  G_SIGNAL_RUN_FIRST	= 1 << 0,
  G_SIGNAL_RUN_LAST	= 1 << 1,
  G_SIGNAL_RUN_CLEANUP	= 1 << 2,
  G_SIGNAL_NO_RECURSE	= 1 << 3,
  G_SIGNAL_DETAILED	= 1 << 4,
  G_SIGNAL_ACTION	= 1 << 5,
  G_SIGNAL_NO_HOOKS	= 1 << 6
} GSignalFlags;
#define G_SIGNAL_FLAGS_MASK  0x7f
typedef enum
{
  G_CONNECT_AFTER	= 1 << 0,
  G_CONNECT_SWAPPED	= 1 << 1
} GConnectFlags;
typedef enum
{
  G_SIGNAL_MATCH_ID	   = 1 << 0,
  G_SIGNAL_MATCH_DETAIL	   = 1 << 1,
  G_SIGNAL_MATCH_CLOSURE   = 1 << 2,
  G_SIGNAL_MATCH_FUNC	   = 1 << 3,
  G_SIGNAL_MATCH_DATA	   = 1 << 4,
  G_SIGNAL_MATCH_UNBLOCKED = 1 << 5
} GSignalMatchType;
#define G_SIGNAL_MATCH_MASK  0x3f
#define	G_SIGNAL_TYPE_STATIC_SCOPE (G_TYPE_FLAG_RESERVED_ID_BIT)


/* --- signal information --- */
struct _GSignalInvocationHint
{
  guint		signal_id;
  GQuark	detail;
  GSignalFlags	run_type;
};
struct _GSignalQuery
{
  guint		signal_id;
  const gchar  *signal_name;
  GType		itype;
  GSignalFlags	signal_flags;
  GType		return_type; /* mangled with G_SIGNAL_TYPE_STATIC_SCOPE flag */
  guint		n_params;
  const GType  *param_types; /* mangled with G_SIGNAL_TYPE_STATIC_SCOPE flag */
};


/* --- signals --- */
guint                 g_signal_newv         (const gchar        *signal_name,
					     GType               itype,
					     GSignalFlags        signal_flags,
					     GClosure           *class_closure,
					     GSignalAccumulator	 accumulator,
					     gpointer		 accu_data,
					     GSignalCMarshaller  c_marshaller,
					     GType               return_type,
					     guint               n_params,
					     GType              *param_types);
guint                 g_signal_new_valist   (const gchar        *signal_name,
					     GType               itype,
					     GSignalFlags        signal_flags,
					     GClosure           *class_closure,
					     GSignalAccumulator	 accumulator,
					     gpointer		 accu_data,
					     GSignalCMarshaller  c_marshaller,
					     GType               return_type,
					     guint               n_params,
					     va_list             args);
guint                 g_signal_new          (const gchar        *signal_name,
					     GType               itype,
					     GSignalFlags        signal_flags,
					     guint               class_offset,
					     GSignalAccumulator	 accumulator,
					     gpointer		 accu_data,
					     GSignalCMarshaller  c_marshaller,
					     GType               return_type,
					     guint               n_params,
					     ...);
void                  g_signal_emitv        (const GValue       *instance_and_params,
					     guint               signal_id,
					     GQuark              detail,
					     GValue             *return_value);
void                  g_signal_emit_valist  (gpointer            instance,
					     guint               signal_id,
					     GQuark              detail,
					     va_list             var_args);
void                  g_signal_emit         (gpointer            instance,
					     guint               signal_id,
					     GQuark              detail,
					     ...);
void                  g_signal_emit_by_name (gpointer            instance,
					     const gchar        *detailed_signal,
					     ...);
guint                 g_signal_lookup       (const gchar        *name,
					     GType               itype);
G_CONST_RETURN gchar* g_signal_name         (guint               signal_id);
void                  g_signal_query        (guint               signal_id,
					     GSignalQuery       *query);
guint*                g_signal_list_ids     (GType               itype,
					     guint              *n_ids);
gboolean	      g_signal_parse_name   (const gchar	*detailed_signal,
					     GType		 itype,
					     guint		*signal_id_p,
					     GQuark		*detail_p,
					     gboolean		 force_detail_quark);
GSignalInvocationHint* g_signal_get_invocation_hint (gpointer    instance);


/* --- signal emissions --- */
void	g_signal_stop_emission		    (gpointer		  instance,
					     guint		  signal_id,
					     GQuark		  detail);
void	g_signal_stop_emission_by_name	    (gpointer		  instance,
					     const gchar	 *detailed_signal);
gulong	g_signal_add_emission_hook	    (guint		  signal_id,
					     GQuark		  detail,
					     GSignalEmissionHook  hook_func,
					     gpointer	       	  hook_data,
					     GDestroyNotify	  data_destroy);
void	g_signal_remove_emission_hook	    (guint		  signal_id,
					     gulong		  hook_id);


/* --- signal handlers --- */
gboolean g_signal_has_handler_pending	      (gpointer		  instance,
					       guint		  signal_id,
					       GQuark		  detail,
					       gboolean		  may_be_blocked);
gulong	 g_signal_connect_closure_by_id	      (gpointer		  instance,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gboolean		  after);
gulong	 g_signal_connect_closure	      (gpointer		  instance,
					       const gchar       *detailed_signal,
					       GClosure		 *closure,
					       gboolean		  after);
gulong	 g_signal_connect_data		      (gpointer		  instance,
					       const gchar	 *detailed_signal,
					       GCallback	  c_handler,
					       gpointer		  data,
					       GClosureNotify	  destroy_data,
					       GConnectFlags	  connect_flags);
void	 g_signal_handler_block		      (gpointer		  instance,
					       gulong		  handler_id);
void	 g_signal_handler_unblock	      (gpointer		  instance,
					       gulong		  handler_id);
void	 g_signal_handler_disconnect	      (gpointer		  instance,
					       gulong		  handler_id);
gboolean g_signal_handler_is_connected	      (gpointer		  instance,
					       gulong		  handler_id);
gulong	 g_signal_handler_find		      (gpointer		  instance,
					       GSignalMatchType	  mask,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gpointer		  func,
					       gpointer		  data);
guint	 g_signal_handlers_block_matched      (gpointer		  instance,
					       GSignalMatchType	  mask,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gpointer		  func,
					       gpointer		  data);
guint	 g_signal_handlers_unblock_matched    (gpointer		  instance,
					       GSignalMatchType	  mask,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gpointer		  func,
					       gpointer		  data);
guint	 g_signal_handlers_disconnect_matched (gpointer		  instance,
					       GSignalMatchType	  mask,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gpointer		  func,
					       gpointer		  data);


/* --- chaining for language bindings --- */
void	g_signal_override_class_closure	      (guint		  signal_id,
					       GType		  instance_type,
					       GClosure		 *class_closure);
void	g_signal_chain_from_overridden	      (const GValue      *instance_and_params,
					       GValue            *return_value);


/* --- convenience --- */
#define g_signal_connect(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, (GConnectFlags) 0)
#define g_signal_connect_after(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, G_CONNECT_AFTER)
#define g_signal_connect_swapped(instance, detailed_signal, c_handler, data) \
    g_signal_connect_data ((instance), (detailed_signal), (c_handler), (data), NULL, G_CONNECT_SWAPPED)
#define	g_signal_handlers_disconnect_by_func(instance, func, data)						\
    g_signal_handlers_disconnect_matched ((instance),								\
					  (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),	\
					  0, 0, NULL, (func), (data))
#define	g_signal_handlers_block_by_func(instance, func, data)							\
    g_signal_handlers_block_matched      ((instance),								\
				          (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),	\
				          0, 0, NULL, (func), (data))
#define	g_signal_handlers_unblock_by_func(instance, func, data)							\
    g_signal_handlers_unblock_matched    ((instance),								\
				          (GSignalMatchType) (G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA),	\
				          0, 0, NULL, (func), (data))


gboolean g_signal_accumulator_true_handled (GSignalInvocationHint *ihint,
					    GValue                *return_accu,
					    const GValue          *handler_return,
					    gpointer               dummy);

/*< private >*/
void	 g_signal_handlers_destroy	      (gpointer		  instance);
void	 _g_signals_destroy		      (GType		  itype);

G_END_DECLS

#endif /* __G_SIGNAL_H__ */
