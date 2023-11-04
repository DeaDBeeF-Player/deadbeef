# ,----------------------------------------------------------------------.
# |                                                                      |
# |  file68 -- Makefile.am -- Use with automake to produce configure.in  |
# |                                                                      |
# |               Copyright (C) 2007-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS          = data68 $(subdirs)
ACLOCAL_AMFLAGS  = -I m4 --install
AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS       = $(ALL_CFLAGS) $(LIB_CFLAGS)
AM_LDFLAGS      = $(ALL_LFLAGS) $(LIB_LFLAGS) -version-info 3:0:0
AM_CPPFLAGS     = -DFILE68_EXPORT=1 -I$(top_srcdir)/sc68 $(unice68_inc)

null=

if REPLAY68
replay68_c=src/replay68.c
else
replay68_c=
endif

mysources = src/error68.c src/file68.c src/gzip68.c src/ice68.c		\
 src/init68.c src/vfs68.c src/vfs68_ao.c src/vfs68_curl.c		\
 src/vfs68_fd.c src/vfs68_file.c src/vfs68_mem.c src/vfs68_null.c	\
 src/vfs68_z.c src/msg68.c src/option68.c src/registry68.c		\
 src/rsc68.c src/string68.c src/timedb68.c src/uri68.c $(replay68_c)	\
 $(null)

apiheaders = sc68/file68_chk.h sc68/file68_ord.h sc68/file68_err.h	\
 sc68/file68.h sc68/file68_api.h sc68/file68_features.h			\
 sc68/file68_vfs.h sc68/file68_vfs_def.h sc68/file68_msg.h		\
 sc68/file68_opt.h sc68/file68_reg.h sc68/file68_rsc.h			\
 sc68/file68_str.h sc68/file68_tag.h sc68/file68_uri.h			\
 sc68/file68_tdb.h $(null)

otherheaders = sc68/file68_zip.h sc68/file68_ice.h			\
 sc68/file68_vfs_ao.h sc68/file68_vfs_curl.h sc68/file68_vfs_fd.h	\
 sc68/file68_vfs_file.h sc68/file68_vfs_mem.h sc68/file68_vfs_null.h	\
 sc68/file68_vfs_z.h src/timedb.inc.h src/replay.inc.h $(null)

myheaders = $(apiheaders) $(otherheaders)

# ,----------------------------------------------------------------------.
# | pkg-config                                                           |
# `----------------------------------------------------------------------'

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = file68.pc

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

if SOURCE_UNICE68
UNICE68_LA           = $(unice68_dir)/libunice68.la
@SET_MAKE@
$(unice68_dir)/libunice68.la:
	$(MAKE) -C $(unice68_dir) $(AM_MAKEFLAGS) libunice68.la
else
UNICE68_LA           =
endif

lib_LTLIBRARIES      = libfile68.la
libfile68_la_SOURCES = $(mysources) $(myheaders)
libfile68_la_LIBADD  = $(UNICE68_LA)

# ,----------------------------------------------------------------------.
# | Install headers                                                      |
# `----------------------------------------------------------------------'

pkginclude_HEADERS   = $(apiheaders)
pkgincludedir        = $(includedir)/sc68
