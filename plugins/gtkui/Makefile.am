EXTRA_DIST = deadbeef.glade

if OS_OSX
osx_sources = retina.m
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
				   ddbtabstrip.c ddbtabstrip.h\
				   ddbvolumebar.c ddbvolumebar.h\
				   trkproperties.c trkproperties.h\
				   coverart.c coverart.h\
				   plcommon.c plcommon.h\
				   prefwin.c\
				   eq.c eq.h\
				   actions.c actions.h\
				   dspconfig.c dspconfig.h\
				   tagwritersettings.c tagwritersettings.h\
				   wingeom.c wingeom.h\
				   pluginconf.h\
				   widgets.c widgets.h\
				   ddbseekbar.h ddbequalizer.h ddbcellrenderertextmultiline.h\
				   ddbseekbar.c ddbequalizer.c ddbcellrenderertextmultiline.c\
				   gtkuigl.c gtkuigl.h\
                   hotkeys.c hotkeys.h\
                   actionhandlers.c actionhandlers.h\
                   pltmenu.c\
                   ctmapping.c ctmapping.h\
                   namedicons.c namedicons.h\
                   tfimport.c tfimport.h\
				   $(SM_SOURCES) $(OSXSRC)

sdkdir = $(pkgincludedir)
sdk_HEADERS = gtkui_api.h

if STATICLINK
if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = ddb_gui_GTK2.la ddb_gui_GTK2.fallback.la ddb_gui_GTK3.la
else
pkglib_LTLIBRARIES = ddb_gui_GTK2.la ddb_gui_GTK2.fallback.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = ddb_gui_GTK3.la
endif
endif
else
if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = ddb_gui_GTK2.la ddb_gui_GTK3.la
else
pkglib_LTLIBRARIES = ddb_gui_GTK2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = ddb_gui_GTK3.la
endif
endif
endif

if HAVE_GTK2
GTKUI_SOURCES_GTK2 = $(GTKUI_SOURCES)

ddb_gui_GTK2_la_SOURCES = $(GTKUI_SOURCES_GTK2)
ddb_gui_GTK2_la_LDFLAGS = -module -avoid-version

ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.a $(JANSSON_LIBS)

ddb_gui_GTK2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS) -DDDB_WARN_DEPRECATED=1

endif

if HAVE_GTK3
GTKUI_SOURCES_GTK3 = $(GTKUI_SOURCES) $(osx_sources)

ddb_gui_GTK3_la_LDFLAGS = -module -avoid-version

ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES_GTK3)
ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.a $(JANSSON_LIBS)
ddb_gui_GTK3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS)
ddb_gui_GTK3_la_OBJCFLAGS = $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS)

endif

# fallback lib
if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

ddb_gui_GTK2_fallback_la_SOURCES = $(GTKUI_SOURCES_GTK2)
ddb_gui_GTK2_fallback_la_LDFLAGS = -module -avoid-version

ddb_gui_GTK2_fallback_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lgthread-2.0 -lrt -lglib-2.0 $(SM_LIBADD) ../libparser/libparser.a $(JANSSON_LIBS)

ddb_gui_GTK2_fallback_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 $(SM_CFLAGS) $(JANSSON_CFLAGS)

endif

