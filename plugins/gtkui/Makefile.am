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
				   widgets.c widgets.h\
				   ddbseekbar.h ddbequalizer.h ddbcellrenderertextmultiline.h\
				   ddbseekbar.c ddbequalizer.c ddbcellrenderertextmultiline.c\
				   gtkuigl.c gtkuigl.h\
                   hotkeys.c hotkeys.h\
                   actionhandlers.c actionhandlers.h\
                   pltmenu.c\
                   ctmapping.c ctmapping.h\
				   $(SM_SOURCES)

sdkdir = $(pkgincludedir)
sdk_HEADERS = gtkui_api.h

EXTRA_DIST = deadbeef.glade

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

# normal lib
if HAVE_GTK2

ddb_gui_GTK2_la_SOURCES = $(GTKUI_SOURCES)
ddb_gui_GTK2_la_LDFLAGS = -module

if STATICLINK
GTK_ROOT_216=@top_srcdir@/$(LIB)/gtk-2.16.0

ddb_gui_GTK2_la_LIBADD = $(LDADD) -L$(GTK_ROOT_216)/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 $(SM_LIBADD)

ddb_gui_GTK2_la_CFLAGS = -std=c99 -I$(GTK_ROOT_216)/include/gtk-2.0 -I$(GTK_ROOT_216)/lib/gtk-2.0/include -I$(GTK_ROOT_216)/include/atk-1.0 -I$(GTK_ROOT_216)/include/cairo -I$(GTK_ROOT_216)/include/pango-1.0 -I$(GTK_ROOT_216)/include -I$(GTK_ROOT_216)/include/glib-2.0 -I$(GTK_ROOT_216)/lib/glib-2.0/include $(SM_CFLAGS)
else
ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) -lGLU
ddb_gui_GTK2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS)
endif

endif

if HAVE_GTK3
GTKGLEXT_SOURCES = \
	gtkglext-gtk3/gtk/gtkglinit.c\
	gtkglext-gtk3/gtk/gtkglversion.c\
	gtkglext-gtk3/gtk/gtkglwidget.c\
	gtkglext-gtk3/gtk/gtkgldebug.h\
	gtkglext-gtk3/gtk/gtkgldefs.h\
	gtkglext-gtk3/gtk/gtkgl.h\
	gtkglext-gtk3/gtk/gtkglinit.h\
	gtkglext-gtk3/gtk/gtkglprivate.h\
	gtkglext-gtk3/gtk/gtkglwidget.h\
	gtkglext-gtk3/gdk/gdkglconfig.c\
	gtkglext-gtk3/gdk/gdkglconfigimpl.c\
	gtkglext-gtk3/gdk/gdkglcontext.c\
	gtkglext-gtk3/gdk/gdkglcontextimpl.c\
	gtkglext-gtk3/gdk/gdkgldrawable.c\
	gtkglext-gtk3/gdk/gdkglinit.c\
	gtkglext-gtk3/gdk/gdkglquery.c\
	gtkglext-gtk3/gdk/gdkglversion.c\
	gtkglext-gtk3/gdk/gdkglwindow.c\
	gtkglext-gtk3/gdk/gdkglwindowimpl.c\
	gtkglext-gtk3/gdk/gdkglconfig.h\
	gtkglext-gtk3/gdk/gdkglconfigimpl.h\
	gtkglext-gtk3/gdk/gdkglcontext.h\
	gtkglext-gtk3/gdk/gdkglcontextimpl.h\
	gtkglext-gtk3/gdk/gdkgldebug.h\
	gtkglext-gtk3/gdk/gdkgldefs.h\
	gtkglext-gtk3/gdk/gdkgldrawable.h\
	gtkglext-gtk3/gdk/gdkgl.h\
	gtkglext-gtk3/gdk/gdkglinit.h\
	gtkglext-gtk3/gdk/gdkglprivate.h\
	gtkglext-gtk3/gdk/gdkglquery.h\
	gtkglext-gtk3/gdk/gdkgltokens.h\
	gtkglext-gtk3/gdk/gdkgltypes.h\
	gtkglext-gtk3/gdk/gdkglwindow.h\
	gtkglext-gtk3/gdk/gdkglwindowimpl.h\
	gtkglext-gtk3/gdk/x11/gdkglconfig-x11.c\
	gtkglext-gtk3/gdk/x11/gdkglcontext-x11.c\
	gtkglext-gtk3/gdk/x11/gdkglquery-x11.c\
	gtkglext-gtk3/gdk/x11/gdkglwindow-x11.c\
	gtkglext-gtk3/gdk/x11/gdkx11glconfig.c\
	gtkglext-gtk3/gdk/x11/gdkx11glcontext.c\
	gtkglext-gtk3/gdk/x11/gdkx11glwindow.c\
	gtkglext-gtk3/gdk/x11/gdkglconfig-x11.h\
	gtkglext-gtk3/gdk/x11/gdkglcontext-x11.h\
	gtkglext-gtk3/gdk/x11/gdkglprivate-x11.h\
	gtkglext-gtk3/gdk/x11/gdkglquery-x11.h\
	gtkglext-gtk3/gdk/x11/gdkglwindow-x11.h\
	gtkglext-gtk3/gdk/x11/gdkglx.h\
	gtkglext-gtk3/gdk/x11/gdkx11glconfig.h\
	gtkglext-gtk3/gdk/x11/gdkx11glcontext.h\
	gtkglext-gtk3/gdk/x11/gdkx11glquery.h\
	gtkglext-gtk3/gdk/x11/gdkx11glwindow.h

