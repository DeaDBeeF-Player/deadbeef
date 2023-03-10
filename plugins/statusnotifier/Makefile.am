if HAVE_STATUSNOTIFIER
pkglib_LTLIBRARIES = statusnotifier.la

statusnotifier_la_CFLAGS = $(CFLAGS) -std=c99 -fPIC -I@top_srcdir@/include

statusnotifier_la_SOURCES = statusnotifier.c statusnotifier.h ddb_statusnotifier.c ddb_statusnotifier.h
statusnotifier_la_LDFLAGS = -module -avoid-version
statusnotifier_la_CFLAGS = -std=c99 $(STATUSNOTIFIER_DEPS_CFLAGS) -I@top_srcdir@/include
statusnotifier_la_LIBADD = $(STATUSNOTIFIER_DEPS_LIBS)

endif

