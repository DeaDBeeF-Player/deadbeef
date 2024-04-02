SUBDIRS = \
		  external\
		  shared\
		  ${PLUGINS_DIRS}\
		  po\
		  intl\
		  pixmaps\
		  icons\
		  src

INTLTOOL_FILES = \
		intltool-extract.in \
		intltool-merge.in \
		intltool-update.in

DISTCLEANFILES = \
		intltool-extract \
		intltool-merge \
		intltool-update

docsdir = $(docdir)

docs_DATA = README help.txt about.txt translators.txt ChangeLog\
	COPYING.GPLv2 COPYING.LGPLv2.1

desktopdir = $(datadir)/applications
desktop_DATA = deadbeef.desktop deadbeef_enqueue.desktop

EXTRA_DIST = $(docs_DATA) $(desktop_DATA) $(INTLTOOL_FILES) translation/extra.c translation/plugins.c examples/decoder_template.c examples/dsp_template.c yasmwrapper.sh\
	include/deadbeef/deadbeef.h include/deadbeef/common.h include/deadbeef/fastftoi.h include/deadbeef/strdupa.h\
	gettext.h
sdkdir = $(pkgincludedir)
sdk_HEADERS = include/deadbeef/deadbeef.h

ACLOCAL_AMFLAGS = -I m4
