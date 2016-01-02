if HAVE_NOTIFY
pkglib_LTLIBRARIES = notify.la
notify_la_SOURCES = notify.c
notify_la_LDFLAGS = -module -avoid-version

notify_la_LIBADD = $(LDADD) $(NOTIFY_LIBS)
AM_CFLAGS = $(CFLAGS) $(NOTIFY_CFLAGS) -std=c99
endif
