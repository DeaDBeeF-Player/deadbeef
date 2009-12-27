if HAVE_GTK
gtkuidir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = gtkui.la
gtkui_la_SOURCES = gtkui.c gtkui.h\
				   callbacks.c interface.c support.c callbacks.h interface.h support.h\
				   gtkplaylist.c gtkplaylist.h\
				   drawing.h gdkdrawing.c\
				   progress.c progress.h\
				   search.c search.h\
				   fileman.c\
				   parser.c parser.h

gtkui_la_LDFLAGS = -module

gtkui_la_LIBADD = $(LDADD) $(GTKUI_DEPS_LIBS)
AM_CFLAGS = -std=c99 $(GTKUI_DEPS_CFLAGS)
endif
