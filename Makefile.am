SUBDIRS = ${MPGMAD_DIR}\
		  gme/Game_Music_Emu-0.5.2\
		  pixmaps\
		  icons\
		  plugins/hotkeys\
		  plugins/nullout\
		  plugins/sid\
		  plugins/ffap\
		  ${ALSA_DIR}\
		  ${LFM_DIR}\
          ${VFS_CURL_DIR}\
          ${GTKUI_DIR}\
          ${VORBIS_DIR}

#SUBDIRS = gme/Game_Music_Emu-0.5.2\
#		  sid/sidplay-libs-2.1.0\
#		  dumb\
#		  pixmaps\
#		  icons\
#		  plugins/hotkeys\
#		  plugins/nullout\
#		  plugins/vtx\
#		  plugins/adplug\
#		  ${ALSA_DIR}\
#		  ${LFM_DIR}\
#          ${MPGMAD_DIR}\
#          ${VORBIS_DIR}\
#          ${FLAC_DIR}\
#          ${WAVPACK_DIR}\
#          ${SNDFILE_DIR}\
#          ${VFS_CURL_DIR}\
#          ${CDDA_DIR}\
#          ${GTKUI_DIR}\
#          ${FFMPEG_DIR}

dumbpath=@top_srcdir@/dumb
gmepath=@top_srcdir@/gme/Game_Music_Emu-0.5.2

bin_PROGRAMS = deadbeef

deadbeef_SOURCES =\
	main.c common.h deadbeef.h\
	plugins.c plugins.h moduleconf.h\
	playlist.c playlist.h \
	streamer.c streamer.h\
	codec.c codec.h\
	messagepump.c messagepump.h\
	conf.c  conf.h\
	playback.h\
	threading_pthread.c threading.h\
	volume.c volume.h\
	session.h session.c \
	junklib.h junklib.c utf8.c utf8.h\
	optmath.h\
	vfs.c vfs.h vfs_stdio.c\
	timeline.c timeline.h\
	cgme.c cdumb.c\
	md5/md5.c md5/md5.h

sdkdir = $(pkgincludedir)
sdk_HEADERS = deadbeef.h

deadbeef_LDADD = $(LDADD) $(DEPS_LIBS) gme/Game_Music_Emu-0.5.2/gme/libgme.a dumb/libdumb.a -lstdc++

AM_CFLAGS = $(DEPS_CFLAGS) -I$(gmepath) -std=c99
AM_CPPFLAGS = $(DEPS_CFLAGS)

docsdir = $(docdir)

docs_DATA = README COPYING.GPLv2 help.txt about.txt ChangeLog COPYING.LGPLv2.1

desktopdir = $(datadir)/applications
desktop_DATA = deadbeef.desktop

EXTRA_DIST = $(docs_DATA) $(desktop_DATA)
