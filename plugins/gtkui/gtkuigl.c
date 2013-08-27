/*
    DeaDBeeF - ultimate music player for GNU/Linux systems with X11
    Copyright (C) 2009-2012 Alexey Yakovenko <waker@users.sourceforge.net>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "../../deadbeef.h"
#include "gtkui.h"
#include "support.h"
#include "gtkuigl.h"

#if 0
static int gl_initialized;
static int gl_init_state;
//PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI;

int
gtkui_gl_init (void) {
    if (gl_initialized) {
        return gl_init_state;
    }
    gl_initialized = 1;
    int argc = 1;
    const char **argv = alloca (sizeof (char *) * argc);
    argv[0] = "deadbeef";
    gboolean success = gdk_gl_init_check (&argc, (char ***)&argv);
    if (!success) {
        fprintf (stderr, "gdk_gl_init_check failed\n");
        gl_init_state = -1;
        return -1;
    }
//    glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGIPROC) glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
    fprintf (stderr, "gdk_gl_init_check success\n");
    gl_init_state = 0;
    return 0;
}

void
gtkui_gl_free (void) {
    // ???
}
#endif
