## Process this file with automake to produce Makefile.in

SUBDIRS = gme/Game_Music_Emu-0.5.2 gme/Game_Music_Emu-0.5.2/gme sid/sidplay-libs-2.1.0 dumb pixmaps

dumbpath=@top_srcdir@/dumb
sidpath=@top_srcdir@/sid/sidplay-libs-2.1.0
gmepath=@top_srcdir@/gme/Game_Music_Emu-0.5.2

bin_PROGRAMS = deadbeef

deadbeef_SOURCES = callbacks.c csid.cpp interface.c playlist.c streamer.c\
	cdumb.c cmp3.c cvorbis.c main.c progress.c support.c\
	cflac.c codec.c messagepump.c\
	cgme.c conf.c gtkplaylist.c palsa.c search.c threading_pthread.c\
	md5/md5.c\
	callbacks.h cmod.h conf.h messages.h progress.h support.h\
	cdumb.h cmp3.h gtkplaylist.h palsa.h threading.h\
	cflac.h codec.h csid.h interface.h playback.h search.h\
	cgme.h common.h cvorbis.h messagepump.h playlist.h streamer.h\
	md5/md5.h md5/md5_loc.h


deadbeef_LDADD = $(LDADD) $(DEPS_LIBS) gme/Game_Music_Emu-0.5.2/gme/libgme.a sid/sidplay-libs-2.1.0/libsidplay2.a dumb/libdumb.a

AM_CFLAGS = $(DEPS_CFLAGS) -I$(gmepath) -std=c99
AM_CPPFLAGS = $(DEPS_CFLAGS) -I$(sidpath)/libsidplay/include -I$(sidpath)/builders/resid-builder/include

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