GTKGLEXT_CFLAGS = -I@top_srcdir@/plugins/gtkui/gtkglext-gtk3 -I@top_srcdir@/plugins/gtkui/gtkglext-gtk3/gdk -DGTK_GL_COMPILATION -DGDK_GL_COMPILATION
GTKGLEXT_LIBS = -lGL

ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES) $(GTKGLEXT_SOURCES)
ddb_gui_GTK3_la_LDFLAGS = -module

if STATICLINK
GTK_ROOT_300=@top_srcdir@/$(LIB)/gtk-3.0.0

ddb_gui_GTK3_la_LIBADD = $(LDADD) -L$(GTK_ROOT_300)/lib -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo-gobject -lpango-1.0 -lcairo -lgobject-2.0 -lgthread-2.0 -lglib-2.0 -lfreetype -lfontconfig $(SM_LIBADD) $(GTKGLEXT_LIBS)

ddb_gui_GTK3_la_CFLAGS = -std=c99 -I$(GTK_ROOT_300)/include/gtk-3.0 -I$(GTK_ROOT_300)/include/pango-1.0 -I$(GTK_ROOT_300)/include/gio-unix-2.0/ -I$(GTK_ROOT_300)/include/atk-1.0 -I$(GTK_ROOT_300)/include/cairo -I$(GTK_ROOT_300)/include/gdk-pixbuf-2.0 -I$(GTK_ROOT_300)/include/freetype2 -I$(GTK_ROOT_300)/include/glib-2.0 -I$(GTK_ROOT_300)/lib/glib-2.0/include $(SM_CFLAGS) $(GTKGLEXT_CFLAGS)

else
ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD) $(GTKGLEXT_LIBS)
ddb_gui_GTK3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(GTKGLEXT_CFLAGS)
endif
endif

# fallback lib
if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

ddb_gui_GTK2_fallback_la_SOURCES = $(GTKUI_SOURCES)
ddb_gui_GTK2_fallback_la_LDFLAGS = -module

ddb_gui_GTK2_fallback_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib $(GTK_ROOT)/lib/libgtk-x11-2.0.la $(GTK_ROOT)/lib/libgdk-x11-2.0.la $(GTK_ROOT)/lib/libpangoft2-1.0.la $(GTK_ROOT)/lib/libpangocairo-1.0.la $(GTK_ROOT)/lib/libgdk_pixbuf-2.0.la -lm $(GTK_ROOT)/lib/libcairo.la $(GTK_ROOT)/lib/libpango-1.0.la $(GTK_ROOT)/lib/libgobject-2.0.la $(GTK_ROOT)/lib/libgmodule-2.0.la $(GTK_ROOT)/lib/libgthread-2.0.la -lrt $(GTK_ROOT)/lib/libglib-2.0.la $(SM_LIBADD)

ddb_gui_GTK2_fallback_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 $(SM_CFLAGS)

endif

