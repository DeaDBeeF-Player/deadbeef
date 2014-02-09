gtkuidir = $(libdir)/$(PACKAGE)

#if HAVE_SM
#if HAVE_ICE
#SM_SOURCES = smclient/eggdesktopfile.h\
#	smclient/eggsmclient-private.h\
#	smclient/eggsmclient.h\
#	smclient/eggdesktopfile.c\
#	smclient/eggsmclient-xsmp.c\
#	smclient/eggsmclient.c
#SM_LIBADD = $(SM_LIBS) $(ICE_LIBS)
#SM_CFLAGS = -DEGG_SM_CLIENT_BACKEND_XSMP -DG_LOG_DOMAIN=\""deadbeef"\"
#endif
#endif

#if OS_OSX
#OSXSRC = retina.m
#endif

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
				   $(SM_SOURCES) $(OSXSRC)

sdkdir = $(pkgincludedir)
sdk_HEADERS = gtkui_api.h

EXTRA_DIST = deadbeef.glade

#GTKGLEXT2_LIBS =
#GTKGLEXT3_LIBS =

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
GTKUI_SOURCES_GTK2 = $(GTKUI_SOURCES)

#	gtkglext-gtk2/gdk/x11/gdkglconfig-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkglconfig-x11.h\
#	gtkglext-gtk2/gdk/x11/gdkglcontext-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkglcontext-x11.h\
#	gtkglext-gtk2/gdk/x11/gdkgldrawable-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkglglxext.c\
#	gtkglext-gtk2/gdk/x11/gdkglglxext.h\
#	gtkglext-gtk2/gdk/x11/gdkgloverlay-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkgloverlay-x11.h\
#	gtkglext-gtk2/gdk/x11/gdkglpixmap-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkglpixmap-x11.h\
#	gtkglext-gtk2/gdk/x11/gdkglprivate-x11.h\
#	gtkglext-gtk2/gdk/x11/gdkglquery-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkglwindow-x11.c\
#	gtkglext-gtk2/gdk/x11/gdkglwindow-x11.h\
#	gtkglext-gtk2/gdk/x11/gdkglx.h\
#	gtkglext-gtk2/gtk/gtkgldebug.h\
#	gtkglext-gtk2/gtk/gtkgldefs.h\
#	gtkglext-gtk2/gtk/gtkgl.h\
#	gtkglext-gtk2/gtk/gtkglinit.c\
#	gtkglext-gtk2/gtk/gtkglinit.h\
#	gtkglext-gtk2/gtk/gtkglprivate.h\
#	gtkglext-gtk2/gtk/gtkglversion.c\
#	gtkglext-gtk2/gtk/gtkglversion.h\
#	gtkglext-gtk2/gtk/gtkglwidget.c\
#	gtkglext-gtk2/gtk/gtkglwidget.h\
#	gtkglext-gtk2/gdk/gdkglconfig.c\
#	gtkglext-gtk2/gdk/gdkglconfig.h\
#	gtkglext-gtk2/gdk/gdkglcontext.c\
#	gtkglext-gtk2/gdk/gdkglcontext.h\
#	gtkglext-gtk2/gdk/gdkgldebug.h\
#	gtkglext-gtk2/gdk/gdkgldefs.h\
#	gtkglext-gtk2/gdk/gdkgldrawable.c\
#	gtkglext-gtk2/gdk/gdkgldrawable.h\
#	gtkglext-gtk2/gdk/gdkglenumtypes.c\
#	gtkglext-gtk2/gdk/gdkglenumtypes.h\
#	gtkglext-gtk2/gdk/gdkglext-config.h\
#	gtkglext-gtk2/gdk/gdkglglext.c\
#	gtkglext-gtk2/gdk/gdkglglext.h\
#	gtkglext-gtk2/gdk/gdkgl.h\
#	gtkglext-gtk2/gdk/gdkglinit.c\
#	gtkglext-gtk2/gdk/gdkglinit.h\
#	gtkglext-gtk2/gdk/gdkglpixmap.c\
#	gtkglext-gtk2/gdk/gdkglpixmap.h\
#	gtkglext-gtk2/gdk/gdkglprivate.h\
#	gtkglext-gtk2/gdk/gdkglquery.c\
#	gtkglext-gtk2/gdk/gdkglquery.h\
#	gtkglext-gtk2/gdk/gdkglshapes.c\
#	gtkglext-gtk2/gdk/gdkglshapes.h\
#	gtkglext-gtk2/gdk/gdkgltokens.h\
#	gtkglext-gtk2/gdk/gdkgltypes.h\
#	gtkglext-gtk2/gdk/gdkglversion.c\
#	gtkglext-gtk2/gdk/gdkglversion.h\
#	gtkglext-gtk2/gdk/gdkglwindow.c\
#	gtkglext-gtk2/gdk/gdkglwindow.h\
#	gtkglext-gtk2/gdk/gdkglfont.h\
#	gtkglext-gtk2/gdk/glext/common-header.h\
#	gtkglext-gtk2/gdk/glext/glext-extra.h\
#	gtkglext-gtk2/gdk/glext/glext.h\
#	gtkglext-gtk2/gdk/glext/glxext-extra.h\
#	gtkglext-gtk2/gdk/glext/glxext.h

