SUBDIRS = pixmaps\
		  icons\
		  ${PLUGINS_DIRS}

bin_PROGRAMS = deadbeef

deadbeef_SOURCES =\
	main.c common.h deadbeef.h\
	plugins.c plugins.h moduleconf.h\
	playlist.c playlist.h \
	streamer.c streamer.h\
	messagepump.c messagepump.h\
	conf.c  conf.h\
	playback.h\
	threading_pthread.c threading.h\
	volume.c volume.h\
	junklib.h junklib.c utf8.c utf8.h u8_lc_map.h\
	optmath.h\
	vfs.c vfs.h vfs_stdio.c\
	timeline.c timeline.h\
	md5/md5.c md5/md5.h\
	metacache.c metacache.h

sdkdir = $(pkgincludedir)
sdk_HEADERS = deadbeef.h

deadbeef_LDADD = $(LDADD) $(DEPS_LIBS) $(ICONV_LIB) -lm

AM_CFLAGS = $(DEPS_CFLAGS) -std=c99
AM_CPPFLAGS = $(DEPS_CFLAGS)

docsdir = $(docdir)

docs_DATA = README COPYING.GPLv2 help.txt about.txt ChangeLog COPYING.LGPLv2.1

desktopdir = $(datadir)/applications
desktop_DATA = deadbeef.desktop

EXTRA_DIST = $(docs_DATA) $(desktop_DATA)
