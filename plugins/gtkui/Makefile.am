EXTRA_DIST = deadbeef.glade\
	gtkui.gresources.xml\
	gtk/menus.ui\
	ui_layouts

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
	layoutmanager.c layoutmanager.h\
	layoutmenu.c layoutmenu.h\
	plmenu.c plmenu.h\
	pluginconf.c pluginconf.h\
	progress.c progress.h\
	rg.c rg.h\
	search.c search.h\
	tagwritersettings.c tagwritersettings.h\
	trkproperties.c trkproperties.h\
	trkproperties_artwork.c trkproperties_artwork.h\
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
	prefwin/prefwinmisc.c prefwin/prefwinmisc.h langnames.h\
	prefwin/prefwinnetwork.c prefwin/prefwinnetwork.h\
	prefwin/prefwinplayback.c prefwin/prefwinplayback.h\
	prefwin/prefwinplugins.c prefwin/prefwinplugins.h\
	prefwin/prefwinsound.c prefwin/prefwinsound.h\
	scriptable/gtkScriptable.c scriptable/gtkScriptable.h\
	scriptable/gtkScriptablePropertySheetEditViewController.c scriptable/gtkScriptablePropertySheetEditViewController.h\
	scriptable/gtkScriptablePropertySheetEditWindowController.c scriptable/gtkScriptablePropertySheetEditWindowController.h\
	scriptable/gtkScriptableListEditViewController.c scriptable/gtkScriptableListEditViewController.h\
	scriptable/gtkScriptableListEditWindowController.c scriptable/gtkScriptableListEditWindowController.h\
	scriptable/gtkScriptableSelectViewController.c scriptable/gtkScriptableSelectViewController.h\
	widgets/w_box.c widgets/w_box.h\
	widgets/w_button.c widgets/w_button.h\
	widgets/w_ctvoices.c widgets/w_ctvoices.h\
	widgets/w_dummy.c widgets/w_dummy.h\
	widgets/w_hvbox.c widgets/w_hvbox.h\
	widgets/w_logviewer.c widgets/w_logviewer.h\
	widgets/w_placeholder.c widgets/w_placeholder.h\
	widgets/w_playlist.c widgets/w_playlist.h\
	widgets/w_playtb.c widgets/w_playtb.h\
	widgets/w_scope.c widgets/w_scope.h\
	widgets/w_seekbar.c widgets/w_seekbar.h\
	widgets/w_spectrum.c widgets/w_spectrum.h\
	widgets/w_splitter.c widgets/w_splitter.h\
	widgets/w_tabbed_playlist.c widgets/w_tabbed_playlist.h\
	widgets/w_tabs.c widgets/w_tabs.h\
	widgets/w_tabstrip.c widgets/w_tabstrip.h\
	widgets/w_volumebar.c widgets/w_volumebar.h\
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

ddb_gui_GTK2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) ../../shared/libtrkpropertiesutil.la ../../shared/libeqpreset.la ../../shared/libdeletefromdisk.la ../../shared/libtftintutil.la ../../shared/analyzer/libanalyzer.la ../../shared/scope/libscope.la ../../shared/scriptable/libscriptable.la ../../shared/libgrowablebuffer.la ../../shared/libpluginsettings.la ../../shared/libparser.la $(JANSSON_LIBS) $(DISPATCH_LIBS)

ddb_gui_GTK2_la_CFLAGS = -std=c99 -Werror -DGLIB_DISABLE_DEPRECATION_WARNINGS -DGDK_DISABLE_DEPRECATION_WARNINGS -DGTK_DISABLE_DEPRECATION_WARNINGS $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS) $(JANSSON_CFLAGS) $(DISPATCH_CFLAGS) $(MEDIALIB_CFLAGS) -I@top_srcdir@/include -I@top_srcdir@/shared -DDDB_WARN_DEPRECATED=1

endif

if HAVE_GTK3
GTKUI_SOURCES_GTK3 = deadbeefapp.c deadbeefapp.h $(GTKUI_SOURCES)

ddb_gui_GTK3_la_SOURCES = $(GTKUI_SOURCES_GTK3)
ddb_gui_GTK3_la_LDFLAGS = -module -avoid-version -lm

ddb_gui_GTK3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS) $(SM_LIBADD) ../../shared/libtrkpropertiesutil.la ../../shared/libeqpreset.la ../../shared/libdeletefromdisk.la ../../shared/libtftintutil.la ../../shared/analyzer/libanalyzer.la ../../shared/scope/libscope.la ../../shared/scriptable/libscriptable.la ../../shared/libgrowablebuffer.la ../../shared/libpluginsettings.la ../../shared/libparser.la $(JANSSON_LIBS) $(DISPATCH_LIBS)

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

# Built-in UI layouts. Everything matching ui_layouts/*.ddblayout is installed,
# only if at least one GTK UI is built.
if HAVE_GTK2
GTKUI_INSTALL_UI_LAYOUTS = yes
else
if HAVE_GTK3
GTKUI_INSTALL_UI_LAYOUTS = yes
endif
endif

uilayoutsdir = $(libdir)/deadbeef/ui_layouts

install-data-local:
	@if test -n "$(GTKUI_INSTALL_UI_LAYOUTS)"; then \
	  $(MKDIR_P) "$(DESTDIR)$(uilayoutsdir)" || exit 1; \
	  for f in $(srcdir)/ui_layouts/*.ddblayout; do \
	    test -f "$$f" || continue; \
	    echo " $(INSTALL_DATA) '$$f' '$(DESTDIR)$(uilayoutsdir)'"; \
	    $(INSTALL_DATA) "$$f" "$(DESTDIR)$(uilayoutsdir)" || exit 1; \
	  done; \
	fi

uninstall-local:
	@if test -n "$(GTKUI_INSTALL_UI_LAYOUTS)"; then \
	  for f in $(srcdir)/ui_layouts/*.ddblayout; do \
	    test -f "$$f" || continue; \
	    echo " rm -f '$(DESTDIR)$(uilayoutsdir)/'`basename \"$$f\"`"; \
	    rm -f "$(DESTDIR)$(uilayoutsdir)/"`basename "$$f"`; \
	  done; \
	fi
