if HAVE_VFS_CURL
pkglib_LTLIBRARIES = vfs_curl.la
vfs_curl_la_SOURCES = vfs_curl.c
vfs_curl_la_LDFLAGS = -module -avoid-version

vfs_curl_la_LIBADD = $(LDADD) $(VFS_CURL_LIBS)
AM_CFLAGS = $(CFLAGS) $(VFS_CURL_CFLAGS) -std=c99
endif
