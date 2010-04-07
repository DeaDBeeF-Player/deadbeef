if HAVE_GTK
gtkuidir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = gtkui.la

gtkui_VALASOURCES = ddbequalizer.vala
gtkui_VALABUILTSOURCES = $(gtkui_VALASOURCES:.vala=.c) ddbequalizer.h

if MAINTAINER_MODE
BUILT_SOURCES = vala.stamp
vala.stamp: $(gtkui_VALASOURCES)
	$(VALAC)  -C -H ddbequalizer.h --library ddbequalizer gtkui.vapi --pkg=gtk+-2.0 $^
	touch $@
endif


gtkui_la_SOURCES = $(gtkui_VALABUILTSOURCES)\
				   gtkui.c gtkui.h\
				   callbacks.c interface.c support.c callbacks.h interface.h support.h\
				   ddblistview.c ddblistview.h\
				   mainplaylist.c\
				   drawing.h gdkdrawing.c\
				   progress.c progress.h\
				   search.c search.h\
				   fileman.c\
				   pluginconf.c\
				   parser.c parser.h\
				   ddbtabstrip.c ddbtabstrip.h\
				   ddbvolumebar.c ddbvolumebar.h\
				   trkproperties.c trkproperties.h\
				   coverart.c coverart.h\
				   plcommon.c plcommon.h\
				   prefwin.c\
				   eq.c eq.h

gtkui_la_LDFLAGS = -module

gtkui_la_LIBADD = $(LDADD) $(GTKUI_DEPS_LIBS) $(NOTIFY_DEPS_LIBS)
AM_CFLAGS = -std=c99 $(GTKUI_DEPS_CFLAGS) $(NOTIFY_DEPS_CFLAGS)

EXTRA_DIST = $(gtkui_VALASOURCES)

if MAINTAINER_MODE
CLEANFILES =								\
	$(BUILT_SOURCES)						\
	$(gtkui_VALABUILTSOURCES)
endif
endif
