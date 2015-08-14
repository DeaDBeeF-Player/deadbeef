/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2015 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include "../../deadbeef.h"
#include "gtkui.h"
#include "support.h"
#ifdef __APPLE__
#undef USE_OPENGL
#endif

#ifdef USE_OPENGL
#include "gtkuigl.h"

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
