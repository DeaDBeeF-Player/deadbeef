if HAVE_VFS_CURL
pkglib_LTLIBRARIES = vfs_curl.la
vfs_curl_la_SOURCES = vfs_curl.c vfs_curl.h
vfs_curl_la_LDFLAGS = -module -avoid-version

vfs_curl_la_LIBADD = $(LDADD) $(CURL_LIBS)
vfs_curl_la_CFLAGS = $(CFLAGS) $(CURL_CFLAGS) -std=c99 -I@top_srcdir@/include
endif
