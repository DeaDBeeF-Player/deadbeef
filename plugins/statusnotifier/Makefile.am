if HAVE_STATUSNOTIFIER
pkglib_LTLIBRARIES = statusnotifier.la

AM_CFLAGS = $(CFLAGS) -std=c99 -fPIC

statusnotifier_la_SOURCES = statusnotifier.c statusnotifier.h ddb_statusnotifier.c ddb_statusnotifier.h
statusnotifier_la_LDFLAGS = -module -avoid-version
statusnotifier_la_LIBADD = $(LDADD) $(STATUSNOTIFIER_DEPS_LIBS)
if HAVE_GTK2
statusnotifier_la_CFLAGS = -std=c99 $(GTK2_DEPS_CFLAGS)
else
statusnotifier_la_CFLAGS = -std=c99 $(GTK3_DEPS_CFLAGS)
endif

endif

