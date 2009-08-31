## Process this file with automake to produce Makefile.in
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = gme/Game_Music_Emu-0.5.2 gme/Game_Music_Emu-0.5.2/gme sid/sidplay-libs-2.1.0 dumb pixmaps plugins/lastfm plugins/ape

dumbpath=@top_srcdir@/dumb
sidpath=@top_srcdir@/sid/sidplay-libs-2.1.0
gmepath=@top_srcdir@/gme/Game_Music_Emu-0.5.2

bin_PROGRAMS = deadbeef

deadbeef_SOURCES =\
	main.c common.h deadbeef.h\
	plugins.c plugins.h\
	callbacks.c interface.c support.c callbacks.h interface.h support.h\
	playlist.c playlist.h gtkplaylist.c gtkplaylist.h\
	streamer.c streamer.h\
	progress.c  progress.h\
	codec.c codec.h\
	messagepump.c messagepump.h messages.h\
	conf.c  conf.h\
	search.c search.h\
	cmp3.c cvorbis.c cflac.c cgme.c cdumb.c csid.cpp\
	palsa.c palsa.h playback.h\
	threading_pthread.c threading.h\
	md5/md5.c md5/md5.h md5/md5_loc.h\
	volume.c volume.h\
	drawing.h gdkdrawing.c\
	session.h session.c gtksession.c

sdkdir = $(pkgincludedir)
sdk_HEADERS = deadbeef.h

deadbeef_LDADD = $(LDADD) $(DEPS_LIBS) gme/Game_Music_Emu-0.5.2/gme/libgme.a sid/sidplay-libs-2.1.0/libsidplay2.a dumb/libdumb.a -lstdc++

AM_CFLAGS = $(DEPS_CFLAGS) $(SIMD_FLAGS) -I$(gmepath) -std=c99
AM_CPPFLAGS = $(DEPS_CFLAGS) -I$(sidpath)/libsidplay/include -I$(sidpath)/builders/resid-builder/include

docsdir = $(datadir)/doc/$(PACKAGE)

docs_DATA = README COPYING config-example

desktopdir = $(datadir)/applications
desktop_DATA = deadbeef.desktop.in

EXTRA_DIST = $(docs_DATA) $(desktop_DATA)

#install-data-local:
#	@$(NORMAL_INSTALL)
#	if test -d $(srcdir)/pixmaps; then \
#	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
#	  for pixmap in $(srcdir)/pixmaps/*; do \
#	    if test -f $$pixmap; then \
#	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
#	    fi \
#	  done \
#	fi
#
#dist-hook:
#	if test -d pixmaps; then \
#	  mkdir $(distdir)/pixmaps; \
#	  for pixmap in pixmaps/*; do \
#	    if test -f $$pixmap; then \
#	      cp -p $$pixmap $(distdir)/pixmaps; \
#	    fi \
#	  done \
#	fi
#
