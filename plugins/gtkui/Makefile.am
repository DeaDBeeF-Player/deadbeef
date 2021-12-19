EXTRA_DIST = deadbeef.glade\
	gtkui.gresources.xml\
	gtk/menus.ui

GTKUI_SOURCES =    gtkui.c gtkui.h\
				   gtkui_deletefromdisk.c gtkui_deletefromdisk.h\
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
				   plcommon.c plcommon.h\
				   plmenu.c plmenu.h\
				   prefwin/prefwin.c prefwin/prefwin.h\
				   prefwin/prefwinappearance.c prefwin/prefwinappearance.h\
				   prefwin/prefwinmedialib.c prefwin/prefwinmedialib.h\
				   prefwin/prefwinmisc.c prefwin/prefwinmisc.h\
				   prefwin/prefwinnetwork.c prefwin/prefwinnetwork.h\
				   prefwin/prefwinplayback.c prefwin/prefwinplayback.h\
				   prefwin/prefwinplugins.c prefwin/prefwinplugins.h\
				   prefwin/prefwinsound.c prefwin/prefwinsound.h\
				   eq.c eq.h\
				   actions.c actions.h\
				   dspconfig.c dspconfig.h\
				   tagwritersettings.c tagwritersettings.h\
				   wingeom.c wingeom.h\
				   pluginconf.h\
				   widgets.c widgets.h\
				   ddbseekbar.h ddbequalizer.h ddbcellrenderertextmultiline.h\
				   ddbseekbar.c ddbequalizer.c ddbcellrenderertextmultiline.c\
                   hotkeys.c hotkeys.h\
                   actionhandlers.c actionhandlers.h\
                   pltmenu.c\
                   ctmapping.c ctmapping.h\
                   namedicons.c namedicons.h\
                   tfimport.c tfimport.h\
                   ddb_splitter.c ddb_splitter.h\
                   ddb_splitter_size_mode.c ddb_splitter_size_mode.h\
                   ../../utf8.c\
                   clipboard.c clipboard.h\
				   rg.c rg.h\
				   medialib/medialibwidget.c medialib/medialibwidget.h\
				   medialib/medialibmanager.c medialib/medialibmanager.h\
				   $(SM_SOURCES) $(OSXSRC)

sdkdir = $(pkgincludedir)
sdk_HEADERS = gtkui_api.h

if STATICLINK
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

ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.la ../../shared/libtrkpropertiesutil.la ../../shared/libeqpreset.la ../../shared/libdeletefromdisk.la ../../shared/libtftintutil.la ../../analyzer/libanalyzer.la ../../scope/libscope.la $(JANSSON_LIBS) $(DISPATCH_LIBS)

ddb_gui_GTK2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS) $(DISPATCH_CFLAGS) -DDDB_WARN_DEPRECATED=1

endif

if HAVE_GTK3
GTKUI_SOURCES_GTK3 = deadbeefapp.c deadbeefapp.h $(GTKUI_SOURCES)

ddb_gui_GTK3_la_LDFLAGS = -module -avoid-version

ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES_GTK3)
ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.la ../../shared/libtrkpropertiesutil.la ../../shared/libeqpreset.la ../../shared/libdeletefromdisk.la ../../shared/libtftintutil.la ../../analyzer/libanalyzer.la ../../scope/libscope.la $(JANSSON_LIBS) $(DISPATCH_LIBS)
ddb_gui_GTK3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS) $(DISPATCH_CFLAGS)
ddb_gui_GTK3_la_OBJCFLAGS = $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS)

nodist_ddb_gui_GTK3_la_SOURCES = gtkui-gresources.c

BUILT_SOURCES=gtkui-gresources.c
CLEANFILES=gtkui-gresources.c

resource_files = \
	gtkui.gresources.xml	\
	gtk/menus.ui

gtkui-gresources.c: $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

endif

