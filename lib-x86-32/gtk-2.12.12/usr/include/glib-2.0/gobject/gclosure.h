/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2000-2001 Red Hat, Inc.
 * Copyright (C) 2005 Imendio AB
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

#ifndef __G_CLOSURE_H__
#define __G_CLOSURE_H__

#include        <gobject/gtype.h>

G_BEGIN_DECLS

/* --- defines --- */
#define	G_CLOSURE_NEEDS_MARSHAL(closure) (((GClosure*) (closure))->marshal == NULL)
#define	G_CLOSURE_N_NOTIFIERS(cl)	 ((cl)->meta_marshal + ((cl)->n_guards << 1L) + \
                                          (cl)->n_fnotifiers + (cl)->n_inotifiers)
#define	G_CCLOSURE_SWAP_DATA(cclosure)	 (((GClosure*) (cclosure))->derivative_flag)
#define	G_CALLBACK(f)			 ((GCallback) (f))


/* -- typedefs --- */
typedef struct _GClosure		 GClosure;
typedef struct _GClosureNotifyData	 GClosureNotifyData;
typedef void  (*GCallback)              (void);
typedef void  (*GClosureNotify)		(gpointer	 data,
					 GClosure	*closure);
typedef void  (*GClosureMarshal)	(GClosure	*closure,
					 GValue         *return_value,
					 guint           n_param_values,
					 const GValue   *param_values,
					 gpointer        invocation_hint,
					 gpointer	 marshal_data);
typedef struct _GCClosure		 GCClosure;


/* --- structures --- */
struct _GClosureNotifyData
{
  gpointer       data;
  GClosureNotify notify;
};
struct _GClosure
{
  /*< private >*/
  volatile      	guint	 ref_count : 15;
  volatile       	guint	 meta_marshal : 1;
  volatile       	guint	 n_guards : 1;
  volatile       	guint	 n_fnotifiers : 2;	/* finalization notifiers */
  volatile       	guint	 n_inotifiers : 8;	/* invalidation notifiers */
  volatile       	guint	 in_inotify : 1;
  volatile       	guint	 floating : 1;
  /*< protected >*/
  volatile         	guint	 derivative_flag : 1;
  /*< public >*/
  volatile       	guint	 in_marshal : 1;
  volatile       	guint	 is_invalid : 1;

  /*< private >*/	void   (*marshal)  (GClosure       *closure,
					    GValue /*out*/ *return_value,
					    guint           n_param_values,
					    const GValue   *param_values,
					    gpointer        invocation_hint,
					    gpointer	    marshal_data);
  /*< protected >*/	gpointer data;

  /*< private >*/	GClosureNotifyData *notifiers;

  /* invariants/constrains:
   * - ->marshal and ->data are _invalid_ as soon as ->is_invalid==TRUE
   * - invocation of all inotifiers occours prior to fnotifiers
   * - order of inotifiers is random
   *   inotifiers may _not_ free/invalidate parameter values (e.g. ->data)
   * - order of fnotifiers is random
   * - each notifier may only be removed before or during its invocation
   * - reference counting may only happen prior to fnotify invocation
   *   (in that sense, fnotifiers are really finalization handlers)
   */
};
/* closure for C function calls, callback() is the user function
 */
struct _GCClosure
{
  GClosure	closure;
  gpointer	callback;
};


/* --- prototypes --- */
GClosure* g_cclosure_new			(GCallback	callback_func,
						 gpointer	user_data,
						 GClosureNotify destroy_data);
GClosure* g_cclosure_new_swap			(GCallback	callback_func,
						 gpointer	user_data,
						 GClosureNotify destroy_data);
GClosure* g_signal_type_cclosure_new		(GType          itype,
						 guint          struct_offset);


/* --- prototypes --- */
GClosure* g_closure_ref				(GClosure	*closure);
void	  g_closure_sink			(GClosure	*closure);
void	  g_closure_unref			(GClosure	*closure);
/* intimidating */
GClosure* g_closure_new_simple			(guint		 sizeof_closure,
						 gpointer	 data);
void	  g_closure_add_finalize_notifier	(GClosure       *closure,
						 gpointer	 notify_data,
						 GClosureNotify	 notify_func);
void	  g_closure_remove_finalize_notifier	(GClosure       *closure,
						 gpointer	 notify_data,
						 GClosureNotify	 notify_func);
void	  g_closure_add_invalidate_notifier	(GClosure       *closure,
						 gpointer	 notify_data,
						 GClosureNotify	 notify_func);
void	  g_closure_remove_invalidate_notifier	(GClosure       *closure,
						 gpointer	 notify_data,
						 GClosureNotify	 notify_func);
void	  g_closure_add_marshal_guards		(GClosure	*closure,
						 gpointer        pre_marshal_data,
						 GClosureNotify	 pre_marshal_notify,
						 gpointer        post_marshal_data,
						 GClosureNotify	 post_marshal_notify);
void	  g_closure_set_marshal			(GClosure	*closure,
						 GClosureMarshal marshal);
void	  g_closure_set_meta_marshal		(GClosure       *closure,
						 gpointer	 marshal_data,
						 GClosureMarshal meta_marshal);
void	  g_closure_invalidate			(GClosure	*closure);
void	  g_closure_invoke			(GClosure 	*closure,
						 GValue	/*out*/	*return_value,
						 guint		 n_param_values,
						 const GValue	*param_values,
						 gpointer	 invocation_hint);

/* FIXME:
   OK:  data_object::destroy		-> closure_invalidate();
   MIS:	closure_invalidate()		-> disconnect(closure);
   MIS:	disconnect(closure)		-> (unlink) closure_unref();
   OK:	closure_finalize()		-> g_free (data_string);

   random remarks:
   - need marshaller repo with decent aliasing to base types
   - provide marshaller collection, virtually covering anything out there
*/

G_END_DECLS

#endif /* __G_CLOSURE_H__ */
