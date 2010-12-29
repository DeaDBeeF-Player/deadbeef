if HAVE_GTKUI
gtkuidir = $(libdir)/$(PACKAGE)
gtkui_VALASOURCES = ddbequalizer.vala ddbseekbar.vala ddbcellrenderertextmultiline.vala
gtkui_VALABUILTSOURCES = $(gtkui_VALASOURCES:.vala=.c) ddbequalizer.h ddbseekbar.h ddbcellrenderertextmultiline.h
if MAINTAINER_MODE
BUILT_SOURCES = vala.stamp
vala.stamp: $(gtkui_VALASOURCES)
	$(VALAC)  -C -H ddbcellrenderertextmultiline.h --library ddbcellrenderertextmultiline --pkg=gtk+-2.0 gdk-keysyms.vapi ddbcellrenderertextmultiline.vala
	$(VALAC)  -C -H ddbequalizer.h --library ddbequalizer gtkui.vapi --pkg=gtk+-2.0 ddbequalizer.vala
	$(VALAC)  -C -H ddbseekbar.h --library ddbseekbar gtkui.vapi --pkg=gtk+-2.0 ddbseekbar.vala
	touch $@

CLEANFILES =								\
	$(BUILT_SOURCES)						\
	$(gtkui_VALABUILTSOURCES)
endif

GTKUI_SOURCES =    gtkui.c gtkui.h\
				   callbacks.c interface.c support.c callbacks.h interface.h support.h\
				   ddblistview.c ddblistview.h\
				   mainplaylist.c mainplaylist.h\
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
				   eq.c eq.h\
				   actions.c actions.h\
				   dspconfig.c dspconfig.h

EXTRA_DIST = $(gtkui_VALASOURCES) deadbeef.glade

if STATICLINK
pkglib_LTLIBRARIES = gtkui.la gtkui.fallback.la
else
pkglib_LTLIBRARIES = gtkui.la
endif

# normal lib
gtkui_la_SOURCES = $(gtkui_VALABUILTSOURCES) $(GTKUI_SOURCES)
gtkui_la_LDFLAGS = -module
gtkui_la_LIBADD = $(LDADD) $(GTKUI_DEPS_LIBS)
gtkui_la_CFLAGS = -std=c99 $(GTKUI_DEPS_CFLAGS)

# fallback lib
if STATICLINK
GTK_ROOT=../../../deadbeef-deps/gtk-debian/usr

gtkui_fallback_la_SOURCES = $(gtkui_VALABUILTSOURCES) $(GTKUI_SOURCES)
gtkui_fallback_la_LDFLAGS = -module

gtkui_fallback_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib $(GTK_ROOT)/lib/libgtk-x11-2.0.la $(GTK_ROOT)/lib/libgdk-x11-2.0.la $(GTK_ROOT)/lib/libpangoft2-1.0.la $(GTK_ROOT)/lib/libpangocairo-1.0.la $(GTK_ROOT)/lib/libgdk_pixbuf-2.0.la -lm $(GTK_ROOT)/lib/libcairo.la $(GTK_ROOT)/lib/libpango-1.0.la $(GTK_ROOT)/lib/libgobject-2.0.la $(GTK_ROOT)/lib/libgmodule-2.0.la $(GTK_ROOT)/lib/libgthread-2.0.la -lrt $(GTK_ROOT)/lib/libglib-2.0.la

gtkui_fallback_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1

endif

endif