#GTKGLEXT_CFLAGS_GTK2 = -I@top_srcdir@/plugins/gtkui/gtkglext-gtk2 -I@top_srcdir@/plugins/gtkui/gtkglext-gtk2/gdk -DGTK_GL_COMPILATION -DGDK_GL_COMPILATION

ddb_gui_GTK2_la_SOURCES = $(GTKUI_SOURCES_GTK2)
ddb_gui_GTK2_la_LDFLAGS = -module -avoid-version

if STATICLINK
GTK_ROOT_216=@top_srcdir@/$(LIB)/gtk-2.16.0

ddb_gui_GTK2_la_LIBADD = $(LDADD) -L$(GTK_ROOT_216)/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lcairo -lpango-1.0 -lfreetype -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 $(SM_LIBADD) ../libparser/libparser.a $(GTKGLEXT2_LIBS)

ddb_gui_GTK2_la_CFLAGS = -std=c99 -I$(GTK_ROOT_216)/include/gtk-2.0 -I$(GTK_ROOT_216)/lib/gtk-2.0/include -I$(GTK_ROOT_216)/include/atk-1.0 -I$(GTK_ROOT_216)/include/cairo -I$(GTK_ROOT_216)/include/pango-1.0 -I$(GTK_ROOT_216)/include -I$(GTK_ROOT_216)/include/glib-2.0 -I$(GTK_ROOT_216)/lib/glib-2.0/include $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK2)
else
ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.a  $(GTKGLEXT2_LIBS)

ddb_gui_GTK2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK2) -DDDB_WARN_DEPRECATED=1
endif

endif

if HAVE_GTK3
GTKUI_SOURCES_GTK3 = $(GTKUI_SOURCES)

