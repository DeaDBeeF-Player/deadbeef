if HAVE_NOTIFY
pkglib_LTLIBRARIES = notify.la
notify_la_SOURCES = notify.c
notify_la_LDFLAGS = -module -avoid-version

notify_la_LIBADD = $(LDADD) $(NOTIFY_LIBS) $(NOTIFY_DEPS_LIBS) $(DISPATCH_LIBS)
notify_la_CFLAGS = -std=c99 $(CFLAGS) $(NOTIFY_CFLAGS) $(NOTIFY_DEPS_CFLAGS) $(DISPATCH_CFLAGS) -I@top_srcdir@/include
endif
