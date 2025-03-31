if HAVE_LYRICS

if HAVE_GTK2
if HAVE_GTK3
pkglib_LTLIBRARIES = lyrics_gtk2.la lyrics_gtk3.la
else
pkglib_LTLIBRARIES = lyrics_gtk2.la
endif
else
if HAVE_GTK3
pkglib_LTLIBRARIES = lyrics_gtk3.la
endif
endif

if HAVE_GTK2
lyrics_gtk2_la_SOURCES = lyrics.c support.c support.h
lyrics_gtk2_la_LDFLAGS = -module -avoid-version
lyrics_gtk2_la_LIBADD = $(LDADD) $(GTK2_DEPS_LIBS)
lyrics_gtk2_la_CFLAGS = $(CFLAGS) -std=c99 $(GTK2_DEPS_CFLAGS) -I@top_srcdir@/include -fPIC
endif

if HAVE_GTK3
lyrics_gtk3_la_SOURCES = lyrics.c
lyrics_gtk3_la_LDFLAGS = -module -avoid-version
lyrics_gtk3_la_LIBADD = $(LDADD) $(GTK3_DEPS_LIBS)
lyrics_gtk3_la_CFLAGS = $(CFLAGS) -std=c99 $(GTK3_DEPS_CFLAGS) -I@top_srcdir@/include -fPIC
endif

endif
