if HAVE_MEDIALIB
pkglib_LTLIBRARIES = medialib.la

medialib_la_SOURCES =\
	medialib.c\
	medialib.h\
	medialibcommon.c\
	medialibcommon.h\
	medialibdb.c\
	medialibdb.h\
	medialibfilesystem.h\
	medialibfilesystem_inotify.c\
	medialibscanner.c\
	medialibscanner.h\
	medialibsource.c\
	medialibsource.h\
	medialibstate.c\
	medialibstate.h\
	medialibtree.c\
	medialibtree.h

medialib_la_LDFLAGS = -module -avoid-version

medialib_la_LIBADD = $(LDADD) $(JANSSON_LIBS) $(DISPATCH_LIBS)
medialib_la_CFLAGS = $(CFLAGS) -std=c99 -fPIC $(JANSSON_CFLAGS) $(DISPATCH_CFLAGS) -I@top_srcdir@/include

sdkdir = $(pkgincludedir)
sdk_HEADERS = medialib.h

endif
