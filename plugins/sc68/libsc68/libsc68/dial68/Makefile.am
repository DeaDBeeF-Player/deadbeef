# ,----------------------------------------------------------------------.
# |                                                                      |
# |  dial68 -- Makefile.am -- Use with automake to produce Makefile.in   | 
# |                                                                      |
# |               Copyright (C) 2007-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

SUBDIRS         = $(winui)
ACLOCAL_AMFLAGS = -I m4 --install
AM_CFLAGS       = $(ALL_CFLAGS) $(LIB_CFLAGS)
AM_CPPFLAGS     = -I$(top_srcdir) $(file68_inc)

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

if use_winui
winui                = winui
dialsub              = winui/libwinui.la
else
winui=
dialsub=
endif

noinst_LTLIBRARIES   = libdial68.la
libdial68_la_SOURCES = dial68.c dial_conf.c dial_tsel.c dial_finf.c
libdial68_la_LDFLAGS = -avoid-version
libdial68_la_LIBADD  = $(dialsub)
noinst_HEADERS       = dial68.h
