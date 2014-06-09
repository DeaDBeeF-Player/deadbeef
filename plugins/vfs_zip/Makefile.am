if HAVE_VFS_ZIP
pkglib_LTLIBRARIES = vfs_zip.la
vfs_zip_la_SOURCES = vfs_zip.c

vfs_zip_la_LDFLAGS = -module -avoid-version

vfs_zip_la_LIBADD = $(LDADD) $(ZLIB_LIBS) $(ZIP_LIBS)
AM_CFLAGS = $(CFLAGS) $(ZLIB_CFLAGS) $(ZIP_CFLAGS) -std=c99
endif
