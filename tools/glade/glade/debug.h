/*  Gtk+ User Interface Builder
 *  Copyright (C) 1998-1999  Damon Chaplin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#ifndef GLADE_DEBUG_H
#define GLADE_DEBUG_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* These allow debugging flags to be passed into Glade via the GLADE_DEBUG
   environment variable. This is useful as you can change options without
   recompiling Glade. Currently the only options supported are:
     'warnings'   - will cause Glade to abort() on any WARNING messages.
                    This only works with GTK 1.1.x.
     'messages'   - causes Glade to output debugging messages (though you must
                    configure Glade with --enable-debug to use this.
     'properties' - will cause Glade to create all widget properties on
		    start-up, and output info about them to stdout.
   To use these, set GLADE_DEBUG to the option strings, with each option
   separated by a ':'. e.g. export GLADE_DEBUG='warnings:messages'. */
extern guint glade_debug_flags;
extern gboolean glade_debug_messages;
extern gboolean glade_debug_properties;

typedef enum {
  GLADE_DEBUG_WARNINGS   = 1 << 0,
  GLADE_DEBUG_MESSAGES   = 1 << 1,
  GLADE_DEBUG_PROPERTIES = 1 << 2
} GladeDebugFlag;


/* Debugging macros. These output messages if glade is compiled with
   --enable-debug, and the 'messages' debugging option is on (i.e. the
   'GLADE_DEBUG' environment variable contains 'messages'. The advantage of
   this is that you can turn the messages on/off without recompiling -
   just change 'GLADE_DEBUG'. */
#ifdef GLADE_DEBUG
#define MSGIN if (glade_debug_messages) \
	g_print("%s:%i: IN %s\n", __FILE__, __LINE__, G_GNUC_PRETTY_FUNCTION)
#define MSGOUT if (glade_debug_messages) \
	g_print("%s:%i: OUT %s\n", __FILE__, __LINE__, G_GNUC_PRETTY_FUNCTION)
#define MSG(message) if (glade_debug_messages) \
	g_print("%s:%i: %s\n", __FILE__, __LINE__, message)
#define MSG1(message, arg) if (glade_debug_messages) \
      g_print("%s:%i: " message "\n", __FILE__, __LINE__, arg)
#define MSG2(message, arg, arg2) if (glade_debug_messages) \
      g_print("%s:%i: " message "\n", __FILE__, __LINE__, arg, arg2)
#define MSG3(message, arg, arg2, arg3) if (glade_debug_messages) \
      g_print("%s:%i: " message "\n", __FILE__, __LINE__, arg, arg2, arg3)
#define MSG4(message, arg, arg2, arg3, arg4) if (glade_debug_messages) \
      g_print("%s:%i: " message "\n", __FILE__, __LINE__, arg, arg2, arg3, arg4)
#else
#define MSGIN
#define MSGOUT
#define MSG(message)
#define MSG1(message, arg)
#define MSG2(message, arg, arg2)
#define MSG3(message, arg, arg2, arg3)
#define MSG4(message, arg, arg2, arg3, arg4)
#endif


void glade_debug_init ();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* GLADE_DEBUG_H */
