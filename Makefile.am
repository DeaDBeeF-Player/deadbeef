## Process this file with automake to produce Makefile.in

SUBDIRS = gme/Game_Music_Emu-0.5.2/gme libsidplay2/sidplay-libs-2.1.0 dumb

#EXTRA_DIST = \
#	autogen.sh \
#	deadbeef.glade \
#	deadbeef.gladep

bin_PROGRAMS = deadbeef

deadbeef_SOURCES = callbacks.c csid.cpp interface.c playlist.c streamer.c\
	cdumb.c cmp3.c cvorbis.c main.c progress.c support.c\
	cflac.c codec.c messagepump.c\
	cgme.c conf.c gtkplaylist.c palsa.c search.c threading_pthread.c\
	md5/md5.c

deadbeef_LDADD = $(LDADD) $(DEPS_LIBS) gme/Game_Music_Emu-0.5.2/gme/libgme.a libsidplay2/sidplay-libs-2.1.0/libsidplay2.a dumb/libdumb.a
AM_CFLAGS = $(DEPS_CFLAGS) -Igme/Game_Music_Emu-0.5.2 -std=c99
AM_CPPFLAGS = $(DEPS_CFLAGS) -Ilibsidplay2/sidplay-libs-2.1.0/libsidplay/include -Ilibsidplay2/sidplay-libs-2.1.0/builders/resid-builder/include

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

