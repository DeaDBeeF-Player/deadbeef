
#ifndef ___gdk_pixbuf_marshal_MARSHAL_H__
#define ___gdk_pixbuf_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:VOID (/tmp/buildd/gtk+2.0-2.12.12/gdk-pixbuf/gdk-pixbuf-marshal.list:25) */
#define _gdk_pixbuf_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

/* VOID:INT,INT,INT,INT (/tmp/buildd/gtk+2.0-2.12.12/gdk-pixbuf/gdk-pixbuf-marshal.list:26) */
extern void _gdk_pixbuf_marshal_VOID__INT_INT_INT_INT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* VOID:POINTER (/tmp/buildd/gtk+2.0-2.12.12/gdk-pixbuf/gdk-pixbuf-marshal.list:27) */
#define _gdk_pixbuf_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* VOID:INT,INT (/tmp/buildd/gtk+2.0-2.12.12/gdk-pixbuf/gdk-pixbuf-marshal.list:28) */
extern void _gdk_pixbuf_marshal_VOID__INT_INT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

G_END_DECLS

#endif /* ___gdk_pixbuf_marshal_MARSHAL_H__ */

