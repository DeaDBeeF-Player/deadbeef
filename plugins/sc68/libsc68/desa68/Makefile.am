# ,----------------------------------------------------------------------.
# |                                                                      |
# |  desa68 -- Makefile.am -- Use with automake to produce Makefile.in   | 
# |                                                                      |
# |               Copyright (C) 2007-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

ACLOCAL_AMFLAGS       = -I m4 --install
AM_CPPFLAGS           = -DDESA68_EXPORT=1
AM_CFLAGS             = $(ALL_CFLAGS) $(LIB_CFLAGS)
AM_LDFLAGS            = $(ALL_LFLAGS) $(LIB_LFLAGS) $(LFLAGS)
LFLAGS                = -avoid-version $(NOINSTALL)

# ,----------------------------------------------------------------------.
# | pkg-config                                                           |
# `----------------------------------------------------------------------'

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = desa68.pc

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

lib_LTLIBRARIES       = libdesa68.la
libdesa68_la_SOURCES  = desa68.c

# ,----------------------------------------------------------------------.
# | Installed headers                                                    |
# `----------------------------------------------------------------------'

include_HEADERS       = desa68.h
