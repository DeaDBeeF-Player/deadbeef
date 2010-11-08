SUBDIRS = pixmaps\
		  icons\
		  ${PLUGINS_DIRS}\
		  po\
		  intl

bin_PROGRAMS = deadbeef

INTLTOOL_FILES = \
		intltool-extract.in \
		intltool-merge.in \
		intltool-update.in

DISTCLEANFILES = \
		intltool-extract \
		intltool-merge \
		intltool-update

deadbeef_SOURCES =\
	main.c common.h deadbeef.h\
	plugins.c plugins.h moduleconf.h\
	playlist.c playlist.h \
	streamer.c streamer.h\
	messagepump.c messagepump.h\
	conf.c  conf.h\
	threading_pthread.c threading.h\
	volume.c volume.h\
	junklib.h junklib.c utf8.c utf8.h u8_lc_map.h\
	optmath.h\
	vfs.c vfs.h vfs_stdio.c\
	md5/md5.c md5/md5.h\
	metacache.c metacache.h\
	gettext.h

sdkdir = $(pkgincludedir)
sdk_HEADERS = deadbeef.h

deadbeef_LDADD = $(LDADD) $(DEPS_LIBS) $(ICONV_LIB) -lm -ldl -lpthread

AM_CFLAGS = $(DEPS_CFLAGS) -std=c99
AM_CPPFLAGS = $(DEPS_CFLAGS)

docsdir = $(docdir)

docs_DATA = README help.txt about.txt translators.txt ChangeLog\
	COPYING.GPLv2 COPYING.LGPLv2.1\
	translation/help.ru.txt\
	translation/help.pt_BR.txt

desktopdir = $(datadir)/applications
desktop_DATA = deadbeef.desktop

EXTRA_DIST = $(docs_DATA) $(desktop_DATA) $(INTLTOOL_FILES) translation/extra.c

ACLOCAL_AMFLAGS = -I m4
