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
convdata_DATA = \
	presets/AAC_(Nero_FAAC).txt\
	presets/FLAC_(compression_level_5).txt\
	presets/MP3_CBR_320_Kbps_(Lame).txt\
	presets/MP3_VBR_192Kbps_(Lame).txt\
	presets/MS_RIFF_WAV.txt\
	presets/MusePack.txt\
	presets/Ogg_Vorbis_(-q_5).txt\
	presets/TTA.txt\
	presets/WavPack.txt

EXTRA_DIST = $(convdata_DATA) converter.glade

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

converter_la_SOURCES = converter.c converter.h
converter_la_LDFLAGS = -module
if HAVE_GTK2
converter_gtk2_la_SOURCES = convgui.c interface.c support.c callbacks.h converter.h interface.h support.h
converter_gtk2_la_LDFLAGS = -module
endif
if HAVE_GTK3
converter_gtk3_la_SOURCES = convgui.c interface.c support.c callbacks.h converter.h interface.h support.h
converter_gtk3_la_LDFLAGS = -module
endif

if STATICLINK
GTK_ROOT=@top_srcdir@/$(LIB)/gtk-2.12.12/usr

converter_gtk2_la_LIBADD = $(LDADD) -L$(GTK_ROOT)/lib $(GTK_ROOT)/lib/libgtk-x11-2.0.la $(GTK_ROOT)/lib/libgdk-x11-2.0.la $(GTK_ROOT)/lib/libpangoft2-1.0.la $(GTK_ROOT)/lib/libpangocairo-1.0.la $(GTK_ROOT)/lib/libgdk_pixbuf-2.0.la -lm $(GTK_ROOT)/lib/libcairo.la $(GTK_ROOT)/lib/libpango-1.0.la $(GTK_ROOT)/lib/libgobject-2.0.la $(GTK_ROOT)/lib/libgmodule-2.0.la $(GTK_ROOT)/lib/libgthread-2.0.la -lrt $(GTK_ROOT)/lib/libglib-2.0.la

converter_gtk2_la_CFLAGS = -std=c99 -I $(GTK_ROOT)/include -I $(GTK_ROOT)/lib/gtk-2.0/include -I $(GTK_ROOT)/include/glib-2.0 -I $(GTK_ROOT)/include/gtk-2.0 -I $(GTK_ROOT)/include/cairo -I $(GTK_ROOT)/lib/glib-2.0/include/ -I $(GTK_ROOT)/include/pango-1.0 -I $(GTK_ROOT)/include/atk-1.0 -DULTRA_COMPATIBLE=1
else
if HAVE_GTK2
converter_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
converter_gtk2_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
endif
if HAVE_GTK3
converter_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
converter_gtk3_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
endif

endif

endif
