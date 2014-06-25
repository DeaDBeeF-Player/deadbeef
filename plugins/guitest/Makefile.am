guitestdir = $(libdir)/$(PACKAGE)

if OS_OSX
osx_sources = retina.m
endif

GUITEST_SOURCES =    gtkui.c gtkui.h\
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

if HAVE_GTK2
pkglib_LTLIBRARIES = ddb_gui_TEST.la
endif

# normal lib
if HAVE_GTK2
GUITEST_SOURCES_GTK2 = $(GUITEST_SOURCES)

ddb_gui_TEST_la_SOURCES = $(GUITEST_SOURCES_GTK2)
ddb_gui_TEST_la_LDFLAGS = -module -avoid-version

ddb_gui_TEST_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS) $(SM_LIBADD) ../libparser/libparser.a  $(GTKGLEXT2_LIBS)

ddb_gui_TEST_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) $(SM_CFLAGS) $(GTKGLEXT_CFLAGS_GTK2) -DDDB_WARN_DEPRECATED=1

endif


