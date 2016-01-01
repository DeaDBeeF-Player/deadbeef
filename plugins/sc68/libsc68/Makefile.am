# ,----------------------------------------------------------------------.
# |                                                                      |
# | libsc68 -- Makefile.am -- Use with automake to produce configure.in  |
# |                                                                      |
# |               Copyright (C) 2007-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS         = io68 emu68 dial68 $(subdirs)
ACLOCAL_AMFLAGS = -I m4 --install

AM_CFLAGS       = $(ALL_CFLAGS) $(LIB_CFLAGS)
AM_CPPFLAGS     = -DSC68_EXPORT=1 -I$(srcdir)/sc68 $(file68_inc)
AM_LDFLAGS      = $(ALL_LFLAGS) $(LIB_LFLAGS) -version-info 3:0:0

# ,----------------------------------------------------------------------.
# | pkg-config                                                           |
# `----------------------------------------------------------------------'

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = sc68.pc

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

# if SOURCE_FILE68
# FILE68_LA = $(file68_dir)/libfile68.la
# @SET_MAKE@
# $(file68_dir)/libfile68.la:
# 	cd "$(file68_dir)" && $(MAKE) $(AM_MAKEFLAGS) libfile68.la
# else
# FILE68_LA =
# endif

lib_LTLIBRARIES    = libsc68.la
libsc68_la_SOURCES = api68.c conf68.c mixer68.c libsc68.c
libsc68_la_LIBADD  = io68/libio68.la emu68/libemu68.la dial68/libdial68.la
if SOURCE_FILE68
libsc68_la_LIBADD += $(file68_dir)/libfile68.la
endif

# ,----------------------------------------------------------------------.
# | Installed headers                                                    |
# `----------------------------------------------------------------------'

noinst_HEADERS     = \
sc68/conf68.h sc68/mixer68.h sc68/trap68.h trapfunc.s

pkginclude_HEADERS = sc68/sc68.h
pkgincludedir      = $(includedir)/sc68

# ,----------------------------------------------------------------------.
# | Assemble and transform TOS trap emulator                             |
# `----------------------------------------------------------------------'

$(builddir)/api68.lo: $(srcdir)/sc68/trap68.h
trap68: $(srcdir)/sc68/trap68.h


bin2h=$(hexdump) -ve '/1 "%d,"'

$(srcdir)/sc68/trap68.h: $(srcdir)/trapfunc.s
	$(as68) $< -o trapfunc.bin >/dev/null
	$(bin2h) trapfunc.bin >$@
	@rm trapfunc.bin


MAINTAINERCLEANFILES = $(srcdir)/sc68/trap68.h
