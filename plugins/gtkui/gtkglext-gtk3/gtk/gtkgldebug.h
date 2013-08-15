/* GtkGLExt - OpenGL Extension to GTK+
 * Copyright (C) 2002-2004  Naofumi Yasufuku
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.
 */

#ifndef __GTK_GL_DEBUG_H__
#define __GTK_GL_DEBUG_H__

#include <glib.h>

#include <gtk/gtkgldefs.h>

G_BEGIN_DECLS

typedef enum {
  GTK_GL_DEBUG_MISC = 1 << 0,
  GTK_GL_DEBUG_FUNC = 1 << 1
} GtkGLDebugFlag;

#ifdef G_ENABLE_DEBUG

#define GTK_GL_NOTE(type, action)               G_STMT_START {  \
  if (gtk_gl_debug_flags & GTK_GL_DEBUG_##type)                 \
    { action; };                                } G_STMT_END

#if __STDC_VERSION__ < 199901L
#  if __GNUC__ >= 2
#    define __func__ __FUNCTION__
#  else
#    define __func__ "<unknown>"
#  endif
#endif

#define GTK_GL_NOTE_FUNC()                      G_STMT_START {  \
  if (gtk_gl_debug_flags & GTK_GL_DEBUG_FUNC)                   \
    g_message (" - %s ()", __func__);           } G_STMT_END

#define GTK_GL_NOTE_FUNC_PRIVATE()              G_STMT_START {  \
  if (gtk_gl_debug_flags & GTK_GL_DEBUG_FUNC)                   \
    g_message (" -- %s ()", __func__);          } G_STMT_END

#else /* !G_ENABLE_DEBUG */

#define GTK_GL_NOTE(type, action)
#define GTK_GL_NOTE_FUNC()
#define GTK_GL_NOTE_FUNC_PRIVATE()

#endif /* G_ENABLE_DEBUG */

GTK_GL_VAR guint gtk_gl_debug_flags;

G_END_DECLS

#endif /* __GTK_GL_DEBUG_H__ */
