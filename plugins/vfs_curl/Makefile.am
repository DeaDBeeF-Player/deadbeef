if HAVE_VFS_CURL
vfs_curldir = $(libdir)/$(PACKAGE)
pkglib_LTLIBRARIES = vfs_curl.la
vfs_curl_la_SOURCES = vfs_curl.c
vfs_curl_la_LDFLAGS = -module

vfs_curl_la_LIBADD = $(LDADD) $(VFS_CURL_LIBS)
AM_CFLAGS = $(CFLAGS) -std=c99
endif
