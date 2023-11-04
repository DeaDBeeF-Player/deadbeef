# ,----------------------------------------------------------------------.
# |                                                                      |
# |   io68 -- Makefile.am -- Use with automake to produce configure.in   |
# |                                                                      |
# |               Copyright (C) 1998-2015 Benjamin Gerard                |
# |                                                                      |
# |               <http://sourceforge.net/users/benjihan>                |
# |                                                                      |
# `----------------------------------------------------------------------'
# @configure_input@

ACLOCAL_AMFLAGS    = -I ../m4 --install
AM_CFLAGS          = $(ALL_CFLAGS) $(LIB_CFLAGS)
AM_CPPFLAGS        = -I$(top_srcdir) $(file68_inc) -DIO68_EXPORT

mysources= io68.c mfp_io.c mfpemul.c mw_io.c mwemul.c paula_io.c	\
 paulaemul.c shifter_io.c ym_envel.c ym_blep.c ym_dump.c ym_io.c	\
 ym_puls.c ymemul.c

myheaders= default.h io68.h io68_api.h mfp_io.h mfpemul.h mw_io.h	\
 mwemul.h paula_io.h paulaemul.h shifter_io.h ym_blep.h ym_dump.h	\
 ym_fixed_vol.h ym_io.h ym_puls.h ymemul.h ymoutorg.h ymout2k9.h

srcextra=\
 ym_atarist_table.c ym_linear_table.c

# ,----------------------------------------------------------------------.
# | Libtool library                                                      |
# `----------------------------------------------------------------------'

noinst_LTLIBRARIES  = libio68.la
libio68_la_SOURCES  = $(mysources) $(myheaders)
libio68_la_LDFLAGS  = -avoid-version
noinst_HEADERS      = $(srcextra)
