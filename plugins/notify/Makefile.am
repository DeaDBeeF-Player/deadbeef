if HAVE_NOTIFY
notifydir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = notify.la
notify_la_SOURCES = notify.c
notify_la_LDFLAGS = -module

notify_la_LIBADD = $(LDADD) $(NOTIFY_LIBS)
AM_CFLAGS = $(CFLAGS) $(NOTIFY_CFLAGS) -std=c99
endif
