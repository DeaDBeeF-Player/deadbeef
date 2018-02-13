pluginpath=@top_srcdir@/plugins/shellexecui
EXTRA_DIST = $(pluginpath)/shellexec.glade

if HAVE_SHELLEXECUI

if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = shellexecui_gtk2.la shellexecui_gtk3.la
else
pkglib_LTLIBRARIES = shellexecui_gtk2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = shellexecui_gtk3.la
endif
endif

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

if HAVE_GTK2
shellexecui_gtk2_la_SOURCES = shellexecui.c interface.c support.c callbacks.c interface.h support.h callbacks.h
shellexecui_gtk2_la_LDFLAGS = -module -avoid-version
shellexecui_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
shellexecui_gtk2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
endif

if HAVE_GTK3
shellexecui_gtk3_la_SOURCES = shellexecui.c interface.c support.c callbacks.c interface.h support.h callbacks.h
shellexecui_gtk3_la_LDFLAGS = -module -avoid-version
shellexecui_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
shellexecui_gtk3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
endif


endif
