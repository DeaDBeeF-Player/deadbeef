EXTRA_DIST = deadbeef.glade\
	gtkui.gresources.xml\
	gtk/menus.ui

GTKUI_SOURCES = \
    ../../src/utf8.c\
	actions.c actions.h\
	callbacks.c callbacks.h\
	support.c support.h\
	interface.c interface.h\
	ddbseekbar.c ddbseekbar.h\
   	ddbcellrenderertextmultiline.c ddbcellrenderertextmultiline.h\
 	ddbequalizer.c ddbequalizer.h\
	ddbtabstrip.c ddbtabstrip.h\
	ddbvolumebar.c ddbvolumebar.h\
	drawing.h gdkdrawing.c\
	dspconfig.c dspconfig.h\
	eq.c eq.h\
	fileman.c\
	gtkui.c gtkui.h\
	gtkui_deletefromdisk.c gtkui_deletefromdisk.h\
	plmenu.c plmenu.h\
	pluginconf.c pluginconf.h\
	progress.c progress.h\
	rg.c rg.h\
	search.c search.h\
	tagwritersettings.c tagwritersettings.h\
	trkproperties.c trkproperties.h\
	widgets.c widgets.h\
	selpropertieswidget.c selpropertieswidget.h\
	wingeom.c wingeom.h\
    actionhandlers.c actionhandlers.h\
    clipboard.c clipboard.h\
    ctmapping.c ctmapping.h\
    ddb_splitter.c ddb_splitter.h\
    ddb_splitter_size_mode.c ddb_splitter_size_mode.h\
    hotkeys.c hotkeys.h\
    namedicons.c namedicons.h\
    pltmenu.c\
    tfimport.c tfimport.h\
	medialib/medialibwidget.c medialib/medialibwidget.h\
	medialib/medialibmanager.c medialib/medialibmanager.h\
	medialib/mlcellrendererpixbuf.c medialib/mlcellrendererpixbuf.h\
	covermanager/covermanager.c covermanager/covermanager.h\
	covermanager/gobjcache.c covermanager/gobjcache.h\
	covermanager/albumartwidget.c covermanager/albumartwidget.h\
	playlist/ddblistview.c playlist/ddblistview.h\
	playlist/ddblistviewheader.c playlist/ddblistviewheader.h\
	playlist/mainplaylist.c playlist/mainplaylist.h\
	playlist/playlistcontroller.c playlist/playlistcontroller.h\
	playlist/playlistrenderer.c playlist/playlistrenderer.h\
	playlist/plcommon.c playlist/plcommon.h\
	playlist/searchplaylist.c playlist/searchplaylist.h\
	prefwin/prefwin.c prefwin/prefwin.h\
	prefwin/prefwinappearance.c prefwin/prefwinappearance.h\
	prefwin/prefwinmedialib.c prefwin/prefwinmedialib.h\
	prefwin/prefwinmisc.c prefwin/prefwinmisc.h\
	prefwin/prefwinnetwork.c prefwin/prefwinnetwork.h\
	prefwin/prefwinplayback.c prefwin/prefwinplayback.h\
	prefwin/prefwinplugins.c prefwin/prefwinplugins.h\
	prefwin/prefwinsound.c prefwin/prefwinsound.h\
	scriptable/gtkScriptable.c scriptable/gtkScriptable.h\
	scriptable/gtkScriptableListEditViewController.c scriptable/gtkScriptableListEditViewController.h\
	scriptable/gtkScriptableListEditWindowController.c scriptable/gtkScriptableListEditWindowController.h\
	scriptable/gtkScriptableSelectViewController.c scriptable/gtkScriptableSelectViewController.h\
	undostack.c undostack.h\
	undointegration.c undointegration.h

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
ddb_gui_GTK2_la_LDFLAGS = -module -avoid-version -lm

ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.la ../../shared/libtrkpropertiesutil.la ../../shared/libeqpreset.la ../../shared/libdeletefromdisk.la ../../shared/libtftintutil.la ../../shared/analyzer/libanalyzer.la ../../shared/scope/libscope.la ../../shared/scriptable/libscriptable.la  $(JANSSON_LIBS) $(DISPATCH_LIBS)

ddb_gui_GTK2_la_CFLAGS = -std=c99 -Werror -DGLIB_DISABLE_DEPRECATION_WARNINGS -DGDK_DISABLE_DEPRECATION_WARNINGS -DGTK_DISABLE_DEPRECATION_WARNINGS $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS) $(DISPATCH_CFLAGS) $(MEDIALIB_CFLAGS) -I@top_srcdir@/include -I@top_srcdir@/shared -DDDB_WARN_DEPRECATED=1

endif

if HAVE_GTK3
GTKUI_SOURCES_GTK3 = deadbeefapp.c deadbeefapp.h $(GTKUI_SOURCES)

ddb_gui_GTK3_la_LDFLAGS = -module -avoid-version -lm

ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES_GTK3)
ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.la ../../shared/libtrkpropertiesutil.la ../../shared/libeqpreset.la ../../shared/libdeletefromdisk.la ../../shared/libtftintutil.la ../../shared/analyzer/libanalyzer.la ../../shared/scope/libscope.la ../../shared/scriptable/libscriptable.la $(JANSSON_LIBS) $(DISPATCH_LIBS)
ddb_gui_GTK3_la_CFLAGS = -std=c99 -Werror -DGLIB_DISABLE_DEPRECATION_WARNINGS -DGDK_DISABLE_DEPRECATION_WARNINGS -DGTK_DISABLE_DEPRECATION_WARNINGS $(GTK3_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS) $(DISPATCH_CFLAGS) $(MEDIALIB_CFLAGS) -I@top_srcdir@/include -I@top_srcdir@/shared -DDDB_WARN_DEPRECATED=1
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

