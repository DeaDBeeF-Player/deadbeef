convdata = \
	convpresets/AAC_VBR_192Kbps_(Nero_FAAC).txt\
	convpresets/AAC_VBR_256Kbps_(Nero_FAAC).txt\
	convpresets/AAC_VBR_320Kbps_(Nero_FAAC).txt\
	convpresets/FLAC_(compression_level_5).txt\
	convpresets/MP3_CBR_320_Kbps_(Lame).txt\
	convpresets/MP3_VBR_192Kbps_(Lame).txt\
	convpresets/MS_RIFF_WAV.txt\
	convpresets/MusePack.txt\
	convpresets/Ogg_FLAC.txt\
	convpresets/Ogg_VBR_192Kbps.txt\
	convpresets/Ogg_VBR_256Kbps.txt\
	convpresets/Ogg_VBR_320Kbps.txt\
	convpresets/Opus.txt\
	convpresets/TTA.txt\
	convpresets/WavPack.txt\
	convpresets/ALAC.txt\
	convpresets/File_Copy.txt\
	convpresets/File_Move.txt

EXTRA_DIST = $(convdata) converter.glade

if HAVE_CONVERTER

if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = converter.la converter_gtk2.la converter_gtk3.la
else
pkglib_LTLIBRARIES = converter.la converter_gtk2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = converter.la converter_gtk3.la
endif
endif

convdatadir = $(libdir)/deadbeef/convpresets
convdata_DATA = $(convdata)

converter_la_CFLAGS =  $(CFLAGS) -I@top_srcdir@/external/mp4p/include -I@top_srcdir@/include -std=c99 -fPIC -DUSE_TAGGING=1
converter_la_SOURCES = converter.c converter.h
converter_la_LDFLAGS = -module -avoid-version
converter_la_LIBADD = $(LDADD) ../../shared/libmp4tagutil.la ../../external/libmp4p.la

if HAVE_GTK2
converter_gtk2_la_SOURCES = convgui.c interface.c support.c callbacks.h converter.h interface.h support.h
converter_gtk2_la_LDFLAGS = -module -avoid-version
converter_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
converter_gtk2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS) -I@top_srcdir@/include
endif

if HAVE_GTK3
converter_gtk3_la_SOURCES = convgui.c interface.c support.c callbacks.h converter.h interface.h support.h
converter_gtk3_la_LDFLAGS = -module -avoid-version
converter_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
converter_gtk3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS) -I@top_srcdir@/include
endif

sdkdir = $(pkgincludedir)
sdk_HEADERS = converter.h


endif
