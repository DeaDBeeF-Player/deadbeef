## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = po intl include share debian win32 @OPT_SUBDIRS@
DIST_SUBDIRS = po intl include share debian win32 ogg123 oggenc oggdec ogginfo \
	vcut vorbiscomment m4

EXTRA_DIST = config.rpath README AUTHORS COPYING CHANGES vorbis-tools.spec config.h mkinstalldirs

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

ACLOCAL_AMFLAGS = -I m4
