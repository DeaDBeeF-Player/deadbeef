notificationdir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = notification.la
notification_la_SOURCES = notification.c
notification_la_LDFLAGS = -module
notification_la_LIBADD = $(LDADD) $(NOTIFICATION_DEPS_LIBS)
AM_CFLAGS = -std=c99 $(NOTIFICATION_DEPS_CFLAGS)