#	gtkglext-gtk3/gtk/gtkglinit.c\
#	gtkglext-gtk3/gtk/gtkglversion.c\
#	gtkglext-gtk3/gtk/gtkglwidget.c\
#	gtkglext-gtk3/gtk/gtkgldebug.h\
#	gtkglext-gtk3/gtk/gtkgldefs.h\
#	gtkglext-gtk3/gtk/gtkgl.h\
#	gtkglext-gtk3/gtk/gtkglinit.h\
#	gtkglext-gtk3/gtk/gtkglprivate.h\
#	gtkglext-gtk3/gtk/gtkglversion.h\
#	gtkglext-gtk3/gtk/gtkglwidget.h\
#	gtkglext-gtk3/gdk/gdkglconfig.c\
#	gtkglext-gtk3/gdk/gdkglconfigimpl.c\
#	gtkglext-gtk3/gdk/gdkglcontext.c\
#	gtkglext-gtk3/gdk/gdkglcontextimpl.c\
#	gtkglext-gtk3/gdk/gdkgldrawable.c\
#	gtkglext-gtk3/gdk/gdkglinit.c\
#	gtkglext-gtk3/gdk/gdkglquery.c\
#	gtkglext-gtk3/gdk/gdkglversion.c\
#	gtkglext-gtk3/gdk/gdkglwindow.c\
#	gtkglext-gtk3/gdk/gdkglwindowimpl.c\
#	gtkglext-gtk3/gdk/gdkglconfig.h\
#	gtkglext-gtk3/gdk/gdkglconfigimpl.h\
#	gtkglext-gtk3/gdk/gdkglcontext.h\
#	gtkglext-gtk3/gdk/gdkglcontextimpl.h\
#	gtkglext-gtk3/gdk/gdkgldebug.h\
#	gtkglext-gtk3/gdk/gdkgldefs.h\
#	gtkglext-gtk3/gdk/gdkgldrawable.h\
#	gtkglext-gtk3/gdk/gdkglenumtypes.h\
#	gtkglext-gtk3/gdk/gdkglext-config.h\
#	gtkglext-gtk3/gdk/gdkgl.h\
#	gtkglext-gtk3/gdk/gdkglinit.h\
#	gtkglext-gtk3/gdk/gdkglprivate.h\
#	gtkglext-gtk3/gdk/gdkglquery.h\
#	gtkglext-gtk3/gdk/gdkgltokens.h\
#	gtkglext-gtk3/gdk/gdkgltypes.h\
#	gtkglext-gtk3/gdk/gdkglversion.h\
#	gtkglext-gtk3/gdk/gdkglwindow.h\
#	gtkglext-gtk3/gdk/gdkglwindowimpl.h\
#	gtkglext-gtk3/gdk/x11/gdkglconfig-x11.c\
#	gtkglext-gtk3/gdk/x11/gdkglcontext-x11.c\
#	gtkglext-gtk3/gdk/x11/gdkglquery-x11.c\
#	gtkglext-gtk3/gdk/x11/gdkglwindow-x11.c\
#	gtkglext-gtk3/gdk/x11/gdkx11glconfig.c\
#	gtkglext-gtk3/gdk/x11/gdkx11glcontext.c\
#	gtkglext-gtk3/gdk/x11/gdkx11glwindow.c\
#	gtkglext-gtk3/gdk/x11/gdkglconfig-x11.h\
#	gtkglext-gtk3/gdk/x11/gdkglcontext-x11.h\
#	gtkglext-gtk3/gdk/x11/gdkglprivate-x11.h\
#	gtkglext-gtk3/gdk/x11/gdkglquery-x11.h\
#	gtkglext-gtk3/gdk/x11/gdkglwindow-x11.h\
#	gtkglext-gtk3/gdk/x11/gdkglx.h\
#	gtkglext-gtk3/gdk/x11/gdkx11glconfig.h\
#	gtkglext-gtk3/gdk/x11/gdkx11glcontext.h\
#	gtkglext-gtk3/gdk/x11/gdkx11glquery.h\
#	gtkglext-gtk3/gdk/x11/gdkx11glwindow.h\
#	gtkglext-gtk3/gdk/gdkglext-config.h\
#	gtkglext-gtk3/gdk/gdkglversion.h

GTKGLEXT_CFLAGS_GTK3 = -I@top_srcdir@/plugins/gtkui/gtkglext-gtk3 -I@top_srcdir@/plugins/gtkui/gtkglext-gtk3/gdk -DGTK_GL_COMPILATION -DGDK_GL_COMPILATION

ddb_gui_GTK3_la_LDFLAGS = -module -avoid-version

if STATICLINK
ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES_GTK3)
GTK_ROOT_300=@top_srcdir@/$(LIB)/gtk-3.0.0

ddb_gui_GTK3_la_LIBADD = $(LDADD) -L$(GTK_ROOT_300)/lib -lgtk-3 -lgdk-3 -latk-1.0 -lgio-2.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo-gobject -lpango-1.0 -lcairo -lgobject-2.0 -lgthread-2.0 -lglib-2.0 -lfreetype $(SM_LIBADD) ../libparser/libparser.a $(GTKGLEXT3_LIBS)

ddb_gui_GTK3_la_CFLAGS = -std=c99 -I$(GTK_ROOT_300)/include/gtk-3.0 -I$(GTK_ROOT_300)/include/pango-1.0 -I$(GTK_ROOT_300)/include/gio-unix-2.0/ -I$(GTK_ROOT_300)/include/atk-1.0 -I$(GTK_ROOT_300)/include/cairo -I$(GTK_ROOT_300)/include/gdk-pixbuf-2.0 -I$(GTK_ROOT_300)/include/freetype2 -I$(GTK_ROOT_300)/include/glib-2.0 -I$(GTK_ROOT_300)/lib/glib-2.0/include $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK3)
else
ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES_GTK3)
ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.a $(GTKGLEXT3_LIBS)
ddb_gui_GTK3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK3)
ddb_gui_GTK3_la_OBJCFLAGS = $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK3)

endif
endif

# fallback lib
if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

ddb_gui_GTK2_fallback_la_SOURCES = $(GTKUI_SOURCES_GTK2)
ddb_gui_GTK2_fallback_la_LDFLAGS = -module -avoid-version

ddb_gui_GTK2_fallback_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0 -lgthread-2.0 -lrt -lglib-2.0 $(SM_LIBADD) ../libparser/libparser.a $(GTKGLEXT2_LIBS)

ddb_gui_GTK2_fallback_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK2)

endif

