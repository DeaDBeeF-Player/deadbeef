#ifndef GTK_DISABLE_DEPRECATED

#ifndef __gtk_marshal_MARSHAL_H__
#define __gtk_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOL:NONE (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:1) */
extern void gtk_marshal_BOOLEAN__VOID (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);
#define gtk_marshal_BOOL__NONE	gtk_marshal_BOOLEAN__VOID

/* BOOL:POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:2) */
extern void gtk_marshal_BOOLEAN__POINTER (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER	gtk_marshal_BOOLEAN__POINTER

/* BOOL:POINTER,POINTER,INT,INT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:3) */
extern void gtk_marshal_BOOLEAN__POINTER_POINTER_INT_INT (GClosure     *closure,
                                                          GValue       *return_value,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint,
                                                          gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_POINTER_INT_INT	gtk_marshal_BOOLEAN__POINTER_POINTER_INT_INT

/* BOOL:POINTER,INT,INT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:4) */
extern void gtk_marshal_BOOLEAN__POINTER_INT_INT (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_INT_INT	gtk_marshal_BOOLEAN__POINTER_INT_INT

/* BOOL:POINTER,INT,INT,UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:5) */
extern void gtk_marshal_BOOLEAN__POINTER_INT_INT_UINT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_INT_INT_UINT	gtk_marshal_BOOLEAN__POINTER_INT_INT_UINT

/* BOOL:POINTER,STRING,STRING,POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:6) */
extern void gtk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER (GClosure     *closure,
                                                                GValue       *return_value,
                                                                guint         n_param_values,
                                                                const GValue *param_values,
                                                                gpointer      invocation_hint,
                                                                gpointer      marshal_data);
#define gtk_marshal_BOOL__POINTER_STRING_STRING_POINTER	gtk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER

/* ENUM:ENUM (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:7) */
extern void gtk_marshal_ENUM__ENUM (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data);

/* INT:POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:8) */
extern void gtk_marshal_INT__POINTER (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

/* INT:POINTER,CHAR,CHAR (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:9) */
extern void gtk_marshal_INT__POINTER_CHAR_CHAR (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* NONE:BOOL (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:10) */
#define gtk_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN
#define gtk_marshal_NONE__BOOL	gtk_marshal_VOID__BOOLEAN

/* NONE:BOXED (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:11) */
#define gtk_marshal_VOID__BOXED	g_cclosure_marshal_VOID__BOXED
#define gtk_marshal_NONE__BOXED	gtk_marshal_VOID__BOXED

/* NONE:ENUM (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:12) */
#define gtk_marshal_VOID__ENUM	g_cclosure_marshal_VOID__ENUM
#define gtk_marshal_NONE__ENUM	gtk_marshal_VOID__ENUM

/* NONE:ENUM,FLOAT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:13) */
extern void gtk_marshal_VOID__ENUM_FLOAT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);
#define gtk_marshal_NONE__ENUM_FLOAT	gtk_marshal_VOID__ENUM_FLOAT

/* NONE:ENUM,FLOAT,BOOL (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:14) */
extern void gtk_marshal_VOID__ENUM_FLOAT_BOOLEAN (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define gtk_marshal_NONE__ENUM_FLOAT_BOOL	gtk_marshal_VOID__ENUM_FLOAT_BOOLEAN

/* NONE:INT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:15) */
#define gtk_marshal_VOID__INT	g_cclosure_marshal_VOID__INT
#define gtk_marshal_NONE__INT	gtk_marshal_VOID__INT

/* NONE:INT,INT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:16) */
extern void gtk_marshal_VOID__INT_INT (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);
#define gtk_marshal_NONE__INT_INT	gtk_marshal_VOID__INT_INT

/* NONE:INT,INT,POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:17) */
extern void gtk_marshal_VOID__INT_INT_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtk_marshal_NONE__INT_INT_POINTER	gtk_marshal_VOID__INT_INT_POINTER

/* NONE:NONE (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:18) */
#define gtk_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID
#define gtk_marshal_NONE__NONE	gtk_marshal_VOID__VOID

/* NONE:OBJECT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:19) */
#define gtk_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT
#define gtk_marshal_NONE__OBJECT	gtk_marshal_VOID__OBJECT

/* NONE:POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:20) */
#define gtk_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER
#define gtk_marshal_NONE__POINTER	gtk_marshal_VOID__POINTER

/* NONE:POINTER,INT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:21) */
extern void gtk_marshal_VOID__POINTER_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_INT	gtk_marshal_VOID__POINTER_INT

/* NONE:POINTER,POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:22) */
extern void gtk_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_POINTER	gtk_marshal_VOID__POINTER_POINTER

/* NONE:POINTER,POINTER,POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:23) */
extern void gtk_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_POINTER_POINTER	gtk_marshal_VOID__POINTER_POINTER_POINTER

/* NONE:POINTER,STRING,STRING (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:24) */
extern void gtk_marshal_VOID__POINTER_STRING_STRING (GClosure     *closure,
                                                     GValue       *return_value,
                                                     guint         n_param_values,
                                                     const GValue *param_values,
                                                     gpointer      invocation_hint,
                                                     gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_STRING_STRING	gtk_marshal_VOID__POINTER_STRING_STRING

/* NONE:POINTER,UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:25) */
extern void gtk_marshal_VOID__POINTER_UINT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_UINT	gtk_marshal_VOID__POINTER_UINT

/* NONE:POINTER,UINT,ENUM (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:26) */
extern void gtk_marshal_VOID__POINTER_UINT_ENUM (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_UINT_ENUM	gtk_marshal_VOID__POINTER_UINT_ENUM

/* NONE:POINTER,POINTER,UINT,UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:27) */
extern void gtk_marshal_VOID__POINTER_POINTER_UINT_UINT (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_POINTER_UINT_UINT	gtk_marshal_VOID__POINTER_POINTER_UINT_UINT

/* NONE:POINTER,INT,INT,POINTER,UINT,UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:28) */
extern void gtk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT (GClosure     *closure,
                                                                 GValue       *return_value,
                                                                 guint         n_param_values,
                                                                 const GValue *param_values,
                                                                 gpointer      invocation_hint,
                                                                 gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_INT_INT_POINTER_UINT_UINT	gtk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT

/* NONE:POINTER,UINT,UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:29) */
extern void gtk_marshal_VOID__POINTER_UINT_UINT (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);
#define gtk_marshal_NONE__POINTER_UINT_UINT	gtk_marshal_VOID__POINTER_UINT_UINT

/* NONE:POINTER,UINT,UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:30) */

/* NONE:STRING (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:31) */
#define gtk_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING
#define gtk_marshal_NONE__STRING	gtk_marshal_VOID__STRING

/* NONE:STRING,INT,POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:32) */
extern void gtk_marshal_VOID__STRING_INT_POINTER (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);
#define gtk_marshal_NONE__STRING_INT_POINTER	gtk_marshal_VOID__STRING_INT_POINTER

/* NONE:UINT (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:33) */
#define gtk_marshal_VOID__UINT	g_cclosure_marshal_VOID__UINT
#define gtk_marshal_NONE__UINT	gtk_marshal_VOID__UINT

/* NONE:UINT,POINTER,UINT,ENUM,ENUM,POINTER (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:34) */
extern void gtk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER (GClosure     *closure,
                                                                   GValue       *return_value,
                                                                   guint         n_param_values,
                                                                   const GValue *param_values,
                                                                   gpointer      invocation_hint,
                                                                   gpointer      marshal_data);
#define gtk_marshal_NONE__UINT_POINTER_UINT_ENUM_ENUM_POINTER	gtk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER

/* NONE:UINT,POINTER,UINT,UINT,ENUM (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:35) */
extern void gtk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data);
#define gtk_marshal_NONE__UINT_POINTER_UINT_UINT_ENUM	gtk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM

/* NONE:UINT,STRING (/build/buildd-gtk+2.0_2.12.12-1~lenny2-i386-2RfKoO/gtk+2.0-2.12.12/gtk/gtkmarshal.list:36) */
extern void gtk_marshal_VOID__UINT_STRING (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);
#define gtk_marshal_NONE__UINT_STRING	gtk_marshal_VOID__UINT_STRING

G_END_DECLS

#endif /* __gtk_marshal_MARSHAL_H__ */

#endif /* GTK_DISABLE_DEPRECATED */
