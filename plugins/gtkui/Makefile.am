if HAVE_GTKUI
gtkuidir = $(libdir)/$(PACKAGE)

if HAVE_SM
if HAVE_ICE
SM_SOURCES = smclient/eggdesktopfile.h\
	smclient/eggsmclient-private.h\
	smclient/eggsmclient.h\
	smclient/eggdesktopfile.c\
	smclient/eggsmclient-xsmp.c\
	smclient/eggsmclient.c
SM_LIBADD = $(SM_LIBS) $(ICE_LIBS)
SM_CFLAGS = -DEGG_SM_CLIENT_BACKEND_XSMP -DG_LOG_DOMAIN=\""deadbeef"\"
endif
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
				   dspconfig.c dspconfig.h\
				   tagwritersettings.c tagwritersettings.h\
				   wingeom.c wingeom.h\
				   pluginconf.h\
				   ddbseekbar.h ddbequalizer.h ddbcellrenderertextmultiline.h\
				   ddbseekbar.c ddbequalizer.c ddbcellrenderertextmultiline.c\
				   $(SM_SOURCES)



sdkdir = $(pkgincludedir)
sdk_HEADERS = gtkui_api.h

EXTRA_DIST = deadbeef.glade

if STATICLINK
pkglib_LTLIBRARIES = ddb_gui_GTK2.la ddb_gui_GTK2.fallback.la
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

# normal lib
if HAVE_GTK2
ddb_gui_GTK2_la_SOURCES = $(GTKUI_SOURCES)
ddb_gui_GTK2_la_LDFLAGS = -module
ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD)
ddb_gui_GTK2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS)
endif
if HAVE_GTK3
ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES)
ddb_gui_GTK3_la_LDFLAGS = -module
ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD)
ddb_gui_GTK3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS)
endif

# fallback lib
if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

ddb_gui_GTK2_fallback_la_SOURCES = $(GTKUI_SOURCES)
ddb_gui_GTK2_fallback_la_LDFLAGS = -module

ddb_gui_GTK2_fallback_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib $(GTK_ROOT)/lib/libgtk-x11-2.0.la $(GTK_ROOT)/lib/libgdk-x11-2.0.la $(GTK_ROOT)/lib/libpangoft2-1.0.la $(GTK_ROOT)/lib/libpangocairo-1.0.la $(GTK_ROOT)/lib/libgdk_pixbuf-2.0.la -lm $(GTK_ROOT)/lib/libcairo.la $(GTK_ROOT)/lib/libpango-1.0.la $(GTK_ROOT)/lib/libgobject-2.0.la $(GTK_ROOT)/lib/libgmodule-2.0.la $(GTK_ROOT)/lib/libgthread-2.0.la -lrt $(GTK_ROOT)/lib/libglib-2.0.la $(SM_LIBADD)

ddb_gui_GTK2_fallback_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1 $(SM_CFLAGS)

endif

endif
