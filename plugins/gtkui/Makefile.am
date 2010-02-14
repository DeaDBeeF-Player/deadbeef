if HAVE_GTK
gtkuidir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = gtkui.la
gtkui_la_SOURCES = gtkui.c gtkui.h\
				   callbacks.c interface.c support.c callbacks.h interface.h support.h\
				   ddblistview.c ddblistview.h\
				   drawing.h gdkdrawing.c\
				   progress.c progress.h\
				   search.c search.h\
				   fileman.c\
				   pluginconf.c\
				   parser.c parser.h\
				   tabs.c tabs.h

gtkui_la_LDFLAGS = -module

gtkui_la_LIBADD = $(LDADD) $(GTKUI_DEPS_LIBS) $(NOTIFY_DEPS_LIBS)
AM_CFLAGS = -std=c99 $(GTKUI_DEPS_CFLAGS) $(NOTIFY_DEPS_CFLAGS)
endif
